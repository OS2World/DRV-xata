/**************************************************************************
 *
 * SOURCE FILE NAME = S506OSM2.C
 *
 * DESCRIPTIVE NAME = Outer State Machine - Module 2
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2007
 *
 * Purpose:  Functions are implemented this OSM Module:
 *
 *		 S506OSM2.C - IORB processing for
 *				- IOCC_DEVICE_CONTROL
 *				  - IOCM_SUSPEND
 *				  - IOCM_RESUME
 *			      ACB Activation/Deactivation
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

VOID NEAR SuspendIORBReq (NPA npA, PIORB pIORB)
{
  USHORT DCFlags = ((PIORB_DEVICE_CONTROL)pIORB)->Flags;

  pIORB->pNxtIORB = 0;

  DISABLE
  if (npA->pSuspendHead) {

    /*------------------------------------------------------------------*/
    /* If a suspend is on the suspendqueue, put imediatesuspends at the */
    /* head of the queue and defered at the end.			*/
    /*------------------------------------------------------------------*/

    if (DCFlags & DC_SUSPEND_IMMEDIATE) {
      pIORB->pNxtIORB	= npA->pSuspendHead;
      npA->pSuspendHead = pIORB;
    } else {
      npA->pSuspendFoot->pNxtIORB = pIORB;
      npA->pSuspendFoot = pIORB;
    }
  } else {

    /*----------------------------------*/
    /*	Put first suspend iorb on Queue */
    /*----------------------------------*/

    npA->pSuspendHead = pIORB;
    npA->pSuspendFoot = pIORB;
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
  npA->SuspendIRQaddr = ((PIORB_DEVICE_CONTROL)(npA->pIORB))->IRQHandlerAddress;

  /*
  ** Goto sleep in the ACBS_SUSPEND state and wait for a resume
  ** IORB.  The HW Resource remains allocated during a suspend.
  ** Note the ACBF_SM_SUSPENDED flag is NOT being turned on
  ** while suspended.  This is to prevent any new IORB requests
  ** from restarting the state machine.  New requests are queued.
  */
  npA->FlagsT |= ATBF_SUSPENDED;
  npA->State   = ACBS_SUSPEND | ACBS_WAIT;

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
/*  ACBF_SM_SUSPENDED is reset and the state machine actually	   */
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
  if (npA->FlagsT & ATBF_SUSPENDED) {
    npA->FlagsT &= ~ATBF_SUSPENDED;

    ((NPUSHORT)&(npA->SuspendIRQaddr))[1] = 0;

    npA->State	= ACBS_START | ACBS_SUSPENDED;

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
PIORB NEAR PreProcessIORBs (NPA npA, PPIORB ppIORBHead)
{
  PIORB  pIORB, pIORBTail, pIORBNext;
  USHORT Cmd;

  pIORBTail = 0;
  for (pIORB = *ppIORBHead; pIORB; pIORB = pIORBNext) {

    UCHAR notify = TRUE;
    pIORBNext = (pIORB->RequestControl & IORB_CHAIN) ? pIORB->pNxtIORB : 0;
    Cmd = REQ (pIORB->CommandCode, pIORB->CommandModifier);

    switch (Cmd) {
      case REQ (IOCC_CONFIGURATION, IOCM_GET_DEVICE_TABLE) :
	GetDeviceTable (pIORB); goto done;

      case REQ (IOCC_CONFIGURATION, IOCM_COMPLETE_INIT) :
	CompleteInit (npA); goto done;

      case REQ (IOCC_RESOURCE, IOCM_REPORT_RESOURCES) :
	GetUnitResources (npA, pIORB); goto done;

      case REQ (IOCC_DEVICE_CONTROL, IOCM_SUSPEND) :
	SuspendIORBReq (npA, pIORB); notify = FALSE; goto done;

      case REQ (IOCC_DEVICE_CONTROL, IOCM_RESUME) :
	ResumeIORBReq (npA, pIORB); goto done;

      case REQ (IOCC_DEVICE_CONTROL, IOCM_GET_QUEUE_STATUS) :
	GetQueueStatus (npA, pIORB);

      done:
	/* this IORB is done, remove it from chain of unprocessed IORBs */
	if (pIORBTail) {
	  pIORBTail->pNxtIORB = pIORBNext;
	} else {
	  *ppIORBHead = pIORBNext;
	}

	/* done processing, optionally notify */
	if (notify) {
	  pIORB->Status |= IORB_DONE;

	  if (pIORB->RequestControl & IORB_ASYNC_POST)
	    ((PIORBNotify)pIORB->NotifyAddress) (pIORB);
	}

	continue;
    }

    /* this IORB was not processed, keep it as running tail of unprocessed IORBs */
    pIORBTail = pIORB;
  }

  if (pIORBTail)
    pIORBTail->pNxtIORB = 0;

  return (pIORBTail);
}

