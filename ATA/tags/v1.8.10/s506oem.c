/**************************************************************************
 *
 * SOURCE FILE NAME = S506OEM.C
 *
 * DESCRIPTIVE NAME = DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : Adapter Driver OEM Specific routines
 ****************************************************************************/

#define P4_LOOP 0

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
#include "s506type.h"
#include "s506ext.h"
#include "s506pro.h"

#pragma optimize(OPTIMIZE, on)

extern RP_GENIOCTL IOCtlRP;

BOOL FAR PciSetReg (USHORT PCIAddr, UCHAR ConfigReg, ULONG val, UCHAR size)
{
  switch (size) {
    case 1 : OutB2 (PCIAddr, PCICONFIG(ConfigReg), (UCHAR)val); break;
    case 2 : OutW2 (PCIAddr, PCICONFIG(ConfigReg), (USHORT)val); break;
    case 4 : OutD2 (PCIAddr, PCICONFIG(ConfigReg), val); break;
  }
  return (FALSE);
}

BOOL FAR PciGetReg (USHORT PCIAddr, UCHAR ConfigReg, PULONG val, UCHAR Size)
{
  switch (Size) {
    case 1: *(PUCHAR)val  = InB2 (PCIAddr, PCICONFIG(ConfigReg)); break;
    case 2: *(PUSHORT)val = InW2 (PCIAddr, PCICONFIG(ConfigReg)); break;
    case 4: *val	  = InD2 (PCIAddr, PCICONFIG(ConfigReg)); break;
  }
  return (FALSE);
}

VOID NEAR SetRegB (USHORT Addr, UCHAR ConfigReg, UCHAR val) {
  PciSetReg (Addr, ConfigReg, val, 1);
}

VOID NEAR SetRegW (USHORT Addr, UCHAR ConfigReg, USHORT val) {
  PciSetReg (Addr, ConfigReg, val, 2);
}

VOID NEAR SetRegD (USHORT Addr, UCHAR ConfigReg, ULONG val) {
  PciSetReg (Addr, ConfigReg, val, 4);
}

USHORT NEAR GetRegB (USHORT Addr, UCHAR ConfigReg) {
  UCHAR Data;
  if (!PciGetReg (Addr, ConfigReg, (PULONG)&Data, 1))
    return (Data);
  else
    return (0);
}

USHORT NEAR GetRegW (USHORT Addr, UCHAR ConfigReg) {
  USHORT Data;
  if (!PciGetReg (Addr, ConfigReg, (PULONG)&Data, 2))
    return (Data);
  else
    return (0);
}

ULONG NEAR GetRegD (USHORT Addr, UCHAR ConfigReg) {
  ULONG Data;
  if (!PciGetReg (Addr, ConfigReg, (PULONG)&Data, 4))
    return (Data);
  else
    return (0);
}

BOOL FAR ReadPCIConfigSpace (NPPCI_INFO npPCIInfo, UCHAR ConfigReg,
			     PULONG Data, UCHAR Size)
{
  NPC	 npC;
  NPCH	 pCfg, pData, p;
  ULONG  Port;
  USHORT PSize;
  UCHAR  through = (Size & 0x10) || !BIOSActive;
  UCHAR  isIO	 = (Size & 0x80);

  npC = npPCIInfo->npC;
  Size &= 7;

  if (!through) {
    pData = npC->CfgTblDriver;
    pCfg = (NPCH)(npPCIInfo->Ident.CfgTable);
    if (pCfg) {
      while (Port = *(pCfg++)) {
	UCHAR isPort;

	PSize = *(pCfg++);
	isPort = PSize & 0x80;
	PSize &= 7;

	if ((Port <= ConfigReg) && (isIO == isPort) && ((Port + PSize) >= (ConfigReg + Size))) {
	  p = pData + ConfigReg - Port;
	  switch (Size) {
	    case 1:  (UCHAR)*Data  = *(UCHAR *)p;  break;
	    case 2:  (USHORT)*Data = *(USHORT *)p; break;
	    case 4:  (ULONG)*Data  = *(ULONG *)p;  break;
	  }
	  return (FALSE);
	}
	pData += PSize;
      }
    }
  }

  if (isIO) {
    Port = ConfigReg + npC->BAR[4].Addr;
    switch (Size) {
      case 1: (UCHAR)*Data  = InB (Port); break;
      case 2: (USHORT)*Data = InW (Port); break;
      case 4: (ULONG)*Data  = InD (Port); break;
    }
  } else
    PciGetReg (npPCIInfo->PCIAddr, ConfigReg, (PULONG)Data, Size);

  return (FALSE);
}

USHORT NEAR ReadConfigB (NPPCI_INFO npPCIInfo, USHORT ConfigReg) {
  UCHAR Data;
  if (!ReadPCIConfigSpace (npPCIInfo, (UCHAR)(ConfigReg & 0xFF), (PULONG)&Data, (UCHAR)((ConfigReg >> 8) | 1)))
    return (Data);
  else
    return (0);
}

USHORT NEAR ReadConfigW (NPPCI_INFO npPCIInfo, UCHAR ConfigReg) {
  USHORT Data;
  if (!ReadPCIConfigSpace (npPCIInfo, ConfigReg, (PULONG)&Data, 2))
    return (Data);
  else
    return (0);
}

ULONG NEAR ReadConfigD (NPPCI_INFO npPCIInfo, UCHAR ConfigReg) {
  ULONG Data;
  if (!ReadPCIConfigSpace (npPCIInfo, ConfigReg, (PULONG)&Data, 4))
    return (Data);
  else
    return (0);
}

USHORT NEAR RConfigB (UCHAR Reg) { return (ReadConfigB (CurPCIInfo, (UCHAR)(Reg + RegOffset))); }
USHORT NEAR RConfigW (UCHAR Reg) { return (ReadConfigW (CurPCIInfo, (UCHAR)(Reg + RegOffset))); }

BOOL FAR WritePCIConfigSpace (NPPCI_INFO npPCIInfo, UCHAR ConfigReg,
			      ULONG Data, UCHAR Size)
{
  NPC	 npC;
  NPCH	 pCfg, pData, p;
  ULONG  Port;
  USHORT PSize;
  UCHAR  through = (Size & 0x10) || !BIOSActive;
  UCHAR  isIO	 = (Size & 0x80);

  npC = npPCIInfo->npC;
  Size &= 7;

  pData = npC->CfgTblDriver;
  pCfg = (NPCH)(npPCIInfo->Ident.CfgTable);
  if (pCfg) {
    while (Port = *(pCfg++)) {
      UCHAR isPort;

      PSize = *(pCfg++);
      isPort = PSize & 0x80;
      PSize &= 7;

      if ((Port <= ConfigReg) && (isIO == isPort) && ((Port + PSize) >= (ConfigReg + Size))) {
	p = pData + ConfigReg - Port;
	switch (Size) {
	  case 1: *(UCHAR *)p  = (UCHAR)Data;	break;
	  case 2: *(USHORT *)p = (USHORT)Data;	break;
	  case 4: *(ULONG *)p  = (ULONG)Data;	break;
	}
	if (!through) return (FALSE);
      }
      pData += PSize;
    }
  }

  if (isIO) {
    Port = ConfigReg + npC->BAR[4].Addr;
    switch (Size) {
      case 1: OutB (Port, (UCHAR)Data);  break;
      case 2: OutW (Port, (USHORT)Data); break;
      case 4: OutD (Port, Data);	 break;
    }
  } else {
    PciSetReg (npPCIInfo->PCIAddr, ConfigReg, Data, Size);
  }
  return (FALSE);
}

VOID NEAR WriteConfigB (NPPCI_INFO npPCIInfo, USHORT ConfigReg, UCHAR Data) {
  WritePCIConfigSpace (npPCIInfo, (UCHAR)(ConfigReg & 0xFF), Data, (UCHAR)((ConfigReg >> 8) | 1));
}

VOID NEAR WriteConfigBI (NPPCI_INFO npPCIInfo, UCHAR ConfigReg, UCHAR Data) {
  WritePCIConfigSpace (npPCIInfo, ConfigReg, Data, 0x11);
}

VOID NEAR WriteConfigW (NPPCI_INFO npPCIInfo, UCHAR ConfigReg, USHORT Data) {
  WritePCIConfigSpace (npPCIInfo, ConfigReg, Data, 2);
}

VOID NEAR WriteConfigD (NPPCI_INFO npPCIInfo, UCHAR ConfigReg, ULONG Data) {
  WritePCIConfigSpace (npPCIInfo, ConfigReg, Data, 4);
}

VOID NEAR WConfigBI (UCHAR Reg, UCHAR  Data) { WriteConfigBI(CurPCIInfo, (UCHAR)(Reg + RegOffset), Data); }
VOID NEAR WConfigB  (UCHAR Reg, UCHAR  Data) { WriteConfigB (CurPCIInfo, (UCHAR)(Reg + RegOffset), Data); }
VOID NEAR WConfigW  (UCHAR Reg, USHORT Data) { WriteConfigW (CurPCIInfo, (UCHAR)(Reg + RegOffset), Data); }
VOID NEAR WConfigD  (UCHAR Reg, ULONG  Data) { WriteConfigD (CurPCIInfo, (UCHAR)(Reg + RegOffset), Data); }

/*
**  ConfigurePCI()
**
**  Configure the PCI device according to the configuration records
**
*/
VOID FAR ConfigurePCI (NPC npC, USHORT InPhase)
{
  NPPCI_INFO npPCIInfo;
  NPCH	 pCfg, pData;
  ULONG  Port;
  UCHAR  Size, s;

  if (npC->npA[0])
    npPCIInfo = &npC->npA[0]->PCIInfo;
  else if (npC->npA[1])
    npPCIInfo = &npC->npA[1]->PCIInfo;
  else
    return;

  if (InPhase == PCIC_START) {
    pData = npC->CfgTblBIOS;
    pCfg = (NPCH)(npPCIInfo->Ident.CfgTable);
    while (Port = *(pCfg++)) {
      Size = (s = *(pCfg++)) & 7;
      if (s & 0x80) {
	Port += npC->BAR[4].Addr;
	switch (Size) {
	  case 1: *pData	   = InB (Port); break;
	  case 2: *(NPUSHORT)pData = InW (Port); break;
	  case 4: *(ULONG *)pData  = InD (Port); break;
	}
      } else {
	PciGetReg (npPCIInfo->PCIAddr, (UCHAR)Port, (PULONG)pData, Size);
      }
      pData += Size;
    }

    memcpy (npC->CfgTblDriver, npC->CfgTblBIOS, sizeof (npC->CfgTblBIOS));
  }

  if ((InPhase == PCIC_INIT_COMPLETE)  ||
      (InPhase == PCIC_RESUME)	       ||
      (InPhase == PCIC_SUSPEND)) {

    pData = (InPhase == PCIC_SUSPEND) || (InPhase == PCIC_START_COMPLETE) ?
	    npC->CfgTblBIOS : npC->CfgTblDriver;
    pCfg = (NPCH)(npPCIInfo->Ident.CfgTable);
    while (Port = *(pCfg++)) {
      Size = (s = *(pCfg++)) & 7;
      if (s & 0x80) {
	Port += npC->BAR[4].Addr;
	switch (Size) {
	  case 1: OutB (Port, *pData);		 break;
	  case 2: OutW (Port, *(NPUSHORT)pData); break;
	  case 4: OutD (Port, *(ULONG *)pData);  break;
	}
      } else {
	PciSetReg (npPCIInfo->PCIAddr, (UCHAR)Port, *(ULONG*)pData, Size);
      }
      pData += Size;
    }
  }

  if (InPhase == PCIC_INIT_COMPLETE)
    if (npPCIInfo->Ident.PCIFunc_InitComplete) {
      if (npC->npA[0])
	(npPCIInfo->Ident.PCIFunc_InitComplete)(npC->npA[0]);
      if (npC->npA[1])
	(npPCIInfo->Ident.PCIFunc_InitComplete)(npC->npA[1]);
    }
}

