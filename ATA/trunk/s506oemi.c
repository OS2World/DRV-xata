/**************************************************************************
 *
 * SOURCE FILE NAME = S506OEMI.C
 *
 * DESCRIPTIVE NAME = DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2007
 *
 * DESCRIPTION : PCI detection code
 ****************************************************************************/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#define INCL_NO_SCB
#define INCL_DOSERRORS
#include "os2.h"
#include "dskinit.h"
#include "dhcalls.h"
#include "strat2.h"
#include "reqpkt.h"
#include "iorb.h"
#include "addcalls.h"
#include "s506cons.h"
#include "s506regs.h"
#include "s506type.h"
#include "s506ext.h"
#include "s506pro.h"

#include "Trace.h"

#define PAGESIZE 4096

#pragma optimize(OPTIMIZE, on)

#define PCITRACER 0
#define TRPORT 0xA010

extern RP_GENIOCTL IOCtlRP;

UCHAR FAR SetupOEMHlp (void)
{
  PCI_PARM_FIND_DEV PCIParmPkt;
  PCI_DATA	    PCIDataPkt;
  PRPH		    pRPH = (PRPH)&IOCtlRP;

  if ((SELECTOROF(OemHlpIDC.ProtIDCEntry) != NULL) &&
      (OemHlpIDC.ProtIDC_DS != NULL))
    return (0);     /* alread initialized */

  /* Setup Global OEMHlp Variables */
  if (DevHelp_AttachDD (OEMHLP_DDName, (NPBYTE)&OemHlpIDC))
    return (1);     /* Couldn't find OEMHLP's IDC */

  if ((SELECTOROF(OemHlpIDC.ProtIDCEntry) == NULL) ||
      (OemHlpIDC.ProtIDC_DS == NULL))
    return (1);     /* Bad Entry Point or Data Segment */

  /* Setup Parm Packet */
  PCIParmPkt.PCISubFunc = PCI_GET_BIOS_INFO;

  /* Setup IOCTL Request Packet */
  IOCtlRP.ParmPacket = (PUCHAR)&PCIParmPkt;
  IOCtlRP.DataPacket = (PUCHAR)&PCIDataPkt;

  if (CallOEMHlp (pRPH)) return (1);
  if (PCIDataPkt.bReturn != PCI_SUCCESSFUL) return (1);

  PCInumBuses = PCIDataPkt.Data_Bios_Info.LastBus + 1;
#if TRACES
  if (Debug & 8) TS("OEMBuses:%d,",PCInumBuses)
#endif
  return (0);
}

USHORT NEAR GetPCIBuses (void) {
  if (BIOSInt13 && SetupOEMHlp()) PCInumBuses = 0;
  return (PCInumBuses);
}


VOID NEAR SetLatency (NPA npA, UCHAR Lat) {
  UCHAR Latency;

  Latency = GetRegB (npA->PCIInfo.PCIAddr, PCIREG_LATENCY);
  if (!(Lat & 1) && (Latency < Lat)) Latency = Lat;
  if (Latency < PCILatency) Latency = PCILatency;
  if ((Lat & 1) && (Latency >= Lat)) Latency = Lat - 1;
  SetRegB (npA->PCIInfo.PCIAddr, PCIREG_LATENCY, Latency);
}

BOOL NEAR CheckWellknown (NPPCI_INFO npPCIInfo) {
  NPUSHORT p;
  USHORT   Id;
  UCHAR    i, j;
  NPPCI_DEVICE q;
  struct { USHORT Vendor, Device; } ChipID;

  *(PULONG)&ChipID = GetRegD (npPCIInfo->PCIAddr, PCIREG_VENDOR_ID);
  npPCIInfo->Ident.Vendor = ChipID.Vendor;
  npPCIInfo->Ident.Device = ChipID.Device;

  for (q = PCIDevice, i = 0; q < (PCIDevice + PCID_Generic); q++, i++) {
    if (ChipID.Vendor == q->Ident.Vendor) {
      for (p = q->ChipListCompatible; (Id = *p) != 0; p += 2) {
	if (Id == ChipID.Device) {
	  ChipID.Device = p[1];
	  break;
	}
      }

      npPCIInfo->CompatibleID = ChipID.Device;

      for (p = q->ChipListFamily, j = 0; (Id = *p) != 0; p++, j++) {
	if (Id == ChipID.Device) {
	  npPCIInfo->Ident.Index = i;
	  npPCIInfo->Level	 = j;
	  return (TRUE);
	}
      }
    }
  }
  return (FALSE);
}

VOID NEAR GetBAR (NPBAR npB, USHORT PCIAddr, UCHAR Index) {
  ULONG Val, Mask;
  UCHAR BAR = PCIREG_BAR0 + 4 * Index;

  npB->Addr = 0;
  npB->Size = 0;

  Val  = GetRegD (PCIAddr, BAR);
  Mask = (Val & PCI_BARTYPE_IO) ? ~3 : ~15;
  npB->phyA = Val & Mask;
  if (!npB->phyA) return;

  SetRegD (PCIAddr, BAR, -1);
  Mask &= GetRegD (PCIAddr, BAR);
  SetRegD (PCIAddr, BAR, Val);

  npB->Size = -Mask;

  if (Val & PCI_BARTYPE_IO) {
    if ((USHORT)npB->phyA >= 0x100) npB->Addr = npB->phyA;
  } else {
    LIN    lPhys;
    USHORT Offset = npB->phyA & (PAGESIZE - 1);
    ULONG  phyA   = npB->phyA & ~ (PAGESIZE - 1);
    USHORT Length = (npB->Size + Offset + (PAGESIZE - 1)) & ~(PAGESIZE - 1);
    PVOID  Ptr	  = (PVOID)&phyA;

    DevHelp_VirtToLin (SELECTOROF (Ptr), OFFSETOF (Ptr), (PLIN)&lPhys);
    DevHelp_VMAllocLite (VMDHA_PHYS, Length, lPhys, &(npB->Addr));
    npB->Addr += Offset;
  }
}

UCHAR NEAR ProbeChannel (NPA npA) {
  UCHAR Dev;

  Dev = InB (DRVHDREG);

#if TRACES
  if (Debug & 8) TS(" P(%02X",Dev)
#endif

  if ((Dev == FX_BUSY) || (Dev == (FX_BUSY | 0x50))) return (1);
  if ((Dev & 0x6F) == 0x6F) {
    OutBdms (DRVHDREG, 0xE0);
    Dev = InB (DRVHDREG);

#if TRACES
   if (Debug & 8) TS(":%02X",Dev)
#endif

    if ((Dev & 0x6F) == 0x6F) {
      OutBdms (DRVHDREG, 0xF0);
      Dev = InB (DRVHDREG);

  #if TRACES
      if (Debug & 8) TS(":%02X",Dev)
  #endif
    }

  }

  if (Dev == 0) {
    OutBdms (DRVHDREG, 0xF0);
    Dev = InB (DRVHDREG);

#if TRACES
  if (Debug & 8) TS(":%02X",Dev)
#endif
  }

  if ((Dev & ~0x10) == FX_DF) return (0);

  Dev &= ~0xEF;

  OutBdms (DRVHDREG, (UCHAR)(Dev | 0x05));
  OutBdms (DRVHDREG, (UCHAR)(Dev | 0xEA));
  Dev = InB (DRVHDREG);
#if TRACES
  if (Debug & 8) TS(":%02X)", Dev)
#endif
  return (((Dev & 0xEF) == 0xEA) ||  // expected result
	  ((Dev & 0xAF) == 0xA0)     // some ATAPI units with fixed reserved bits
	  );
}

VOID NEAR SetupT13StandardController (NPA npA)
{
  NPC	 npC	= npA->npC;
  USHORT BMBase = npC->BAR[4].Addr;
  USHORT Base1	= npC->BAR[npA->IDEChannel ? 3 : 1].Addr;

  if (npA->IDEChannel >= 2) return;

  // standard T13 ATA host resource setup

  DEVCTLREG = Base1 | 2;

  switch (npA->IDEChannel) {
    case 0:
      if (!(npA->FlagsI.b.native & PCI_IDE_NATIVE_IF1)) {
	npA->IRQLevel = PRIMARY_I;
	DEVCTLREG     = PRIMARY_C;
	npC->IrqPIC   = npC->IrqAPIC = 0;
      }
      if (BMBase) {
	BMCMDREG    = BMBase;
	npA->BMSize = 8;
      }
      break;

    case 1:
      if (!(npA->FlagsI.b.native & PCI_IDE_NATIVE_IF2)) {
	npA->IRQLevel = SECNDRY_I;
	DEVCTLREG     = SECNDRY_C;
	npC->IrqPIC   = npC->IrqAPIC = 0;
      }
      if (BMBase) {
	BMCMDREG    = BMBase + 8;
	npA->BMSize = 8;
      }
  }
}

UCHAR NEAR CheckSATAPhy (NPU npU) {
  if (SERROR) {
    USHORT Status = InD (SSTATUS);
    ULONG  Diag   = InD (SERROR);
#if TRACES
    if (Debug & 8) TraceStr (" S(%X:%X)", Status, Diag);
#endif
    if ((Status & SSTAT_DET) & (SSTAT_DEV_OK | SSTAT_COM_OK)) {
      OutD (SERROR, Diag);
      npU->Flags |= UCBF_PHYREADY;
      return TRUE;
    }
  }
  return FALSE;
}

UCHAR NEAR CollectSCRPorts (NPU npU)
{
  NPA npA = npU->npA;

  if (SSTATUS) {
    if (npA->SCR.Offsets) {
      SERROR   = SSTATUS + 4 * npA->SCR.Ofs.SError;
      SCONTROL = SSTATUS + 4 * npA->SCR.Ofs.SControl;
      SSTATUS  = SSTATUS + 4 * npA->SCR.Ofs.SStatus;
    }
    return (TRUE);
  } else {
    return (FALSE);
  }
}

UCHAR NEAR nextController (NPPCI_INFO Enum) {
  UCHAR rc = FALSE;
  static UCHAR maxFnc = MAX_PCI_FUNCTIONS;
  static struct {
    UCHAR Rev, ProgIF, Subclass, Class;
  } ClassCode;
  UCHAR HeaderType;

#define PciAddr (Enum->PCIAddr)
#define Bus ((UCHAR *)&PciAddr)[1]
#define DevFnc ((UCHAR *)&PciAddr)[0]
#define Fnc (DevFnc & 0x07)

  while (++PciAddr != -1) {
    if (Fnc >= maxFnc)
      PciAddr = (PciAddr + (MAX_PCI_FUNCTIONS - 1)) & ~(MAX_PCI_FUNCTIONS - 1);
    if (Bus >= PCInumBuses) break;

    *(ULONG *)&ClassCode = GetRegD (PciAddr, PCIREG_CLASS_CODE);
    HeaderType = GetRegB (PciAddr, PCIREG_HEADER_TYPE);

    if (Fnc == 0) {
#if TRACES
      if (!DevFnc && (Debug & 8)) TS("\nBus %d:",Bus)
#endif
      maxFnc = MAX_PCI_FUNCTIONS;     // default to full function enumeration

      if (!(HeaderType & 0x80)) {  // this is a single function device
	maxFnc = 1;		   // enumerate first function only

	/* some old PCI-ISA-bridges have broken PCI config spaces ! */
	if (*(USHORT*)&(ClassCode.Class) == 0x0106) maxFnc++;
      }
    }

    HeaderType &= ~0x80;

    if (HeaderType && (HeaderType <= 2)) {    // this is a bridge device
      UCHAR SubBus = GetRegB (PciAddr, PCIREG_SUBORDINATE_BUS);
#if TRACES
      if (Debug & 8) TS("[->%d]",SubBus)
#endif
      if (!SubBus || (SubBus == 0xFF)) continue;
      SubBus++;
      if (PCInumBuses < SubBus) PCInumBuses = SubBus;
    }

    if (ClassCode.Class != PCI_MASS_STORAGE) continue;

    rc = CheckWellknown (Enum);
    if (!rc &&
	(ClassCode.Subclass == PCI_IDE_CONTROLLER) &&
       !(ClassCode.ProgIF & (PCI_IDE_NATIVE_IF1 | PCI_IDE_NATIVE_IF2))) {
      Enum->Ident.Index = PCID_Generic;
      rc = TRUE;
    }
    if (!rc) continue;

#if TRACES
    if (Debug & 8) {
      TraceStr ("& %X/%X=%04X/%04X,%d/%d:", Bus, DevFnc,
		 Enum->Ident.Vendor, Enum->Ident.Device, Enum->Ident.Index, Enum->Level);
    }
#endif
    break;
  }

  return (rc);
}

#define CUT_OFF (4 * MAX_PCI_DEVICES)

USHORT FAR EnumPCIDevices (void)
{
  UCHAR rc;
  UCHAR count = 0;
  NPC	npC;
  NPA	npA;
  NPU	npU;
  struct {
    UCHAR Rev, ProgIF, Subclass, Class;
  } ClassCode;
  static PCI_INFO Enum;
  USHORT Cmd;
  NPPCI_DEVICE npDev;
  UCHAR Channel;
  UCHAR Int;
  UCHAR i;

  GetPCIBuses();

  npC = ChipTable;

  Enum.PCIAddr = -1;
  while (nextController (&Enum)) {
    *(PULONG)&ClassCode = GetRegD (Enum.PCIAddr, PCIREG_CLASS_CODE);
    Enum.Ident.Revision = ClassCode.Rev;

    Cmd = GetRegW (Enum.PCIAddr, PCIREG_COMMAND);
    if (!(Cmd & (PCI_CMD_IO | PCI_CMD_MEM))) continue;

    Cmd &= ~PCI_COMMAND_INTX_DIS;
    SetRegW (Enum.PCIAddr, PCIREG_COMMAND, Cmd | PCI_CMD_BME_MEM);

    Int = GetRegB (Enum.PCIAddr, PCIREG_INT_LINE);
    if (Int == 0xFF) Int = 0;  // invalid interrupt setup!

    if (ClassCode.Subclass != PCI_IDE_CONTROLLER)
      ClassCode.ProgIF = PCI_IDE_BUSMASTER | PCI_IDE_NATIVE_IF1 | PCI_IDE_NATIVE_IF2;
    ClassCode.ProgIF &= PCI_IDE_BUSMASTER | PCI_IDE_NATIVE_IF1 | PCI_IDE_NATIVE_IF2;

    if (ClassCode.ProgIF & (PCI_IDE_NATIVE_IF1 | PCI_IDE_NATIVE_IF2)) {
      switch (Int) {
	case PRIMARY_I :
	  if (AdapterTable[0].IOPorts[0] == PRIMARY_P) AdapterTable[0].IOPorts[0] = 0;
	  break;

	case SECNDRY_I :
	  if (AdapterTable[1].IOPorts[0] == SECNDRY_P) AdapterTable[1].IOPorts[0] = 0;
	  break;
      }
    }

    npDev = PCIDevice + Enum.Ident.Index;
    xBridgePCIAddr = (Enum.PCIAddr & ((0xF8 & npDev->xBMask) | 0xFF00))
		   | ((npDev->xBMask & 0x07) << 3);
    xBridgeDevice   = GetRegW (xBridgePCIAddr, PCIREG_DEVICE_ID);
    xBridgeRevision = GetRegB (xBridgePCIAddr, PCIREG_REVISION);

    if (npC->populatedChannels) npC++;
    fclrmem (npC, sizeof (*npC));

    npC->numChannels = 2; // default setup
    npC->populatedChannels = 0;

    for (i = 0; i <= 5; i++)
      GetBAR (npC->BAR + i, Enum.PCIAddr, i);

    *(NPUSHORT)(&npC->IrqPIC) = ACPIGetPCIIRQs (Enum.PCIAddr);
    if (!npC->IrqPIC)  npC->IrqPIC  = Int;
    if (!npC->IrqAPIC) npC->IrqAPIC = npC->IrqPIC;

    for (Channel = 0; Channel < npC->numChannels; Channel++) {
      USHORT savedAdapterFlags;
      USHORT savedUnitFlags[2];
      UCHAR  EnableReg, EnableBit, Enable;
      USHORT Base0 = -1;

      if (!(npDev->Ident.Revision & NONSTANDARD_HOST) && (Channel < 2)) {
	if (ClassCode.ProgIF & (PCI_IDE_NATIVE_IF1 | PCI_IDE_NATIVE_IF2))
	  Base0 = npC->BAR[Channel ? 2 : 0].Addr;
	else
	  Base0 = Channel ? SECNDRY_P : PRIMARY_P;
      }
      npA = LocateATEntry (Base0, Enum.PCIAddr, Channel);
      if (!npA) continue;

      npU = npA->UnitCB;
      npU[0].npA = npU[1].npA = npA;

      npA->IDEChannel	   = Channel;
      npA->PCIInfo.PCIAddr = Enum.PCIAddr;
      npA->PCIInfo.npC	   = npA->npC = npC;
      npA->FlagsI.All	   = ClassCode.ProgIF;
      npA->HardwareType    = npDev->Ident.Index;
      npA->maxUnits	   = 0;
      npA->PCIDeviceMsg[0] = '\0';

      npA->PCIInfo.CompatibleID   = Enum.CompatibleID;
      npA->PCIInfo.Ident	  = npDev->Ident;
      npA->PCIInfo.Ident.Vendor   = Enum.Ident.Vendor;
      npA->PCIInfo.Ident.Device   = Enum.Ident.Device;
      npA->PCIInfo.Ident.Revision = ClassCode.Rev;
      npA->PCIInfo.Ident.Index	  = Enum.Ident.Index;
      npA->PCIInfo.Level	  = Enum.Level;

      npA->Cap	       = CHIPCAP_DEFAULT;
      npA->IRQDelay    = 0;
      npA->SCR.Offsets = 0x210;

      SetupT13StandardController (npA);
      // in case of legacy mode, IrqPIC and IrqAPIC are 0!
      if (npC->IrqPIC) npA->IRQLevel = npC->IrqPIC;

      if (0 == Channel) {
	EnableReg = npDev->EnableReg0;
	EnableBit = npDev->EnableBit & 0x0F;
      } else {
	EnableReg = npDev->EnableReg1;
	EnableBit = npDev->EnableBit >> 4;
      }

      // test for required resources

      if (npA->FlagsI.b.native) {
	if (!(npDev->Ident.Revision & MODE_NATIVE_OK) || !npA->IRQLevel) goto Recycle;
	if (!(npDev->Ident.Revision & NONSTANDARD_HOST) && !npC->BAR[4].Addr) goto Recycle;
      }

      Enable = GetRegB (Enum.PCIAddr, EnableReg);
      if (EnableBit & 0x08) {
	EnableBit ^= 0x0F;
	Enable = ~Enable;
      }
      if ((Channel < 2) && !(Enable & (1 << EnableBit)) && !(npA->FlagsT & ATBF_BAY)) goto Recycle;

      CollectPorts (npA);
      npU[0].SStatus = npU[1].SStatus = 0;

      savedAdapterFlags = npA->FlagsT;
      savedUnitFlags[0] = npU[0].FlagsT;
      savedUnitFlags[1] = npU[1].FlagsT;

      if (npDev->Ident.ChipAccept (npA) &&
	  !HandleFoundAdapter (npA, npDev)) {
	count++;
	if (*(NPUSHORT)(&npC->IrqPIC) && (npC->IrqPIC != Int) && !(npA->FlagsT & ATBF_DISABLED))
	  APICRewire |= 1 << (npA - AdapterTable);

	#if TRACES
	  if (Debug & 8) TS("%d ",count)
	#endif
      } else {
	npA->FlagsT   = savedAdapterFlags; // restore cmdline options
	npU[0].FlagsT = savedUnitFlags[0];
	npU[1].FlagsT = savedUnitFlags[1];
      Recycle:
	DATAREG = 0;  // return adapter slot for reuse
      }
    }
  }

  if (npC->populatedChannels) npC++;
  cChips = npC - ChipTable;
  return (count);
}

UCHAR NEAR HandleFoundAdapter (NPA npA, NPPCI_DEVICE npDev)
{
  UCHAR isPopulated, hasPhy;
  NPC	npC = npA->npC;
  NPU	npU;

  if (!npA->maxUnits)
    npA->maxUnits = npA->Cap & CHIPCAP_SATA ? 1 : 2;

  hasPhy = FALSE;
  isPopulated = FALSE;
  for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->maxUnits); npU++) {
    if (CollectSCRPorts (npU)) {
      UCHAR isAttached;
      hasPhy = TRUE;
      isAttached   = CheckSATAPhy (npU);
      isPopulated |= isAttached;
      if (!isAttached) npU->FlagsT |= UTBF_DISABLED;
    }
  }
  if (!hasPhy)
    isPopulated = ProbeChannel (npA);

  if (isPopulated || (npA->FlagsT & (ATBF_BAY | ATBF_POPULATED))) {
    if (BMCMDREG < 0x100) {
      npA->Cap &= ~(CHIPCAP_ULTRAATA | CHIPCAP_ATADMA | CHIPCAP_ATAPIDMA);
    } else {
      // disable simplex bit if possible

      if (npDev->Ident.Revision & SIMPLEX_DIS) {
	UCHAR Value;

	if ((Value = InB (BMSTATUSREG)) & BMISTA_SIMPLEX)
	  OutB (BMSTATUSREG, (UCHAR)(Value & ~BMISTA_SIMPLEX));
      }
    }

    SetLatency (npA, npDev->Latency);

    npC->npA[npA->IDEChannel] = npA;
    npA->npPCIDeviceMsg = (npA->PCIDeviceMsg[0] == 0) ?
			    PCIDevice[MEMBER(npA).Index].npDeviceMsg :
			    npA->PCIDeviceMsg;
    if (npA->FlagsI.b.native) npA->FlagsT |= ATBF_INTSHARED;
    npA->SGAlign = MEMBER(npA).SGAlign - 1;

    npC->populatedChannels++;
    return (FALSE);
  } else {
    return (TRUE);
  }
}
