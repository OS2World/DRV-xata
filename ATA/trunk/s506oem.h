/**************************************************************************
 *
 * SOURCE FILE NAME =  S506OEM.H
 *
 * DESCRIPTIVE NAME =  DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2009
 *
 * DESCRIPTION : Locally defined equates.
 *****************************************************************************/

#define  BUS_PCMCIA	 3
#define  BUS_EISA	 1
#define  BUS_PCI	 2
#define  BUS_UNKNOWN	 0

/*---------------------------------------------*/
/* PCI Defines				       */
/*---------------------------------------------*/

#define PCI_FUNC	    0x0b
#define PCI_GET_BIOS_INFO   0
#define PCI_FIND_DEVICE     1
#define PCI_FIND_CLASS_CODE 2
#define PCI_READ_CONFIG     3
#define PCI_WRITE_CONFIG    4

#define PCI_SUCCESSFUL		 0

#define MAX_PCI_BASE_REGS	 4
#define MAX_PCI_DEVICES 	32	/* max per system */
#define MAX_PCI_FUNCTIONS	 8	/* max per device */

/*---------------------------------*/
/*				   */
/* PCI Configuration Regs	   */
/*				   */
/*---------------------------------*/

#define PCIREG_VENDOR_ID	0x00
#define PCIREG_DEVICE_ID	0x02
#define PCIREG_COMMAND		0x04
#define PCIREG_STATUS		0x06
#define PCIREG_CLASS_CODE	0x08
#define PCIREG_REVISION 	0x08
#define PCIREG_PROGIF		0x09
#define PCIREG_SUBCLASS 	0x0A
#define PCIREG_CLASS		0x0B
#define PCIREG_CACHE_LINE	0x0C
#define PCIREG_LATENCY		0x0D
#define PCIREG_HEADER_TYPE	0x0E
#define PCIREG_BAR0		0x10
#define PCIREG_BAR1		0x14
#define PCIREG_BAR2		0x18
#define PCIREG_BAR3		0x1C
#define PCIREG_BAR4		0x20
#define PCIREG_BAR5		0x24
#define PCIREG_IO_BASE		PCIREG_BAR0
#define PCIREG_INT_LINE 	0x3C
#define PCIREG_MIN_GRANT	0x3E
#define PCIREG_MAX_LAT		0x3F
#define PCIREG_SUBORDINATE_BUS	0x1A

/*-----------------------------------*/
/* PCI Configuration Register values */
/*-----------------------------------*/

#define PCI_MASS_STORAGE	0x01	/* PCI Base Class code */
#define PCI_IDE_CONTROLLER	0x01	/* PCI Sub Class code  */
#define PCI_RAID_CONTROLLER	0x04	/* PCI Sub Class code  */
#define PCI_SATA_CONTROLLER	0x06	/* PCI Sub Class code  */
#define PCI_IDE_NATIVE_IF1	0x01
#define PCI_IDE_NATIVE_IF2	0x04
#define PCI_IDE_BUSMASTER	0x80
#define PCI_STATUS_MASTER_ABORT 0x2000
#define PCI_COMMAND_INTX_DIS	0x0400
#define PCI_CLASS_HOST_BRIDGE	0x0600
#define PCI_CLASS_ISA_BRIDGE	0x0601
#define PCI_BARTYPE_IO		0x01

typedef enum {
  undetermined = 0,
  Cmd640, RZ1000, Intel, Via, ALi, SiS, CMD64x, Promise, Cyrix,
  HPT36x, HPT37x, AEC, ServerWorks, Opti, AMD, NVidia, PromiseTX,
  SiI68x, ITE, NetCell, ATI, SiISata, JMicron, PromiseMIO, Marvell, Initio,
  AHCI, BcmSata,
  generic = -1
} tHardwareType;

/*
** Indices into the PCIDevice[] array of supported PCI devices,
** defined in s506data.c.
*/
#define PCID_PIIX	    0  /* Intel PIIX IDE */
#define PCID_VIA571	    1  /* VIA 571 IDE */
#define PCID_ALi5229	    2  /* ALi 5229 IDE */
#define PCID_SiS5513	    3  /* SiS 5513 IDE */
#define PCID_CMD64x	    4  /* CMD 64x  IDE */
#define PCID_PDCx	    5  /* Promise UltraX */
#define PCID_PDCTX2x	    6  /* Promise UltraX TX2 */
#define PCID_CX5530	    7  /* Cyrix 5530 IDE */
#define PCID_HPT366	    8  /* HighPoint 366 */
#define PCID_AMD756	    9  /* AMD 756 IDE */
#define PCID_AEC62x	   10  /* ARTOP AEC62x */
#define PCID_SMSC	   11  /* SLC 90C66 */
#define PCID_SW 	   12  /* ServerWorks SB */
#define PCID_OPTI	   13  /* Opti */
#define PCID_nForce	   14  /* NVidia nForce */
#define PCID_Geode	   15  /* NS Geode SCx200 */
#define PCID_SII680	   16  /* SII680 */
#define PCID_ITE	   17  /* ITE 8212 */
#define PCID_AEC2	   18  /* Artop ATP867 */
#define PCID_NetCell	   19  /* NetCell SyncRAID */
#define PCID_ATIIXP	   20  /* ATI IXP */
#define PCID_ATIIXPSATA    21  /* ATI IXP SATA */
#define PCID_JMicron	   22  /* JMicron SATA */
#define PCID_PromiseMIO    23  /* Promise MMIO */
#define PCID_Marvell	   24  /* Marvell */
#define PCID_ATIIXPAHCI    25  /* ATI IXP AHCI */
#define PCID_Initio	   26  /* Initio */
#define PCID_Broadcom	   27  /* Broadcom */
#define PCID_IntelSCH	   28  /* Intel SCH "Atom" */
#define PCID_ITE8213	   29  /* ITE8213 (ICH8M-lookalike)*/
#define PCID_Generic	   30  /* Generic IDE */

#define MAX_PCI_DEVICE_DESCRIPT  PCID_Generic+1

/*
** Define PCI Configuration space access locations.
*/
#define PCI_CONFIG_ADDRESS   0x0CF8
#define PCI_CONFIG_DATA      0x0CFC

/*-------------------------------*/
/* PCI Structures		 */
/* -----------------		 */
/*				 */
/*				 */
/*-------------------------------*/

typedef struct _PCI_IDENT {
   USHORT     Device;
   USHORT     Vendor;
   UCHAR      Index;	  // in PCI device array: hardware type
   UCHAR      Revision;   // in PCI device array: PCI native mode allowed
   UCHAR      TModes;	  // PIO, MWDMA, Channel Timing restrictions
   UCHAR      SGAlign;	  // DMA buffer alignment restriction

   /* PCI device specific functions to call:	       */
   BOOL       (NEAR *ChipAccept) (struct _A NEAR *);
   NPUSHORT   CfgTable;
   VOID       (NEAR *PCIFunc_InitComplete)(struct _A NEAR *);
   int	      (NEAR *CheckIRQ) (struct _A NEAR *);
   USHORT     (NEAR *GetPIOMode) (struct _A NEAR *, UCHAR);
   VOID       (NEAR *Setup) (struct _A NEAR *);
   VOID       (NEAR *CalculateTiming) (struct _A NEAR *);
   VOID       (NEAR *TimingValue) (struct _U NEAR *);
   VOID       (NEAR *ProgramChip) (struct _A NEAR *);
#if ENABLEBUS
   VOID       (NEAR *EnableBus) (struct _A NEAR *, UCHAR);
#endif
   VOID       (NEAR *EnableInterrupts) (struct _A NEAR *);

   VOID       (NEAR *SetTF) (struct _A NEAR *, USHORT);
   VOID       (NEAR *GetTF) (struct _A NEAR *, USHORT);
   VOID       (NEAR *SetupDMA) (struct _A NEAR *);
   VOID       (NEAR *StartDMA) (struct _A NEAR *);
   VOID       (NEAR *StopDMA) (struct _A NEAR *);
   VOID       (NEAR *ErrorDMA) (struct _A NEAR *);

   VOID       (NEAR *PreReset) (struct _A NEAR *);
   VOID       (NEAR *PostReset) (struct _A NEAR *);
   VOID       (NEAR *PostInitUnit) (struct _U NEAR *);
   VOID       (NEAR *StartStop) (struct _A NEAR *, UCHAR);
} PCI_IDENT, NEAR *NPPCI_IDENT;

/* Discovered at initialization, run-time resident, 1 per IDE IF */
typedef struct _PCI_INFO {
   USHORT     PCIAddr;
   PCI_IDENT  Ident;
   USHORT     CompatibleID;	/* compatible PCI device ID */
   struct _C NEAR *npC; 	/* chip */
   UCHAR      Level;
} PCI_INFO, NEAR *NPPCI_INFO;

/* Identifies PCI devices, 1 per supported PCI device. */
typedef struct _PCI_DEVICE {
   PCI_IDENT  Ident;
   NPUSHORT   ChipListCompatible, ChipListFamily;
   UCHAR      EnableReg0, EnableReg1; // PCI cfg register to check for enabled channel
   UCHAR      EnableBit;  // bit to test (0-3: prim, 4-7: sec) in register
			  // x=0-7: bit x needs to be set
			  // x=F-8: bit ~x needs to be clear
   UCHAR      xBMask;	  // related bridge device (PCI function 0!)
			  // bit 3-7: and mask, bit 0-2: or mask
   UCHAR      Latency;	  // PCI latency; bit 0=0: min latency, 1: max latency
   NPSZ       npDeviceMsg;	/* Verbose mode identification string		    */
} PCI_DEVICE, NEAR *NPPCI_DEVICE;

#define CHIPCAP_ATAPIO32   0x0001
#define CHIPCAP_ATADMARD   0x0002
#define CHIPCAP_ATADMAWR   0x0004
#define CHIPCAP_ATAPIPIO32 0x0008
#define CHIPCAP_ATAPIDMARD 0x0010
#define CHIPCAP_ATAPIDMAWR 0x0020
#define CHIPCAP_LBA48DMA   0x0040
#define CHIPCAP_SATA	   0x0080
#define CHIPCAP_ATA33	   0x0100
#define CHIPCAP_ATA66	   0x0200
#define CHIPCAP_ATA100	   0x0400
#define CHIPCAP_ATA133	   0x0800
#define CHANREQ_ATA66	   0x1000
#define CHANREQ_ATA100	   0x2000
#define CHANREQ_ATA133	   0x4000
#define CHANCAP_CABLE80    0x8000

#define CHIPCAP_PIO32	 (CHIPCAP_ATAPIO32   | CHIPCAP_ATAPIPIO32)
#define CHIPCAP_ATADMA	 (CHIPCAP_ATADMARD   | CHIPCAP_ATADMAWR)
#define CHIPCAP_ATAPIDMA (CHIPCAP_ATAPIDMARD | CHIPCAP_ATAPIDMAWR)
#define CHIPCAP_ULTRAATA (CHIPCAP_ATA33 | CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133)
#define CHIPCAP_DEFAULT  (CHIPCAP_PIO32 | CHIPCAP_ATADMA | CHIPCAP_LBA48DMA | CHIPCAP_ATAPIDMA | CHIPCAP_ATA33)
#define CHANCAP_SPEED	 (CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133)

#define TR_PIO_EQ_DMA	    0x01      /* PIO and MWDMA share timing registers	  */
#define TR_PIO0_WITH_DMA    0x02      /* MWDMA despite PIO mode 0 (PIIX)	  */
#define TR_PIO_SHARED	    0x04      /* master and slave share PIO/MWDMA timings */
#define TR_UDMA_SHARED	    0x08      /* master and slave share UDMA timings */
#define TR_SAME_DMA_TYPE    0x10      /* master and slave must be both MWDMA or UDMA */

#define MODE_COMPAT_ONLY       0      /* only compatibility mode is allowed */
#define MODE_NATIVE_OK	       1      /* native mode is allowed */
#define INTX_DIS	    0x20      /* enable INTX irq routing */
#define SIMPLEX_DIS	    0x40      /* disable simplex bit */
#define NONSTANDARD_HOST    0x80      /* doesn't follow T13 ATA host standard */

#define PCIC_SUSPEND	    0x0001    /* Kernel suspend    */
#define PCIC_RESUME	    0x0002    /* Kernel resume	   */
#define PCIC_START	    0x0004    /* Before driver initializes the HW, bios initialized state */
#define PCIC_START_COMPLETE 0x0008    /* After driver initializes the HW, before returning to bios */
#define PCIC_INIT_COMPLETE  0x0010    /* All Driver's initialization is complete, BIOS is done with HW */

typedef struct {
   UCHAR      PCISubFunc;
   USHORT     Device, Vendor;
   UCHAR      Index;
}PCI_PARM_FIND_DEV;

typedef struct {
   UCHAR  PCISubFunc;
   ULONG  ClassCode;
   UCHAR  Index;
}PCI_PARM_FIND_CLASSCODE;

typedef struct {
   UCHAR  PCISubFunc;
   UCHAR  BusNum;
   UCHAR  DevFunc;
   UCHAR  ConfigReg;
   UCHAR  Size;
}PCI_PARM_READ_CONFIG;

typedef struct {
   UCHAR  PCISubFunc;
   UCHAR  BusNum;
   UCHAR  DevFunc;
   UCHAR  ConfigReg;
   UCHAR  Size;
   ULONG  Data;
}PCI_PARM_WRITE_CONFIG;

typedef struct _PCI_DATA {
   UCHAR bReturn;
   union {
      struct {
	 UCHAR HWMech;
	 UCHAR MajorVer;
	 UCHAR MinorVer;
	 UCHAR LastBus;
      } Data_Bios_Info;
      struct {
	 UCHAR	BusNum;
	 UCHAR	DevFunc;
      }Data_Find_Dev;
      union {
	 ULONG	ulData;
	 USHORT usData;
	 UCHAR	ucData;
      }Data_Read_Config;
   };
} PCI_DATA, FAR *PPCI_DATA;

/*---------------------------------------------*/
/* ONTRACK Support			       */
/*					       */
/* OnTrack geometry constants.		       */
/*---------------------------------------------*/
/*
** The maximum number of Ontrack cylinders is:
**   1024 Cylinders * 255 Heads * 63 Sectors = 16,450,560
**					     = 0x00fb0400 Total Sectors
*/
#define MAX_ONTRACK_CYLINDERS		   1024L
#define MAX_ONTRACK_HEADS		    255L
#define MIN_ONTRACK_HEADS		      4L
#define ONTRACK_SECTORS_PER_TRACK	     63L
#define MAX_ONTRACK_SECTORS_PER_HEAD	  (ULONG)(ONTRACK_SECTORS_PER_TRACK * MAX_ONTRACK_CYLINDERS)
#define MAX_ONTRACK_SECTORS		  (ULONG)(MAX_ONTRACK_SECTORS_PER_HEAD * MAX_ONTRACK_HEADS)
#define ONTRACK_SECTOR_OFFSET		     63

/*-------------------------------*/
/* Promise Controller Support	 */
/* --------------------------	 */
/*				 */
/*-------------------------------*/

#define BIOS_MEM_LOW	0x000C0000l
#define BIOS_MEM_HIGH	0x000E0000l
#define BIOS_SIZE	(16*1024l)

#define PROMISE_ID_STRING_OFFSET  0x10

typedef UCHAR	BYTE, NEAR *NPBYTE, FAR *PBYTE;
typedef USHORT	WORD, NEAR *NPWORD, FAR *PWORD;
typedef ULONG	DWORD, NEAR *NPDWORD, FAR *PDWORD;

#define STATUS_ERR_INVPAR	    0x0013

/*
 * IOCTL Function Code Definitions
 */

#define DSKSP_CAT_SMART 	    0x80  /* SMART IOCTL category */

#define DSKSP_SMART_ONOFF	    0x20  /* turn SMART on or off */
#define DSKSP_SMART_AUTOSAVE_ONOFF  0x21  /* turn SMART autosave on or off */
#define DSKSP_SMART_SAVE	    0x22  /* force save of SMART data */
#define DSKSP_SMART_GETSTATUS	    0x23  /* get SMART status (pass/fail) */
#define DSKSP_SMART_GET_ATTRIBUTES  0x24  /* get SMART attributes table */
#define DSKSP_SMART_GET_THRESHOLDS  0x25  /* get SMART thresholds table */
#define DSKSP_SMART_GET_LOG	    0x26  /* get SMART log	  table */
#define DSKSP_SMART_AUTO_OFFLINE    0x27  /* set SMART offline autosave timer */
#define DSKSP_SMART_EXEC_OFFLINE    0x28  /* execute SMART immediate offline */

#define SMART_CMD_ON	  1		  /* on value for related SMART functions */
#define SMART_CMD_OFF	  0		  /* off value for related SMART functions */

#define DSKSP_CAT_GENERIC	    0x90  /* generic IOCTL category */

#define DSKSP_GEN_GET_COUNTERS	    0x40  /* get general counter values table */

#define DSKSP_GET_UNIT_INFORMATION  0x41  /* get unit configuration and BM DMA counters */
#define DSKSP_GET_INQUIRY_DATA	    0x42  /* get ATA/ATAPI inquiry data */
#define DSKSP_GET_NATIVE_MAX	    0x43  /* get native capacity */
#define DSKSP_SET_NATIVE_MAX	    0x44  /* set report capacity */
#define DSKSP_SET_PROTECT_MBR	    0x45  /* set protection of sector 0 */
#define DSKSP_READ_SECTOR	    0x46  /* read one sector */
#define DSKSP_GET_DEVICETABLE	    0x47  /* query ADD device table */
#define DSKSP_POWER		    0x48  /* call suspend/resume function */
#define DSKSP_RESET		    0x49  /* issue channel reset */
//#define DSKSP_TEST_LASTACCESSED     0x47  /* is last accessed unit ? */

#define DSKSP_CAT_TEST_CALLS	    0xF0  /* internal testing calls category */

#define DSKSP_TEST_CONFIGURE_UNIT   0x80  /* configure ATA/ATAPI units */

#define DSKSP_CAT_PCMCIA	    0xF1  /* internal PCMCIA calls category */

#define DSKSP_CAT_TRACER	    0xF2
#define DSKSP_TRACE_CONTROL	    0x80

/*
 * Parameters for SMART and generic commands
 */

typedef struct _DSKSP_CommandParameters
{
  BYTE	      byPhysicalUnit;		  /* physical unit number 0-n */
					  /* 0 = Pri/Mas, 1=Pri/Sla, 2=Sec/Mas, etc. */
} DSKSP_CommandParameters, NEAR *NPDSKSP_CommandParameters, FAR *PDSKSP_CommandParameters;

/*
 * SMART Attribute table item
 */

typedef struct _S_Attribute
{
  BYTE	      byAttribID;		  /* attribute ID number */
  WORD	      wFlags;			  /* flags */
  BYTE	      byValue;			  /* attribute value */
  BYTE	      byVendorSpecific[8];	  /* vendor specific data */
} S_Attribute;

/*
 * SMART Attribute table structure
 */

typedef struct _DeviceAttributesData
{
  WORD	      wRevisionNumber;		  /* revision number of attribute table */
  S_Attribute Attribute[30];		  /* attribute table */
  BYTE	      byReserved[6];		  /* reserved bytes */
  WORD	      wSMART_Capability;	  /* capabilities word */
  BYTE	      byReserved2[16];		  /* reserved bytes */
  BYTE	      byVendorSpecific[125];	  /* vendor specific data */
  BYTE	      byCheckSum;		  /* checksum of data in this structure */
} DeviceAttributesData, NEAR *NPDeviceAttributesData, FAR *PDeviceAttributesData;

/*
 * SMART Device Threshold table item
 */

typedef struct _S_Threshold
{
  BYTE	      byAttributeID;		  /* attribute ID number */
  BYTE	      byValue;			  /* threshold value */
  BYTE	      byReserved[10];		  /* reserved bytes */
} S_Threshold;

/*
 * SMART Device Threshold table
 */

typedef struct _DeviceThresholdsData
{
  WORD	      wRevisionNumber;		  /* table revision number */
  S_Threshold Threshold[30];		  /* threshold table */
  BYTE	      byReserved[18];		  /* reserved bytes */
  BYTE	      VendorSpecific[131];	  /* vendor specific data */
  BYTE	      byCheckSum;		  /* checksum of data in this structure */
} DeviceThresholdsData, NEAR *NPDeviceThresholdsData, FAR *PDeviceThresholdsData;

/*
 * Device Media Access Counters
 */

typedef struct _DeviceCountersData
{
  WORD	      wRevisionNumber;		  /* counter structure revision */
  ULONG       TotalReadOperations;	  /* total read operations performed */
  ULONG       TotalWriteOperations;	  /* total write operations performed */
  ULONG       TotalWriteErrors; 	  /* total write errors encountered */
  ULONG       TotalReadErrors;		  /* total read errors encountered */
  ULONG       TotalSeekErrors;		  /* total seek errors encountered */
  ULONG       TotalSectorsRead; 	  /* total number of sectors read */
  ULONG       TotalSectorsWritten;	  /* total number of sectors written */

  ULONG       TotalBMReadOperations;	  /* total bus master DMA read operations */
  ULONG       TotalBMWriteOperations;	  /* total bus master DMA write operations */
  ULONG       ByteMisalignedBuffers;	  /* total buffers on odd byte boundary */
  ULONG       TransfersAcross64K;	  /* total buffers crossing a 64K page boundary */
  USHORT      TotalBMStatus;		  /* total bad busmaster status */
  USHORT      TotalBMErrors;		  /* total bad busmaster error */
  ULONG       TotalIRQsLost;		  /* total lost interrupts */
  USHORT      TotalDRQsLost;		  /* total lost data transfer requests */
  USHORT      TotalBusyErrors;		  /* total device busy timeouts        */
  USHORT      TotalBMStatus2;		  /* total bad busmaster status */
  USHORT      TotalChipStatus;		  /* total bad chip status */
  USHORT      ReadErrors[4];
  USHORT      WriteErrors[2];
  USHORT      SeekErrors[2];
  USHORT      SATAErrors;
} DeviceCountersData, NEAR *NPDeviceCountersData, FAR *PDeviceCountersData;

/*
 * Unit Configuration and Counters
 */

typedef struct _UnitInformationData
{
  WORD	      wRevisionNumber;		  /* structure revision number */
  union {
    struct {
      WORD    wTFBase;			  /* task file register base addr */
      WORD    wDevCtl;			  /* device control register addr */
    } rev0;
    ULONG     dTFBase;			  /* task file register base addr */
  };
  WORD	      wIRQ;			  /* interrupt request level */
  WORD	      wFlags;			  /* flags */
  BYTE	      byPIO_Mode;		  /* PIO transfer mode programmed */
  BYTE	      byDMA_Mode;		  /* DMA transfer mode programmed */
  ULONG       UnitFlags1;
  USHORT      UnitFlags2;
} UnitInformationData, NEAR *NPUnitInformationData, FAR *PUnitInformationData;

/*
 * Unit Information Flags Definitions
 */

#define UIF_VALID	    0x8000	  /* unit information valid */
#define UIF_TIMINGS_VALID   0x4000	  /* timing information valid */
#define UIF_RUNNING_BMDMA   0x2000	  /* running Bus Master DMA on unit */
#define UIF_RUNNING_DMA     0x1000	  /* running slave DMA on unit */
#define UIF_SATA	    0x0004	  /* SATA	      */
#define UIF_SLAVE	    0x0002	  /* slave on channel */
#define UIF_ATAPI	    0x0001	  /* ATAPI device if 1, ATA otherwise */

/*
 * Configure ATA/ATAPI operating parameters (i.e., Bus Master DMA on/off,
 * PIO transfer mode, DMA transfer mode, etc.)
 */

typedef struct _ConfigureUnitParameters
{
  BYTE	      byPhysicalUnit;		  /* physical unit number 0-n */
					  /* 0 = Pri/Mas, 1=Pri/Sla, 2=Sec/Mas, etc. */
  struct
  {
    BYTE	CommandBits;		  /* indicates which supplied values to be set */
					  /* according to CommandBits mask value. Values */
					  /* in comments on subsequent values indicate */
					  /* or mask for CommandBits to enable that */						/* value to be set */
    BYTE	OnOff;			  /* 0x0001, 0 = Bus Master disable, 1 = Bus Master enable */
    BYTE	PIO_Mode;		  /* 0x0002, PIO mode to set hardware to */
    BYTE	DMA_Mode;		  /* 0x0004, DMA mode to set hardware to */
  } BusMasterDMA;

} ConfigureUnitParameters, NEAR *NPConfigureUnitParameters, FAR *PConfigureUnitParameters;

/*
 * CommandBits values
 */

#define CUPCB_BUSMASTERENABLE	0x0001	  /* enable/disable bus master DMA */
#define CUPCB_SETPIOMODE	0x0002	  /* PIO_Mode contains value */
#define CUPCB_SETDMAMODE	0x0004	  /* DMA_Mode contains value */

typedef struct _getNativeMax {
  ULONG numSectors;
} GetNativeMax, NEAR *NPGetNativeMax, FAR *PGetNativeMax;

typedef struct _setNativeMax {
  BYTE	mode;				  /* 0: LBA volatile, 1: LBA, 2: CHS */
  union {
    ULONG numLBASectors;
    struct {
      BYTE   numSectors;
      USHORT numCylinders;
      BYTE   numHeads;
    } CHS;
  } value;
} SetNativeMax, NEAR *NPSetNativeMax, FAR *PSetNativeMax;

typedef struct _setProtMBR {
  BOOL	enable;
} SetProtectMBR, NEAR *NPSetProtectMBR, FAR *PSetProtectMBR;

typedef struct _ReadWriteSectorParameters {
  BYTE	      byPhysicalUnit;		  /* physical unit number 0-n */
					  /* 0 = Pri/Mas, 1=Pri/Sla, 2=Sec/Mas, etc. */
  ULONG       RBA;
} ReadWriteSectorParameters, NEAR *NPReadWriteSectorParameters, FAR *PReadWriteSectorParameters;
