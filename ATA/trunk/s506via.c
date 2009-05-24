/**************************************************************************
 *
 * SOURCE FILE NAME = S506VIA.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2009
 *
 * DESCRIPTION : Adapter Driver VIA routines.
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

#define PCITRACER 0
#define TRPORT 0xA800

#pragma optimize(OPTIMIZE, on)

#define PCI_VIA_IDEENABLE	0x40	    /* VIA IDE enables */
#define PCI_VIA_IDECONFIG	0x41	    /* VIA IDE configuration */
#define PCI_AMD_CABLE		0x42	    /* AMD IDE cable configuration */
#define PCI_VIA_IDEFIFOCFG	0x43	    /* VIA IDE FIFO configuration */
#define PCI_VIA_IDEMISC2	0x45	    /* VIA IDE misc. control */
#define PCI_VIA_IDEMISC3	0x46	    /* VIA IDE FIFO flush control */
#define PCI_VIA_IDETIM		0x48	    /* VIA IDE pulse timing */
#define PCI_VIA_IDEASU		0x4C	    /* VIA IDE address setup time */
#define PCI_VIA_CMDTIM		0x4E	    /* VIA IDE command timing */
#define PCI_VIA_IDEULTRA	0x50	    /* VIA IDE Ultra DMA timing */
#define PCI_NFC_IDEENABLE	0x50	    /* NFC IDE enables */
#define PCI_NFC_IDECONFIG	0x51	    /* NFC IDE configuration */
#define PCI_NFC_IDETIM		0x58	    /* NFC IDE pulse timing */
#define PCI_NFC_IDEASU		0x5C	    /* NFC IDE address setup time */
#define PCI_NFC_CMDTIM		0x5E	    /* NFC IDE command timing */
#define PCI_NFC_IDEULTRA	0x60	    /* NFC IDE Ultra DMA timing */
#define PCI_VIA2_IDETIM 	0xA8	    /* VT 6421 pulse timing */
#define PCI_VIA2_IDEULTRA	0xB0	    /* VT 6421 Ultra DMA timing */

/*----------------------------------------------------------------*/
/* Defines for VIA_level					  */
/*----------------------------------------------------------------*/
#define NOVIA		    0
#define VIA		    1	/* VIA586	       */
#define VIAA		    2	/* VIA586A/B	       */
#define AMD		    3	/* AMD751	       */
#define VIAB		    4	/* VIA596/A and VIA686 */
#define AMDA		    5	/* AMD756		(with ATA66)	 */
#define AMDB		    6	/* AMD766 and nForce	(with ATA100)	 */
#define VIAC		    7	/* VIA596B		(with ATA66)	 */
#define VIAD		    8	/* VIA686A and 8231	(with ATA66)	 */
#define AMDC		    9	/* AMD768		(with ATA100)	 */
#define VIAE		   10	/* VIA686B and 8233	(with ATA100)	 */
#define VIAF		   11	/* 8233A,8235,8237	(with ATA133)	 */
				/* PATA RAID		(with ATA133)	 */
#define VIAG		   12	/* SATA RAID				 */

/* remap some structure members */

#define VIA_IDEtim   PIIX_IDEtim
#define VIA_Ultratim PIIX_SIDEtim

#define PciInfo (&npA->PCIInfo)
#define PAdr PciInfo->PCIAddr

static UCHAR VIABase[] = {0x02, 0x00, 0x62, 0x00};

BOOL NEAR AcceptVIA (NPA npA)
{
  UCHAR  PCIDataB;
  USHORT Cable;
  UCHAR  Reg = VIABase[npA->IDEChannel & 3] + PCI_VIA_IDEULTRA;
  CHAR	 *Name = "571";

  Cable = GetRegW (PAdr, Reg);

  while (xBridgePCIAddr && GetRegW (xBridgePCIAddr, PCIREG_SUBCLASS) != PCI_CLASS_ISA_BRIDGE)
    xBridgePCIAddr += 8;
  xBridgeDevice = GetRegW (xBridgePCIAddr, PCIREG_DEVICE_ID);

  switch (PciInfo->CompatibleID) {
    case 0x1571: PciInfo->Level = VIA; break;
    case 0x0571:
      switch (xBridgeDevice) {
	case 0x586 : PciInfo->Level = (xBridgeRevision >= 0x20) ? VIAA : VIA;  break;
	case 0x596 : PciInfo->Level = (xBridgeRevision >= 0x10) ? VIAC : VIAB; break;
	case 0x686 : PciInfo->Level = (xBridgeRevision >= 0x40) ? VIAE :
				      (xBridgeRevision >= 0x10) ? VIAD : VIAB; break;
	default    : PciInfo->Level = VIAB;
		     if (xBridgeDevice >= 0x1000) {
		       UCHAR Test0, Test1;

		       SetRegB (PAdr, Reg, (UCHAR)(Cable & ~0x18));
		       Test0 = GetRegB (PAdr, Reg);
		       SetRegB (PAdr, Reg, (UCHAR)(Cable | 0x18));
		       Test1 = GetRegB (PAdr, Reg);
		       SetRegB (PAdr, Reg, (UCHAR)Cable);

		       Test0 ^= Test1;

		       if (Test0 & 0x10)	    // can 80WIRE bit be toggled ?
			 PciInfo->Level = (xBridgeDevice >= 0x3147) ? VIAF : VIAE;

		       else if (Test0 & 0x08)	    // can ATA66 bit be toggled ?
			 PciInfo->Level = VIAD;
		     }
      }
      break;
    case 0xC409: Name = "VX855";  npA->npC->numChannels = 1; PciInfo->Level = VIAF; break;
    case 0x5324: Name = "CX700";  PciInfo->Level = VIAF; break;
    case 0x3164: Name = "VT6410"; PciInfo->Level = VIAF; break;
    case 0x3149: {
	UCHAR SATAInc = 0x40;

	PciInfo->Level = VIAG; Name = "VT6420";
	npA->maxUnits = 2;

	PCIDataB = (GetRegB (PAdr, 0x49) & 0x60) >> 5;
	switch (PCIDataB) {
	  case 0: npA->maxUnits = 1;
		  break;
	  case 1: if (1 == npA->IDEChannel) SATAInc = 0;
		  break;
	}

	if (SATAInc) {
	  GenericSATA (npA);
	  npA->UnitCB[0].SStatus = npA->npC->BAR[5].Addr + npA->IDEChannel * 2 * SATAInc;
	  if (npA->maxUnits > 1)
	    npA->UnitCB[1].SStatus = npA->UnitCB[0].SStatus + SATAInc;
	}
      }
      break;
    case 0x3249: {
	NPC npC = npA->npC;

	UCHAR SATAInc = 0x40;

	PciInfo->Level = VIAG; Name = "VT6421";
	DATAREG   = npC->BAR[npA->IDEChannel].Addr;
	DEVCTLREG = DATAREG + 10;
	BMCMDREG  = npC->BAR[4].Addr + 8 * npA->IDEChannel;
	CollectPorts (npA);

	npC->numChannels = 3;
	if (npA->IDEChannel >= 2) {
	  MEMBER(npA).CfgTable = CfgNull;
	  PciInfo->Level = VIAF;
	  Cable ^= 0x1010;
	  SATAInc = 0;	// PATA port
	}

	if (SATAInc) {
	  GenericSATA (npA);
	  npA->Cap |= CHIPCAP_ATAPIDMA;
	  npA->UnitCB[0].SStatus = npC->BAR[5].Addr + npA->IDEChannel * SATAInc;
	}
      }
      break;
    case 0x3349:
      PciInfo->Level = VIAG; Name = "VT8251";
      if (!AcceptAHCI (npA)) return (FALSE);
      break;
  }

  sprntf (npA->PCIDeviceMsg, VIA571Msgtxt, Name);

  switch (PciInfo->Level) {
    case VIA  : npA->Cap &= ~CHIPCAP_ULTRAATA; break;
    case VIAF : npA->Cap |= CHIPCAP_ATA133;
    case VIAE : npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;
		Cable &= 0x1010;
		if (((xBridgeDevice == 0x0686) && (xBridgeRevision <= 0x42)) ||
		    ((xBridgeDevice == 0x8231) && (xBridgeRevision <= 0x12)))
		  Fixes |= 1; // enable official VIA fix
		break;
    case VIAC :
    case VIAD : npA->Cap |= CHIPCAP_ATA66;
		Cable &= 0x0008;
		break;
  }

  /* determine the presence of a 80wire cable as per defined procedure	 */
  /* hack! There is no defined cable detection procedure, use BIOS hints */

  if (Cable) npA->Cap |= CHANCAP_CABLE80;

  // fixup VIA southbridge bug and other bugs

  PCIDataB = ~0xE0;

  switch (0x7FFF & GetRegW (0, PCIREG_DEVICE_ID)) { // north bridge
    case 0x3099:			   // KT266x
// seems to cause problems on some machines
//	SetRegB (0, PCIREG_LATENCY, 0);
      Reg = 0x95; goto MWQfix;
    case 0x0305:
      switch (GetRegB (0, PCIREG_REVISION)) {
	case 0x81:			   // KL133
	case 0x84: PCIDataB = ~0xC0;	   // KM133
      }
    case 0x3102:
    case 0x3112:
      Reg = 0x55;
    MWQfix:
      SetRegB (0, Reg,	       // correct memory write queue timer (MWQ)
	       (UCHAR)(PCIDataB & GetRegB (0, Reg)));
      if (Reg == 0x95) break;
      /* fallthrough */
    case 0x0391:
      if (Fixes & 1) {	    // official fix from VIA
	PCIDataB = (GetRegB (0, 0x76) & ~0x20)
		 | 0x10;			// rotate master priority on
	SetRegB (0, 0x76, PCIDataB);		// every PCI master grant
      }

      if (Fixes & 2) {	    // inofficial fix, some machines need it too!
	PCIDataB = GetRegB (0, 0x70)
		 & ~0x06;   // disable PCI delayed transaction
			    // disable PCI master read caching
	SetRegB (0, 0x70, PCIDataB);
	SetRegB (0, 0x75, 0x80); // turn off PCI Latency timeout (set to 0 clocks)
      }

      if (Fixes & 4) {	    // derived from ViaLatency
	PCIDataB = (GetRegB (0, 0x75) & ~0x07) | 3;
	SetRegB (0, 0x75, PCIDataB);	   // PCI Latency timeout = 3 * 32 clocks
	SetRegB (0, PCIREG_LATENCY, 0x00); // guaranteed time slice 0 PCI clocks
      }
  }

  if (npA->FlagsI.b.native) METHOD(npA).CheckIRQ = BMCheckIRQ;

  return (TRUE);
}

#define PCIDEV_AMD751	0x7401
#define PCIDEV_AMD756	0x7409
#define PCIDEV_AMD766	0x7411
#define PCIDEV_AMD768	0x7441
#define PCIDEV_AMD8111	0x7469
#define PCIDEV_GEODELX	0x209A

BOOL NEAR AcceptAMD (NPA npA)
{
  USHORT Cable = 0;
  USHORT val;

  switch (PciInfo->CompatibleID) {
    case PCIDEV_AMD751:
      PciInfo->Level = AMD;
      val = 751;
      break;
    case PCIDEV_AMD756:
      PciInfo->Level = AMDA;
      npA->Cap |= CHIPCAP_ATA66;
      val = 756;

      /* determine the presence of a 80wire cable as per defined procedure   */
      /* hack! There is no defined cable detection procedure, use BIOS hints */

      Cable = GetRegW (PAdr, (UCHAR)(npA->IDEChannel ? PCI_VIA_IDEULTRA : PCI_VIA_IDEULTRA + 2));
      Cable &= (Cable >> 4) & 0x0404;
      break;
    case PCIDEV_AMD8111:
      npA->Cap |= CHIPCAP_ATA133;
      PciInfo->Level = AMDC;
      val = 8111;
      goto Viper;
    case PCIDEV_GEODELX:
      PciInfo->Level = AMDC;
      val = 5536;
      goto Viper;
    case PCIDEV_AMD768:
      PciInfo->Level = AMDC;
      val = 768;
      goto Viper;
    case PCIDEV_AMD766:
      PciInfo->Level = AMDB;
      val = 766;
    Viper:
      npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100;

      /* determine the presence of a 80wire cable as per defined procedure   */

      Cable = GetRegB (PAdr, PCI_AMD_CABLE);
      Cable = (Cable >> (2 * npA->IDEChannel)) & 0x03;
      break;
  }

  if (Cable) npA->Cap |= CHANCAP_CABLE80;

  sprntf (npA->PCIDeviceMsg, AMDxMsgtxt, val);
  return (TRUE);
}

#define PCIDEV_NFORCE  0x01BC
#define PCIDEV_NFORCE2 0x0065
#define PCIDEV_NFORCE3 0x00D5
#define PCIDEV_NFORCE4 0x0035
#define PCIDEV_NFORCE5 0x0265
#define PCIDEV_NFORCESATA 0x008E
#define PCIDEV_NFORCECK8  0x0036
#define PCIDEV_NFORCEADMA 0x0266
#define PCIDEV_NFORCEAHCI 0x044C

BOOL NEAR AcceptNVidia (NPA npA)
{
  ULONG  BAR5;
  USHORT Cable;
  static char Name[4] = " 00";

  Name[0] = '0' + PciInfo->Level;
  if (PciInfo->Level < 1)
    Name[0] = '\0';
  else if (PciInfo->Level < 5)
    Name[1] = '\0';

  /*
   * Apply NForce2 C1 Halt Disconnect fix from NVidia as shown in
   * http://marc.theaimsgroup.com/?l=linux-kernel&m=108362246902784&w=2
   */

  if (GetRegW (0, PCIREG_DEVICE_ID) == 0x01E0) {
    SetRegD (0, 0x6C, (GetRegB (0, PCIREG_REVISION) < 0xC1) ? 0x1F01FF01 : 0x9F01FF01);
  }

  /* chip bug: disable prefetching on *all* nForce PATA controllers */

  SetRegB (PAdr, PCI_NFC_IDECONFIG,
	   (UCHAR)(~0xF0 & GetRegB (PAdr, PCI_NFC_IDECONFIG)));

  PciInfo->Level = AMDB;  // defeat chip bug on nForce/2/3 !
  npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133;

  switch (PciInfo->CompatibleID) {
    case PCIDEV_NFORCE:
      npA->Cap &= ~CHIPCAP_ATA133;
      break;

    case PCIDEV_NFORCESATA:
    case PCIDEV_NFORCECK8:
    case PCIDEV_NFORCEADMA:
    case PCIDEV_NFORCEAHCI:
      GenericSATA (npA);
      npA->Cap |= CHIPCAP_ATAPIDMA;
      BAR5 = npA->npC->BAR[5].Addr;
      npA->UnitCB[0].SStatus = BAR5 + npA->IDEChannel * 0x40;
      SetRegB (PAdr, PCI_NFC_IDEENABLE,
	       (UCHAR)(0x04 | GetRegB (PAdr, PCI_NFC_IDEENABLE)));
      break;
  }

  switch (PciInfo->CompatibleID) {
    case PCIDEV_NFORCECK8:
      Name[0] = 'C';
      Name[1] = 'K';
      Name[2] = '8';
      OutB (BAR5 + 0x440, 0xFF); // clear IRQ status
      OutB (BAR5 + 0x441, 0x11); // mask IRQ enable
//	ISRPORT = BAR5 + 0x440;
//	npA->HWSpecial = npA->IDEChannel * 0x10;
      break;

    case PCIDEV_NFORCEADMA:
      Name[0] = 'M';
      Name[1] = '5';
      Name[2] = '1';
      OutD (BAR5 + 0x400, InD (BAR5 + 0x400) & ~0x6ul); // clear NCQ
      OutD (BAR5 + 0x440, 0x00FF00FF); // clear IRQ status
      OutD (BAR5 + 0x444, 0x00010001); // mask IRQ enable
//	ISRPORT = BAR5 + 0x440 + npA->IDEChannel * 2;
//	npA->HWSpecial = 0x01;
      break;

    case PCIDEV_NFORCEAHCI:
      Name[0] = 'M';
      Name[1] = '6';
      Name[2] = '5';
      if (!AcceptAHCI (npA)) return (FALSE);
      break;
  }

  sprntf (npA->PCIDeviceMsg, NForceMsgtxt, Name);

  /* determine the presence of a 80wire cable as per defined procedure	 */
  /* hack! There is no defined cable detection procedure, use BIOS hints */

  Cable = GetRegW (PAdr, (UCHAR)(npA->IDEChannel ? PCI_NFC_IDEULTRA : PCI_NFC_IDEULTRA + 2));
  Cable &= (Cable >> 4) & 0x0404;
  if (Cable) npA->Cap |= CHANCAP_CABLE80;

  if (npA->FlagsI.b.native) METHOD(npA).CheckIRQ = BMCheckIRQ;

  return (TRUE);
}

USHORT NEAR GetVIAPio (NPA npA, UCHAR Unit) {
  UCHAR Timing, Reg;

  Reg = npA->HardwareType == NVidia ? PCI_NFC_IDETIM : PCI_VIA_IDETIM;
  Reg += 3 - Unit - 2 * npA->IDEChannel;
  Timing = ReadConfigB (&npA->PCIInfo, Reg);
  return (2 + (Timing & 0x0F) + (UCHAR)(Timing >> 4));
}

#define BYTE(x) ((NPCH)&(npA->x))
#define WORD(x) ((NPUSHORT)&(npA->x))
#define CYCLE_DEFAULT 0xA8A8
#define ULTRA_DEFAULT 0x0707

/*-------------------------------*/
/*				 */
/* TimeRegBaseValue()		 */
/*				 */
/*-------------------------------*/
VOID VIATimingValue (NPU npU)
{
  NPA	npA = npU->npA;
  UCHAR Data, DefUDMA, Unit = 1 - npU->UnitId;
  UCHAR (*p)[4][7];

  Data = VIA_ModeSets[PCIClockIndex][npU->InterfaceTiming];
  if ((Data < 0x33) && (npA->PCIInfo.Level == VIAF))
    Data += 0x11; // add one clock active/inactive
  BYTE(VIA_IDEtim)[Unit] = Data ^ CYCLE_DEFAULT;

  p = &VIA_UltraModeSets;
  if (npA->HardwareType == Via) {
    if (npA->Cap & CHIPCAP_ATA133)
      p = &VIA_Ultra133ModeSets;
    else if (npA->Cap & CHIPCAP_ATA100)
      p = &VIA_Ultra100ModeSets;
    else if (npA->Cap & CHANREQ_ATA66) {
      BYTE(VIA_Ultratim)[0] |= 0x08;
      p = &VIA_Ultra66ModeSets;
    }
    DefUDMA = (*p)[3][0];
  } else {
    DefUDMA = 0;
  }

  // Ultra DMA timing register default setting is slowest UDMA

  BYTE(VIA_Ultratim)[2] = BYTE(VIA_Ultratim)[3] =  DefUDMA & 0x07;

  if (npU->UltraDMAMode)
    BYTE(VIA_Ultratim)[Unit] |= BYTE(VIA_Ultratim)[2]
			      ^ (*p)[PCIClockIndex][npU->UltraDMAMode - 1];
}

/*-------------------------------*/
/*				 */
/* ProgramVIAChip()		 */
/*				 */
/*-------------------------------*/
VOID ProgramVIAChip (NPA npA)
{
  UCHAR  Reg, Base;
  USHORT PCIData;
  UCHAR  PCIDataB;
  UCHAR  Ch = npA->IDEChannel;

  if (npA->HardwareType == NVidia) RegOffset = 0x10;
  if (Ch >= 2) RegOffset = VIABase[2] & ~0x03;
  Ch &= 1;

  /*************************************/
  /* Write IDE timing Register	       */
  /*************************************/

  Reg = (Ch & 1) ? PCI_VIA_IDETIM : PCI_VIA_IDETIM + 2;

  if (npA->VIA_IDEtim) {
    WConfigW (Reg, CYCLE_DEFAULT ^ (USHORT)(npA->VIA_IDEtim));

    if (npA->HardwareType != NVidia) {
      PCIDataB = RConfigB (PCI_VIA_IDEASU) | (Ch ? 0x0F : 0xF0);
      WConfigB (PCI_VIA_IDEASU, PCIDataB);	// address setup 4 clocks
    }

    PCIDataB = RConfigB (PCI_VIA_IDECONFIG);
    if (npA->FlagsT & ATBF_ATAPIPRESENT)
      PCIDataB &= (Ch ? ~0x30 : ~0xC0);       // disble prefetch/post buffers
    if (CurLevel == AMDB) PCIDataB &= ~0xF0;  // AMD76x/nForce prefetch bug!
    if (CurLevel == VIAF) PCIDataB |=  0xF0;  // VIA ATA/133 can always prefetch
    if (PCIDataB != RConfigB (PCI_VIA_IDECONFIG))
      WConfigB (PCI_VIA_IDECONFIG, PCIDataB);

    if (npA->HardwareType == Via) {
      PCIDataB = RConfigB (PCI_VIA_IDEMISC3) | (Ch ? 0x40 : 0x80);
      WConfigB (PCI_VIA_IDEMISC3, PCIDataB);

      if (CurLevel < VIAE) {
	PCIDataB = (RConfigB (PCI_VIA_IDEFIFOCFG) & 0x8F) | 0x30;
	WConfigB (PCI_VIA_IDEFIFOCFG, PCIDataB);
      }

      PCIDataB = RConfigB (PCI_VIA_IDEMISC2);
    }

    if (npA->Cap & CHIPCAP_ULTRAATA) {	      // if HW Ultra capable
      Reg += PCI_VIA_IDEULTRA - PCI_VIA_IDETIM;

      PCIData = RConfigW (Reg) & ~0xC7C7;
      if (CurLevel != VIAE) PCIData &= ~0xC7CF;
      WConfigW (Reg, (PCIData | WORD(VIA_Ultratim)[0]) ^ WORD(VIA_Ultratim)[1]);

      if (CurLevel < VIAE) PCIDataB |= 0x03;
    }

    if (npA->HardwareType == Via) {
      /***********************/
      /* Undocumented by VIA */
      /***********************/

      WConfigB (PCI_VIA_IDEMISC2, (UCHAR)(PCIDataB | 0x10));
    }
  }
}
