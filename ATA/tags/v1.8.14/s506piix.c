/**************************************************************************
 *
 * SOURCE FILE NAME = S506PIIX.C
 * $Id$
 *
 * DESCRIPTIVE NAME = DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : Adapter Driver PIIX, SMSC and ITE8213 routines.
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

/*----------------------------------------------------------------*/
/* Defines for PIIX_level	     PCI Config Device IDs	  */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/
#define NOPIIX		    0
#define PIIX		    1	/* PIIX  1230h Triton	   */
#define PIIX3		    2	/* PIIX3 7010h		   */
#define PIIX4		    3	/* PIIX4 7111h PIIX4e 7199h ICH0 2421h */
#define SLC66		    4	/* SLC90C66		   */
#define ICH		    5	/* ICH	 2411h 7601h	   */
#define ICH2		    6	/* ICH2  244B		   */
#define ICH3		    6	/* ICH3  248B		   */
#define ICH4		    7	/* ICH4  24CB		   */
#define ITE		    8   /* ITE8213			   */

/*----------------------------------------------------------------*/
/* PIIX PCI to IDE Bridge Device IDs				  */
/*----------------------------------------------------------------*/
#define   PIIX_PCIIDE_DEV_ID  0x1230
#define  PIIX3_PCIIDE_DEV_ID  0x7010
#define  PIIX4_PCIIDE_DEV_ID  0x7111
#define  PIIX4ePCIIDE_DEV_ID  0x7199
#define  PIIX4xPCIIDE_DEV_ID  0x84CA
#define   ICH0_PCIIDE_DEV_ID  0x2421
#define    ICH_PCIIDE_DEV_ID  0x2411
#define    ICHxPCIIDE_DEV_ID  0x7601
#define    ICH2PCIIDE_DEV_ID  0x244B
#define   ICH2MPCIIDE_DEV_ID  0x244A
#define    ICH3PCIIDE_DEV_ID  0x248B
#define   ICH3MPCIIDE_DEV_ID  0x248A
#define    ICH4PCIIDE_DEV_ID  0x24CB
#define    CICHPCIIDE_DEV_ID  0x245B
#define    ICH5PCIIDE_DEV_ID  0x24DB
#define   ICH5aPCIIDE_DEV_ID  0x25A2
#define   ICH5SPCIIDE_DEV_ID  0x24D1
#define  ICH5RSPCIIDE_DEV_ID  0x24DF
#define  ICH5aSPCIIDE_DEV_ID  0x25A3
#define ICH5aRSPCIIDE_DEV_ID  0x25B0
#define    ICH6PCIIDE_DEV_ID  0x266F
#define   ICH6SPCIIDE_DEV_ID  0x2651
#define  ICH6SRPCIIDE_DEV_ID  0x2652
#define    ICH7PCIIDE_DEV_ID  0x27DF
#define   ICH7SPCIIDE_DEV_ID  0x27C0
#define   ICH8SPCIIDE_DEV_ID  0x2820
#define  ICH8S2PCIIDE_DEV_ID  0x2825
#define   ICH9SPCIIDE_DEV_ID  0x2920
#define  ICH10SPCIIDE_DEV_ID  0x3A00
#define ICH10S2PCIIDE_DEV_ID  0x3A06
#define 	 IDER_DEV_ID  0x2986
#define 	  SCH_DEV_ID  0x811A

#define  SLC66PCIIDE_DEV_ID  0x9130

#define      ITE8213_DEV_ID  0x8213

#define IDETIM_IDE	     0x8000	/* IDE Decode Enable bit */
#define PCI_PIIX_IDETIM      0x40	/* 0x40-41 IDE timing */
#define PCI_PIIX_SIDETIM     0x44	/* PIIX3 slave IDE timing register  */
#define PCI_PIIX_SDMACTL     0x48	/* PIIX4 Ultra DMA Control Register */
#define PCI_PIIX_SDMATIM     0x4A	/* PIIX4 Ultra DMA Timing Register  */
#define PCI_PIIX_IDECFG      0x54	/* ICH	 IDE configuration Register */
#define PCI_PIIX_MAP	     0x90	/* ICH SATA Address Map Register    */

#define PCI_SMSC_CABLE	     0x47	/* SMSC cable configuration Register */
#define PCI_ITE_CABLE	     0x42	/* ITE cable configuration Register */

#define ACBX_IDETIM_MODE0    0x8000	/* mode 0 PIO timing */
#define ACBX_IDETIM_DTE1     0x0080	/* DMA fast timing only drive 1 */
#define ACBX_IDETIM_PPE1     0x0040	/* prefetct/posting enable drive 1 */
#define ACBX_IDETIM_IE1      0x0020	/* IORDY samply point drive 1 */
#define ACBX_IDETIM_TIME1    0x0010	/* fast timing select drive 1 */
#define ACBX_IDETIM_DTE0     0x0008	/* DMA fast timing only drive 0 */
#define ACBX_IDETIM_PPE0     0x0004	/* prefetct/posting enable drive 0 */
#define ACBX_IDETIM_IE0      0x0002	/* IORDY samply point drive 0 */
#define ACBX_IDETIM_TIME0    0x0001	/* fast timing select drive 0 */
#define ACBX_IDETIM_ITE1     0x4000	/* PIIX3 slave timing enable */

#define SCH_D0TIM	     0x80	// Device 0 timing register
#define SCH_D1TIM	     0x84	// Device 1 timing register
#define SCH_USD 	     (1ul << 31) // Use Synchronous DMA
#define SCH_PPE 	     (1ul << 30) // Prefetch/Post Enable
#define SCH_UDM_SHIFT	     16		// Ultra DMA mode shift, bits 18:16
#define SCH_MDM_SHIFT	     8		// Multi-word DMS mode shift, bits 9:8
#define SCH_PIO_SHIFT	     0		// PIO mode shift, bits 2:0

#define PciInfo (&npA->PCIInfo)

/**
 * Check detected device acceptable for use and set up
 * Fails only if SATA device without physical device attached
 * @return TRUE if accepted else FALSE
 */

BOOL NEAR AcceptPIIX (NPA npA)
{
  UCHAR modifier = ' ';			// Adapter description modifier (char or number)
  NPCH	str = PIIXxMsgtxt;		// Adapter description, overridden for ICH
  UCHAR ichregmap;

  ichregmap = GetRegB (PciInfo->PCIAddr, PCI_PIIX_MAP);

  switch (PciInfo->CompatibleID) {  // besser vielleicht PciInfo->Level!

    case PIIX_PCIIDE_DEV_ID:
    {
      PciInfo->Level = PIIX;
      if ((MEMBER(npA).Revision < 2) || /* Rev is not capable of proper DMA. */
	  ((xBridgeDevice == 0x84C4) && (xBridgeRevision < 4))) { // check for ORION
	npA->Cap &= ~(CHIPCAP_ATADMA | CHIPCAP_ATAPIDMA);
      }
      npA->Cap &= ~CHIPCAP_ULTRAATA;

      MEMBER(npA).CfgTable = CfgPIIX;
      MEMBER(npA).TModes |= TR_PIO_SHARED; // PIIX has only one timing register set
    }
    break;

    case PIIX3_PCIIDE_DEV_ID:
      PciInfo->Level = PIIX3;
      npA->Cap &= ~CHIPCAP_ULTRAATA;
      modifier = '3';
      MEMBER(npA).CfgTable = CfgPIIX3;
      break;

    case PIIX4_PCIIDE_DEV_ID:
      PciInfo->Level = PIIX4;
      if (MEMBER(npA).Device == ICH0_PCIIDE_DEV_ID) {
	modifier = '0';
	str = ICHxMsgtxt;
      } else {
	modifier = '4';
	MEMBER(npA).CfgTable = CfgPIIX4;
      }
      break;

    case ICH_PCIIDE_DEV_ID:
      PciInfo->Level = ICH;
      npA->Cap |= CHIPCAP_ATA66;
      modifier = 1;
      str = ICHxMsgtxt;
      break;

    case ICH2PCIIDE_DEV_ID:
      modifier = 2;
      goto ICHCommon;

    case ICH3PCIIDE_DEV_ID:
      modifier = 3;
      goto ICHCommon;

    case ICH4PCIIDE_DEV_ID:
      modifier = 4;
      goto ICHCommon;

    case ICH5PCIIDE_DEV_ID:
      modifier = 5;
      goto ICHCommon;

    case ICH6PCIIDE_DEV_ID:
      modifier = 6;
      goto ICHCommon;

    case ICH7PCIIDE_DEV_ID:
      modifier = 7;
    ICHCommon:
      str = ICHxMsgtxt;
      // npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133;
      npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;
      PciInfo->Level = ICH2;
      break;

    case ICH5SPCIIDE_DEV_ID:
      modifier = 5;
      ichregmap >>= 1;
      if (ichregmap & 2) {
	npA->maxUnits = 2;
	if ((ichregmap & 1) != npA->IDEChannel) goto ICHCommon; // combined, PATA port
      }
      goto SATACommon;

    case ICH10SPCIIDE_DEV_ID:
      npA->maxUnits = 2;
    case ICH10S2PCIIDE_DEV_ID: // master only!
      modifier = 10;
      goto SATACommon;

    case ICH9SPCIIDE_DEV_ID:
      npA->maxUnits = 2;
      modifier = 9;
      goto SATACommon;

    case ICH8SPCIIDE_DEV_ID:
      npA->maxUnits = 2;

    case ICH8S2PCIIDE_DEV_ID: // master only!
      modifier = 8;
      goto SATACommon;

    case ICH7SPCIIDE_DEV_ID:
      modifier = 7;
      goto SATACommon1;

    case ICH6SPCIIDE_DEV_ID:
      modifier = 6;

    SATACommon1:
      npA->maxUnits = 2;		// Force 2 units
      if (ichregmap & (1 << npA->IDEChannel)) goto ICHCommon;  // combined, PATA port

    SATACommon:
      npA->Cap |= CHIPCAP_SATA;
      // npA->ProgIF.b.native = 1;  // required (at least) for ICH8 // 2011-07-27 SHL fixme to be gone
      npA->ProgIF.b.native = PCI_IDE_NATIVE_IF1;  // required (at least) for ICH8
      if (modifier >= 6) {
	// ICH6+
	NPC   npC  = npA->npC;
	ULONG BAR5 = npC->BAR[5].Addr;	// fixme to be sure this is ABAR/SIDPBA1-AHCI?
	NPU   npU  = npA->UnitCB;

	if (BAR5) {
	  if (BAR5 < 0x10000) {
	    // IO mapped index
	    UCHAR Port = npA->IDEChannel * 2;
	    SSTATUS = ((ULONG)INDEXED (Port << 8) << 16) | (BAR5 & 0xFFFF);
	    SERROR   = SSTATUS | 0x20000;
	    SCONTROL = SSTATUS | 0x10000;
	    if (npA->maxUnits > 1) {
	      npU++;
	      SSTATUS = ((ULONG)INDEXED ((Port+1) << 8) << 16) | (BAR5 & 0xFFFF);
	      SERROR   = SSTATUS | 0x20000;
	      SCONTROL = SSTATUS | 0x10000;
	    }
	    npA->SCR.Offsets = 0;  // do *not* allocate SCR ports by default method
	  } else {
	    // Assume memory mapped AHCI registers
	    // fixme to know why can assume
	    // is AHCI_PI != 0
	    if (InD (BAR5 | AHCI_PI)) {
	      UCHAR Port = npA->IDEChannel;
	      UCHAR havePhy = 0;
	      SSTATUS = GetAHCISCR (npA, Port);
	      if (SSTATUS)
		havePhy = 1;
	      else
		npU->FlagsT |= UTBF_DISABLED;

	      if (npA->maxUnits > 1) {
		npU++;
		SSTATUS = GetAHCISCR (npA, Port + 2);
		if (SSTATUS)
		  havePhy |= 2;
		else
		  npA->maxUnits = 1;
	      }
	      if (!havePhy) return (FALSE);
	      npA->SCR.Offsets = 0x3120;	// STATUS, ERROR, CONTROL - see CollectSCRPorts
	    }
	  }
	}
      } // ICH6+
      goto ICHCommon;

    case IDER_DEV_ID:
      str = IDERMsgtxt;
      PciInfo->Level = ICH;
      MEMBER(npA).CfgTable = CfgNull;
      METHOD(npA).GetPIOMode = 0;
      npA->FlagsT |= ATBF_BIOSDEFAULTS;
      break;

  } // switch CompatibleID

  sprntf (npA->PCIDeviceMsg, str, modifier);

  if (npA->ProgIF.b.native) METHOD(npA).CheckIRQ = BMCheckIRQ;

  if (npA->Cap & CHIPCAP_ATA66) {
    /* determine the presence of a 80wire cable as per defined procedure */
    UCHAR Cable = GetRegB (PciInfo->PCIAddr, PCI_PIIX_IDECFG);
    if (Cable & (npA->IDEChannel ? 0xC0 : 0x30))
      npA->Cap |= CHANCAP_CABLE80;
  }

  return (TRUE);
}

BOOL NEAR AcceptSCH (NPA npA)
{
  npA->npC->numChannels = 1;
  npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;

  // there's no architected method to determine cable type
  // Assume 80 pin cable if running DMA mode2 or better
  if ((GetRegB (PciInfo->PCIAddr, SCH_D0TIM + 2) > 2) ||
      (GetRegB (PciInfo->PCIAddr, SCH_D1TIM + 2) > 2))
    npA->Cap |= CHANCAP_CABLE80;

  return (TRUE);
}

BOOL NEAR AcceptSMSC (NPA npA)
{
  UCHAR Cable;

  PciInfo->Level = SLC66;
  npA->Cap |= CHIPCAP_ATA66;

  /* determine the presence of a 80wire cable as per defined procedure */

  Cable = GetRegB (PciInfo->PCIAddr, PCI_SMSC_CABLE);
  if (!(Cable & (npA->IDEChannel ? 2 : 1)))
    npA->Cap |= CHANCAP_CABLE80;

  return (TRUE);
}

BOOL NEAR AcceptITE8213 (NPA npA)
{
  UCHAR Cable;

  npA->npC->numChannels = 1;
  if (npA->IDEChannel) return (FALSE);

  sprntf (npA->PCIDeviceMsg, ITEMsgtxt, MEMBER(npA).Device);
  PciInfo->Level = ITE;
  npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133;

  /* determine the presence of a 80wire cable as per defined procedure */

  Cable = GetRegB (PciInfo->PCIAddr, PCI_ITE_CABLE);
  if (!(Cable & 2))
    npA->Cap |= CHANCAP_CABLE80;

  return (TRUE);
}

#define IDEtim	(USHORT)npA->PIIX_IDEtim
#define SIDEtim (UCHAR)npA->PIIX_SIDEtim

USHORT NEAR GetPIIXPio (NPA npA, UCHAR Unit) {
  USHORT Timing;

  Timing = ReadConfigW (PciInfo, (UCHAR)(PCI_PIIX_IDETIM + 2 * npA->IDEChannel));
  if (PciInfo->Level == PIIX) Unit = 0;
  if (Unit == 0) {
    if (!(Timing & ACBX_IDETIM_TIME0))
      return (20);
    else
      return (9 - ((UCHAR)(Timing >> 12) & 0x03) - ((UCHAR)(Timing >> 8) & 0x03));
  } else {
    if (!(Timing & ACBX_IDETIM_TIME1) || !(Timing & ACBX_IDETIM_ITE1))
      return (20);
    else {
      Timing = ReadConfigB (PciInfo, PCI_PIIX_SIDETIM);
      if (npA->IDEChannel) Timing >>= 4;
      return (9 - ((UCHAR)(Timing >> 2) & 0x03) - (Timing & 0x03));
    }
  }
}

/**
 * Return number of clocks for selected PIO mode
 */

USHORT NEAR GetSCHPio (NPA npA, UCHAR Unit) {
  switch (ReadConfigB (PciInfo, (UCHAR)(Unit ? SCH_D1TIM : SCH_D0TIM))) {
    case 4:  return (4);		// Mode 4 - 4 clocks
    case 3:  return (6);		// Mode 3 - 6 clocks
    default: return (20);		// Others - 20 clocks
  }
}

/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID PIIXTimingValue (NPU npU)
{
  NPA	npA = npU->npA;
  UCHAR Flags;
  UCHAR value;

  value = PIIX_ModeSets[PCIClockIndex][npU->InterfaceTiming];

  if (npU->UnitId) {					     /* Slave timing */
    SIDEtim = ((UCHAR)(value * 0x14) & 0xF0) >> 4;
    Flags = ACBX_IDETIM_TIME1 | ACBX_IDETIM_IE1 | ACBX_IDETIM_PPE1 | ACBX_IDETIM_DTE1;
  } else {
    IDEtim = value << 8;
    Flags = ACBX_IDETIM_TIME0 | ACBX_IDETIM_IE0 | ACBX_IDETIM_PPE0 | ACBX_IDETIM_DTE0;
  }

  /*****************************************/
  /* if FAST timing on unit then enable it */
  /*****************************************/
  if (npU->InterfaceTiming | npU->UltraDMAMode) {
    value = ACBX_IDETIM_TIME1 | ACBX_IDETIM_IE1 | ACBX_IDETIM_TIME0 | ACBX_IDETIM_IE0;
    if (npU->CurDMAMode && !npU->CurPIOMode) // if DMA with PIO0: DMA Timing Enable Only bit
      value |= ACBX_IDETIM_DTE1 | ACBX_IDETIM_DTE0;
    if (!(npU->Flags & UCBF_ATAPIDEVICE)) // Enable Pre-Fetch/Posting if FIXED Disk
      value |= ACBX_IDETIM_PPE1 | ACBX_IDETIM_PPE0;
    if (CurLevel == ITE)  // Pre-Fetch/Posting reversed on ITE8213
      value ^= ACBX_IDETIM_PPE1 | ACBX_IDETIM_PPE0;
    (UCHAR)IDEtim |= Flags & value;
    if (npU->UnitId && (CurLevel >= PIIX3)) /* Slave Timing Register enable */
      IDEtim |= ACBX_IDETIM_ITE1;
  } else {				     // PIO0, no DMA
    if (CurLevel <= PIIX3) npA->Cap &= ~CHIPCAP_PIO32;
  }
  IDEtim |= ACBX_IDETIM_MODE0;	// set enable decode bit
}


/*-------------------------------*/
/*				 */
/* ProgramPIIXChip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramPIIXChip (NPA npA)
{
  UCHAR i = npA->IDEChannel;
  UCHAR Data;

  /*************************************/
  /* Write IDE timing Register	       */
  /*************************************/

  if (IDEtim) WConfigW ((UCHAR)(i ? PCI_PIIX_IDETIM + 2 : PCI_PIIX_IDETIM), IDEtim);

  /*************************************/
  /* Setup PIIX SLAVE Timing Register. */
  /*************************************/

  if (SIDEtim) {
    UCHAR Mask = i ? 0x0F : 0xF0;

    WConfigB (PCI_PIIX_SIDETIM, (UCHAR)
	      ((RConfigB (PCI_PIIX_SIDETIM) & Mask) | ((SIDEtim * 0x11) & ~Mask)));
  }

  /*************************************************/
  /*		   Ultra DMA Setup		   */
  /*************************************************/

  if (npA->Cap & CHIPCAP_ULTRAATA) {
    NPU    npU;
    UCHAR  UCtl;
    USHORT UTim, UATA = 0;

    UCtl = RConfigB (PCI_PIIX_SDMACTL);
    UTim = RConfigW (PCI_PIIX_SDMATIM);
    if (npA->Cap & CHIPCAP_ATA66)
      UATA = RConfigW (PCI_PIIX_IDECFG);

    UCtl &= ~(i ? 0x0C : 0x03);
    if (CurLevel == SLC66) {
      UTim &= ~(i ? 0x7700 : 0x0077);
    } else {
      UTim &= ~(i ? 0x3300 : 0x0033);
      UATA &= ~(i ? 0xC00C : 0x3003);
    }
    for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->cUnits); npU++) {
      UCHAR UDMA = npU->UltraDMAMode;

      if (UDMA) {		  /* Ultra Device */
	UCHAR Shift = 2 * i + npU->UnitId;

	Data = (CurLevel == SLC66 ? SMSC_UltraModeSets : PIIX_UltraModeSets)[PCIClockIndex][UDMA - 1];

	UCtl |= 0x01 << Shift;			      /* Turn On Bit  */
	UATA |= ((Data >> 4) & 1) << Shift;	      /* Set ATA-66 bits     */
	UATA |= ((Data >> 5) & 1) << (Shift + 12);    /* Set ATA-100 bits    */
	UTim |= (Data & 0x07) << (4 * Shift);	      /* Set new timing bits */
      }
    }

    WConfigB (PCI_PIIX_SDMACTL, UCtl);
    WConfigW (PCI_PIIX_SDMATIM, UTim);
    if (npA->Cap & CHIPCAP_ATA66) {
      if (CurLevel != ITE) UATA |= 0x0400; // Ping Pong enable
      WConfigW (PCI_PIIX_IDECFG, UATA);
    }
  }
}

/**
 * Program Paulsbo SCH PATA controller
 */

VOID ProgramSCH (NPA npA) {
  NPU npU;
  UCHAR Reg = SCH_D0TIM;

  for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->cUnits); npU++, Reg += SCH_D1TIM - SCH_D0TIM) {
    ULONG val;

    if (npU->Flags & UCBF_NOTPRESENT) continue;

    val = npU->CurPIOMode;
    if (npU->UltraDMAMode)
      val |= ((ULONG)(npU->UltraDMAMode-1) << SCH_UDM_SHIFT) | SCH_USD;
    else if (npU->CurDMAMode)
      val |= (npU->CurDMAMode << SCH_MDM_SHIFT);
    // 2012-10-02 SHL FIXME to know if should set PPE only for PIO mode
    if (!(npU->Flags & UCBF_ATAPIDEVICE)) val |= SCH_PPE;

    WConfigD (Reg, val);
  }
}

#if ENABLEBUS
/*-------------------------------*/
/*				 */
/* EnableBus()			 */
/*				 */
/*-------------------------------*/
VOID EnableBusPIIX (NPA npA, UCHAR enable)
{
  UCHAR  Data, Mask, Reg;
  USHORT PciAddr;

  if (npA->Cap & CHIPCAP_ATA66) {
    PciAddr = npA->PCIInfo.PCIAddr;
    Reg = PCI_PIIX_IDECFG + 2;
    Mask = npA->IDEChannel ? 0x0C : 0x03;
    PciGetReg (PciAddr, Reg, (PULONG)&Data, 1);
    Data &= ~Mask;				      // enable
    if (!enable) Data |= 0x22 & Mask;		      // disable: drive low
  } else {
    PciAddr = npA->PCIInfo.PCIAddr & ~7;
    Reg = 0xB1;
    Mask = npA->IDEChannel ? 0x10 : 0x08;
    PciGetReg (PciAddr, Reg, (PULONG)&Data, 1);
    Data &= ~Mask;				      // enable
    if (!enable) Data |= 0x18 & Mask;		      // disable: tristate
  }
  PciSetReg (PciAddr, Reg, Data, 1);
}
#endif
