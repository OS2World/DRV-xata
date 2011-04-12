/**************************************************************************
 *
 * SOURCE FILE NAME = S506GNRC.C
 * $Id$
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : Adapter Driver generic busmaster routines.
 *
 ****************************************************************************/

#define INCL_NOPMAPI
#define INCL_DOSINFOSEG
#define INCL_NO_SCB
#define INCL_INITRP_ONLY
#define INCL_DOSERRORS
#include "os2.h"
#include "dskinit.h"

#include "devhdr.h"
#include "iorb.h"
#include "reqpkt.h"
#include "dhcalls.h"
#include "addcalls.h"
#include "devclass.h"

#include "s506cons.h"
#include "s506type.h"
#include "s506regs.h"
#include "s506ext.h"
#include "s506pro.h"

#include "Trace.h"

#pragma optimize(OPTIMIZE, on)

#define PciInfo (&npA->PCIInfo)
#define PAdr PciInfo->PCIAddr

BOOL NEAR AcceptGeneric (NPA npA)
{
  if ((GenericBM > 0) && npA->FlagsI.b.busmaster && (npA->BMICOM & 0xFFF0)) {
    USHORT Port;
    UCHAR  err = FALSE;

    Port = npA->BMICOM;
    outp (Port, 0);
    if (0 != inp (Port)) err = TRUE;

    Port += 4;
    outp (Port, 0x54);	// bits 0 and 1 are zero!
    if (0x54 != inp (Port)) err = TRUE;

    Port += 2;
    outp (Port, 0xAA);
    if (0xAA != inp (Port)) err = TRUE;

    if (err) {
      TTYWrite (2, "Generic: busmaster registers not available");
      npA->Cap &= ~(CHIPCAP_ATADMA | CHIPCAP_ATAPIDMA);
    } else {
      if (npA->FlagsI.b.native) METHOD(npA).CheckIRQ = BMCheckIRQ;
    }
  } else {
    npA->Cap &= ~(CHIPCAP_ATADMA | CHIPCAP_ATAPIDMA);
  }

  npA->Cap &= ~CHIPCAP_PIO32;
  npA->IRQDelay = 12; // delayed interrupt
  npA->FlagsT |= ATBF_BIOSDEFAULTS;
  return (TRUE);
}

// ---------------------------------------------------------------

BOOL NEAR AcceptNetCell (NPA npA)
{
  npA->Cap &= ~CHIPCAP_PIO32;
  npA->Cap &= ~CHIPCAP_ATAPIDMA;
  npA->Cap |= CHANCAP_SPEED | CHANCAP_CABLE80;
  npA->FlagsT |= ATBF_BIOSDEFAULTS;
  npA->maxUnits = 2;
  return (TRUE);
}

USHORT NEAR GetNetCellPio (NPA npA, UCHAR Unit) {
  NPU npU = &(npA->UnitCB[Unit]);

  if (npU->FoundUDMAMode >= 8) npA->Cap |= CHIPCAP_SATA;
  return (GetGenericPio (npA, Unit));
}

// ---------------------------------------------------------------

BOOL NEAR AcceptMarvell (NPA npA)
{
  NPC	npC  = npA->npC;
  ULONG BAR5 = npC->BAR[5].Addr;

  // AHCI mode enabled?
//  if (BAR5 && (InD (BAR5 | AHCI_GHC) & AHCI_GHC_AHCIENABLED))
//    OutD (BAR5 | AHCI_GHC, 0);

  npA->Cap	  &= ~CHIPCAP_PIO32;
  npA->maxUnits    = 2;
  npC->numChannels = 1;

  if (BAR5) {
    UCHAR  lastPort = InB (BAR5 | AHCI_CAP) & 0x1F;
    USHORT PortOfs  = lastPort * 0x80 + 0x100;
    ULONG  IRQen;

    if (!((InD (BAR5 | AHCI_PI) >> lastPort) & 1))
      return (FALSE); // port not implemented

    if (!(InD (BAR5 + PortOfs + 0x44)))
      return (FALSE); // not PATA port
  }

  if (!(InB (BMCMDREG + 1) & 1))
    npA->Cap |= CHANCAP_SPEED | CHANCAP_CABLE80;

  sprntf (npA->PCIDeviceMsg, MarvellMsgtxt, MEMBER(npA).Device);
  return (TRUE);
}

// ---------------------------------------------------------------

BOOL NEAR AcceptAHCI (NPA npA)
{
  NPC	npC	= npA->npC;
  ULONG BAR5	= npC->BAR[5].Addr;
  UCHAR havePhy = 0;

  // Fail if AHCI mode enabled
  if (BAR5 && (InD (BAR5 | AHCI_GHC) & AHCI_GHC_AHCIENABLED)) return (FALSE);

  npA->maxUnits = 2;
  npA->SCR.Offsets = 0x3120;
  npA->Cap |= CHIPCAP_SATA | CHIPCAP_ATAPIDMA;
  METHOD(npA).GetPIOMode      = GetGenericPio;
  METHOD(npA).Setup	      = SetupCommon;
  METHOD(npA).CalculateTiming = CalculateAdapterTiming;
  METHOD(npA).ProgramChip     = NULL;
  MEMBER(npA).CfgTable	      = CfgNull;

  if (npA->UnitCB[0].SStatus = GetAHCISCR (npA, npA->IDEChannel + 0))
    havePhy = 1;
  else
    npA->UnitCB[0].FlagsT |= UTBF_DISABLED;

  if (npA->UnitCB[1].SStatus = GetAHCISCR (npA, npA->IDEChannel + 2))
    havePhy |= 2;
  else
    npA->maxUnits = 1;

  return (havePhy);
}

// ---------------------------------------------------------------

USHORT NEAR GetGenericPio (NPA npA, UCHAR Unit) {
  NPU npU = &(npA->UnitCB[Unit]);

  switch (npU->BestPIOMode) {
    case 4: return (4); break;
    case 3: return (6); break;
    default: return (20); break;
  }
}

/*--------------------------------------------*/
/* CalculateGenericAdapterTiming	      */
/* ----------------------		      */
/*					      */
/* Arguments:				      */
/*	npA = nptr to Adapter Control Block   */
/*					      */
/* Actions:				      */
/*					      */
/*	Picks optimal settings for PIO/DMA    */
/*	transfer for the selected ACB.	      */
/*					      */
/* Returns:				      */
/*	Nothing 			      */
/*					      */
/*					      */
/*--------------------------------------------*/

VOID CalculateGenericAdapterTiming (NPA npA)
{
  NPU npU;

  for (npU = npA->UnitCB; npU < (npA->UnitCB + MAX_UNITS); npU++) { /* For each attached unit */
    if (npU->Flags & UCBF_NOTPRESENT) continue;

    npU->CurPIOMode = npU->FoundPIOMode;

    if ((npA->Cap & CHIPCAP_SATA) ||
       (BMSTATUSREG && (InB (BMSTATUSREG) & (npU == npA->UnitCB ? 0x20 : 0x40)))) {
      if (npU->CurDMAMode > 0)	 npU->CurPIOMode = npU->CurDMAMode + 2;
      if (npU->UltraDMAMode > 0) npU->CurDMAMode = 3 + (npU->UltraDMAMode - 1);
    } else {
      npU->CurDMAMode = npU->UltraDMAMode = 0;
      npU->Flags &= ~UCBF_BM_DMA;
    }
  }
}

/*--------------------------------------------*/
/* GenericInitComplete			      */
/*--------------------------------------------*/

VOID NEAR GenericInitComplete (NPA npA)
{
}

/*----------------------------------------------------*/
/* CheckIRQ					      */
/*						      */
/* return TRUE	if interrupt request is from us       */
/* return FALSE if interrupt request is *not* from us */
/*----------------------------------------------------*/

int NEAR NonsharedCheckIRQ (NPA npA) {
  IODly (npA->IRQDelayCount);

  if (npA->BM_CommandCode) {
    UCHAR Loop;

    for (Loop = 256; --Loop != 0;) {
      BMSTATUS = InB (BMSTATUSREG);
      if (BMSTATUS & BMISTA_INTERRUPT) {
	npA->Flags |= ACBF_BMINT_SEEN;
	OutB (BMCMDREG, npA->BM_CommandCode &= npA->BM_StopMask); /* turn OFF Start bit */
	OutB (BMSTATUSREG, (UCHAR)(BMSTATUS & BMISTA_MASK));
	if (!(BMSTATUS & BMISTA_ACTIVE)) {
	  npA->BM_CommandCode = 0;
	  goto getStatus;
	}
      }
      P4_REP_NOP
    }

    OutB (BMCMDREG, npA->BM_CommandCode &= npA->BM_StopMask); /* turn OFF Start bit */
    npA->npU->DeviceCounters.TotalChipStatus++;
  }

getStatus:
  STATUS = InB (STATUSREG);
  ENABLE

  if (STATUS & FX_BUSY) CheckBusy (npA);
  if (STATUS & FX_ERROR) ERROR = InB (ERRORREG);

  return (1);
}


int NEAR BMCheckIRQ (NPA npA) {
  IODly (npA->IRQDelayCount);
  DISABLE
  BMSTATUS = InB (BMSTATUSREG);
  if (BMSTATUS & BMISTA_INTERRUPT) {	      // interrupt is signalled pending
    OutB (BMCMDREG, npA->BM_CommandCode &= ~BMICOM_START); /* turn OFF Start bit */
    STATUS = InB (STATUSREG);	// status available after stopping BM !
    npA->Flags |= ACBF_BMINT_SEEN;
    npA->BM_CommandCode = 0;
    OutB (BMSTATUSREG, (UCHAR)(BMSTATUS & BMISTA_MASK));
    ENABLE

    if (STATUS & FX_BUSY) CheckBusy (npA);
    if (STATUS & FX_ERROR) ERROR = InB (ERRORREG);

    return (1);
  } else {
    ENABLE
    return (0);
  }
}

/*--------------------------------------------*/
/* GenericSetupTaskFile 		      */
/*--------------------------------------------*/

#define outpdelay(Port,Data) OutBd (Port,Data,npA->IODelayCount)

VOID NEAR GenericSetTF (NPA npA, USHORT IOMask)
{
  if (IOMask & FM_HIGH) {  // LBA48 addressing
    if (IOMask & FM_HFEAT  ) { outpdelay (FEATREG  , 0);    } // FEAT	H
    if (IOMask & FM_HSCNT  ) { outpdelay (SECCNTREG, 0);    } // SECCNT H
    if (IOMask & FM_LBA3   ) { outpdelay (LBA3REG,   LBA3); } // LBA3
    if (IOMask & FM_LBA4   ) { outpdelay (LBA4REG,   LBA4); } // LBA4
    if (IOMask & FM_LBA5   ) { outpdelay (LBA5REG,   LBA5); } // LBA5
  }
    if (IOMask & FM_PFEAT  ) { outpdelay (FEATREG  , FEAT); } // FEAT
    if (IOMask & FM_PSCNT  ) { outpdelay (SECCNTREG, SECCNT);} // SECCNT
    if (IOMask & FM_LBA0   ) { outpdelay (LBA0REG,   LBA0); } // LBA0
    if (IOMask & FM_LBA1   ) { outpdelay (LBA1REG,   LBA1); } // LBA1
    if (IOMask & FM_LBA2   ) { outpdelay (LBA2REG,   LBA2); } // LBA2
}

VOID NEAR GenericGetTF (NPA npA, USHORT IOMask)
{
  if (IOMask & (FM_LBA4 | FM_LBA5)) {  // LBA48 addressing
    OutB (DEVCTLREG, (UCHAR)(DEVCTL | FX_HOB));
    if (IOMask & FM_LBA3   ) { LBA3 = InB (LBA3REG); } // LBA3
    if (IOMask & FM_LBA4   ) { LBA4 = InB (LBA4REG); } // LBA4
    if (IOMask & FM_LBA5   ) { LBA5 = InB (LBA5REG); } // LBA5
    OutB (DEVCTLREG, DEVCTL);
  } else {
    if (IOMask & FM_LBA3   ) { LBA3 = InB (DRVHDREG) & 0x0F; } // LBA3
  }
  if (IOMask & FM_PFEAT  ) { FEAT   = InB (FEATREG); } // FEAT
  if (IOMask & FM_PSCNT  ) { SECCNT = InB (SECCNTREG);} // SECCNT
  if (IOMask & FM_LBA0	 ) { LBA0   = InB (LBA0REG); } // LBA0
  if (IOMask & FM_LBA1	 ) { LBA1   = InB (LBA1REG); } // LBA1
  if (IOMask & FM_LBA2	 ) { LBA2   = InB (LBA2REG); } // LBA2
}

/*--------------------------------------------*/
/* GenericStartOperation		      */
/*--------------------------------------------*/

VOID NEAR GenericStartOp (NPA npA)
{
  if (npA->BM_CommandCode & BMICOM_START) {
    OutB (BMCMDREG, (UCHAR)(npA->BM_CommandCode & ~BMICOM_START));
    OutB (COMMANDREG, COMMAND);
    OutB (BMCMDREG, npA->BM_CommandCode); /* Start BM controller */
  } else {
    OutB (COMMANDREG, COMMAND);
  }
}

/*---------------------------------------------*/
/* SetupDMAdefault			       */
/*---------------------------------------------*/

VOID NEAR GenericSetupDMA (NPA npA)
{
  OutB (BMSTATUSREG, (UCHAR)(BMISTA_ERROR |  /* clear Error Bit       */
		      BMISTA_INTERRUPT |     /* clear INTR flag       */
		      npA->BMStatus));

  OutD (npA->BMIDTP, npA->ppSGL);
}


/*--------------------------------------------*/
/* GenericStopDMA			      */
/*--------------------------------------------*/

VOID NEAR GenericStopDMA (NPA npA)
{
  if (npA->BM_CommandCode) {
    /*****************************************/
    /* Note: Important to write out correct  */
    /* direction bit per Intel		     */
    /* Dani: ALI needs to be stopped by zero!*/
    /*****************************************/

    OutB (BMCMDREG, (UCHAR)(npA->BM_CommandCode & npA->BM_StopMask)); /* turn OFF Start bit */
    npA->BM_CommandCode = 0;
  }
}

/*--------------------------------------------*/
/* GenericErrorDMA			      */
/*--------------------------------------------*/

VOID NEAR GenericErrorDMA (NPA npA)
{
  OutB (BMSTATUSREG, (UCHAR)(BMISTA_ERROR |   /* clear Error Bit       */
			     npA->BMStatus)); /* clear INTR flag       */
}

/*--------------------------------------------*/
/* GenericSATA				      */
/*--------------------------------------------*/

VOID NEAR GenericSATA (NPA npA)
{
  npA->Cap |= CHIPCAP_SATA;
  npA->Cap &= ~(CHIPCAP_ATAPIDMA);
  METHOD(npA).GetPIOMode = NULL;
  METHOD(npA).Setup	 = NULL;
  MEMBER(npA).CfgTable	 = CfgNull;
}

/*--------------------------------------------*/
/* GetAHCISCR				      */
/*--------------------------------------------*/

ULONG NEAR GetAHCISCR (NPA npA, USHORT Port)
{
  ULONG BAR5 = npA->npC->BAR[5].Addr;
  ULONG CAP;

  if (!BAR5 || (Port >= 32)) return 0;

  CAP = InD (BAR5 | AHCI_CAP);
  /* if possible, check physical presence of port */
  if (CAP && ~CAP) {
    UCHAR numPorts = CAP & 0x1F;
    ULONG PI = InD (BAR5 | AHCI_PI);

    if (Port > numPorts) return 0;
    if (PI && !((PI >> Port) & 1)) return 0;
  }

  return (BAR5 + (AHCI_PORTREG_OFFSET + AHCI_SSTS + (Port * AHCI_PORTREG_BYTES)));
}

