/**************************************************************************
 *
 * SOURCE FILE NAME = S506HPT.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : Adapter Driver HPT routines.
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

#define PCI_HPT_IDETIM		0x40	    /* HPT IDE pulse timing (+4)     */
#define PCI_HPT_MISC1		0x50	    /* HPT IDE control register 1    */
#define PCI_HPT_MISC2		0x51	    /* HPT IDE control register 2    */
#define PCI_HPT_MISC3		0x52	    /* HPT IDE control register 3    */
#define PCI_HPT_MISC4		0x54	    /* HPT IDE control register 4    */
#define PCI_HPT_MISC5		0x55	    /* HPT IDE control register 5    */
#define PCI_HPT_MISC6		0x56	    /* HPT IDE control register 6    */
#define PCI_HPT_CTRL1		0x5A	    /* HPT IDE control status 1      */
#define PCI_HPT_CTRL2		0x5B	    /* HPT IDE control status 2      */
#define PCI_HPT_FLOW		0x5C	    /* HPT IDE pci clock frequency   */
#define PCI_HPT_FHIGH		0x5E	    /* HPT IDE pci clock frequency   */
#define PCI_HPT_FTEST		0x70	    /* HPT IDE pci clock frequency   */
#define PCI_HPT_FCNT		0x78	    /* HPT IDE pci clock frequency   */

#define IO_OFS 0x20  // offset from PCI config addr to BM IO port addr

#define IO_HPT_IDETIM  (IO_OFS + PCI_HPT_IDETIM)
#define IO_HPT_MISC1   (IO_OFS + PCI_HPT_MISC1)
#define IO_HPT_MISC2   (IO_OFS + PCI_HPT_MISC2)
#define IO_HPT_MISC3   (IO_OFS + PCI_HPT_MISC3)
#define IO_HPT_MISC4   (IO_OFS + PCI_HPT_MISC4)
#define IO_HPT_MISC5   (IO_OFS + PCI_HPT_MISC5)
#define IO_HPT_MISC6   (IO_OFS + PCI_HPT_MISC6)
#define IO_HPT_CTRL1   (IO_OFS + PCI_HPT_CTRL1)
#define IO_HPT_CTRL2   (IO_OFS + PCI_HPT_CTRL2)
#define IO_HPT_FCNT    (IO_OFS + PCI_HPT_FCNT)
#define IO_HPT_FLOW    (IO_OFS + PCI_HPT_FLOW)
#define IO_HPT_FHIGH   (IO_OFS + PCI_HPT_FHIGH)
#define IO_HPT_FTEST   (IO_OFS + PCI_HPT_FTEST)

/*----------------------------------------------------------------*/
/* Defines for HPT_level	     PCI Config Device IDs	  */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/
#define HPTA		    0	/* HPT with Ultra66	       */
#define HPTB		    1	/* HPT with Ultra100	       */
#define HPTC		    2	/* HPT with Ultra133	       */

/* remap some structure members */

#define HPT_IDE0tim PIIX_IDEtim
#define HPT_IDE1tim PIIX_SIDEtim

#define PCI_ID_HPT372A 0x0005
#define PCI_ID_HPT302  0x0006
#define PCI_ID_HPT371  0x0007
#define PCI_ID_HPT374  0x0008
#define PCI_ID_HPT372N 0x0009

#define PciInfo (&npA->PCIInfo)

UCHAR SetupHPTPLL (NPA npA) {
  USHORT Port, i, j;
  UCHAR  rc = 0;
  union {
    struct { UCHAR l, c, h, f; } x;
    ULONG v;
  } Divisor;
  static UCHAR PciTab[8]   = { 0x9C, 0xB0, 0xC8, 0xFF,	0x55, 0x70, 0x7F, 0xFF };
  static UCHAR ScaleTab[8] = { 0x20, 0x26, 0x2A, 0x3F,	0x17, 0x23, 0x27, 0x3C };

  Port = (npA->BMICOM & ~0x0F) + IO_HPT_FTEST;
  j = 0;		     // 33 MHz
  if (PciInfo->CompatibleID >= PCI_ID_HPT372N) j += 4;
  i = inpw (Port);
  if ((inpw (Port + 2) == 0xABCD) && ((i & 0xF000) == 0xE000)) {
    i &= 0x1FF;
    if (i >= PciTab[j]) j++; // 40 MHz
    if (i >= PciTab[j]) j++; // 50 MHz
    if (i >= PciTab[j]) j++; // 66 MHz
  }
  j &= 3;
  if (npA->Cap & CHIPCAP_ATA133) j += 4;

  Divisor.x.l = ScaleTab[j];
  Divisor.x.h = ScaleTab[j] + (j & 2) + 2;
  Divisor.x.c = 1;
  Divisor.x.f = 0;

  Port += IO_HPT_CTRL2 - IO_HPT_FTEST;

  for (i = 0; i++ < 6;) {
    OutD (Port + 1, Divisor.v);
    outp (Port, 0x21);
    for (j = 0; j < 0x5000; j++) if (inp (Port) & 0x80) goto test_stable;
  not_stable:
    if (i & 1) {
      Divisor.x.l -= i >> 1;
      Divisor.x.h -= i >> 1;
    } else {
      Divisor.x.l += i >> 1;
      Divisor.x.h += i >> 1;
    }
    continue;
  test_stable:
    for (j = 0; j < 0x1000; j++) if (!(inp (Port) & 0x80)) goto not_stable;
    rc = (npA->Cap & CHIPCAP_ATA133 ? 2 : 1);
  }

  Divisor.x.c = 0;
  OutD (Port + 1, Divisor.v);

  return (rc);
}


BOOL NEAR AcceptHPT (NPA npA)
{
  USHORT val = 0, Reg;
  UCHAR  Cable, save;
  static UCHAR gotPLL;

  npA->ExtraPort = (npA->BMICOM & 0xFFF0) | 0x10;
  npA->ExtraSize = 0xF0;
  // docs say scatter gather needs to be 32bit word aligned !
  PciInfo->Level = HPTA;
  npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;

  if (PciInfo->CompatibleID >= PCI_ID_HPT372A)
    PciInfo->Ident.Revision = PciInfo->CompatibleID;

  switch (PciInfo->Ident.Revision) {
    case 2:
      val += 2;  // 368
    case 1:
    case 0:
      val += 366;
      METHOD(npA).SetupDMA = HPT36xSetupDMA;
      METHOD(npA).ErrorDMA = HPT36xErrorDMA;
      METHOD(npA).PreReset = HPT36xPreReset;
      MEMBER(npA).CfgTable = CfgHPT36x;
      MEMBER(npA).SGAlign  = 2;  // HPT366 docs say scatter gather is SFF8038i compliant
      npA->HardwareType = HPT36x;
      npA->BMSize = 0x100;
      npA->ExtraPort = npA->ExtraSize = 0;
      npA->Cap &= ~CHIPCAP_ATA100;
      PciInfo->npC->numChannels = 1;
      break;

    case 3:
    case 4:
      val = 370; goto HPT37x;  // do *not* use PLL!

    case 8:
      val += 2;   // 374
    case 5:
    case 9:
      val += 1;   // 372
    case 7:
      val += 69;  // 371
    case 6:
      val += 302; // 302
    default:
      npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133;
      if (npA->IDEChannel == 0) gotPLL = SetupHPTPLL (npA);
      PciInfo->Level += gotPLL;

    HPT37x:
     // use at least 1 clock pre_time !

     Reg = npA->BMICOM + IO_HPT_IDETIM + 2;
     outp (Reg + 0, 0x40 | inp (Reg + 0));
     outp (Reg + 4, 0x40 | inp (Reg + 4));
  }
  sprntf (npA->PCIDeviceMsg, HPT36xMsgtxt, val);

  /* determine the presence of a 80wire cable as per defined procedure */

  // enable cable status ports, BIOS may have disabled them !
  if ((val == 374) && (PciInfo->PCIAddr & 1)) {
    Reg = npA->BMICOM + (npA->IDEChannel ? IO_HPT_MISC6 + 1 - 8
				     : IO_HPT_MISC3 + 1);
    outp (Reg, (UCHAR)(0x80 | (save = inp (Reg))));
  } else {
    Reg = npA->BMICOM | IO_HPT_CTRL2;
    outp (Reg, (UCHAR)(~0x01 & (save = inp (Reg))));
  }

  // enable interrupts, BIOS may have diabled them !
  // no - do that later: some hardware may fire interrupts continuously!

  Cable = inp (npA->BMICOM | IO_HPT_CTRL1); // Cable status bits
  outp (Reg, save);

#if TRACES
  if (Debug & 2) TraceStr ("C:%02X,", Cable);
#endif
  if (!(Cable & (2 >> npA->IDEChannel))) npA->Cap |= CHANCAP_CABLE80;
  return (TRUE);
}



/*--------------------------------------------*/
/*					      */
/* ProbeHPTClock()			      */
/*					      */
/*					      */
/*--------------------------------------------*/
UCHAR NEAR HPTClkIdx (NPA npA)
{
  if (npA->HardwareType == HPT36x) {
    UCHAR Value;

    if (PCIClock) return (PCIClockIndex);

    Value = inp (BMBASE + IO_HPT_IDETIM + 1);
    Value = (Value & 0x0F) + (Value >> 4);
    if (Value < 16)	  return (1);  // 25 MHz
    else if (Value <= 20) return (0);  // 33 MHz
    else		  return (2);  // 40 MHz
  } else {
    return (npA->PCIInfo.Level);
  }
}


USHORT NEAR GetHPTPio (NPA npA, UCHAR Unit) {
  UCHAR Timing;
  USHORT Reg = BMBASE + IO_HPT_IDETIM + 4 * Unit;

  if (npA->HardwareType == HPT37x) {
    Reg += 8 * npA->IDEChannel;
  }

  Timing = inp (Reg);
  return ((Timing & 0x0F) + (Timing >> 4));
}


/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID HPTTimingValue (NPU npU)
{
  NPA	npA = npU->npA;
  ULONG *pV;
  UCHAR UDMA, PIO, Clock;

#define pB ((NPCH)pV)

  pV = npU->UnitId ? &(npA->HPT_IDE1tim) : &(npA->HPT_IDE0tim);

  Clock = HPTClkIdx (npA);

  PIO = npU->InterfaceTiming;
  *pV = ((npA->HardwareType == HPT37x) ? HPT37x_ModeSets : HPT_ModeSets)[Clock][PIO];

  if (npU->Flags & UCBF_BM_DMA) {
    UDMA = npU->UltraDMAMode;
    if (UDMA > 0) {
      pB[3] |= 0x10;
      if (npA->HardwareType == HPT37x) {
	pB[2] |= HPT37x_UltraModeSets[Clock][UDMA - 1] << 2;
      } else {
	pB[2] |= HPT_UltraModeSets[Clock][UDMA - 1] << 0;
      }
    } else {
      pB[3] |= 0x20;
    }
  }
}

/*-------------------------------*/
/*				 */
/* ProgramHPTChip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramHPTChip (NPA npA)
{
  UCHAR  DataB;
  USHORT Data;
  UCHAR  Register = PCI_HPT_IDETIM + 8 * npA->IDEChannel;

  /*************************************/
  /* Write IDE timing Register	       */
  /*************************************/

  if (npA->HPT_IDE0tim) WConfigD (Register, npA->HPT_IDE0tim);
  if (npA->HPT_IDE1tim) WConfigD ((UCHAR)(Register + 4), npA->HPT_IDE1tim);

  if (npA->HardwareType == HPT36x) {
    DataB = RConfigB (PCI_HPT_MISC2) & ~0x80;
    WConfigB (PCI_HPT_MISC2, DataB);
  } else {
    Register = npA->IDEChannel ? PCI_HPT_MISC4 : PCI_HPT_MISC1;

    Data = (RConfigW (Register) & ~0x200) | 0x132;
    WConfigW (Register, Data);

    DataB = RConfigB (PCI_HPT_CTRL1) & ~0x10;
    WConfigB (PCI_HPT_CTRL1, DataB);

    WConfigB (PCI_HPT_CTRL2, (UCHAR)(CurLevel == HPTA ? 0x23 : 0x21));
  }

  DataB = RConfigB (PCIREG_MIN_GRANT);
  if (DataB < 0x08)
    WConfigBI (PCIREG_MIN_GRANT, 0x08);

  DataB = RConfigB (PCIREG_MAX_LAT);
  if (DataB < 0x08)
    WConfigBI (PCIREG_MAX_LAT, 0x08);
}

/*---------------------------------------------*/
/* HPTCheckIRQ				       */
/*---------------------------------------------*/

int NEAR HPTCheckIRQ (NPA npA) {
  UCHAR Value;

  DISABLE
  BMSTATUS = inp (BMSTATUSREG);
  if (BMSTATUS & BMISTA_INTERRUPT) {
    if (npA->BM_CommandCode & BMICOM_START) {
      if (!(BMSTATUS & BMISTA_ACTIVE))
	outp (BMCMDREG, npA->BM_CommandCode &= BMICOM_RD); /* turn OFF Start bit */
      else
	npA->npU->DeviceCounters.TotalChipStatus++;
    }
    STATUS = inp (STATUSREG);
    npA->Flags |= ACBF_BMINT_SEEN;
    outp (BMSTATUSREG, BMSTATUS & BMISTA_MASK);
    ENABLE

    if (STATUS & FX_BUSY) CheckBusy (npA);
    if (STATUS & FX_ERROR) ERROR = InB (ERRORREG);

    return (1);
  } else {
    ENABLE
    return (0);
  }
}

VOID NEAR HPTEnableInt (NPA npA) {
  USHORT Port = BMBASE | IO_HPT_CTRL1;
  outp (Port, ~0x10 & inp (Port));
}

VOID NEAR HPT36xPreReset (NPA npA)
{
  outp (BMCMDREG + IO_HPT_MISC2, 0x73);
}

VOID NEAR HPT37xPreReset (NPA npA)
{
  USHORT Port;

  Port = BMCMDREG + (npA->IDEChannel ? IO_HPT_MISC4 - 8 : IO_HPT_MISC1);
  outp (Port, 0x37);
  Port++;
  outp (Port, inp (Port) & ~0x02);
  HPTEnableInt (npA);
}

/*---------------------------------------------*/
/* HPTSetupDMA				       */
/*---------------------------------------------*/

VOID NEAR HPT36xSetupDMA (NPA npA)
{
  outp (BMSTATUSREG, BMISTA_ERROR |  /* clear Error Bit       */
		 BMISTA_INTERRUPT |  /* clear INTR flag       */
		 npA->BMStatus);

  outp (BMCMDREG | IO_HPT_MISC2, 0x73);
  OutD (BMDTPREG, npA->ppSGL);
}

VOID NEAR HPT37xSetupDMA (NPA npA)
{
  outp (BMSTATUSREG, BMISTA_ERROR |  /* clear Error Bit       */
		 BMISTA_INTERRUPT |  /* clear INTR flag       */
		 npA->BMStatus);

  outp (BMCMDREG + (npA->IDEChannel ? IO_HPT_MISC4 - 8 : IO_HPT_MISC1), 0x37);
  OutD (BMDTPREG, npA->ppSGL);
}

/*--------------------------------------------*/
/* HPTErrorDMA				      */
/*--------------------------------------------*/

VOID NEAR HPT36xErrorDMA (NPA npA)
{
  USHORT Port;
  outp (BMSTATUSREG, BMISTA_ERROR |  /* clear Error Bit       */
		     npA->BMStatus); /* clear INTR flag       */
  Port = BMCMDREG | IO_HPT_MISC1;
  outp (Port, 0x37);
  outp (Port+1, 0x73);
}

VOID NEAR HPT37xErrorDMA (NPA npA)
{
  outp (BMSTATUSREG, BMISTA_ERROR |  /* clear Error Bit       */
		     npA->BMStatus); /* clear INTR flag       */
  outp (BMCMDREG + (npA->IDEChannel ? IO_HPT_MISC4 - 8 : IO_HPT_MISC1), 0x37);
}


