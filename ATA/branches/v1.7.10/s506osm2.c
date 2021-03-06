/**************************************************************************
 *
 * SOURCE FILE NAME = S506OSM2.C
 *
 * DESCRIPTIVE NAME = Outer State Machine - Module 2
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 * Purpose:  Functions are implemented this OSM Module:
 *
 *		 S506OSM2.C - IORB processing for
 *				- IOCC_DEVICE_CONTROL
 *				  - IOCM_SUSPEND
 *				  - IOCM_RESUME
 *			      ACB Activation/Deactivation
 *			      DMA Buffer Allocate/Deallocate
 *			      Motor Idle Watchdog routine
 ****************************************************************************/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#include "os2.h"
#include "dskinit.h"
#include "iorb.h"
#include "dhcalls.h"
#include "addcalls.h"
#include "scsi.h"

#define INCL_INITRP_ONLY
#include "reqpkt.h"

#include "s506cons.h"
#include "s506type.h"
#include "s506regs.h"
#include "s506ext.h"
#include "s506pro.h"

#pragma optimize(OPTIMIZE, on)

/*------------------------------------------------------------------------*/
/*  SUSPEND FUNCTIONS							  */
/*  -----------------							  */
/*------------------------------------------------------------------------*/

/*---------------------------------------------------------*/
/*  SuspendIORBReq()					   */
/*							   */
/*  This routine handles suspend IORBs. The IORB is queued */
/*  on the HWResource structure for the UNIT it is	   */
/*  directed to.					   */
/*							   */
/*  Called from PreProcessIORBs(); this IORB is placed on  */
/*  the HWResources's suspend IORB queue.                  */
/*---------------------------------------------------------*/

VOID NEAR SuspendIORBReq (NPA npA, PIORB pNewIORB)
{
  USHORT DCFlags = ((PIORB_DEVICE_CONTROL)pNewIORB)->Flags;

  pNewIORB->pNxtIORB = 0;

  /*------------------------------------------------------------------*/
  /* If a suspend is on the suspendqueue, put imediatesuspends at the */
  /* head of the queue and defered at the end.			      */
  /*------------------------------------------------------------------*/

  DISABLE
  if (npA->pSuspendHead) {
    if (DCFlags & DC_SUSPEND_IMMEDIATE) {
      pNewIORB->pNxtIORB   = npA->pSuspendHead;
      npA->pSuspendHead  = pNewIORB;
    } else {
      npA->pSuspendFoot->pNxtIORB = pNewIORB;
      npA->pSuspendFoot = pNewIORB;
    }
  }

  /*----------------------------------*/
  /*  Put first suspend iorb on Queue */
  /*----------------------------------*/

  else {
    npA->pSuspendHead = pNewIORB;
    npA->pSuspendFoot = pNewIORB;
    npA->CountUntilSuspend = (DCFlags & DC_SUSPEND_IMMEDIATE) ? IMMEDIATE_COUNT
								: DEFERRED_COUNT;
  }
  ENABLE
}

/*-----------------------------------------------------------*/
/*  Suspend()						     */
/*							     */
/*  Process a suspend IORB.  Leave the HWResources allocated */
/*  and the state machine active and waited, just like	     */
/*  during interrupt processing.  Notify the requestor the   */
/*  suspend request was approved.			     */
/*-----------------------------------------------------------*/
VOID NEAR Suspend (NPA npA)
{
  if (npA->npHWR->npOwnerACB == npA) {
    npA->SuspendIRQaddr = ((PIORB_DEVICE_CONTROL)(npA->pIORB))->IRQHandlerAddress;
  } else {
    npA->IORBStatus |= IORB_ERROR;
    npA->IORBError   = IOERR_CMD_SYNTAX;
  }

  /*
  ** Goto sleep in the ACBS_SUSPEND state and wait for a resume
  ** IORB.  The HW Resource remains allocated during a suspend.
  ** Note the ACBF_SM_ACTIVE flag is NOT being turned off
  ** while suspended.  This is to prevent any new IORB requests
  ** from restarting the state machine.  New requests are queued.
  */
  npA->Flags |= ACBF_WAITSTATE;
  npA->npHWR->Flags |= HWRF_SUSPENDED;
  npA->State	= ACBS_SUSPEND;

  IORBDone (npA);
}

/*--------------------------------------------------------------*/
/*  SuspendState()						*/
/*								*/
/*  The state machine enters ACBS_SUSPEND state as a result of	*/
/*  processing a valid suspend request.  Once in ACBS_SUSPEND	*/
/*  state the driver can do nothing except wait for a resume	*/
/*  IORB and as such we should never reach this routine.	*/
/*  (But, you would want to know about it if you ever did!)	*/
/*--------------------------------------------------------------*/
VOID NEAR SuspendState (NPA npA)
{
  _asm int 3
}

/*--------------------------------------------------------------*/
/*  ResumeIORBReq()						*/
/*								*/
/*  Restart processing on the requested ACB's HW Resources.     */
/*  This request can always be handled immediately because	*/
/*  once suspended, all we can do is wait to be resumed.  If	*/
/*  the HW Resource is not currently suspended, it is an error. */
/*								*/
/*  Called from PreProcessIORBs(); this IORB is not placed on	*/
/*  the ACB's IORB queue.                                       */
/*								*/
/*  Restart the current ACB's state machine which can do        */
/*  nothing until the HWResources for this RESUME operation are */
/*  freed.  FreeHWResources() starts the next waiting ACB.  The */
/*  next waiting ACB cannot be the current ACB because		*/
/*  AllocateHWResources() does not get called until		*/
/*  ACBF_SM_ACTIVE is reset and the state machine actually	*/
/*  starts the IORB.  While this ACB was suspended, requests	*/
/*  were queued to the ACB's IORB queue but not to the          */
/*  HWResource's ACB queue until the state machine is started   */
/*  and it is attempted to allocate the HW resources.		*/
/*								*/
/*   StartState() -> AllocateHWResources()			*/
/*								*/
/*  The state machine remains active (ACBF_WAITSTATEed) while	*/
/*  in the ACBS_SUSPEND state we are currently terminating.	*/
/*  This is the same model followed for interrupt state.   The	*/
/*  difference between suspend and interrupt states is that	*/
/*  when interrupt state is continued there may be more work to */
/*  do but when a suspend state is resumed there is no more	*/
/*  work to complete the current operation.  Hence the state	*/
/*  can be forced to ACBS_START.				*/
/*--------------------------------------------------------------*/
VOID NEAR ResumeIORBReq (NPA npA, PIORB pIORB)
{
  if (npA->npHWR->Flags & HWRF_SUSPENDED) {
    FreeHWResources (npA);
    npA->State	= ACBS_START;
    npA->Flags &= ~ACBF_SM_ACTIVE;
  } else { /* the HW Resource was not suspended */
    pIORB->ErrorCode = IOERR_CMD_SYNTAX;
    pIORB->Status   |= IORB_ERROR;
  }
}

/*-------------------------------------------------------*/
/* PreProcessIORBs					 */
/*							 */
/* This filters incomming IORBs as they are passed	 */
/* to the ADD driver.					 */
/*							 */
/* Certain IORBs such as RESUME are removed		 */
/* from the IORB stream and routed directly to their	 */
/* handling routines.					 */
/*							 */
/* The remaining IORBs are placed on the device queue.	 */
/*							 */
/* SUSPEND IORBs are placed on the HWResource suspend	 */
/* queue.						 */
/*							 */
/*-------------------------------------------------------*/
PIORB NEAR PreProcessIORBs (NPA npA, NPU npU, PPIORB ppFirstIORB)
{
  PIORB  pIORB, pIORBPrev, pIORBNext;
  USHORT Cmd;

#define CmdCode (Cmd >> 8)
#define CmdModifier (UCHAR)(Cmd & 0xFF)

  pIORB     = *ppFirstIORB;
  pIORBPrev = 0;

  do {
    pIORBNext = (pIORB->RequestControl & IORB_CHAIN) ? pIORB->pNxtIORB : 0;
    Cmd = REQ (pIORB->CommandCode, pIORB->CommandModifier);

    if ((Cmd == REQ (IOCC_ADAPTER_PASSTHRU, IOCM_EXECUTE_CDB)) ||
	(CmdCode == IOCC_CONFIGURATION) ||
	(CmdCode == IOCC_RESOURCE)	||
	(CmdCode == IOCC_DEVICE_CONTROL)) {

      if (pIORBPrev) {
	pIORBPrev->pNxtIORB = pIORBNext;
      } else {
	*ppFirstIORB = pIORBNext;
      }

      if (CmdCode == IOCC_RESOURCE) Cmd = 10 + CmdModifier;
      if (CmdCode == IOCC_CONFIGURATION) Cmd = 8 + CmdModifier;
      if (CmdCode == IOCC_ADAPTER_PASSTHRU) {
	PCHAR SCSICmd = ((PIORB_ADAPTER_PASSTHRU)pIORB)->pControllerCmd;

	Cmd = 0;
	if ((SCSICmd[0] == SCSI_START_STOP_UNIT) && (SCSICmd[4] == 2))
	  Cmd = IOCM_EJECT_MEDIA;
      }

      switch (CmdModifier) {
	case 9 /* IOCM_GET_DEVICE_TABLE */ :
	  GetDeviceTable (pIORB); break;

	case 10 /* IOCM_COMPLETE_INIT */ :
	  CompleteInit (npA); break;

	case 11 /* IOCM_REPORT_RESOURCES */ :
	  GetUnitResources (npA, pIORB); break;

	case IOCM_SUSPEND :
	  SuspendIORBReq (npA, pIORB); break;

	case IOCM_RESUME :
	  ResumeIORBReq (npA, pIORB); break;

	case IOCM_GET_QUEUE_STATUS :
	  GetQueueStatus (npA, pIORB); break;

	case IOCM_EJECT_MEDIA :
	case IOCM_LOCK_MEDIA :
	case IOCM_UNLOCK_MEDIA :
	  ProcessLockUnlockEject (npU, pIORB, CmdModifier); break;

	default:
	  pIORB->Status   |= IORB_ERROR;
	  pIORB->ErrorCode = IOERR_CMD_NOT_SUPPORTED;
      }

      if (CmdModifier != IOCM_SUSPEND) {
	pIORB->Status |= IORB_DONE;

	if (pIORB->RequestControl & IORB_ASYNC_POST)
	  ((PIORBNotify)pIORB->NotifyAddress) (pIORB);
      }
      continue;
    }
    pIORBPrev = pIORB;
  } while (pIORB = pIORBNext);

  if (pIORBPrev)
    pIORBPrev->pNxtIORB = 0;

  return (pIORBPrev);
}


/*-------------------------------------------------------*/
/* AllocateHWResources					 */
/*							 */
/* This routine controls sharing of Hardware Resources,  */
/* HWResource[], between two or more controllers which	 */
/* may share these resources.				 */
/*							 */
/* If we are in a 'shared' scenario, this routine        */
/* serializes the requestors of the shared resource.	 */
/*							 */
/* A requesting ACB is added to the HWResource wait	 */
/* only if it not the current owner of the HWResource.	 */
/*							 */
/*-------------------------------------------------------*/
USHORT NEAR AllocateHWResources (NPA npA)
{
  NPHWRESOURCE npHWR;
  USHORT       rc = 0;

  npA->npNextACB = 0;

  DISABLE

  npHWR = npA->npHWR;
  /*------------------------------------------*/
  /* If we are SUSPENDED, then indicate that  */
  /* resources were not allocated to the      */
  /* requestor. 			      */
  /*------------------------------------------*/
  if (npHWR->Flags & HWRF_SUSPENDED)
    rc = 1;

  /*------------------------------------------------*/
  /* If there is no current owner for the resources */
  /* make the requesting ACB the current owner	    */
  /*------------------------------------------------*/
  else if(!npHWR->npOwnerACB)
    npHWR->npOwnerACB = npA;

  /*-----------------------------------------------*/
  /* If someone is requesting resources and they   */
  /* do not match the current owner, then they get */
  /* queued.					   */
  /*-----------------------------------------------*/
  else if (npHWR->npOwnerACB != npA)
    rc = 1;

  /*---------------------------------------------*/
  /* If rc != 0 as determined by the logic above */
  /* the requestor must be blocked.		 */
  /*						 */
  /* The requesting ACB is placed on a queue for */
  /* the resource.				 */
  /*---------------------------------------------*/
  if (rc) {
    if (npHWR->npFootACB)
      npHWR->npFootACB->npNextACB = npA;
    else
      npHWR->npHeadACB = npA;
    npHWR->npFootACB = npA;

    npA->Flags |= ACBF_WAITSTATE;
  }

  ENABLE

  return (rc);
}


/*--------------------------------------------------*/
/* FreeHWResources				    */
/*						    */
/* This routine releases Hardware Resources owned   */
/* by an ACB.					    */
/*						    */
/* If there are other ACBs waiting and an Immediate */
/* Suspend is not pending, then restart processing  */
/* of the queued requestor for the resources.	    */
/*						    */
/*--------------------------------------------------*/
NPA NEAR FreeHWResources (NPA npA)
{
  NPHWRESOURCE npHWR = npA->npHWR;

  DISABLE

  npA->SuspendIRQaddr = 0L;
  npHWR->Flags &= ~HWRF_SUSPENDED;

  if (npHWR->npOwnerACB = npHWR->npHeadACB) {
    if (npHWR->npHeadACB = npHWR->npOwnerACB->npNextACB)
      npHWR->npOwnerACB->npNextACB = 0;
    else
      npHWR->npFootACB = 0;

    /*
    ** Start the state machine for the next waiting ACB.  Set the
    ** SM_ACTIVE flag because this may be starting an ACB that is
    ** currently ACBF_WAITSTATEed.
    */
    npHWR->npOwnerACB->Flags |= ACBF_SM_ACTIVE;
    ENABLE

    StartSM (npHWR->npOwnerACB);
  }
  ENABLE

  return (npHWR->npOwnerACB);
}
