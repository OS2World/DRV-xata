/**************************************************************************
 *
 * SOURCE FILE NAME = S506SII.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 2002-2007
 *
 * DESCRIPTION : Adapter Driver Silicon Image routines.
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

#define PCIDEV_SII680  0x0680
#define PCIDEV_SII3112 0x3112
#define PCIDEV_SII3114 0x3114

#define PCI_SII_IDEMODE 	0x80	    /* SII IDE mode	       +4    */
#define PCI_SII_IDESCSC 	0x8A	    /* SII IDE sys control status    */
#define PCI_SII_IDECFG0 	0xA0	    /* SII IDE config & status ch0   */
#define PCI_SII_IDECFG1 	0xB0	    /* SII IDE config & status ch1   */
#define PCI_SII_IDETFTIM	0xA2	    /* SII IDE taskfile timing +16   */
#define PCI_SII_IDETIM		0xA4	    /* SII IDE PIO timing      +16   */
#define PCI_SII_IDEMTIM 	0xA8	    /* SII IDE MWDMA timing    +16   */
#define PCI_SII_IDEUTIM 	0xAC	    /* SII IDE Ultra timing    +16   */

/*----------------------------------------------------------------*/
/* Defines for SII_level	     PCI Config Device IDs	  */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/
#define SII		    0	/* SII with Ultra133	       */
#define SII_SATA0	    1	/* SII SATA 3112 compatible    */
#define SII_SATA1	    2	/* SII SATA 3114 compatible    */
#define SII_SATAIXP	    3	/* ATI SATA IXP compatible     */

/* remap some structure members */

#define SII_IDEtim   PIIX_IDEtim
#define SII_Mode     PIIX_SIDEtim
#define SII_MWDMAtim DMAtim[0]
#define SII_Ultratim DMAtim[1]

#define PciInfo (&npA->PCIInfo)
#define PCIAddr PciInfo->PCIAddr

VOID NEAR SiISATA (NPA npA)
{
  static USHORT PortOffsetTF[4] = {  0x80,  0xC0, 0x280, 0x2C0 };
  static USHORT PortOffsetBM[4] = {  0x00,  0x08, 0x200, 0x208 };
  static USHORT PortOffsetSC[4] = { 0x100, 0x180, 0x300, 0x380 };
  UCHAR  DataB;
  USHORT DataW;
  NPU	 npU = npA->UnitCB;
  NPC	 npC = npA->npC;
  ULONG  BA5 = npC->BAR[5].Addr;

  GenericSATA (npA);
  npA->SCR.Offsets = 0x021;
  SSTATUS = BA5 | PortOffsetSC[npA->IDEChannel];

  if (npA->FlagsT & ATBF_BIOSDEFAULTS) {  // use generic PIO mode
    npA->Cap |= CHIPCAP_ATAPIDMA;
    if (PciInfo->CompatibleID == PCIDEV_SII3114) {
      npA->maxUnits = 2;
      npU[1].SStatus = BA5 + PortOffsetSC[npA->IDEChannel + 2];
      npU[1].FlagsT |= UTBF_NOTUNLOCKHPA;
    }
    npA->FlagsT &= ~ATBF_BIOSDEFAULTS;

  } else {				 // use MMIO

    MEMBER(npA).CfgTable   = CfgNull;
    METHOD(npA).GetPIOMode = GetSIISATAPio;
    METHOD(npA).Setup	   = SetupCommon;
    METHOD(npA).ProgramChip= ProgramSIISATAChip;
    METHOD(npA).CheckIRQ   = SIICheckIRQ;

    if (PciInfo->Level == SII_SATA0)
      METHOD(npA).StartStop = SIIStartStop;

    DATAREG   = BA5 | PortOffsetTF[npA->IDEChannel];
    DEVCTLREG = DATAREG + 10;
    BMCMDREG  = BA5 | PortOffsetBM[npA->IDEChannel];
    CollectPorts (npA);
    BMCMDREG	|= 0x10; // enable large PRD mode, thus don't touch Sii3114 "magic bit"
    BMSTATUSREG |= 0x10;

    if (PciInfo->CompatibleID == PCIDEV_SII3114) {
      // magic "4 ports" bit
      if (npA->IDEChannel == 0) OutB (BA5 | 0x200, 0x02);
      npC->numChannels = 4;
    }

    DataB = (GetRegB (PCIAddr, PCIREG_CACHE_LINE) / 8) + 1;
    DataW = (npA->IDEChannel * 0x104) & 0x204;
    OutB (BA5 | 0x40 | DataW, DataB);
    OutB (BA5 | 0x41 | DataW, DataB);
    OutB (DATAREG | 0x21, 0x31);
    OutD (SSTATUS | 0x4C, 0x10401554);
    OutD (SSTATUS | 0x48, 0);  // disable SATA interrupts
    OutD (SERROR, InD (SERROR));
    if (npA->IDEChannel == (npC->numChannels - 1))
      OutW (BA5 | 0x4A, 0); // unmask interrupts
  }
}

BOOL NEAR AcceptSII (NPA npA)
{
  sprntf (npA->PCIDeviceMsg, SII68xMsgtxt, MEMBER(npA).Device);

  switch (PciInfo->CompatibleID) {
    case PCIDEV_SII680:
     // enable 2*PCIClock operation
      SetRegB (PCIAddr, PCI_SII_IDESCSC, 0x20);

      npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;
      if ((3 * (0x30 & GetRegB (PCIAddr, PCI_SII_IDESCSC))) & 0x20) // 0x10 or 0x20
	npA->Cap |= CHIPCAP_ATA133;

      /* determine the presence of a 80wire cable as per defined procedure */
      if (1 & GetRegB (PCIAddr, (UCHAR)(npA->IDEChannel ? PCI_SII_IDECFG1
							: PCI_SII_IDECFG0)))
	npA->Cap |= CHANCAP_CABLE80;
      break;

    case PCIDEV_SII3112:
    case PCIDEV_SII3114: SiISATA (npA); break;
  }
  return (TRUE);
}

BOOL NEAR AcceptIXPSATA (NPA npA)
{
  PciInfo->Level = 3;
  npA->FlagsT	|= ATBF_BIOSDEFAULTS;
  SiISATA (npA);
  return (TRUE);
}


USHORT NEAR GetSIIPio (NPA npA, UCHAR Unit) {
  USHORT Timing;

  Timing = ReadConfigW (PciInfo, (UCHAR)(PCI_SII_IDETIM + 2 * Unit + 0x10 * npA->IDEChannel));
  return ((Timing & 0x3F) + ((Timing >> 6) & 0x3F));
}

USHORT NEAR GetSIISATAPio (NPA npA, UCHAR Unit) {
  NPU npU = npA->UnitCB + Unit;

//  if (npU->SATAGeneration == 0) {
  if (npU->Features & 0x100) {
    npU->MaxXSectors = 15; // SiI3112 bug
    npU->SecPerBlk   = 8;  // SiI3112 bug
  }

  METHOD(npA).SetupTF	= SIISetupTF;
  METHOD(npA).StartDMA	= SIIStartOp;
  return (GetGenericPio (npA, Unit));
}

#define DFLT_PIO  0x328A328Aul
#define DFLT_UDMA 0x40094009ul

/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID SIITimingValue (NPU npU)
{
  NPA	npA  = npU->npA;
  UCHAR Unit = npU->UnitId;
  UCHAR Time, Mode;

#define BYTE(x) ((NPCH)&(npA->x))
#define WORD(x) ((NPUSHORT)&(npA->x))

  Time = npU->CurPIOMode;
  Time = (Time >= 3) ? Time - 2 : 0;
  WORD(SII_IDEtim)[Unit] = SII_ModeSets[Time] ^ (USHORT)DFLT_PIO;
  Mode = 1;

  Time = npU->CurDMAMode;
  if (Time > 0) {
    WORD(SII_MWDMAtim)[Unit] = SII_MDMAModeSets[Time-1] ^ (USHORT)DFLT_PIO;
    Mode = 2;
  }

  Time = npU->UltraDMAMode;
  if (Time > 0) {
    WORD(SII_Ultratim)[Unit] = (UCHAR)DFLT_UDMA ^
      (0x7F & ((npA->Cap & CHIPCAP_ATA133 ? SIS_Ultra133ModeSets : SIS_Ultra100ModeSets)[Time-1]));
    Mode = 3;
  }
  BYTE(SII_Mode)[0] |= Mode << (4 * Unit);
}

/*-------------------------------*/
/*				 */
/* ProgramSIIChip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramSIIChip (NPA npA)
{
  /*************************************/
  /* Write IDE timing Register	       */
  /*************************************/

  if (BYTE(SII_Mode)[0]) {
    WConfigB ((UCHAR)(npA->IDEChannel ? PCI_SII_IDEMODE + 4 : PCI_SII_IDEMODE), BYTE(SII_Mode)[0]);

    RegOffset = npA->IDEChannel ? 0x10 : 0;

    WConfigW (PCI_SII_IDETFTIM, (USHORT)DFLT_PIO);
    WConfigD (PCI_SII_IDETIM  , npA->SII_IDEtim   ^ DFLT_PIO);
    WConfigD (PCI_SII_IDEMTIM , npA->SII_MWDMAtim ^ DFLT_PIO);
    WConfigD (PCI_SII_IDEUTIM , npA->SII_Ultratim ^ DFLT_UDMA);
  }
}

VOID ProgramSIISATAChip (NPA npA)
{
  UCHAR newVal = BYTE(SII_Mode)[0];

  if (newVal) {
#if 0
    UCHAR oldVal = InB (DATAREG | 0x34);
    if ((newVal & ~oldVal) & 0x22)
      OutB (DATAREG | 0x34, (UCHAR)((newVal & 0x22) | (oldVal & 0x11))); // set data transfer mode
#else
      OutW (DATAREG | 0x34, newVal); // set data transfer mode
#endif
  }
}

VOID SIISetupTF (NPA npA)
{
  UCHAR IOMask;
  NPCH	p;

  IOMask = npA->IOPendingMaskSave = npA->IOPendingMask;
  p = npA->IORegs;
  for (IOMask = ~IOMask; IOMask; IOMask >>= 1) {
    if (IOMask & 1) *p = 0;
    p++;
  }
  DRVHD &= 0x4F;
  npA->IOPendingMask = 0;
}

VOID NEAR SIIStartOp (NPA npA)
{
  if (COMMAND == FX_SETMAXEXT) {
    OutB (COMMANDREG, COMMAND);
    return;
  }
  if (npA->IOPendingMaskSave & FM_HIGH) {  // LBA48 addressing
    OutB (DRVHDREG, (UCHAR)(DRVHD & ~0x0F));
    OutB (FEATREG, FEAT);
    OutD (DATAREG | 0x10, *(ULONG *)(npA->IORegs));
    OutD (DATAREG | 0x18, *(ULONG *)(&LBA3 - 1) & 0xFFFFFF00);
    OutW (DATAREG | 0x14, *(USHORT *)&LBA1);
    OutB (DATAREG | 0x17, COMMAND);
  } else {
    OutD (DATAREG | 0x10, *(ULONG *)(npA->IORegs));
    OutD (DATAREG | 0x14, *(ULONG *)&LBA1);
  }
  if (npA->BM_CommandCode & BMICOM_START)
    OutB (BMCMDREG, npA->BM_CommandCode); /* Start BM controller */
}

VOID NEAR SIIStartStop (NPA npA, UCHAR State)
{
  NPC	 npC  = npA->npC;
  ULONG  Port = 0x50 | npC->BAR[5].Addr;
  USHORT Leds;

  if (State)
    npC->HWSpecial |=  (1 << npA->IDEChannel);
  else
    npC->HWSpecial &= ~(1 << npA->IDEChannel);

  Leds = (npC->HWSpecial & 0x0F) << 1;
  Leds |= Leds != 0;
  OutW (Port | 4, 0xFF & ~Leds);
  OutD (Port, Leds ? 0x0201FC00 : 0x0301FC00);
}

#if 0
int NEAR SIICheckIRQ (NPA npA) {
  DISABLE
  BMSTATUS = InB (BMSTATUSREG);
  if (BMSTATUS & BMISTA_INTERRUPT) {	      // interrupt is signalled pending
    UCHAR Data;

    OutB (BMSTATUSREG, (UCHAR)(BMSTATUS & BMISTA_MASK));
    if ((Data = InB (DATAREG | 0x21)) & 0x08) {
      if (Data & 0x10) OutB (DATAREG | 0x21, Data); // Watchdog!
//	if (npA->BM_CommandCode & BMICOM_START)
	OutB (BMCMDREG, npA->BM_CommandCode &= ~BMICOM_START); /* turn OFF Start bit */
      STATUS = InB (STATUSREG);
      npA->Flags |= ACBF_BMINT_SEEN;
      ENABLE
      return (1);
    } else {
      npA->npU->DeviceCounters.TotalBMStatus2++;
      ENABLE
      return (0);
    }
  } else {
    ENABLE
    return (0);
  }
}
#else

int NEAR SIICheckIRQ (NPA npA) {
  union {
    ULONG Status2;
    struct {
      UCHAR Cmd;
      UCHAR filler1;
      UCHAR Status;
      UCHAR filler2;
    };
  } BM;

  DISABLE
  BM.Status2 = InD (BMCMDREG);
  BMSTATUS = BM.Status;
  if ((BMSTATUS & BMISTA_INTERRUPT) || (BM.Cmd & 0x10)) { // interrupt is signalled pending
    UCHAR Data;

    if (BM.Cmd & 0x10) {
      NPU npU = npA->UnitCB + 0;
      OutD (SERROR, InD (SERROR));
    }

    BM.Status &= BMISTA_MASK;
    BM.Cmd     = (BM.Cmd & ~BMICOM_START) | 0x10;
    BM.filler2 = 0;
BM.filler1 |= 0x3F;
    OutD (BMCMDREG, BM.Status2);
//	if (Data & 0x10) OutB (DATAREG | 0x21, Data); // Watchdog!
    if (BMSTATUS & BMISTA_INTERRUPT) {
      STATUS = InB (STATUSREG);
      npA->Flags |= ACBF_BMINT_SEEN;
    }
    ENABLE
    return (1);
  } else {
    ENABLE
    return (0);
  }
}

#endif

