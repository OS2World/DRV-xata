/**************************************************************************
 *
 * SOURCE FILE NAME = S506GEOM.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : Device Geometry and Features stuff
 ****************************************************************************/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#define INCL_DOSINFOSEG
#define INCL_NO_SCB
#define INCL_DOSERRORS
#include <os2.h>
#include <dskinit.h>

#include <iorb.h>
#include "strat2.h"
#include <reqpkt.h>
#include <dhcalls.h>
#include <addcalls.h>

#include "s506cons.h"
#include "s506type.h"
#include "s506pro.h"
#include "s506ext.h"

#include "Trace.h"

//#define FAKE_HUGE_DRIVE 1

#pragma optimize(OPTIMIZE, on)

#define PCITRACER 0
#define TRPORT 0x9004

extern RP_GENIOCTL IOCtlRP;

/**
 * Extract and determine the logical and physical geometry for unit.
 */

VOID DetermineUnitGeometry (NPU npU)
{
  NPA	 npA;
  NPGEO  npGEO;
  ULONG  MaxSec;
  USHORT SecPerCyl;
  UCHAR  rc;

  T('G') T('<')
  if ((npU->Flags & (UCBF_NOTPRESENT | UCBF_FORCE)) == UCBF_NOTPRESENT) return;
  npA = npU->npA;

  // Reconsile the physical geometries.

  if (npU->FlagsT & UTBF_IDEGEOMETRYVALID) {
    npGEO = &npU->IDEPhysGeom;
  } else {
    npU->Status = UTS_NOT_PRESENT;
  }
  npU->PhysGeom = *npGEO;

  if ((npU->Flags & (UCBF_NOTPRESENT | UCBF_ATAPIDEVICE)) == 0) {
    // Get the BIOS Parameter Block Logical geometry for this drive.

    if (!BPBGeometryGet (npU)) {
      if (npU->Flags & UCBF_ONTRACK)
	npU->PhysGeom.TotalSectors -= ONTRACK_SECTOR_OFFSET;

      if (npU->BPBLogGeom.NumHeads * npU->BPBLogGeom.SectorsPerTrack)
	npU->FlagsT |= UTBF_BPBGEOMETRYVALID;
    }

    // Get the BIOS reported and Logical geometry (only if BPB geom failed!)

    if (InitActive && !Int13GeometryGet (npU))
      if (npU->I13LogGeom.NumHeads * npU->I13LogGeom.SectorsPerTrack)
	npU->FlagsT |= UTBF_I13GEOMETRYVALID;
  }

  // Calculate a logical geometry based on the physical geometry.

  LogGeomCalculate (&npU->IDELogGeom, &npU->PhysGeom);
  if ((npU->IDELogGeom.NumHeads * npU->IDELogGeom.SectorsPerTrack) == 0)
    npU->FlagsT &= ~UTBF_IDEGEOMETRYVALID;

#if TRACES
  if (Debug & 16) {
    TWRITE(16)
    TraceStr ("LGeoB: H:%u S:%u",      npU->BPBLogGeom.NumHeads,
				       npU->BPBLogGeom.SectorsPerTrack);
    TWRITE(16)
    TraceStr ("LGeo3: H:%u S:%u",      npU->I13LogGeom.NumHeads,
				       npU->I13LogGeom.SectorsPerTrack);
    TWRITE(16)
    TraceStr ("LGeoI: C:%u H:%u S:%u", npU->IDELogGeom.TotalCylinders,
				       npU->IDELogGeom.NumHeads,
				       npU->IDELogGeom.SectorsPerTrack);
    TWRITE(16)
  }
#endif

  // Reconsile the logical geometries.

 if (npU->FlagsT & UTBF_BPBGEOMETRYVALID)
    npGEO = (NPGEO)&npU->BPBLogGeom;

  else if (npU->FlagsT & UTBF_I13GEOMETRYVALID)
    npGEO = (NPGEO)&npU->I13LogGeom;

  else if (npU->FlagsT & UTBF_IDEGEOMETRYVALID)
    npGEO = &npU->IDELogGeom;

  else
    npU->Status = UTS_NOT_PRESENT;

  // The physical geometry is the best source of the total number
  // of user addressable sectors.

  npU->LogGeom.NumHeads        = npGEO->NumHeads;
  npU->LogGeom.SectorsPerTrack = npGEO->SectorsPerTrack;
  rc = AdjustCylinders (&npU->LogGeom, npU->PhysGeom.TotalSectors);

  if (rc) {					  // cylinder overflow
    if (!(npU->Flags  & UCBF_NOTPRESENT) &&
	!(npU->FlagsT & UTBF_BPBGEOMETRYVALID))   // unformatted
      LogGeomCalculateLBAAssist ((NPGEO1)&npU->LogGeom, npU->PhysGeom.TotalSectors);
    npU->LogGeom.TotalSectors = (USHORT)(npU->LogGeom.NumHeads * npU->LogGeom.SectorsPerTrack)
			      * (ULONG)npU->LogGeom.TotalCylinders;
  }

  AdjustCylinders (&npU->PhysGeom, npU->PhysGeom.TotalSectors);
  AdjustCylinders (&npU->LogGeom , npU->LogGeom.TotalSectors);
  npU->MaxTotalCylinders = npU->LogGeom.TotalCylinders;
  npU->MaxTotalSectors	 = npU->LogGeom.TotalSectors;

  T('>')
}


UCHAR NEAR AdjustCylinders (NPGEO npGEO, ULONG TotalSectors) {
  USHORT SecPerCyl;
  UCHAR  rc = FALSE;

  /*---------------------------------------------*/
  /* Adjust the cylinder count in the physical	 */
  /* geometry to the last full cylinder.	 */
  /*---------------------------------------------*/

  npGEO->TotalSectors = TotalSectors;
  SecPerCyl = npGEO->SectorsPerTrack * npGEO->NumHeads;
  if (SecPerCyl > 0) {
    ULONG TotalCylinders  = TotalSectors / SecPerCyl;

#if TRACES
    if (Debug & 16) {
      TraceStr ("%lx/%x=%lx,",TotalSectors, SecPerCyl, TotalCylinders);
    }
#endif

    npGEO->TotalSectors   = TotalCylinders * SecPerCyl;
    npGEO->TotalCylinders = TotalCylinders;
    if (TotalCylinders >> 16) {
      npGEO->TotalCylinders = 65535;
      rc = TRUE;
    }
  }
  return (rc);
}

//
//   IdentifyGeometryGet()
//
// Perform an ATA identify function to get the drive's IDE geometry.  This is
// the best source of the physical geometry.  Calculate a corresponding logical
// geometry.  The calculated logical geometry will only be used as a last
// resort.
// This routine also checks for and enables drive capabilities based on the
// content of the drive's identify data.
//
BOOL NEAR IdentifyGeometryGet (NPU npU)
{
  NPIDENTIFYDATA npID;

  npID = ((NPIDENTIFYDATA)ScratchBuf) + npU->UnitIndex;

  T('a')
  // The routines in the following block have nothing to do with
  // the drive's geometry, but do require the drive's identify
  // data, so they are called here for convenience of accessing
  // the identify data.

  IDStringsExtract (npU->ModelNum, npID);
  IdleTimerEnable (npU, npID);
  UCBSetupDMAPIO (npU, npID);
  LPMEnable (npU, npID);

  if (npU->Flags & UCBF_ATAPIDEVICE) return (TRUE);

  // LBA is mandatory now!
  if (!(npID->IDECapabilities & FX_LBASUPPORTED) && !(npID->LBATotalSectors))
    return (TRUE);

  MediaStatusEnable (npU, npID);
  MultipleModeEnable (npU, npID);
  AcousticEnable (npU, npID);
  APMEnable (npU, npID);

  if (npID->GeneralConfig.Word == GC_CFA) {
    npU->Flags |= UCBF_CFA | UCBF_READY;
    npU->Flags &= ~UCBF_NOTPRESENT;
  }

  if (npU->FlagsT & UTBF_NOTUNLOCKHPA)
    npU->CmdEnabled &= ~UCBS_HPROT;
  else
    UnlockMax (npU, npID);

  // Get the physical geometry from the identify data.
  IDEGeomExtract (&npU->IDEPhysGeom, npID);
////////////////
#ifdef FAKE_HUGE_DRIVE
  if (npU->Features & 0xF0)
    if ((npU->Features & 0xF0) == 0xF0)
      npU->IDEPhysGeom.TotalSectors = (ULONG)-1;
    else
      npU->IDEPhysGeom.TotalSectors *= (npU->Features & 0xF0);
#endif
////////////////
  // Verify the extracted physcial geometry is reasonable.
  if (!PhysGeomValidate ((NPGEO1)&npU->IDEPhysGeom)) {
    T('b')
    npU->FlagsT |= UTBF_IDEGEOMETRYVALID;
    return (FALSE);
  }
  return (TRUE);
}


void NEAR UnlockMax (NPU npU, NPIDENTIFYDATA npID)
{
  volatile ULONG *pTotal;
  NPA	 npA = npU->npA;
  ULONG  RMaxL;
  USHORT RMaxH = 0;
  char	 isLBA48 = !!((npU->CmdSupported >> 16) & FX_LBA48SUPPORTED);

  if (!(npU->CmdEnabled & UCBS_HPROT)) return;

  npU->ReqFlags = UCBR_READMAX;
  NoOp (npU);
  if (isLBA48) {
    RMaxH = *(USHORT *)&LBA4;
    RMaxL = (*(ULONG *)&LBA0 & 0xFFFFFF) | ((ULONG)LBA3 << 24);
  } else {
    RMaxL = *(ULONG *)&LBA0 & 0xFFFFFFF;
  }
  if (++RMaxL == 0) RMaxH++;
#if TRACES
  TraceStr("rm(%d)%X%08lX,", isLBA48, RMaxH, RMaxL);
#endif

  npU->PhysGeom.SectorsPerTrack = npID->LogSectorsPerTrack;
  npU->PhysGeom.NumHeads	= npID->LogNumHeads;

  if (isLBA48) {
    if (npID->LBA48TotalSectorsH) {
      npU->FoundLBASize = -1;
      npU->CmdEnabled &= ~UCBS_HPROT;
      goto UnlockMaxExit;
    }
    pTotal = &npID->LBA48TotalSectorsL;
  } else {
    pTotal = &npID->LBATotalSectors;
  }

  npU->FoundLBASize = *pTotal;
  if (RMaxL == npU->FoundLBASize) {
    npU->CmdEnabled &= ~UCBS_HPROT;
    goto UnlockMaxExit;
  }

  npU->ReqFlags = UCBR_READMAX | UCBR_SETMAXNATIVE | UCBR_SETPARAM;
  NoOp (npU);
  IdentifyDevice (npU, npID);

  if (isLBA48) {
#if TRACES
    TraceStr("#%lX%08lX,", npID->LBA48TotalSectorsH, npID->LBA48TotalSectorsL);
#endif
    if ((npID->LBA48TotalSectorsH != RMaxH) || (npID->LBA48TotalSectorsL != RMaxL)) {
      /*
       * unlocking failed: try LBA28 unlock first and then LBA48 unlock
       */
      ((NPUSHORT)&(npU->CmdSupported))[1] &= ~FX_LBA48SUPPORTED;
      npU->ReqFlags = UCBR_READMAX | UCBR_SETMAXNATIVE;
      NoOp (npU);

      ((NPUSHORT)&(npU->CmdSupported))[1] |= FX_LBA48SUPPORTED;
      npU->ReqFlags = UCBR_READMAX | UCBR_SETMAXNATIVE | UCBR_SETPARAM;
      NoOp (npU);

      /* check again */

      IdentifyDevice (npU, npID);
#if TRACES
      TraceStr("#%lX%08lX,", npID->LBA48TotalSectorsH, npID->LBA48TotalSectorsL);
#endif
      if ((npID->LBA48TotalSectorsH != RMaxH) || (npID->LBA48TotalSectorsL != RMaxL))
	npU->CmdEnabled &= ~UCBS_HPROT;
    }
  } else {
    if (npID->LBATotalSectors != RMaxL) npU->CmdEnabled &= ~UCBS_HPROT;
  }

UnlockMaxExit:
  npU->ReqFlags = 0;

#if TRACES
  TS("P%X", !!((USHORT)npU->CmdEnabled & UCBS_HPROT));
  TS("N:%lX:",npU->FoundLBASize)
  if (isLBA48)
    TraceStr("%lX%08lX,", npID->LBA48TotalSectorsH, npID->LBA48TotalSectorsL);
  else
    TraceStr("%lX,", npID->LBATotalSectors);
#endif
}


//
//   IDEGeomExtract()
//
// Extract the physcial (default) geometry from the drive's identify
// data and determine the number of user accessable sectors the drive
// contains.
// If this is a large drive then the reported total number of sectors
// will not match the geometry and this routine will report the IDE
// physical geometry as invalid.  Such large drives are expected to
// contain more than: 15,481,935 user accessable sectors.
// (63 SectorsPerTrack * 15 Heads * 16383 Cylinders)
//
VOID NEAR IDEGeomExtract (NPGEO npIDEGeom, NPIDENTIFYDATA npId)
{
  ULONG logicalTotalSectors;

  // The ATA Identify data contains the physical, aka default,
  // IDE geometry in words 1, 3, and 6:
  //	word 1 = total cylinders
  //	word 3 = total heads
  //	word 6 = default sectors per track
  // Note: if total cylinders is reported as 0, it should be
  // interpreted as the maximum number of ATA cylinders.

  // Dani:
  // After a reset the *current* translated geometry is equal to the default
  // geometry. The BIOS might have changed that, so use that one if valid!

#if TRACES
  if (Debug & 16) {
    TraceStr ("%u/%u/%u,%u/%u/%u/%lu,%lu",
      npId->SectorsPerTrack,
      npId->NumHeads,
      npId->TotalCylinders,
      npId->LogSectorsPerTrack,
      npId->LogNumHeads,
      npId->LogNumCyl,
      npId->LogTotalSectors,
      npId->LBA48TotalSectorsL);
  }
#endif

  logicalTotalSectors = (ULONG)(npId->LogNumCyl)
		      * (ULONG)(npId->LogSectorsPerTrack * npId->LogNumHeads);
  if (npId->LBATotalSectors &&
      ((npId->LBATotalSectors - npId->LogTotalSectors) > (npId->LBATotalSectors >> 2)))
    logicalTotalSectors = 0;
  if ((logicalTotalSectors > 0) &&
      (logicalTotalSectors == npId->LogTotalSectors) &&
      (npId->LogNumHeads > 1)) {
    npIDEGeom->SectorsPerTrack = npId->LogSectorsPerTrack;
    npIDEGeom->NumHeads        = npId->LogNumHeads;
    npIDEGeom->TotalCylinders  = npId->LogNumCyl;
  } else {
    npIDEGeom->SectorsPerTrack = npId->SectorsPerTrack;
    npIDEGeom->NumHeads        = npId->NumHeads;
    npIDEGeom->TotalCylinders  = npId->TotalCylinders;
  }
  if (npIDEGeom->TotalCylinders == 0)
    npIDEGeom->TotalCylinders = ATA_MAX_CYLINDERS;

  // Compute the drive's legacy capacity based on the ATA Identify
  // reported physical (default) geometry.
  logicalTotalSectors = (USHORT)(npIDEGeom->SectorsPerTrack * npIDEGeom->NumHeads)
		      * (ULONG)npIDEGeom->TotalCylinders;

  // The identify data also contains the drive's actual capacity.
  // If drive's actual capacity is larger than the capacity
  // implied from the above physical geometry, then to access the
  // entire drive via BIOS, Enhanced BIOS support is required.

  if (npId->LBATotalSectors == 0) {
    // This is an old drive and has no LBA support and the reported
    // physical geometry contains the entire drive.

    npIDEGeom->TotalSectors = logicalTotalSectors;

  } else if (logicalTotalSectors == npId->LBATotalSectors) {
    // This drive has LBA support and is small enough that the
    // reported physical geometry contains the entire drive.

    npIDEGeom->TotalSectors = logicalTotalSectors;

  } else {
    // This is a new large drive, the entire drive cannot be
    // accessed from reported physical geometry.  The IDE reported
    // total number sectors does contain the entire drive.

    if (npId->CommandSetSupported[1] & FX_LBA48SUPPORTED) {
      if (npId->LBA48TotalSectorsH)
	npIDEGeom->TotalSectors = 0xFFFFFFFF;
      else
	npIDEGeom->TotalSectors = npId->LBA48TotalSectorsL;
    } else {
      npIDEGeom->TotalSectors = npId->LBATotalSectors;
    }
  }

#if TRACES
  if (Debug & 16) {
    TWRITE(16)
    TraceStr ("GeoI: C:%u H:%u S:%u T:%lu", npIDEGeom->TotalCylinders,
					    npIDEGeom->NumHeads,
					    npIDEGeom->SectorsPerTrack,
					    npIDEGeom->TotalSectors);
    TWRITE(16)
  }
#endif
}

//
//   LogGeomCalculate()
//
// Calculate a "best guess" logical geometry for this drive based
// entirely on the input physcial geometry.
//
VOID NEAR LogGeomCalculate (NPGEO npLogGeom, NPGEO npPhysGeom)
{
  if (npPhysGeom->TotalCylinders  <= BIOS_MAX_CYLINDERS &&
   /* npPhysGeom->NumHeads	  <= BIOS_MAX_NUMHEADS && */
      npPhysGeom->SectorsPerTrack <= BIOS_MAX_SECTORSPERTRACK) {
    // This is a small drive, < 8GB.  No translation or enhanced
    // BIOS is required to access the entire drive.  Use the physical
    // geometry as the logical geometry.

    *npLogGeom = *npPhysGeom;

  } else if (npPhysGeom->TotalSectors > TRANSLATED_MAX_TOTALSECTORS) {
    USHORT SecPerCyl;

    // The drive is large (> 8 GigaBytes) and is reporting its physical
    // geometry as larger that the largest compatible legacy geometry.
    // If this legacy geometry is used as the logical geometry then we
    // will not see the entire drive.  Adjust the logical geometry.

    // Leave NumHeads and SecPerTrk the same as the HW has reported it to be.

    npLogGeom->NumHeads = npPhysGeom->NumHeads;
    npLogGeom->SectorsPerTrack = npPhysGeom->SectorsPerTrack;

    // Expand the logical geometry to the entire drive by increasing the
    // number of logical cylinders.

  } else if (npPhysGeom->TotalCylinders > BIOS_MAX_CYLINDERS) {
    // This drive requires a CHS translation but no enhanced BIOS to
    // access the entire drive.

    LogGeomCalculateLBAAssist ((NPGEO1)npLogGeom, npPhysGeom->TotalSectors);
  }

  AdjustCylinders (npLogGeom, npPhysGeom->TotalSectors);
}


//
//   LogGeomCalculateLBAAssist()
//
// Calculate the logical geometry based on the input physcial geometry
// using the LBA Assist Translation algorithm.
//
VOID NEAR LogGeomCalculateLBAAssist (NPGEO1 npLogGeom, ULONG TotalSectors)
{
  UCHAR numSpT	 = BIOS_MAX_SECTORSPERTRACK;
  UCHAR numHeads = BIOS_MAX_NUMHEADS;
  ULONG Cylinders;

  if (TotalSectors <= (BIOS_MAX_CYLINDERS * 128 * BIOS_MAX_SECTORSPERTRACK)) {
    USHORT temp = (TotalSectors - 1)
		/ (BIOS_MAX_CYLINDERS * BIOS_MAX_SECTORSPERTRACK);

    if (temp < 16)	numHeads = 16;
    else if (temp < 32) numHeads = 32;
    else if (temp < 64) numHeads = 64;
    else		numHeads = 128;
  }

  do {				  // deal with *really* huge disks (> 502 GiB)
    Cylinders = TotalSectors / (USHORT)(numHeads * numSpT);
    if (Cylinders >> 16) {
      if (numSpT < 128)
	numSpT = (numSpT << 1) | 1;
      else
	Cylinders = 65535;	  // overflow !
    }
  } while (Cylinders >> 16);

  npLogGeom->TotalCylinders  = Cylinders;
  npLogGeom->NumHeads	     = numHeads;
  npLogGeom->SectorsPerTrack = numSpT;

#if TRACES
  if (Debug & 16) {
    TraceStr ("GLBA: C:%u H:%u S:%u T:%lu", npLogGeom->TotalCylinders,
					    npLogGeom->NumHeads,
					    npLogGeom->SectorsPerTrack,
					    TotalSectors);
    TWRITE(16)
  }
#endif
}

//
//   Int13GeometryGet()
//
BOOL NEAR Int13GeometryGet (NPU npU)
{
  BOOL	 rc;
  UCHAR  Disk, UnitId = npU->UnitId;
  OEMDiskInfo DiskInfo;

  if ((npU->Flags & UCBF_REMOVABLE) || (npU->DriveId < 0x80)) return (TRUE);

  fclrmem (&DiskInfo, sizeof (DiskInfo));

  /* Setup IOCTL Request Packet */
  IOCtlRP.Function = 0x0E;
  IOCtlRP.ParmPacket = (PUCHAR)&Disk;
  IOCtlRP.DataPacket = (PUCHAR)&DiskInfo;

  rc = TRUE;
  UnitId <<= 4;
  for (Disk = 0x80; Disk < (0x80 + MAX_ADAPTERS * 2); Disk++) {
    if (CallOEMHlp ((PRPH)&IOCtlRP)) break;
    if ((DiskInfo.BasePort == npU->npA->IOPorts[0]) &&
	((DiskInfo.HeadRegister & 0x10) == UnitId)) {
#if TRACES
#ifdef USE_EDD
  if (Debug & 16) TraceStr ("DI:%X/%X/%X/%X(%ld/%d/%d),",DiskInfo.BasePort,DiskInfo.HeadRegister,DiskInfo.Flags,DiskInfo.BIOSInfo,DiskInfo.Cylinders,(USHORT)DiskInfo.Heads,(USHORT)DiskInfo.SpT);
#else
  if (Debug & 16) TraceStr ("DI:%X/%X,",DiskInfo.BasePort,DiskInfo.HeadRegister);
#endif
#endif
      npU->DriveId = Disk;
#ifdef USE_EDD
      npU->I13LogGeom.TotalCylinders  = DiskInfo.Cylinders;
      npU->I13LogGeom.NumHeads	      = DiskInfo.Heads;
      npU->I13LogGeom.SectorsPerTrack = DiskInfo.SpT;
      if (DiskInfo.Flags & 2) {
	rc = FALSE;
      } else if ((DiskInfo.BIOSInfo & 0x608) == 0x008) {
	LogGeomCalculateBitShift (&npU->I13LogGeom, &npU->I13LogGeom);
	rc = FALSE;
      }
#endif
      break;
    }
  }

  if (rc && !(npU->FlagsT & UTBF_BPBGEOMETRYVALID))
    // Get the BIOS logical geometry from legacy Int 13h, function 8h.
    rc = Int13Fun08GetLogicalGeom (npU);

  return (rc);
}

/**
 * Override sectors/track and heads/cyl if valid LVM data found
 * @return TRUE if LVM data exists
 */

BOOL NEAR CheckLVM (NPGEO2 npGeo)
{
  UCHAR isLVM = FALSE;
  PDLA_Table_Sector pDLA = (PDLA_Table_Sector)&BootRecord;

  if ((pDLA->DLA_Signature1 == DLA_TABLE_SIGNATURE1) &&
      (pDLA->DLA_Signature2 == DLA_TABLE_SIGNATURE2)) {

    isLVM =
    npGeo->SectorsPerTrack = pDLA->Sectors_Per_Track;
    npGeo->NumHeads	   = pDLA->Heads_Per_Cylinder;
    TS("LVM:%d,",isLVM)
  }
  return (isLVM);
}

BOOL NEAR CheckMBRConsistency (NPGEO2 npGeo)
{
  UCHAR MBRconsistent;
  UCHAR partitionType;
  UCHAR i;

  MBRconsistent = FALSE;

  for (i = 0; i < BR_MAX_PARTITIONS; i++) {
    partitionType = BootRecord.partitionTable[i].systemIndicator;
    if (partitionType != 0) {
      UCHAR  Heads, Sectors;
      USHORT SectorsPerCylinder;

      Heads   = BootRecord.partitionTable[i].end.head + 1;	// 1..N
      Sectors = BootRecord.partitionTable[i].end.sector & 0x3F;	// 0..63, 0 should not occur
      SectorsPerCylinder = Heads * Sectors;

      if (SectorsPerCylinder) { // offset plus length must be an integral number of SPC
	ULONG LBA, Tracks;

	MBRconsistent = FALSE;
	LBA = BootRecord.partitionTable[i].length + BootRecord.partitionTable[i].offset;
	Tracks = LBA / Heads;
	if (Tracks * Heads != LBA) break;
	if (Tracks % Sectors) {
	  Sectors |= 0x40;
	  if (Tracks % Sectors) {
	    Sectors |= 0x80;
	    if (Tracks % Sectors) break;
	  }
	}
      } else {
	if (*(USHORT *)&(BootRecord.partitionTable[i].end.sector) == 0xFFFF)
	  continue;
	else
	  MBRconsistent = FALSE;
      }
      if ((npGeo->NumHeads != 0) && (npGeo->NumHeads != Heads)) break;
      if ((npGeo->SectorsPerTrack != 0) && (npGeo->SectorsPerTrack != Sectors)) break;

      npGeo->NumHeads	     = Heads;
      npGeo->SectorsPerTrack = Sectors;
      MBRconsistent = TRUE;
    } // if partitionType
  } // for
  if (i < BR_MAX_PARTITIONS) {
    npGeo->NumHeads	   = 0;
    npGeo->SectorsPerTrack = 0;
    MBRconsistent = FALSE;
  }
  TS("MBR:%d,",MBRconsistent)
  return (MBRconsistent);
}

//
//   BPBGeometryGet()
//
// Read the media's BIOS Parameter Block values for sectors per track and
// number of heads.  Return FALSE if valid information is being returned and
// TRUE if a valid BIOS Parameter Block could not be located.
//
BOOL NEAR BPBGeometryGet (NPU npU)
{
  NPA	 npA = npU->npA;
  UCHAR  i, j;
  UCHAR  partitionType;
  ULONG  extendedPartitionStartSector;
  ULONG  partitionBootRecordSector;
  UCHAR  containsExtendedPartition;
  UCHAR  MBRconsistent, isBPB;
  USHORT rc;

  // Initialize the return values.

  npU->BPBLogGeom.NumHeads	  = 0;
  npU->BPBLogGeom.SectorsPerTrack = 0;

  // Locate the partition table.  Read Sector (LBA) 0, the Master
  // Boot Record (MBR) and verify its signature.

  npU->LongTimeout = -1; // wait up to 30s on first read
  rc = ReadDrive (npU, 0, 1, (NPBYTE)&BootRecord);
  npU->LongTimeout = 0;

  if (rc || BootRecord.signature != BR_SIGNATURE)
    return TRUE;

  // look at the MBR and make an educated guess

  isBPB = FALSE;
  MBRconsistent = CheckMBRConsistency (&npU->BPBLogGeom);
  if (!MBRconsistent) {
    PPARTITION_BOOT_RECORD pBoot = (PPARTITION_BOOT_RECORD)&BootRecord;
    UCHAR  Heads, Sectors;
    USHORT SectorsPerCylinder;

    isBPB = (((pBoot->jumpNear[0] == 0xEB) && (pBoot->jumpNear[2] == 0x90)) ||
	     ((pBoot->jumpNear[0] == 0xE9) && (pBoot->jumpNear[2] == 0x00)));

    if (isBPB) isBPB = (pBoot->bytesPerSector == 512)  ||
		       (pBoot->bytesPerSector == 1024) ||
		       (pBoot->bytesPerSector == 2048);

    Heads = pBoot->heads;
    Sectors = pBoot->sectorsPerTrack;
    SectorsPerCylinder = Heads * Sectors;

    if (isBPB) isBPB = /* (Sectors <= BR_MAX_SECTORSPERTRACK) && */
		       (Heads <= BR_MAX_HEADS);

    if (isBPB) isBPB = (SectorsPerCylinder > 0) &&
		       ((pBoot->hiddenSectors % SectorsPerCylinder) == 0) &&
		       ((pBoot->totalSectors32 % SectorsPerCylinder) == 0);

    if (isBPB && (npU->FlagsT & UTBF_IDEGEOMETRYVALID))
      isBPB = (pBoot->totalSectors32 <= npU->IDEPhysGeom.TotalSectors);

    if (isBPB) {
      npU->BPBLogGeom.NumHeads	      = Heads;
      npU->BPBLogGeom.SectorsPerTrack = Sectors;
      return (FALSE);
    } else {
      return (TRUE);  // MBR is inconsistent (unformatted or just garbage)
		      // and doesn't contain a valid BPB.
    }
  } else {  // look for LVM data
    if (npU->BPBLogGeom.SectorsPerTrack) {
      if (!ReadDrive (npU, npU->BPBLogGeom.SectorsPerTrack - 1, 1, (NPBYTE)&BootRecord) &&
	  CheckLVM (&npU->BPBLogGeom))
	return (FALSE);
    }
    for (i = BIOS_MAX_SECTORSPERTRACK - 1; i > 0; i--)
      if (!ReadDrive (npU, i, 1, (NPBYTE)&BootRecord) &&
	  CheckLVM (&npU->BPBLogGeom) &&
	  (npU->BPBLogGeom.SectorsPerTrack == (i + 1)))
	return (FALSE);
  }

  // Look for Ez-Drive and On-Track drives.  If detected, reread
  // the partition table from the offset MBR and for
  //	On-Track: enable a 63 block offset and CHS translation
  //	Ez-Drive: enable floppy boot
  // then use the Ez-Drive partition table in LBA 1.
  for (i = 0; i < BR_MAX_PARTITIONS; i++) {
    partitionType = BootRecord.partitionTable[i].systemIndicator;

    if (partitionType == BR_PARTTYPE_ONTRACK) {
      // For On-Track controlled drives, the partition table
      // should be in LBA 63.  Reread the MBR from LBA 63.
      if (ReadDrive (npU, ONTRACK_SECTOR_OFFSET, 1, (NPBYTE)&BootRecord) ||
	  (BootRecord.signature != BR_SIGNATURE)) {
	return TRUE;
      } else {
	// This is an On-Track controlled drive.
	npU->Flags |= UCBF_ONTRACK;
	break;
      }
    } else if (partitionType == BR_PARTTYPE_EZDRIVE) {
      // For Ez-Drive controlled drives, the partiton table
      // should be in LBA 1.  Reread the MBR from LBA 1.
      if (ReadDrive (npU, 1, 1, (NPBYTE)&BootRecord) ||
	  (BootRecord.signature != BR_SIGNATURE)) {
	return TRUE;
      } else {
	// This is an Ez-Drive controlled drive.
	npU->Flags |= UCBF_EZDRIVEFBP;
	break;
      }
    }
  }

  // Look for a Primary Partition from which the BPB fields can be extracted.

  for (i = 0; i < BR_MAX_PARTITIONS; i++) {
    partitionType = BootRecord.partitionTable[i].systemIndicator;
    if (partitionType == BR_PARTTYPE_FAT01 ||
	partitionType == BR_PARTTYPE_FAT12 ||
	partitionType == BR_PARTTYPE_FAT16 ||
	partitionType == BR_PARTTYPE_FAT32 ||
	partitionType == BR_PARTTYPE_FAT32X ||
	partitionType == BR_PARTTYPE_OS2IFS) {
      // Read this primary partition's first block.  The first block is a
      // Partition Boot Record which contains the BIOS Parameter Block.

      partitionBootRecordSector = BootRecord.partitionTable[i].offset;
      if (!ReadAndExtractBPB (npU, partitionBootRecordSector,
			      &npU->BPBLogGeom)) {
	// Found a valid BPB, all done!
	return FALSE;
      } else {
	continue;
      }
    }
  }

  // No primary partition, so look for an extended partition and follow the
  // chain of extended partitions if necessary to get to a valid partition
  // boot record.

  for (i = 0; i < BR_MAX_PARTITIONS; i++) {
    partitionType = BootRecord.partitionTable[i].systemIndicator;
    if (partitionType == BR_PARTTYPE_EXTENDED ||
	partitionType == BR_PARTTYPE_EXTENDEDX) {
      extendedPartitionStartSector = BootRecord.partitionTable[i].offset;
      containsExtendedPartition = TRUE;
      while (containsExtendedPartition) {
	// This is an extended partition.  The first sector of an extended
	// partition has the same format as a Master Boot Record but is
	// called an Extended Boot Record.  There is a limit of 1 extended
	// partition per extended partition but this relationship is
	// recursive.

	if (ReadDrive (npU, extendedPartitionStartSector, 1,
		       (NPBYTE)&BootRecord) ||
	    (BootRecord.signature != BR_SIGNATURE)) {
	  containsExtendedPartition = FALSE;
	} else {
	  // Look through the Extended Boot Record for a valid logical
	  // block device partition.

	  for (j = 0; j < BR_MAX_PARTITIONS; j++) {
	    partitionType = BootRecord.partitionTable[j].
			    systemIndicator;
	    if (partitionType == BR_PARTTYPE_FAT01 ||
		partitionType == BR_PARTTYPE_FAT12 ||
		partitionType == BR_PARTTYPE_FAT16 ||
		partitionType == BR_PARTTYPE_FAT32 ||
		partitionType == BR_PARTTYPE_FAT32X ||
		partitionType == BR_PARTTYPE_OS2IFS) {
	      partitionBootRecordSector = extendedPartitionStartSector +
				   BootRecord.partitionTable[j].offset;
	      if (!ReadAndExtractBPB (npU, partitionBootRecordSector,
				      &npU->BPBLogGeom)) {
		 // Found a valid BPB, all done!
		return FALSE;
	      }
	    }
	  }

	  // Did not find an extended logical device partition, so look
	  // through this Extended Boot Record once more to find another
	  // extended partition contained within the current extended
	  // partition.

	  containsExtendedPartition = FALSE;  // Assume there are no more
	  for (j = 0; j < BR_MAX_PARTITIONS; j++) {
	    partitionType = BootRecord.partitionTable[j].systemIndicator;
	    if (partitionType == BR_PARTTYPE_EXTENDED ||
		partitionType == BR_PARTTYPE_EXTENDEDX) {
	      containsExtendedPartition = TRUE;
	      extendedPartitionStartSector += BootRecord.partitionTable[j].offset;
	      // Only 1 extended partition can be contained within an
	      // extended partition, so OK to quit.
	      break;
	    }
	  }
	}
      }
    }
  }

  return !MBRconsistent;  // Valid BPB was not found
}


//
//   ReadAndExtractBPB()
//
// Read the indicated sector, and interpret it as a Partition Boot Record.
// Verify the sector is a valid Partition Boot Record and if so return the
// BIOS Parameter Block's number of heads and sectors per track.  Return FALSE
// if valid data is being returned in these parameters and return TRUE if
// the BPB data is not valid.
//
BOOL NEAR ReadAndExtractBPB (NPU npU, ULONG partitionBootRecordSectorNum,
			     NPGEO2 npLogGeom)
{
  if (ReadDrive (npU, partitionBootRecordSectorNum, 1, (NPBYTE)&PartitionBootRecord) ||
     ((PartitionBootRecord.signature != PBR_SIGNATURE1) &&
      (PartitionBootRecord.signature != PBR_SIGNATURE2) &&
      (PartitionBootRecord.signature != PBR_SIGNATURE3))) {
    return TRUE;
  } else {
    // Validate the Partition Boot Record.
    // Check signature?? partitionBootRecord.signature
    // Extract the BPB fields.

    if ((PartitionBootRecord.sectorsPerTrack > 0)			&&
	(PartitionBootRecord.heads > 0) 				&&
//	  (PartitionBootRecord.sectorsPerTrack <= BR_MAX_SECTORSPERTRACK) &&
	(PartitionBootRecord.heads <= BR_MAX_HEADS)) {
      npLogGeom->SectorsPerTrack = PartitionBootRecord.sectorsPerTrack;
      npLogGeom->NumHeads	 = PartitionBootRecord.heads;
      return FALSE;
    } else {
      return TRUE;
    }
  }
}


//
//   PhysGeomValidate()
//
BOOL NEAR PhysGeomValidate (NPGEO1 npGeom)
{
  if ((npGeom->TotalCylinders > 1) &&
      (npGeom->NumHeads > 1) && (npGeom->NumHeads <= ATA_MAX_NUMHEADS) &&
      (npGeom->SectorsPerTrack > 1) /* && (npGeom->SectorsPerTrack <= ATA_MAX_SECTORSPERTRACK)*/)
    return (FALSE);

  return (TRUE);
}


//
//   IDStringsExtract()
//
// Extract the identification strings from the identify data.  Look for
// the Western Digital signature.
//
VOID NEAR IDStringsExtract (NPCH s, NPIDENTIFYDATA npID)
{
  /*---------------------------------------*/
  /* Save Model/Firmware Info		   */
  /*---------------------------------------*/
  memset (s, ' ', sizeof (npID->ModelNum) + sizeof (npID->FirmwareRN));
  strnswap (s, npID->ModelNum, sizeof (npID->ModelNum));
  strnswap (s + sizeof (npID->ModelNum), npID->FirmwareRN, sizeof (npID->FirmwareRN));
  s[sizeof (npID->ModelNum) + sizeof (npID->FirmwareRN)] = '\0';
}


//
//   MediaStatusEnable()
//
// Determine if this drive supports Media Status reporting.
//
VOID NEAR MediaStatusEnable (NPU npU, NPIDENTIFYDATA npID)
{
  NPA  npA = npU->npA;
  BOOL rc = TRUE;	// default to not enabled

  TS("M:%X,", npID->MediaStatusWord)

  // check for media status support
  if (npID->MediaStatusWord & 1) {

    // Media Status reporting is supported
    npU->Flags |= UCBF_MEDIASTATUS;
    rc = IssueSetFeatures (npU, FX_ENABLE_MEDIA_STATUS, 0);

    if (!rc) {
      npU->Flags |= UCBF_REMOVABLE; // indicate removable media
      npU->ReqFlags = ACBR_GETMEDIASTAT;
      NoOp (npU);
  TS("m%X,",npU->MediaStatus)
      if (!(ERROR & FX_ABORT)) {
	IssueSetFeatures (npU, FX_DISABLE_MEDIA_STATUS, 0);

	if (npU->MediaStatus & FX_NM) {
	  npU->Flags &= ~UCBF_READY;
	} else if (npU->MediaStatus & FX_MC) {
	  npU->Flags |= UCBF_BECOMING_READY;
	} else {
	  npU->Flags |= UCBF_READY;
	}
      }
    }
  }
}


//
//   MultipleModeEnable()
//
// Determine if this drive supports transfering multiple blocks
// blocks per interrupt.
//
VOID NEAR MultipleModeEnable (NPU npU, NPIDENTIFYDATA npID)
{
  USHORT cSec, cCur;
  USHORT cCap;
  UCHAR  cSMS;
  UCHAR  rc = FALSE;	   // default to not enabled

  /*---------------------------------------------------------*/
  /* Check for SET MULTIPLE support			     */
  /*							     */
  /* Note: We check a Bit 15 of NumSectorsPerInt which is a  */
  /*	   Vendor-Unique bit to validate that the drive has  */
  /*	   implemented Set Multiple support properly.	     */
  /* Note: Or Bit 1 of AdditionalWordsValid which is a	     */
  /*	   validation bit for the DMA information stored     */
  /*	   later in the Identify block.  If this bit is set  */
  /*	   then the drive is assumed to be relatively new    */
  /*	   and as such supports SMS.			     */
  /*							     */
  /*---------------------------------------------------------*/

  cSec = npID->NumSectorsPerInt;
  cCur = npID->LogNumSectorsPerInt;
  cCap = npID->AdditionalWordsValid;
  cSMS = ~(npU->SMSCount);

  if (cCur & 0x100) npU->FoundSecPerBlk = cCur & 0xFF;
  npU->SecPerBlk = 1;	// default value

  TSTR ("SMS:%X/%X/%d,", cSec, cCur, cSMS);

  if (npU->npA->FlagsT & ATBF_BIOSDEFAULTS) {
    if (npU->FoundSecPerBlk) {
      if ((npU->SecPerBlk = npU->FoundSecPerBlk) > 1)
	npU->Flags |= UCBF_MULTIPLEMODE;
      rc = TRUE;
    }
  } else {
    if ((cSec & FX_SECPERINTVALID) || (cCap & FX_WORDS64_70VALID)) {
      cSec &= 0xFF;
      if (cSMS < cSec) cSec = cSMS;
      if (cSec > 1) {	    // multiple mode is ok to use
	npU->SecPerBlk = cSec;
	rc = TRUE;
      }
    }
  }

  if (rc) {
    npU->Flags |= UCBF_SMSENABLED;
    if (npU->SecPerBlk > MAX_MULTMODE_BLK)
      npU->SecPerBlk = MAX_MULTMODE_BLK;
  }
}


//
//   IdleTimerEnable()
//
// (Determine if this drive supports the power management feature set
// and has these commands enabled. If not, idle time settings are ignored.)
//
VOID NEAR IdleTimerEnable (NPU npU, NPIDENTIFYDATA npID)
{
  /*-------------------------------------*/
  /* Check for Idle Time Setting	 */
  /*					 */
  /*-------------------------------------*/

  if (npU->IdleTime != 0) {
    USHORT Time, Data;

    Time = (UCHAR)~(npU->IdleTime);
    Data = Time * 12;
    Time = Data / 13;
    if (Time < 1) Time = 1;
    if (Data > 240) {
      Data = ((Data + (15 * 12)) / (30 * 12)) + 240;
      Time = (Data - 241) * 30;
      if (Time < 30) Time = 20;
      if (Data > 264) {   // > 12h
	Data = 0;	  // no idle
	Time = 0;
      }
      if (Data > 251) {
	Data = 253;	  // 8h
	Time = 7 * 60;
      }
    }
    npU->IdleTime = (UCHAR)~Data;
    if (Time > 0) npU->IdleCountInit = Time * 2 - 1;
  }
}

//
//   AcousticMgmtEnable()
//
// (Determine if this drive supports the acoustic management feature set
// and has these commands enabled. If not, noise level settings are ignored.)
//
VOID NEAR AcousticEnable (NPU npU, NPIDENTIFYDATA npID)
{
  /*-------------------------------------*/
  /* Check for Noise Lvl Setting	 */
  /*-------------------------------------*/

  if (npU->CmdSupported & UCBS_ACOUSTIC) {
    npU->FoundAMLevel = (UCHAR)(npID->AcousticManagement) & 0x7F;

    if (npU->AMLevel != 0) {
      UCHAR Noise;

      Noise = ~(npU->AMLevel);
      if (Noise > 126) Noise = 126;
      if (npU->FoundAMLevel != Noise)
	npU->AMLevel = ~Noise;
      else
	npU->AMLevel = 0;
    }
  } else {
    npU->FoundAMLevel = npU->AMLevel = 0;
  }
}

//
//   APMEnable()
//
// (Determine if this drive supports the advanced power management feature set
// If not, APM level settings are ignored.)
//
VOID NEAR APMEnable (NPU npU, NPIDENTIFYDATA npID)
{
  if (npU->CmdSupported & UCBS_APM) {
    TSTR ("APM:%X,", npU->APMLevel);
  } else {
    npU->APMLevel = 0;
  }
}

//
//   LPMEnable()
//
// Check for valid link power managent selection and infer preset value
// from SATA CONTROL register
//
VOID NEAR LPMEnable (NPU npU, NPIDENTIFYDATA npID)
{
  if ((npU->SATACmdSupported & FX_DIPMSUPPORTED) && SCONTROL) {
    if (npU->LPMLevel != 0) {
      UCHAR Level;

      Level = ~(npU->LPMLevel);
      switch (Level) {
	case 0:  Level = npU->FoundLPMLevel ^ 3;  // LPM as found in PHY
		 if (Level) Level ^= 0x83;
		 break;
	case 1:  Level = 0x80; break;		  // override: all modes
	case 2:  Level = 0x82; break;		  // override: no Slumber
	default: Level = 0x00; break;		  // override: no PM
      }
      npU->LPMLevel = Level;
      TSTR ("LPM:%X,", Level);
    }
  } else {
    npU->LPMLevel = 0;
  }
}

#if SUPPORT_DRIVETYPE // 17 Aug 10 SHL fixme to be gone if code will never be used

/*------------------------------------*/
/*				      */
/* DriveTypeToGeometry()	      */
/*				      */
/*------------------------------------*/

USHORT NEAR DriveTypeToGeometry (USHORT DriveType, NPGEO1 npGEO)
{
  if (DriveType < MAX_DRIVE_TYPES && DriveTypeTable[DriveType].Head != -1) {
    npGEO->TotalCylinders  = DriveTypeTable[DriveType].Cyl;
    npGEO->NumHeads	   = DriveTypeTable[DriveType].Head;
    npGEO->SectorsPerTrack = 17;
    return (0);
  } else {
    return (1);
  }
}
#endif
