/**************************************************************************
 *
 * SOURCE FILE NAME = S506OPTI.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 2001-2006
 *
 * DESCRIPTION : Adapter Driver Opti routines.
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

#define PCI_OPTI_IDECONFIG	 0x40	     /* OPTI IDE configuration */
#define PCI_OPTI_IDEFEATURES	 0x42	     /* OPTI IDE enhanced feature */
#define PCI_OPTI_IDETIM 	 0x43	     /* OPTI IDE enhanced timing  */
#define PCI_OPTI_IDEULTRASEL	 0x44	     /* OPTI IDE Ultra DMA timing */
#define PCI_OPTI_IDEULTRAMODE	 0x45	     /* OPTI IDE Ultra DMA timing */

#define OPTI_DEV621 0xC621
#define OPTI_DEV568 0xD568
#define OPTI_DEV768 0xD768

#define READ_REG  0 /* index of Read cycle timing register */
#define WRITE_REG 1 /* index of Write cycle timing register */
#define CNTRL_REG 3 /* index of Control register */
#define STRAP_REG 5 /* index of Strap register */
#define MISC_REG  6 /* index of Miscellaneous register */

/*----------------------------------------------------------------*/
/* Defines for OPTI_level	      PCI Config Device IDs	   */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/
#define NOOPTI		     0
#define OPTI		     1	 /* OPTI w/o busmaster	 */
#define OPTIA		     2	 /* OPTI		 */
#define OPTIB		     3	 /* OPTI w/Ultra	 */

/* remap some structure members */

#define OPTI_IDEtim   PIIX_IDEtim
#define OPTI_Ultratim PIIX_SIDEtim

#define PciInfo (&npA->PCIInfo)

BOOL NEAR AcceptOPTI (NPA npA)
{
  switch (PciInfo->CompatibleID) {
    case OPTI_DEV568:
      if (xBridgeDevice < 0xC700) {
	PciInfo->Level = OPTIA;
	npA->Cap &= ~CHIPCAP_ULTRAATA;
      } else {
	PciInfo->Level = OPTIB;
      }
      break;
    case OPTI_DEV768:
      PciInfo->Level = OPTIB;
      break;
    default:
      PciInfo->Level = OPTI;
      METHOD(npA).PCIFunc_InitComplete = GenericInitComplete;
      npA->Cap &= ~(CHIPCAP_ULTRAATA | CHIPCAP_ATADMA | CHIPCAP_ATAPIDMA);
  }

  npA->IRQDelay = 4; // slow interrupt
  return (TRUE);
}


UCHAR ReadIDEReg (NPA npA, UCHAR Idx) {
  USHORT Port = DATAREG + 1;
  UCHAR  Val;

  DISABLE
  inpw (Port);
  inpw (Port);
  outp (++Port, 0x03);
  Val = inp (DATAREG + Idx);
  outp (Port, 0x83);
  ENABLE
  return (Val);
}


USHORT NEAR GetOPTIPio (NPA npA, UCHAR Unit) {
  UCHAR Timing;

  Timing = ReadConfigB (PciInfo, PCI_OPTI_IDETIM);
  Timing >>= 2 * (npA->IDEChannel * 2 + Unit);
  switch (Timing & 3) {
    case 1: return 6;
    case 2: return 4;
    default: return (20);
  }
}

/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID OPTITimingValue (NPU npU)
{
  NPA	 npA = npU->npA;
  USHORT Data;
  NPCH	 p = (NPCH)&(npA->OPTI_IDEtim);
  UCHAR  Shift = 2 * (2 * npA->IDEChannel + npU->UnitId);

  PCIClockIndex = ReadIDEReg (npA, STRAP_REG) & 1 ? 1 : 0;

  Data = OPTI_ModeSets[PCIClockIndex][npU->InterfaceTiming];
  p[npU->UnitId] = Data & 0xFF;
  p[2] |= npU->InterfaceTiming << Shift;
  Data >>= 8;
  if (p[3] < (UCHAR)Data) p[3] = Data;

  if (npU->UltraDMAMode > 0) {
    npA->OPTI_Ultratim |= 1 << (Shift / 2);
#if 1
    npA->OPTI_Ultratim |= (npU->UltraDMAMode - 1) << (4 + Shift);
#endif
  }
}

/*-------------------------------*/
/*				 */
/* ProgramOPTIChip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramOPTIChip (NPA npA)
{
  UCHAR  i;
  UCHAR  PCIDataB;
  USHORT PCIData;
  USHORT Port = DATAREG + 1;
  NPCH	 p = (NPCH)&(npA->OPTI_IDEtim);

  /*************************************/
  /* Write IDE timing Register	       */
  /*************************************/

  if (npA->OPTI_IDEtim) {

    PCIDataB = RConfigB (PCI_OPTI_IDECONFIG);
    PCIDataB |= 0x23;
    WConfigB (PCI_OPTI_IDECONFIG, PCIDataB);

//  must be set up by BIOS !
//  WConfigB (PCI_OPTI_IDEFEATURES, 0x56 | 1);

    DISABLE
    PCIDataB = RConfigB (PCI_OPTI_IDETIM);
    inpw (Port);
    inpw (Port);
    outp (++Port, 0x03);

    outp (DATAREG + CNTRL_REG, 0x11);

    outp (DATAREG + MISC_REG, 0x00);
    i = 3 << (4 * npA->IDEChannel);
    if (p[0]) {
      outp (DATAREG + READ_REG, p[0]);
      outp (DATAREG + WRITE_REG, p[0]);

      PCIDataB = (PCIDataB & ~i) | (p[2] & i);
      WConfigBI (PCI_OPTI_IDETIM, PCIDataB);
    }

    i <<= 2;
    if (p[1]) {
      outp (DATAREG + MISC_REG, 0x01);
      outp (DATAREG + READ_REG, p[1]);
      outp (DATAREG + WRITE_REG, p[1]);

      PCIDataB = (PCIDataB & ~i) | (p[2] & i);
      WConfigBI (PCI_OPTI_IDETIM, PCIDataB);
    }

    outp (DATAREG + MISC_REG, p[3] | (UCHAR)((CurLevel > OPTI) ? 0x40 : 0));
    outp (DATAREG + CNTRL_REG, 0x95);

    outp (Port, 0x83);
    ENABLE
  }

  if (npA->Cap & CHIPCAP_ULTRAATA) {
    PCIData = RConfigW (PCI_OPTI_IDEULTRASEL);
    PCIData &= npA->IDEChannel ? ~0x0F0C : ~0x00F3;
    PCIData |= npA->OPTI_Ultratim;
    WConfigW (PCI_OPTI_IDEULTRASEL, PCIData);
  }
}
