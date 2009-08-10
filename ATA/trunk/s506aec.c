/**************************************************************************
 *
 * SOURCE FILE NAME = S506AEC.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : Adapter Driver AEC routines.
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

#define PCIDEV_AEC6210 0x0005
#define PCIDEV_AEC6260 0x0006
#define PCIDEV_AEC6280 0x0009
#define PCIDEV_AEC6285 0x000A

#define PCI_AEC_IDETIM		0x40	    /* AEC IDE pulse timing (+2)     */
#define PCI_AEC6210_IDEULTRATIM 0x54	    /* AEC IDE ultra command timing  */
#define PCI_AEC6260_IDEULTRATIM 0x44	    /* AEC IDE ultra command timing  */
#define PCI_AEC_IDECABLE	0x49	    /* AEC IDE 80wire cable port     */
#define PCI_AEC_IDEENABLE	0x4A	    /* AEC IDE enable port	     */


/*----------------------------------------------------------------*/
/* Defines for AEC_level	     PCI Config Device IDs	  */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/
#define NOAEC		    0
#define AEC		    1	/* AEC with Ultra		*/
#define AECA		    2	/* AEC with ATA/66		*/
#define AECB		    3	/* AEC with ATA/133		*/

/* remap some structure members */

#define AEC_IDEtim PIIX_IDEtim
#define AEC_Ultratim PIIX_SIDEtim

#define PciInfo (&npA->PCIInfo)

BOOL NEAR AcceptAEC (NPA npA)
{
  USHORT val = 850;

  switch (PciInfo->CompatibleID) {
    case PCIDEV_AEC6260 :
      npA->Cap |= CHIPCAP_ATA66;
      val = 860;
      break;
    case PCIDEV_AEC6280 :
      npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;
      if (0x10 & inp ((npA->BMICOM & ~15) + 2))
	npA->Cap |= CHIPCAP_ATA133;
      val = 865;
      break;
  }
  sprntf (npA->PCIDeviceMsg, AEC62xMsgtxt, val);

  if (npA->Cap & CHIPCAP_ATA66) {
    /* determine the presence of a 80wire cable as per defined procedure */

    outp (((npA->BMICOM | 8) + 2), 0x10 | inp ((npA->BMICOM | 8) + 2)); //input!
    if (!((npA->IDEChannel ? 0x02 : 0x01) &
	  GetRegB (PciInfo->PCIAddr, PCI_AEC_IDECABLE))) // cable status bits
      npA->Cap |= CHANCAP_CABLE80;
  }

  return (TRUE);
}


USHORT NEAR GetAECPio (NPA npA, UCHAR Unit) {
  UCHAR  Reg = Unit + 2 * npA->IDEChannel;
  USHORT Timing;

  if (!(npA->Cap & CHIPCAP_ATA66)) {
    Timing = 0x11 * ReadConfigW (PciInfo, (UCHAR)(PCI_AEC_IDETIM + 2 * Reg));
  } else {
    Timing = 0x101 * ReadConfigB (PciInfo, (UCHAR)(PCI_AEC_IDETIM + Reg));
  }

  Timing = (Timing & 0xF00F) - 0x1001;
  return (2 + (Timing & 0x0F) + (Timing >> 12));
}

/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID AECTimingValue (NPU npU)
{
  NPA	npA = npU->npA;
  UCHAR value;

#define pB ((NPCH)&(npA->AEC_IDEtim))
#define pV ((NPUSHORT)&(npA->AEC_IDEtim))

  value = AEC62X0_ModeSets[npU->InterfaceTiming];
  if (!(npA->Cap & CHIPCAP_ATA66))
    pV[npU->UnitId] = (value * 17) & 0x0F0F;
  else
    pB[npU->UnitId] = value;

  if (npU->UltraDMAMode > 0) {
    value = npU->UltraDMAMode;
    if ((npA->Cap & CHIPCAP_ATA133) && (value == 6)) value = 5;
//    if (value > 6) value = 6;
    (UCHAR)npA->AEC_Ultratim |= value << (npU->UnitId * (npA->Cap & CHIPCAP_ATA66 ? 4 : 2));
  }
}

/*-------------------------------*/
/*				 */
/* ProgramAECChip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramAECChip (NPA npA)
{
  UCHAR i = 2 * npA->IDEChannel;
  UCHAR Data;

  /***************************************/
  /* Setup AEC timing for this channel	 */
  /***************************************/

  if (!(npA->Cap & CHIPCAP_ATA66)) {
    UCHAR Mask = i ? 0x0F : 0xF0;

    if (pV[0]) WConfigW ((UCHAR)(PCI_AEC_IDETIM + 2 * i + 0), pV[0]);
    if (pV[1]) WConfigW ((UCHAR)(PCI_AEC_IDETIM + 2 * i + 2), pV[1]);

    Data = (RConfigB (PCI_AEC6210_IDEULTRATIM) & Mask)
	 | (((UCHAR)npA->AEC_Ultratim * 0x11) & ~Mask);
    WConfigB (PCI_AEC6210_IDEULTRATIM, Data);
  } else {
    if (pB[0]) WConfigB ((UCHAR)(PCI_AEC_IDETIM + i + 0), pB[0]);
    if (pB[1]) WConfigB ((UCHAR)(PCI_AEC_IDETIM + i + 1), pB[1]);

    WConfigB ((UCHAR)(PCI_AEC6260_IDEULTRATIM + npA->IDEChannel),
		  (UCHAR)npA->AEC_Ultratim);

    if (npA->Cap & CHIPCAP_ATA100) {
//	WConfigBI (PCIREG_LATENCY, 0x90);
      WConfigB (PCI_AEC_IDEENABLE, (UCHAR)(0x80 | RConfigB (PCI_AEC_IDEENABLE))); //enable burst mode
    }
  }
}


#define AEC2_IDETIM		8	    /* AEC2 IDE pulse timing (+1)    */
#define AEC2_CMDTIM		10	    /* AEC2 IDE command timing	     */
#define AEC2_UDMA		11	    /* AEC2 IDE UDMA timing	     */

#define AEC2_Time PIIX_IDEtim
#define bTime2 ((NPCH)&(npA->AEC2_Time))

BOOL NEAR AcceptAEC2 (NPA npA)
{
  NPC	npC = npA->npC;
  UCHAR cable;

  npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133;
  sprntf (npA->PCIDeviceMsg, AEC62xMsgtxt, 867);

  DATAREG   = npC->BAR[0].Addr + 0x10 * npA->IDEChannel;
  DEVCTLREG = DATAREG + 14;
  BMCMDREG  = DATAREG + 0x40;
  CollectPorts (npA);

  npC->numChannels = 4;

  npA->AEC2_Time = InD (BMCMDREG + AEC2_IDETIM);

  /* determine the presence of a 80wire cable as per defined procedure */
  /* there is no defined procedure - infer cable from UDMA setting     */

  cable = ((bTime2[3] | 0x88) - 0x33) & 0x77;
  if (cable) npA->Cap |= CHANCAP_CABLE80;

  return (TRUE);
}

USHORT NEAR GetAEC2Pio (NPA npA, UCHAR Unit) {
  UCHAR Timing;

  Timing = bTime2[Unit];
  return ((Timing & 0x0F) + (Timing >> 4));
}

/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID AEC2TimingValue (NPU npU)
{
  NPA	npA = npU->npA;
  UCHAR mask = npU->UnitId ? 0x70 : 0x07;

  bTime2[npU->UnitId] = AEC62X0_ModeSets[npU->InterfaceTiming];
  bTime2[3] = (bTime2[3] & ~mask) | ((npU->UltraDMAMode * 0x11) & mask);
}

/*-------------------------------*/
/*				 */
/* ProgramAEC2Chip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramAEC2Chip (NPA npA)
{
  OutD (BMCMDREG + AEC2_IDETIM, npA->AEC2_Time);
}


