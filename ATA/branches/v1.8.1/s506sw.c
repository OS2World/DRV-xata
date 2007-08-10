/**************************************************************************
 *
 * SOURCE FILE NAME = S506SW.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : Adapter Driver ServerWorks OSB routines.
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

#define PCI_SW_PIOCOUNT        0x4B	   /* SW IDE PIO prefetch cycles   */
#define PCI_SW_PIOCONTROL      0x48	   /* SW IDE PIO control	   */
#define PCI_SW_PIOTIM	       0x40	   /* SW IDE PIO timings (+2)	   */
#define PCI_SW_MDMATIM	       0x44	   /* SW IDE MW DMA timings (+2)   */
#define PCI_SW_UDMACONTROL     0x54	   /* SW IDE Ultra DMA control	   */
#define PCI_SW_UDMATIM	       0x56	   /* SW IDE Ultra DMA timings (+1)*/
#define PCI_SW_PIO	       0x4A	   /* SW IDE PIO mode (CSB5)   (+1)*/


/*----------------------------------------------------------------*/
/* Defines for OSB_level	     PCI Config Device IDs	  */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/
#define NOOSB		    0
#define OSB4		    1	/* OSB4 			*/
#define CSB5		    2	/* CSB5  ATA66			*/
#define CSB5_100	    3	/* CSB5  ATA100 		*/
#define ATI_IXP 	    4	/* IXP1  ATA100 		*/
#define ATI_IXP2	    5	/* IXP2  ATA133 		*/

/* remap some structure members */

#define OSB_PIOtim ((NPCH)&(npA->PIIX_IDEtim))
#define OSB_DMAtim ((NPCH)&(npA->PIIX_SIDEtim))

#define PCIDEV_OSB4   0x0211
#define PCIDEV_CSB5   0x0212
#define PCIDEV_CSB6   0x0213
#define PCIDEV_HT1000 0x0214

#define PciInfo (&npA->PCIInfo)

BOOL NEAR AcceptOSB (NPA npA)
{
  USHORT val = 4;
  UCHAR  enable = FALSE;

  PciInfo->Level = OSB4;
  switch (PciInfo->CompatibleID) {
    case PCIDEV_HT1000:
      PciInfo->Level = CSB5_100;
      npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;
      npA->npC->numChannels = 1;
      val = 1000;
      goto CSBx;
    case PCIDEV_CSB6:
      if (MEMBER(npA).Revision >= 0xA0) { // check for version A1.0 and up
	PciInfo->Level = CSB5_100;
	npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;
      } else {
	PciInfo->Level = CSB5;
//	  npA->Cap |= CHIPCAP_ATA66;
      }
      val = 6;
      goto CSBx;
    case PCIDEV_CSB5:
      if (MEMBER(npA).Revision >= 0x92) { // check for version A2.0 and up
	PciInfo->Level = CSB5_100;
	npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;
      } else {
	PciInfo->Level = CSB5;
	npA->Cap |= CHIPCAP_ATA66;
      }
      val = 5;
    CSBx:
      /* determine the presence of a 80wire cable as per defined procedure   */
      /* hack! There is no defined cable detection procedure, use BIOS hints */

      enable = GetRegB (PciInfo->PCIAddr, (UCHAR)(PCI_SW_UDMATIM + npA->IDEChannel));
      enable = (enable > 0x20) || ((enable & 0x0F) > 2);
      break;
  }
  sprntf (npA->PCIDeviceMsg, OSBMsgtxt, val);

  if (enable) npA->Cap |= CHANCAP_CABLE80;
  return (TRUE);
}


BOOL NEAR AcceptIXP (NPA npA)
{
  UCHAR enable = FALSE;

  PciInfo->Level += ATI_IXP;
  npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;
  if (PciInfo->Level >= ATI_IXP2) npA->Cap |= CHIPCAP_ATA133;

  /* determine the presence of a 80wire cable as per defined procedure	 */
  /* hack! There is no defined cable detection procedure, use BIOS hints */

  enable = GetRegB (PciInfo->PCIAddr, (UCHAR)(PCI_SW_UDMATIM + npA->IDEChannel));
  enable = (enable > 0x20) || ((enable & 0x0F) > 2);
  if (enable) npA->Cap |= CHANCAP_CABLE80;
  return (TRUE);
}


USHORT NEAR GetOSBPio (NPA npA, UCHAR Unit) {
  UCHAR Timing;

  Timing = ReadConfigB (PciInfo, (UCHAR)(PCI_SW_PIOTIM + 1 - Unit + 2 * npA->IDEChannel));
  return (2 + (Timing & 0x0F) + (UCHAR)(Timing >> 4));
}

/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID OSBTimingValue (NPU npU)
{
  NPA	 npA = npU->npA;
  USHORT value;
  UCHAR  PIO, UDMA;

  PIO = npU->CurPIOMode;
  OSB_PIOtim[3] |= PIO << (4 * npU->UnitId);
  PIO = (PIO >= 3) ? PIO - 2 : 0;

  value = OSB_ModeSets[PCIClockIndex][PIO];
  OSB_PIOtim[npU->UnitId] = value;
//  if (PIO)
//    OSB_PIOtim[2] |= 0x08 >> npU->UnitId;
//  if (!(npU->Flags & UCBF_ATAPIDEVICE))
//    OSB_PIOtim[2] |= 0xA0 >> npU->UnitId;

  UDMA = npU->UltraDMAMode;
  if (UDMA) {
    value = OSB_UltraModeSets[PCIClockIndex][UDMA - 1];
    OSB_DMAtim[2] |= value << (4 * npU->UnitId);
    OSB_DMAtim[3] |= (1 << npU->UnitId);
  } else {
    value = OSB_ModeSets[PCIClockIndex][npU->CurDMAMode];
    OSB_DMAtim[npU->UnitId] = value;
  }
}

/*-------------------------------*/
/*				 */
/* ProgramOSBChip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramOSBChip (NPA npA)
{
  UCHAR Data, Reg, old;
  USHORT save;

  /*************************************/
  /* Write IDE timing Register	       */
  /*************************************/

  if (CurLevel == OSB4) {
    save = CurPCIInfo->PCIAddr;
    CurPCIInfo->PCIAddr &= 0xFFF8;    // point to south bridge function 0
    old = RConfigB (0x65);
    Data = (old & ~0x20) | 0x40;  // enable ATA/33, disable 600ns interrupt mask
    if (Data != old)
      WConfigBI (0x65, Data);
    CurPCIInfo->PCIAddr = save;
  } else if (CurLevel < ATI_IXP) {
    old = RConfigB (0x5A);
    Data = (old & ~0x40) | 0x02;		  // enable ATA/66
    if (npA->Cap & CHIPCAP_ATA100) Data |= 0x03;  // enable ATA/100
    if (Data != old)
      WConfigBI (0x5A, Data);
  }

  Reg = PCI_SW_PIOTIM + 2 * npA->IDEChannel;

  if (OSB_PIOtim[0] != 0) WConfigB ((UCHAR)(Reg + 1), OSB_PIOtim[0]);
  if (OSB_PIOtim[1] != 0) WConfigB ((UCHAR)(Reg + 0), OSB_PIOtim[1]);

  if ((npA->Cap & CHIPCAP_ATA66) && (*(NPUSHORT)OSB_PIOtim != 0))
    WConfigB ((UCHAR)(PCI_SW_PIO + npA->IDEChannel), OSB_PIOtim[3]);

  Reg += PCI_SW_MDMATIM - PCI_SW_PIOTIM;

  if (OSB_DMAtim[0] != 0) WConfigB ((UCHAR)(Reg + 1), OSB_DMAtim[0]);
  if (OSB_DMAtim[1] != 0) WConfigB ((UCHAR)(Reg + 0), OSB_DMAtim[1]);

  Data = (RConfigB (PCI_SW_UDMACONTROL)
       & (npA->IDEChannel ? ~0x0C : ~0x03))
       | (OSB_DMAtim[3] << (2 * npA->IDEChannel));
  WConfigB (PCI_SW_UDMACONTROL, Data);

  if (OSB_DMAtim[3])
    WConfigB ((UCHAR)(PCI_SW_UDMATIM + npA->IDEChannel), OSB_DMAtim[2]);
}
