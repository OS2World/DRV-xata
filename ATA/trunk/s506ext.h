/**************************************************************************
 *
 * SOURCE FILE NAME =  S506EXT.H
 *
 * DESCRIPTIVE NAME =  DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2007
 *
 * DESCRIPTION : Data External References
 *
 ****************************************************************************/

#include "cmdphdr.h"

/*-------------------------------------------------------------------*/
/*								     */
/*	Static Data						     */
/*								     */
/*								     */
/*-------------------------------------------------------------------*/

extern NPA	    ACBPtrs[MAX_ADAPTERS];
extern NPA	    SocketPtrs[MAX_ADAPTERS];
extern NPA	    PCCardPtrs[MAX_ADAPTERS];
extern IHDRS	    IHdr[MAX_IRQS];
extern PFN	    Device_Help;
extern ULONG	    ppDataSeg;
extern HDRIVER	    hDriver;
extern USHORT	    ADDHandle;
extern UCHAR	    cChips;
extern UCHAR	    cAdapters;
extern UCHAR	    cUnits;
extern UCHAR	    cSockets;
extern UCHAR	    DriveId;
extern UCHAR	    InitActive;
extern UCHAR	    InitIOComplete;
extern UCHAR	    InitComplete;
extern UCHAR	    BIOSActive;
extern UCHAR	    BIOSInt13;
extern UCHAR	    EzDrivePresent;
extern struct _GINFOSEG _far *pGlobal;
extern volatile SHORT _far *msTimer;
extern struct DevClassTableStruc _far *pDriverTable;
extern ULONG	    WaitDRQCount;
extern ULONG	    CheckReadyCount;
extern USHORT	    IODelayCount;
extern USHORT	    ElapsedTimerHandle;
extern USHORT	    IdleTickerHandle;
extern ULONG	    ReqBlockID;
extern UCHAR	    PCInumBuses;
extern UCHAR	    PowerState;
extern USHORT	    Fixes;
extern NPPCI_INFO   CurPCIInfo;
extern UCHAR	    CurLevel;
extern UCHAR	    RegOffset;
extern SEL	    DSSel, CSSel, FlatSel, SelSAS;
extern NPU	    LastAccessedUnit;
extern NPA	    LastAccessedPCCardUnit;

typedef VOID _cdecl _loadds FAR Strat1Entry (VOID);
typedef Strat1Entry FAR *PStrat1Entry;

#if AUTOMOUNT
extern PStrat1Entry OS2LVM;
#endif
extern USHORT UStatus;

#if 0
extern	char haveIRQ;
extern	char haveIO;
extern	USHORT rcIO, rcIRQ, rcCFG;
extern	struct IO_P  SetIO;
extern	struct IRQ_P SetIRQ;
extern	struct GCI_P GetConfig;
#endif
extern struct GTD_P  GetTuple;
extern UCHAR  AdapterNameATAPI[17];  /* Adapter Name ASCIIZ string	    */
extern UCHAR  AdapterNamePCCrd[17];  /* Adapter Name ASCIIZ string	    */

extern UCHAR CmdTable[2*12];

extern UCHAR  ALI_ModeSets[4][3];
extern UCHAR  ALI_UltraModeSets[4][5];
extern UCHAR  ALI_Ultra100ModeSets[7];
extern UCHAR  CMD_ModeSets[4][3];
extern UCHAR  CMD_UltraModeSets[4][6];
extern ULONG  CX_PIOModeSets[4][3];
extern ULONG  CX_DMAModeSets[4][5];
extern ULONG  HPT_ModeSets[3][3];
extern UCHAR  HPT_UltraModeSets[3][6];
extern ULONG  HPT37x_ModeSets[3][3];
extern UCHAR  HPT37x_UltraModeSets[3][7];
extern USHORT PDC_ModeSets[3];
extern USHORT PDC_100ModeSets[3];
extern USHORT PDC_133ModeSets[3];
extern USHORT PDC_DMAModeSets[5];
extern USHORT PDC_DMA66ModeSets[8];
extern USHORT PDC_DMA100ModeSets[8];
extern USHORT PDC_DMA133ModeSets[9];
extern UCHAR  AEC62X0_ModeSets[3];
extern UCHAR  PIIX_ModeSets[4][3];
extern UCHAR  PIIX_UltraModeSets[4][7];
extern UCHAR  SMSC_UltraModeSets[4][7];
extern USHORT SIS_ModeSets[4][3];
extern UCHAR  SIS_UltraModeSets[4][6];
extern UCHAR  SIS_Ultra66ModeSets[4][6];
extern UCHAR  SIS_Ultra100ModeSets[6];
extern UCHAR  SIS_Ultra133ModeSets[7];
extern ULONG  SIS2_100ModeSets[3];
extern ULONG  SIS2_133ModeSets[3];
extern USHORT SIS2_UltraModeSets[7];
extern UCHAR  VIA_ModeSets[4][3];
extern UCHAR  VIA_UltraModeSets[4][7];
extern UCHAR  VIA_Ultra66ModeSets[4][7];
extern UCHAR  VIA_Ultra100ModeSets[4][7];
extern UCHAR  VIA_Ultra133ModeSets[4][7];
extern UCHAR  OSB_ModeSets[4][3];
extern UCHAR  OSB_UltraModeSets[4][7];
extern USHORT OPTI_ModeSets[2][3];
extern UCHAR  OPTI_UltraModeSets[3];
extern USHORT SII_ModeSets[3];
extern USHORT SII_MDMAModeSets[2];
extern UCHAR  ITE_ModeSets[2][3];
extern UCHAR  ITE_UltraModeSets[2][7];

extern USHORT CfgICH[];
extern USHORT CfgPIIX4[];
extern USHORT CfgPIIX3[];
extern USHORT CfgPIIX[];
extern USHORT CfgGeneric[];
extern USHORT CfgNull[];
extern USHORT CfgVIA[];
extern USHORT CfgALI[];
extern USHORT CfgPDC[];
extern USHORT CfgHPT36x[];
extern USHORT CfgHPT37x[];
extern USHORT CfgSIS[];
extern USHORT CfgSIS2[];
extern USHORT CfgCMD[];
extern USHORT CfgCX[];
extern USHORT CfgAEC[];
extern USHORT CfgOSB[];
extern USHORT CfgNFC[];
extern USHORT CfgSII[];
extern USHORT CfgITE[];

extern UCHAR	    PCIClock;
extern UCHAR	    PCIClockIndex;
extern USHORT	    Debug;
extern PCHAR	    WritePtr;
extern USHORT	    WritePtrRestart, WritePtrErr;
extern PCHAR	    ReadPtr;
extern UCHAR	    TracerEnabled, TraceErrors;
extern CHAR	    Beeps;
extern UCHAR	    LinesPrinted;
extern UCHAR	    NotShutdown;
extern USHORT	    Delay500;

/*---------------------------------------------------------------*/
/*								 */
/*	PCI Configuration Operations				 */
/*								 */
/*								 */
/*---------------------------------------------------------------*/

extern PCI_DEVICE     PCIDevice[MAX_PCI_DEVICE_DESCRIPT];
extern USHORT	      ReqCount;
extern ULONG	      Queue;

extern UCHAR	      TimerPool[TIMER_POOL_SIZE];
extern CHAR	      OEMHLP_DDName[];
extern CHAR	      PCMCIA_DDName[];
extern CHAR	      ACPICA_DDName[];
extern IDCTABLE       OemHlpIDC;
extern IDCTABLE       CSIDC;
extern IDCTABLE       ACPIIDC;
extern USHORT	      CSHandle;
extern UCHAR	      CSRegistrationComplete;
extern UCHAR	      NumSockets, FirstSocket, MaxNumSockets;
extern UCHAR	      cSockets;
extern UCHAR	      Inserting;
extern UCHAR	      IRQprefLevel;
extern USHORT	      BasePortpref;
extern ULONG	      CSCtxHook;
extern USHORT	      CSIrqAvail;
extern struct CI_Info CIInfo;

extern ACB	      ACBPool[];
extern CHAR	      DTMem[(MAX_ADAPTERS+1)*MR_1K_LIMIT - 1];


/*-------------------------------------------------------------------*/
/*								     */
/*	Initialization Data					     */
/*								     */
/*								     */
/*-------------------------------------------------------------------*/

extern CHAR	 _far MsgBuffer[8191];
extern CHAR	 _far MsgBufferEnd;
extern CHAR	      TraceBuffer[1000];
extern NPCH	      npTrace;
extern NPVOID	      MemTop;
extern NPA	      npAPool;
extern CCB	      ChipTable[MAX_ADAPTERS];
extern ACB	      AdapterTable[MAX_ADAPTERS];
extern UCHAR	      Verbose;
extern IORB_EXECUTEIO InitIORB;
extern SCATGATENTRY   ScratchSGList;
extern SCATGATENTRY   ScratchSGList1;
extern PDDD_PARM_LIST pDDD_Parm_List;
extern UCHAR	      GenericBM;
extern USHORT	      xBridgePCIAddr;
extern USHORT	      xBridgeDevice;
extern UCHAR	      xBridgeRevision;
extern UCHAR	      PCILatency;

// Scratch buffers used to get the BIOS Parameter Block
// during initializaion.
extern BOOT_RECORD	     BootRecord;
extern BOOT_RECORD	     ExtendedBootRecord;
extern PARTITION_BOOT_RECORD PartitionBootRecord;
extern ULONG	      RegBlock;

extern MSGTABLE       InitMsg;
extern NPSZ	      AdptMsgs[];
extern NPSZ	      UnitMsgs[];
extern NPSZ	      MsgSMS;
extern NPSZ	      MsgAML;
extern NPSZ	      MsgLBAOn;
extern NPSZ	      MsgDMAOn;
extern NPSZ	      MsgDMAtxt;
extern NPSZ	      MsgBMOn;
extern NPSZ	      MsgTypeBDMAOn;
extern NPSZ	      MsgTypeFDMAOn;
extern NPSZ	      MsgTypeMDMAOn;
extern NPSZ	      MsgSGOn;
extern NPSZ	      MsgONTrackOn;
extern NPSZ	      MsgEzDrive;
extern NPSZ	      MsgEzDriveFBP;
extern NPSZ	      ATAPIMsg;
extern NPSZ	      MsgUnitForce;
extern NPSZ	      MsgUnitPCMCIA;
extern NPSZ	      MsgACBNotVia;
extern NPSZ	      MsgNull;
extern UCHAR	      SII68xMsgtxt[];
extern UCHAR	      CMD64xMsgtxt[];
extern UCHAR	      RZ1000Msgtxt[];
extern UCHAR	      PIIXxMsgtxt[];
extern UCHAR	      VIA571Msgtxt[];
extern UCHAR	      SiS5513Msgtxt[];
extern UCHAR	      ALI5229Msgtxt[];
extern UCHAR	      PDCxMsgtxt[];
extern UCHAR	      ICHxMsgtxt[];
extern UCHAR	      ICHSATAMsgtxt[];
extern UCHAR	      Cx5530Msgtxt[];
extern UCHAR	      HPT36xMsgtxt[];
extern UCHAR	      AMDxMsgtxt[];
extern UCHAR	      AEC62xMsgtxt[];
extern UCHAR	      SLC90E66Msgtxt[];
extern UCHAR	      OSBMsgtxt[];
extern UCHAR	      OPTIMsgtxt[];
extern UCHAR	      NForceMsgtxt[];
extern UCHAR	      ITEMsgtxt[];
extern UCHAR	      IXPMsgtxt[];
extern UCHAR	      NsSCxMsgtxt[];
extern UCHAR	      NetCellMsgtxt[];
extern UCHAR	      JMMsgtxt[];
extern UCHAR	      PromiseMIOtxt[];
extern UCHAR	      MarvellMsgtxt[];
extern UCHAR	      InitioMsgtxt[];
extern UCHAR	      GenericBMMsgtxt[];
extern UCHAR	      GenericMsgtxt[];
extern UCHAR	      ParmErrMsg[];
extern UCHAR	      VersionMsg[];
extern UCHAR	      EBIOSIdString[];
extern UCHAR	      PromiseIdString[];
extern UCHAR	      ScratchBuf[SCRATCH_BUF_SIZE];

extern UCHAR	      ErrMsg1[];
extern UCHAR	      ErrMsg2[];
extern UCHAR	      ErrMsg3[];
extern UCHAR	      ErrMsg4[];

/*-------------------------------------------------------------------*/
/*								     */
/*	Drive Type Table					     */
/*								     */
/*								     */
/*-------------------------------------------------------------------*/

 extern DRIVETYPE      DriveTypeTable[MAX_DRIVE_TYPES];


/*-------------------------------------------------------------------*/
/*								     */
/*	Command Line Parser Data				     */
/*								     */
/*								     */
/*-------------------------------------------------------------------*/

extern PSZ	     pcmdline1;
extern PSZ	     pcmdline_slash;
extern PSZ	     pcmdline_start;
extern INT	     tokv_index;
extern INT	     state_index;
extern INT	     length;
extern CHARBYTE      tokvbuf[];
extern NPOPT	     pend_option;
extern NPOPT	     ptable_option;
extern BYTE	     *poutbuf1;
extern BYTE	     *poutbuf_end;
extern CC	     cc;
extern USHORT	     outbuf_len;
extern PBYTE	     poutbuf;
extern OPTIONTABLE   opttable;
extern USHORT	     DrvrNameSize;
extern DRIVERSTRUCT  DriverStruct;
extern ADAPTERSTRUCT  AdapterStruct;
extern DEVICESTRUCT  DevStruct;
extern UCHAR	     DevDescriptNameTxt[16];

/*----------------------------------------------------------------*/
/*								  */
/*	Mini-VDM Data						  */
/*								  */
/*								  */
/*----------------------------------------------------------------*/
extern BOOL	     VDMInt13Created;
extern BOOL	     VDMInt13Active;
extern VDMINT13CB    VDMInt13;
extern ULONG	     VDMDskBlkID;
extern ULONG	     VDMInt13BlkID;

/*-------------------------------------------------------------------*/
/*								     */
/*	Verbose Data						     */
/*								     */
/*								     */
/*-------------------------------------------------------------------*/

extern UCHAR VPCIInfo[];
extern UCHAR VPCardInfo[];
extern UCHAR VControllerInfo[];
extern UCHAR VUnitInfo1[];
extern UCHAR VUnitInfo2[];
extern UCHAR VModelInfo[];
extern UCHAR VTimingInfo[];
extern UCHAR VModelUnknown[];
extern UCHAR VBPBInfo[];
extern UCHAR VCMDInfo[];
extern UCHAR VI13Info[];
extern UCHAR VIDEInfo[];
extern UCHAR VGeomInfo0[];
extern UCHAR VGeomInfo1[];
extern UCHAR VGeomInfo2[];
extern UCHAR VGeomInfo3[];
extern UCHAR VBlankLine[];
extern UCHAR VString[];
extern UCHAR VSata0[];
extern UCHAR VSata1[];
extern UCHAR VSata2[];
extern UCHAR VULTRADMAString[];
extern UCHAR VDMAString[];
extern UCHAR VPIOString[];
extern UCHAR VPauseVerbose0[];
extern UCHAR VPauseVerbose1[];

 extern void *_CodeEnd;
