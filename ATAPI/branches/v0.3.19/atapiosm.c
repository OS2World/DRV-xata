/**************************************************************************
 *
 * SOURCE FILE NAME = ATAPIOSM.C
 *
 * DESCRIPTION : OUTER STATE MACHINE for ATAPI driver
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 ***************************************************************************/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#define INCL_INITRP_ONLY
#include "os2.h"
#include "stddef.h"
#include "dskinit.h"

#include "iorb.h"
#include "addcalls.h"
#include "dhcalls.h"
#include "reqpkt.h"

#include "scsi.h"
#include "cdbscsi.h"
#include "ata.h"

#include "atapicon.h"
#include "atapireg.h"
#include "atapityp.h"
#include "atapiext.h"
#include "atapipro.h"

#pragma optimize(OPTIMIZE, on)

#define PCITRACER 0
#define TRPORT 0x9020

#if PCITRACER
#define TR(x) outp (TRPORT,(x));
#else
#define TR(x)
#endif

#if 0
void IBeep (USHORT freq) {
  USHORT i;
  DevHelp_Beep (freq, 10);
  for (i = 0xFFFF;i != 0; i--)
    IODelay ();
  }
#endif

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ  StartOSM			   บ
บ				   บ
บ  Outer State Machine Router	   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR StartOSM (NPA npA) {
  if (0 == saveINC (&(npA->OSMUseCount))) {
    do {
      do {
	npA->OSMFlags &= ~ACBOF_WAITSTATE;
	switch (npA->OSMState) {
	  case ACBOS_START_IORB:
	    StartIORB (npA);
	    break;

	  case ACBOS_ISM_COMPLETE:
	    ISMComplete (npA);
	    break;

	  case ACBOS_COMPLETE_IORB:
	    CompleteIORB (npA);
	    break;

	  case ACBOS_RESUME_COMPLETE:
	    Resume_Complete (npA);
	    break;

	  case ACBOS_SUSPEND_COMPLETE:
	    Suspend_Complete (npA);
	    break;

	  default:
	    break;
	}
      } while (!(npA->OSMFlags & ACBOF_WAITSTATE));
    } while (saveDEC (&(npA->OSMUseCount)));
  }
}

/* ออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR StartIORB (NPA npA)
{
  PIORB  pIORB = npA->pIORB;
  USHORT Cmd;
  NPU	 npU;

#define CmdCode (Cmd >> 8)
#define CmdModifier (UCHAR)(Cmd & 0xFF)

  if (SELECTOROF(pIORB)) {
    UCHAR CmdError = 0;

    Cmd = (pIORB->CommandCode << 8) | pIORB->CommandModifier;

    npA->IORBError  = 0;
    npA->TimerFlags = 0;
    npU 	    = (NPU) pIORB->UnitHandle;

#if PCITRACER
  outpw (TRPORT, Cmd);
//  outpw (TRPORT, (USHORT)npA);
  outpw (TRPORT, npU);
  outpw (TRPORT, npU->Flags);
  if (CmdCode == 8)
    if (CmdModifier == 3) {
      PPassThruATA picp = (PPassThruATA)((PIORB_ADAPTER_PASSTHRU)pIORB)->pControllerCmd;
      if (picp) outp (TRPORT+1,picp->TaskFileIn.Command);
//	outpw (TRPORT+1,npU->ReqFlags);
    } else if (CmdModifier == 2) {
      outp (TRPORT+1,((PIORB_ADAPTER_PASSTHRU)pIORB)->pControllerCmd[0]);
    }
#endif

    // The IORB requested timeout can override the default
    // timeout.
    if (pIORB->Timeout > npA->IRQTimeOutMin) {
      if (pIORB->Timeout > 100000UL)  /* prevent timer overruns */
	npA->IRQTimeOut = 100000000UL;
      else
	npA->IRQTimeOut = pIORB->Timeout * 1000UL;
    } else
      npA->IRQTimeOut = npA->IRQTimeOutDef;

    npA->npU	  = npU;
    npA->UnitId   = npU->UnitId;
    npA->ISMFlags = 0;
    npA->OSMState = ACBOS_COMPLETE_IORB;

    switch (Cmd) {
      case ((IOCC_CONFIGURATION << 8) | IOCM_GET_DEVICE_TABLE) : GetDeviceTable (npA); break;
      case ((IOCC_CONFIGURATION << 8) | IOCM_COMPLETE_INIT) :	 CompleteInit(); break;

      case ((IOCC_GEOMETRY << 8) | IOCM_GET_MEDIA_GEOMETRY)  : GetMediaGeometry (npA); break;
      case ((IOCC_GEOMETRY << 8) | IOCM_SET_MEDIA_GEOMETRY)  : break;
      case ((IOCC_GEOMETRY << 8) | IOCM_GET_DEVICE_GEOMETRY) : GetDeviceGeometry (npA); break;

      case ((IOCC_EXECUTE_IO << 8) | IOCM_READ) :
	    if (npU->Capabilities & UCBC_DIRECT_ACCESS_DEVICE)
	      ValidateAndSend (npA, CmdModifier);
	    break;
      case ((IOCC_EXECUTE_IO << 8) | IOCM_WRITE) :
      case ((IOCC_EXECUTE_IO << 8) | IOCM_READ_VERIFY) :
      case ((IOCC_EXECUTE_IO << 8) | IOCM_WRITE_VERIFY) :
	    if (npU->Capabilities & UCBC_DIRECT_ACCESS_DEVICE)
	      ValidateAndSend (npA, CmdModifier);
	    else
	      CmdError = 1;
	    break;

      case ((IOCC_FORMAT << 8) | IOCM_FORMAT_TRACK) :
	    if (npU->Capabilities & UCBC_DIRECT_ACCESS_DEVICE)
	      Format_Track (npA);
	    else
	      CmdError = 1;
	    break;

      case ((IOCC_UNIT_STATUS << 8) | IOCM_GET_UNIT_STATUS) :
      case ((IOCC_UNIT_STATUS << 8) | IOCM_GET_MEDIA_SENSE):  UnitStatusMediaSense (npA, CmdModifier); break;
      case ((IOCC_UNIT_STATUS << 8) | IOCM_GET_LOCK_STATUS) : GetLockStatus (npA); break;

      case ((IOCC_DEVICE_CONTROL << 8) | IOCM_ABORT) :
      case ((IOCC_DEVICE_CONTROL << 8) | IOCM_RESET) :
      case ((IOCC_DEVICE_CONTROL << 8) | IOCM_SUSPEND) :
      case ((IOCC_DEVICE_CONTROL << 8) | IOCM_RESUME) :
      case ((IOCC_DEVICE_CONTROL << 8) | IOCM_GET_QUEUE_STATUS) : break;
      case ((IOCC_DEVICE_CONTROL << 8) | IOCM_LOCK_MEDIA) :
      case ((IOCC_DEVICE_CONTROL << 8) | IOCM_UNLOCK_MEDIA) :
      case ((IOCC_DEVICE_CONTROL << 8) | IOCM_EJECT_MEDIA) :   ValidateAndSend (npA, (UCHAR)(CmdModifier + LOCKUNLOCKEJECT)); break;

      case ((IOCC_ADAPTER_PASSTHRU << 8) | IOCM_EXECUTE_ATA) : SetupAndExecute (npA, SAE_EXECUTE_ATA); break;
      case ((IOCC_ADAPTER_PASSTHRU << 8) | IOCM_EXECUTE_CDB) : SetupAndExecute (npA, SAE_EXECUTE_CDB); break;

      default: CmdError = 1;
    }

    if (CmdError) SetCmdError (npA);
  } else {
    npA->OSMFlags |= ACBOF_WAITSTATE; /* How did we get here? */
  }
}

USHORT NEAR _fastcall GetSegmentLimit (SEL s)
{
  _asm { LSL AX, s }
}

USHORT NEAR ProcessSense (NPA npA)
{
  PSCSI_STATUS_BLOCK pSCSISB;
  PIORB pIORB = npA->pIORB;

  if (npA->OSMFlags & ACBOF_SENSE_DATA_ACTIVE) {
  TR(0x36)
    npA->OSMFlags    &= ~ACBOF_SENSE_DATA_ACTIVE;
    npA->OSMReqFlags &= ~ACBR_SENSE_DATA;

    if (0 == npA->IORBError) ProcessSenseData (npA);

    /*ออออออออออออออออออออออออออออออออออออออออออออป
    บ pass sense data back to caller if requested บ
    ศออออออออออออออออออออออออออออออออออออออออออออ*/
    if (pIORB->RequestControl & IORB_REQ_STATUSBLOCK) {
      USHORT SenseLen, Limit;

      pSCSISB = MAKEP (SELECTOROF (pIORB), (NPBYTE)(pIORB->pStatusBlock));
      Limit = GetSegmentLimit (SELECTOROF (pSCSISB->SenseData))
	    -		       OFFSETOF   (pSCSISB->SenseData);
      SenseLen = pSCSISB->ReqSenseLen;
      if (SenseLen > Limit) SenseLen = Limit;
      if (SenseLen > SENSE_DATA_BYTES) SenseLen = SENSE_DATA_BYTES;
      memcpy (pSCSISB->SenseData, SenseDataBuf[npA->Index], SenseLen);
      pSCSISB->Flags  = STATUS_SENSEDATA_VALID;
      pIORB->Status  |= IORB_STATUSBLOCK_AVAIL;
    }
  } else if (npA->OSMReqFlags & ACBR_SENSE_DATA) {
    return (TRUE);
  }
  return (FALSE);
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ ISMComplete() 		   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR ISMComplete (NPA npA)
{
  NPU npU = npA->npU;

  TR(17)

  npA->OSMFlags &= ~ACBOF_WAITSTATE;

  /*อออออออออออออออออออออออออออออออออออออออออป
  บ Is a Test Unit Ready in Progress ?	     บ
  ศอออออออออออออออออออออออออออออออออออออออออ*/
  if (npA->OSMReqFlags & ACBR_TEST_UNIT_RDY) {

  TR(0x32)

    if (npA->OSMFlags & ACBOF_SENSE_DATA_ACTIVE) {
      ProcessSense (npA);
      ProcessTURSense (npA);

    } else if (npA->OSMReqFlags & ACBR_SENSE_DATA) {
      goto DoSenseData;

    } else {							/* SUCCESS */
      npU->MediaStatus &= ~(MF_MC | MF_NOMED);
      npA->OSMState	= ACBOS_START_IORB;
    }
  }

  /*ออออออออออออออออออออออออออออออออออออออออออออออออออออป
  บ Is this an read capacity request (GetMediaGeometry) บ
  ศอออออออออออออออออออออออออออออออออออออออออออออออออออออ*/
  else if (npA->OSMFlags & ACBOF_READCAPACITY_ACTIVE) {

  TR(0x31)

    if (ProcessSense (npA)) goto DoSenseData;

    npA->OSMFlags &= ~ACBOF_READCAPACITY_ACTIVE;
    ProcessCapacity (npA);
    npA->OSMState = ACBOS_COMPLETE_IORB;

  } else if (npA->OSMFlags & ACBOF_MODESENSE_ACTIVE) {

  TR(0x34)

    if (ProcessSense (npA)) goto DoSenseData;

    npA->OSMFlags &= ~ACBOF_MODESENSE_ACTIVE;
    ProcessCDBModeSense (npA);
    npA->OSMState = ACBOS_COMPLETE_IORB;

  } else if (npA->OSMFlags & ACBOF_INQUIRY_ACTIVE) {

  TR(0x35)

    if (ProcessSense (npA)) goto DoSenseData;

    npA->OSMFlags &= ~ACBOF_INQUIRY_ACTIVE;
    ProcessCDBInquiry (npA);
    npA->OSMState = ACBOS_COMPLETE_IORB;
  }

  /*อออออออออออออออออออออออออออออออออออออออออป
  บ is an internal request sense in progress บ
  ศอออออออออออออออออออออออออออออออออออออออออ*/
  else if (npA->OSMFlags & ACBOF_SENSE_DATA_ACTIVE) {

  TR(0x33)

    ProcessSense (npA);

    if (!(npA->OSMReqFlags & ACBR_RESET)) {
      if (npA->OSMReqFlags & ACBR_DISCARD_ERROR) {
	npA->OSMReqFlags &= ~ACBR_DISCARD_ERROR;
      } else {
	ProcessSenseData (npA);
      }
      npA->OSMState = ACBOS_COMPLETE_IORB;
    }
  }

  /*ออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
  บ Is this an internal Identify request (GetMediaGeometry) บ
  ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออ*/
  else if (npA->OSMFlags & ACBOF_IDENTIFY_ACTIVE) {
    npA->OSMFlags &= ~ACBOF_IDENTIFY_ACTIVE;

  TR(0x30)

    ProcessIdentify (npA);
    npA->OSMState = ACBOS_COMPLETE_IORB;
  }

  /*ออออออออออออออออออออออออออออออออออออออออออออป
  บ	was a reset request successful		บ
  ศออออออออออออออออออออออออออออออออออออออออออออ*/
  else if (npA->OSMFlags & ACBOF_RESET_ACTIVE) {
    npA->OSMFlags &= ~ACBOF_RESET_ACTIVE;
    npU->ReqFlags &= ~UCBR_RESET;		     /* Clear Flags */
    npA->ISMFlags &= ~ACBIF_ATA_OPERATION;

  TR(18)

    if (BSYWAIT(npA)) { 		       /* successful reset */

  TR(19)

      npA->OSMReqFlags &= ~ACBR_RESET;
      npA->OSMState	= ACBOS_START_IORB;
      npA->IORBStatus	= 0;
      npA->IORBError	= 0;
    } else {

  TR(20)

      npA->OSMState = ACBOS_ISM_COMPLETE;
    }
  }

  /*ออออออออออออออออออออออออออออออออออออออออออออป
  บ	was there a request for a reset ?	บ
  ศออออออออออออออออออออออออออออออออออออออออออออ*/
  else if (npA->OSMReqFlags & ACBR_RESET) {

  TR(21)

    cResets++;
    npA->cResetRequests++;

    /*ออออออออออออออออออออออออออออออออออออออป
    บ if over the reset limit, return error บ
    ศออออออออออออออออออออออออออออออออออออออ*/
    if (npA->cResetRequests > MAXRESETS) {
      npA->IORBError	= IOERR_DEVICE_NONSPECIFIC;
      npA->OSMReqFlags &= ~ACBR_RESET;
      npA->OSMState	= ACBOS_COMPLETE_IORB;
    } else {
      /*อออออออออออออออออออออออออออป
      บ   Issue Reset request	   บ
      ศอออออออออออออออออออออออออออ*/
      SetupAndExecute (npA, SAE_RESET);
    }
  }

  /*ออออออออออออออออออออออออออออออออออออป
  บ was there a request for sense data	บ
  ศออออออออออออออออออออออออออออออออออออ*/
  else if (npA->OSMReqFlags & ACBR_SENSE_DATA) {
DoSenseData:

  TR(22)

    SetupAndExecute (npA, SAE_REQUESTSENSE);
  } else {

  TR(23)

    npA->OSMState = ACBOS_COMPLETE_IORB;
  }
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR CompleteIORB (NPA npA)
{
  PIORB  pIORB;
  USHORT ErrorCode = 0;
  NPU	 npU;

  TR(24)

  pIORB     = npA->pIORB;
  npU	    = npA->npU;
  ErrorCode = npA->IORBError;

  if (npA->TimerFlags)
    ErrorCode = IOERR_UNIT_NOT_READY;

  if (ErrorCode) {
    pIORB->ErrorCode  = ErrorCode;
    pIORB->Status    |= IORB_ERROR;
  }

#if PCITRACER
  outpw (TRPORT, pIORB->Status);
  outpw (TRPORT, pIORB->ErrorCode);
#endif

  /*ออออออออออออออออออออออออป
  บ	 Mark it DONE	    บ
  ศออออออออออออออออออออออออ*/
  pIORB->Status |= IORB_DONE;

  npA->pIORB = 0;

  if (pIORB->RequestControl & IORB_ASYNC_POST) {
    NPCMDIO npCmdIO = npA->npCmdIO;

    if ((npU->Capabilities & UCBC_SPEC_REV_17B)    &&
	(npCmdIO->ATAPIPkt[0] == SCSI_MODE_SENSE_10) &&
	((npCmdIO->ATAPIPkt[2] & REV_17B_PAGE_CODE_MASK) == REV_17B_PAGE_CAPABILITIES))
      Convert_17B_to_ATAPI (npCmdIO->IOSGPtrs.pSGList->ppXferBuf);

    (*pIORB->NotifyAddress)(pIORB);
  }

  DISABLE

  if (npA->ResourceFlags & ACBRF_CURR_OWNER) {
    if (npA->pHeadIORB) {			       /* more stuff to do */
      if (QueryOtherAdd (npA)) {	   /* other add has stuff in queue */
	npA->OSMState	    = ACBOS_RESUME_COMPLETE;
	npA->OSMFlags	   |= ACBOF_WAITSTATE;
	npA->ResourceFlags &= ~ACBRF_CURR_OWNER;
	ENABLE
	SuspendResumeOtherAdd (npA, IOCM_RESUME);
      } else {				      /* other add has empty queue */
	npA->OSMState = ACBOS_START_IORB;

	NextIORB (npA);      /* we said above we had stuff to */
			       /* do and now we don't ? */
      }
    } else {				   /* no more work to do */
      npA->OSMState	    = ACBOS_RESUME_COMPLETE;
      npA->OSMFlags	   |= ACBOF_WAITSTATE;
      npA->ResourceFlags &= ~ACBRF_CURR_OWNER;
      ENABLE
      SuspendResumeOtherAdd (npA, IOCM_RESUME);
    }
  } else {
    npA->OSMState = ACBOS_START_IORB;

    if (NextIORB (npA)) {			/* DISABLE is in NextIORB */
      npA->OSMFlags |= ACBOF_WAITSTATE;
      npA->OSMFlags &= ~ACBOF_SM_ACTIVE;
    }
  }
  ENABLE

  TR(30)
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ ProcessTURSense()		   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR ProcessTURSense (NPA npA)
{
  UCHAR ASC;

  npA->OSMState = ACBOS_START_IORB;
  ASC = SenseDataBuf[npA->Index][offsetof (SCSI_REQSENSE_DATA, AddSenseCode)];

  if (SetMediaFlags (npA, ASC)) {
    npA->OSMReqFlags &= ~ACBR_TEST_UNIT_RDY;	   /* Turn off TUR on Error */
    npA->IORBError = (ASC > MaxAddSenseDataEntry) ? IOERR_ADAPTER_REFER_TO_STATUS
						  : AddSenseDataMap[ASC];
    npA->OSMState = ACBOS_COMPLETE_IORB;
  }
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ ProcessSenseData()		   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR ProcessSenseData (NPA npA)
{
  UCHAR ASC;

  ASC = SenseDataBuf[npA->Index][offsetof (SCSI_REQSENSE_DATA, AddSenseCode)];
  SetMediaFlags (npA, ASC);
  npA->IORBError = (ASC > MaxAddSenseDataEntry) ? IOERR_ADAPTER_REFER_TO_STATUS
						: AddSenseDataMap[ASC];
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
UCHAR NEAR SetMediaFlags (NPA npA, UCHAR ASC)
{
  NPU npU = npA->npU;

  switch (ASC) {
    case ASC_MEDIUM_NOT_PRESENT:
      npU->MediaStatus &= ~MF_MC;
      npU->MediaStatus |= MF_NOMED;
      break;

    case ASC_MEDIA_CHANGED:	   // called once after media insertion
      npU->MediaStatus &= ~MF_NOMED;
      npU->MediaStatus |= MF_MC;
      break;

    case ASC_WRITE_PROTECTED_MEDIA:
      npU->MediaStatus |= MF_WRT_PT;
      break;

    default:
      return (TRUE);
  }
  return (FALSE);
}


/* ออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR Resume_Complete (NPA npA)
{
  DISABLE
  npA->suspended = 1;

  npA->OSMFlags |= ACBOF_WAITSTATE;

  if (NextIORB (npA)) {
    npA->OSMState  = ACBOS_START_IORB;
    npA->OSMFlags &= ~ACBOF_SM_ACTIVE;
  } else {
    npA->OSMState  = ACBOS_SUSPEND_COMPLETE;
    ENABLE

    SuspendResumeOtherAdd (npA, IOCM_SUSPEND);
  }
}

/* ออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR Suspend_Complete (NPA npA)
{
  DISABLE
  npA->suspended      = 0;
  npA->OSMState       = ACBOS_START_IORB;
  npA->OSMFlags      &= ~ACBOF_WAITSTATE;
  npA->ResourceFlags |= ACBRF_CURR_OWNER;
  ENABLE
}

/* ออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
USHORT NEAR NextIORB (NPA npA)
{
  DISABLE
  if (npA->pIORB = npA->pHeadIORB) {
    if	(!(npA->pHeadIORB = npA->pIORB->pNxtIORB)) {
      npA->pFootIORB = 0;
    }
  }

  return ((npA->pIORB) ? 0 : 1);
}

/*อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
บ			 IOCC_CONFIGUARTION Functions			       บ
ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
บ				   บ
ศออออออออออออออออออออออออออออออออ */
VOID NEAR CompleteInit (VOID)
{
  UCHAR i;
  NPA	npA;
  NPU	npU;

  InitIOComplete = TRUE;

  for (i = 0; i < cAdapters; i++) {
    npA = ACBPtrs[i].npA;
    npU = npA->UnitCB;

    for (npU = npA->UnitCB; npU < (npA->UnitCB + MAX_UNITS); npU++) {
      if ((npU->Flags & (UCBF_CDWRITER | UCBF_RSJ)) == (UCBF_CDWRITER | UCBF_RSJ)) {
	AllocDeallocChangeUnit (npA->SharedhADD, npU->hUnit, IOCM_CHANGE_UNITINFO,
				npU->savedUI);
	npU->Flags &= ~UCBF_ALLOCATED;
      }
    }
  }
}

VOID NEAR GetDeviceTable (NPA npA)
{
  PIORB_CONFIGURATION pIORB = (PIORB_CONFIGURATION) npA->pIORB;

  USHORT	RequiredLength;
  UCHAR 	i, j, CurrentAdapter, CurrentUnit;
  NPA		npAP;
  NPU		npU;
  PDEVICETABLE	pDT;
  NPADAPTERINFO npADPT;
  NPUNITINFO	npUI;
  PUSHORT	pA;

  pDT = pIORB->pDeviceTable;
  pA  = (PUSHORT)&(pDT->pAdapter[0]);
  npADPT = (NPADAPTERINFO)ScratchBuf;
  CurrentAdapter = 0;

  for (i = 0; i < cAdapters; i++) {
    npAP = ACBPtrs[i].npA;

    *pA = (USHORT)npADPT;

    clrmem (npADPT, sizeof (ADAPTERINFO));
    memcpy (npADPT->AdapterName, AdapterName, sizeof (AdapterName));
    npADPT->AdapterIOAccess = AI_IOACCESS_PIO;
    npADPT->AdapterHostBus  = AI_HOSTBUS_ISA | AI_BUSWIDTH_16BIT;
    npADPT->AdapterFlags    = AF_16M | AF_HW_SCATGAT;
    npADPT->AdapterDevBus   = AI_DEVBUS_SCSI_2 | AI_DEVBUS_16BIT;
    npADPT->AdapterSCSITargetID = 7;
    npADPT->AdapterSCSILUN  = 0;

    npUI = &npADPT->UnitInfo[0];
    CurrentUnit = 0;
    for (j = 0; j < MAX_UNITS; j++) {
      npU = &npAP->UnitCB[j];
      if (npU->Status != UTS_OK) continue;
      npU = npU->npUnext;

      while (npU != NULL) {
	memcpy (npUI, npU->pUI, sizeof (UNITINFO));
	npUI->AdapterIndex     = CurrentAdapter;
	npUI->UnitIndex        = CurrentUnit;
	npUI->UnitSCSITargetID = npU->UnitId;
	npUI->UnitSCSILUN      = npU->LUN;
	CurrentUnit++;
	npUI++;
	npU = npU->npUnext;
      }
    }
    if (CurrentUnit == 0) continue;

    npADPT->AdapterUnits = CurrentUnit;
    CurrentAdapter++;
    pA++;
    npADPT = (NPADAPTERINFO)npUI;
  }

  RequiredLength = sizeof (DEVICETABLE) + (CurrentAdapter - 1) * sizeof (NPADAPTERINFO)
		 + ((USHORT)npADPT - (USHORT)ScratchBuf);

  if (pIORB->DeviceTableLen < RequiredLength) {
    npA->IORBError = IOERR_CMD_SYNTAX;
  } else {
    USHORT Offset;

    pDT->ADDLevelMajor = ADD_LEVEL_MAJOR;
    pDT->ADDLevelMinor = ADD_LEVEL_MINOR;
    pDT->ADDHandle     = ADDHandle;
    pDT->TotalAdapters = CurrentAdapter;

    memcpy (pA, ScratchBuf, (USHORT)npADPT - (USHORT)ScratchBuf);
    Offset = (USHORT)pA - (USHORT)ScratchBuf;
    for (i = 0; i < CurrentAdapter; i++)
      *(--pA) += Offset;
//*(NPUSHORT)(ScratchBuf1-2) = RequiredLength;
//memcpy (ScratchBuf1, pDT, RequiredLength);
  }
}

/*อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
บ			 IOCC_UNIT_CONTROL Functions			       บ
ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ*/

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR GetLockStatus (NPA npA)
{
  ((PIORB_UNIT_STATUS)npA->pIORB)->UnitStatus = 0x03; /* Eject, Lock, Unlock supported, */
						      /* but no lock status		*/
}

/*อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
บ			   IOCC_GEOMETRY Functions			       บ
ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ*/
/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
บ GetMediaGeometry()		   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR GetMediaGeometry (NPA npA)
{
  NPU npU = npA->npU;

  if (!(npU->Capabilities & UCBC_DIRECT_ACCESS_DEVICE)) return;

  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ Issue Test Unit Ready on the very first access  ณ
  ณ to the unit or if required by the device	    ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  if ((npU->DrvDefFlags & DDF_TESTUNITRDY_REQ)	||
    (!(npU->DrvDefFlags & DDF_1ST_TUR_4_GMG))) {
    if (! (npA->OSMReqFlags & ACBR_TEST_UNIT_RDY)) {
      SetupAndExecute (npA, SAE_TESTUNITREADY);
      return;
    }
    npU->DrvDefFlags |=  DDF_1ST_TUR_4_GMG;
    npA->OSMReqFlags &= ~ACBR_TEST_UNIT_RDY;
  }

  if (npU->MediaStatus & MF_NOMED) {
    PIORBH pIORB = npA->pIORB;
    *((PIORB_GEOMETRY)pIORB)->pGeometry = npU->DeviceGEO;
    return;
  }

  TR(0x40) TR(npU->UnitType)

  if (npU->DrvDefFlags & DDF_LS120) {

  TR(0x42)

    /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
    ณ Issue Identify Data ณ
    ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
    SetupAndExecute (npA, SAE_INTERNAL_IDENTIFY);
  } else {

  TR(0x41)

    SetupAndExecute (npA, SAE_READ_CAPACITY);
  }
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
บ GetDeviceGeometry()		   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR GetDeviceGeometry (NPA npA)
{
  NPU npU = npA->npU;

  if (!(npU->Capabilities & UCBC_DIRECT_ACCESS_DEVICE)) return;

  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ Get Geometry based on Drive definitionณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  *(((PIORB_GEOMETRY)(npA->pIORB))->pGeometry) = npU->DeviceGEO;
}


/*อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
บ			  IOCC_EXECUTE_IO Functions			       บ
ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ*/
/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR ValidateAndSend (NPA npA, UCHAR command)
{
  NPU npU = npA->npU;

  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ Check Media in the drive	  ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  if (MediaInvalid4Drive (npA)) return;

  switch (command) {
    case IOCM_READ:
      SetupAndExecute (npA, SAE_READ);
      break;

    case IOCM_READ_VERIFY:
      SetupAndExecute (npA, SAE_READ_VERIFY);
      break;

    case IOCM_WRITE:
      SetupAndExecute (npA, SAE_WRITE);
      break;

    case IOCM_WRITE_VERIFY:
      SetupAndExecute (npA, SAE_WRITE_VERIFY);
      break;

    case (IOCM_LOCK_MEDIA + LOCKUNLOCKEJECT):
      SetupAndExecute (npA, SAE_LOCK_MEDIA);
      break;

    case (IOCM_UNLOCK_MEDIA + LOCKUNLOCKEJECT):
      SetupAndExecute (npA, SAE_UNLOCK_MEDIA);
      break;

    case (IOCM_EJECT_MEDIA + LOCKUNLOCKEJECT):
      npU->MediaStatus &= ~MF_MC;
      npU->MediaStatus |= MF_NOMED;
      SetupAndExecute (npA, SAE_EJECT_MEDIA);
      break;

    default:
      SetCmdError (npA);
   }
}

/*อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
บ			  IOCC_FORMAT Functions 			       บ
ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ*/
/* ออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR Format_Track (NPA npA)
{
  SetupAndExecute (npA, SAE_FORMAT_TRACK);
}

/*อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
บ			 IOCC_UNIT_STATUS Functions			       บ
ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ*/
/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR UnitStatusMediaSense (NPA npA, UCHAR command)
{
  PIORB_UNIT_STATUS pIORB = (PIORB_UNIT_STATUS) npA->pIORB;
  NPU		    npU   = npA->npU;
  USHORT	    mediatype;

  pIORB->UnitStatus = 0; US_POWER;

  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ Issue Test Unit Ready ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  if (!(npA->OSMReqFlags & ACBR_TEST_UNIT_RDY)) {
    SetupAndExecute (npA, SAE_TESTUNITREADY);
    return;
  }

  npA->OSMReqFlags &= ~ACBR_TEST_UNIT_RDY;

  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ If Media Changed, then we should return the error ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  if (npU->MediaStatus & MF_MC)
    npA->IORBError = IOERR_MEDIA_CHANGED;

  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ Media is NOT present in unit ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  else if (npU->MediaStatus & MF_NOMED) {
    if (command == IOCM_GET_MEDIA_SENSE)
      npA->IORBError = IOERR_MEDIA_NOT_PRESENT;
    else
      npA->IORBError = IOERR_UNIT_NOT_READY;

  } else {
    /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
    ณ Media is Present in the Driveณ
    ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
    if (command == IOCM_GET_MEDIA_SENSE) {
      mediatype = (npU->MediaType & 0x30);

      if (mediatype == UHD_MEDIUM)
	pIORB->UnitStatus = US_MEDIA_LARGE_FLOPPY;	   /* > 2.88 MB */
      else if (mediatype == HD_MEDIUM)
	pIORB->UnitStatus = US_MEDIA_144MB;		   /* 144 MB */
      else if (mediatype == DD_MEDIUM)
	pIORB->UnitStatus = US_MEDIA_720KB;		   /* 720 KB */
    } else {
       /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
       ณ Media is present in the drive = US_READY  ณ
       ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
      pIORB->UnitStatus = US_READY | US_POWER;
    }
  }
}

VOID NEAR ChangeUnitStatus (NPU npU, PIORB_CHANGE_UNITSTATUS pIORB)
{
  NPA	 npA	= npU->npA;
  USHORT UnitId = npU->UnitId;

  npU = npA->npU = npA->UnitCB + UnitId;

  while (npU) {
    npU->Flags |= UCBF_NOTPRESENT;
    npU = npU->npUnext;
  }

  if (pIORB->UnitStatus & US_PORT_UPDATE) {
    int i;

    npA->StatusPort = pIORB->StatusPort;
    npA->BasePort   = pIORB->BasePort;
    for (i = FI_PDAT; i <= FI_PCMD; i++) {
      npA->IOPorts[i] = npA->BasePort + i;
      npA->IORegs[i]  = 0;
    }
    DEVCTL = DEFAULT_ATAPI_DEVCON_REG;

    npA->IRQLevel = pIORB->IRQLevel;
    npA->InternalCmd.IOSGPtrs.iPortAddress = npA->BasePort;
    npA->ExternalCmd.IOSGPtrs.iPortAddress = npA->BasePort;
  }

  if (pIORB->UnitStatus & US_PRESENT) {
    memcpy (ScratchBuf, pIORB->pIdentifyData, 512);
    npU = ConfigureUnit (npA, (NPIDENTIFYDATA)ScratchBuf, TRUE);

    if (npU) {
      npU->Flags &= ~UCBF_NOTPRESENT;
      npU->Flags |= UCBF_READY;

      if ((pIORB->UnitStatus & (US_BM_READ | US_BM_WRITE)) == (US_BM_READ | US_BM_WRITE))
	npU->Flags |= UCBF_BM_DMA;
      else
	npU->Flags &= ~UCBF_BM_DMA;
    }
    npA->npUactive[UnitId] = npU;
  } else {
    npA->npUactive[UnitId] = NULL;
  }
}

/*อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป
บ		       Other Routines					       บ
ศอออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ*/
/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ MediaInvalid4Drive()		   บ
ศอออออออออออออออออออออออออออออออออ*/
USHORT NEAR MediaInvalid4Drive (NPA npA)
{
  NPU npU = npA->npU;
  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ If Media Changed, then let them know  ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  if (npU->MediaStatus & MF_MC) {
    npA->IORBError = IOERR_MEDIA_CHANGED;
    npA->OSMState  = ACBOS_COMPLETE_IORB;
    return (MV_INVALID);
  } else {
    return (MV_VALID);
  }
}

ULONG NEAR _fastcall Swap32 (ULONG x) {
  _asm {
    xchg al, ah
    xchg dl, dh
    xchg ax, dx
  }
}

USHORT NEAR _fastcall Swap16 (SHORT x) {
  _asm xchg al, ah
}

void NEAR ModifyForTape (NPU npU, NPCH c) {
  switch (c[0]) {
    case SCSI_WRITE_FILEMARKS :
      if (npU->UnitType == ONSTREAM_TAPE) c[4] = 0;
      /* fall through */
//    case SCSI_REWIND		:
//    case SCSI_LOCATE		:
    case SCSI_READ_6	      :
    case SCSI_WRITE_6	      :
      c[1] |= 0x01;
      break;
    case SCSI_START_STOP_UNIT :
      if ((npU->UnitType == ONSTREAM_TAPE) &&
	 (((c[4] & 3) == EJECT_THE_MEDIA) || ((c[4] & 3) == 0)))
	c[4] = (c[4] & ~EJECT_THE_MEDIA) | EJECT_THE_MEDIA_ONSTREAM;
      c[1] |= 0x01;
      break;
    case SCSI_ERASE	      :
      c[1] |= 0x03;
      break;
    case SCSI_MODE_SELECT     :
      c[1] = (c[1] & ~0x01) | 0x10;
      break;
//    case SCSI_MODE_SENSE	:
//	c[1] |= 0x08;
//	break;
  }
}

void NEAR ModifyCDB6 (NPA npA, NPCH c) {
  NPCMDIO npCmdIO = npA->npCmdIO;
  int i;

  switch (c[0]) {
    case SCSI_READ_6:
    case SCSI_WRITE_6:
    case SCSI_SEEK_6:
      c[8] = c[4]; c[7] = c[6] = 0; c[5] = c[3]; c[4] = c[2];
      c[3] = c[1] & 0x1F; c[2] = 0; c[1] &= 0xE0;
      c[0] += SCSI_READ_10 - SCSI_READ_6;
      break;

    case SCSI_MODE_SELECT: {
      NPCH   d = IdentDataBuf[npA->Index];
      USHORT NewNumBytes;

      NewNumBytes = (c[4] > 0) ? c[4] + 4 : 0;
      c[8] = NewNumBytes; c[7] = NewNumBytes >> 8;
      c[4] = c[5] = c[6] = c[3];
      c[0] = SCSI_MODE_SELECT_10;

      if (NewNumBytes) { /* Data area is allocated */
	USHORT Len;

	Len = MapSGList (npA);
	if (Len > (IDENTIFY_DATA_BYTES - 4)) Len = IDENTIFY_DATA_BYTES - 4;
	memcpy (d + 4, MAKEP (npA->MapSelector, 0), Len);
	d[1] = d[4]; d[2] = d[5]; d[3] = d[6];
	d[0] = d[4] = d[5] = d[6] = 0;

	npCmdIO->IOSGPtrs.pSGList	      = &(IDDataSGList[npA->Index]);
	npCmdIO->IOSGPtrs.pSGList->ppXferBuf  = ppIdentDataBuf[npA->Index];
	npCmdIO->IOSGPtrs.pSGList->XferBufLen = NewNumBytes;
      }
      npCmdIO->cXferBytesRemain = NewNumBytes;
      break;
    }

    case SCSI_MODE_SENSE: {
      USHORT NumBytes;

      if (c[4] > 0) {
	NumBytes = c[4] + 4;
	c[8] = NumBytes; c[7] = NumBytes >> 8;
	npCmdIO->cXferBytesRemain = NumBytes;

	clrmem (IdentDataBuf[npA->Index], NumBytes);
	npCmdIO->IOSGPtrs.pSGList	      = &(IDDataSGList[npA->Index]);
	npCmdIO->IOSGPtrs.pSGList->ppXferBuf  = ppIdentDataBuf[npA->Index];
	npCmdIO->IOSGPtrs.pSGList->XferBufLen = NumBytes;

	npA->OSMFlags |= ACBOF_MODESENSE_ACTIVE;
      } else {
	c[8] = c[7] = 0;
      }
      c[4] = c[5] = c[6] = c[3];
      c[0] = SCSI_MODE_SENSE_10;
      break;
    }
  }
}

void NEAR CalcXferLen (NPA npA) {
  PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO) npA->pIORB;
  PSCATGATENTRY   pTempSGList = pIORB->pSGList;
  NPCMDIO	  npCmdIO = npA->npCmdIO;
  USHORT	  i;

  npCmdIO->IOSGPtrs.SGOffsetStart = (ULONG)pTempSGList;

  for (i = pIORB->cSGList; i > 0; i--, pTempSGList++)
    npCmdIO->cXferBytesRemain += pTempSGList->XferBufLen;

  npCmdIO->IOSGPtrs.iSGListStart = (npCmdIO->cXferBytesRemain > 0xFFFFUL) ?
    0xFFFF : (USHORT)npCmdIO->cXferBytesRemain;
}

USHORT NEAR MapSGList (NPA npA)
{
  NPCMDIO npCmdIO = npA->npCmdIO;
  USHORT  Len = npCmdIO->IOSGPtrs.iSGListStart;
  LIN	  lSGList;

  DevHelp_VirtToLin (SELECTOROF (npCmdIO->IOSGPtrs.SGOffsetStart),
		     OFFSETOF (npCmdIO->IOSGPtrs.SGOffsetStart), &lSGList);
  DevHelp_PageListToGDTSelector (npA->MapSelector, Len, 6, lSGList);
  return (Len);
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ SetupAndExecute()		   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR SetupAndExecute (NPA npA, UCHAR command)
{
  ULONG 		 ppExternalSenseDataBuf;
  PSCSI_STATUS_BLOCK	 pSCSISB;
  PIORB_EXECUTEIO	 pIORB = (PIORB_EXECUTEIO) npA->pIORB;
  PIORB_ADAPTER_PASSTHRU pIORB_PT;
  PIORB_FORMAT		 pIORB_FMT;
  PFORMAT_CMD_TRACK	 pFMT;
  USHORT		 mediatype;
  CHS_ADDR		 CHSAddr;
  PSCATGATENTRY 	 pTempSGList = pIORB->pSGList;
  USHORT		 i, BlockCount;
  ULONG 		 BlockCountL;
  NPU			 npU = npA->npU;
  NPCMDIO		 npCmdIO = npA->npCmdIO;

  TR(0)

  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ Clear SG Pointer Entries		  ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  clrmem (&npCmdIO->IOSGPtrs, sizeof (ADD_XFER_IO));

  npCmdIO->IOSGPtrs.iPortAddress = npA->BasePort;
  npCmdIO->IOSGPtrs.Selector	 = npA->MapSelector;
  npCmdIO->IOSGPtrs.pSGList	 = &(IDDataSGList[npA->Index]);
  npCmdIO->IOSGPtrs.pSGList->ppXferBuf = ppIdentDataBuf[npA->Index];

  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ Clear bytes counts for transfers	  ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  npCmdIO->cXferBytesRemain   = 0;
  npCmdIO->cXferBytesComplete = 0;

  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ Clear the ATAPI PacKet		  ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  clrmem (npCmdIO->ATAPIPkt, MAX_ATAPI_PKT_SIZE);

  BlockCount = pIORB->BlockCount;

  switch (command) {
    case SAE_TESTUNITREADY:
      npCmdIO->ATAPIPkt[0] = FXP_TESTUNIT_READY;
      npA->OSMReqFlags	|= ACBR_TEST_UNIT_RDY;
      break;

    case SAE_INTERNAL_IDENTIFY:
      npCmdIO->IOSGPtrs.pSGList->XferBufLen = IDENTIFY_DATA_BYTES;
      npCmdIO->cXferBytesRemain = IDENTIFY_DATA_BYTES;
      npCmdIO->IOSGPtrs.cSGList = 1;

      npU->ReqFlags |= UCBR_IDENTIFY;
      npA->ISMFlags |= ACBIF_ATA_OPERATION;		 /* ATA CMD */
      npA->OSMFlags |= ACBOF_IDENTIFY_ACTIVE;
      break;

    case SAE_READ_CAPACITY:
      npCmdIO->ATAPIPkt[0]	= FXP_READ_CAPACITY;
      npCmdIO->IOSGPtrs.pSGList->XferBufLen = 8;
      npCmdIO->cXferBytesRemain = 8;
      npCmdIO->IOSGPtrs.cSGList = 1;

      clrmem (IdentDataBuf[npA->Index], 8);

      npA->OSMFlags |= ACBOF_READCAPACITY_ACTIVE;
      break;

    case SAE_READ:
      npCmdIO->ATAPIPkt[0] = FXP_READ_10;	  /* READ */

    Sae_RWCommon:
      npCmdIO->IOSGPtrs.cSGList = pIORB->cSGList;
      npCmdIO->IOSGPtrs.pSGList = pIORB->pSGList;

      CalcXferLen (npA);
      BlockCountL= npCmdIO->cXferBytesRemain >> (9 + (npU->MediaGEO.BytesPerSector >> 10));
      if (BlockCountL < BlockCount) BlockCount = BlockCountL;

    Sae_RWCommon2:
      (ULONG) npCmdIO->ATAPIPkt[2] = Swap32 (pIORB->RBA); /* LBA */
     (USHORT) npCmdIO->ATAPIPkt[7] = Swap16 (BlockCount); /* Transfer Length */
      break;

    case SAE_WRITE:
      npCmdIO->ATAPIPkt[0] = FXP_WRITE_10;		/* WRITE  */
      goto Sae_RWCommon;

    case SAE_WRITE_VERIFY:
      npCmdIO->ATAPIPkt[0] = FXP_WRITE_VERIFY;		/* VERIFY */
      goto Sae_RWCommon;

    case SAE_READ_VERIFY:
      npCmdIO->ATAPIPkt[0]  = FXP_VERIFY;     /* Verify media only */
      npCmdIO->ATAPIPkt[1] &= ~BYTECHK;       /* ByteChk = 0	   */
      goto Sae_RWCommon2;

    case SAE_REQUESTSENSE:
      npA->OSMFlags |= ACBOF_SENSE_DATA_ACTIVE;
      /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
      ณ Note: we are setting npCmdIO to InternalCmd ณ
      ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
      npCmdIO = npA->npCmdIO = &npA->InternalCmd;

      npCmdIO->cXferBytesRemain = SENSE_DATA_BYTES;

      SenseDataSGList[npA->Index].ppXferBuf  = ppSenseDataBuf[npA->Index];
      SenseDataSGList[npA->Index].XferBufLen = SENSE_DATA_BYTES;

      npCmdIO->IOSGPtrs.cSGList       = 1;
      npCmdIO->IOSGPtrs.pSGList       = &(SenseDataSGList[npA->Index]);
      npCmdIO->IOSGPtrs.iPortAddress  = npA->BasePort;
      npCmdIO->IOSGPtrs.Selector      = npA->MapSelector;
      npCmdIO->IOSGPtrs.iSGList       = 0;
      npCmdIO->IOSGPtrs.SGOffset      = 0;

      clrmem (npCmdIO->ATAPIPkt, MAX_ATAPI_PKT_SIZE);

      npCmdIO->ATAPIPkt[0] = FXP_REQUEST_SENSE;
      npCmdIO->ATAPIPkt[4] = SENSE_DATA_BYTES;
      break;

    case SAE_LOCK_MEDIA:					   /* LOCK */
      npCmdIO->ATAPIPkt[4] = 0x01;  /* Prevent bit */
      /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
      ณ vv FALL THROUGH vvณ
      ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
    case SAE_UNLOCK_MEDIA:					 /* UNLOCK */
      npCmdIO->ATAPIPkt[0] = FXP_PRV_ALWMEDRMVL;
      npA->OSMReqFlags	  |= ACBR_DISCARD_ERROR;
      break;

    case SAE_EJECT_MEDIA:					  /* EJECT */
      npCmdIO->ATAPIPkt[0] = FXP_START_STOPUNIT;
      npCmdIO->ATAPIPkt[4] = EJECT_THE_MEDIA;
      break;

    case SAE_RESET:						 /* RESET */
      npA->OSMFlags |= ACBOF_RESET_ACTIVE;
      npU->ReqFlags |= UCBR_RESET;
      npA->ISMFlags |= ACBIF_ATA_OPERATION;
      break;

    case SAE_EXECUTE_ATA: {
      PPassThruATA  picp;

      pIORB_PT = (PIORB_ADAPTER_PASSTHRU) npA->pIORB;
      picp = (PPassThruATA) pIORB_PT->pControllerCmd;

      if ((npU->ReqFlags == 0) && (picp != NULL)) {
	// copy caller supplied tack file register info

	FEAT	= picp->TaskFileIn.Features;
	SECCNT	= picp->TaskFileIn.SectorCount;
	SECNUM	= picp->TaskFileIn.SectorNumber;
	CYLL	= picp->TaskFileIn.CylinderLow;
	CYLH	= picp->TaskFileIn.CylinderHigh;
	COMMAND = picp->TaskFileIn.Command;

	npU->ReqFlags |= UCBR_PASSTHRU;
      }
      /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
      ณ determine number of bytes in s/g list ณ
      ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
      CalcXferLen (npA);

      if (npCmdIO->cXferBytesRemain) {	  /* Data area is allocated */
	npCmdIO->IOSGPtrs.Mode = (pIORB_PT->Flags & PT_DIRECTION_IN) ?
				  PORT_TO_SGLIST : SGLIST_TO_PORT;
	npCmdIO->IOSGPtrs.cSGList = pIORB_PT->cSGList;
	npCmdIO->IOSGPtrs.pSGList = pIORB_PT->pSGList;

	if (npU->ReqFlags & UCBR_IDENTIFY)
	  npCmdIO->IOSGPtrs.Mode = PORT_TO_SGLIST;
      }

      if (npU->ReqFlags == 0) {
	SetCmdError (npA);
	return;
      }
      npA->ISMFlags |= ACBIF_ATA_OPERATION;
      break;
    }
    case SAE_EXECUTE_CDB:
      pIORB_PT = (PIORB_ADAPTER_PASSTHRU) npA->pIORB;

      if (pIORB_PT->ControllerCmdLen <= npU->CmdPacketLength) {
	NPCH c = npCmdIO->ATAPIPkt;
	NPU npUactive = npA->npUactive[npU->UnitId];

	/*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
	ณ Fill in the atapi packet		ณ
	ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
	memcpy (npCmdIO->ATAPIPkt, pIORB_PT->pControllerCmd, pIORB_PT->ControllerCmdLen);

	if (InitComplete && npUactive && !(npU->MaxLUN))
	  npU = npA->npU = npUactive;

	/*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
	ณ determine number of bytes in s/g list ณ
	ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
	CalcXferLen (npA);

	if (npCmdIO->cXferBytesRemain) {    /* Data area is allocated */
	  npCmdIO->IOSGPtrs.cSGList = pIORB_PT->cSGList;
	  npCmdIO->IOSGPtrs.pSGList = pIORB_PT->pSGList;
	}

	if ((c[0] == FXP_START_STOPUNIT) && (c[4] == EJECT_THE_MEDIA)) {
	  npU->MediaStatus &= ~MF_MC;
	  npU->MediaStatus |= MF_NOMED;
	}

//	  if ((npU->UnitType == UIB_TYPE_TAPE) ||
//	     (npU->UnitType == ONSTREAM_TAPE))
	if (npU->UnitType == ONSTREAM_TAPE) ModifyForTape (npU, c);

	if (c[0] == SCSI_INQUIRY) {
	  c[4] = (c[4] + 1) & ~1;
	  npA->OSMFlags |= ACBOF_INQUIRY_ACTIVE;
	} else {
	  if (npU->Flags & UCBF_XLATECDB6) ModifyCDB6 (npA, c);
	}
      } else {
	SetCmdError (npA);
	return;
      }
      break;

    case SAE_FORMAT_TRACK:
      if (!(npU->DrvDefFlags & DDF_LS120)) {
	SetCmdError (npA);
	return;
      }

      pIORB_FMT = (PIORB_FORMAT) npA->pIORB;
      pFMT	= (PFORMAT_CMD_TRACK) pIORB_FMT->pFormatCmd;

      npCmdIO->IOSGPtrs.cSGList = pIORB_FMT->cSGList;
      npCmdIO->IOSGPtrs.pSGList = pIORB_FMT->pSGList;

      for (i = 0; i < pIORB_FMT->cSGList; i++, pTempSGList++)
	npCmdIO->cXferBytesRemain += pTempSGList->XferBufLen;

      if (pIORB->iorbh.RequestControl & IORB_CHS_ADDRESSING)
	CHSAddr = *(PCHS_ADDR) &pFMT->RBA;
      else
	ADD_ConvRBAtoCHS (pFMT->RBA, &npU->MediaGEO, &CHSAddr);

      npCmdIO->ATAPIPkt[0] = FXP_FORMAT_UNIT;	     /* Format CMD  */

      mediatype = (npU->MediaType & 0x30);	     /* Unformatted */
      if (mediatype == HD_MEDIUM)
	 mediatype |= 0x04;	 /* Formatted HD */
      else
	 mediatype |= 0x01;	 /* Formatted DD or UHD */

      npCmdIO->ATAPIPkt[2]  = mediatype;	/* Valid Media Type */
      npCmdIO->ATAPIPkt[6] |= 0x20;		/* sector size (nx(256)) */
      npCmdIO->ATAPIPkt[6] |= ((CHSAddr.Head & 1) << 2);  /* Head */

      if ((mediatype & 0x30) < 0x30)		/* HD or DD only!  */
	npCmdIO->ATAPIPkt[6] |= 0x02;		/* Single Track FMT*/

      if (pFMT->Flags & FF_VERIFY)
	npCmdIO->ATAPIPkt[6] |= 0x01;		/* Verdor Cert/Verify */

      npCmdIO->ATAPIPkt[7] = CHSAddr.Cylinder;	/* TRACK */
      break;

    default:
      SetCmdError (npA);
      return;
  }

  npA->OSMFlags |= ACBOF_WAITSTATE;
  npA->OSMState  = ACBOS_ISM_COMPLETE;

  StartOSMRequest (npA);
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR ProcessIdentify (NPA npA)
{
  NPU	 npU   = npA->npU;
  PIORBH pIORB = npA->pIORB;

  if (npA->IORBError) {
    /* Return the error */
  } else {
    /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
    ณ  Get the Media geometry	ณ
    ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
    ExtractMediaGeometry (npA);

    if (MediaInvalid4Drive (npA)) {
      /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
      ณ Media changed error			 ณ
      ณ IORBError is set in MediaInvalid4Drive() ณ
      ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
      return;
    }
    /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
    ณ If No Medium, then we should return error    ณ
    ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
    if (npU->MediaStatus & MF_NOMED) {
      npA->IORBError = IOERR_MEDIA_NOT_PRESENT;
    } else {
      /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
      ณ Return the media geometry ณ
      ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
      *((PIORB_GEOMETRY) pIORB)->pGeometry = npU->MediaGEO;
    }
  }
}

VOID NEAR RecalculateGeometry (NPGEOMETRY npGEO) {
  ULONG  SectorsPerCylinder, NumSectors;

#if PCITRACER
  outp (TRPORT, 0xCC);
  outpw(TRPORT, npGEO->BytesPerSector);
  outpd(TRPORT, npGEO->TotalSectors);
#endif

  NumSectors = npGEO->TotalSectors;

  if (NumSectors < 2097152L) {		    /* 0 - 1GB		  */
    USHORT NumTracks, NumHeads, MaxHeads;

    NumTracks = NumSectors / 64;
    NumHeads  = 64; // 2;
    if (NumTracks < 16384)
      MaxHeads = 64;
    else
      MaxHeads = 128;

    while (NumTracks && (NumHeads < MaxHeads) && !(NumTracks & 1)) {
      NumTracks >>= 1;
      NumHeads	<<= 1;
    }
    npGEO->NumHeads	   = NumHeads;
    npGEO->SectorsPerTrack = 32;
  } else if (NumSectors < 8257536L) {	    /* 1GB - ~4GB (3.93)  */
    npGEO->NumHeads	   = 128;
    npGEO->SectorsPerTrack = 63;
  } else {				    /* > 4GB		  */
    npGEO->NumHeads	   = 255;
    npGEO->SectorsPerTrack = 63;
  }

  SectorsPerCylinder	= npGEO->NumHeads * npGEO->SectorsPerTrack;
  npGEO->TotalCylinders = NumSectors / SectorsPerCylinder;
  npGEO->TotalSectors	= (ULONG)(npGEO->TotalCylinders * SectorsPerCylinder);
  npGEO->Reserved	= 0;

#if PCITRACER
  outpw(TRPORT, npGEO->TotalCylinders);
  outpw(TRPORT, npGEO->NumHeads);
  outpw(TRPORT, npGEO->SectorsPerTrack);
  outpd(TRPORT, npGEO->TotalSectors);
#endif
}

VOID NEAR ProcessCapacity (NPA npA)
{
  NPU	 npU   = npA->npU;
  PIORBH pIORB = npA->pIORB;

#if PCITRACER
  outpw (TRPORT, pIORB->Status);
  outpw (TRPORT, npU->MediaStatus);
#endif

  if (npA->IORBError) {
     /* Return the error */
  } else {
    npU->MediaGEO.TotalSectors =
		 1 + Swap32 (((ULONG *)(IdentDataBuf[npA->Index]))[0]);
    npU->MediaGEO.BytesPerSector =
		     Swap32 (((ULONG *)(IdentDataBuf[npA->Index]))[1]);

    RecalculateGeometry (&(npU->MediaGEO));

    /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
    ณ If No Medium, then we should return error    ณ
    ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
    if (npU->MediaStatus & MF_NOMED) {
      *((PIORB_GEOMETRY)pIORB)->pGeometry = npU->DeviceGEO;
      npA->IORBError = IOERR_MEDIA_NOT_PRESENT;
    } else {
      /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
      ณ Return the media geometry ณ
      ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
      *((PIORB_GEOMETRY)pIORB)->pGeometry = npU->MediaGEO;
    }
  }
}

VOID NEAR ProcessCDBModeSense (NPA npA)
{
  npA->npCmdIO->ATAPIPkt[0] = SCSI_MODE_SENSE;

  if (npA->IORBError) {
    /* Return the error */
  } else {
    USHORT Len;
    NPCH   s = IdentDataBuf[npA->Index];

    s[6] = s[3]; s[5] = s[2]; s[4] = s[1];
    Len = MapSGList (npA);
    if (Len > (IDENTIFY_DATA_BYTES - 4)) Len = IDENTIFY_DATA_BYTES - 4;
    memcpy (MAKEP (npA->MapSelector, 0), s + 4, Len);
  }
}

VOID NEAR ProcessCDBInquiry (NPA npA)
{
  if (npA->IORBError) {
    /* Return the error */
  } else {
    USHORT Len;
    PCHAR  d;

TR(0x37)
    Len = MapSGList (npA);
    d = MAKEP (npA->MapSelector, 0);

    if (Len > 2) d[2] |= 5;			// ansi revision
    if (Len > 3) d[3]  = (d[3] & 0xF0) | 2;	// response format
  }
}

/* ออออออออออออออออออออออออออออออออป
บ				   บ
บ ExtractMediaGeometry()	   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR ExtractMediaGeometry (NPA npA)
{
  NPU		 npU = npA->npU;
  NPIDENTIFYDATA npID;

  npID = (NPIDENTIFYDATA)&(IdentDataBuf[npA->Index]);

  /*ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ
  ณ	      LS-120 Drive		  ณ
  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ*/
  clrmem (&(npU->MediaGEO), sizeof (GEOMETRY));
  npU->MediaType = (0x3F & npID->MediaTypeCode);

  if (!(npU->MediaType)) {
    npU->MediaStatus = MF_NOMED;

  } else {
    npU->MediaGEO.TotalCylinders  = npID->CurrentCylHdSec[0];
    npU->MediaGEO.NumHeads	  = npID->CurrentCylHdSec[1];
    npU->MediaGEO.SectorsPerTrack = npID->CurrentCylHdSec[2];
    npU->MediaGEO.BytesPerSector  = npID->BytesPerSector;
    npU->MediaGEO.TotalSectors	  = (ULONG) (npU->MediaGEO.TotalCylinders *
					     npU->MediaGEO.NumHeads	  *
					     npU->MediaGEO.SectorsPerTrack);
    npU->MediaStatus &= ~ (MF_NOMED | MF_MC);
  }
}

/* ออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
VOID NEAR SetCmdError (NPA npA)
{
  npA->IORBError = IOERR_CMD_NOT_SUPPORTED;
  npA->OSMFlags &= ~ACBOF_WAITSTATE;
}
