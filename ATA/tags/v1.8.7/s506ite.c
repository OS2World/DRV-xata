/**************************************************************************
 *
 * SOURCE FILE NAME = S506ITE.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : Adapter Driver ITE routines.
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

#define ITE_CONFIG	 0x40	     /* ITE IDE configuration	      */
#define ITE_MODECTRL	 0x50	     /* ITE IDE mode control	      */
#define ITE_PIOTIM	 0x54	     /* ITE IDE PIO pulse timing (+4) */
#define ITE_UDMATIM	 0x56	     /* ITE IDE DMA pulse timing (+4) */

/*----------------------------------------------------------------*/
/* Defines for IT_level 	    PCI Config Device IDs	  */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/
#define ITEA		   0	    /* ITE RAID 		  */
#define ITEB		   1	    /* ITE non-RAID		  */

/* remap some structure members */

#define ITE_tim ((NPCH)&(npA->PIIX_IDEtim))

#define PciInfo (&npA->PCIInfo)
#define Addr PciInfo->PCIAddr


BOOL NEAR AcceptITE (NPA npA)
{
  USHORT Config;

  npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133;

  sprntf (npA->PCIDeviceMsg, ITEMsgtxt, MEMBER(npA).Device);

  if (GetRegB (Addr, ITE_MODECTRL) & 1) {
    // switch to bypass mode

    if (PciInfo->Level == ITEA)
      SetRegB (Addr, 0x5E, 0x01);      // reset local CPU

    // set to bypass mode

    SetRegB (Addr, ITE_MODECTRL, 0x80);
    SetRegB (Addr, ITE_MODECTRL, 0x00);

    DevHelp_ProcBlock ((ULONG)(PVOID)&AcceptITE, 50, 0);

    // set default PIO timing register of both channels

    SetRegB (Addr, ITE_PIOTIM + 0, 0xA3);
    SetRegB (Addr, ITE_PIOTIM + 4, 0xA3);

    SetRegD (Addr, 0x4C, 0x02040204);
    SetRegB (Addr, 0x42, 0x36);
  }

  Config = GetRegW (Addr, ITE_CONFIG);
  SetRegW (Addr, ITE_CONFIG, Config | 0xA000);

  if (!(Config & (npA->IDEChannel ? 8 : 4))) npA->Cap |= CHANCAP_CABLE80;
  return (TRUE);
}


/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID ITETimingValue (NPU npU)
{
  NPA	npA = npU->npA;
  UCHAR clock;

  clock = 0; // 66 MHz
  if ((npA->Cap & (CHANREQ_ATA133 | CHANREQ_ATA100)) == CHANREQ_ATA100)
    clock = 1; // 50 MHz

  ITE_tim[0] = ITE_ModeSets[clock][npU->InterfaceTiming];
  ITE_tim[1] = clock << (npA->IDEChannel + 1);

  if (npU->UltraDMAMode)
    ITE_tim[npU->UnitId + 2] = ITE_UltraModeSets[clock][npU->UltraDMAMode - 1];
  else if (npU->CurDMAMode)
    ITE_tim[1] |= (npU->UnitId + 1) << (npA->IDEChannel ? 5 : 3);

}

/*-------------------------------*/
/*				 */
/* ProgramITEChip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramITEChip (NPA npA)
{
  UCHAR DataB, Reg;

  /*************************************/
  /* Write IDE timing Register	       */
  /*************************************/

  DataB = RConfigB (ITE_MODECTRL) & (npA->IDEChannel ? ~0x64 : ~0x1A);
  WConfigB (ITE_MODECTRL, (UCHAR)(DataB | ITE_tim[1]));
  Reg = npA->IDEChannel ? ITE_PIOTIM + 4 : ITE_PIOTIM;
  WConfigB (Reg, ITE_tim[0]);
  if (((NPUSHORT)ITE_tim)[1]) WConfigW ((UCHAR)(Reg + 2), ((NPUSHORT)ITE_tim)[1]);
}
