/**************************************************************************
 *
 * SOURCE FILE NAME = ATAPIEXT.H
 *
 * DESCRIPTION : Data External References
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 ***************************************************************************/

#include "rmbase.h"

/*-------------------------------------------------------------------*/
/*								     */
/*	Static Data						     */
/*								     */
/*								     */
/*-------------------------------------------------------------------*/

extern USHORT	       DriveMediaType[];

typedef UCHAR	       tIdentDataBuf[IDENTIFY_DATA_BYTES];
extern SCATGATENTRY    IDDataSGList[MAX_ADAPTERS];
extern ULONG	       ppIdentDataBuf[MAX_ADAPTERS];
extern tIdentDataBuf   IdentDataBuf[MAX_ADAPTERS];

typedef UCHAR	       tSenseDataBuf[SENSE_DATA_BYTES];
extern SCATGATENTRY    SenseDataSGList[MAX_ADAPTERS];
extern ULONG	       ppSenseDataBuf[MAX_ADAPTERS];
extern tSenseDataBuf   SenseDataBuf[MAX_ADAPTERS];

extern ACBPTRS	    ACBPtrs[MAX_ADAPTERS];
extern PFN	    Device_Help;
extern PFN	    RM_Help;
extern PFN	    RM_Help0;
extern PFN	    RM_Help3;
extern USHORT	    ADDHandle;
extern UCHAR	    cAdapters;
extern UCHAR	    cUnits;
extern UCHAR	    InitIOComplete;
extern USHORT	    MachineID;
extern struct _GINFOSEG _far *pGlobal;
extern volatile SHORT _far *msTimer;
extern ULONG	    ppDataSeg;
extern ULONG	    IODelayCount;
extern UCHAR	    InitComplete;
extern UCHAR	    AdapterName[17];  /* Adapter Name ASCIIZ string */
extern UCHAR	    ExportSCSI;
extern NPU	    isCDWriter;
extern UCHAR	    DirTable[32];
extern UCHAR	    INTTIMEOUTMSG[];
extern USHORT	    AddSenseDataMap[];
extern USHORT	    MaxAddSenseDataEntry;
extern USHORT	    cResets;
extern UNITINFO     NewUnitInfoA;
extern UNITINFO     NewUnitInfoB;
extern USHORT	    LevelInterrupt;
extern USHORT	    Delay500;
extern PCHAR	    WritePtr;
extern PCHAR	    ReadPtr;
extern struct DevClassTableStruc FAR *pDriverTable;
extern CHAR    _far MsgBuffer[8191];
extern CHAR    _far MsgBufferEnd;

extern SCSI_REQSENSE_DATA no_audio_status;
extern SCSI_REQSENSE_DATA medium_not_present;
extern SCSI_REQSENSE_DATA invalid_field_in_cmd_pkt;
extern SCSI_REQSENSE_DATA invalid_cmd_op_code;
extern SCSI_INQDATA	  inquiry_data;
extern UCHAR		  mode_sense_10_page_cap[];
extern UCHAR		  mode_sense_10_page_all[];

extern USHORT SenseKeyMap[16];

/*-------------------------------------------------------------------*/
/*								     */
/*	Area to build Control Blocks				     */
/*								     */
/*								     */
/*-------------------------------------------------------------------*/

extern UCHAR	    ACBPool[ACB_POOL_SIZE + TIMER_POOL_SIZE +
			     UNITINFO_POOL_SIZE + SG_POOL_SIZE];
extern USHORT	    ACBPoolAvail;
extern NPBYTE	    npAPool;

/*-------------------------------------------------------------------*/
/*								     */
/*	Initialization Data					     */
/*								     */
/*								     */
/*-------------------------------------------------------------------*/

extern UCHAR		      savedhADD[2];
extern CHAR		      foundFloppy;
extern CHAR		      ZIPtoA;

extern ACB		      AdapterTable[];
extern UCHAR		      Verbose;
extern UCHAR		      Installed;
extern SCATGATENTRY	      IdentifySGList;
extern CHAR		      MatshitaID[];
extern CHAR		      Nec01ID[];
extern CHAR		      Nec01FWID[];
extern CHAR		      Nec02ID[];
extern CHAR		      LS120ID[];
extern CHAR		      ZIP_DriveID[];
extern CHAR		      OnStreamID[];
extern UCHAR		      OnstreamInit[12];
extern CHAR		      Disconnect[];
extern CHAR		      BlankSerial[];
extern PDDD_PARM_LIST	      pDDD_Parm_List;
extern IDCTABLE 	      DDTable;
extern MSGTABLE 	      InitMsg;
extern NPSZ		      ProtocolTypeMsgs[];
extern NPSZ		      DeviceTypeMsgs[];
extern UCHAR		      DeviceTypeMapper[];
extern NPSZ		      CMDDRQTypeMsgs[];
extern NPSZ		      MsgSMSOn;
extern NPSZ		      MsgLBAOn;
extern NPSZ		      MsgNull;
extern UCHAR		      ParmErrMsg[];
extern UCHAR		      VersionMsg[];
extern UCHAR		      ScratchBuf[SCRATCH_BUF_SIZE];
extern UCHAR		      ScratchBuf1[SCRATCH_BUF_SIZE];
extern IORB_ADAPTER_PASSTHRU  InitIORB;

extern UCHAR		      StringBuffer[];
extern UCHAR		      INTStringBuffer[];

extern			      DiskDDHeader;
extern HDRIVER		      hDriver;
extern UCHAR		      SearchKeytxt[];
extern DEVICESTRUCT	      DevStruct;
extern DRIVERSTRUCT	      DriverStruct;
extern UCHAR		      DrvrNameTxt[];
extern USHORT		      DrvrNameSize;

extern UCHAR		      VControllerInfo[];
extern UCHAR		      VUnitInfo[];
extern UCHAR		      VModelInfo[];
extern UCHAR		      MsgPio32[];
extern UCHAR		      MsgDma[];
extern UCHAR		      MsgCDB6[];
extern UCHAR		      MsgLUN[];
extern UCHAR		      MsgSkip[];
extern UCHAR		      MsgOk[];

extern VOID		     *_CodeEnd;
