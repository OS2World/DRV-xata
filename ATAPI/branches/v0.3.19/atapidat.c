/**************************************************************************
 *
 * SOURCE FILE NAME = ATAPIDAT.C
 *
 * DESCRIPTION : Global Data for ATAPI driver
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION :
 ***************************************************************************/
#define INCL_NOPMAPI
#define INCL_DOSINFOSEG
#define INCL_INITRP_ONLY
#include "os2.h"
#include "dskinit.h"

#include "iorb.h"
#include "addcalls.h"
#include "dhcalls.h"
#include "reqpkt.h"

#include "scsi.h"
#include "cdbscsi.h"

#include "atapicon.h"
#include "atapireg.h"
#include "atapityp.h"
#include "atapiext.h"
#include "atapipro.h"

#define YEAR  2006
#define MONTH 4
#define DAY   20

ACBPTRS      ACBPtrs[MAX_ADAPTERS] = { { 0, &AdapterIRQ0 },
				       { 0, &AdapterIRQ1 },
				       { 0, &AdapterIRQ2 },
				       { 0, &AdapterIRQ3 },
				       { 0, &AdapterIRQ4 },
				       { 0, &AdapterIRQ5 },
				       { 0, &AdapterIRQ6 },
				       { 0, &AdapterIRQ7 } };

UCHAR	     AdapterName[17]	   = "ATAPI           "; /* Adapter Name ASCII string */
UCHAR	     ExportSCSI 	   = 1;
UCHAR	     startresume	   = 0;
UCHAR	     startsuspend	   = 0;
PFN	     Device_Help	   = 0L;
USHORT	     ADDHandle		   = 0;
UCHAR	     cAdapters		   = 0;
UCHAR	     cUnits		   = 0;
UCHAR	     InitIOComplete	   = 0;
PGINFOSEG    pGlobal		   = 0;
volatile SHORT _far *msTimer	   = 0;
ULONG	     ppDataSeg		   = 0L;
ULONG	     IODelayCount	   = 0;
NPU	     isCDWriter 	   = 0;
UCHAR	     InitComplete	   = 0;
PCHAR	     WritePtr		   = 0;
PCHAR	     ReadPtr		   = 0;
struct DevClassTableStruc FAR *pDriverTable = NULL;
SCATGATENTRY IDDataSGList[MAX_ADAPTERS]    = { 0 };

tSenseDataBuf SenseDataBuf[MAX_ADAPTERS]   = { 0 };
ULONG	     ppSenseDataBuf[MAX_ADAPTERS]  = { 0 };
SCATGATENTRY SenseDataSGList[MAX_ADAPTERS] = { 0 };

UCHAR	     DirTable[32]	   = {
				0x09, 0x20, 0x14, 0x84, 0x00, 0x22, 0xE0, 0xF5,
				0xC0, 0x08, 0x8C, 0x04, 0x00, 0x00, 0x00, 0x01,
				0xE0, 0x22, 0x10, 0x00, 0x34, 0x22, 0xE2, 0x01,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

USHORT AddSenseDataMap[] = {
   0,				     /* 00h No sense information	      */
   IOERR_RBA_ADDRESSING_ERROR,	     /* 01h No index/sector signal	      */
   IOERR_RBA_ADDRESSING_ERROR,	     /* 02h No seek complete		      */
   IOERR_RBA_CRC_ERROR, 	     /* 03h Write fault 		      */
   IOERR_UNIT_NOT_READY,	     /* 04h Drive not ready		      */
   IOERR_ADAPTER_DEVICE_TIMEOUT,     /* 05h LUN not selected		      */
   IOERR_RBA_ADDRESSING_ERROR,	     /* 06h No track zero found 	      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 07h Multiple drives selected	      */
   IOERR_ADAPTER_DEVICE_TIMEOUT,     /* 08h Logical unit communication failure*/
   IOERR_RBA_ADDRESSING_ERROR,	     /* 09h Track following error	      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 0Ah Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 0Bh Reserved			      */
   IOERR_RBA_CRC_ERROR, 	     /* 0Ch Write Error 		      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 0Dh Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 0Eh Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 0Fh Reserved			      */
   IOERR_RBA_ADDRESSING_ERROR,	     /* 10h ID CRC or ECC error 	      */
   IOERR_RBA_CRC_ERROR, 	     /* 11h Unrecovered Read error	      */
   IOERR_RBA_ADDRESSING_ERROR,	     /* 12h No address mark found	      */
   IOERR_RBA_ADDRESSING_ERROR,	     /* 13h No addr mark in data field	      */
   IOERR_RBA_ADDRESSING_ERROR,	     /* 14h No record found		      */
   IOERR_RBA_ADDRESSING_ERROR,	     /* 15h Seek positioning error	      */
   IOERR_RBA_ADDRESSING_ERROR,	     /* 16h Data synchroniztion mark error    */
   IOERR_RBA_CRC_ERROR, 	     /* 17h Recovered read w/ retries	      */
   IOERR_RBA_CRC_ERROR, 	     /* 18h Recovered read data w/correction  */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 19h Defect list error		      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 1Ah Parameter overrun		      */
   IOERR_ADAPTER_DEVICEBUSCHECK,     /* 1Bh Synchronous transfer err	      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 1Ch Primary defect list not found     */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 1Dh Compare error		      */
   IOERR_RBA_ADDRESSING_ERROR,	     /* 1Eh Recovered ID with ECC correction  */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 1Fh Reserved			      */
   IOERR_CMD_SYNTAX,		     /* 20h Invalid command op code	      */
   IOERR_RBA_ADDRESSING_ERROR,	     /* 21h Illegal logical blk addr	      */
   IOERR_CMD_SYNTAX,		     /* 22h Illegal func for dev type	      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 23h Reserved			      */
   IOERR_CMD_SYNTAX,		     /* 24h Illegal field in CDW	      */
   IOERR_CMD_SYNTAX,		     /* 25h Invalid LUN 		      */
   IOERR_CMD_SYNTAX,		     /* 26h Invalid param list field	      */
   IOERR_MEDIA_WRITE_PROTECT,	     /* 27h Write protected		      */
   IOERR_MEDIA_CHANGED, 	     /* 28h Medium changed		      */
   IOERR_DEVICE_RESET,		     /* 29h Power on reset or bus device reset*/
   IOERR_MEDIA_CHANGED, 	     /* 2Ah Mode select parameters changed    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 2Bh Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 2Ch Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 2Dh Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 2Eh Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 2Fh Reserved			      */
   IOERR_MEDIA_NOT_SUPPORTED,	     /* 30h Incompatible cartridge	      */
   IOERR_MEDIA_NOT_FORMATTED,	     /* 31h Medium format corrupted	      */
   IOERR_MEDIA_NOT_FORMATTED,	     /* 32h No defect spare location available*/
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 33h Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 34h Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 35h Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 36h Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 37h Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 38h Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 39h Reserved			      */
   IOERR_MEDIA_NOT_PRESENT,	     /* 3Ah Medium Not Present		      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 3Bh Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 3Ch Reserved			      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 3Dh Reserved			      */
   IOERR_UNIT_NOT_READY,	     /* 3Eh LUN  not self-configured yet      */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 3Fh Reserved			      */
   IOERR_DEVICE_DIAGFAIL,	     /* 40h RAM failure 		      */
   IOERR_DEVICE_DIAGFAIL,	     /* 41h Data path diagnostic failure      */
   IOERR_DEVICE_DIAGFAIL,	     /* 42h Power-on diag failure	      */
   IOERR_DEVICE_DEVICEBUSCHECK,      /* 43h Message reject error	      */
   IOERR_DEVICE,		     /* 44h Internal controller error	      */
   IOERR_UNIT_NOT_READY,	     /* 45h Select/reselect failed	      */
   IOERR_DEVICE_BUSY,		     /* 46h Unsuccessful soft reset	      */
   IOERR_DEVICE_DEVICEBUSCHECK,      /* 47h SCSI interface parity error       */
   IOERR_DEVICE_DEVICEBUSCHECK,      /* 48h Initiator detected error	      */
   IOERR_DEVICE_DEVICEBUSCHECK,      /* 49h Inappropriate/illegal message     */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 50H - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 51H - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 52H - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 53H - Media Load or eject failed    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 54H - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 55H - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 56H - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 57H - Unable to recover TOC	    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 58H - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 59H - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 5AH - Operator request/state change */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 5BH - LOG exception		    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 5CH - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 5DH - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 5EH - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 5FH - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 60H - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 61H - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 62H - Reserved			    */
   IOERR_ADAPTER_REFER_TO_STATUS,    /* 63H - End of User area encountered  */
   IOERR_MEDIA_NOT_SUPPORTED	     /* 64H - Illegal Mode for this track   */
   };

USHORT	     MaxAddSenseDataEntry = (sizeof( AddSenseDataMap )/sizeof( USHORT ));

USHORT	     cResets	  = 0;	  /* Global reset counter */

UNITINFO     NewUnitInfoA = { 0 };
UNITINFO     NewUnitInfoB = { 0 };

USHORT	     ACBPoolAvail	   = ACB_POOL_SIZE + TIMER_POOL_SIZE +
				     UNITINFO_POOL_SIZE;

SCATGATENTRY IdentifySGList		   = { 0, 512 };

IORB_ADAPTER_PASSTHRU InitIORB		   = { 0 };

UCHAR	     ScratchBuf[SCRATCH_BUF_SIZE]  = { 0 };
UCHAR	     ScratchBuf1[SCRATCH_BUF_SIZE] = { 0 };

tIdentDataBuf IdentDataBuf[MAX_ADAPTERS]   = { 0 };
ULONG	     ppIdentDataBuf[MAX_ADAPTERS]  = { 0 };

/* SCSI Sense Data */
SCSI_REQSENSE_DATA   no_audio_status = {
/* Error,   Sense Key,	     Additional Len,  ASC,  ASCQ */
/* ----     ----	     ----	      ----  ---- */
   0x70, 0, 0x00, {0,0,0,0}, 0x0a, {0,0,0,0}, 0x00, 0x15, 0, {0,0,0}};

SCSI_REQSENSE_DATA   medium_not_present = {
   0x70, 0, 0x02, {0,0,0,0}, 0x0a, {0,0,0,0}, 0x3a, 0x00, 0, {0,0,0}};

SCSI_REQSENSE_DATA   invalid_field_in_cmd_pkt = {
   0x70, 0, 0x05, {0,0,0,0}, 0x0a, {0,0,0,0}, 0x24, 0x00, 0, {0,0,0}};

SCSI_REQSENSE_DATA   invalid_cmd_op_code = {
   0x70, 0, 0x05, {0,0,0,0}, 0x0a, {0,0,0,0}, 0x20, 0x00, 0, {0,0,0}};


/* SCSI Inquiry data */
SCSI_INQDATA   inquiry_data = {
   0x05, 0x80, 0x00, 0x01, 0x1f, 0x00, 0x00, 0x00,
   "ATAPI   ",                /* Vendor Identification */
   "disconnected    ",        /* Product Identification */
   "    "                     /* Product Revision */
};


/* SCSI Mode Sense data */
UCHAR	 mode_sense_10_page_cap[LEN_MODE_SENSE_10] = {
   0x00, 0x16, 0x71, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x2a, 0x0e, 0x07, 0x07, 0x71, 0x6F, 0x29, 0x00,
   0x01, 0x61, 0x00, 0x10, 0x00, 0x40, 0x01, 0x61
};

UCHAR	 mode_sense_10_page_all[8] = {
   0x00, 0x76, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00
};


/*
** Initialize device strings.
*/
CHAR	    MatshitaID[]	   = "                    MATSHITA CR-571 0.2d";
CHAR	    Nec01ID[]		   = "NEC                 CD-ROM DRIVE:260    ";
CHAR	    Nec01FWID[] 	   = "1.01    ";
CHAR	    Nec02ID[]		   = "EN C                DCR-MOD IREV2:06    ";
CHAR	    ZIP_DriveID[]	   = "IOMEGA  ZIP ";
CHAR	    OnStreamID[]	   = " OnStream DI-30";
CHAR	    Disconnect[]	   = "[disconnected]";

UCHAR	    OnstreamInit[12]	   = {3 + 8, 0, 0, 0, 0x36 + 0x80, 6, 'O','S','/','2', 0, 0};

UCHAR DeviceTypeMapper[32] = {
  UIB_TYPE_DISK, UIB_TYPE_TAPE, UIB_TYPE_PRINTER, UIB_TYPE_PROCESSOR,
  UIB_TYPE_WORM, UIB_TYPE_CDROM, UIB_TYPE_SCANNER, UIB_TYPE_OPTICAL_MEMORY,
  UIB_TYPE_CHANGER, UIB_TYPE_COMM,
  UIB_TYPE_ATAPI, UIB_TYPE_ATAPI, UIB_TYPE_ATAPI, UIB_TYPE_ATAPI, UIB_TYPE_ATAPI,
  UIB_TYPE_ATAPI, UIB_TYPE_ATAPI, UIB_TYPE_ATAPI, UIB_TYPE_ATAPI, UIB_TYPE_ATAPI,
  UIB_TYPE_ATAPI, UIB_TYPE_ATAPI, UIB_TYPE_ATAPI, UIB_TYPE_ATAPI, UIB_TYPE_ATAPI,
  UIB_TYPE_ATAPI, UIB_TYPE_ATAPI, UIB_TYPE_ATAPI, UIB_TYPE_ATAPI, UIB_TYPE_ATAPI,
  UIB_TYPE_ATAPI, UIB_TYPE_UNKNOWN
};

USHORT SenseKeyMap[16] = {
  0, IOERR_ADAPTER_REFER_TO_STATUS, IOERR_UNIT_NOT_READY, IOERR_ADAPTER_REFER_TO_STATUS,
  IOERR_DEVICE, IOERR_CMD_SYNTAX, IOERR_MEDIA_CHANGED, IOERR_ADAPTER_REFER_TO_STATUS,
  IOERR_ADAPTER_REFER_TO_STATUS, IOERR_ADAPTER_REFER_TO_STATUS, IOERR_ADAPTER_REFER_TO_STATUS, IOERR_ADAPTER_REFER_TO_STATUS,
  IOERR_ADAPTER_REFER_TO_STATUS, IOERR_ADAPTER_REFER_TO_STATUS, IOERR_ADAPTER_REFER_TO_STATUS, IOERR_ADAPTER_REFER_TO_STATUS
};

ACB	     AdapterTable[MAX_ADAPTERS] = { 0 };

/* The throw away section begins with npAPool.	(At the end of init, it has
   been incremented by the amount of used memory for internal structures.  Any
   unused portion, and all the init data is then thrown away.) */

NPBYTE	     npAPool		   = ACBPool;
UCHAR	     ACBPool[]		   = { 0 };

/*
** Start of Initialization Data
**
** This is the address of the discardable data.  All data
** references after this point will be discarded after init.
** The throw away section begins with BeginInitData.  At the
** end of init, it has been incremented by the amount of used
** memory for internal structures.  Any unused portion, and
** all the init data is then thrown away.
*/

UCHAR	     savedhADD[2];
CHAR	     foundFloppy;
CHAR	     ZIPtoA = 0;

UCHAR	     Verbose		   = FALSE;
UCHAR	     Installed		   = FALSE;

IDCTABLE     DDTable		   = { 0 };


#define      MSG_REPLACEMENT_STRING  1178
MSGTABLE     InitMsg		   = { MSG_REPLACEMENT_STRING, 1, 0 };

//UCHAR        ParmErrMsg[]	     = " Warning: DaniATAPI.FLT - Invalid CONFIG.SYS parameters near pos %d";
UCHAR	     ParmErrMsg[]	   = "SYS    : DaniATAPI.FLT - Invalid CONFIG.SYS parameters near pos %d";
UCHAR	     VersionMsg[]	   = "            Daniela's ATAPI Filter Version "VERSION;
UCHAR	     StringBuffer[80]	   = { 0 };

UCHAR	     SearchKeytxt[]	   = "IDE_0";
HDRIVER      hDriver		   = 0L;
PFN	     RM_Help		   = 0L;
PFN	     RM_Help0		   = 0L;
PFN	     RM_Help3		   = 0L;
ULONG	     RMFlags		   = 0L;

UCHAR  DrvrNameTxt[]	 = "DaniATAPI.FLT";
USHORT DrvrNameSize	 = sizeof(DrvrNameTxt);
UCHAR  DrvrDescriptTxt[] = "Filter Driver for IDE ATAPI devices";
UCHAR  VendorNameTxt[]	 = "DANI";

DRIVERSTRUCT DriverStruct =
{
   DrvrNameTxt, 			     /* DrvrName		*/
   DrvrDescriptTxt,			     /* DrvrDescript		*/
   VendorNameTxt,			     /* VendorName		*/
   CMVERSION_MAJOR,			     /* MajorVer		*/
   CMVERSION_MINOR,			     /* MinorVer		*/
   YEAR,MONTH,DAY,			     /* Date			*/
   0,					     /* DrvrFlags		*/
   DRT_ADDDM,				     /* DrvrType		*/
   DRS_FILTER,				     /* DrvrSubType		*/
   NULL 				     /* DrvrCallback		*/
};

/*----------------------------------------------*/
/* Device Description				*/
/*----------------------------------------------*/
UCHAR DevDescriptNameTxt[70] = "ATAPI_0 ";

DEVICESTRUCT DevStruct =
{
   DevDescriptNameTxt,	 /* DevDescriptName; */
   DS_FIXED_LOGICALNAME, /* DevFlags;	     */
   DS_TYPE_CDROM	 /* DevFlags;	     */
};

UCHAR VControllerInfo[] = "Controller:%1d  Port:%04x IRQ:%02x  Status:%s";
UCHAR VUnitInfo[]	= " Unit:%1d Status:%s%s%s%s%s";
UCHAR VModelInfo[]	= "  Model:%s";
UCHAR MsgPio32[]	= " PIO32";
UCHAR MsgDma[]		= " DMA";
UCHAR MsgCDB6[] 	= " X/CDB6";
UCHAR MsgLUN[]		= " LUN:0";
UCHAR MsgSkip[] 	= "skipped";
UCHAR MsgOk[]		= "OK";

