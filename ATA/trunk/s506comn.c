/**************************************************************************
 *
 * SOURCE FILE NAME = S506COMN.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2006
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

/*--------------------------------------------*/
/* SetupCommonPre()			      */
/*--------------------------------------------*/
USHORT SetupCommonPre (NPA npA)
{
  CHAR	 Unit;
  USHORT Cap;
  NPU	 npU;
  UCHAR  MinIFTiming, MinUDMA;

#define Restrictions (npA->PCIInfo.Ident.TModes)

  /*
   ** If there were no units attached, then the ACB has been
   ** deallocated, so do no configuration on this IDE channel.
   */

  if (!npA->cUnits) return 1;

  npA->PIIX_IDEtim  = 0;   /* clear timing value initially */
  npA->PIIX_SIDEtim = 0;   /* clear timing value initially */
  npA->DMAtim[0]    = 0;   /* clear timing value initially */
  npA->DMAtim[1]    = 0;   /* clear timing value initially */

  if (npA->Cap & CHIPCAP_SATA) npA->Cap |= CHANCAP_CABLE80 | CHANCAP_SPEED;
  if (npA->Cap & CHANCAP_CABLE80) npA->FlagsT |= ATBF_80WIRE;
  Cap = npA->Cap;
  if (!(npA->FlagsT & ATBF_80WIRE))
    Cap &= ~CHANCAP_SPEED;

  MinIFTiming = 2;
  MinUDMA = 15;

  for (npU = npA->UnitCB, Unit = npA->cUnits; --Unit >= 0; npU++) {
    if (!(Cap & CHIPCAP_ULTRAATA)) npU->UltraDMAMode = 0;

    if (!(Cap & CHIPCAP_ATA66) && (npU->UltraDMAMode > ULTRADMAMODE_2))
      npU->UltraDMAMode = ULTRADMAMODE_2;

    if (!(Cap & CHIPCAP_ATA100) && (npU->UltraDMAMode > ULTRADMAMODE_4))
      npU->UltraDMAMode = ULTRADMAMODE_4;

    if (!(Cap & CHIPCAP_ATA133) && (npU->UltraDMAMode > ULTRADMAMODE_5))
      npU->UltraDMAMode = ULTRADMAMODE_5;

TSTR("UF:%lX,O:%X", npU->Flags, npU->FlagsT);

    if (npU->FlagsT & UTBF_NOT_BM) npU->Flags &= ~UCBF_BM_DMA;

    if (npU->Flags & UCBF_ATAPIDEVICE) {
      npA->FlagsT |= ATBF_ATAPIPRESENT;
      if ((!npU->ATAVersion || (npA->HardwareType == generic))
	 && !(npU->FlagsT & UTBF_SET_BM))
	npU->Flags &= ~UCBF_BM_DMA;

      if (npA->Cap & CHIPCAP_ATAPIPIO32)  npU->Flags |= UCBF_PIO32;
      if (!(npA->Cap & CHIPCAP_ATAPIDMA)) npU->Flags &= ~UCBF_BM_DMA;
    } else {
      if (npA->Cap & CHIPCAP_ATAPIO32)	npU->Flags |= UCBF_PIO32;
      if (!(npA->Cap & CHIPCAP_ATADMA)) npU->Flags &= ~UCBF_BM_DMA;
    }

    if (npU->Features & 0x2000) npU->Flags &= ~UCBF_PIO32;

    npU->InterfaceTiming = (npU->CurPIOMode >= 3) ? npU->CurPIOMode - 2 : 0;

    if (!(npU->UltraDMAMode | npU->CurDMAMode))
      npU->Flags &= ~UCBF_BM_DMA;

    if (npU->Flags & UCBF_BM_DMA) {
      if (npU->CurDMAMode && !npU->UltraDMAMode) {
	if ((npU->InterfaceTiming == 0) && (Restrictions & TR_PIO0_WITH_DMA)) {
	  npU->InterfaceTiming = npU->CurDMAMode;
	} else if (Restrictions & TR_PIO_EQ_DMA) {
	  if (npU->InterfaceTiming > npU->CurDMAMode)
	    npU->InterfaceTiming = npU->CurDMAMode;
	}
      } else {
	if (MinUDMA > npU->UltraDMAMode) MinUDMA = npU->UltraDMAMode;
      }
    } else {
      npU->CurDMAMode = npU->UltraDMAMode = 0;
    }

    if (MinIFTiming > npU->InterfaceTiming)
      MinIFTiming = npU->InterfaceTiming;

TS("F:%lX,", npU->Flags)
  }

  npA->FlagsT &= ~ATBF_BM_DMA;

  for (npU = npA->UnitCB, Unit = npA->cUnits; --Unit >= 0; npU++) {

    if (Restrictions & TR_PIO_SHARED) npU->InterfaceTiming = MinIFTiming;
    if ((npU->Flags & UCBF_BM_DMA) && npU->CurDMAMode && !npU->UltraDMAMode) {
      npU->CurDMAMode = npU->InterfaceTiming;
      if (!npU->CurDMAMode) npU->Flags &= ~UCBF_BM_DMA;
    }
    if (npU->InterfaceTiming == 0) {
      npU->CurPIOMode = 0;
    } else {
      if (npU->CurPIOMode || !(Restrictions & TR_PIO0_WITH_DMA))
	npU->CurPIOMode = npU->InterfaceTiming + 2;
    }
    if (npU->UltraDMAMode && (Restrictions & TR_UDMA_SHARED))
      npU->UltraDMAMode = MinUDMA;

    if (npU->UltraDMAMode >= ULTRADMAMODE_3) npA->Cap |= CHANREQ_ATA66;
    if (npU->UltraDMAMode >= ULTRADMAMODE_5) npA->Cap |= CHANREQ_ATA100;
    if (npU->UltraDMAMode >= ULTRADMAMODE_6) npA->Cap |= CHANREQ_ATA133;

    if (npU->Flags & UCBF_BM_DMA) npA->FlagsT |= ATBF_BM_DMA;
  }

#if TRACES
  if (Debug & 2) {
    TraceStr ("Cbl:%02X,Lvl:%X,Cap:%04X,", npA->FlagsT & ATBF_80WIRE, npA->PCIInfo.Level, npA->Cap);
  }
#endif

  CurPCIInfo = &npA->PCIInfo;
  CurLevel   = npA->PCIInfo.Level;
  RegOffset  = 0;

  return 0;
}


/*--------------------------------------------*/
/* SetupCommonPost()			      */
/*--------------------------------------------*/
VOID SetupCommonPost (NPA npA)
{
  UCHAR Data, Mask;
  NPU	npU;

  RegOffset = 0;

  if (npA->FlagsT & ATBF_BM_DMA) {
    PciGetReg (CurPCIInfo->PCIAddr, PCIREG_COMMAND, (PULONG)&Data, 1);
    if (!(Data & PCI_CMD_BM))
      WConfigBI (PCIREG_COMMAND, (UCHAR)(Data | PCI_CMD_BM));
  }

  /******************************************/
  /* Configure each unit to proper transfer */
  /* mode for PIO and/or DMA		    */
  /******************************************/

  Data = 0;
  Mask = BMISTA_D0DMA;

  for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->cUnits); npU++, Mask <<= 1) {
    if (npU->Flags & UCBF_NOTPRESENT) continue;

    if (!(npA->FlagsT & ATBF_BIOSDEFAULTS)) {
      if (npU->UltraDMAMode) npU->CurDMAMode = npU->UltraDMAMode + 2;
      if (npU->Flags & UCBF_BM_DMA) Data |= Mask;
    }

    SelectUnit (npU);  /* select unit */

    if ((npU->Status == UTS_OK) && !CheckBusy (npA)) { /* check ready status */
      if (npU->AMLevel != 0) IssueSetFeatures (npU, FX_SETAMLVL, (UCHAR)(0x80 + ~(npU->AMLevel)));
    }
  }

  /*************************/
  /* Set DMA Capable Bits  */
  /*************************/

  if (BMSTATUSREG > 0x100) {
    OutB (BMSTATUSREG, Data);
    npA->BMStatus = Data | BMISTA_INTERRUPT;
  }
}


/*--------------------------------------------*/
/*					      */
/* SetupCommon()			      */
/*					      */
/*					      */
/*--------------------------------------------*/
VOID NEAR SetupCommon (NPA npA)
{
  if (SetupCommonPre (npA)) return;
  if (CurPCIInfo->Ident.CalculateTiming) CurPCIInfo->Ident.CalculateTiming (npA);
  if (CurPCIInfo->Ident.ProgramChip)	 CurPCIInfo->Ident.ProgramChip (npA);
  SetupCommonPost (npA);
}

VOID NEAR PickClockIndex (void)
{
  /**********************************/
  /* Pick correct clock index value */
  /**********************************/

  if (PCIClock <= 25) {
    PCIClockIndex = 1;
    PCIClock = 25;
  } else if (PCIClock > 33) {
    if (PCIClock <= 38) {
      PCIClockIndex = 2;
      PCIClock = 37;
    } else if (PCIClock <= 42) {
      PCIClockIndex = 3;
      PCIClock = 41;
    } else if (PCIClock <= 51) {
      PCIClockIndex = 4;
      PCIClock = 50;
    } else {
      PCIClockIndex = 5;
      PCIClock = 66;
    }
  } else {
    PCIClockIndex = 0;
    PCIClock = 33;
  }
}

/*--------------------------------------------*/
/* CalculateAdapterTiming		      */
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

VOID CalculateAdapterTiming (NPA npA)
{
  NPU npU;

  PickClockIndex();

  for (npU = npA->UnitCB; npU < (npA->UnitCB + MAX_UNITS); npU++) { /* For each attached unit */
    if (npU->Flags & UCBF_NOTPRESENT) continue;

    /*********************************/
    /* Choose timing register value  */
    /*********************************/
    if (METHOD(npA).TimingValue) METHOD(npA).TimingValue (npU);

    TSTR ("I%d/P%d/D%d/U%d/F:%lX,", npU->InterfaceTiming, npU->CurPIOMode, npU->CurDMAMode, npU->UltraDMAMode, npU->Flags);
  }
}
