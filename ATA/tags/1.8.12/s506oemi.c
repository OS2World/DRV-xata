/**************************************************************************
 *
 * SOURCE FILE NAME = S506OEMI.C
 * $Id$
 *
 * DESCRIPTIVE NAME = DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2009
 * Portions Copyright (c) 2010, 2011 Steven H. Levine
 * distributed under the terms of the GNU Lesser General Public License
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

/**
 * Set up for OEMHlp IDC call and call IDC to get PCI BIOS info
 * Sets PCInumBuses if IDC call does not fail
 * Ensures IDC call occurs only once
 * @returns 0 if OK, 1 if failed
 */

UCHAR FAR SetupOEMHlp (void)
{
  PCI_PARM_FIND_DEV PCIParmPkt;
  PCI_DATA	    PCIDataPkt;
  PRPH		    pRPH = (PRPH)&IOCtlRP;

  if ((SELECTOROF(OemHlpIDC.ProtIDCEntry) != NULL) &&
      (OemHlpIDC.ProtIDC_DS != NULL))
    return (0);     /* already initialized */

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
# if TRACES
    if (Debug & 8) TS("OEMBuses:%d,",PCInumBuses)
# endif
  return (0);
}

USHORT NEAR GetPCIBuses (void)
{
  if (BIOSInt13 && SetupOEMHlp()) PCInumBuses = 0;
  return (PCInumBuses);
}

VOID FAR CheckLegacyPorts (VOID)
{
  UCHAR Int;

  for (Int = PRIMARY_I; Int <= SECNDRY_I; Int++) {
    USHORT Port = (Int == PRIMARY_I) ? PRIMARY_P : SECNDRY_P;
    NPA    npA	= LocateATEntry (Port, 0, 0);
    if (!npA) continue;

    DEVCTLREG	  = Port + (PRIMARY_C - PRIMARY_P);
    npA->IRQLevel = Int;
    STATUSREG	  = 0;
    BMCMDREG	  = 0;
    npA->Cap	  = 0;
    MEMBER(npA).Vendor = 0;
    npA->maxUnits = 2;
    npA->FlagsT  |= ATBF_BIOSDEFAULTS;
    CollectPorts (npA);
  }
}

VOID NEAR SetLatency (NPA npA, UCHAR Lat)
{
  UCHAR Latency;

  Latency = GetRegB (npA->PCIInfo.PCIAddr, PCIREG_LATENCY);
  if (!(Lat & 1) && (Latency < Lat)) Latency = Lat;
  if (Latency < PCILatency) Latency = PCILatency;
  if ((Lat & 1) && (Latency >= Lat)) Latency = Lat - 1;
  SetRegB (npA->PCIInfo.PCIAddr, PCIREG_LATENCY, Latency);
}

/**
 * Check for known PCI ids
 * @return TRUE if found otherwise FALSE
 */

BOOL NEAR CheckWellknown (NPPCI_INFO npPCIInfo)
{
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
	  ChipID.Device = p[1];		// Found compatible - override id
	  break;
	}
      } // for compatibles

      npPCIInfo->CompatibleID = ChipID.Device;	// Remember

      for (p = q->ChipListFamily, j = 0; (Id = *p) != 0; p++, j++) {
	if (Id == ChipID.Device) {
	  npPCIInfo->Ident.Index = i;	// Remember ChipListCompatible device index
	  npPCIInfo->Level	 = j;	// Remember ChipListFamily family index
	  return (TRUE);
	}
      } // for family
    } // if vendor
  } // for device
  return (FALSE);
}

VOID NEAR GetBAR (NPBAR npB, USHORT PCIAddr, UCHAR Index)
{
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

/**
 * Probe controller for active channels
 * Code tries to empirically exclude controllers that will not work correctly
 * @calledby HandleFoundAdapter
 * @return TRUE if found
 */

UCHAR NEAR ProbeChannel (NPA npA)
{
  UCHAR Dev = InB (DRVHDREG);

# if TRACES
    if (Debug & 8) TS(" P(%02X",Dev)
# endif

// DEVHDREG - Device/Head register, typically baseaddr+6, orginally called Drive/Head

#define DEVHD_RO7	0x80      // 7 - reserved, typically r/o = 1, but not always
#define DEVHD_LBA	0x40      // 6 - LBA - r/w, 1 = LBA mode enabled
#define DEVHD_RO5	0x20      // 5 - reserved, typically r/o = 1, but not always
#define DEVHD_DEV1	0x10      // 4 - Device# = r/w, 0 = device#0, 1 = device#1
#define DEVHD_RW3	0x08      // 3 - LBA27, HS3 - typically r/w
#define DEVHD_RW2	0x04      // 2 - LBA26, HS2 - typicall r/w
#define DEVHD_RW1	0x02      // 1 - LBA25, HS1 - typically r/w
#define DEVHD_RW0	0x01      // 0 - LBA24, HS0 - typically r/w

  // 2011-07-20 SHL fixme to know why - probably empirically determine by Daniela?
  if (Dev == DEVHD_RO7 || Dev == (DEVHD_RO7|DEVHD_LBA|DEVHD_DEV1)) return (1); // OK - 0x80 or 0xD0

  #define LBA_RO5_BITS3_0 (DEVHD_LBA|DEVHD_RO5|0xF) // 0x6F - fixme to understand why
  #define LBA_DEV0 (DEVHD_RO7|DEVHD_LBA|DEVHD_RO5)	// 0xE0
  if ((Dev & LBA_RO5_BITS3_0) == LBA_RO5_BITS3_0) {
    // All r/w bits set, try to select LBA, device#0
    OutBdms (DRVHDREG, LBA_DEV0);	// Try LBA, device#0, 0xE0
    Dev = InB (DRVHDREG);
#   if TRACES
      if (Debug & 8) TS(":%02X",Dev)
#   endif

    // 2011-07-20 SHL fixme to know why
    #define RO7_LBA_RO5_DEV1 (DEVHD_RO7|DEVHD_LBA|DEVHD_RO5|DEVHD_DEV1)	// 0xF0
    if ((Dev & LBA_RO5_BITS3_0 ) == LBA_RO5_BITS3_0) {
      // All r/w bits still set, try to select LBA and device#1 (0xF0)
      OutBdms (DRVHDREG, RO7_LBA_RO5_DEV1);	// Enable LBA, select device#1, 0xF0
      Dev = InB (DRVHDREG);
#     if TRACES
	if (Debug & 8) TS(":%02X",Dev)
#     endif
    }

  } // if LBA_RO5_BITS3_0 (0x6F)

  if (Dev == 0) {
    // No bits set - probably in some sort of off state - try to turn on
    OutBdms (DRVHDREG, RO7_LBA_RO5_DEV1);	// Try to selelct LBA, device#1 (0xF0)
    Dev = InB (DRVHDREG);
#   if TRACES
      if (Debug & 8) TS(":%02X",Dev)
#   endif

    // 2011-11-11 SHL ticket #8, ICH1 with ATAPI CD attached returns 0x7f here
    // 2011-12-08 SHL ticket #8, ICH2 with ATAPI CD attached returns 0x30 here
    if (Dev == 0x7f || Dev == 0x30) {
      OutBdms (DRVHDREG, DEVHD_RO7 | DEVHD_RO5);	// Try to set just R/O bits
      Dev = InB (DRVHDREG);
#     if TRACES
	if (Debug & 8) TS(":%02X",Dev)
#     endif
    } // if 0x7f or 0x30
  } // Dev == 0

  if ((Dev & ~DEVHD_DEV1) == DEVHD_RO5) return (0);	// fail - can't set RO7 (~0x20 - 0xDF)

  Dev &= ~DEVHD_DEV1;	// ~0xEF = 0xA0, typically 0xE0 here

  #define MASK1 (DEVHD_RW2 | DEVHD_RW0)	// 0x05
  OutBdms (DRVHDREG, (UCHAR)(Dev | MASK1));	// Try to set to 0xE5
  #define MASK2 (DEVHD_RO7|DEVHD_LBA|DEVHD_RO5|DEVHD_RW3|DEVHD_RW1)	// 0xEA
  OutBdms (DRVHDREG, (UCHAR)(Dev | MASK2));	// Try to set to 0xEA
  Dev = InB (DRVHDREG);
# if TRACES
    if (Debug & 8) TS(":%02X)", Dev)
# endif

  #define OK1_MASK (DEVHD_RO7|DEVHD_LBA|DEVHD_RO5|0xf) // 0xEF
  #define OK1_BITS (DEVHD_RO7|DEVHD_LBA|DEVHD_RO5|DEVHD_RW3|DEVHD_RW1) // 0xEA
  #define OK2_MASK (DEVHD_RO7|DEVHD_RO5|0xf) // 0xAF
  #define OK2_BITS (DEVHD_RO7|DEVHD_RO5) // 0xA0
  // expected result - some ATAPI units have non-writable bits 3..0
  return (Dev & OK1_MASK) == OK1_BITS || (Dev & OK2_MASK) == OK2_BITS;
}

/**
 * Setup adapter assuming standard T13 controller.
 * Setup bypassed if more than two channels since it can't be T13
 */

VOID NEAR SetupT13StandardController (NPA npA)
{
  NPC	 npC	= npA->npC;
  USHORT BMBase = npC->BAR[4].Addr;
  USHORT Base1	= npC->BAR[npA->IDEChannel ? 3 : 1].Addr;

  if (npA->IDEChannel >= 2) return;	// Can't be T13

  // standard T13 ATA host resource setup

  DEVCTLREG = Base1 | 2;

  switch (npA->IDEChannel) {
    case 0:
      if (!(npA->ProgIF.b.native & PCI_IDE_NATIVE_IF1)) {
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
      if (!(npA->ProgIF.b.native & PCI_IDE_NATIVE_IF2)) {
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

/**
 * Check SATA Phy OK
 * @return TRUE if OK
 */

UCHAR NEAR CheckSATAPhy (NPU npU)
{
  if (SERROR) {
    USHORT Status  = InD (SSTATUS);
    USHORT Control = InD (SCONTROL);
    ULONG  Diag    = InD (SERROR);
#   if TRACES
      if (Debug & 8) TraceStr (" S(%X:%X:%X)", Status, (USHORT)Diag, Control);
#   endif
    if ((Status & SSTAT_DET) & (SSTAT_DEV_OK | SSTAT_COM_OK)) {
      OutD (SERROR, Diag);
      npU->FoundLPMLevel = (Control & SCTRL_IPM) >> 8;
      npU->Flags |= UCBF_PHYREADY;
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * Set SATA control register addresses if address and offsets known
 * @return TRUE if set
 */

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

/**
 * Scan PCI bus for next well known or generic controller
 * @return TRUE if found otherwise FALSE
 */

UCHAR NEAR nextController (NPPCI_INFO Enum)
{
  UCHAR ok = FALSE;
  static UCHAR BusInc = 1;
  static UCHAR maxFnc = MAX_PCI_FUNCTIONS;
  static struct {
    UCHAR Rev, ProgIF, Subclass, Class;
  } ClassCode;
  UCHAR HeaderType;

#define PciAddr (Enum->PCIAddr)
#define Bus ((UCHAR *)&PciAddr)[1]
#define DevFnc ((UCHAR *)&PciAddr)[0]
#define Fnc (DevFnc & 0x07)

  // Try next
  while (++PciAddr != -1) {
    if (Fnc >= maxFnc)
      PciAddr = (PciAddr + (MAX_PCI_FUNCTIONS - 1)) & ~(MAX_PCI_FUNCTIONS - 1); // Next device
    if (Bus >= PCInumBuses) break;	// Done

    *(ULONG *)&ClassCode = GetRegD (PciAddr, PCIREG_CLASS_CODE);
    HeaderType = GetRegB (PciAddr, PCIREG_HEADER_TYPE);

    if (Fnc == 0) {
#     if TRACES
	if (!DevFnc && (Debug & 8)) TS("\nBus %d:",Bus)
#     endif
      maxFnc = MAX_PCI_FUNCTIONS;     // default to full function enumeration

#     define HEADER_TYPE_MULTIFUNCTION 0x80
      if (~HeaderType & HEADER_TYPE_MULTIFUNCTION) {
	// single function device - enumerate first function only
	maxFnc = 1;

	/* Some old PCI-ISA-bridges have broken PCI config spaces
	   Assume they have 2 functions
	 */
#       define CLASS_PCI_ISA_BRIDGE 0x0106	// Intel order
	if (*(USHORT*)&(ClassCode.Class) == CLASS_PCI_ISA_BRIDGE) maxFnc++;
      }
    }

    HeaderType &= ~HEADER_TYPE_MULTIFUNCTION;	// Strip

    if (HeaderType && (HeaderType <= 2)) {
      // this is a bridge device
      UCHAR SubBus = GetRegB (PciAddr, PCIREG_SUBORDINATE_BUS);
#     if TRACES
	if (Debug & 8) TS("[->%d]",SubBus)
#     endif
      if (SubBus == 0 || (SubBus == 0xFF)) continue;
      SubBus += BusInc;
      if (SubBus < BusInc) SubBus = 0xFF;
      if (PCInumBuses < SubBus) PCInumBuses = SubBus;	// Remember to scan
    } else {
      // count all HOST->PCI bridges
#     define CLASS_HOST_PCI_BRIDGE 0x0006	// Intel order
      if (*(USHORT*)&(ClassCode.Class) == CLASS_HOST_PCI_BRIDGE) BusInc++;
    }

    if (ClassCode.Class != PCI_MASS_STORAGE) continue;

    ok = CheckWellknown (Enum);
    if (!ok &&
	ClassCode.Subclass == PCI_IDE_CONTROLLER &&
	(ClassCode.ProgIF & (PCI_IDE_NATIVE_IF1 | PCI_IDE_NATIVE_IF2)) == 0) {
      // 2011-07-27 SHL ticket #9 fixme to switch to compatibility if chipset supports it
      // Programmed for compatibility mode
      Enum->Ident.Index = PCID_Generic;	// Pass to caller
      ok = TRUE;			// Accept
    }
    if (!ok) continue;			// No match

    // 2011-07-27 SHL fixme to pass back ClassCode maybe?

#   if TRACES
      if (Debug & 8) {
	TraceStr ("& %X/%X=%04X/%04X,%d/%d:",
		  Bus, DevFnc, Enum->Ident.Vendor, Enum->Ident.Device,
		  Enum->Ident.Index, Enum->Level);
      }
#   endif
    break;				// Got match
  } // while PciAddr

  return ok;
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
    if (!(Cmd & (PCI_CMD_IO | PCI_CMD_MEM))) continue;	// No access mode enabled

    Cmd &= ~PCI_COMMAND_INTX_DIS;		// Ensure INTX signalling enabled
    SetRegW (Enum.PCIAddr, PCIREG_COMMAND, Cmd | PCI_CMD_BME_MEM);

    Int = GetRegB (Enum.PCIAddr, PCIREG_INT_LINE);
    if (Int == 0xFF) Int = 0;  // invalid interrupt setup!

    if (ClassCode.Subclass != PCI_IDE_CONTROLLER)
      ClassCode.ProgIF = PCI_IDE_BUSMASTER | PCI_IDE_NATIVE_IF1 | PCI_IDE_NATIVE_IF2;
    else {
      // 2011-07-27 SHL ticket #9 fixme to switch to compatibility if chipset supports if
      ClassCode.ProgIF &= PCI_IDE_BUSMASTER | PCI_IDE_NATIVE_IF1 | PCI_IDE_NATIVE_IF2;
    }

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

      if (!(npDev->Ident.Revision & NONSTANDARD_HOST) && Channel < 2) {
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
      npA->ProgIF.ProgIF   = ClassCode.ProgIF;
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

      if (npA->ProgIF.b.native) {
	if (!(npDev->Ident.Revision & MODE_NATIVE_OK) || !npA->IRQLevel) goto Recycle;
	if (!(npDev->Ident.Revision & NONSTANDARD_HOST) && !npC->BAR[4].Addr) goto Recycle;
      }

      Enable = GetRegB (Enum.PCIAddr, EnableReg);
      if (EnableBit & 0x08) {
	EnableBit ^= 0x0F;
	Enable = ~Enable;
      }
      if (Channel < 2 && !(Enable & (1 << EnableBit)) && !(npA->FlagsT & ATBF_BAY)) goto Recycle;

      CollectPorts (npA);
      npU[0].SStatus = npU[1].SStatus = 0;

      savedAdapterFlags = npA->FlagsT;
      savedUnitFlags[0] = npU[0].FlagsT;
      savedUnitFlags[1] = npU[1].FlagsT;

      if (npDev->Ident.ChipAccept (npA) && !HandleFoundAdapter (npA, npDev)) {
	count++;
	if (*(NPUSHORT)(&npC->IrqPIC) && (npC->IrqPIC != Int) && !(npA->FlagsT & ATBF_DISABLED))
	  APICRewire |= 1 << (npA - AdapterTable);

#       if TRACES
	  if (Debug & 8) TS("%d ",count)
#       endif
      } else {
	npA->FlagsT   = savedAdapterFlags; // restore cmdline options
	npU[0].FlagsT = savedUnitFlags[0];
	npU[1].FlagsT = savedUnitFlags[1];
Recycle:
	DATAREG = 0;  // return adapter slot for reuse
      }
    } // for channel
  } // while nextController

  if (npC->populatedChannels) npC++;	// Remember chip
  cChips = npC - ChipTable;		// Recalc count
  return (count);
}

/**
 * Handle found SATA adapter
 * @calledby EnumPCIDevices
 * @return FALSE if channels found else TRUE (sorta backwards)
 */

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
      if (!isAttached) {
	npU->FlagsT |= UTBF_DISABLED;
	// switch off unused ports
	OutD (SCONTROL, (InD (SCONTROL) & ~SCTRL_DET) | SCTRL_DISABLE);
	InD (SCONTROL);
      } // if !attached
    }
  } // for

  if (!hasPhy)
    isPopulated = ProbeChannel (npA);

  if (isPopulated || (npA->FlagsT & ATBF_BAY)) {
    npA->FlagsT |= ATBF_POPULATED;
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
    // enable interrupt sharing only if at least one of channels is in PCI native mode
    if (npA->ProgIF.b.native)
      npA->FlagsT |= ATBF_INTSHARED;

    npA->SGAlign = MEMBER(npA).SGAlign - 1;

    npC->populatedChannels++;		// Count channels used
    return (FALSE);			// OK

  } else {
    return (TRUE);			// No channels
  }
}
