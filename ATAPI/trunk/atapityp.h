/**************************************************************************
 *
 * SOURCE FILE NAME = ATAPITYP.H
 *
 * DESCRIPTION : Locally defined structures for this driver.
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2007
 *
 ****************************************************************************/

typedef VOID FAR _loadds _fastcall IRQEntry (UCHAR IRQLevel);
typedef IRQEntry FAR *PIRQEntry;

typedef struct _A  NEAR *NPA, FAR *PA;
typedef struct _U  NEAR *NPU;

typedef VOID NEAR OSMEntry (NPA npA);
typedef OSMEntry NEAR *NPOSMEntry;

typedef struct _Sense_Data
{
  USHORT error_code	: 7;
  USHORT valid		: 1;
  USHORT segment_number : 8;

  UCHAR  sense_key	: 4;
  UCHAR  reserved_1	: 1;
  UCHAR  ILI		: 1;
  UCHAR  EOM		: 1;
  UCHAR  filemark	: 1;

  ULONG  information;
  UCHAR  additional_sense_length;
  UCHAR  command_specific_info[4];
  UCHAR  additional_sense_code;
  UCHAR  additional_sense_code_qualifier;
  UCHAR  FRU_Code;
  UCHAR  sensekey_specific[3];
} SENSE_DATA, NEAR *NPSENSE_DATA, FAR *PSENSE_DATA;

typedef struct _GENCONFIGWORD
{
/*			 Bit Field(s)  ÄÄÄ¿   ÚÄ Description		     */
/*					ÚÄÁÄ¿ ³ 			     */
/*					³   ³ ³ 			     */

   unsigned int CmdPktSize   : 2;    /*  0- 1 Command Packet Size	     */
   unsigned int 	     : 3;    /*  2- 4 Reserved			     */
   unsigned int CmdDRQType   : 2;    /*  5- 6 CMD DRQ Type		     */
   unsigned int Removable    : 1;    /*   7   Removable 		     */
   unsigned int DeviceType   : 5;    /*  8-12 Device Type		     */
   unsigned int 	     : 1;    /*  13   Reserved			     */
   unsigned int ProtocolType : 2;    /* 14-15 Protocol Type		     */

}GENCONFIGWORD;

/* defines for Identify Drive Data - Capabilities Word */
#define IDD_VendorUniqueMask  0x00FF
#define IDD_DMASupported      0x0100
#define IDD_LBASupported      0x0200
#define IDD_IORDYDisablable   0x0400
#define IDD_IORDYSupported    0x0800

typedef struct _IDENTIFYDATA IDENTIFYDATA, NEAR *NPIDENTIFYDATA,
					   FAR *PIDENTIFYDATA;

typedef struct _IDENTIFYDATA
{

/*		      word number in structure	Ä¿ ÚÄ Description
						 ³ ³			     */

   GENCONFIGWORD     GenConfig; 	     /*  0 General configuration     */
   USHORT	     Reserved1[3];	     /*  1 Not Used		     */
   USHORT	     MediaTypeCode;	     /*  4 Medium Type Code LS-120   */
   USHORT	     BytesPerSector;	     /*  5 BytesPerSector	     */
   USHORT	     Reserved7[4];	     /*  1 Not Used		     */
   CHAR 	     Serial[20];	     /* 10 Serial Number	     */
   USHORT	     BufferType;	     /* 20 Buffer Type		     */
   USHORT	     BufferSize;	     /* 21 Buffer Size		     */
   USHORT	     Reserved2; 	     /* 22 Not Used		     */
   CHAR 	     Firmware[8];	     /* 23 Firmware Revision	     */
   CHAR 	     ModelNum[40];	     /* 27 Model Number 	     */
   USHORT	     Reserved3[2];	     /* 47 Not Used		     */
   USHORT	     Capabilities;	     /* 49 Capabilities 	     */
   USHORT	     Reserved4; 	     /* 50 Not Used		     */
   USHORT	     PIOCycleTiming;	     /* 51 PIO Cycle Timing	     */
   USHORT	     DMACycleTiming;	     /* 52 DMA Cycle Timing	     */
   USHORT	     ValidityofOtherWords;   /* 53 Validity of words 54-58   */
					     /* 		 and 64-70   */
   USHORT	     CurrentCylHdSec[3];     /* 54 Current		     */
					     /*    Cylinder/Heads/Sectors    */
   USHORT	     CurrentCapacity[2];     /* 57 Current Capacity	     */
   USHORT	     Reserved5[3];	     /* 59 Not Used		     */
   USHORT	     SingleDMAmode;	     /* 62 Single DMA Mode	     */
   USHORT	     MultiwordDMAmode;	     /* 63 Multiword DMA Mode	     */
   USHORT	     EnhancedPIOmode;	     /* 64 Enhanced PIO Mode	     */
   USHORT	     BlindPIOminCycTime;     /* 65 Blind PIO Minimum Cycle   */
					     /*    Time 		     */
   USHORT	RecMultiWordDMAxferCycleTime;/* 66 Recommended Multi Word    */
					     /*    DMA Transfer Cycle Time   */
   USHORT      MinPIOxferCyclewithoutFlowCon;/* 67 Minimum PIO Transfer      */
					     /*    Cycle Time without Flow   */
					     /*    Control		     */
   USHORT    MinPIOxferCyclewithIORDYFlowCon;/* 68 Minimum PIO Transfer      */
					     /*    Cycle Time with IORDY     */
					     /*    Flow Control 	     */
   USHORT	     Reserved6[126-69];      /* 69			     */
   USHORT	     LastLUN;		     /* 126 Last LUN		     */
   USHORT	     MediaStatusWord;	     /* 127 Media Status Support Flag*//*V189445*/
   USHORT	     Reserved8[256-128];     /* 128 Not Used		     */
} IDENTIFYDATA;

/*----------------------------------*/
/* Physical Region Descriptor	    */
/* --------------------------	    */
/*				    */
/* One per physical memory region.  */
/*				    */
/*----------------------------------*/

typedef struct _PRD
{
  ULONG     MR_PhysicalBaseAddress;	      /* memory region's physical base address */
  ULONG     ByteCountAndEOT;		      /* byte count and End of Table flag */
} PRD;

#define PRD_EOT_FLAG	0x80000000L	      /* End of table indicator */

typedef struct _PRD  NEAR *NPPRD, FAR *PPRD;  /* typedef for pointers to PRD */

/*-------------------------------*/
/* Unit Control Block		 */
/* ------------------		 */
/*				 */
/* One per drive on controller.  */
/*				 */
/*-------------------------------*/

typedef struct _U
{
  /*-----------------------------------*/
  /* Associated Control Block Pointers */
  /*-----------------------------------*/
  NPA		   npA;
  USHORT	   Flags;
  UCHAR 	   Capabilities;
  UCHAR 	   ReqFlags;
  UCHAR 	   CmdPacketLength;

  /*--------------------*/
  /* Unit Number (0/1)	*/
  /*--------------------*/
  UCHAR 	   UnitId;
  union {
    UCHAR	   DriveHead;
    struct {
      USHORT	   LUN:3;
      USHORT	   reserved1:1;
      USHORT	   Drive:1;
      USHORT	   reserved2:1;
      USHORT	   Rev_17B:1;
    }		   DriveLUN;
  };
  UCHAR 	   CDBLUN;
  UCHAR 	   MaxLUN;
  UCHAR 	   LUN;

  UCHAR 	   UnitType;
  UCHAR 	   DrvDefFlags;
  UCHAR 	   MediaStatus;
  UCHAR 	   MediaType;
  GEOMETRY	   MediaGEO;
  GEOMETRY	   DeviceGEO;

  PUNITINFO	   pUI;
  NPUNITINFO	   savedUI;
  USHORT	   hUnit;	  // shared unit handle

  UCHAR 	   Status;

  USHORT	   Types;
  USHORT	   TimeOut;

  NPCH		   ModelNum;

  NPU		   npUnext;
} UCB;

/* Flags definitions */
#define UCBF_ALLOCATED	    0x0001
#define UCBF_READY	    0x0002
#define UCBF_XLATECDB6	    0x0004
#define UCBF_CDWRITER	    0x0008
#define UCBF_LARGE_FLOPPY   0x0010
#define UCBF_EJECT	    0x0020
#define UCBF_SCSI	    0x0040
#define UCBF_RSJ	    0x0080
#define UCBF_NOTRMV	    0x0100
#define UCBF_PIO32	    0x0200
#define UCBF_BM_DMA	    0x0400
/* For device swaping support */
#define UCBF_FORCE	    0x0800
#define UCBF_NOTPRESENT     0x1000
#define UCBF_IGNORE	    0x2000
#define UCBF_PROXY	    0x4000
#define UCBF_DYNAPROXY	    0x8000


/* UCB->ReqFlags definitions */
#define UCBR_IDENTIFY	    0x1
#define UCBR_RESET	    0x2
#define UCBR_PASSTHRU	    0x4

/* Unit Capabilities */
#define UCBC_DIRECT_ACCESS_DEVICE  0x01
#define UCBC_CDROM_DEVICE	   0x02
#define UCBC_TAPE_DEVICE	   0x04
#define UCBC_DRIVE_LUN		   0x08
#define UCBC_CDB_LUN		   0x10
#define UCBC_REMOVABLE		   0x80
#define UCBC_INTERRUPT_DRQ	   0x40
#define UCBC_SPEC_REV_17B	   0x20

/*------------------------*/
/* Drive Definition Flags */		       /* UCB->DrvDefFlags */
/*------------------------*/
#define  DDF_TESTUNITRDY_REQ 0x01 /* Bit field if TUR must be xcuted prior to others */
#define  DDF_VALIDITY_REQ    0x02 /* Bit field to indicate if media must be	*/
				  /* Validated for this drive ie: 1.44 in LS120 */
#define  DDF_1ST_TUR_4_GMG   0x04 /* indicate first time for TUR in GetMediaGEO */
#define  DDF_LS120	     0x08 /* special treatment of LS-120 */

/*-------------------------*/
/* Media Type Codes	   */			/* UCB->MediaType */
/*-------------------------*/
#define  UHD_MEDIUM	    0x0030	/* LS-120   Media 120 MB  Value */
#define   HD_MEDIUM	    0x0020	/*		  1.44 MB Value */
#define   DD_MEDIUM	    0x0010	/*		  720 KB  Value */

/*---------------------*/
/* Media Status Flags  */
/*---------------------*/
#define  MF_RD_PT	    0x80     /* Read Protected	  */
#define  MF_WRT_PT	    0x40     /* Write Proteted	  */
#define  MF_MC		    0x20     /* Medium Changed	  */
#define  MF_RSVD	    0x10     /*   RESERVED	  */
#define  MF_MCR 	    0x08     /* Eject Pressed - Medium Change Request */
#define  MF_RSVD1	    0x04     /*   RESERVED	  */
#define  MF_NOMED	    0x02     /* No Medium Present */
#define  MF_NOTIFY_NOMED    0x01     /*   RESERVED	  */


#define UTS_OK			0
#define UTS_NOT_PRESENT 	1
#define UTS_SKIPPED		2


typedef struct _CMDIO
{
  ADD_XFER_IO_X IOSGPtrs;
  ULONG 	cXferBytesRemain;
  ULONG 	cXferBytesComplete;
  UCHAR 	ATAPIPkt[MAX_ATAPI_PKT_SIZE];
} CMDIO, NEAR *NPCMDIO;


typedef enum {
  undetermined = 0,
  Cmd640, RZ1000, Intel, Via, ALi, SiS, CMD64x, Promise, Cyrix,
  HPT36x, HPT37x, AEC, ServerWorks,
  generic = -1
} tHardwareType;


/*-------------------------------*/
/* Adapter Control Block	 */
/* ---------------------	 */
/*				 */
/* One per controller.		 */
/*				 */
/*-------------------------------*/


#define FI_MAX_REGS	12

typedef struct _A
{
  USHORT	BasePort;
  USHORT	StatusPort;

  UCHAR 	FlagsT;

  /*--------------------------------------*/
  /* IORB Queue for units on this adapter */
  /*--------------------------------------*/
  PIORBH	pHeadIORB;
  PIORBH	pFootIORB;
  PIORBH	pIORB;

  /*---------------------------------------------------*/
  /* Controller Register addresses/contents, IRQ level */
  /*---------------------------------------------------*/
  USHORT	IOPorts[8];
  UCHAR 	IORegs[FI_MAX_REGS];
  USHORT	IOPendingMask;

  PIRQEntry	IRQHandler;

  /*-----------------------*/
  /* Inner State Variables */
  /*-----------------------*/
  UCHAR 	ISMUseCount;
  UCHAR 	ISMState;
  UCHAR 	ISMFlags;

  NPOSMEntry	ISMDoneReturn;

  /*-----------------------*/
  /* Outer State Variables */
  /*-----------------------*/
  UCHAR 	OSMUseCount;
  UCHAR 	OSMState;
  USHORT	OSMFlags;
  UCHAR 	OSMReqFlags;

  /*--------------------------------*/
  /* Current operation variables    */
  /*--------------------------------*/
  USHORT	IORBStatus;
  USHORT	IORBError;
  SEL		MapSelector;
  NPCMDIO	npCmdIO;
  CMDIO 	ExternalCmd;
  CMDIO 	InternalCmd;

  /*--------------------------------*/
  /* Timer Flags/Values 	    */
  /*--------------------------------*/

  UCHAR 	TimerFlags;

  /*----------------------*/
  /* Timer Loop Counters  */
  /*----------------------*/
  ULONG 	DelayedResetInterval;
  ULONG 	DelayedResetCtr;
  ULONG 	DelayedRetryInterval;
  ULONG 	IRQTimeOut;
  ULONG 	IRQTimeOutMin;
  ULONG 	IRQTimeOutDef;

  /*--------------------------------------------------*/
  /* Timer Handles				      */
  /*						      */
  /* (Also update TIMERS_PER_ACB when adding timers)  */
  /*--------------------------------------------------*/
  USHORT	IRQTimeOutHandle;

  /*-------------------------------------------------*/
  /* Unit Control Blocks - Unit specific information */
  /*-------------------------------------------------*/
  UCHAR 	cUnits;
  UCHAR 	UnitId;
  NPU		npU;
  NPU		npUactive[MAX_UNITS];
  UCHAR 	cResetRequests;

  IORB_DEVICE_CONTROL IORB;

  PADDEntry	SharedEP;
  UCHAR 	SharedhADD;
  USHORT	SharedhUnit;
  UCHAR 	ResourceFlags;
  UCHAR 	suspended;
  UCHAR 	Flags;			    /* Initialized state flags */

  ULONG 	ppSGL;			    /* Bus Master DMA SG list */
  NPPRD 	pSGL;			    /* ptr to BM S/G list  */
  UCHAR 	BMStatus;
  UCHAR 	BM_CommandCode; 	    /* command for BMDMA controller */
  UCHAR 	BM_StopMask;
  USHORT	BMICOM; 		    /* IDE Command Register */
  USHORT	BMISTA; 		    /* IDE Status Register */
  USHORT	BMIDTP; 		    /* IDE Descriptor Table Pointer */

  UCHAR 	HardwareType;
  UCHAR 	Index;
  UCHAR 	SCSIOffset;
  UCHAR 	SGAlign;

  UCB		UnitCB[MAX_UNITS];
  UCHAR 	ModelNum[2][40+8+1];
} ACB;

#define ACBF_BM_DMA		     0x0001

/* ACB->ISMFlags values */

#define ACBIF_WAITSTATE 	     0x02
#define ACBIF_ATA_OPERATION	     0x04
#define ACBIF_BUSMASTERDMAIO	     0x08
#define ACBIF_BUSMASTERDMA_FORCEPIO  0x10
#define ACBIF_PIO32		     0x20

/* ACB->OSMFlags values */

#define ACBOF_WAITSTATE 	     0x01
#define ACBOF_SM_ACTIVE 	     0x02
#define ACBOF_RESET_ACTIVE	     0x04
#define ACBOF_SENSE_DATA_ACTIVE      0x08
#define ACBOF_IDENTIFY_ACTIVE	     0x10
#define ACBOF_READCAPACITY_ACTIVE    0x20
#define ACBOF_MODESENSE_ACTIVE	     0x40
#define ACBOF_INQUIRY_ACTIVE	     0x80

/* ACB->ReqFlags values */

#define ACBR_RESET		     0x01
#define ACBR_SENSE_DATA 	     0x02
#define ACBR_TEST_UNIT_RDY	     0x04
#define ACBR_DISCARD_ERROR	     0x08

/* ACB->OSMState values */

#define ACBOS_START_IORB	 1
#define ACBOS_ISM_COMPLETE	 2
#define ACBOS_COMPLETE_IORB	 3
#define ACBOS_RESUME_COMPLETE	 4
#define ACBOS_SUSPEND_COMPLETE	 5

/* ACB->ISMState values */

#define ACBIS_START_STATE	       1
#define ACBIS_INTERRUPT_STATE	       2
#define ACBIS_COMPLETE_STATE	       3
#define ACBIS_WRITE_ATAPI_PACKET_STATE 4

/* ACB->TimerFlags */
#define ACBT_BUSY	0x01
#define ACBT_DRQ	0x02
#define ACBT_IRQ	0x04
#define ACBT_READY	0x08
#define ACBT_INTERRUPT	0x10

/* ACB->ResourceFlags */
#define ACBRF_SHARED	 0x01
#define ACBRF_CURR_OWNER 0x02

/* Bus Master DMA Variable and Register definitions */

#define BMICOM_TO_HOST	   0x08    /* PCI Bus read when 0, write when 1 */
#define BMICOM_START	   0x01    /* Start when 1, stop when 0 */

#define BMISTA_SIMPLEX	   0x80    /* simplex DMA channels when 1 */
#define BMISTA_D1DMA	   0x40    /* drive 1 DMA capable */
#define BMISTA_D0DMA	   0x20    /* drive 0 DMA capable */
#define BMISTA_INTERRUPT   0x04    /* IDE interrupt was detected */
#define BMISTA_ERROR	   0x02    /* transfer error detected */
#define BMISTA_ACTIVE	   0x01    /* Bus Master DMA active */

#define ATBF_DISABLED		  0x0001
#define ATBF_IGNORE		  0x0002
#define ATBF_SATA		  0x0004

/*-------------------------------*/
/* ACB Pointer Block		 */
/* -----------------		 */
/*				 */
/*				 */
/*				 */
/*-------------------------------*/

typedef struct _APTRS
{
  NPA	    npA;
  PIRQEntry IRQHandler;
} ACBPTRS;

