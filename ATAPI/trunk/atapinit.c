/**************************************************************************
 *
 * SOURCE FILE NAME = ATAPINIT.C
 *
 * DESCRIPTION : DaniATAPI driver initialization
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2007
 *
 * DESCRIPTION : initialization routines
 ***************************************************************************/

#define INCL_NOPMAPI
#define INCL_DOSERRORS
#define INCL_DOSINFOSEG
#define INCL_INITRP_ONLY
#include "os2.h"
#include "devclass.h"
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
#include "rmcalls.h"
#include "cmdpext.h"

#pragma optimize(OPTIMIZE, on)

#define PCITRACER 0
#define TRPORT 0xc410

#if PCITRACER
#define TR(x) outp (TRPORT,(x));
#else
#define TR(x)
#endif

#undef UIB_TYPE_ATAPI
#define UIB_TYPE_ATAPI 0x20

VOID NEAR set_data_in_sg (NPA, NPCH, USHORT);

// If the CDROM is not present, use this code to Fake the
// operations so that OS2CDROM.DMD believes that a CD-ROM
// is really there but empty

USHORT NEAR FakeATA (NPA npA)
{
  PIORB pIORB = npA->pIORB;

#if PCITRACER
  outp (TRPORT+1, 0xFB);
#endif

  pIORB->Status    = IORB_DONE | IORB_ERROR;
  pIORB->ErrorCode = IOERR_CMD_SYNTAX;	   // Failure code
  return (1);	   //error, unknown command
}

//
// Fake ATAPI requests
//
USHORT NEAR FakeATAPI (NPA npA)
{
  PIORB 	       pIORB   = npA->pIORB;
  NPU		       npU     = (NPU)(pIORB->UnitHandle);
  NPCMDIO	       npCmdIO = npA->npCmdIO;
  PSCSI_REQSENSE_DATA  pSenseData; /* for setting return status */
  struct CDB_ModeSense_10 NEAR *npCDB;
  UCHAR 	       ASC;	   /* additional sense code */
  PSCSI_STATUS_BLOCK   pSCSISB;    /* pointer to the SCSI status block */
  USHORT	       ErrorCode = 0;

  /* ATAPI Commands for CD-ROM Drives */

#if PCITRACER
  outp (TRPORT+1, 0xFA);
#endif

  switch (npCmdIO->ATAPIPkt[0]) {
    case SCSI_INQUIRY :
      /* set INQUIRY data in s/g list. */

      if (InitComplete && npA->npU[npU->UnitId].npUnext)
	inquiry_data.DevType = UIB_TYPE_UNKNOWN;
      else
	inquiry_data.DevType = npU->pUI->UnitType;

#if PCITRACER
  outp (TRPORT+2, inquiry_data.DevType);
#endif
      set_data_in_sg (npA, (NPCH)&inquiry_data, sizeof (SCSI_INQDATA));
      pSenseData = &no_audio_status;	   /* set OK status */
      break;

    case SCSI_MODE_SELECT_10 :
      /* set Invalid field in Command Packet */
      pSenseData = &invalid_field_in_cmd_pkt;
      break;

    case SCSI_MODE_SENSE_10 :
      npCDB = (struct CDB_ModeSense_10 NEAR *)(npCmdIO->ATAPIPkt);

       /* set NOT-READY status */
      pSenseData = &medium_not_present;

      if (npCDB->PC == PC_DEFAULT || npCDB->PC == PC_CURRENT) {
	if (npCDB->page_code == PAGE_CAPABILITIES) {
	/* set mode sense data in s/g list. */
	  *(NPUSHORT)(mode_sense_10_page_cap + 10) =
	   (npU->Flags & UCBF_CDWRITER) ? 0x0707 : 0;
	  set_data_in_sg (npA, mode_sense_10_page_cap, LEN_MODE_SENSE_10);
	  pSenseData = &no_audio_status;       /* set OK status */
	} else if (npCDB->page_code == PAGE_ALL) {
	  set_data_in_sg (npA, mode_sense_10_page_all, 8);
	  pSenseData = &no_audio_status;       /* set OK status */
	}
      }
      break;

    case SCSI_REQUEST_SENSE :
      /* sense data is read in s/g list.* */
      set_data_in_sg (npA, (NPCH)&medium_not_present, sizeof (SCSI_REQSENSE_DATA));
      pSenseData = &no_audio_status;	   /* set OK status */
      break;

    case SCSI_TEST_UNIT_READY :
    default :
      /* set NOT-READY status */
      pSenseData = &medium_not_present;
      npU->MediaStatus = MF_NOMED;
      break;

  }


  /* set ErrorCode from additional Sense Code. */
  /* pSenseData must be set appropriately. */

  ASC = pSenseData->AddSenseCode;
  if (ASC > MaxAddSenseDataEntry) {
    ErrorCode = IOERR_ADAPTER_REFER_TO_STATUS;
  } else {
    ErrorCode = AddSenseDataMap[ASC];
  }
  npA->IORBError = ErrorCode; /* make sure */

  if (ErrorCode) {
    /* return sense data if requested */

    if (pIORB->RequestControl & IORB_REQ_STATUSBLOCK) {
      pSCSISB = MAKEP (SELECTOROF(pIORB), (NPBYTE)pIORB->pStatusBlock);
      memcpy (pSCSISB->SenseData, pSenseData,
	      (pSCSISB->ReqSenseLen > SENSE_DATA_BYTES) ?
		  SENSE_DATA_BYTES : pSCSISB->ReqSenseLen);

      /* sense data is vaild */
      pSCSISB->Flags |= STATUS_SENSEDATA_VALID;
      /* status info returned */
      pIORB->Status |= IORB_STATUSBLOCK_AVAIL;
    }
  }

  return (FALSE);
}


/*----------------------------------------------------------------*/
/* Function:   Copy data from buffer to S/G list.		  */
/* Input:      pbuf, pIORB, buf_len				  */
/* Output:     none						  */
/*----------------------------------------------------------------*/
VOID NEAR set_data_in_sg (NPA npA, NPCH pbuf, USHORT len)
{
  USHORT l;

  l = MapSGList (npA);
  if (len < l) l = len;
  memcpy (MAKEP (npA->MapSelector, 0), pbuf, l);
}


/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
PIORB _cdecl _loadds FAR NotifyIORBDone (PIORB pIORB)
{
  USHORT AwakeCount;

  if (pIORB->Status & IORB_DONE)
    DevHelp_ProcRun ((ULONG) pIORB, &AwakeCount);
}

NPU NEAR ConfigureUnit (NPA npA, NPIDENTIFYDATA npID, UCHAR Reconfigure)
{
  NPU	npU = npA->npU;
  UCHAR UnitType = 0xff;
  UCHAR Capabilities = 0;
  UCHAR Model = 0;
  USHORT v, *p;

#define  NEC_CDR_260	 1
#define  NEC_CDR_250	 2
#define  MATSHITA_CR_571 3

  {
    USHORT i;
    for (i = ATA_BACKOFF; i > 0; i--) IODelay();
  }

  npU->LUN = 0;
  npU->DriveLUN.Rev_17B = 0;

  if (_strncmp (MatshitaID, npID->ModelNum, sizeof(npID->ModelNum)))
    Model = MATSHITA_CR_571;
  else if (_strncmp (Nec01ID, npID->ModelNum, sizeof(npID->ModelNum))) {
    Model = NEC_CDR_260;
    if (_strncmp (Nec01FWID, npID->Firmware, sizeof(npID->Firmware))) {
      USHORT timeout = (INIT_TIMEOUT_LONG * 4) / 1000L;
      Capabilities |= UCBC_SPEC_REV_17B;

      if (timeout > npA->IRQTimeOutMin) {
	npA->IRQTimeOutMin = timeout;
	npA->IRQTimeOutDef = (INIT_TIMEOUT_LONG * 4);
      }
    }
  } else if (_strncmp (Nec02ID, npID->ModelNum, sizeof (npID->ModelNum))) {
    Model = NEC_CDR_250;
  }

  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ Save Model/Serial/Firmware Information ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  memset (npU->ModelNum, ' ', sizeof (npID->ModelNum) + sizeof (npID->Firmware));
  if ((Model == NEC_CDR_260) && (Capabilities & UCBC_SPEC_REV_17B)) {
    _strncpy (npU->ModelNum, npID->ModelNum, sizeof (npID->ModelNum));
  } else {
    strnswap (npU->ModelNum, npID->ModelNum, sizeof (npID->ModelNum));
    strnswap (npU->ModelNum + sizeof (npID->ModelNum), npID->Firmware, sizeof (npID->Firmware));
    npU->ModelNum[sizeof (npID->ModelNum) + sizeof (npID->Firmware)] = '\0';
  }

  /* ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ CMD DRQ Type      ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  if (npID->GenConfig.CmdDRQType == 1) Capabilities |= UCBC_INTERRUPT_DRQ;
  if (npID->GenConfig.Removable)       Capabilities |= UCBC_REMOVABLE;

  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ Device Type       ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  UnitType = DeviceTypeMapper[npID->GenConfig.DeviceType];

  switch (UnitType) {
    case UIB_TYPE_DISK :
      Capabilities |= UCBC_DIRECT_ACCESS_DEVICE;

      /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
      ณ      Check for LS-120 Drive	     ณ
      ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
      if ((*((NPUSHORT)(npU->ModelNum + 0)) == 0x534C) &&  // "LS"
	  (*(	       (npU->ModelNum + 2)) == '-' )   &&
	  (*((NPUSHORT)(npU->ModelNum + 5)) == 0x2030) &&  // "0 "
	 ((*((NPUSHORT)(npU->ModelNum + 3)) == 0x3231) ||  // LS-120
	  (*((NPUSHORT)(npU->ModelNum + 3)) == 0x3432))) { // LS-240
	UnitType = LS_120;
      }
      /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
      ณ     Check for IOMEGA ZIP Drive	     ณ
      ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
      else if (_strncmp (npU->ModelNum, ZIP_DriveID, ZIPID_STRGSIZE)) {
	UnitType = ZIP_DRIVE;
      }
      break;

    case UIB_TYPE_CDROM :
      Capabilities |= UCBC_CDROM_DEVICE;
      switch (Model) {
	case NEC_CDR_260 :
	case NEC_CDR_250 :     npU->CmdPacketLength = 12;	    break;
	case MATSHITA_CR_571 : Capabilities &= ~UCBC_SPEC_REV_17B;  break;
	default:	       Capabilities &= ~UCBC_SPEC_REV_17B;
      }
      if (Capabilities & UCBC_SPEC_REV_17B)
	npU->DriveLUN.Rev_17B = 1;
      break;

    case UIB_TYPE_TAPE :
      Capabilities |= UCBC_TAPE_DEVICE;
      if (_strncmp (npU->ModelNum, OnStreamID, ONSTREAMID_STRGSIZE))
	UnitType = ONSTREAM_TAPE;
      break;

    case UIB_TYPE_OPTICAL_MEMORY :
      Capabilities |= UCBC_DIRECT_ACCESS_DEVICE;
      break;

    default:
      Capabilities &= ~(UCBC_DIRECT_ACCESS_DEVICE | UCBC_CDROM_DEVICE);
  }

  if (Reconfigure) {
    while (npU) {
      if ((npU->UnitType == UnitType) ||
	 ((npU->UnitType == ZIP_DRIVE) && (UnitType == UIB_TYPE_DISK))) break;
      npU = npU->npUnext;
    }
  } else {
    npU->UnitType = UnitType;
  }

  if (npU != NULL) {
    if (Reconfigure) {
      npA->npU = npU;
      npU->Flags &= ~UCBF_NOTPRESENT;
      npU->LUN = 0;
    }

    npU->Flags		&= ~UCBF_XLATECDB6;
    npU->Capabilities	 = Capabilities;

    /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
    ณ CMD Packet Size	ณ
    ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
    switch (npID->GenConfig.CmdPktSize) {
      case 0 : npU->CmdPacketLength = 12; break;
      case 1 : npU->CmdPacketLength = 16; break;
    }

    npU->MediaStatus = 0;

    if (UnitType == LS_120) {
      npU->DrvDefFlags = DDF_LS120;
      npU->MediaType   = npID->MediaTypeCode;

      if (!(npU->MediaType)) {
	npU->MediaStatus = MF_NOMED;
      } else {
	npU->MediaGEO.Reserved	      = 0;
	npU->MediaGEO.TotalCylinders  = npID->CurrentCylHdSec[0];
	npU->MediaGEO.NumHeads	      = npID->CurrentCylHdSec[1];
	npU->MediaGEO.SectorsPerTrack = npID->CurrentCylHdSec[2];
	npU->MediaGEO.BytesPerSector  = npID->BytesPerSector;
	npU->MediaGEO.TotalSectors    = (npU->MediaGEO.TotalCylinders *
					 npU->MediaGEO.NumHeads       *
					 npU->MediaGEO.SectorsPerTrack);
      }
      GetGeometry (npU, 0x3C300);

    } else if (UnitType == ZIP_DRIVE) {
      ULONG Capacity;

      Model = npU->ModelNum[ZIPID_STRGSIZE];
      Capacity = (Model == '7') ? 0x1668B4 : ((Model == '2') ? 0x7783C : 0x30000);
      GetGeometry (npU, Capacity);
      npU->MediaGEO    = npU->DeviceGEO;
      npU->DrvDefFlags = DDF_TESTUNITRDY_REQ;
      npU->Flags      &= ~UCBF_LARGE_FLOPPY;

    } else if ((UnitType == UIB_TYPE_DISK) || (UnitType == UIB_TYPE_OPTICAL_MEMORY)) {
      GetGeometry (npU, 0x10000);
      npU->MediaGEO    = npU->DeviceGEO;
      npU->DrvDefFlags = DDF_TESTUNITRDY_REQ;
      npU->Flags      &= ~UCBF_LARGE_FLOPPY;

    } else if (UnitType == UIB_TYPE_CDROM) {
      npU->Flags |= UCBF_XLATECDB6;
      GetCDCapabilities (npU);

    } else if (UnitType == ONSTREAM_TAPE) {
      InitOnstream (npU);
    }
    if (!(npU->Flags & UCBF_XLATECDB6)) GetCDBTranslation (npU);
    GetLUNMode (npU, npID->LastLUN);
  }
  return (npU);
}

/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
USHORT NEAR ATAPIIdentify (NPA npA)
{
  NPIDENTIFYDATA npID;
  NPU		 npU;

  npID = (NPIDENTIFYDATA)ScratchBuf;

  npU = npA->UnitCB + npA->UnitId;
  if (!GetIdentifyData (npU, npID)) {
    npU = ConfigureUnit (npA, npID, FALSE);
    return (npU->UnitType);
  } else {
    return (-1);
  }
}


/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
USHORT NEAR InitSuspendResumeOtherAdd (NPA npA, USHORT function)
{
  NPIORB_DEVICE_CONTROL pIORB;

  pIORB = (NPIORB_DEVICE_CONTROL) &npA->IORB;
  clrmem (&npA->IORB, sizeof (npA->IORB));

  pIORB->iorbh.Length	       = sizeof(IORB_DEVICE_CONTROL);
  pIORB->iorbh.UnitHandle      = npA->SharedhUnit;
  pIORB->iorbh.CommandCode     = IOCC_DEVICE_CONTROL;
  pIORB->iorbh.CommandModifier = function;
  pIORB->Flags		      |= DC_SUSPEND_IMMEDIATE;

  if (function == IOCM_SUSPEND) {
    pIORB->Flags	      = DC_SUSPEND_IMMEDIATE | DC_SUSPEND_IRQADDR_VALID;
    pIORB->IRQHandlerAddress  = npA->IRQHandler;
  }

  return (Execute (npA->SharedhADD, (NPIORB)pIORB));
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
USHORT NEAR CheckReady (NPA npA)
{
  UCHAR Status;
  SHORT Stop;

  npA->TimerFlags &= ~ACBT_BUSY;
  Stop = *msTimer + MAX_WAIT_READY;

  while ((*msTimer - Stop) < 0) {
    Status = inp (STATUSREG);
    if (!(Status & FX_BUSY) && (Status & FX_READY)) break;
  }

  if (Status & FX_BUSY) npA->TimerFlags |= ACBT_BUSY;
  STATUS = Status;

  return (((npA->TimerFlags & ACBT_BUSY) || !(Status & FX_READY)) ? 1 : 0);
}

/*-------------------------------*/
/*				 */
/* SaveMsg ()			 */
/*				 */
/*-------------------------------*/
VOID SaveMsg (PSZ Buf)
{
  if (*(PULONG)Buf != 0x20535953) {  /* != 'SYS ' */
    CHAR c;

    while ((WritePtr < &MsgBufferEnd) && ((c = *(Buf++)) != 0))
      *(WritePtr++) = c;
  }
  if (WritePtr < &MsgBufferEnd) {
    *(WritePtr++) = 0x0D;
    *(WritePtr++) = 0x0A;
  }
}

void AssignSGList (void) {
  NPA	 npA;
  USHORT p;
  int i, j;

  i = (USHORT)ppDataSeg & (MR_2K_LIMIT - 1);
  p = (((USHORT)npAPool + i + (MR_2K_LIMIT - 1)) & ~(MR_2K_LIMIT - 1)) - i;
  for (i = 0; i < cAdapters; i++) {
    BOOL busmaster = FALSE;

    npA = ACBPtrs[i].npA;
    for (j = 0; j < npA->cUnits; j++)
      if (npA->UnitCB[j].Flags & (UCBF_BM_DMA | UCBF_FORCE))
	busmaster = TRUE;

    if (busmaster) {
      npA->pSGL  = (NPPRD)p;
      npA->ppSGL = ppDataSeg + p;
      p += MR_2K_LIMIT;
    }
  }
  npAPool = (NPVOID)p;
}


/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
VOID NEAR DriverInit (PRPINITIN pRPH)
{
  PRPINITIN	       pRPI = (PRPINITIN)  pRPH;
  PRPINITOUT	       pRPO = (PRPINITOUT) pRPH;
  NPA		       npA;
  PDDD_PARM_LIST       pDDD_Parm_List;
  PMACHINE_CONFIG_INFO pMCHI;
  PBYTE 	       pTimerPool;
  PSZ		       pCmdLine;
  PSEL		       Sel;

  /*ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
    ณSetup DevHelp service entry point for DHCALLS library ณ
    ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู */
  Device_Help = pRPH->DevHlpEP;
  DevHelp_GetDOSVar (DHGETDOSV_SYSINFOSEG, 0, (PPVOID)&Sel);
  SELECTOROF(pGlobal) = *Sel;
  msTimer = (PSHORT)&(pGlobal->msecs);
  pDDD_Parm_List = (PDDD_PARM_LIST) pRPI->InitArgs;
  WritePtr = (PCHAR)&WritePtr;
  OFFSETOF (WritePtr) = 0;
  DevHelp_VirtToPhys (WritePtr, (PULONG)&ppDataSeg);
  WritePtr = MsgBuffer;
  pMCHI = MAKEP (SELECTOROF(pDDD_Parm_List), (USHORT) pDDD_Parm_List->machine_config_table);
  if (pMCHI->BusInfo & BUSINFO_MCA) goto Deinstall;
  DevHelp_GetDOSVar (DHGETDOSV_DEVICECLASSTABLE, 1, (PPVOID)&pDriverTable);
  pCmdLine = MAKEP (SELECTOROF(pDDD_Parm_List), (USHORT)pDDD_Parm_List->cmd_line_args);

  {
    PSZ   p, pCmd;
    NPSZ  q = "R"VERSION;
    UCHAR c;
    USHORT w;

    pCmd = p = pCmdLine;

    while (*p == ' ') p++;
    w = *(PUSHORT)p & ~0x2020;
    p += 2;
    c = *p & ~0x20;
    if ((w == 0x4249) && (c == 'M'))
      ExportSCSI = FALSE;

    p = pCmd;
    while (*p > ' ') {
      c = *q;
      if (c == '\0')
	c = ' ';
      else
	q++;
      *(p++) = c;
    }
    SaveMsg (pCmd);
  }

  for (npA = AdapterTable; npA < (AdapterTable + MAX_ADAPTERS); npA++) {
    npA->FlagsT = ATBF_DISABLED;
    npA->UnitCB[0].npA	  = npA->UnitCB[1].npA	  = npA;
    npA->UnitCB[0].Status = npA->UnitCB[1].Status = UTS_NOT_PRESENT;
  }

  /*ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
    ณCheck the paramters on the config.sys line ณ
    ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู */
  if (ParseCmdLine (pCmdLine)) {
    sprntf (StringBuffer, ParmErrMsg, (USHORT)pcmdline1 - (USHORT)pCmdLine);
    Verbose++;
    TTYWrite (StringBuffer);
    Verbose--;
  }

  pTimerPool = (PBYTE) InitAllocate (TIMER_POOL_SIZE);
  ADD_InitTimer (pTimerPool, TIMER_POOL_SIZE);
  IODelayCount = Delay500 * 2;

#if PCITRACER
  outpw (TRPORT+2, 0xDEDE);
#endif
  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ Find ATAPI Units that have been identified ณ
  ณ and claim them.			       ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  ScanForLegacyFloppies();
  GetATAPIUnits();

Deinstall:

  if (Installed) {
    AssignSGList();
    PrintInfo (AdapterTable);
    *(WritePtr++) = 0x1A;

    pRPO->CodeEnd    = (USHORT)&_CodeEnd;
    pRPO->DataEnd    = (USHORT)npAPool - 1;
    pRPO->rph.Status = STDON;
  } else {
    ADD_DeInstallTimer();

    pRPO->CodeEnd    = 0;
    pRPO->DataEnd    = 0;
    pRPO->rph.Status = STDON + STERR + ERROR_I24_QUIET_INIT_FAIL;
  }
}

/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
NPBYTE NEAR InitAllocate (USHORT Size)
{
  NPBYTE p = (NPBYTE) -1;

  if (Size < ACBPoolAvail) {
    p = (NPBYTE) npAPool;

    clrmem (npAPool, Size);

    npAPool	 += Size;
    ACBPoolAvail -= Size;
  }
  return (p);
}


/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
VOID NEAR PrintInfo (NPA npA)
{
  NPU	npU;
  UCHAR i, j;

  TTYWrite (VersionMsg);

  for (j = 0; j < MAX_ADAPTERS ; j++, npA++) {
    if (!(npA->FlagsT & ATBF_DISABLED)) {
      sprntf (StringBuffer, VControllerInfo, j, npA->BasePort, MsgOk);
      TTYWrite (StringBuffer);

      npU = npA->UnitCB;
      for (i = 0; i < MAX_UNITS; i++, npU++) {
	if (npU->Status == UTS_OK) {
	  MsgLUN[5] = '1' + npU->MaxLUN;
	  sprntf (StringBuffer, VUnitInfo, i, MsgOk,
		  (npU->Flags & UCBF_PIO32     ? MsgPio32 : ""),
		  (npU->Flags & UCBF_BM_DMA    ? MsgDma   : ""),
		  (npU->Flags & UCBF_XLATECDB6 ? MsgCDB6  : ""),
		  (npU->MaxLUN > 0	       ? MsgLUN   : ""));
	  TTYWrite (StringBuffer);

	  sprntf (StringBuffer, VModelInfo, npU->ModelNum);
	  TTYWrite (StringBuffer);

	} else if (npU->Status == UTS_SKIPPED) {
	  sprntf (StringBuffer, VUnitInfo, i, MsgSkip, "", "", "");
	  TTYWrite (StringBuffer);
	}
      }
    }
  }
}

/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
VOID NEAR TTYWrite (NPSZ Buf)
{
  if (Verbose) {
    InitMsg.MsgStrings[0] = Buf;
    DevHelp_Save_Message ((NPBYTE) &InitMsg);
  }
  SaveMsg (Buf);
}

VOID NEAR InitIfInstalled (VOID)
{
  if (!Installed) {
    Installed = TRUE;

    DevHelp_RegisterDeviceClass (AdapterName, (PFN)&ATAPIADDEntry,
				 0, 1, &ADDHandle);
    RMCreateDriver (&DriverStruct, &hDriver);
  }
}

VOID NEAR RMDevice (NPA npA, UCHAR j)
{
  UCHAR      UnitId = npA->UnitId;
  NPU	     npU    = npA->npU;
  HANDLELIST HandleList;
  PSZ	     SearchKey = SearchKeytxt;
  HDRIVER    hDevice;

  HandleList.cMaxHandles = 1;
  SearchKey[4] = '0' + j;

  //ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  //ณ Find the IDE adapter in the resource tree ณ
  //ณ and attach a device			ณ
  //ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู

  if (!(RMKeyToHandleList (HANDLE_PHYS_TREE, SearchKey,
			   (PHANDLELIST)&HandleList))) {
    if (HandleList.cHandles) {
      DevStruct.DevDescriptName[6] = '0' + UnitId;
      DevStruct.DevDescriptName[8] = '\0';
      if (npU->ModelNum[0]) {
	_strncpy ((NPCH)DevStruct.DevDescriptName + 8, npU->ModelNum, 40);
	DevStruct.DevDescriptName[48] = '\0';
      }
      if (npA->npUactive[UnitId])
	DevStruct.DevType = npA->npUactive[UnitId]->pUI->UnitType;
      else
	DevStruct.DevType = UIB_TYPE_UNKNOWN;
      DevStruct.DevFlags = npU->Capabilities & UCBC_REMOVABLE ?
			   DS_REMOVEABLE_MEDIA : DS_FIXED_LOGICALNAME;
      RMCreateDevice (hDriver,
		      &hDevice,
		      &DevStruct,
		      HandleList.Handles[0],
		      NULL);
    }
  }
}

/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
VOID NEAR GetATAPIUnits()
{
  NPA			npA;
  NPU			npU;
  NPIORB_CONFIGURATION	pIORB;
  NPDEVICETABLE 	npDeviceTable;
  NPADAPTERINFO 	npAI;
  NPUNITINFO		npUI;
  NPRESOURCE_ENTRY	pRE;

  UCHAR 		hADD;
  UCHAR 		j, k, maxA, maxU;

  USHORT		Port_Flags, DMA_Flags;
  UCHAR 		ConfigACB = FALSE;

  npDeviceTable = (NPDEVICETABLE)ScratchBuf1;

  for (hADD = 1; hADD <= pDriverTable->DCCount; hADD++) {
    pIORB			 = (NPIORB_CONFIGURATION) &InitIORB;
    pIORB->iorbh.Length 	 = sizeof(IORB_CONFIGURATION);
    pIORB->iorbh.CommandCode	 = IOCC_CONFIGURATION;
    pIORB->iorbh.CommandModifier = IOCM_GET_DEVICE_TABLE;
    pIORB->pDeviceTable 	 = (PVOID) ScratchBuf1;
    pIORB->DeviceTableLen	 = sizeof(ScratchBuf1);

    clrmem (ScratchBuf1, sizeof (ScratchBuf1));

    if (Execute (hADD, (NPIORB)pIORB)) continue;

    if ((maxA = npDeviceTable->TotalAdapters) > MAX_ADAPTERS) maxA = MAX_ADAPTERS;
    for (j = 0, npA = AdapterTable; j < maxA; j++, npA++) {
      npAI = npDeviceTable->pAdapter[j];
      Port_Flags = DMA_Flags = 0;

      if (npA->FlagsT & ATBF_IGNORE) continue;
      if (!(npAI->AdapterDevBus & (AI_DEVBUS_ST506 | AI_DEVBUS_ST506_II)))
	continue;

      ConfigACB = TRUE;
      for (k = 0; k < npAI->AdapterUnits; k++) {
	npUI = &(npAI->UnitInfo[k]);

	if (npUI->UnitIndex >= MAX_UNITS) continue;
	npU = npA->UnitCB + npUI->UnitIndex;

	if (npU->Flags & UCBF_IGNORE) continue;

	if (npUI->UnitType == UIB_TYPE_ATAPI) {
	  InitIfInstalled();

	  if (ConfigACB) {
	    ConfigACB = FALSE;

	    ACBPtrs[cAdapters].npA = npA;

	    npA->IRQHandler	   = ACBPtrs[cAdapters].IRQHandler;
	    npA->Index		   = cAdapters;
	    npA->FlagsT 	  &= ~ATBF_DISABLED;
	    npA->SharedEP	   = *(PADDEntry FAR *)&(pDriverTable->DCTableEntries[hADD-1].DCOffset);
	    npA->SharedhADD	   = hADD;
	    npA->SharedhUnit	   = npUI->UnitHandle;

	    pRE = (NPRESOURCE_ENTRY) ScratchBuf;
	    clrmem (ScratchBuf, sizeof (ScratchBuf));

	    if (Issue_ReportResources (pRE, npUI->UnitHandle, hADD)) {
	      npA->FlagsT |= ATBF_DISABLED;
	      continue; 			 /* go to next adapter */
	    }

	    /* Is there an IRQ entry? */
	    /* Is there a port entry? */

	    if ((pRE->Max_Resource_Entry < RE_PORT) ||
		(!pRE->cIRQ_Entries)		    ||
		(!pRE->cPort_Entries)) {
	      npA->FlagsT |= ATBF_DISABLED;
	      continue; 			 /* go to next adapter */
	    }

	    npA->BasePort = pRE->npPort_Entry->StartPort;
	    Port_Flags	  = (pRE->npPort_Entry)[0].Port_Flags;

	    npA->StatusPort   = (pRE->npPort_Entry)[1].StartPort;
	    npA->HardwareType = (signed char)((pRE->npPort_Entry)[1].Port_Flags &0xFF);

	    if (pRE->cPort_Entries > 2) {
	      DMA_Flags = (pRE->npPort_Entry)[2].Port_Flags;
	      if (!(DMA_Flags & 1)) {
		UCHAR Data;

		if (DMA_Flags & 16) npA->FlagsT |= ATBF_SATA;

		npA->BMICOM = (pRE->npPort_Entry)[2].StartPort;
		npA->BMISTA = npA->BMICOM+2;
		npA->BMIDTP = npA->BMISTA+2;
		npA->BM_StopMask = (npA->HardwareType == ALi) ? 0 : BMICOM_TO_HOST;
		switch (npA->HardwareType) {
		  case Cyrix:  npA->SGAlign = 16 - 1; break;
		  case HPT37x: npA->SGAlign =  4 - 1; break;
		  default:     npA->SGAlign =  2 - 1;
		}

		Data = inp (BMSTATUSREG);
		npA->BMStatus = (Data != 0xFF) ? Data & 0xE0 : 0;
		if (DMA_Flags & 2) {  // DMA setup is valid
		  if (DMA_Flags & 4) npA->UnitCB[0].Flags |= UCBF_BM_DMA;
		  if (DMA_Flags & 8) npA->UnitCB[1].Flags |= UCBF_BM_DMA;
		} else {
		  if (npA->BMStatus & BMISTA_D0DMA) npA->UnitCB[0].Flags |= UCBF_BM_DMA;
		  if (npA->BMStatus & BMISTA_D1DMA) npA->UnitCB[1].Flags |= UCBF_BM_DMA;
		}
		npA->Flags |= ACBF_BM_DMA;
	      }
	    }

	    cAdapters++;
	    ConfigureACB (npA);
	  }

	  npA->npU = npU;
	  npU->UnitId = npA->UnitId = npUI->UnitIndex;
	  npU->hUnit  = npUI->UnitHandle;

	  if (npU->Status != UTS_SKIPPED) {
	    if (Port_Flags & (0x100 << npU->UnitId)) npU->Flags |= UCBF_PIO32;

	    if (npUI->UnitFlags & UF_FORCE) {
	      npU->Flags |= UCBF_FORCE;
	      if (npUI->UnitFlags & UF_NOTPRESENT) npU->Flags |= UCBF_NOTPRESENT;
	    }

	    if (!ClaimUnit (npA, npUI)) {
	      npA->cUnits = npU->UnitId + 1;
	      npU->Status = UTS_OK;
	      RMDevice (npA, j);
	    }
	  }
	}
      } // unit
    } // adapter
  } // driver
}

void AdaptToUnitType (NPU npU, NPUNITINFO npOldUnitInfo) {
  NPA	     npA = npU->npA;
  NPUNITINFO npUI;

  npU->pUI = npU->savedUI = npUI = (NPUNITINFO) InitAllocate (sizeof (UNITINFO));
  memcpy (npUI, npOldUnitInfo, sizeof (UNITINFO));

  npUI->FilterADDHandle = 0;
  npUI->UnitHandle = (USHORT)npU;
  npUI->UnitType   = npU->UnitType;
  npUI->UnitFlags  = UF_NOSCSI_SUPT;

  /*ออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
  บ Claim ATAPI Devices that are TYPE_DISK and LS-120 or ZIP_DRIVE  บ
  ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ*/
  if ((npU->UnitType == LS_120) || (npU->UnitType == ZIP_DRIVE)) {
    npUI->UnitType = UIB_TYPE_DISK;
    npUI->Reserved = 0xD512;	// prevent DaniN512.FLT to filter this unit

    /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
    ณ Setup LS_120 as Floppy if Legacy physical ณ
    ณ floppys not seen in the system		ณ
    ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
    if (npU->UnitType == LS_120) {
      USHORT timeout = (INIT_TIMEOUT_LONG * 3) / 1000L;
      // Longer default required for LS-120 device on read verify

      if (timeout > npA->IRQTimeOutMin) {
	npA->IRQTimeOutMin = timeout;
	npA->IRQTimeOutDef = (INIT_TIMEOUT_LONG * 3);
      }

      if (foundFloppy == 0) {
	npUI->UnitFlags |= (UF_A_DRIVE | UF_LARGE_FLOPPY);
	foundFloppy = 1;
      } else if (foundFloppy == 1) {
	if ((NewUnitInfoA.UnitFlags & (UF_A_DRIVE | UF_B_DRIVE)) == (UF_A_DRIVE | UF_B_DRIVE)) {
	  NewUnitInfoA.UnitFlags &= ~UF_B_DRIVE;
	  if (Remove_DriveFlag (0, &NewUnitInfoA)) {
	    foundFloppy = 2;
	  }
	}
	if (foundFloppy == 1) {
	  foundFloppy = 2;
	  npUI->UnitFlags |= (UF_B_DRIVE | UF_LARGE_FLOPPY);
	} else {
	  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
	  ณ We must become an X: drive & remain in Large Floppy Mode ณ
	  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
	  if (npU->Flags & UCBF_LARGE_FLOPPY)
	    npUI->UnitFlags |= UF_LARGE_FLOPPY;
	}
      } else {
	/*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
	ณ Will become X: drive & remain in Large Floppy Mode ณ
	ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
	if (npU->Flags & UCBF_LARGE_FLOPPY)
	  npUI->UnitFlags |= UF_LARGE_FLOPPY;
      }
    } else {  // UnitType == ZIP_DRIVE
      USHORT timeout = (INIT_TIMEOUT_LONG * 2) / 1000L;
      if (timeout > npA->IRQTimeOutMin) {
	npA->IRQTimeOutMin = timeout;
	npA->IRQTimeOutDef = (INIT_TIMEOUT_LONG * 2);
      }

//	if (ZIPtoA && (pGlobal->bootdrive == 1) && !(npU->Flags & UCBF_NOTPRESENT)) {
      if (ZIPtoA && !(npU->Flags & UCBF_NOTPRESENT)) {
	USHORT Mask = (ZIPtoA == 1) ? UF_A_DRIVE : UF_B_DRIVE;

	if (foundFloppy) {
	  if ((foundFloppy == 1) &&
	     ((NewUnitInfoA.UnitFlags & (UF_A_DRIVE | UF_B_DRIVE)) == (UF_A_DRIVE | UF_B_DRIVE))) {
	    NewUnitInfoA.UnitFlags &= ~Mask;
	    if (Remove_DriveFlag (0, &NewUnitInfoA))
	      foundFloppy = 2;
	  } else if (foundFloppy == 2) {
	    NewUnitInfoB.UnitFlags &= ~UF_B_DRIVE;
	    if (!Remove_DriveFlag (1, &NewUnitInfoB)) {
	      NewUnitInfoA.UnitFlags |=  UF_B_DRIVE;
	      NewUnitInfoA.UnitFlags &= ~UF_A_DRIVE;
	      if (!Remove_DriveFlag (0, &NewUnitInfoA))
		foundFloppy = 1;
	    }
	  }
	}
	if (foundFloppy != 2) {
	  npUI->UnitFlags |= Mask;
	}
	ZIPtoA = 0;
      }
    }
  } else if (npU->UnitType == UIB_TYPE_DISK) {
    USHORT timeout = (INIT_TIMEOUT_LONG * 2) / 1000L;
    if (timeout > npA->IRQTimeOutMin) {
      npA->IRQTimeOutMin = timeout;
      npA->IRQTimeOutDef = (INIT_TIMEOUT_LONG * 2);
    }
  } else if (npU->UnitType == UIB_TYPE_OPTICAL_MEMORY) {
    USHORT timeout = (INIT_TIMEOUT_LONG * 2) / 1000L;
    if (timeout > npA->IRQTimeOutMin) {
      npA->IRQTimeOutMin = timeout;
      npA->IRQTimeOutDef = (INIT_TIMEOUT_LONG * 2);
    }
  } else if (npU->UnitType == UIB_TYPE_CDROM) {
    npUI->UnitFlags |= UF_CHANGELINE | UF_NODASD_SUPT;
  } else {
    if (npU->UnitType == ONSTREAM_TAPE) npUI->UnitType = UIB_TYPE_TAPE;
    npUI->UnitFlags |= UF_NODASD_SUPT;
  }
  if (!(npU->Flags & UCBF_NOTRMV))
    npUI->UnitFlags |= UF_REMOVABLE;
}

void NEAR SetupUCB (NPU npU, PUSHORT Types) {
  npU->npUnext = NULL;
  npU->Flags |= UCBF_NOTPRESENT;
  npU->Flags &= ~UCBF_XLATECDB6;

  if (*Types & ATAPITYPE_ZIP) {
    *Types &= ~ATAPITYPE_ZIP;

    npU->UnitType     = ZIP_DRIVE;
    npU->Capabilities = UCBC_DIRECT_ACCESS_DEVICE | UCBC_REMOVABLE;
    npU->DrvDefFlags  = DDF_TESTUNITRDY_REQ;
    npU->MediaStatus  = MF_NOMED;
    npU->DeviceGEO.TotalSectors = 0x7783C;

  } else if (*Types & (ATAPITYPE_CDROM | ATAPITYPE_CDWRITER)) {
    if (*Types & ATAPITYPE_CDWRITER) {
      npU->Flags |= UCBF_CDWRITER;
      isCDWriter = npU;
    }

    *Types &= ~(ATAPITYPE_CDROM | ATAPITYPE_CDWRITER);

    npU->UnitType     = UIB_TYPE_CDROM;
    npU->Capabilities = UCBC_CDROM_DEVICE | UCBC_REMOVABLE;
    npU->Flags	     |= UCBF_XLATECDB6;

  } else if (*Types & ATAPITYPE_LS120) {
    *Types &= ~ATAPITYPE_LS120;

    npU->UnitType     = LS_120;
    npU->Capabilities = UCBC_DIRECT_ACCESS_DEVICE | UCBC_REMOVABLE;
    npU->DrvDefFlags  = DDF_LS120;
    npU->MediaType    = 0;
    npU->MediaStatus  = MF_NOMED;
    npU->DeviceGEO.TotalSectors = 0x3C300;

  } else if (*Types & ATAPITYPE_MO) {
    *Types &= ~ATAPITYPE_MO;

    npU->UnitType     = UIB_TYPE_OPTICAL_MEMORY;
    npU->Capabilities = UCBC_DIRECT_ACCESS_DEVICE | UCBC_REMOVABLE;
    npU->DrvDefFlags  = DDF_TESTUNITRDY_REQ;
    npU->MediaType    = 0;
    npU->MediaStatus  = MF_NOMED;
    npU->DeviceGEO.TotalSectors = 0x10000;

  } else if (*Types & ATAPITYPE_OTHER) {
    *Types &= ~ATAPITYPE_OTHER;

    npU->UnitType     = UIB_TYPE_UNKNOWN;
    npU->Capabilities = UCBC_REMOVABLE;
  }

  if (npU->Capabilities & UCBC_DIRECT_ACCESS_DEVICE) {
    npU->DeviceGEO.BytesPerSector = 512;
    RecalculateGeometry (&(npU->DeviceGEO));
  }
}

/*อออออออออออออออออออออออออออออออออป
บ ClaimUnit()			   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
/*
** With Resume = FALSE, issue IOCM_ALLOCATE_UNIT command to the device
** driver that identified the ATAPI unit.  The unit can be real or
** FORCEd but must be identified as an ATAPI device at power-on boot
** initialization. The IOCM_ALLOCATE_UNIT command is done only once at
** initialization time.  At APMResume(), Resume = TRUE, try to recognize
** and activate the drive that MAY now be present.  If UCBF_NOTPRESENT
** is set then unit is definitely not currently present.
** UCBF_NOTRPRESENT is always reset when this routine is called from
** APMResume().
**
** Normaly the device's registers are defined for FORCEd adapters but
** in the case of PCMCIA attached adapters, it is possible the registers
** may not yet be defined in AdapterTable.  In this case short circuit
** ATAPIIdentify() and set the UCBF_NOTPRESENT and return.
*/

USHORT NEAR ClaimUnit (NPA npA, NPUNITINFO npOldUnitInfo)
{
  USHORT    rc = 0;
  USHORT    UnitType, Types, UnitId;

  ULONG     OldIRQTimeOut;
  NPU	    npU, npUn, npUp;
  USHORT    l, num_retry;
  PUNITINFO pUI;

  npU	 = npA->npU;
  UnitId = npA->UnitId;
  Types  = npU->Types;

  npA->npUactive[UnitId] = NULL;
  npU->DriveHead = DEFAULT_ATAPI_DRV_SLCT_REG | (UnitId ? UNIT1 : UNIT0);

  /*
  ** PCMCIA adapters may not have registers defined yet.
  ** If not then the FORCEd unit is definitely not present.
  ** The registers will be defined during PCMCIA card insertion
  ** event procssing for this driver.
  */

  if (!npA->BasePort) npU->Flags |= UCBF_NOTPRESENT;

  if (npU->Flags & UCBF_NOTPRESENT) {
    npU->CmdPacketLength = 12;
    if (Types) {
      SetupUCB (npU, &Types);
      UnitType = npU->UnitType;
    } else {
      UnitType = UIB_TYPE_UNKNOWN;
    }
    strcpy (npU->ModelNum, Disconnect);
  } else {
    OldIRQTimeOut      = npA->IRQTimeOutDef;
    npA->IRQTimeOutDef = INIT_TIMEOUT_SHORT;

    UnitType = ATAPIIdentify (npA);

    if ((UnitType == 0xffff) && !(npU->Flags & UCBF_FORCE)) {
      /* If the ATAPIIdentify fail, we send the ATAPI Reset to recover it and
      * retry again. For Diamond 8x cdrom, sometimes it needs to retry many times
      * until there is no error. */
      for (num_retry = 0; num_retry < ATAPI_NUM_RETRIES; num_retry++) {
	ATAPIReset (npA);				/* Wait 15 seconds */
	for (l = 0; l < ATAPI_RESET_DELAY; l++) IODelay();

	UnitType = ATAPIIdentify (npA);
	if (UnitType != 0xffff) break;
      }
    }
    npA->IRQTimeOutDef = OldIRQTimeOut;
  }

  npU->UnitType = UnitType;

  switch (UnitType) {
    case UIB_TYPE_CDROM 	 : if (Types & ATAPITYPE_CDWRITER)
				     npU->Flags |= UCBF_CDWRITER;
				   Types &= ~(ATAPITYPE_CDROM | ATAPITYPE_CDWRITER);
				   break;
    case UIB_TYPE_OPTICAL_MEMORY : Types &= ~ATAPITYPE_MO; break;
    case UIB_TYPE_DISK:
    case ZIP_DRIVE		 : Types &= ~ATAPITYPE_ZIP  ; break;
    case LS_120 		 : Types &= ~ATAPITYPE_LS120; break;
  }

  isCDWriter = (npU->Flags & UCBF_CDWRITER) ? npU : NULL;

  if (!(npU->Flags & UCBF_NOTPRESENT))
    npA->npUactive[UnitId] = npU;

  npUp = npU;
  if ((npU->Flags & UCBF_FORCE) && Types) {
    npUn = (NPU) InitAllocate (sizeof (UCB));
    memcpy (npUn, npU, sizeof (UCB));

    if ((UnitType != UIB_TYPE_DISK) && (UnitType != ZIP_DRIVE) && (Types & ATAPITYPE_ZIP)) {
      SetupUCB (npU, &Types);
      if (!(npUn->Flags & UCBF_NOTPRESENT))
	npA->npUactive[UnitId] = npUn;
    } else {
      SetupUCB (npUn, &Types);
    }
    npUp->npUnext = npUn;
    npUn->Flags |= UCBF_PROXY;
    AdaptToUnitType (npUn, npOldUnitInfo);
    npUp = npUn;

    while (Types) {
      npUn = (NPU) InitAllocate (sizeof (UCB));
      memcpy (npUn, npU, sizeof (UCB));

      npUp->npUnext = npUn;
      npUn->Flags |= UCBF_PROXY;
      SetupUCB (npUn, &Types);
      AdaptToUnitType (npUn, npOldUnitInfo);
      npUp = npUn;
    }
  }
  AdaptToUnitType (npU, npOldUnitInfo);

  if (npU->MaxLUN > 0) {
    UCHAR LUN;
    NPCH  Buffer = IdentDataBuf[npA->Index];

    for (LUN = 1; LUN <= npU->MaxLUN; LUN++) {
      npUn = (NPU) InitAllocate (sizeof (UCB));
      memcpy (npUn, npU, sizeof (UCB));

      npUp->npUnext = npUn;
      npUn->Flags |= UCBF_PROXY;
      if (npUn->Capabilities & UCBC_DRIVE_LUN) npUn->DriveLUN.LUN = LUN;
      if (npUn->Capabilities & UCBC_CDB_LUN)   npUn->CDBLUN = LUN << 5;
      npU->LUN = LUN;
      GetSCSIInquiry (npUn);
      npUn->UnitType = Buffer[0] & 0x0F;
      Types = 1 << npUn->UnitType;
      SetupUCB (npUn, &Types);
      npU->Flags &= ~UCBF_NOTPRESENT;
      if (!(npUn->Flags & UCBF_XLATECDB6)) GetCDBTranslation (npUn);
      AdaptToUnitType (npUn, npOldUnitInfo);
      npUp = npUn;
    }
  }

  pUI = npU->pUI;
  pUI->FilterADDHandle = ADDHandle;
  if (ExportSCSI && (npU->Flags & UCBF_SCSI)) {
    pUI->UnitSCSITargetID &= 7;
    if (isCDWriter && (pUI->UnitSCSITargetID == 0))
      npA->SCSIOffset = 1;
    pUI->UnitSCSITargetID += npA->SCSIOffset;
  }

  if (pUI->UnitType != UIB_TYPE_ATAPI) {
    if (!(AllocDeallocChangeUnit (npA->SharedhADD, npU->hUnit, IOCM_ALLOCATE_UNIT, NULL))) {
      AllocDeallocChangeUnit (npA->SharedhADD, npU->hUnit, IOCM_CHANGE_UNITINFO, pUI);
    }
  } else {
    rc = 1;
  }

  ClaimUnitExit:

  return (rc);
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
USHORT NEAR Issue_ReportResources (NPRESOURCE_ENTRY pRE,
				   USHORT UnitHandle, UCHAR hADD)
{
  NPIORB_RESOURCE npIORB;

  npIORB			= (NPIORB_RESOURCE) &InitIORB;
  clrmem (npIORB, sizeof (InitIORB));

  npIORB->iorbh.Length		= sizeof(IORB_RESOURCE);
  npIORB->iorbh.UnitHandle	= UnitHandle;
  npIORB->iorbh.CommandCode	= IOCC_RESOURCE;
  npIORB->iorbh.CommandModifier = IOCM_REPORT_RESOURCES;
  npIORB->ResourceEntryLen	= sizeof(ScratchBuf);
  npIORB->pResourceEntry	= pRE;

  return (Execute (hADD, (NPIORB)npIORB));
}

/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
VOID NEAR ConfigureACB (NPA npA)
{
  UCHAR i;

  for (i = FI_PDAT; i <= FI_PCMD; i++) {
    npA->IOPorts[i] = npA->BasePort + i;
    npA->IORegs[i]  = 0;
  }

  if (npA->StatusPort == 0) npA->StatusPort = DRVHDREG | 0x300;

  DEVCTL = DEFAULT_ATAPI_DEVCON_REG;

  npA->IRQTimeOutDef	    = INIT_TIMEOUT_LONG;
  npA->IRQTimeOutMin	    = INIT_TIMEOUT_LONG / 1000L;
  npA->DelayedResetInterval = DELAYED_RESET_INTERVAL;

  DevHelp_AllocGDTSelector (&(npA->MapSelector), 1);
  npA->InternalCmd.IOSGPtrs.iPortAddress = npA->BasePort;
  npA->InternalCmd.IOSGPtrs.Selector	 = npA->MapSelector;
  npA->ExternalCmd.IOSGPtrs.iPortAddress = npA->BasePort;
  npA->ExternalCmd.IOSGPtrs.Selector	 = npA->MapSelector;
  npA->npCmdIO = &npA->ExternalCmd;

  npA->suspended = 1;

  npA->UnitCB[0].ModelNum = npA->ModelNum[0];
  npA->UnitCB[1].ModelNum = npA->ModelNum[1];

  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ Identify Data SGlist buffer for Geometry ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  IDDataSGList[npA->Index].ppXferBuf = ppIdentDataBuf[npA->Index]
    = ppDataSeg + (USHORT)&(IdentDataBuf[npA->Index]);
  SenseDataSGList[npA->Index].ppXferBuf = ppSenseDataBuf[npA->Index]
    = ppDataSeg + (USHORT)&(SenseDataBuf[npA->Index]);
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR ATAPIReset (NPA npA)
{
  UCHAR i;

  outp (DRVHDREG, npA->npU->DriveHead & (DEFAULT_ATAPI_DRV_SLCT_REG | UNIT1));
  IODelay();
  outp (COMMANDREG, FX_SOFTRESET);

  for (i = 9; --i > 0;)
    if (!CheckReady (npA)) break;
}

NPCH NEAR PrepBuffer (NPU npU, USHORT Len)
{
  NPCH Buffer;

  Buffer = IdentDataBuf[npU->npA->Index];
  IDDataSGList[npU->npA->Index].XferBufLen = Len;
  clrmem (Buffer, Len);
  return (Buffer);
}

NPIORB NEAR InitSendCDB (NPU npU, UCHAR CmdLen, NPCH Cmd) {
  NPIORB_ADAPTER_PASSTHRU npIORB = &InitIORB;

  clrmem (npIORB, sizeof (IORB_ADAPTER_PASSTHRU));

  npIORB->iorbh.Length		= sizeof(IORB_ADAPTER_PASSTHRU);
  npIORB->iorbh.UnitHandle	= (USHORT)npU;
  npIORB->iorbh.CommandCode	= IOCC_ADAPTER_PASSTHRU;
  npIORB->iorbh.CommandModifier = IOCM_EXECUTE_CDB;
  npIORB->ControllerCmdLen	= CmdLen;
  npIORB->pControllerCmd	= (PBYTE)Cmd;
  npIORB->cSGList		= 1;
  npIORB->pSGList		= IDDataSGList + npU->npA->Index;

  return ((NPIORB)npIORB);
}

USHORT NEAR ExecRepeated (NPIORB npIORB) {
  USHORT rc, ErrorCode = 0;

  do {
    rc = Execute (0, (NPIORB)npIORB);

  TR(rc);

    if (rc) {
      if (ErrorCode == rc) {
	return (ErrorCode);
      } else {
	ErrorCode = rc;
	if ((ErrorCode == IOERR_CMD_NOT_SUPPORTED) ||
	    (ErrorCode == IOERR_CMD_SYNTAX) ||
	    (ErrorCode == IOERR_CMD_ABORTED)) {
	  return (-1);
	}
      }
    } else {
      return (0);
    }
  } while (ErrorCode);

  return (ErrorCode);
}

USHORT NEAR ExecRepeated2 (NPIORB npIORB) {
  USHORT rc, ErrorCode = 0;

  do {
    rc = Execute (0, npIORB);

  TR(rc)

    if (rc) {
      if (ErrorCode == rc) {
	return (0);
      } else {
	ErrorCode = rc;
      }
    } else {
      return (0);
    }
  } while (ErrorCode);

  return (ErrorCode);
}

USHORT NEAR GetIdentifyData (NPU npU, NPIDENTIFYDATA npID)
{
  NPIORB_ADAPTER_PASSTHRU npIORB;

  npU->ReqFlags |= UCBR_IDENTIFY;

  IdentifySGList.ppXferBuf = ppDataSeg + (USHORT)npID;

  npIORB = (NPIORB_ADAPTER_PASSTHRU)InitSendCDB (npU, 0, NULL);
  npIORB->iorbh.CommandModifier = IOCM_EXECUTE_ATA;
  npIORB->pSGList		= &IdentifySGList;

  Execute (0, (NPIORB)npIORB);

  if ((npIORB->iorbh.Status & IORB_ERROR) || (npID->ModelNum[0] == 0)) {  // if failure, check if forced unit, if so set flag if
    if (npU->Flags & UCBF_FORCE) {   // not present
      npU->Flags |= UCBF_NOTPRESENT; // set fake flag
      return (TRUE);
    }
  }

  return (FALSE);
}

USHORT NEAR GetSCSIInquiry (NPU npU)
{
  static UCHAR CDB6[6] = {SCSI_INQUIRY, 0, 0, 0, sizeof (SCSI_INQDATA), 0};

  PrepBuffer (npU, sizeof (SCSI_INQDATA));
  return (Execute (0, InitSendCDB (npU, sizeof (CDB6), CDB6)));
}

USHORT NEAR GetTUR (NPU npU)
{
  static UCHAR CDB6[6] = {SCSI_TEST_UNIT_READY, 0, 0, 0, 0, 0};

  return (Execute (0, InitSendCDB (npU, sizeof (CDB6), CDB6)));
}


NPCH NEAR DoModeSense10 (NPU npU, UCHAR Page)
{
  static UCHAR CDB[10] = {SCSI_MODE_SENSE_10, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  NPCH	       Buffer;
  USHORT       Len, nextLen;

  CDB[2] = Page | 0x80;  // get default values
  Len = 0;
  do {
    *(NPUSHORT)(CDB+7) = Swap16 (Len + 2);
    Buffer = PrepBuffer (npU, Len + 2);
    if (ExecRepeated (InitSendCDB (npU, sizeof (CDB), CDB))) return (NULL);
    nextLen = Swap16 (*(NPUSHORT)Buffer);
    if (nextLen < 8) nextLen = 8;
    if (nextLen <= Len) break;
    Len = nextLen;
  } while (1);

  return (Buffer);
}

#if 1

VOID NEAR GetLUNMode (NPU npU, USHORT LastLUN)
{
  if (LastLUN <= 7) {
    npU->MaxLUN = LastLUN;
    npU->Capabilities |= UCBC_CDB_LUN;
  } // else some error in IDENTIFY data

  npU->DriveLUN.LUN = npU->CDBLUN = 0;
}

#else

VOID NEAR GetMaxLUN (NPU npU)
{
  NPCH Buffer;

  Buffer = DoModeSense10 (npU, 0x1B);
  if (Buffer && ((npU->MaxLUN = Buffer[3+8]) != 0))
    npU->MaxLUN = (npU->MaxLUN - 1) & 7;
}


VOID NEAR GetLUNMode (NPU npU, USHORT LastLUN)
{
  NPUSHORT Buffer;
  UCHAR    Type0, CurType;
  UCHAR    i, j, Mode = 0, minMode, maxMode, LUN;

  Buffer = (NPUSHORT)(IdentDataBuf[npU->npA->Index]);

  npU->MaxLUN = LastLUN;
  if (LastLUN > 7) npU->MaxLUN = 0;  // some error in IDENTIFY data

  GetSCSIInquiry (npU);

  Type0 = *Buffer & 0x1F;
  GetMaxLUN (npU);

  LUN = 0;
  minMode = 1; maxMode = 2;
  for (j = 4; j > 0; j >>= 1) {
    LUN |= j;
    for (i = minMode; i <= maxMode; i++) {
      npU->CDBLUN	= (i & 1) ? (LUN << 5) : 0;
      npU->DriveLUN.LUN = (i & 2) ?  LUN : 0;

      CurType = (IOERR_CMD_SYNTAX == GetSCSIInquiry (npU)) ? 0x1F : *Buffer & 0x1F;
      if ((CurType != 0x1F) && (IOERR_CMD_SYNTAX == GetTUR (npU))) CurType = 0x1F;
      if (CurType == 0x1F) {
	LUN &= ~j;
      } else {
	if (npU->MaxLUN == 0) GetMaxLUN (npU);
      }

      if (CurType != Type0) {
	minMode = maxMode = Mode = i;
	break;
      }
    }
  }
  if (npU->MaxLUN) {
    if (Mode == 0) Mode = 1;
  } else if (Mode != 0) {
    npU->MaxLUN = LUN;
  }
  npU->Capabilities &= ~(UCBC_DRIVE_LUN | UCBC_CDB_LUN);
  if (Mode & 1) npU->Capabilities |= UCBC_CDB_LUN;
  if (Mode & 2) npU->Capabilities |= UCBC_DRIVE_LUN;

  npU->DriveLUN.LUN = npU->CDBLUN = 0;
}

#endif

VOID NEAR GetCDBTranslation (NPU npU)
{
  static UCHAR CDB6[6] = {SCSI_MODE_SENSE, 8, 0, 0, 2, 0};

  IDDataSGList[npU->npA->Index].XferBufLen = 2;
  CDB6[2] = (npU->Capabilities & (UCBC_TAPE_DEVICE)) ? 0x2A : 0x3F;
  npU->Flags &= ~UCBF_XLATECDB6;

  if ((USHORT)-1 == ExecRepeated (InitSendCDB (npU, sizeof (CDB6), CDB6)))
    npU->Flags |= UCBF_XLATECDB6;
}

VOID NEAR GetGeometry (NPU npU, ULONG DefaultSize)
{
  NPIORB       npIORB;
  NPCH	       Buffer;
  static UCHAR ModeSense05[10] = {SCSI_MODE_SENSE_10, 8, 0x05, 0, 0, 0, 0, 0, 0x32+10, 0};
  static UCHAR ReadCapacity = FXP_READ_CAPACITY;
  USHORT       BpS;
  ULONG        TotalSectors;

  Buffer = 8 + PrepBuffer (npU, 0x32+10);
  npIORB = InitSendCDB (npU, sizeof (ModeSense05), ModeSense05);

  if ((USHORT)-1 != ExecRepeated (npIORB)) {
    npU->DeviceGEO.NumHeads	   = Buffer[4];
    npU->DeviceGEO.SectorsPerTrack = Buffer[5];
    npU->DeviceGEO.TotalCylinders  = Swap16 (*(NPUSHORT)(Buffer + 8));
    for (BpS = Swap16 (*(NPUSHORT)(Buffer + 6)); BpS > 512; BpS >>= 1)
      npU->DeviceGEO.TotalCylinders <<= 1;
    npU->DeviceGEO.BytesPerSector  = BpS;

    npU->DeviceGEO.TotalSectors    = (npU->DeviceGEO.TotalCylinders *
				      npU->DeviceGEO.NumHeads	    *
				      npU->DeviceGEO.SectorsPerTrack);
  }

  Buffer = PrepBuffer (npU, 8);
  InitSendCDB (npU, sizeof (ReadCapacity), &ReadCapacity);

  if (ExecRepeated (npIORB)) {
    TotalSectors = DefaultSize;
  } else {
    TotalSectors = Swap32 (*(ULONG *)(Buffer + 0)) + 1;
    for (BpS = Swap32 (*(ULONG *)(Buffer + 4)); BpS > 512; BpS >>=1)
      TotalSectors <<= 1;
  }
  if (TotalSectors > npU->DeviceGEO.TotalSectors) {
    npU->DeviceGEO.TotalSectors   = TotalSectors;
    npU->DeviceGEO.BytesPerSector = 512;
    RecalculateGeometry (&(npU->DeviceGEO));
  }
}

VOID NEAR GetCDCapabilities (NPU npU)
{
  NPCH Buffer;

  npU->Flags &= ~UCBF_CDWRITER;
  Buffer = DoModeSense10 (npU, 0x2A);
  if (Buffer && (Buffer[3+8] & 0x33)) npU->Flags |= UCBF_CDWRITER;
}

VOID NEAR InitOnstream (NPU npU)
{
  NPCH	       Buffer;
  static UCHAR ModeSel[6] = {SCSI_MODE_SELECT, 0x10, 0, 0, sizeof (OnstreamInit), 0};

  npU->Flags &= ~UCBF_XLATECDB6;
  Buffer = PrepBuffer (npU, sizeof (OnstreamInit));
  memcpy (Buffer, OnstreamInit, sizeof (OnstreamInit));

  ExecRepeated2 (InitSendCDB (npU, sizeof (ModeSel), ModeSel));
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ   ScanForLegacyFloppies()	   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
//
// Get the list of device drivers from the kernel.  Call each driver and
// ask for the Unit/Adapter information.  Collect the legacy floppy drive
// unit info and determine the number of physical floppy units present in
// the system.	Record the presence or absence of the flopp drives in the
// variables:
//	      A_Found
//	      B_Found
//
VOID NEAR ScanForLegacyFloppies()
{
  NPIORB_CONFIGURATION pIORB;
  NPDEVICETABLE        npDeviceTable;
  NPADAPTERINFO        npAI;
  NPUNITINFO	       npUI;
  UCHAR 	       hADD, hFilter;
  UCHAR 	       j, k;

  // Initialize default legacy floppy drive existence state.
  foundFloppy = 0;

  clrmem (&(NewUnitInfoA), sizeof(NewUnitInfoA));
  clrmem (&(NewUnitInfoB), sizeof(NewUnitInfoB));

  npDeviceTable = (NPDEVICETABLE)ScratchBuf1;

  for (hADD = 1; hADD <= pDriverTable->DCCount; hADD++) {
    clrmem (ScratchBuf1, sizeof (ScratchBuf1));

    pIORB = (NPIORB_CONFIGURATION)&InitIORB;
    clrmem (pIORB, sizeof (IORB_CONFIGURATION));
    pIORB->iorbh.Length 	 = sizeof(IORB_CONFIGURATION);
    pIORB->iorbh.CommandCode	 = IOCC_CONFIGURATION;
    pIORB->iorbh.CommandModifier = IOCM_GET_DEVICE_TABLE;
    pIORB->pDeviceTable 	 = (PVOID)ScratchBuf1;
    pIORB->DeviceTableLen	 = sizeof(ScratchBuf1);

    if (Execute (hADD, (NPIORB)pIORB)) continue;

    for (j = 0; j < npDeviceTable->TotalAdapters; j++) {
      npAI = npDeviceTable->pAdapter[j];
      if ((npAI->AdapterDevBus & 0x00FF) == AI_DEVBUS_FLOPPY) {
	npUI = npAI->UnitInfo;
	for (k = 0; k < npAI->AdapterUnits; k++, npUI++) {
	  if (npUI->UnitType == UIB_TYPE_DISK) {
	    if ((npUI->UnitFlags & (UF_NODASD_SUPT | UF_DEFECTIVE | UF_NOSCSI_SUPT | UF_REMOVABLE))
		== (UF_NOSCSI_SUPT | UF_REMOVABLE)) {
	      if ((foundFloppy == 0) && (npUI->UnitFlags & UF_A_DRIVE)) {
		NewUnitInfoA = *npUI;
		foundFloppy = 1;
	      }
	      else if ((foundFloppy == 1) && (npUI->UnitFlags & UF_B_DRIVE)) {
		NewUnitInfoB = *npUI;
		foundFloppy = 2;
	      } else
		continue;

	      // CALL ADD or Filter with IORB to allocate this unit.
	      hFilter = npUI->FilterADDHandle;
	      if (hFilter == 0) hFilter = hADD;
	      savedhADD[foundFloppy - 1] = hFilter;
	    }
	  }
	}
      }
    }
  }
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/

BOOL NEAR Remove_DriveFlag (USHORT Unit, NPUNITINFO pNewUnitInfo)
{
  BOOL rc = TRUE;

  if (savedhADD[Unit]) {
    if (AllocDeallocChangeUnit (savedhADD[Unit], pNewUnitInfo->UnitHandle,
				IOCM_ALLOCATE_UNIT, NULL)) {
      return (TRUE);
    } else {
      rc = AllocDeallocChangeUnit (savedhADD[Unit], pNewUnitInfo->UnitHandle,
				   IOCM_CHANGE_UNITINFO, pNewUnitInfo);
      AllocDeallocChangeUnit (savedhADD[Unit], pNewUnitInfo->UnitHandle,
			      IOCM_DEALLOCATE_UNIT, pNewUnitInfo);
    }
  }
  return (rc);
}

