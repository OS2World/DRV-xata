/**************************************************************************
 *
 * SOURCE FILE NAME = S506JMB.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2007
 *
 * DESCRIPTION : Adapter Driver JMicron routines.
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

#define JMB_CONFIG	 0x40	     /* JMB IDE configuration	  */
#define JMB_CONFIG1	 0x80	     /* JMB IDE configuration 1   */
#define JMB_TIM 	 0x44	     /* JMB IDE pulse timing (+1) */

/*----------------------------------------------------------------*/
/* Defines for IT_level 	    PCI Config Device IDs	  */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/

/* remap some structure members */

#define JMB_tim ((NPCH)&(npA->PIIX_IDEtim))

#define PciInfo (&npA->PCIInfo)
#define Adr PciInfo->PCIAddr

#define AHCI_GHC 4
#define AHCI_GHC_AHCIENABLED 1

#define SATA0	 0
#define SATA1	 1
#define PATA0	 8
#define PATA1	 9

BOOL NEAR AcceptJMB (NPA npA)
{
  NPC	npC = npA->npC;
  ULONG BAR5;
  ULONG Control, Control1;
  UCHAR Map0, Map1;

  Control  = GetRegD (Adr, JMB_CONFIG);
  Control1 = GetRegD (Adr, JMB_CONFIG1);

  // dual function mode ?
  if (Control & 0x202) {
    UCHAR IDEfnc = (Control & 0x02) >> 1;

    // function =  AHCI ?
    if ((Adr & 0x01) != IDEfnc) return (FALSE);

    GetBAR (npC->BAR + 5, Adr ^ 1, 5);
  }

  BAR5 = npC->BAR[5].Addr;

  // check if port is enabled
  if (!(Control & (UCHAR)(1 << (4 * npA->IDEChannel)))) return (FALSE);

  // AHCI mode enabled?
  if (BAR5 && (InD (BAR5 | AHCI_GHC) & AHCI_GHC_AHCIENABLED)) return (FALSE);

  sprntf (npA->PCIDeviceMsg, JMMsgtxt, MEMBER(npA).Device & 0xFFF);

  npC->numChannels = 2;

  switch (PciInfo->Level) {
    case 0 : // JMB360 single SATA
	     npC->numChannels = 1;
	     break;

    case 5 : // JMB368 dual PATA, no SATA
	     break;

    default: // mixed SATA/PATA
	     break;
  }

  if ((Control >> 16) & 0x80) {  // combined SATA+PATA or PATA+SATA
    npA->maxUnits = 2;
    Map0 = (SATA1 << 4) | SATA0;
    Map1 = PATA0;
  } else {
    npA->maxUnits = 1;
    Map0 = SATA0;
    Map1 = SATA1;
  }

  if ((Control1 >> 24) & 1) Map0 = PATA1;
  if (npA->IDEChannel != ((Control >> 22) & 1)) Map0 = Map1;

  switch (Map0) {
    case PATA0:
      if (!(Control & 0x08)) npA->Cap |= CHANCAP_CABLE80 | CHANCAP_SPEED;
      return (TRUE);

    case PATA1:
      npA->FlagsT |= ATBF_BIOSDEFAULTS;
      if (!((Control1 >> 16) & 0x08)) npA->Cap |= CHANCAP_CABLE80 | CHANCAP_SPEED;
      return (TRUE);
  }

  npA->SCR.Offsets = 0x3120;
  GenericSATA (npA);
  npA->Cap |= CHIPCAP_ATAPIDMA;
  npA->UnitCB[0].SStatus = GetAHCISCR (npA, Map0 & 0x07);
  if (npA->maxUnits > 1) {
    npA->UnitCB[1].SStatus = GetAHCISCR (npA, (Map0 >> 4) & 0x07);
    npA->UnitCB[1].FlagsT |= UTBF_NOTUNLOCKHPA;
  }

  return (TRUE);
}

USHORT NEAR GetJMBPio (NPA npA, UCHAR Unit) {
  UCHAR Timing;

  Timing = ReadConfigB (PciInfo, (UCHAR)(JMB_TIM + Unit));
  switch (Timing & 0x07) {
    case 4:  return  (4);
    case 3:  return  (6);
    default: return (20);
  }
}

/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/

// done implicitly by SETFEATURES

/*-------------------------------*/
/*				 */
/* ProgramJMBChip()		 */
/*				 */
/*-------------------------------*/

// done implicitly by SETFEATURES

