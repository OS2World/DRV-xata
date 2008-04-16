/**************************************************************************
 *
 * SOURCE FILE NAME =  S506REGS.H
 *
 * DESCRIPTIVE NAME =  DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2008
 *
 ****************************************************************************/

#ifndef __S506REGS_H__
#define __S506REGS_H__

/* Hard disk controller commands */

#define FX_NORETRY		0x01 /* do not perform retries on i/o	      */
#define FX_ECC			0x02 /* ECC mode during i/o		      */
#define FX_NULL 		0x00 /* NULL (packet/queued feature set only) */
#define FX_CFA_GET_EXT_ERROR	0x03 /* CFA get extended error		      */
#define FX_RECAL		0x10 /* recalibrate			      */
#define FX_CREAD		0x20 /* read				      */
#define FX_CREADEXT		0x24 /* read ext			      */
#define FX_CWRITE		0x30 /* write				      */
#define FX_CWRITEEXT		0x34 /* write ext			      */
#define FX_VERIFY		0x40 /* verify track.			      */
#define FX_VERIFYEXT		0x42 /* verify track. ext		      */
#define FX_SEEK 		0x70 /* seek track			      */
#define FX_SETP 		0x91 /* Set parameters			      */
#define FX_CPQPWMODE		0x98 /* Check if drive powered on	      */
#define FX_CPWRMODE		0xE5 /* Check if drive powered on	      */
#define FX_FLUSH		0xE7 /* flush cache			      */
#define FX_FLUSHEXT		0xEA /* flush cache ext 		      */
#define FX_IDENTIFY		0xEC /* Identify drive			      */
#define FX_CREADMUL		0xC4 /* Read Multiple			      */
#define FX_CREADMULEXT		0x29 /* Read Multiple ext		      */
#define FX_CWRITEMUL		0xC5 /* Write Multiple			      */
#define FX_CWRITEMULEXT 	0x39 /* Write Multiple ext		      */
#define FX_SETMUL		0xC6 /* Set Multiple Mode		      */
#define FX_DMAREAD		0xC8 /* DMA Read			      */
#define FX_DMAREADEXT		0x25 /* DMA Read ext			      */
#define FX_DMAWRITE		0xCA /* DMA Write			      */
#define FX_DMAWRITEEXT		0x35 /* DMA Write ext			      */
#define FX_GET_MEDIA_STATUS	0xDA /* get media status command	      */
#define FX_LOCK_DOOR		0xDE /* lock drive door 		      */
#define FX_UNLOCK_DOOR		0xDF /* unlock drive door		      */
#define FX_EJECT_MEDIA		0xED /* eject media			      */
#define FX_SETFEAT		0xEF /* Set Features			      */
#define FX_SMARTCMD		0xB0 /* SMART command			      */
#define FX_ENABLE_MEDIA_STATUS	0x95 /* enable media status command	      */
#define FX_DISABLE_MEDIA_STATUS 0x31 /* disable media status command	      */
#define FX_ACK_MEDIA_CHANGE	0xDB /* acknowledge media change	      */
#define FX_SETSTANDBY		0xE2 /* set standby timer		      */
#define FX_STANDBYIMM		0xE0 /* enter standby mode		      */
#define FX_SETIDLE		0xE3 /* set idle timer			      */
#define FX_IDLEIMM		0xE1 /* enter idle mode 		      */
#define FX_SETAMLVL		0x42 /* set acoustics management level	      */
#define FX_READMAX		0xF8 /* read native max address 	      */
#define FX_READMAXEXT		0x27 /* read native max address ext	      */
#define FX_SETMAX		0xF9 /* set max address 		      */
#define FX_SETMAXEXT		0x37 /* set max address ext		      */
#define FX_FREEZELOCK		0xF5 /* security freeze lock		      */

/* ATAPI Protocol Unique Commands */
#define FX_ATAPI_RESET		0x08 /* Reset ATAPI device		      */
#define FX_ATAPI_IDENTIFY	0xA1 /* Identify ATAPI device		      */

#define FX_SETXFERMODE		0x03 /* Set Transfer Mode		      */
#define FX_PIOMODE0IORDY	0x00 /*    PIO Mode 0			      */
#define FX_PIOMODE0		0x01 /*    PIO Mode 0 without Flow Control    */
#define FX_PIOMODEX		0x08 /*    PIO Mode x with Flow Control       */
#define FX_MWDMAMODEX		0x20 /*    multiword DMA Mode x 	      */
#define FX_ULTRADMAMODEX	0x40 /*   Ultra DMA Mode x - 5 4 3 2 1 0      */
#define FX_DEVICE_SPIN_UP	0x07
#define FX_ENABLE_WCACHE	0x02 /* Write Cache			      */
#define FX_DISABLE_WCACHE	0x82
#define FX_ENABLE_RAHEAD	0xAA /* Read look ahead 		      */
#define FX_DISABLE_RAHEAD	0x55
#define FX_ENABLE_APM		0x05 /* Enable APM (level)		      */
#define FX_APM_MIN		0x80 /* minimum power consumption w/o standby */
#define FX_APM_MAX		0xFE /* maximum power consumption w/o standby */
#define FX_DISABLE_APM		0x85
#define FX_ENABLE_CFA_POWER1	0x0A /* allow up to 500mA		      */
#define FX_ENABLE_SATA_FEATURE	0x10
#define FX_DISABLE_SATA_FEATURE 0x90
#define FX_SATA_FEATURE_DIPM	0x03 /* device initiated power state transitions */
#define FX_SATA_FEATURE_SSP	0x06 /* software settings preservation	      */

/* SMART functions */

#define FX_SMART_READ_VALUES	   0xD0
#define FX_SMART_READ_THRESHOLDS   0xD1
#define FX_SMART_AUTOSAVE	   0xD2
#define FX_SMART_SAVE		   0xD3
#define FX_SMART_IMMEDIATE_OFFLINE 0xD4
#define FX_SMART_READ_LOG	   0xD5
#define FX_SMART_WRITE_LOG	   0xD6
#define FX_SMART_ENABLE 	   0xD8
#define FX_SMART_DISABLE	   0xD9
#define FX_SMART_STATUS 	   0xDA
#define FX_SMART_AUTO_OFFLINE	   0xDB

/* Status bits */

#define FX_BUSY      0x80	  /* Status port busy bit		     */
#define FX_DRDY      0x40	  /* Status port ready bit		     */
#define FX_DF	     0x20	  /* Device Fault			     */
#define FX_DSC	     0x10	  /* Seek complete			     */
#define FX_DRQ	     0x08	  /* Data Request bit			     */
#define FX_CORR      0x04	  /* Corrected data			     */
#define FX_IDX	     0x02	  /* Index				     */
#define FX_ERROR     0x01	  /* Error status			     */

/* Device Control Register bits */

#define FX_nIEN      0x02	  /* Interrupt Enable (0=enable; 1=disable)  */
#define FX_SRST      0x04	  /* Soft Reset 			     */
#define FX_DCRRes    0x00	  /* Device Control Reg - reserved bits      */
#define FX_HOB	     0x80	  /* LBA48 read high order bits 	     */

/* Error Register bits */

#define FX_AMNF      0x01	  /* Address Mark Not Found		     */
#define FX_TRK0      0x02	  /* Track 0 not found during Restore Cmd    */
#define FX_NM	     0x02	  /* No Media				     */
#define FX_ABORT     0x04	  /* Aborted Command			     */
#define FX_MCR	     0x08	  /* Media eject requested		     */
#define FX_IDNF      0x10	  /* ID Not Found			     */
#define FX_MC	     0x20	  /* Media changed			     */
#define FX_ECCERROR  0x40	  /* Data ECC Error			     */
#define FX_WRT_PRT   0x40	  /* write protect			     */
#define FX_BADBLK    0x80	  /* Bad Block was detected in the ID field  */
#define FX_RD_PRT    0x80	  /* read protect			     */
#define FX_ICRC      0x80	  /* ULTRA DMA CRC error		     */

/* Error Register - Diagnostic Codes */

#define FX_DIAG_PASSED	  0x01	  /* All drives passed			     */
#define FX_DIAG_FORMATTER 0x02	  /* Formatter device error		     */
#define FX_DIAG_BUFFER	  0x03	  /* Sector buffer error		     */
#define FX_DIAG_ECC	  0x04	  /* ECC circuitry error		     */
#define FX_DIAG_uP	  0x05	  /* Controlling uP error		     */
#define FX_DIAG_DRIVE1	  0x80	  /* Drive 1 failed flag - Select Drive 1    */

/* I/O ports for AT hard drives */

#define PRIMARY_P    0x01F0	   /* Default primary	ports		      */
#define SECNDRY_P    0x0170	   /* Default secondary ports		      */
#define PRIMARY_C    0x03F6	   /* Default primary	control port	      */
#define SECNDRY_C    0x0376	   /* Default secondary control port	      */
#define PRIMARY_I	 14	   /* Fixed interrupt IRQ #		      */
#define SECNDRY_I	 15	   /* Second IDE interrupt IRQ #	      */

/* Array indices (IOPorts[] and IORegs[]) for I/O port address and contents   */

/* Output Regs */

#define FI_PDAT      0		   /* read/write data			      */

#define FI_PFEAT     1		   /* features register 		      */
#define FI_PSCNT     2		   /* sector count register		      */
#define FI_PINTRSN   2		   /* ATAPI interrupt reason		      */
#define FI_PLBA0     3		   /* sector number register		      */
#define FI_PLBA1     4		   /* cylinder register (low)		      */
#define FI_PLBA2     5		   /* cylinder register (high)		      */
#define FI_PDRHD     6		   /* drive/head register		      */
#define FI_PCMD      7		   /* command register			      */
#define FI_PLBA3     8		   /* LBA3 register			      */
#define FI_PLBA4     9		   /* LBA4 register			      */
#define FI_PLBA5    10		   /* LBA5 register			      */

#define FI_RFDR     11		   /* fixed disk register		      */
#define FI_PERR     12		   /* error register			      */
#define FI_PSTAT    13		   /* status register			      */

#define FM_PDAT    (1 << FI_PDAT ) /* read/write data		      */
#define FM_PFEAT   (1 << FI_PFEAT) /* features register 	      */
#define FM_PSCNT   (1 << FI_PSCNT) /* sector count register	      */
#define FM_LBA0    (1 << FI_PLBA0) /* LBA0 register		      */
#define FM_PCYLL   (1 << FI_PLBA1) /* cylinder register (low)	      */
#define FM_LBA1    (1 << FI_PLBA1) /* LBA1 register		      */
#define FM_PCYLH   (1 << FI_PLBA2) /* cylinder register (high)	      */
#define FM_LBA2    (1 << FI_PLBA2) /* LBA2 register		      */
#define FM_PDRHD   (1 << FI_PDRHD) /* drive/head register		      */
#define FM_PCMD    (1 << FI_PCMD ) /* command register		      */
#define FM_LBA3    (1 << FI_PLBA3) /* LBA3 register		      */
#define FM_LBA4    (1 << FI_PLBA4) /* LBA4 register		      */
#define FM_LBA5    (1 << FI_PLBA5) /* LBA5 register		      */
#define FM_HFEAT   (1 << 11	 )
#define FM_HSCNT   (1 << 12	 )
#define FM_LOW	   (FM_PFEAT | FM_PSCNT | FM_LBA0 | FM_LBA1 | FM_LBA2 | FM_PCMD)
#define FM_HIGH    (FM_HFEAT | FM_HSCNT | FM_LBA3 | FM_LBA4 | FM_LBA5)


#define DATAREG     (npA->IOPorts[FI_PDAT])
#define DEVCTL	    (npA->IORegs[FI_RFDR])
#define DEVCTLREG   (npA->CtrlPort)
#define FEAT	    (npA->IORegs[FI_PFEAT])
#define FEATREG     (npA->IOPorts[FI_PFEAT])
#define SECCNT	    (npA->IORegs[FI_PSCNT])
#define SECCNTREG   (npA->IOPorts[FI_PSCNT])
#define INTRSNREG   (npA->IOPorts[FI_PSCNT])
#define SECTOR	    (npA->IORegs[FI_PLBA0])
#define LBA0	    (npA->IORegs[FI_PLBA0])
#define SECTORREG   (npA->IOPorts[FI_PLBA0])
#define LBA0REG     (npA->IOPorts[FI_PLBA0])
#define LBA1	    (npA->IORegs[FI_PLBA1])
#define CYLL	    (npA->IORegs[FI_PLBA1])
#define CYLLREG     (npA->IOPorts[FI_PLBA1])
#define LBA1REG     (npA->IOPorts[FI_PLBA1])
#define LBA2	    (npA->IORegs[FI_PLBA2])
#define CYLH	    (npA->IORegs[FI_PLBA2])
#define CYLHREG     (npA->IOPorts[FI_PLBA2])
#define LBA2REG     (npA->IOPorts[FI_PLBA2])
#define LBA3	    (npA->IORegs[FI_PLBA3])
#define LBA3REG     (npA->IOPorts[FI_PLBA0])
#define LBA4	    (npA->IORegs[FI_PLBA4])
#define LBA4REG     (npA->IOPorts[FI_PLBA1])
#define LBA5	    (npA->IORegs[FI_PLBA5])
#define LBA5REG     (npA->IOPorts[FI_PLBA2])
#define DRVHD	    (npA->IORegs[FI_PDRHD])
#define DRVHDREG    (npA->IOPorts[FI_PDRHD])
#define COMMAND     (npA->IORegs[FI_PCMD])
#define COMMANDREG  (npA->IOPorts[FI_PCMD])
#define STATUS	    (npA->IORegs[FI_PSTAT])
#define STATUSREG   (npA->IOPorts[FI_PCMD])
#define ERROR	    (npA->IORegs[FI_PERR])
#define ERRORREG    (npA->IOPorts[FI_PFEAT])
#define BMSTATUS    (npA->BMSta)
#define BMSTATUSREG (npA->BMISTA)
#define BMCMDREG    (npA->BMICOM)
#define BMDTPREG    (npA->BMIDTP)
#define BMBASE	    (npA->BMICOM & ~0x0F)

#define ISRPORT     (npA->ISRPort)

#define SSTATUS     (npU->SStatus)
#define SERROR	    (npU->SError)
#define SCONTROL    (npU->SControl)

#define TESTBUSY (InB (STATUSREG) & FX_BUSY)
#define TESTDRDY ((InB (STATUSREG) & (FX_BUSY | FX_DRDY)) != FX_DRDY)

#define SSTAT_DET	    0x000F
#define SSTAT_DEV_OK	    0x1
#define SSTAT_COM_OK	    0x3
#define SSTAT_OFFLINE	    0x4
#define SSTAT_SPD	    0x00F0
#define SSTAT_SPD_1	    0x1
#define SSTAT_SPD_2	    0x2
#define SDIAG_PHYRDY_CHANGE 0x0001
#define SCTRL_DET	    SSTAT_DET
#define SCTRL_RESET	    0x1
#define SCTRL_DISABLE	    0x4
#define SCTRL_SPD	    SSTAT_SPD
#define SCTRL_SPD_LIMIT_1   SSTAT_SPD_1
#define SCTRL_IPM	    0x0F00
#define SCTRL_IPM_ALL	    0x0
#define SCTRL_IPM_NO_PARTL  0x1
#define SCTRL_IPM_NO_SLMBR  0x2
#define SCTRL_IPM_NONE	    0x3

//
// Boot Record Partiton Table Entry
//
typedef struct _PARTITION_ADDRESS
{
   UCHAR	     head;
   UCHAR	     sector;
   UCHAR	     cylinder;
} PARTITION_ADDRESS, *PPARTITION_ADDRESS;

typedef struct _PARTITION_TABLE_ENTRY
{
   UCHAR	     bootIndicator;
   PARTITION_ADDRESS start;
   UCHAR	     systemIndicator;
   PARTITION_ADDRESS end;
   ULONG	     offset;
   ULONG	     length;
} PARTITION_TABLE_ENTRY, *PPARTITION_TABLE_ENTRY;

// systemIndicator (partition type) definitions
#define BR_PARTTYPE_FAT01      0x01
#define BR_PARTTYPE_FAT12      0x04
#define BR_PARTTYPE_FAT16      0x06
#define BR_PARTTYPE_FAT32      0x0B
#define BR_PARTTYPE_FAT32X     0x0C
#define BR_PARTTYPE_OS2IFS     0x07
#define BR_PARTTYPE_EXTENDED   0x05
#define BR_PARTTYPE_EXTENDEDX  0x0F
#define BR_PARTTYPE_ONTRACK    0x54
#define BR_PARTTYPE_EZDRIVE    0x55

//
// Master or Extended Boot Record
//
#define BR_PROGRAM_LENGTH	446
#define BR_MAX_PARTITIONS	  4
#define BR_SIGNATURE	     0xAA55

// Boundary Limits for drives
#define BR_MAX_SECTORSPERTRACK	255 // 63
#define BR_MAX_HEADS		255
#define BR_MAX_PHYS_HEADS	 16

typedef struct _BOOT_RECORD
{
   UCHAR		 program[BR_PROGRAM_LENGTH];
   PARTITION_TABLE_ENTRY partitionTable[BR_MAX_PARTITIONS];
   USHORT		 signature;
} BOOT_RECORD, *PBOOT_RECORD;

//
// Partition or Extended Partition Boot Record
//
#define PBR_OEM_NAME_LENGTH	  8
#define PBR_VOLUME_LABEL_LENGTH  11
#define PBR_PROGRAM_LENGTH	193
#define PBR_SIGNATURE1	       0x28
#define PBR_SIGNATURE2	       0x29
#define PBR_SIGNATURE3	       0x80

typedef struct _PARTITION_BOOT_RECORD
{
   UCHAR		 jumpNear[3];
   CHAR 		 OEMNameAndVersion[PBR_OEM_NAME_LENGTH];
   USHORT		 bytesPerSector;
   UCHAR		 sectorsPerCluster;
   USHORT		 additionalBootSectors;
   UCHAR		 FATs;
   USHORT		 maxRootDirectoryEntries;
   USHORT		 totalSectors16;
   UCHAR		 mediaDescriptor;
   USHORT		 sectorsPerFAT;
   USHORT		 sectorsPerTrack;
   USHORT		 heads;
   ULONG		 hiddenSectors;
   ULONG		 totalSectors32;
   UCHAR		 physicalDriveNumber;
   UCHAR		 reserved01;
   UCHAR		 signature;
   ULONG		 volumeSerialNumber;
   CHAR 		 volumeLabel[PBR_VOLUME_LABEL_LENGTH];
   UCHAR		 reserved02[8];
   UCHAR		 program[PBR_PROGRAM_LENGTH];
   UCHAR		 reserved03[257];
} PARTITION_BOOT_RECORD, *PPARTITION_BOOT_RECORD;

/* The following definitions define the drive letter assignment table used by LVM.
   For each partition table on the disk, there will be a drive letter assignment table in the last sector
   of the track containing the partition table. */

/* NOTE: DLA stands for Drive Letter Assignment. */

#define DLA_TABLE_SIGNATURE1  0x424D5202L
#define DLA_TABLE_SIGNATURE2  0x44464D50L

/* Define the size of a Partition Name.  Partition Names are user defined names given to a partition. */
#define PARTITION_NAME_SIZE  20

/* Define the size of a volume name.  Volume Names are user defined names given to a volume. */
#define VOLUME_NAME_SIZE  20

/* Define the size of a disk name.  Disk Names are user defined names given to physical disk drives in the system. */
#define DISK_NAME_SIZE	  20

typedef struct _DLA_Entry { /* DE */
     ULONG Volume_Serial_Number;		/* The serial number of the volume that this partition belongs to. */
     ULONG Partition_Serial_Number;		/* The serial number of this partition. */
     ULONG Partition_Size;			/* The size of the partition, in sectors. */
     ULONG Partition_Start;			/* The starting sector of the partition. */
     UCHAR On_Boot_Manager_Menu;		/* Set to TRUE if this volume/partition is on the Boot Manager Menu. */
     UCHAR Installable; 			/* Set to TRUE if this volume is the one to install the operating system on. */
     char  Drive_Letter;			/* The drive letter assigned to the partition. */
     BYTE  Reserved;
     char  Volume_Name[VOLUME_NAME_SIZE];	/* The name assigned to the volume by the user. */
     char  Partition_Name[PARTITION_NAME_SIZE]; /* The name assigned to the partition. */
  } DLA_Entry;

typedef struct _DLA_Table_Sector { /* DTS */
     ULONG DLA_Signature1;	       /* The magic signature (part 1) of a Drive Letter Assignment Table. */
     ULONG DLA_Signature2;	       /* The magic signature (part 2) of a Drive Letter Assignment Table. */
     ULONG DLA_CRC;		       /* The 32 bit CRC for this sector.  Calculated assuming that this field and all unused space in the sector is 0. */
     ULONG Disk_Serial_Number;	       /* The serial number assigned to this disk. */
     ULONG Boot_Disk_Serial_Number;    /* The serial number of the disk used to boot the system.  This is for conflict resolution when multiple volumes
					  want the same drive letter.  Since LVM.EXE will not let this situation happen, the only way to get this situation
					  is for the disk to have been altered by something other than LVM.EXE, or if a disk drive has been moved from one
					  machine to another.  If the drive has been moved, then it should have a different Boot_Disk_Serial_Number.  Thus,
					  we can tell which disk drive is the "foreign" drive and therefore reject its claim for the drive letter in question.
					  If we find that all of the claimaints have the same Boot_Disk_Serial_Number, then we must assign drive letters on
					  a first come, first serve basis.											*/
     ULONG Install_Flags;	       /* Used by the Install program. */
     ULONG Cylinders;
     ULONG Heads_Per_Cylinder;
     ULONG Sectors_Per_Track;
     char  Disk_Name[DISK_NAME_SIZE];  /* The name assigned to the disk containing this sector. */
     UCHAR Reboot;		       /* For use by Install.  Used to keep track of reboots initiated by install. */
     BYTE  Reserved[3]; 	       /* Alignment. */
				       /* These are the four entries which correspond to the entries in the partition table. */
  } DLA_Table_Sector, *PDLA_Table_Sector;



/* Identify Data */

typedef struct _GENCONFIGWORD {
/*			 Bit Field(s)  DDD?   ZD Description	      */
/*					ZDAD? 3 		      */
/*					3   3 3 		      */

   unsigned int CmdPktSize   : 2;    /*  0- 1 Command Packet Size     */
   unsigned int Incomplete   : 1;    /*  2    response incomplete     */
   unsigned int 	     : 2;    /*  3- 4 Reserved		      */
   unsigned int CmdDRQType   : 2;    /*  5- 6 CMD DRQ Type	      */
   unsigned int Removable    : 1;    /*   7   Removable 	      */
   unsigned int DeviceType   : 5;    /*  8-12 Device Type	      */
   unsigned int 	     : 1;    /*  13   Reserved		      */
   unsigned int Protocol     : 2;    /* 14-15 Protocol Type	      */

}GENCONFIGWORD, NEAR *NPGENCONFIGWORD;

typedef struct _IDENTIFYDATA NEAR *NPIDENTIFYDATA, FAR *PIDENTIFYDATA;

typedef struct _IDENTIFYDATA
{
  union {				/*  0 General configuration bits      */
    USHORT	  Word;
    GENCONFIGWORD Bits;
  }		GeneralConfig;
  USHORT	TotalCylinders; 	/*  1 Default Translation - Num cyl   */
  USHORT	SpecificConfig; 	/*  2 Spin up type		      */
  USHORT	NumHeads;		/*  3			  - Num heads */
  USHORT	NumUnformattedbpt;	/*  4 Unformatted Bytes   - Per track */
  USHORT	NumUnformattedbps;	/*  5			  - Per sector*/
  USHORT	SectorsPerTrack;	/*  6 Default Translation - Sec/Trk   */
  USHORT	NumBytesISG;		/*  7 Byte Len - inter-sector gap     */
  USHORT	NumBytesSync;		/*  8	       - sync field	      */
  USHORT	NumWordsVUS;		/*  9 Len - Vendor Unique Info	      */
  CHAR		SerialNum[20];		/* 10 Serial number		      */
  USHORT	CtrlType;		/* 20 Controller Type		      */
  USHORT	CtrlBufferSize; 	/* 21 Ctrl buffer size - Sectors      */
  USHORT	NumECCBytes;		/* 21 ECC bytes -  read/write long    */
  CHAR		FirmwareRN[8];		/* 23 Firmware Revision 	      */
  CHAR		ModelNum[40];		/* 27 Model number		      */
  USHORT	NumSectorsPerInt;	/* 47 Multiple Mode - Sec/Blk	      */
  USHORT	DoubleWordIO;		/* 48 Double word IO Flag	      */
  USHORT	IDECapabilities;	/* 49 Capability Flags Word	      */
  USHORT	IDECapabilities2;	/* 50				      */
  USHORT	PIOMode;		/* 51 Transfer Cycle Timing - PIO     */
  USHORT	DMAMode;		/* 52			    - DMA     */
  USHORT	AdditionalWordsValid;	/* 53 Additional words valid	      */
  USHORT	LogNumCyl;		/* 54 Current Translation - Num Cyl   */
  USHORT	LogNumHeads;		/* 55			    Num Heads */
  USHORT	LogSectorsPerTrack;	/* 56			    Sec/Trk   */
  ULONG 	LogTotalSectors;	/* 57			    Total Sec */
  USHORT	LogNumSectorsPerInt;	/* 59				      */
  ULONG 	LBATotalSectors;	/* 60 ULONG Mode - Sectors	      */
  USHORT	DMADir; 		/* 62 ATAPI: DMADir	    ATA:SWDMA */
  USHORT	DMAMWordFlags;		/* 63				      */
  USHORT	AdvancedPIOModes;	/* 64 Advanced PIO modes supported    */
  USHORT	MinMWDMACycleTime;	/* 65 Minimum multiword DMA cycle time */
  USHORT	RecMWDMACycleTime;	/* 66 Recommended MW DMA cycle time   */
  USHORT	MinPIOCycleTimeWOFC;	/* 67 Minimum PIO cycle time without IORDY */
  USHORT	MinPIOCycleTime;	/* 68 Minimum PIO cycle time   */
  USHORT	Reserved2[71-69];	/* 69			       */
  USHORT	ReleaseDelay;		/* 71			       */
  USHORT	ServiceDelay;		/* 72			       */
  USHORT	ATAPIMajorVersion;	/* 73			       */
  USHORT	ATAPIMinorVersion;	/* 74			       */
  USHORT	QueueDepth;		/* 75			       */
  USHORT	SATACapabilities;	/* 76			       */
  USHORT	SATAreserved;		/* 77			       */
  USHORT	SATAFeatSupported;	/* 78			       */
  USHORT	SATAFeatEnabled;	/* 79			       */
  USHORT	MajorVersion;		/* 80			       */
  USHORT	MinorVersion;		/* 81			       */
  USHORT	CommandSetSupported[3]; /* 82			       */
  USHORT	CommandSetEnabled[3];	/* 85			       */
  USHORT	UltraDMAModes;		/* 88 Ultra DMA Modes	       */
  USHORT	Reserved5[93-89];	/* 89			       */
  USHORT	HardwareTestResult;	/* 93 hardware test result     */
  USHORT	AcousticManagement;	/* 94			       */
  USHORT	Reserved6[100-95];	/* 95			       */
  ULONG 	LBA48TotalSectorsL;	/* 100			       */
  ULONG 	LBA48TotalSectorsH;	/* 102			       */
  USHORT	Reserved7[127-104];	/* 104			       */
  USHORT	MediaStatusWord;	/* 127 media status word       */
  USHORT	Reserved8[256-128];	/*			       */

} IDENTIFYDATA;

#define FX_SECPERINTVALID	0x8000	/* Word 47 - Valid bit		      */
#define FX_LOGSECPERINTVALID	0x0100	/* Word 59 - Valid bit		      */
#define FX_DMASUPPORTED 	0x0100	/* Word 49 DMA supported bit	      */
#define FX_LBASUPPORTED 	0x0200	/* Word 49 ULONG supported bit		*/
#define FX_IORDYSUPPORTED	0x0800	/* Word 49 IORDY supported bit	      */
#define FX_DISIORDYSUPPORTED	0x0400	/* Word 49 disable IORDY supported bit*/
#define FX_POWERSUPPORTED	0x2000	/* Word 49    PowerMgmt supported bit */
#define FX_POWERSUPPORTED2	0x0008	/* Word 82/85 PowerMgmt supported bit */
#define FX_POWERSUPPORTED3	0x0008	/* Word 83/86 APM	supported bit */
#define FX_80PINCABLE		0x2000	/* Word 93   80pin cable detected bit */
#define FX_AMSUPPORTED		0x0200	/* Word 83   AcoustMgmt supported bit */
#define FX_SMARTSUPPORTED	0x0001	/* Word 82/85 PowerMgmt supported bit */
#define FX_WCACHESUPPORTED	0x0020	/* Word 82/85 WrCache	supported bit */
#define FX_HPROTSUPPORTED	0x0400	/* Word 82/85 Host prot supported bit */
#define FX_LBA48SUPPORTED	0x0400	/* Word 83    48Bit ULONG supported bit */
#define FX_FLUSHXSUPPORTED	0x2000	/* Word 83    Flush ext supported bit */
#define FX_DIPMSUPPORTED	0x0008	/* Word 78/79 dev-inited pm supported bit */
#define FX_SSPSUP		0x0040	/* Word 78/79 SATA SSP	supported bit */
#define FX_WORDS54_58VALID	0x0001	/* Words 54-58 in ID valid	      */
#define FX_WORDS64_70VALID	0x0002	/* Words 64-70 in ID valid	      */
#define FX_WORD88_VALID 	0x0004	/* Word 88 valid ->Ultra DMA	      */

#define BIOS_GET_LEGACY_DRIVE_PARAM	 0x08

#define ATASIGL 0x00
#define ATASIGH 0x00

#define SATASIGL 0x3C
#define SATASIGH 0x3C

#define ATAPISIGL 0x14
#define ATAPISIGH 0xEB

#define SATAPISIGL 0x69
#define SATAPISIGH 0x96

/* General Configuration defines       */
/*  protocols			       */
#define GC_PROT_ATA		 ( 0x00 | 0x01 )
#define GC_PROT_ATAPI		  0x02
/*  Device Types		       */
#define GC_DEVTYP_DIRECT_ACCESS   0x00
#define GC_DEVTYP_STREAMING_TAPE  0x01
#define GC_DEVTYP_CDROM 	  0x05
#define GC_DEVTYP_OPTICAL_MEMORY  0x07
#define GC_DEVTYP_UNKNOWN	  0x1F

#define GC_CFA			  0x848A

// Mini-VDM interface structures.
typedef struct _KMREGS {
  ULONG 	   R_EAX;
  ULONG 	   R_EBX;
  ULONG 	   R_ECX;
  ULONG 	   R_EDX;
  ULONG 	   R_EBP;
  ULONG 	   R_ESI;
  ULONG 	   R_EDI;
  ULONG 	   R_DS;
  ULONG 	   R_ES;
  ULONG 	   R_FS;
  ULONG 	   R_GS;
  ULONG 	   R_CS;
  ULONG 	   R_EIP;
  ULONG 	   R_EFLAGS;
  ULONG 	   R_SS;
  ULONG 	   R_ESP;
} _KMREGS;

#define EFLAGS_CF  0x00000001 // Carry Flag
#define EFLAGS_AF  0x00000010 // Auxiliary Carry Flag
#define EFLAGS_IF  0x00000200 // Interrupt Enable Flag

// See VDMINT13.Int13State

#define VDMINT13_IDLE	  0
#define VDMINT13_START	  1
#define VDMINT13_COMPLETE 2

//
// See VDMINT13.Int13Functions
//
#define MAX_INT13_FUNCTIONS	    5
#define INT13_FCN_DISKDDCALLOUT     0*8     //VDM     -> DISK DD
#define INT13_FCN_SIMULATEIRQ	    1*8     //DISK DD -> VDM
#define INT13_FCN_SIMULATETMR	    2*8     //DISK DD -> VDM

typedef struct _VDMINT13CB {

   // These vectors provide linkage between this device driver and
   // Int 13 mini-VDM.

   ULONG	   Int13Functions[MAX_INT13_FUNCTIONS*2];

   // These variables provide the address of the Linear=Real
   // I/O buffer mapped in the VDM address space.

   UCHAR  far	   *pInt13BufReal;
   UCHAR  far	   *pInt13BufLin;
   UCHAR  far	   *pInt13BufProt;
   USHORT	   Int13IOBufSectors;

   // This variable indicates where we are in processing an INT 13
   // request.
   //
   // Note: It is set to START by the device driver, and to COMPLETE
   //	    by the V86 code just prior to dispatching the request
   //	    to the Int 13 BIOS

   USHORT	   Int13State ;
   ULONG	   Int13CreateRc;


   UCHAR	   Int13PriClass; // These are variables used by the VDM
   UCHAR	   Int13PriLevel;
   ULONG	   Int13pTCB;
   ULONG	   Int13hvdm;
   ULONG	   Int13IRQCtxHook;
   USHORT	   Int13IRQPending;

   ULONG	   Reserved[10];

   _KMREGS	   Int13Regs;	  // Register set passed to the BIOS in V86 mode

} VDMINT13CB;

#pragma pack (1)

typedef struct {
  USHORT  reserved;
  USHORT  Flags;
  ULONG   Cylinders;
  ULONG   Heads;
  ULONG   SpT;
  ULONG   SectorsL, SectorsH;
  USHORT  BytesPerSector;
  ULONG   reserved1;
  USHORT  BasePort, StatusPort;
  UCHAR   HeadRegister;
  UCHAR   reserved2;
  UCHAR   IRQInfo;
  UCHAR   MultipleCount;
  UCHAR   DMAInfo;
  UCHAR   PIOInfo;
  USHORT  BIOSInfo;
  USHORT  reserved3;
  UCHAR   DPText;
} OEMDiskInfo, *NPOEMDiskinfo, * FAR POEMDISKINFO;

#endif
