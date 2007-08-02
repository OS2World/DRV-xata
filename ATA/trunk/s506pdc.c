/**************************************************************************
 *
 * SOURCE FILE NAME = S506PDC.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2007
 *
 * DESCRIPTION : Adapter Driver Promise PDC202xx routines.
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

#define PCIDEV_PDC33   0x4D33
#define PCIDEV_PDC66   0x4D38
#define PCIDEV_PDC100  0x4D30
#define PCIDEV_PDC101  0x0D30
#define PCIDEV_PDC102  0x4D68
#define PCIDEV_PDC103  0x6268
#define PCIDEV_PDC133  0x4D69
#define PCIDEV_PDC134  0x1275
#define PCIDEV_PDC135  0x5275

#define PCI_PDC_IDEENABLE	0x50	    /* PDC IDE enables		     */
#define PCI_PDC_IDESTATUS	0x51	    /* PDC IDE cable status	     */
#define PCI_PDC_IDETIM		0x60	    /* PDC IDE timings (+4)	     */
#define TX2_PDC_IDETIM		0x0C	    /* PDC IDE timings (+8) TX2      */
#define TX2_PDC_MM_IDETIM0	0x110C	    /* PDC IDE timings (+8) TX2      */
#define TX2_PDC_MM_IDETIM1	0x120C	    /* PDC IDE timings (+8) TX2      */
#define PDC_MMC0		0x1A	    /* PDC master mode control 0     */
#define PDC_MMC1		0x1B	    /* PDC master mode control 1     */
#define PDC_SCR1		0x1D	    /* PDC system control register 1 */
#define PDC_SCR3		0x1F	    /* PDC system control register 3 */
#define PDC_ATAPICOUNTER	0x20	    /* PDC ATAPI bm counter (+4)     */

#define SYNC_ERRDY_EN 0xC0
#define SYNC_IN       0x80  // different on master/slave
#define ERRDY_EN      0x40  // different on master/slave
#define IORDY_EN      0x20  // PIO IOREADY
#define PREFETCH_EN   0x10  // PIO Prefetch

#define IORDYp_NO_SPEED 0x4F
#define SPEED_DIS	0x0F

#define DMARQp	  0x80
#define IORDYp	  0x40
#define DMAR_EN   0x20
#define DMAW_EN   0x10

/*----------------------------------------------------------------*/
/* Defines for PDC_level	     PCI Config Device IDs	  */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/
#define NOPDC		    0
#define PDC		    1	/* PDC				*/
#define PDCA		    2	/* PDC with Ultra/66		*/
#define PDCB		    3	/* PDC with Ultra/100		*/
#define PDCC		    4	/* PDC with Ultra/100 TX2	*/
#define PDCD		    5	/* PDC with Ultra/133		*/

/* remap some structure members */

#define PDCs CHIPs
#define PDC_PIOtim PIIX_IDEtim
#define PDC_DMAtim PIIX_SIDEtim
typedef USHORT (*pSetting)[2];

#define PciInfo (&npA->PCIInfo)

BOOL NEAR AcceptPDC (NPA npA)
{
  UCHAR val;

  npA->ExtraPort = (npA->BMICOM & 0xFFF0) | 0x10;
  npA->ExtraSize = 0x30;
  npA->Cap &= ~(CHIPCAP_ATAPIPIO32 | CHIPCAP_ATAPIDMA);

  switch (PciInfo->CompatibleID) {
    case PCIDEV_PDC66:
      npA->Cap |= CHIPCAP_ATA66;
      val = 66;
      break;
    case PCIDEV_PDC100:
      npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;
      npA->IRQDelay = 10;
      val = 100;
      break;
    default:
      npA->ExtraSize = 0x10;
      npA->Cap &= ~(CHIPCAP_ATAPIDMA | CHIPCAP_LBA48DMA);
      val = 33;
  }

  sprntf (npA->PCIDeviceMsg, PDCxMsgtxt, val);

  if (npA->Cap & CHIPCAP_ATA66) {
    /* determine the presence of a 80wire cable as per defined procedure */

    if (!(GetRegB (PciInfo->PCIAddr, PCI_PDC_IDESTATUS) &
	 (npA->IDEChannel ? 0x08 : 0x04)))	      // cable status bit
      npA->Cap |= CHANCAP_CABLE80;
  }
  return (TRUE);
}

BOOL NEAR AcceptPDCTX2 (NPA npA)
{
  npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;

  outp (npA->BMICOM | 9, 3);
  if ((inp (npA->BMICOM | 11) & 0x1F) < 10) {
    npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133;
  }

  sprntf (npA->PCIDeviceMsg, PDCxMsgtxt,
	  (PciInfo->CompatibleID == PCIDEV_PDC133 ? 133 : 100));

  /* determine the presence of a 80wire cable as per defined procedure */

  outp (npA->BMICOM + 1, 0x0B);
  if (!(inp (npA->BMICOM + 3) & 0x04))		     // cable status bit
    npA->Cap |= CHANCAP_CABLE80;

  return (TRUE);
}


USHORT NEAR GetPDCPio (NPA npA, UCHAR Unit) {
  return (0x1F & ReadConfigB (PciInfo, (UCHAR)(PCI_PDC_IDETIM + 1 + 4 * (Unit + 2 * npA->IDEChannel))));
}

/*--------------------------------------------*/
/*					      */
/* SetupPDC()				      */
/*					      */
/*					      */
/*--------------------------------------------*/
VOID NEAR SetupPDCTX2 (NPA npA)
{
  if (SetupCommonPre (npA)) return;
  CalculateAdapterTiming (npA);
  SetupCommonPost (npA);
  ProgramPDCChipTX2 (npA);
}


/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID PDCTimingValue (NPU npU)
{
  NPA	 npA = npU->npA;
  USHORT value = 0;
  UCHAR  Mode;
  NPUSHORT p, q;

  Mode = npU->CurPIOMode;
  Mode = (Mode >= 3) ? Mode - 2 : 0;

  if (Mode > 0) value |= 0x4000 | IORDY_EN;
  if (!(npU->Flags & UCBF_ATAPIDEVICE)) value |= 0x8000 | PREFETCH_EN;

  if (npA->HardwareType == PromiseTX) {
    value &= 0xFF00;

    p = PDC_100ModeSets;
    q = PDC_DMA100ModeSets;
    if (npA->Cap & CHIPCAP_ATA133) {
      p = PDC_133ModeSets;
      q = PDC_DMA133ModeSets;
    }
    (*(pSetting)&(npA->DMAtim[0]))[0] = q[2];
  } else {
    value &= 0x00FF;

    value |= SYNC_ERRDY_EN;
    if ((npU->UnitId) && (npA->UnitCB[0].Flags & UCBF_NOTPRESENT))
      value &= ~SYNC_ERRDY_EN;

    p = PDC_ModeSets;
    q = PDC_DMAModeSets;
    if (npA->Cap & CHANREQ_ATA66) q = PDC_DMA66ModeSets;
  }
  (*(pSetting)&(npA->PDC_PIOtim))[npU->UnitId] = p[Mode] | value;

  Mode = npU->CurDMAMode - 1;
  if (Mode > 1) Mode = 0;

  if (npU->UltraDMAMode > 0) Mode = 2 + (npU->UltraDMAMode - 1);
  (*(pSetting)&(npA->PDC_DMAtim))[npU->UnitId] = q[Mode];
}

VOID NEAR ProgramPDCIO (NPU npU)  {
  NPA	 npA  = npU->npA;
  USHORT DMA  = 0x5FDF;
  USHORT UDMA;
  USHORT Reg, Data;
  UCHAR  Unit;

  Unit = npU->UnitId;
  Reg = (npA->IDEChannel ? TX2_PDC_MM_IDETIM1 : TX2_PDC_MM_IDETIM0) + 8 * Unit;

  UDMA = (*(pSetting)&(npA->DMAtim[0]))[0];
  Data = (*(pSetting)&(npA->PDC_DMAtim))[Unit];
  if (Data & 0x8000)
    UDMA = Data;
  else
    DMA  = Data;

  Data = (*(pSetting)&(npA->PDC_PIOtim))[Unit];
  if (Data == 0)
    return;

  OutD (npA->npC->BAR[5].Addr + Reg, ((ULONG)DMA << 16) | (Data & 0x3FFF));

  DMA  = ((UDMA & 0x0F) << 8) | (UDMA & 0xF0) | 0x0A;
  Data = (((Data >> 8) * 0x404) & 0xFF00) | (UDMA >> 8);

  OutD (npA->npC->BAR[5].Addr + Reg + 4, ((ULONG)Data << 16) | DMA);
}

/*-------------------------------*/
/*				 */
/* ProgramPDCChip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramPDCChip (NPA npA)
{
  NPU	 npU;
  UCHAR  Unit;
  USHORT Data;

  if (!(npA->Cap & CHIPCAP_ATA66)) {
    Data = RConfigB (PCIREG_INT_LINE);
    WConfigB (PCIREG_INT_LINE | 0x80, (UCHAR)Data);
  }

  Data = ReadConfigB (CurPCIInfo, 0x801F);	       // enable UDMA
  WriteConfigB (CurPCIInfo, 0x801F, (UCHAR)(Data | 0x01));
  WriteConfigB (CurPCIInfo, 0x801A, 0x01);
  WriteConfigB (CurPCIInfo, 0x801B, 0x01);

  npU = &(npA->UnitCB[0]);
  if ((!(npU->Flags & UCBF_NOTPRESENT)) &&
      (npU->FoundUDMAMode) && (!npU->UltraDMAMode) && (npU->CurDMAMode))
    npA->npC->HWSpecial = 3;

  npU = &(npA->UnitCB[1]);
  if ((!(npU->Flags & UCBF_NOTPRESENT)) &&
      (npU->FoundUDMAMode) && (!npU->UltraDMAMode) && (npU->CurDMAMode))
    npA->npC->HWSpecial = 3;

  /*************************************/
  /* Write IDE timing Register	       */
  /*************************************/

  if (npA->Cap & CHIPCAP_ATA66) {
    Data = ReadConfigB (CurPCIInfo, 0x8011);
    if (npA->IDEChannel == 0) {
      Data &= ~0x02;
      if (npA->Cap & CHANREQ_ATA66) Data |= 0x02;
    } else {
      Data &= ~0x08;
      if (npA->Cap & CHANREQ_ATA66) Data |= 0x08;
    }
    WriteConfigB (CurPCIInfo, 0x8011, (UCHAR)Data);

    npA->HWSpecial = npA->IDEChannel ? PDC_ATAPICOUNTER+4 + 2 - 8 : PDC_ATAPICOUNTER + 2;
  }

  for (Unit = 0; Unit < MAX_UNITS; Unit++) {
    ULONG  Setting;
    USHORT d;
    UCHAR  Reg = PCI_PDC_IDETIM + (8 * npA->IDEChannel) + (4 * Unit);

    Data = (*(pSetting)&(npA->PDC_PIOtim))[Unit];
    if (Data == 0)
      continue;

    d = (*(pSetting)&(npA->PDC_DMAtim))[Unit];

    Setting = ((ULONG)((d >> 8) | 0x40) << 16) | (d << 8) | Data;
    WConfigD (Reg, Setting);
  }
}

VOID ProgramPDCChipTX2 (NPA npA)
{
  ProgramPDCIO (&npA->UnitCB[0]);
  ProgramPDCIO (&npA->UnitCB[1]);
}

/*---------------------------------------------*/
/* PromiseCheckIRQ			       */
/*---------------------------------------------*/

int NEAR PromiseTX2CheckIRQ (NPA npA) {
  USHORT Port;
  register USHORT Loop;

  Port = BMSTATUSREG;
  DISABLE
  outp (Port - 1, 0x0B);
  if (inp (Port + 1) & 0x20) {
    STATUS = inp (STATUSREG);
    npA->Flags |= ACBF_BMINT_SEEN;
    ENABLE

    if (BMCMDREG && (inp (BMCMDREG) & BMICOM_START)) {
      Port = BMSTATUSREG;
      for (Loop = 0; --Loop != 0;) {
	if ((BMSTATUS = inp (Port)) & BMISTA_INTERRUPT) {
	  outp (BMCMDREG, npA->BM_CommandCode &= BMICOM_RD); /* turn OFF Start bit */
	  BMSTATUS = inp (Port);
	  break;
	}
	P4_REP_NOP
      }
    }
    if (STATUS & FX_BUSY) CheckBusy (npA);
    if (STATUS & FX_ERROR) ERROR = InB (ERRORREG);

    return (1);
  }
  ENABLE
  return (0);
}

int NEAR PromiseCheckIRQ (NPA npA) {   // Ultra66, Ultra100
  UCHAR  Value, Mask;
  USHORT Port;
  register USHORT Loop;

  Port = BMCMDREG | PDC_SCR1;
  Mask = npA->IDEChannel ? 0x50 : 0x05;
  DISABLE
  Value = inp (Port) & Mask;
  if (Value & 0x44) {
    outp (BMCMDREG + (npA->IDEChannel ? PDC_ATAPICOUNTER+4 + 3 - 8
				      : PDC_ATAPICOUNTER   + 3), 0); // stop counter

    STATUS = inp (STATUSREG);
    npA->Flags |= ACBF_BMINT_SEEN;
    ENABLE

    if (BMCMDREG && (inp (BMCMDREG) & BMICOM_START)) {
      if (Value & 0x11) {
	Stop:
	Port = BMSTATUSREG;
	IODly (npA->IRQDelayCount);
	for (Loop = 0; --Loop != 0;) {
	  if ((BMSTATUS = inp (Port)) & BMISTA_INTERRUPT) break;
	  IODly (Delay500 << 2);
	  P4_REP_NOP
	}
      } else {
	for (Loop = 15000; --Loop != 0;) {
	  if (inp (Port) & Mask & 0x11) goto Stop;
	  P4_REP_NOP
	}
	ENABLE
	return (0);
      }
    }
    if (STATUS & FX_BUSY) CheckBusy (npA);
    if (STATUS & FX_ERROR) ERROR = InB (ERRORREG);

    return (1);
  }
  ENABLE
  return (0);
}

VOID NEAR PromiseInitComplete (NPA npA)
{
  NPC	npC = npA->npC;
  UCHAR j;

  if (npC->HWSpecial) {
    if (npC->HWSpecial & 2) {
      USHORT Port = npC->BAR[4].Addr | PDC_SCR3;

      j = inp (Port);
      outp (Port, j | 0x10);
      DevHelp_ProcBlock ((ULONG)&(CompleteInit), 100UL, WAIT_IS_INTERRUPTABLE);
      outp (Port, j);
      DevHelp_ProcBlock ((ULONG)&(CompleteInit), 100UL, WAIT_IS_INTERRUPTABLE);

      npC->HWSpecial = 1;
    }
    outp (DRVHDREG, (npA->UnitCB[0].Flags & UCBF_NOTPRESENT) ? 0x10 : 0x00);
  }
}
															/* vvv, @V151345 */
VOID NEAR PDCPostReset (NPA npA)
{
  USHORT Port = BMBASE;

  outp (Port + PDC_SCR3, inp (Port + PDC_SCR3) | 0x01);
  outp (Port + PDC_MMC0, 0x01);
  outp (Port + PDC_MMC1, 0x01);
}

VOID NEAR PDCPreReset (NPA npA)
{
  USHORT Port = BMBASE;

  outp (Port + PDC_MMC0, 0x00);
  outp (Port + PDC_MMC1, 0x00);
}

/*---------------------------------------------*/
/* PDCSetupDMA				       */
/*---------------------------------------------*/

VOID NEAR PDCSetupDMA (NPA npA)
{
  USHORT Port = npA->BMICOM | PDC_SCR1;
  UCHAR  Shift = npA->IDEChannel * 4;
  USHORT Loops;

  outp (BMSTATUSREG, BMISTA_ERROR |  /* clear Error Bit       */
		 BMISTA_INTERRUPT |  /* clear INTR flag       */
		 npA->BMStatus);

  for (Loops = 5000; --Loops > 0;)
    if ((inp (Port) >> Shift) & 1) break;

  OutD (BMDTPREG, npA->ppSGL);
}

VOID NEAR PDCStartOp (NPA npA)
{
  if (npA->BM_CommandCode & BMICOM_START) {
    outp (BMCMDREG, npA->BM_CommandCode & ~BMICOM_START);
    outp (COMMANDREG, COMMAND);

    if ((npA->IOPendingMask & FM_HIGH) && (npA->HWSpecial)) {
      UCHAR Mode = (npA->npU->UltraDMAMode ? 4 : 0)
		 | (npA->ReqFlags & ACBR_WRITE ? 2 : 1);

      outpw (BMCMDREG + npA->HWSpecial, (Mode << 8) | 0xFF);
    }
    outp (BMCMDREG, npA->BM_CommandCode); /* Start BM controller */
  } else {

    outp (COMMANDREG, COMMAND);
  }
}

BOOL NEAR AcceptPDCMIO (NPA npA)
{
  NPC	npC  = npA->npC;
  NPU	npU  = npA->UnitCB;
  ULONG BAR3 = npC->BAR[3].Addr;
  ULONG Adr;
  USHORT val = 203;
  int	i;
  UCHAR *Reorder = NULL;
  static UCHAR ReorderTX4[4] = { 3, 1, 0, 2 };

  if (!BAR3) return (FALSE);

  npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133;
  npA->Cap &= ~CHIPCAP_PIO32;

  switch (PciInfo->Level) {
    case 0:
      i = InD (BAR3 + 0x48);
      npC->numChannels = 2 + (i & 2 ? 1 : 0) + (i & 1 ? 1 : 0);
      val = 206;
      break;

    case 1:
      npC->numChannels = 4;
      GenericSATA (npA);
      OutD (BAR3 + 0x6C, 0x00FF00FF);
      break;

    case 2:
      i = InD (BAR3 + 0x48);
      npC->numChannels = 3 + (i & 2 ? 1 : 0);
      if (npA->IDEChannel < 2) GenericSATA (npA);
      OutD (BAR3 + 0x6C, 0x00FF00FF);
      break;

    case 3:
      npC->numChannels = 4;
      Reorder = ReorderTX4;
      GenericSATA (npA);
      OutD (BAR3 + 0x60, 0x00FF00FF);
      val = 405;
      break;

    case 4:
      npC->numChannels = 3;
      if (npA->IDEChannel < 2) GenericSATA (npA);
      OutD (BAR3 + 0x60, 0x00FF00FF);
      val = 205;
      break;

    case 5:
      npC->numChannels = 4;
      Reorder = ReorderTX4;
      GenericSATA (npA);
      OutD (BAR3 + 0x60, 0x00FF00FF);
      val = 407;
      break;

    case 6:
      npC->numChannels = 3;
      if (npA->IDEChannel < 2) GenericSATA (npA);
      OutD (BAR3 + 0x60, 0x00FF00FF);
      val = 207;
      break;
  }

  if (Reorder) npA->IDEChannel = Reorder[npA->IDEChannel];

  Adr = BAR3 + 0x200 + (npA->IDEChannel << 7);
  for (i = FI_PDAT; i <= FI_PCMD; i++)
    npA->IOPorts[i] = Adr + (i << 2);
  DEVCTLREG = Adr + 0x38;
  BMCMDREG  = Adr + 0x60;
  BMDTPREG  = Adr + 0x44;
  BMSTATUSREG = 0;
  SSTATUS   = BAR3 + 0x400 + (npA->IDEChannel << 8);
  ISRPORT   = BAR3 + 0x40;

  OutD (BMCMDREG, (InD (BMCMDREG) & ~0x3F9F) | (npA->IDEChannel + 1));
  OutD (BAR3 + (npA->IDEChannel + 1) * 4, 1);

  if (!(npA->Cap & CHIPCAP_SATA)) {  // PATA channel
    SSTATUS = 0;

    // test for 80wire cable

    if (!(InD (BMCMDREG) & 0x01000000)) npA->Cap |= CHANCAP_CABLE80;
    METHOD(npA).CalculateTiming = CalculateAdapterTiming;
  }

  npA->Cap &= ~(CHIPCAP_ATAPIDMA);

  sprntf (npA->PCIDeviceMsg, PromiseMIOtxt, val);
  return (TRUE);
}

VOID NEAR PromiseMSetupDMA (NPA npA)
{
  OutD (BMDTPREG, npA->ppSGL);
}

VOID NEAR PromiseMStartOp (NPA npA)
{
  ULONG sr = (ULONG)((ISRPORT & ~0xFFF) + (npA->IDEChannel + 1) * 4);

  OutD (sr, 1);
  if (npA->BM_CommandCode & BMICOM_START) {
    OutB (COMMANDREG, COMMAND);
    OutD (BMCMDREG, (InD (BMCMDREG) & ~0xC0) | (npA->BM_CommandCode & BMICOM_RD ? 0x80 : 0xC0));
  } else {
    OutD (COMMANDREG, COMMAND);
  }
}

VOID NEAR PromiseMStopDMA (NPA npA)
{
  if (npA->BM_CommandCode) {
    OutD (BMCMDREG, InD (BMCMDREG) & ~0x80);
    npA->BM_CommandCode = 0;
  }
}

VOID NEAR PromiseMErrorDMA (NPA npA)
{
}

int NEAR PromiseMCheckIRQ (NPA npA) {
  USHORT Mask;
  NPC	 npC = npA->npC;

  DISABLE
  Mask = 2 << npA->IDEChannel;
  npC->HWSpecial |= InD (ISRPORT);
  if (npC->HWSpecial & Mask) {
    ULONG sr = (ULONG)((ISRPORT & ~0xFFF) + (npA->IDEChannel + 1) * 4);

    npC->HWSpecial &= ~Mask;
    OutD (ISRPORT, Mask);
    npA->Flags |= ACBF_BMINT_SEEN;
    BMSTATUS = 0;
    STATUS = InD (STATUSREG);
    ENABLE

//    OutD (BMCMDREG, (InD (BMCMDREG) & ~0x3F9F) | (npA->IDEChannel + 1));
    OutD (sr, 1);

    if (STATUS & FX_BUSY) CheckBusy (npA);
    if (STATUS & FX_ERROR) ERROR = InB (ERRORREG);

    return (1);
  }
  ENABLE
  return (0);
}

