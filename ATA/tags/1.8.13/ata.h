/**************************************************************************
 *
 * SOURCE FILE NAME =  ATA.H
 *
 * DESCRIPTIVE NAME =  ADD/DM include file
 *		       ATAPI Passthru structure
 *
 * COPYRIGHT Daniela Engert 2000-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * Purpose:	 Defines the ATAPI passthru structure.
 *
 ****************************************************************************/

#ifndef __ATA_H__
#define __ATA_H__

typedef struct _PassthruATA
{
  UCHAR        byPhysicalUnit;	   /* physical unit number 0-n */
				   /* 0 = Pri/Mas, 1=Pri/Sla, 2=Sec/Mas, etc. */
  struct
  {
    UCHAR	 Features;	   /* features register */
    UCHAR	 SectorCount;	   /* sector count register */
    UCHAR	 Lba0_SecNum;	   /* LBA0, sector number register */
    UCHAR	 Lba1_CylLo;	   /* LBA1, cylinder low register */
    UCHAR	 Lba2_CylHi;	   /* LBA2, cylinder high register */
    UCHAR	 Lba3;		   /* LBA3 register */
    UCHAR	 Lba4;		   /* LBA4 register */
    UCHAR	 Lba5;		   /* LBA5 register */
    UCHAR	 Command;	   /* command register */
  } TaskFileIn;
  struct
  {
    UCHAR	 Error; 	   /* error register */
    UCHAR	 SectorCount;	   /* sector count register */
    UCHAR	 Lba0_SecNum;	   /* LBA0, sector number register */
    UCHAR	 Lba1_CylLo;	   /* LBA1, cylinder low register */
    UCHAR	 Lba2_CylHi;	   /* LBA2, cylinder high register */
    UCHAR	 Lba3;		   /* LBA3 register */
    UCHAR	 Lba4;		   /* LBA4 register */
    UCHAR	 Lba5;		   /* LBA5 register */
    UCHAR	 Status;	   /* status register */
  } TaskFileOut;
  USHORT	RegisterMapW;	   /* bits to indicate what */
				   /* registers are to be */
				   /* transferred to/from H/W */
  USHORT	RegisterMapR;	   /* bits to indicate what */
				   /* registers are to be */
				   /* transferred to/from H/W */
} PassThruATA, NEAR* NPPassThruATA, FAR* PPassThruATA;

/*
 * Flags for RegisterTransferMap
 */

#define RTM_FEATURES  0x0002
#define RTM_SECCNT    0x0004
#define RTM_SECNUM    0x0008
#define RTM_CYLLO     0x0010
#define RTM_CYLHI     0x0020
#define RTM_LBA0      0x0008
#define RTM_LBA1      0x0010
#define RTM_LBA2      0x0020
#define RTM_COMMAND   0x0080
#define RTM_LBA3      0x0100
#define RTM_LBA4      0x0200
#define RTM_LBA5      0x0400

#define RTM_ERROR     0x0002
#define RTM_STATUS    0x0080

#define RTM_LBA (RTM_LBA0 | RTM_LBA1 | RTM_LBA2 | RTM_LBA3)
#define RTM_CYL (RTM_CYLLO | RTM_CYLHI)

#endif /* __ATA_H__ */
