/**************************************************************************
 *
 * SOURCE FILE NAME = S506SIS.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 2000-2006
 *
 * DESCRIPTION : Adapter Driver SIS routines.
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

#define PCI_SIS_IDETIM		0x40	    /* SIS IDE pulse timing (+2)     */
#define PCI_SIS_IDECMDTIM	0x48	    /* SIS IDE command timing	     */
#define PCI_SIS_IDESTATUS	0x48	    /* SIS IDE status register (530) */
#define PCI_SIS_IDEENABLE	0x4A	    /* SIS IDE enables		     */
#define PCI_SIS_IDECTRL1	0x4B	    /* SIS IDE gen. control 1	     */
#define PCI_SIS2_CHAN0		0x50	    /* SIS IDE channel control0      */
#define PCI_SIS2_CHAN1		0x52	    /* SIS IDE channel control1      */
#define PCI_SIS2_IDECTRL	0x57	    /* SIS IDE gen. control	     */
#define PCI_SIS2_IDETIM 	0x70	    /* SIS IDE pulse timing (+4)     */

/*----------------------------------------------------------------*/
/* Defines for SIS_level	     PCI Config Device IDs	  */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/
#define NOSIS		    0
#define SIS		    1	/* SIS w/o Ultra	       */
#define SISA		    2	/* SIS with Ultra (maxlat=32)  */
#define SISB		    3	/* SIS with Ultra	       */
#define SISC		    4	/* SIS with Ultra66	       */
#define SISD		    5	/* SIS with Ultra100 (old)     */
#define SISE		    6	/* SIS with Ultra100	       */
#define SISF		    7	/* SIS with Ultra133 (old)     */
#define SISG		    8	/* SIS with Ultra133	       */
#define SISH		    9	/* SIS SATA		       */

/* -------------------------------------------------------------------------
		  SiS EIDE Timing register layout

	| 0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  |
--------|----------------------------------------------------------------|
ATA/33	| Recovery time | rsrvd | test |   Active  | rsrvd | Ultra | Uen |
--------|----------------------------------------------------------------|
ATA/66	| Recovery time | rsrvd | test |   Active  | r.|   Ultra   | Uen |
--------|----------------------------------------------------------------|
ATA/100 | Recovery time |  Active  | r.|     Ultra     |   rsrvd   | Uen |
ATA/133 |----------------------------------------------------------------|

Ultra DMA time quantum:

ATA/33 : PCI clock	(30 ns)
ATA/66 : PCI clock / 2	(15 ns)
ATA/100: 10 ns
ATA/133: 7.5 ns

----------------------------------------------------------------------------*/

/* remap some structure members */

#define SIS_IDEtim PIIX_IDEtim
#define SIS_IDE1tim PIIX_SIDEtim

#define PciInfo (&npA->PCIInfo)
#define PCIAddr PciInfo->PCIAddr

BOOL NEAR AcceptSIS (NPA npA)
{
  UCHAR save57, PCR, SATAmul = 1;

  SetRegB (PCIAddr, PCI_SIS2_IDECTRL,			       // change DevId
	   (UCHAR)(~0x80 & (save57 = GetRegB (PCIAddr, PCI_SIS2_IDECTRL))));
  MEMBER(npA).Device = GetRegW (PCIAddr, PCIREG_DEVICE_ID);
  sprntf (npA->PCIDeviceMsg, SiS5513Msgtxt, MEMBER(npA).Device);

  switch (MEMBER(npA).Device) {
    case 0x1184:
    case 0x1180:
      PCR = GetRegB (PCIAddr, 0x67);
      if (PCR & 0x10) {
	npA->maxUnits = 1;
	SATAmul = 2;
	goto SATAcommon;
      }
    /* fall through */

    case 0x1183:
    case 0x1182:
    case 0x0183:
    case 0x0182:
      npA->maxUnits = 2;
      goto SATAcommon;
      break;

    case 0x0181:
    case 0x0180:
      PCR = GetRegB (PCIAddr, 0x90);
      if (PCR & 0x30) {
	if (!npA->IDEChannel) goto SiS5518common; // combined PATA & SATA
	npA->maxUnits = 2;
      } else {
	npA->maxUnits = 1;
	SATAmul = 2;
      }

    SATAcommon: {
      UCHAR Port;
      NPU npU = npA->UnitCB;

      PciInfo->Level = SISH;
      GenericSATA (npA);
      Port = npA->IDEChannel * SATAmul;
      if (SSTATUS) {
	SSTATUS = npA->npC->BAR[5].Addr + Port * 0x20;
	npU++;
	Port++;
	SSTATUS = npA->npC->BAR[5].Addr + Port * 0x20;
      } else {

	SSTATUS = ((ULONG)PCICONFIG (0xC0 + Port * 0x10) << 16) | PCIAddr;
	SERROR	 = SSTATUS | 0x40000;
	SCONTROL = SSTATUS | 0x80000;
	if (npA->maxUnits > 1) {
	  npU++;
	  Port++;
	  SSTATUS = ((ULONG)PCICONFIG (0xC0 + Port * 0x10) << 16) | PCIAddr;
	  SERROR   = SSTATUS | 0x40000;
	  SCONTROL = SSTATUS | 0x80000;
	}
	npA->SCR.Offsets = 0;  // do *not* allocate SCR ports by default method
      }
    }
      break;

    case 0x5517:
    SiS5517:
      // south bridge 961
      npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;
      PciInfo->Level = SISE;
      if ((GetRegB (PCIAddr & ~0x07, PCIREG_REVISION) >= 0x10) &&
	  (GetRegB (PCIAddr, 0x49) & 0x80)) {
	// south bridge 961 rev > 0x10 with ATA/133 enabled
	npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133;
	PciInfo->Level = SISF;
      }
      break;

    case 0x5518:
      // south bridge 962++
      npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133;
      PciInfo->Level = SISG;

      if (!(GetRegB (PCIAddr, (UCHAR)(npA->IDEChannel ? PCI_SIS2_CHAN1 : PCI_SIS2_CHAN0)) & 2))
	return (FALSE);

    SiS5518common:
      METHOD(npA).CfgTable    = CfgSIS2;
      METHOD(npA).GetPIOMode  = GetGenericPio;
      METHOD(npA).TimingValue = SIS2TimingValue;
      METHOD(npA).ProgramChip = ProgramSIS2Chip;

      save57 = (save57 & ~0x80) | 0x40; 		    // remap registers
      break;

    case 0x5513:
    default:

      if (MEMBER(npA).Revision >= 0xD0) { // check for SiS timing register layout
	USHORT Save, Set, Get;
#define ID xBridgeDevice
#define REV xBridgeRevision

	PciInfo->Level = (ID >= 0x5581) ? SISA : SISB;	  // default to ATA/33

	Save = GetRegW (PCIAddr, PCI_SIS_IDETIM);
	Set = Save | 0x7000;
	SetRegW (PCIAddr, PCI_SIS_IDETIM, Set);
	Get = GetRegW (PCIAddr, PCI_SIS_IDETIM);
	SetRegW (PCIAddr, PCI_SIS_IDETIM, Save);

	if ((Get & 0x7000) == 0) {    // bits 12-14 fixed at all zero -> ATA/100
	  PciInfo->Level = SISE;
	  npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;
	  if (ID >= 0x645) {			  // ID is hex !!!
	    UCHAR save4A;

	    SetRegB (PCIAddr, PCI_SIS_IDEENABLE, (UCHAR)(0x10 | // change DevId
		     (save4A = GetRegB (PCIAddr, PCI_SIS_IDEENABLE))));
	    MEMBER(npA).Device = GetRegW (PCIAddr, PCIREG_DEVICE_ID);
	    if (0x5517 == MEMBER(npA).Device) goto SiS5517;
	    SetRegB (PCIAddr, PCI_SIS_IDEENABLE, save4A);
	  }
	} else if (((Set ^ Get) & 0x1000) == 0) { // bit 12 fixed at zero &
	  PciInfo->Level = SISC;		  // bits 13/14 changeable -> ATA/66
	  npA->Cap |= CHIPCAP_ATA66;
	  if ((ID == 0x730) ||			  // SiS730
	     ((ID == 0x630) && (REV >= 0x30)) ||  // SiS630S/ET
	      (ID == 0x550)) {			  // SiS550 have old layout
	    npA->Cap |= CHIPCAP_ATA100; 	  // but are ATA/100!
	  }
	}

#if TRACES
      if (Debug & 2) TraceStr ("Set:%04X Get:%04X -> level:%d", Set, Get, PciInfo->Level);
#endif
      } else {
	PciInfo->Level = SIS;
	npA->Cap &= ~CHIPCAP_ULTRAATA;
      }
      break;
  }

  SetRegB (PCIAddr, PCI_SIS2_IDECTRL, save57);

  if (PciInfo->Level < SISG) {
    if (!(GetRegB (PCIAddr, PCI_SIS_IDEENABLE) & (npA->IDEChannel ? 4 : 2)))
      return (FALSE);
  }

  if (npA->Cap & CHIPCAP_ATA66) {
    /* determine the presence of a 80wire cable as per defined procedure */

    if (!((npA->IDEChannel ? 0x20 : 0x10) & GetRegB (PCIAddr, PCI_SIS_IDESTATUS)))
      npA->Cap |= CHANCAP_CABLE80;
  }

  if (PciInfo->Level <= SISB) npA->IRQDelay = -1; // delayed interrupt
  return (TRUE);
}



USHORT NEAR GetSISPio (NPA npA, UCHAR Unit) {
  USHORT Timing;
  static UCHAR Rec[16] = {12,1,2,3,4,5,6,7,8,9,10,11,13,14,15,15};
  static UCHAR Act[8]  = {8,1,2,3,4,5,6,12};

  Timing = ReadConfigW (&npA->PCIInfo, (UCHAR)(PCI_SIS_IDETIM + 2 * (Unit + 2 * npA->IDEChannel)));
  return (Rec[Timing & 0x0F] +
	  Act[(Timing >> (npA->Cap & CHIPCAP_ATA100 ? 4 : 8)) & 0x07]);
}

/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID SISTimingValue (NPU npU)
{
  NPA npA = npU->npA;
  NPUSHORT pV = ((NPUSHORT)&(npA->SIS_IDEtim));

#define pB ((NPCH)pV)

  pV += npU->UnitId;
  *pV = (npA->PCIInfo.Level >= SISE ? 0x007F : 0x070F)
      & SIS_ModeSets[PCIClockIndex][npU->InterfaceTiming];

  if (npU->UltraDMAMode > 0) {
    USHORT UDMA = npU->UltraDMAMode - 1;

    if (npA->PCIInfo.Level >= SISE)
      pB[1] = (npA->Cap & CHIPCAP_ATA133 ? SIS_Ultra133ModeSets
					 : SIS_Ultra100ModeSets)[UDMA];
    else
      pB[1] |= (npA->Cap & CHIPCAP_ATA66 ? SIS_Ultra66ModeSets
					 : SIS_UltraModeSets)[PCIClockIndex][UDMA];
  }
}

VOID SIS2TimingValue (NPU npU)
{
  NPA npA = npU->npA;
  ULONG *pV;

#define pB ((NPCH)pV)
#define pS ((NPUSHORT)pV)

  pV = npU->UnitId ? &(npA->SIS_IDE1tim) : &(npA->SIS_IDEtim);

  if (npU->UltraDMAMode > 0)
    *pS = SIS2_UltraModeSets[npU->UltraDMAMode - 1];

  *pV |= (*pB & 8 ? SIS2_133ModeSets : SIS2_100ModeSets)[npU->InterfaceTiming];
  if (!(npU->Flags & UCBF_ATAPIDEVICE)) *pB |= 1;   // prefetch enable
}

/*-------------------------------*/
/*				 */
/* ProgramSISDhip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramSISChip (NPA npA)
{
  WConfigB (PCI_SIS_IDEENABLE, (UCHAR)(0x80 | RConfigB (PCI_SIS_IDEENABLE)));

  if (CurLevel == SISA) WConfigBI (PCIREG_LATENCY, 0x20);
  if (CurLevel == SISC) WConfigBI (PCIREG_LATENCY, 0x10);

  WConfigD ((UCHAR)(npA->IDEChannel ? PCI_SIS_IDETIM + 4 : PCI_SIS_IDETIM), npA->SIS_IDEtim);
}

VOID ProgramSIS2Chip (NPA npA)
{
  UCHAR Reg;

  Reg = npA->IDEChannel ? PCI_SIS2_IDETIM + 8 : PCI_SIS2_IDETIM;
  if ((UCHAR)npA->SIS_IDEtim)  WConfigD ((UCHAR)(Reg + 0), npA->SIS_IDEtim);
  if ((UCHAR)npA->SIS_IDE1tim) WConfigD ((UCHAR)(Reg + 4), npA->SIS_IDE1tim);
}

