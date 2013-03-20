/**************************************************************************
 *
 * SOURCE FILE NAME =  S506TYPE.H
 * $Id$
 *
 * DESCRIPTIVE NAME =  DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2009
 * Portions Copyright (c) 2010, 2011 Steven H. Levine
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : Locally defined structures for this driver.
 *
 ****************************************************************************/

#include "rmcalls.h"
#include "s506oem.h"
#include "s506regs.h"
#include "ata.h"
#include "pcmcia.h"

typedef struct _APTRS  NEAR *NPAPTR;
typedef struct _IHDRS  NEAR *NPIHDRS;
typedef struct _A      NEAR *NPA, FAR *PACB;
typedef struct _U      NEAR *NPU;
typedef struct _C      NEAR *NPC;
typedef struct _BAR    NEAR *NPBAR;

typedef struct _GEO {
  UCHAR  NumHeads;
  UCHAR  SectorsPerTrack;
  USHORT TotalCylinders;
  ULONG  TotalSectors;
} GEO, NEAR *NPGEO, FAR *PGEO;

typedef struct _GEO1 {
  UCHAR  NumHeads;
  UCHAR  SectorsPerTrack;
  USHORT TotalCylinders;
} GEO1, NEAR *NPGEO1;

typedef struct _GEO2 {
  UCHAR  NumHeads;
  UCHAR  SectorsPerTrack;
} GEO2, NEAR *NPGEO2;

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

typedef struct _BAR {
  ULONG  Addr;
  ULONG  phyA;
  USHORT Size;
} tBAR;

/*-------------------------------*/
/* Chip Control Block		 */
/* ------------------		 */
/*				 */
/* One per PCI function 	 */
/*				 */
/*-------------------------------*/

typedef struct _C
{
  NPA		npA[MAX_CHANNELS];		// 4
  USHORT	Cap;
  tBAR		BAR[6];
  UCHAR 	IrqPIC;
  UCHAR 	IrqAPIC;
  USHORT	HWSpecial;
  UCHAR 	numChannels;
  UCHAR 	populatedChannels;

  UCHAR 	CfgTblBIOS[0x40];
  UCHAR 	CfgTblDriver[0x40];
} CCB;

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
  NPA		npA;						       // 55
  USHORT	FlagsT; 					       // 80
  NPA		npAF;						       // 55
  ULONG 	Flags;						       //100
  ULONG 	FlagsF; 					       //100
  USHORT	ReqFlags;					       // 30

  /*--------------------*/
  /* Unit Number (0/1)	*/
  /*--------------------*/
  UCHAR 	UnitId; 					       // 70
  UCHAR 	Status; 					       // 22
  UCHAR 	UltraDMAMode;			/* Ultra DMA Mode   */ // 70
  CHAR		LongTimeout;					       // 11

  ULONG 	SStatus;		    /* SATA Status register address	*/
  ULONG 	SError; 		    /* SATA Error register address	*/
  ULONG 	SControl;		    /* SATA Control register address	*/

  /*------------------------------------------*/
  /* Physical/Logical Geometry for this drive */
  /*------------------------------------------*/
  GEO		LogGeom;					       // 20
  GEO		PhysGeom;					       // 15
  UCHAR 	SecPerBlk;					       //  9

  /*------------------------*/
  /* DMA/PIO Timing Values  */
  /*------------------------*/
  UCHAR 	CurDMAMode;			/* Current DMA mode */ // 35
  UCHAR 	CurPIOMode;			/* Current PIO mode */ // 30
  UCHAR 	InterfaceTiming;		/* interface timing */ // 24

  DeviceCountersData DeviceCounters;				       // 35

  /*------------------------*/
  /* INT 13 FCN=08 Geometry */
  /*------------------------*/
  GEO1		I13LogGeom;					       // 18

  /*------------------------*/
  /* BPB Geometry	    */
  /*------------------------*/
  GEO2		BPBLogGeom;					       // 14

  /*-------------------*/
  /* Identify Geometry */
  /*-------------------*/
  GEO		IDEPhysGeom;					       // 15
  GEO		IDELogGeom;					       // 11

  ULONG 	CmdSupported;		// Supported command features  // 12
  ULONG 	CmdEnabled;					       //  8
  USHORT	SATACmdSupported;	// Supported SATA features
  USHORT	SATACmdEnabled;		// Enabled SATA features

  /*------------------------------------------*/
  /* Replacement UNITINFO from Filter ADD     */
  /*------------------------------------------*/
  PUNITINFO	pUnitInfo;					       //  8
  PUNITINFO	pUnitInfoF;					       //  8

  UCHAR 	FoundUDMAMode;					       //  9
  UCHAR 	IdleTime;					       //  9

  UCHAR 	DriveHead;					       //  8
  UCHAR 	BestPIOMode;					       //  8
  UCHAR 	FoundDMAMode;					       //  8
  UCHAR 	AMLevel;					       //  9
  UCHAR 	FoundAMLevel;					       //  4
  ULONG 	FoundLBASize;					       //  4
  UCHAR 	FoundPIOMode;					       //  4
  UCHAR 	APMLevel;					       //  9
  UCHAR 	LPMLevel;					       //  9
  UCHAR 	FoundLPMLevel;					       //  9

  USHORT	Features;					       //  5
  USHORT	IdleCounter, IdleCountInit;			       // 3,4
  CHAR		FlushCount;					       //  5
  UCHAR 	TimeOut;					       //  2
  UCHAR 	SMSCount;					       //  5
  USHORT	MaxRate;					       //  4

  UCHAR 	ATAVersion;

  UCHAR 	MediaStatus;			/* cur media status */ //  4
  UCHAR 	UnitIndex;					       //  4
  UCHAR 	DriveId;					       //  3
  UCHAR 	WriteProtect;

  UCHAR 	FoundSecPerBlk; 				       //  3
  ULONG 	MaxTotalSectors;				       //  4
  USHORT	MaxTotalCylinders;				       //  4
  USHORT	MaxXSectors;					       //  2
  USHORT	SATAGeneration; 				       //  2
  /*------------------------*/
  /* Model/Firmware Rev     */
  /*------------------------*/
  NPCH		ModelNum;					       //  8
} UCB;

/* Flags definitions */   /* Changed all Flag defs to ULONGs. */
#define UCBF_ATAPIDEVICE    0x00000001
#define UCBF_REMOVABLE	    0x00000002
#define UCBF_MULTIPLEMODE   0x00000004	/* needs to be bit 2 for CmdIdx */
#define UCBF_NOTPRESENT     0x00000008
#define UCBF_READY	    0x00000010
#define UCBF_SMSENABLED     0x00000020
#define UCBF_LBAMODE	    0x00000040
#define UCBF_DISABLERESET   0x00000080
#define UCBF_BM_DMA	    0x00000100
#define UCBF_FORCE	    0x00000200
#define UCBF_BECOMING_READY 0x00000400

#define UCBF_CFA	    0x00001000	/* CompactFlash */
#define UCBF_HPROT	    0x00002000	/* Host protected area encountered */
#define UCBF_DISABLEIORDY   0x00004000	/* supports disable IORDY */
#define UCBF_PCMCIA	    0x00008000	/* PCCard */

#define UCBF_DIAG_FAILED    0x00010000
#define UCBF_LOCKED	    0x00020000
#define UCBF_WCACHEENABLED  0x00040000
#define UCBF_MEDIASTATUS    0x00080000
#define UCBF_IGNORE_BM_ACTV 0x00100000	/* ignore BM active on ALi */
#define UCBF_PIO32	    0x00200000	/* 32 bit PIO capable */

/* For docking support */
#define UCBF_PHYREADY	    0x00400000
#define UCBF_FAKE	    0x00800000	/* fake unit handle for export to ATAPI */
#define UCBF_CHANGELINE     0x01000000

#define UCBF___________     0x02000000	/* no writes to MBR */
#define UCBF_ONTRACK	    0x10000000
#define UCBF_EZDRIVEFBP     0x20000000
#define UCBF_NODASDSUPPORT  0x40000000
#define UCBF_ALLOCATED	    0x80000000

#define UTBF_I13GEOMETRYVALID	0x4000
#define UTBF_IDEGEOMETRYVALID	0x2000
#define UTBF_BPBGEOMETRYVALID	0x1000
#define UTBF_SET_BM		0x0800
#define UTBF_NOT_BM		0x0400
#define UTBF_NOTUNLOCKHPA	0x0080
#define UTBF_DISABLED		0x0001

#define UTS_OK			0
#define UTS_NOT_PRESENT 	1
#define UTS_READ_0_FAILED	2
#define UTS_NOT_READY		3
#define UTS_NO_MASTER		4
#define UTS_NOT_INITIALIZED	5

/* ReqFlags definitions */
#define UCBR_SETPARAM	    0x8000
#define UCBR_SETIDLETIM     0x4000
#define UCBR_SETPIOMODE     0x2000
#define UCBR_SETDMAMODE     0x1000
#define UCBR_SETMULTIPLE    0x0800
#define UCBR_READMAX	    0x0400
#define UCBR_SETMAXNATIVE   0x0200
#define UCBR_ENABLEWCACHE   0x0100
#define UCBR_DISABLEWCACHE  0x0080
#define UCBR_ENABLERAHEAD   0x0040
#define UCBR_FREEZELOCK     0x0020
#define UCBR_ENABLESSP	    0x0010
#define UCBR_ENABLEDIPM     0x0008
#define UCBR_GETMEDIASTAT   0x0004
//	ACBR NONDATA	    0x0001

#define UCBS_SMART	    0x00000001	/* S.M.A.R.T. active  */
#define UCBS_SECURITY	    0x00000002	/* Security supported */
#define UCBS_RAHEAD	    0x00000040	/* Read look ahead supported */
#define UCBS_WCACHE	    0x00000020	/* Write caching supported */
#define UCBS_POWER	    0x00000008	/* power mgmt active  */
#define UCBS_ACOUSTIC	    0x02000000	/* acoustic mgmt active  */
#define UCBS_HPROT	    0x00000400	/* Host protected area encountered */
#define UCBS_APM	    0x00080000	/* Advanced Power Management suppported */

typedef enum { CtNone = 0, CtATA = 1, CtATAPI = 2} eControllerType;

/*-------------------------------*/
/* Adapter Control Block	 */
/* ---------------------	 */
/*				 */
/* One per controller.		 */
/*				 */
/*-------------------------------*/


#define METHOD(x) (x)->PCIInfo.Ident
#define MEMBER(x) (x)->PCIInfo.Ident

#define FI_MAX_REGS	14

typedef struct _A
{
  NPA		npIntNext;
  UCHAR 	atInterrupt;
  UCHAR 	State;						       // 30
  USHORT	(NEAR *IntHandler)(NPA npA);

  UCHAR 	Socket; 					       // 13

  UCHAR 	IRQLevel;					       // 18
  ULONG 	CtrlPort;					       // 20
  ULONG 	IOPorts[8];					       //  *
  ULONG 	BMICOM; 		    /* IDE Command Register */ // 30
  ULONG 	BMISTA; 		    /* IDE Status Register  */ // 30
  ULONG 	BMIDTP; 		    /* IDE BM DMA pointer   */ //
  ULONG 	ISRPort;

  USHORT	FlagsT; 					       // 100
  UCHAR 	Status; 					       // 35

  /*--------------------------------------*/
  /* IORB Queue for units on this adapter */
  /*--------------------------------------*/
  PIORBH	pHeadIORB;					       //  9
  PIORBH	pFootIORB;					       //  4
  PIORBH	pIORB;						       // 21

  /*---------------------------------------------------*/
  /* Controller Register addresses/contents, IRQ level */
  /*---------------------------------------------------*/
  UCHAR 	IORegs[FI_MAX_REGS];				       //  *
  USHORT	IOPendingMask;					       // 17

  UCHAR 	IDEChannel;					       // 70
  UCHAR 	HWSpecial;					       // 11
  UCHAR 	HardwareType;					       // 25
  USHORT	Cap;						       // 37

  USHORT	IRQTimeOut;					       // 24

  /*-----------------*/
  /* State Variables */
  /*-----------------*/
  volatile UCHAR FsmUseCount;					       //  5
  UCHAR 	cUnits; 					       // 11
  USHORT	Flags;						       // 65
  USHORT	ReqMask;					       // 21

  NPU		npU;						       // 35

  /*--------------------------------*/
  /* Current operation variables    */
  /*--------------------------------*/
  USHORT	IORBStatus;					       // 14
  USHORT	IORBError;					       // 12

  ULONG 	PIIX_IDEtim;					       //  *
  ULONG 	PIIX_SIDEtim;					       //  *
  UCHAR 	BM_CommandCode; 				       // 30
  UCHAR 	BMSta;						       // 30

  ADD_XFER_IO_X IOSGPtrs;					       // 29

  /* vvvvv  cleared before start of operation  vvvv */
	ULONG	ReqFlags;					       // 60

	ULONG	SecRemain;					       //  7

	USHORT	SecReq; 					       //  5
	USHORT	SecToGo;					       // 18
	USHORT	SecPerInt;					       // 11

	UCHAR	MediaStatusMask; /* media status bits in error register */
	UCHAR	TimerFlags;					       // 21
	UCHAR	DataErrorCnt;					       //  6
	UCHAR	BusyTimeoutCnt; 				       //  2
	UCHAR	cResets;					       //  2
  /* ^^^^^  cleared before start of operation  ^^^^ */

  /*--------------------------------*/
  /* Timer Flags/Values 	    */
  /*--------------------------------*/
  USHORT	IRQDelayCount;					       // 10
  USHORT	IODelayCount;					       //  3

  ULONG 	ElapsedTime;					       //  3
  USHORT	TimeOut;					       //  6

  ULONG 	RBA;						       //  3
  ULONG 	BytesToTransfer;				       //  4
  UCHAR 	BM_StopMask;					       //  3
  UCHAR 	BM_StrtMask;					       //  3
  UCHAR 	BMStatus;					       //  3

  /*----------------------*/
  /* Timer Loop Counters  */
  /*----------------------*/
  UCHAR 	DelayedResetInterval;				       //  2
  UCHAR 	DelayedResetCtr;				       //  4
  UCHAR 	DelayedRetryInterval;				       //  3
  USHORT	IRQLongTimeOut; 				       //  4

  /*--------------------------------------------------*/
  /* Timer Handles				      */
  /*						      */
  /* (Also update TIMERS_PER_ACB when adding timers)  */
  /*--------------------------------------------------*/
  USHORT	IRQTimerHandle; 				       // 10
  USHORT	RetryTimerHandle;				       //  3
  USHORT	ResetTimerHandle;				       //  4

  /*-------------------------------------------------*/
  /* Pause Resume Support			     */
  /*-------------------------------------------------*/
  /* Pointer to the first and last suspend IORB for  */
  /* this ACB.	Only suspend requests are placed on  */
  /* this queue in SuspendIORBReq().		     */
  PIORB 	pSuspendHead;					       //  9
  PIORB 	pSuspendFoot;					       //  4
  USHORT	(FAR * _fastcall SuspendIRQaddr)(UCHAR);	       //  5
  /* Depending on the type suspend request, DC_SUSPEND_DEFERRED or     */
  /* DC_SUSPEND_IMMEDIATE, this field holds a count-down of IORBs to   */
  /* process in this driver before completing the suspend operation.   */
  UCHAR 	CountUntilSuspend;				       //  6

  ULONG 	ppSGL;		    /* Bus Master DMA SG list */       //  2
  NPPRD 	pSGL;		    /* ptr to BM S/G list  */	       //  2
  UCHAR 	SGAlign;	    /* alignment test bit mask */      //  2

  ULONG 	DMAtim[2];		    /* DMA Timing */	       //  3
  NPIHDRS	npIHdr; 					       //  3

  PCI_INFO	PCIInfo;					       // 70

  IORB_ADAPTER_PASSTHRU PTIORB; 				       // 40
  PassThruATA	icp;						       // 70

  /*-------------------------------------------------*/
  /* Unit Control Blocks - Unit specific information */
  /*-------------------------------------------------*/
  UCB		UnitCB[MAX_UNITS];				       // 70

  SCATGATENTRY	IdentifySGList; 				       //  3
  IDENTIFYDATA	IdentifyBuf;					       //  2

  USHORT	BMSize; 					       //  2
  USHORT	ExtraPort;					       //  3
  UCHAR 	ExtraSize;					       //  2
  USHORT	IRQTimeOutSet;					       //  3
  CHAR		IRQDelay;					       //  6
  union {
    struct {
      // 2011-07-27 SHL	fixme to match pci spec better
      // the reality is
      // 3 - secondary channel mode programmable, 1 = yes
      // 2 - secondary channel operating mode, 1 = native, 0 = compatbile
      // 1 - primary channel mode programmable 
      // 0 - primary channel operating mode, 1 = native, 0 = compatbile
      // ----
      // actually it is two PCI native mode bits and one bit for programming
      // possibility but we filter out the latter one (see ClassCode.ProgIF
      // init in EnumPCIDevices)
      unsigned native:3;
      unsigned unused:4;
      unsigned busmaster:1;
    } b;
    UCHAR ProgIF;
  } ProgIF;  // was FlagsI
  union {
    USHORT Offsets;
    struct {
      unsigned SStatus :4;
      unsigned SError  :4;
      unsigned SControl:4;
    } Ofs;
  } SCR;

  NPSZ		npPCIDeviceMsg;
  NPC		npC;
  UCHAR 	InsertState;
  UCHAR 	maxUnits;
  USHORT	Retries;
  UCHAR 	Controller[2];
  USHORT	LocatePCI;
  UCHAR 	LocateChannel;
  struct GCI_P	CSConfig;					       // 11
  UCHAR 	PCIDeviceMsg[20];				       //  2
  UCHAR 	ResourceBuf[sizeof (AHRESOURCE) + 5 * sizeof (HRESOURCE)]; // 4
  UCHAR 	ModelNum[2][40+8+1];				       //  2

} ACB;

#define TIMERS_PER_ACB	  3

#define TIMER_POOL_SIZE  (sizeof(ADD_TIMER_POOL) +			  \
			   ((TIMERS_PER_ACB * MAX_ADAPTERS) + 2)    \
			     * sizeof(ADD_TIMER_DATA))

/* ACB->Flags values */

#define ACBF_INTERRUPT		0x8000
#define ACBF_BMINT_SEEN 	0x1000
#define ACBF_MULTIPLEMODE	0x0200
#define ACBF_DISABLERETRY	0x0040

/* ACB->FlagsT values */

#define ATBF_DISABLED		0x8000
#define ATBF_ON 		0x4000
#define ATBF_POPULATED		0x2000
#define ATBF_ATAPIPRESENT	0x1000
#define ATBF_FORCE (ATBF_BAY | ATBF_PCMCIA)
#define ATBF_BAY		0x0200
#define ATBF_PCMCIA		0x0100
#define ATBF_INTSHARED		0x0040
#define ATBF_BIOSDEFAULTS	0x0020
#define ATBF_BM_DMA		0x0004
#define ATBF_SUSPENDED		0x0002
#define ATBF_80WIRE		0x0001

/* ACB->ReqFlags values */

#define ACBR_DMAIO		0x80000000
#define ACBR_BM_DMA_FORCEPIO	0x40000000
#define ACBR_NONPASSTHRU	0x20000000
#define ACBR_PASSTHRU		0x10000000

#define ACBR_READ		0x01000000
#define ACBR_WRITE		0x02000000
#define ACBR_VERIFY		0x04000000
#define ACBR_WRITEV		0x08000000
#define ACBR_RESETCONTROLLER	0x00100000

#define ACBR_NONDATA		0x0001

#define ACBR_IDENTIFY		0x00010000
#define ACBR_LOCK	       (0x00010000 | ACBR_NONDATA)
#define ACBR_UNLOCK	       (0x00020000 | ACBR_NONDATA)
#define ACBR_EJECT	       (0x00030000 | ACBR_NONDATA)
#define ACBR_INTERNALCOMMAND	0x000F0000

// warning, warning, these are predefined as top bits
#define ACBR_SETIDLETIM 	UCBR_SETIDLETIM
#define ACBR_SETPARAM		UCBR_SETPARAM
#define ACBR_SETPIOMODE 	UCBR_SETPIOMODE
#define ACBR_SETDMAMODE 	UCBR_SETDMAMODE
#define ACBR_SETMULTIPLE	UCBR_SETMULTIPLE
#define ACBR_READMAX		UCBR_READMAX
#define ACBR_SETMAXNATIVE	UCBR_SETMAXNATIVE
#define ACBR_ENABLEWCACHE	UCBR_ENABLEWCACHE
#define ACBR_ENABLERAHEAD	UCBR_ENABLERAHEAD
#define ACBR_DISABLEWCACHE	UCBR_DISABLEWCACHE
#define ACBR_FREEZELOCK 	UCBR_FREEZELOCK
#define ACBR_ENABLEDIPM 	UCBR_ENABLEDIPM
#define ACBR_ENABLESSP		UCBR_ENABLESSP
#define ACBR_GETMEDIASTAT	UCBR_GETMEDIASTAT

#define ACBR_BLOCKIO	       (ACBR_READ | ACBR_WRITE | ACBR_VERIFY | ACBR_WRITEV)
#define ACBR_MULTIPLEMODE      (ACBR_READ | ACBR_WRITE)
#define ACBR_NONBLOCKIO        (ACBR_PASSTHRU | ACBR_NONPASSTHRU)

#define ACBR_SETUP	       (ACBR_SETPARAM | \
				ACBR_SETIDLETIM | ACBR_SETPIOMODE | \
				ACBR_SETMULTIPLE | ACBR_SETDMAMODE | \
				ACBR_ENABLEWCACHE | ACBR_DISABLEWCACHE | \
				ACBR_ENABLERAHEAD | ACBR_FREEZELOCK | \
				ACBR_ENABLEDIPM | ACBR_ENABLESSP)

#define ACBR_OTHERIO	       (ACBR_SETUP | \
				ACBR_READMAX | ACBR_SETMAXNATIVE | \
				ACBR_GETMEDIASTAT | \
				ACBR_NONDATA)

/* ACB->State values */

#define ACBS_START	0	/* Dispatch and start the IORB	      */
#define ACBS_RETRY	1	/* Calculate position on disk	      */
#define ACBS_INTERRUPT	2	/* Start verify portion of write      */
#define ACBS_DONE	3	/* I/O is done. 		      */
#define ACBS_ERROR	4	/* Have an error		      */
#define ACBS_PATARESET	5	/*				      */
#define ACBS_SATARESET	6	/*				      */
#define ACBS_SUSPEND	7	/* Wait while another DD uses this HW */

#define ACBS_WAIT	0x80
#define ACBS_SUSPENDED	0x40
#define ACBS_STATEMASK	0x0F

/* ACB->InsertState values */

#define ACBI_WAITReset	0	/* wait for reset deassertion	      */
#define ACBI_WAITCtrlr	1	/* wait for controller detection      */
#define ACBI_WAITCmd1	2	/* wait for command 1 complete	      */
#define ACBI_WAITCmd2	3	/* wait for command 1 complete	      */
#define ACBI_WAITIdent	4	/* wait for IDENTIFY done	      */
#define ACBI_ExecDefer	5	/* wait for deferred stuff	      */
#define ACBI_STOP	6	/* stop further processing	      */

#define ACBI_SATA_WAITPhy1     7  /* delay for COM establishment	*/
#define ACBI_SATA_WAITPhy2     8  /* delay for COM establishment	*/
#define ACBI_SATA_WAITPhyReady 9  /* wait for COM establishment 	*/


#define ATS_OK			0
#define ATS_NOT_PRESENT 	1
#define ATS_SKIPPED		2
#define ATS_SET_IRQ_FAILED	3
#define ATS_ALLOC_IRQ_FAILED	4
#define ATS_ALLOC_IO_FAILED	5
#define ATS_NO_DEVICES_FOUND	6
#define ATS_PCCARD_INSERTED	7
#define ATS_PCCARD_NOT_INS	8
#define ATS_BAY 		9
#define ATS_ALLOC_SPINLOCK_FAILED	10


/* ACB->TimerFlags */
#define ACBT_BUSY	0x0001
#define ACBT_DRQ	0x0002
#define ACBT_IRQ	0x0004
#define ACBT_READY	0x0008
#define ACBT_RESETFAIL	0x0010
#define ACBT_SATACOMM	0x0020

#define BMICOM_RD	  0x08	  /* Read when 0, write when 1 */
#define BMICOM_START	  0x01	  /* Start when 1, stop when 0 */

#define BMISTA_SIMPLEX	  0x80	  /* simplex DMA channels when 1 */
#define BMISTA_D1DMA	  0x40	  /* drive 1 DMA capable */
#define BMISTA_D0DMA	  0x20	  /* drive 0 DMA capable */
#define BMISTA_INTERRUPT  0x04	  /* IDE interrupt was detected */
#define BMISTA_ERROR	  0x02	  /* transfer error detected */
#define BMISTA_ACTIVE	  0x01	  /* Bus Master DMA active */
#define BMISTA_MASK (BMISTA_SIMPLEX | BMISTA_D0DMA | BMISTA_D1DMA | BMISTA_INTERRUPT)


/*-------------------------------*/
/* DriveType			 */
/* ---------			 */
/*				 */
/* One per Drive Type.		 */
/*				 */
/*-------------------------------*/

typedef struct _DRIVETYPE
{
  USHORT	Cyl:11;
  SHORT 	Head:5;
} DRIVETYPE;

#define MAX_DRIVE_TYPES 	48

typedef struct _IHDRS {
  NPA		npA;
  VOID		(NEAR *IRQHandler)(VOID);
  UCHAR 	IRQLevel;
  UCHAR 	IRQShared;
} IHDRS;

