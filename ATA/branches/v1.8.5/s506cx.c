/**************************************************************************
 *
 * SOURCE FILE NAME = S506CX.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : Adapter Driver Cyrix routines.
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

#define CX_IDEPIOTIM	   0x20        /* CX IDE PIO pulse timing (+8) */
#define CX_IDEDMATIM	   0x24        /* CX IDE DMA pulse timing (+8) */

/*----------------------------------------------------------------*/
/* Defines for CX_level 	    PCI Config Device IDs	 */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/
#define NOCX		   0
#define CX		   1   /* CX w/o Ultra		     */
#define CXA		   2   /* CX with Ultra 	     */

/* remap some structure members */

#define CX_PIOtim ((PULONG)&(npA->PIIX_IDEtim))
#define CX_DMAtim (npA->DMAtim)

#define PciInfo (&npA->PCIInfo)

BOOL NEAR AcceptCx (NPA npA)
{
  // Cyrix5530 can do BM-DMA only to full cache lines!

  npA->ExtraPort = (npA->BMICOM & 0xFFF0) | 0x10;
  npA->ExtraSize = 0x70;
  return (TRUE);
}


USHORT NEAR GetCxPio (NPA npA, UCHAR Unit) {
  USHORT Timing;

  Timing = ReadConfigW (PciInfo, (UCHAR)(0x80 + CX_IDEPIOTIM + 8 * (Unit + 2 * npA->IDEChannel)));
  return (3 + ((Timing >> 12) & 0x0F) + ((Timing >> 8) & 0x0F) + ((Timing >> 4) & 0x0F));
}

/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID CxTimingValue (NPU npU)
{
  NPA npA = npU->npA;
  UCHAR UDMA, DMA, PIO;

  PIO = npU->CurPIOMode;
  PIO = (PIO >= 3) ? PIO - 2 : 0;
  CX_PIOtim[npU->UnitId] = CX_PIOModeSets[PCIClockIndex][PIO];

  UDMA = npU->UltraDMAMode;
  DMA = npU->CurDMAMode - 1;
  if (DMA > 1)	DMA = 0;
  if (UDMA > 0) DMA = 2 + (UDMA - 1);
  CX_DMAtim[npU->UnitId] = CX_DMAModeSets[PCIClockIndex][DMA];
}

/*-------------------------------*/
/*				 */
/* ProgramCxChip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramCxChip (NPA npA)
{
  UCHAR Reg;
  UCHAR Unit;

  /*************************************/
  /* Write IDE timing Register	       */
  /*************************************/

  Reg = 0x80 + CX_IDEPIOTIM + npA->IDEChannel * 16;
  for (Unit = 0; Unit < 2; Unit++, Reg += 8) {
    if (CX_PIOtim[Unit]) WConfigD (Reg, CX_PIOtim[Unit]);
    if (CX_DMAtim[Unit]) WConfigD (Reg, CX_DMAtim[Unit]);
  }
}
