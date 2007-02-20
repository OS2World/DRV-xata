/**************************************************************************
 *
 * SOURCE FILE NAME =  ATA.H
 *
 * DESCRIPTIVE NAME =  ADD/DM include file
 *		       ATAPI Passthru structure
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1996, 1997
 *	       COPYRIGHT Daniela Engert 2000-2006
 *
 * Purpose:	 Defines the ATAPI passthru structure.
 *
 ****************************************************************************/

#ifndef __ATA_H__
#define __ATA_H__

typedef struct _PassthruATA
{
  UCHAR        byPhysicalUnit;		   /* physical unit number 0-n */
					  /* 0 = Pri/Mas, 1=Pri/Sla, 2=Sec/Mas, etc. */
  struct
  {
    UCHAR	 Features;		   /* features register */
    UCHAR	 SectorCount;		   /* sector count register */
    UCHAR	 SectorNumber;		   /* sector number register */
    UCHAR	 CylinderLow;		   /* cylinder low register */
    UCHAR	 CylinderHigh;		   /* cylinder high register */
    UCHAR	 DriveHead;		   /* drive/head register */
    UCHAR	 Command;		   /* command register */
  } TaskFileIn;
  struct
  {
    UCHAR	 Error; 		   /* error register */
    UCHAR	 SectorCount;		   /* sector count register */
    UCHAR	 SectorNumber;		   /* sector number register */
    UCHAR	 CylinderLow;		   /* cylinder low register */
    UCHAR	 CylinderHigh;		   /* cylinder high register */
    UCHAR	 DriveHead;		   /* drive/head register */
    UCHAR	 Status;		   /* status register */
  } TaskFileOut;
  USHORT	RegisterTransferMap;	  /* bits to indicate what */
					  /* registers are to be */
					  /* transferred to/from H/W */
} PassThruATA, NEAR* NPPassThruATA, FAR* PPassThruATA;

/*
 * Flags for RegisterTransferMap
 */

#define PTA_RTMWR_FEATURES	  0x0002
#define PTA_RTMWR_SECTORCOUNT	  0x0004
#define PTA_RTMWR_SECTORNUMBER	  0x0008
#define PTA_RTMWR_CYLINDERLOW	  0x0010
#define PTA_RTMWR_CYLINDERHIGH	  0x0020
#define PTA_RTMWR_DRIVEHEAD	  0x0040
#define PTA_RTMWR_COMMAND	  0x0080

#define PTA_RTMWR_REGMASK	  0x00FF

#define PTA_RTMRD_ERROR 	  0x0200
#define PTA_RTMRD_SECTORCOUNT	  0x0400
#define PTA_RTMRD_SECTORNUMBER	  0x0800
#define PTA_RTMRD_CYLINDERLOW	  0x1000
#define PTA_RTMRD_CYLINDERHIGH	  0x2000
#define PTA_RTMRD_DRIVEHEAD	  0x4000
#define PTA_RTMRD_STATUS	  0x8000

#define PTA_RTMRD_REGMASK	  0xFF00
#define PTA_RTMRD_SHIFT 	  8

#endif /* __ATA_H__ */
