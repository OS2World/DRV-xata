/**************************************************************************
 *
 * SOURCE FILE NAME = S506CMD.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : Adapter Driver CMD 64X routines.
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

#define PCI_CMD_IDECFG		0x50	    /* CMD IDE configuration	     */
#define PCI_CMD_IDEENABLE	0x51	    /* CMD IDE enables		     */
#define PCI_CMD_IDECSR		0x79	    /* CMD IDE control status	     */
#define PCI_CMD_IDECMDTIM	0x52	    /* CMD IDE command timing	     */
#define PCI_CMD_IDEASU0 	0x53	    /* CMD IDE address setup 0	     */
#define PCI_CMD_IDEASU1 	0x55	    /* CMD IDE address setup 1	     */
#define PCI_CMD_IDEASU23	0x57	    /* CMD IDE address setup 2/3     */
#define PCI_CMD_IDETIM0 	0x54	    /* CMD IDE timing 0 	     */
#define PCI_CMD_IDETIM1 	0x56	    /* CMD IDE timing 1 	     */
#define PCI_CMD_IDETIM2 	0x58	    /* CMD IDE timing 2 	     */
#define PCI_CMD_IDETIM3 	0x5B	    /* CMD IDE timing 3 	     */
#define PCI_CMD_BURSTLEN	0x59	    /* CMD IDE burst len / 8	     */
#define PCI_CMD_MRDMODE 	0x71	    /* CMD IDE memory read mode      */
#define PCI_CMD_IDEUTIM0	0x73	    /* CMD IDE Ultra timing 0	     */
#define PCI_CMD_IDEUTIM1	0x7B	    /* CMD IDE Ultra timing 1	     */

/*----------------------------------------------------------------*/
/* Defines for CMD_level	     PCI Config Device IDs	  */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/
#define NOCMD		    0
#define CMD		    1	/* CMD w/o busmaster	       */
#define CMDA		    2	/* CMD w/o Ultra	       */
#define CMDB		    3	/* CMD with Ultra	       */
#define CMDC		    4	/* CMD with Ultra66	       */
#define CMDD		    5	/* CMD with Ultra100	       */

/* remap some structure members */

#define CMD_IDEtim PIIX_IDEtim
#define CMD_Ultratim PIIX_SIDEtim

#define PciInfo (&npA->PCIInfo)
#define PCIAddr PciInfo->PCIAddr

BOOL NEAR AcceptCMD (NPA npA)
{
  UCHAR  enable;
  USHORT CmdReg;

  sprntf (npA->PCIDeviceMsg, CMD64xMsgtxt, MEMBER(npA).Device);

  if (MEMBER(npA).Device < 0x643) {
    if (MEMBER(npA).Revision == 0)
      return (FALSE);
    PciInfo->Level = CMD;
    npA->Cap = 0;
  } else if (MEMBER(npA).Device < 0x648) {
    PciInfo->Level = CMDB;
    if (MEMBER(npA).Revision < 3) {
      PciInfo->Level = CMDA;
      npA->Cap &= ~CHIPCAP_ULTRAATA;
    }
  } else if (MEMBER(npA).Device == 0x648) {
    PciInfo->Level = CMDC;
    npA->Cap |= CHIPCAP_ATA66;
  } else if (MEMBER(npA).Device == 0x649) {
    PciInfo->Level = CMDD;
    npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;
  }

  if (npA->FlagsI.b.native)
    if (PciInfo->Level < CMDB)
      return (FALSE);
    else
      METHOD(npA).CheckIRQ = CMDCheckIRQ;

  if (PciInfo->Level <= CMDB)
    METHOD(npA).PCIFunc_InitComplete = GenericInitComplete;

  enable = GetRegB (PCIAddr, PCI_CMD_IDEENABLE);
  if (PciInfo->Level < CMDB)
    enable |= 1 << 2;
  if (PciInfo->Level < CMDA)
    enable |= 1 << 3;
  if (!(enable & (0x04 << npA->IDEChannel)) && !(npA->FlagsT & ATBF_BAY))
    return (FALSE);

  if (npA->Cap & CHIPCAP_ATA66) {
    /* determine the presence of a 80wire cable as per defined procedure */

    enable = GetRegB (PCIAddr, PCI_CMD_IDECSR);
    if (enable & (1 << npA->IDEChannel)) npA->Cap |= CHANCAP_CABLE80;
  }
  return (TRUE);
}


USHORT NEAR GetCMDPio (NPA npA, UCHAR Unit) {
  UCHAR Timing, Register;
  static UCHAR Reg[4] = {PCI_CMD_IDETIM0, PCI_CMD_IDETIM1, PCI_CMD_IDETIM2, PCI_CMD_IDETIM3};

  Timing = ReadConfigB (PciInfo, Reg[Unit + 2 * npA->IDEChannel]);
  Register = Timing >> 4;
  if (Register == 0) Register = 16;
  Timing = (1 + Timing) & 0x0F;
  if (Timing == 0) Timing = 1;
  else if (Timing == 1) Timing = 16;
  return (Timing + Register);
}

/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID CMDTimingValue (NPU npU)
{
  NPA	npA = npU->npA;
  UCHAR Unit = npU->UnitId;
  UCHAR value;
  UCHAR UDMA;

#define BYTE(x) ((NPCH)&(npA->x))

  value = CMD_ModeSets[PCIClockIndex][npU->InterfaceTiming];
  if ((npA->PCIInfo.Level == CMD) && ((value & 0x0F) == 0x0F))
    value = (value & 0xF0) | 0x01;
  BYTE(CMD_IDEtim)[Unit] |= value;

  BYTE(CMD_IDEtim)[2] = CMD_ModeSets[PCIClockIndex][0];

  UDMA = npU->UltraDMAMode;
  if (UDMA > 0) {
    if ((PCIClockIndex == 1) && (UDMA >= ULTRADMAMODE_2)) /* max.UDMA1 @ 25MHz */
      npU->UltraDMAMode = UDMA = ULTRADMAMODE_1;

    UDMA -= 1;
    value = CMD_UltraModeSets[PCIClockIndex][UDMA];
    value = (value << (2 * Unit)) | ((UDMA > 2 ? 5 : 1) << Unit);
    BYTE(CMD_Ultratim)[0] |= value;
  }
}

/*-------------------------------*/
/*				 */
/* ProgramCMDChip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramCMDChip (NPA npA)
{
  UCHAR PCIDataB;

  /*************************************/
  /* Write IDE timing Register	       */
  /*************************************/

  if (BYTE(CMD_IDEtim)[2]) WConfigB (PCI_CMD_IDECMDTIM, BYTE(CMD_IDEtim)[2]);
  WConfigB (PCI_CMD_BURSTLEN , 512 / 8);
  if (CurLevel == CMD) {
    PCIDataB = RConfigB (PCI_CMD_IDEENABLE) | 0x23;   // set fast timings
    WConfigB (PCI_CMD_IDEENABLE, PCIDataB);
  } else {
    PCIDataB = RConfigB (PCI_CMD_MRDMODE) & ~0x30;    // clear IRQs
    PCIDataB |= 0x01;	  // MEMORY READ MULTIPLE
    WConfigB (PCI_CMD_MRDMODE, PCIDataB);
  }

  if (npA->IDEChannel == 0) {
    if (CurLevel > CMDA) {
      PCIDataB = RConfigB (PCI_CMD_IDECFG);
      WConfigB (PCI_CMD_IDECFG, (UCHAR)(PCIDataB | 4));
    }
    PCIDataB = RConfigB (PCI_CMD_IDEENABLE) | 0xC0;
    if (!(npA->UnitCB[0].Flags & (UCBF_NOTPRESENT | UCBF_ATAPIDEVICE)))
      PCIDataB &= ~0x40;
    if ((npA->cUnits == 2) &&
	!(npA->UnitCB[1].Flags & (UCBF_NOTPRESENT | UCBF_ATAPIDEVICE)))
      PCIDataB &= ~0x80;
    WConfigB (PCI_CMD_IDEENABLE, PCIDataB);

    PCIDataB >>= 4;

    WConfigB (PCI_CMD_IDEASU0, 0xC0);
    WConfigB (PCI_CMD_IDEASU1, 0xC0);

    WConfigB (PCI_CMD_IDETIM0, BYTE(CMD_IDEtim)[0]);
    WConfigB (PCI_CMD_IDETIM1, BYTE(CMD_IDEtim)[1]);
    if (CurLevel > CMDA) WConfigB (PCI_CMD_IDEUTIM0, BYTE(CMD_Ultratim)[0]);

  } else {
    PCIDataB = (RConfigB (PCI_CMD_IDEASU23) & ~0xD0) | 0xCC;
    if (!(npA->UnitCB[0].Flags & (UCBF_NOTPRESENT | UCBF_ATAPIDEVICE)))
      PCIDataB &= ~0x04;
    if ((npA->cUnits == 2) &&
	!(npA->UnitCB[1].Flags & (UCBF_NOTPRESENT | UCBF_ATAPIDEVICE)))
      PCIDataB &= ~0x08;
    if (CurLevel > CMDA) PCIDataB |= 0x10;
    WConfigB (PCI_CMD_IDEASU23, PCIDataB);

    if (CurLevel > CMD) {
      WConfigB (PCI_CMD_IDETIM2, BYTE(CMD_IDEtim)[0]);
      WConfigB (PCI_CMD_IDETIM3, BYTE(CMD_IDEtim)[1]);
    }
    if (CurLevel > CMDA) WConfigB (PCI_CMD_IDEUTIM1, BYTE(CMD_Ultratim)[0]);
  }

  if (CurLevel == CMD) {
    if (PCIDataB & 0x04) npA->UnitCB[0].Flags &= ~UCBF_PIO32;
    if (PCIDataB & 0x08) npA->UnitCB[1].Flags &= ~UCBF_PIO32;
  }
}

/*-------------------------------*/
/*				 */
/* CMDCheckIRQ			 */
/*				 */
/*-------------------------------*/
int NEAR CMDCheckIRQ (NPA npA) {
  USHORT Port;
  UCHAR  Value, Mask;

  Port = (BMCMDREG & ~0x0F) | 0x01;
  Mask = 0x04 << npA->IDEChannel;
  DISABLE
  Value = inp (Port) & (~0x0C | Mask);
  if (Value & Mask) {
    if (BMCMDREG && (inp (BMCMDREG) & BMICOM_START)) {
      outp (BMCMDREG, npA->BM_CommandCode &= BMICOM_RD); /* turn OFF Start bit */
    }
    BMSTATUS = inp (BMSTATUSREG);
    STATUS = inp (STATUSREG);
    npA->Flags |= ACBF_BMINT_SEEN;
    if (BMSTATUS & BMISTA_INTERRUPT)
      outp (BMSTATUSREG, BMSTATUS & BMISTA_MASK);
    outp (Port, Value | 0x01);
    ENABLE
    return (1);
  } else {
    ENABLE
    return (0);
  }
}


