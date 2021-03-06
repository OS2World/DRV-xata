/**************************************************************************
 *
 * SOURCE FILE NAME = S506PIIX.C
 *
 * DESCRIPTIVE NAME = DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : Adapter Driver PIIX and SMSC routines.
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

#define  SLC66PCIIDE_DEV_ID  0x9130

#define IDETIM_IDE	     0x8000	/* IDE Decode Enable bit */
#define PCI_PIIX_IDETIM      0x40	/* 0x40-41 IDE timing */
#define PCI_PIIX_SIDETIM     0x44	/* PIIX3 slave IDE timing register  */
#define PCI_PIIX_SDMACTL     0x48	/* PIIX4 Ultra DMA Control Register */
#define PCI_PIIX_SDMATIM     0x4A	/* PIIX4 Ultra DMA Timing Register  */
#define PCI_PIIX_IDECFG      0x54	/* ICH	 IDE configuration Register */
#define PCI_PIIX_MAP	     0x90	/* ICH SATA Address Map Register    */

#define PCI_SMSC_CABLE	     0x47	/* SMSC cable configuration Register */

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

#define PciInfo (&npA->PCIInfo)

BOOL NEAR AcceptPIIX (NPA npA)
{
  UCHAR val = ' ';
  NPCH	str = PIIXxMsgtxt;
  UCHAR map;

  map = GetRegB (PciInfo->PCIAddr, PCI_PIIX_MAP);

  switch (PciInfo->CompatibleID) {  // besser vielleicht PciInfo->Level!
    case PIIX_PCIIDE_DEV_ID: {
      PciInfo->Level = PIIX;

      if ((MEMBER(npA).Revision < 2) || /* Rev is not capable of proper DMA. */
	  ((xBridgeDevice == 0x84C4) && (xBridgeRevision < 4))) { // check for ORION
	npA->Cap &= ~(CHIPCAP_ATADMA | CHIPCAP_ATAPIDMA);
      }
      npA->Cap &= ~CHIPCAP_ULTRAATA;

      MEMBER(npA).CfgTable = CfgPIIX;
      MEMBER(npA).TModes |= TR_PIO_SHARED; // PIIX has only one timing register set
      break;
    }
    case PIIX3_PCIIDE_DEV_ID:
      PciInfo->Level = PIIX3;
      npA->Cap &= ~CHIPCAP_ULTRAATA;
      val = '3';
      MEMBER(npA).CfgTable = CfgPIIX3;
      break;
    case PIIX4_PCIIDE_DEV_ID:
      PciInfo->Level = PIIX4;
      if (MEMBER(npA).Device == ICH0_PCIIDE_DEV_ID) {
	val = '0';
	str = ICHxMsgtxt;
      } else {
	val = '4';
	MEMBER(npA).CfgTable = CfgPIIX4;
      }
      break;
    case ICH_PCIIDE_DEV_ID:
      PciInfo->Level = ICH;
      npA->Cap |= CHIPCAP_ATA66;
      str = ICHxMsgtxt;
      break;
    case ICH2PCIIDE_DEV_ID:
      val = '2'; goto ICHCommon;
    case ICH3PCIIDE_DEV_ID:
      val = '3'; goto ICHCommon;
    case ICH4PCIIDE_DEV_ID:
      val = '4'; goto ICHCommon;
    case ICH5PCIIDE_DEV_ID:
      val = '5'; goto ICHCommon;
    case ICH6PCIIDE_DEV_ID:
      val = '6'; goto ICHCommon;
    case ICH7PCIIDE_DEV_ID:
      val = '7';
    ICHCommon:
      str = ICHxMsgtxt;
    ICHCommon1:
//	npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133;
      npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;
      PciInfo->Level = ICH2;
      break;
    case ICH5SPCIIDE_DEV_ID:
      val = '5';
      map >>= 1;
      if (map & 2) {
	npA->maxUnits = 2;
	if ((map & 1) != npA->IDEChannel) goto ICHCommon; // combined, PATA port
      }
      goto SATACommon;
    case ICH9SPCIIDE_DEV_ID:
      npA->maxUnits = 2;
      val = '9';
      goto SATACommon;
    case ICH8SPCIIDE_DEV_ID:
      npA->maxUnits = 2;
    case ICH8S2PCIIDE_DEV_ID: // master only!
      val = '8';
      goto SATACommon;
    case ICH7SPCIIDE_DEV_ID:
      val = '7';
      goto SATACommon1;
    case ICH6SPCIIDE_DEV_ID:
      val = '6';
    SATACommon1:
      npA->maxUnits = 2;
      if (map & (1 << npA->IDEChannel)) goto ICHCommon;  // combined, PATA port
    SATACommon:
      npA->Cap |= CHIPCAP_SATA;
      npA->Cap &= ~CHIPCAP_ATAPIDMA;
      goto ICHCommon;
      break;
  }
  sprntf (npA->PCIDeviceMsg, str, val);

  if (npA->FlagsI.b.native) METHOD(npA).CheckIRQ = BMCheckIRQ;

  if (npA->Cap & CHIPCAP_ATA66) {
    UCHAR Cable;

    /* determine the presence of a 80wire cable as per defined procedure */

    Cable = GetRegB (PciInfo->PCIAddr, PCI_PIIX_IDECFG);
    if (Cable & (npA->IDEChannel ? 0xC0 : 0x30))
      npA->Cap |= CHANCAP_CABLE80;
  }

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
    (UCHAR)IDEtim |= Flags & value;
    if (npU->UnitId && (CurLevel >= PIIX3)) /* Slave Timing Register enable */
      IDEtim |= ACBX_IDETIM_ITE1;
  } else {				     // PIO0, no DMA
    if (CurLevel <= PIIX3) npA->Cap &= ~CHIPCAP_PIO32;
  }
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
      UATA |= 0x0400; // Ping Pong enable
      WConfigW (PCI_PIIX_IDECFG, UATA);
    }
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
