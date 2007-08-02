/**************************************************************************
 *
 * SOURCE FILE NAME = ATAPIGEN.C
 *
 * DESCRIPTION : General functions for ATAPI driver
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 ***************************************************************************/

#define INCL_NOPMAPI
#define INCL_DOSINFOSEG
#include "os2.h"
#include "devcmd.h"
#include "dskinit.h"
#include "devclass.h"

#include "iorb.h"
#include "strat2.h"
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

#pragma optimize(OPTIMIZE, on)

/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ  IODelay			   บ
บ				   บ
บ  Wait for 1 ๆsec		   บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
VOID NEAR IODelay()
{
  _asm { mov cx, IODelayCount
	 l: loop l }
}

/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ  strncpy			   บ
บ				   บ
บ  Copies the contents of string   บ
บ  (s) to that of string (d) for   บ
บ  (n) characters.  (d) is then    บ
บ  null terminated.		   บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
VOID NEAR strncpy (PSZ d, PSZ s, USHORT n)
{
   while (*s && n--) *d++ = *s++;
   *d = 0;

   return;
}

USHORT NEAR CheckWorker (NPA npA, UCHAR Mask, UCHAR Test)
{
  UCHAR Status;
  SHORT Stop;

  Stop = *msTimer + (Mask & FX_DRQ ? MAX_WAIT_DRQ : MAX_WAIT_BSY);
  do {
    Status = inp (DEVCTLREG);
    IODelay();
    if ((Status & Mask) == Test) return (TRUE);
  } while ((*msTimer - Stop) < 0);

  return (FALSE);
}


/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ  BSYWAIT			   บ
บ				   บ
บ  Poll for BSY until success or   บ
บ  until exceed max poll time	   บ
บ				   บ
บ  Returns True if BSY clear	   บ
บ  False if exceeded max poll time บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
USHORT NEAR BSYWAIT (NPA npA)
{
  return (CheckWorker (npA, FX_BUSY, 0));
}

/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ  DRQ_SET_WAIT 		   บ
บ				   บ
บ  Poll for DRQ until asserted or  บ
บ  until exceed max poll time	   บ
บ				   บ
บ  Returns True if DRQ set	   บ
บ  False if exceeded max poll time บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
USHORT NEAR BSY_CLR_DRQ_SET_WAIT (NPA npA)
{
  return (CheckWorker (npA, FX_BUSY | FX_DRQ, FX_DRQ));
}

/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ  DRQ_AND_BSY_CLEAR_WAIT	   บ
บ				   บ
บ  Poll for DRQ and BSY until both บ
บ  are clear or until exceed max   บ
บ  poll time			   บ
บ				   บ
บ  Returns True if DRQ AND BSY are บ
บ  both clear,			   บ
บ  False if exceeded max poll time บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
USHORT NEAR DRQ_AND_BSY_CLEAR_WAIT  (NPA npA)
{
  return (CheckWorker (npA, FX_BUSY | FX_DRQ, 0));
}

USHORT NEAR DRQ_CLEAR_WAIT (NPA npA)
{
  return (CheckWorker (npA, FX_DRQ, 0));
}


USHORT NEAR ExecuteNB (UCHAR hADD, NPIORB npIORB) {
  npIORB->Status	 = 0;
  npIORB->RequestControl = 0;
  npIORB->NotifyAddress  = NULL;

  (*(PADDEntry FAR *)&(pDriverTable->DCTableEntries[hADD-1].DCOffset))((PIORB)(npIORB));
  while (!(npIORB->Status & IORB_DONE))  /* Wait till done */
    ;

  if (npIORB->Status & IORB_ERROR)
    return (npIORB->ErrorCode ? npIORB->ErrorCode : 1);
  else
    return (0);
}

USHORT NEAR Execute (UCHAR hADD, NPIORB npIORB) {
  npIORB->Status	 = 0;
  npIORB->RequestControl = IORB_ASYNC_POST;
  npIORB->NotifyAddress  = (PIORBNotify)&NotifyIORBDone;

  if (hADD)
    (*(PADDEntry FAR *)&(pDriverTable->DCTableEntries[hADD-1].DCOffset))((PIORB)(npIORB));
  else
    ATAPIADDEntry ((PIORB)npIORB);

  DISABLE
  while (!(npIORB->Status & IORB_DONE)) {
    DevHelp_ProcBlock ((ULONG)(PIORB)npIORB, -1, WAIT_IS_NOT_INTERRUPTABLE);
    DISABLE
  }
  ENABLE

  if (npIORB->Status & IORB_ERROR)
    return (npIORB->ErrorCode ? npIORB->ErrorCode : 1);
  else
    return (0);
}


USHORT NEAR SuspendResumeOtherAdd (NPA npA, UCHAR function)
{
  NPIORB_DEVICE_CONTROL pIORB;

  pIORB = (NPIORB_DEVICE_CONTROL) &npA->IORB;
  clrmem (&npA->IORB, sizeof (npA->IORB));

  pIORB->iorbh.Length	       = sizeof(IORB_DEVICE_CONTROL);
  pIORB->iorbh.UnitHandle      = npA->SharedhUnit;
  pIORB->iorbh.CommandCode     = IOCC_DEVICE_CONTROL;
  pIORB->iorbh.CommandModifier = IOCM_RESUME;
  pIORB->iorbh.RequestControl  = IORB_ASYNC_POST;
  pIORB->iorbh.NotifyAddress   = (PIORBNotify)&NotifySuspendResumeDone;

  if (function == IOCM_SUSPEND) {
    pIORB->iorbh.CommandModifier = IOCM_SUSPEND;
    pIORB->IRQHandlerAddress	 = npA->IRQHandler;

    pIORB->Flags = DC_SUSPEND_DEFERRED | DC_SUSPEND_IRQADDR_VALID;
  }

  ((NPA *)pIORB->iorbh.DMWorkSpace)[0] = npA;

  (npA->SharedEP)((PIORB)(pIORB));
}

ULONG NEAR QueryOtherAdd (NPA npA)
{
  NPIORB_DEVICE_CONTROL pIORB;

  pIORB = (NPIORB_DEVICE_CONTROL) &npA->IORB;
  clrmem (&npA->IORB, sizeof (npA->IORB));

  pIORB->iorbh.Length	       = sizeof(IORB_DEVICE_CONTROL);
  pIORB->iorbh.UnitHandle      = npA->SharedhUnit;
  pIORB->iorbh.CommandCode     = IOCC_DEVICE_CONTROL;
  pIORB->iorbh.CommandModifier = IOCM_GET_QUEUE_STATUS;

  ExecuteNB (npA->SharedhADD, (NPIORB)pIORB);
  return (pIORB->QueueStatus);
}

PIORB _cdecl _loadds FAR NotifySuspendResumeDone (PIORB pIORB)
{
  StartOSM (((NPA *)pIORB->DMWorkSpace)[0]);
}

/*อออออออออออออออออออออออออออออออออป
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
USHORT NEAR AllocDeallocChangeUnit (UCHAR hADD, USHORT UnitHandle, UCHAR function, PUNITINFO pUI)
{
  NPIORB_UNIT_CONTROL pIORB;

  pIORB = (NPIORB_UNIT_CONTROL) &InitIORB;
  clrmem (&InitIORB, sizeof (InitIORB));

  pIORB->iorbh.Length	       = sizeof(IORB_UNIT_CONTROL);
  pIORB->iorbh.UnitHandle      = UnitHandle;
  pIORB->iorbh.CommandCode     = IOCC_UNIT_CONTROL;
  pIORB->iorbh.CommandModifier = function;

  if (function == IOCM_CHANGE_UNITINFO) {
    pIORB->UnitInfoLen = sizeof (UNITINFO);
    pIORB->pUnitInfo   = pUI;
  }

  if (function == IOCM_ALLOCATE_UNIT) {
    return (Execute (hADD, (NPIORB)pIORB));
  } else {
    return (ExecuteNB (hADD, (NPIORB)pIORB));
  }
}

VOID NEAR EjectMedia (NPU Unit)
{
  NPIORB_DEVICE_CONTROL   npIORB  = (NPIORB_DEVICE_CONTROL)&InitIORB;
  NPIORB_ADAPTER_PASSTHRU npIORBP = (NPIORB_ADAPTER_PASSTHRU)&InitIORB;
  USHORT rc, ErrorCode = 0;
  NPCH	 CDB6 = (NPCH)&(npIORBP->iorbh.ADDWorkSpace);

  do {
    clrmem (npIORB, sizeof (InitIORB));

    npIORB->iorbh.Length	  = sizeof (IORB_DEVICE_CONTROL);
    npIORB->iorbh.UnitHandle	  = (USHORT)Unit;
    npIORB->iorbh.CommandCode	  = IOCC_DEVICE_CONTROL;
    npIORB->iorbh.CommandModifier = IOCM_UNLOCK_MEDIA;

    rc = Execute (0, (NPIORB)npIORB);
    if (ErrorCode == rc)
      break;
    else
      ErrorCode = rc;
  } while (ErrorCode);

  do {
    clrmem (npIORB, sizeof (InitIORB));

    npIORB->iorbh.Length	  = sizeof (IORB_UNIT_STATUS);
    npIORB->iorbh.UnitHandle	  = (USHORT)Unit;
    npIORB->iorbh.CommandCode	  = IOCC_UNIT_STATUS;
    npIORB->iorbh.CommandModifier = IOCM_GET_MEDIA_SENSE;

    rc = Execute (0, (NPIORB)npIORB);
    if (ErrorCode == rc)
      break;
    else
      ErrorCode = rc;
  } while (ErrorCode == IOERR_MEDIA_CHANGED);

#if 0
  if (((Unit->UnitType == UIB_TYPE_CDROM) ||
       (Unit->UnitType == UIB_TYPE_TAPE)  ||
       (Unit->UnitType == ONSTREAM_TAPE)) &&
      (ErrorCode == IOERR_MEDIA_NOT_PRESENT)) return;
#else
  if ((((Unit->UnitType == UIB_TYPE_CDROM) ||
	(Unit->UnitType == ONSTREAM_TAPE)) &&
       (ErrorCode == IOERR_MEDIA_NOT_PRESENT))	||
      (Unit->UnitType == UIB_TYPE_TAPE))
       return;
#endif

  clrmem (npIORBP, sizeof (InitIORB));
  CDB6[0] = SCSI_START_STOP_UNIT;
//  CDB6[1] = (Unit->UnitType == UIB_TYPE_CDROM) ? 0 : 1;
  CDB6[1] = (Unit->UnitType == UIB_TYPE_TAPE) ? 1 : 0;
  CDB6[4] = EJECT_THE_MEDIA;

  npIORBP->iorbh.Length 	 = sizeof (IORB_ADAPTER_PASSTHRU);
  npIORBP->iorbh.UnitHandle	 = (USHORT)Unit;
  npIORBP->iorbh.CommandCode	 = IOCC_ADAPTER_PASSTHRU;
  npIORBP->iorbh.CommandModifier = IOCM_EXECUTE_CDB;
  npIORBP->ControllerCmdLen	 = 6;
  npIORBP->pControllerCmd	 = (PBYTE)CDB6;

  Execute (0, (NPIORB)npIORBP);
  ErrorCode = 0;
  if ((Unit->UnitType == UIB_TYPE_TAPE) ||
      (Unit->UnitType == ONSTREAM_TAPE)) {
    USHORT Count = 0;

    do {
      clrmem (npIORB, sizeof (InitIORB));

      npIORB->iorbh.Length	    = sizeof (IORB_UNIT_STATUS);
      npIORB->iorbh.UnitHandle	    = (USHORT)Unit;
      npIORB->iorbh.CommandCode     = IOCC_UNIT_STATUS;
      npIORB->iorbh.CommandModifier = IOCM_GET_MEDIA_SENSE;

      rc = Execute (0, (NPIORB)npIORB);
      if (ErrorCode == rc) {
	ErrorCode = rc;
	if (ErrorCode == IOERR_UNIT_NOT_READY) {
	  DevHelp_ProcBlock ((ULONG) npIORB, 100, WAIT_IS_INTERRUPTABLE);
	  if (++Count > 100) ErrorCode = 0;
	}
      } else {
	ErrorCode = rc;
      }
    } while (ErrorCode && (ErrorCode != IOERR_MEDIA_NOT_PRESENT));
  }
}

/*
ษออออออออออออออออออออออออออออออออออป
บ				   บ
บ  IDECDStr			   บ
บ				   บ
บ  Stategy Routine for IBMIDECD    บ
บ				   บ
บ  Accepts only the Init Base	   บ
บ  device drivers command (0x1B).  บ
บ  Returns unknown command for	   บ
บ  all others.			   บ
บ				   บ
ศออออออออออออออออออออออออออออออออออผ */
VOID FAR _cdecl IDECDStr()
{
  PRPH	 pRPH;
  USHORT Cmd;

  _asm {
    mov word ptr pRPH[0], bx	   /*  pRPH is initialized to	       */
    mov word ptr pRPH[2], es	   /*  ES:BX passed from the kernel    */
  }

  pRPH->Status = STATUS_DONE;

  switch (pRPH->Cmd) {
    case CMDInitBase :
      DriverInit ((PRPINITIN) pRPH);
      break;

    case CMDInitComplete:
      if (APMAttach() == 0) {
	APMRegister ((PAPMHANDLER)APMEventHandler,
		     APM_NOTIFYSETPWR | APM_NOTIFYNORMRESUME | APM_NOTIFYCRITRESUME, 0);
      }
      InitComplete = 1;
      break;

    case CMDShutdown :
      if (((PRPSAVERESTORE)pRPH)->FuncCode) {
	NPA   npA;
	NPU   npU;
	UCHAR i, j;

	for (i = 0; i < cAdapters; i++) {
	  npA = ACBPtrs[i].npA;
	  for (j = 0; j < npA->cUnits; j++) {
	    npU = npA->npUactive[j];
	    if (npU && (npU->Flags & UCBF_EJECT) && (npU->Status == UTS_OK))
	      EjectMedia (npU);
	  }
	}
      }
      break;

    case CMDOpen :
      if (SELECTOROF(ReadPtr) == NULL)
	ReadPtr = MsgBuffer;
      break;

    case CMDClose  :
    case CMDInputS :
    case CMDInputF :
      /* noop */
      break;

    case CMDINPUT : {
      PRP_RWV pRqR = (PRP_RWV)pRPH;
      PCHAR   ReadBuf;
      USHORT  Len;

      if (SELECTOROF (ReadPtr) != NULL) {
	Len = WritePtr - ReadPtr;
	if (Len > 0) {
	  if (Len > pRqR->NumSectors)
	    Len = pRqR->NumSectors;

	  DISABLE
	  DevHelp_PhysToVirt (pRqR->XferAddr, pRqR->NumSectors, &ReadBuf);
	  memcpy (ReadBuf, ReadPtr, Len);
	  ENABLE

	  ReadPtr += Len;
	  pRqR->NumSectors = Len;
	  if (WritePtr == ReadPtr)
	    SELECTOROF (ReadPtr) = NULL;
	} else {
	  pRqR->NumSectors = 0;
	  SELECTOROF (ReadPtr) = NULL;
	}
      } else {
	pRqR->NumSectors = 0;
      }
      break;
    }

    default :
      pRPH->Status = STATUS_DONE | STATUS_ERR_UNKCMD;
  }
}

/*--------------------------------------------------------
** Register for APM messages at the end of Initialization.
  ---------------------------------------------------------*/

/*------------------------------------*/
/* APM Suspend/Resume Support	      */
/* Reinitializes adapters and units   */
/* following a resume event.	      */
/*------------------------------------*/
USHORT FAR _loadds _cdecl APMEventHandler (PAPMEVENT Event)
{
  USHORT Message = (USHORT)Event->ulParm1;
  USHORT PwrState;

  if (Message == APM_SETPWRSTATE) {
    PwrState = (USHORT)(Event->ulParm2 >> 16);
    if (PwrState != APM_PWRSTATEREADY)
      return  (APMSuspend());
  } else if (Message == APM_NORMRESUMEEVENT ||
	     Message == APM_CRITRESUMEEVENT) {
    return(APMResume());
  }
  return 0;
}

/**************************************
 * APMSuspend is currently a no-op
 *************************************/
USHORT NEAR APMSuspend()
{
  return 0;
}

USHORT NEAR APMResume()
{
  NPA	 npA;
  NPU	 npU;
  USHORT i;
  USHORT j, rc;
  USHORT Flags;

  for (i = 0; i < cAdapters; i++) {
    npA = ACBPtrs[i].npA;
    if (!(npA->FlagsT & ATBF_DISABLED)) {
      npU = npA->UnitCB;
      for (j = 0; j < MAX_UNITS; j++, npU++) {
	if (npU->Flags & UCBF_FORCE) {
	  if (npU->Flags & UCBF_NOTPRESENT)
	    npU->Flags &= ~UCBF_READY;
	  else
	    npU->Flags |= UCBF_READY;
	}
      }
    }
  }
  return 0;
}

void NEAR _cdecl outpd (USHORT port, ULONG val)
{
  _asm mov dx,port;

  /* mov eax,val */
  _asm _emit 0x66;
  _asm mov ax,val;

  /* out dx,eax */
  _asm _emit 0x66;
  _asm out dx,ax;
}

ULONG NEAR inpd (USHORT port)
{
  _asm mov dx,port;

  /* in eax, dx */
  _asm _emit 0x66;
  _asm in ax,dx;

  /* shld edx, eax, 16 */
  _asm _emit 0x66;
  _asm _emit 0x0F;
  _asm _emit 0xA4;
  _asm _emit 0xC2;
  _asm _emit 0x10;
}
