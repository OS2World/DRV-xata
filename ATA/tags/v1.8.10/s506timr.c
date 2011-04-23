/**************************************************************************
 *
 * SOURCE FILE NAME = S506TIMR.C
 *
 * DESCRIPTIVE NAME = DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
*
 * DESCRIPTION : Adapter Driver timer driven entry points.
 ****************************************************************************/

 #define INCL_NOBASEAPI
 #define INCL_NOPMAPI
 #define INCL_NO_SCB
 #define INCL_INITRP_ONLY
 #define INCL_DOSERRORS
 #include "os2.h"
 #include "dskinit.h"
 #include "iorb.h"
 #include "reqpkt.h"
 #include "dhcalls.h"
 #include "addcalls.h"

 #include "s506cons.h"
 #include "s506type.h"
 #include "s506regs.h"
 #include "s506ext.h"
 #include "s506pro.h"

#pragma optimize(OPTIMIZE, on)

#define PCITRACER 0
#define TRPORT 0xA800

#define npA ((NPA)pA)

//
// IRQTimer()
//
// Expected interrupt timeout routine.
//

VOID FAR _cdecl IRQTimer (USHORT TimerHandle, PACB pA)
{
  ADD_CancelTimer (TimerHandle);

  npA->IRQTimerHandle = 0;
  npA->TimerFlags    |= ACBT_IRQ;

#if PCITRACER
  outpw (TRPORT, 0xAF00 | (npA->State & ACBS_WAIT));
#endif
  if (npA->State & ACBS_WAIT) {
    // The device may be ready to transfer data but failed to interrupt.  If so
    // just continue.
    NPU npU = npA->npU;

    METHOD(npA).StopDMA (npA);	/* controller is locked, Clear Active bit */

    if (SERROR && InD (SERROR)) {
      IssueSATAReset (npU);
      npA->TimerFlags |= ACBT_SATACOMM;
      npA->State       = ACBS_ERROR;
      ReInitUnit (npU);

    } else if (npA->BM_CommandCode & BMICOM_START) {
      USHORT PCIStatus;

      PciGetReg (npA->PCIInfo.PCIAddr, PCIREG_STATUS, (PULONG)&PCIStatus, 2);
      if (PCIStatus & PCI_STATUS_MASTER_ABORT) {
	PciSetReg (npA->PCIInfo.PCIAddr, PCIREG_STATUS, PCI_STATUS_MASTER_ABORT, 2);
	npA->npU->Flags &= ~UCBF_BM_DMA;
	npA->State = ACBS_ERROR;
      } else if (CheckBusy (npA)) {
	npA->State = ACBS_ERROR;
      }
    } else {
      if (CheckBusy (npA)) {
	if (npA->ElapsedTime < IRQ_TIMEOUT_INTERVAL) {
	  ADD_StartTimerMS (&npA->IRQTimerHandle, npA->IRQTimeOut,
			   (PFN)IRQTimer, npA);
	  return;
	}
      }
    }
    StartSM (npA);
  }
}


/*---------------------------------------------*/
/* DelayedRetry 			       */
/* ------------ 			       */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/

VOID FAR _cdecl DelayedRetry (USHORT hRetryTimer, PACB pA)
{
  ADD_CancelTimer (hRetryTimer);
  npA->RetryTimerHandle = 0;

  npA->TimerFlags = 0;
  npA->State = ACBS_RETRY;
  StartSM (npA);
}

/*---------------------------------------------*/
/* DelayedReset 			       */
/* ------------ 			       */
/*					       */
/* This routine restarts the state machine to  */
/* determine if a reset has completed.	       */
/*					       */
/*---------------------------------------------*/

VOID FAR _cdecl DelayedReset (USHORT hResetTimer, PACB pA)
{
  ADD_CancelTimer (hResetTimer);
  npA->ResetTimerHandle = 0;

  npA->State = ACBS_PATARESET;
  StartSM (npA);
}

#undef npA

/*---------------------------------------------*/
/* ElapsedTimer 			       */
/* ------------ 			       */
/*					       */
/*					       */
/*---------------------------------------------*/

VOID FAR _cdecl ElapsedTimer (USHORT hElapsedTimer, ULONG Unused)
{
  NPA npA;
  NPU npU;
  UCHAR i;

  for (i = 0; i < MAX_ADAPTERS; i++) {
    if ((npA = ACBPtrs[i]) != NULL) {
      npA->ElapsedTime += ELAPSED_TIMER_INTERVAL;

#if 0
      // check SATA PHY for ready/not ready changes

      if ((npA->FlagsT & ATBF_BAY) && SSTATUS) {
	UCHAR Status = InD (SSTATUS) & SSTAT_DET;
	if (npU->Flags & UCBF_PHYREADY) {
	  if (Status != SSTAT_COM_OK) {
	    npU->Flags &= ~UCBF_PHYREADY;
	    SATARemoval (npU);
	  }
	} else {
	  if (Status == SSTAT_COM_OK) {
	    npU->Flags |= UCBF_PHYREADY;
	    StartDetectSATA (npU);
	  }
	}
      }
#endif
    }
  }
}

/*---------------------------------------------*/
/* IdleTicker				       */
/* ----------				       */
/*					       */
/*					       */
/*---------------------------------------------*/

VOID FAR _cdecl IdleTicker (USHORT hTimer, ULONG Unused)
{
  NPA npA;
  NPU npU;
  UCHAR i;

  for (i = 0; i < MAX_ADAPTERS; i++) {
    if ((npA = ACBPtrs[i]) != NULL) {
      for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->cUnits); npU++) {
	if (npU->IdleCounter > 0) {
	  if (--npU->IdleCounter == 0) npU->LongTimeout = TRUE;
	}
      }
    }
  }
}

/*------------------------------------*/
/*				      */
/*				      */
/*				      */
/*------------------------------------*/

VOID NEAR _fastcall IODly (USHORT Count)
{
  _asm { mov cx, ax
	 inc cx
      l:
	 loop l }
}

VOID FAR _fastcall IODlyFar (USHORT Count) {
  _asm { mov cx, ax
	 inc cx
      l:
	 loop l }
}

