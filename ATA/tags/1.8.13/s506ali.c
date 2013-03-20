/**************************************************************************
 *
 * SOURCE FILE NAME = S506ALI.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : Adapter Driver ALI routines.
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

#define PCIV_ALI	    0x10B9
#define PCID_ALISOUTHBRIDGE 0x1533

#define PCI_ALI_IDEATA66DET	0x4A	    /* ALI IDE ATA-66 cable detect     */
#define PCI_ALI_IDEATA66EN	0x4B	    /* ALI IDE ATA-66 enables	       */
#define PCI_ALI_IDEATAxxEN	0x49	    /* ALI IDE ATA-xx enables	       */
#define PCI_ALI_IDECFG		0x4F	    /* ALI IDE ATA config	       */
#define PCI_ALI_IDEENABLE	0x50	    /* ALI IDE enables		       */
#define PCI_ALI_IDECDCTRL	0x53	    /* ALI IDE ATAPI control	       */
#define PCI_ALI_IDEFIFO 	0x54	    /* ALI IDE FIFO control (+1)       */
#define PCI_ALI_IDEASU		0x58	    /* ALI IDE address setup time (+4) */
#define PCI_ALI_IDECMDTIM	0x59	    /* ALI IDE command timing (+4)     */
#define PCI_ALI_IDETIM		0x5A	    /* ALI IDE pulse timing (+4)       */
#define PCI_ALI_IDEULTRA	0x56	    /* ALI IDE Ultra DMA timing (+1)   */

/*----------------------------------------------------------------*/
/* Defines for ALI_level	     PCI Config Device IDs	  */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/
#define NOALI		    0
#define ALI		    1	/* ALI w/o Ultra	       */
#define ALIA		    2	/* ALI 1543C-E with Ultra      */
#define ALIB		    3	/* ALI with Ultra	       */
#define ALIC		    4	/* ALI with ATA-66	       */
#define ALID		    5	/* ALI with ATA-100	       */
#define ALIE		    6	/* ALI with ATA-133	       */
#define ALIF		    7	/* ALI with ATA-133 RAID       */
#define ALIG		    8	/* ALI with SATA	       */

/* remap some structure members */

#define ALI_IDEtim   PIIX_IDEtim
#define ALI_Ultratim PIIX_SIDEtim

#define PciInfo (&npA->PCIInfo)
#define PCIAddr PciInfo->PCIAddr

BOOL NEAR AcceptALI (NPA npA)
{
  UCHAR Value, save_en, save_43, save_4D, save_cc;
  static UCHAR SCROfs[6] = { 0x60, 0xC0, 0x90, 0xA0, 0xD0, 0xE0 };

  sprntf (npA->PCIDeviceMsg, ALI5229Msgtxt, PciInfo->CompatibleID);

  Value = npA->IDEChannel;

  switch (PciInfo->CompatibleID) {
    case 0x5229:
      save_en = GetRegB (PCIAddr, PCI_ALI_IDEENABLE);
      if (!(save_en & 2))
	SetRegB (PCIAddr, PCI_ALI_IDEENABLE, (UCHAR)(save_en | 2)); /* enable channel readout */

      save_cc = GetRegB (PCIAddr, PCIREG_PROGIF);
      if (!(save_cc & 0x40)) {	    /* prog if is in compatibility mode */
	save_43 = GetRegB (PCIAddr, 0x43);
	save_4D = GetRegB (PCIAddr, 0x4D);

	SetRegB (PCIAddr, 0x43, 0x3F);
	SetRegB (PCIAddr, 0x4D, 0x00);
	SetRegB (PCIAddr, PCIREG_PROGIF, (UCHAR)(save_cc | 0x40));

	Value = GetRegB (PCIAddr, PCIREG_PROGIF);

	SetRegB (PCIAddr, PCIREG_PROGIF, save_cc);
	SetRegB (PCIAddr, 0x4D, save_4D);
	SetRegB (PCIAddr, 0x43, save_43);
      } else {
	Value = save_cc;
      }

      if (!(save_en & 2))
	SetRegB (PCIAddr, PCI_ALI_IDEENABLE, save_en);

      if (!(Value & 0x40))	  /* prog if is in compatibility mode */
	Value |= 0x20 | 0x10;	  /* assume both channels enabled     */

      if (!(Value & (0x20 >> npA->IDEChannel)) && !(npA->FlagsT & ATBF_BAY))
	return (FALSE);

      if (Value & (PCI_IDE_NATIVE_IF1 << (2 * npA->IDEChannel)))
	return (FALSE);
      break;

    case 0x5228:
      PciInfo->Level = ALIF;
      npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133 | CHIPCAP_LBA48DMA | CHANCAP_CABLE80;
      METHOD(npA).CheckIRQ = BMCheckIRQ;
      return (TRUE);
      break;

    case 0x5287: {
      NPC npC = npA->npC;

      npC->numChannels = 4;

      switch (npA->IDEChannel) {
	case 2:
	  DATAREG   = npC->BAR[0].Addr + 8;
	  DEVCTLREG = npC->BAR[1].Addr + 4;
	  break;
	case 3:
	  DATAREG   = npC->BAR[2].Addr + 8;
	  DEVCTLREG = npC->BAR[3].Addr + 4;
	  break;
      }
      BMCMDREG = npC->BAR[4].Addr + npA->IDEChannel * 8;
      CollectPorts (npA);
    }
    /* fall through */
    case 0x5289:
      Value += 2;
    /* fall through */
    case 0x5281: {
      NPU npU = npA->UnitCB;

      GenericSATA (npA);
      METHOD(npA).CheckIRQ = BMCheckIRQ;
      SSTATUS  = ((ULONG)PCICONFIG (SCROfs[Value]) << 16) | PCIAddr;
      SERROR   = SSTATUS | 0x40000;
      SCONTROL = SSTATUS | 0x80000;
      npA->SCR.Offsets = 0;  // do *not* allocate SCR ports by default method
#if 0
      SetRegB (PCIAddr, 0x5C, 0x01);
      SetRegB (PCIAddr, 0x5D, 0x31);
      SetRegB (PCIAddr, 0x5E, 0x31);

      if ((InD (SERROR) >> 16) & SDIAG_PHYRDY_CHANGE)
	DevHelp_ProcBlock ((ULONG)(PVOID)&AcceptALI, 500, 0);
#endif
      return (TRUE);
    }
  }

  npA->Cap &= ~CHIPCAP_LBA48DMA;

  if (MEMBER(npA).Revision > 0xC4) {
    PciInfo->Level = ALIE;
    npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133 | CHIPCAP_LBA48DMA;

  } else if (MEMBER(npA).Revision == 0xC4) {
    PciInfo->Level = ALID;
    npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;

  } else if (MEMBER(npA).Revision >= 0xC2) {
    PciInfo->Level = ALIC;
    npA->Cap |= CHIPCAP_ATA66;

  } else if (MEMBER(npA).Revision >= 0xC1) {
    PciInfo->Level = ALIB;
    npA->Cap &= ~CHIPCAP_ATAPIDMAWR;

    if ((MEMBER(npA).Revision == 0xC1) &&
	(xBridgeRevision == 0xC3) &&			   /* this is a 1543C */
	((0x1E & GetRegB (xBridgePCIAddr, 0x5E)) == 0x12)) /* This is a flaky 1543C-E */
      PciInfo->Level = ALIA;
  } else {
    if (MEMBER(npA).Revision >= 0x20) {
      PciInfo->Level = ALIB;
    } else {
      PciInfo->Level = ALI;
      npA->Cap &= ~CHIPCAP_ULTRAATA;
    }
    npA->Cap &= ~CHIPCAP_ATAPIDMA;
  }

  if (PciInfo->Level <= ALIB) npA->IRQDelay = 12;

  if (npA->Cap & CHIPCAP_ATA66) {
    UCHAR save, Val, Cable;

    /* determine the presence of a 80wire cable as per defined procedure */

    save = GetRegB (xBridgePCIAddr, 0x79);
    Val = (MEMBER(npA).Revision == 0xC2) ? (1 << 2) : (1 << 1);
    SetRegB (xBridgePCIAddr, 0x79, (UCHAR)(save | Val));

    Val = GetRegB (PCIAddr, PCI_ALI_IDEATA66EN);
    SetRegB (PCIAddr, PCI_ALI_IDEATA66EN, (UCHAR)(Val | (1 << 3)));

    SetRegB (PCIAddr, PCI_ALI_IDEATA66DET, (UCHAR)(~0x04 & // enable R/W simplex
	     (Cable = GetRegB (PCIAddr, PCI_ALI_IDEATA66DET))));
    SetRegB (PCIAddr, PCI_ALI_IDEATA66EN, Val);
    SetRegB (xBridgePCIAddr, 0x79, save);

    if (!(Cable & (1 << npA->IDEChannel))) npA->Cap |= CHANCAP_CABLE80;
  }

  return (TRUE);
}


USHORT NEAR GetALIPio (NPA npA, UCHAR Unit) {
  UCHAR Timing;

  Timing = ReadConfigB (PciInfo, (UCHAR)(PCI_ALI_IDETIM + Unit + 4 * npA->IDEChannel));
  return (2 + ((Timing - 1) & 0x0F) + (((UCHAR)(Timing >> 4) - 1) & 0x07));
}

/*--------------------------------------------*/
/*					      */
/* SetupALI()				      */
/*					      */
/*					      */
/*--------------------------------------------*/
VOID NEAR SetupALI (NPA npA)
{
  CHAR Unit;
  NPU  npU = npA->UnitCB;

  for (Unit = npA->cUnits; --Unit >= 0; npU++) {
    if (!(npU->Flags & UCBF_ATAPIDEVICE) && (CurLevel == ALIA) &&
	(npU->ATAVersion < 4) &&
	(*(ULONG*)(npU->ModelNum) == 0x20434457UL)) {  /* 'WDC ' */
      npU->UltraDMAMode = 0;
    }
  }

  SetupCommon (npA);
}

/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID ALITimingValue (NPU npU)
{
  NPA npA = npU->npA;
  UCHAR Unit = npU->UnitId;
  UCHAR UDMA, PIO;
  NPCH	pTim = (NPCH)&(npA->ALI_IDEtim);

  pTim[0] = 0x02;
  pTim[1] = ALI_ModeSets[PCIClockIndex][0];

  PIO = npU->InterfaceTiming;
  pTim[2 + Unit] = ALI_ModeSets[PCIClockIndex][PIO];
  Unit *= 4;

  UDMA = npU->UltraDMAMode;

  pTim = (NPCH)&(npA->ALI_Ultratim);
  pTim[0] &= (UCHAR)0xF0 >> Unit;
  if (UDMA > 0) {
    pTim[0] |= (0x0C ^ (npA->Cap & CHIPCAP_ATA100 ?
	    ALI_Ultra100ModeSets : ALI_UltraModeSets[PCIClockIndex])[UDMA-1]) << Unit;

    /*************************************************************/
    /* If running above UDMA1 on some chips ignore BM active bit */
    /*************************************************************/

    if ((npU->UltraDMAMode >= ULTRADMAMODE_0) && (CurLevel <= ALIB))
      npU->Flags |= UCBF_IGNORE_BM_ACTV;
  }
}

/*-------------------------------*/
/*				 */
/* ProgramALIChip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramALIChip (NPA npA)
{
  UCHAR Reg, PCIDataB;

  /*************************************/
  /* Set ATA-66 mode		       */
  /*************************************/

  if (npA->Cap & CHIPCAP_ATA66) {
    WConfigW (PCI_ALI_IDEATA66DET, RConfigW (PCI_ALI_IDEATA66DET) | 0x4120);
    WConfigB (PCI_ALI_IDEATAxxEN, 2);
    WConfigB (PCI_ALI_IDECFG, (UCHAR)(RConfigB (PCI_ALI_IDECFG) & ~0x40));
  }

  /*************************************/
  /* Write IDE timing Register	       */
  /*************************************/

  WConfigD ((UCHAR)(npA->IDEChannel ? PCI_ALI_IDEASU+4 : PCI_ALI_IDEASU), npA->ALI_IDEtim);

  /**********************************/
  /* Set FIFO operation mode	    */
  /**********************************/

  Reg = PCI_ALI_IDEFIFO + npA->IDEChannel;
  PCIDataB = RConfigB (Reg);
  PCIDataB = (PCIDataB & 0xF0) | (npA->UnitCB[0].Flags & UCBF_ATAPIDEVICE ? 0 : 0x05);
  PCIDataB = (PCIDataB & 0x0F) | (npA->UnitCB[1].Flags & UCBF_ATAPIDEVICE ? 0 : 0x50);
  WConfigB (Reg, PCIDataB);

  Reg += PCI_ALI_IDEULTRA - PCI_ALI_IDEFIFO;
  WConfigB (Reg, (UCHAR)(0x44 ^ (UCHAR)npA->ALI_Ultratim));

  PCIDataB = RConfigB (PCI_ALI_IDECDCTRL)
	   & ~0x07;	/* enable ATAPI PIO mode FIFO, disable ATAPI DMA mode */
  if (npA->Cap & CHIPCAP_ATAPIDMA)
    PCIDataB |= 0x01;	/* enable ATAPI DMA mode */
  WConfigB (PCI_ALI_IDECDCTRL, PCIDataB);

  PCIDataB = RConfigB (PCI_ALI_IDEATA66EN)
	   & ~0x80;	/* disable ATAPI DMA write mode */
  if (npA->Cap & CHIPCAP_ATAPIDMAWR)
    PCIDataB |= 0x80;	/* enable ATAPI DMA write mode */
  WConfigB (PCI_ALI_IDEATA66EN, PCIDataB);
}
