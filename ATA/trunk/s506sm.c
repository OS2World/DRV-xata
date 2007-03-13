/**************************************************************************
 *
 * SOURCE FILE NAME = S506SM.C
 *
 * DESCRIPTIVE NAME = DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2007
 *
 * DESCRIPTION : I/O State Machine for ATA Adapter Driver
 *
 ****************************************************************************/

 #define INCL_NOPMAPI
 #define INCL_NO_SCB
 #define INCL_INITRP_ONLY
 #define INCL_DOSINFOSEG
 #include "os2.h"
 #include <stddef.h>
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

#undef TRACES
#define TRACES 0
#include "Trace.h"

#pragma optimize(OPTIMIZE, on)

#define PCITRACER 0
#define TRPORT 0xD020

#if 0
void IBeep (USHORT freq) {
  USHORT i;
  DevHelp_Beep (freq, 10);
  for (i = 0xFFFF;i != 0; i--)
    IODly (IODelayCount);
}
#endif

/*---------------------------------------------*/
/*  StartSM()				       */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/
VOID NEAR StartSM (NPA npA)
{
  /*-------------------------------------------------*/
  /* ACB Use Count Checks			     */
  /* --------------------			     */
  /* The state machine is reentrant on a per ACB     */
  /* basis.					     */
  /*-------------------------------------------------*/

#if PCITRACER
  outpw (TRPORT, 0xDE60);
#endif

  if (0 == saveINC (&(npA->UseCount))) {
    do {
      do {
	npA->State &= ~ACBS_WAIT;

	switch (npA->State & ACBS_STATEMASK) {
	  case ACBS_START:
	    StartState (npA);
	    break;
	  case ACBS_INTERRUPT:
	    TERR(npA->npU,0xAA)
	    InterruptState(npA);
	    break;
	  case ACBS_DONE:
	    DoneState(npA);
	    break;
	  case ACBS_SUSPEND:
	    SuspendState (npA);
	    break;
	  case ACBS_RETRY:
	    TERR(npA->npU,0xBB)
	    RetryState (npA);
	    break;
	  case ACBS_PATARESET:
	    ResetCheck (npA);
	    break;
	  case ACBS_ERROR:
	    TERR(npA->npU,0xEE)
	    ErrorState (npA);
	    break;
	}
      } while (!(npA->State & ACBS_WAIT));
    } while (saveDEC (&(npA->UseCount)));
  }

#if PCITRACER
  outpw (TRPORT, 0xDE61);
#endif

  _asm {
    pop si
    leave
    sti
    ret
  }
}

#undef PCITRACER
#define PCITRACER 0

#if 0
/*---------------------------------------------*/
/* FixedIRQ0, FixedIRQ1, FixedIRQ0, FixedIRQ1  */
/* ------------------------------------------  */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/

USHORT FAR FixedIRQ0() {
  return (HandleInterrupts (IHdr[0].npA) >> 1);
}

USHORT FAR FixedIRQ1() {
  return (HandleInterrupts (IHdr[1].npA) >> 1);
}

USHORT FAR FixedIRQ2() {
  return (HandleInterrupts (IHdr[2].npA) >> 1);
}

USHORT FAR FixedIRQ3() {
  return (HandleInterrupts (IHdr[3].npA) >> 1);
}

USHORT FAR FixedIRQ4() {
  return (HandleInterrupts (IHdr[4].npA) >> 1);
}

USHORT FAR FixedIRQ5() {
  return (HandleInterrupts (IHdr[5].npA) >> 1);
}

USHORT FAR FixedIRQ6() {
  return (HandleInterrupts (IHdr[6].npA) >> 1);
}

USHORT FAR FixedIRQ7() {
  return (HandleInterrupts (IHdr[7].npA) >> 1);
}

USHORT NEAR HandleInterrupts (NPA npA) {
  USHORT Claimed = 0;

  while (npA) {
    Claimed |= npA->IntHandler (npA);
    npA = npA->npIntNext;
  }
  return (~Claimed);
}
#endif

USHORT NEAR FixedInterrupt (NPA npA)
{
  USHORT Claimed = 0;
  int	 rcCheck;

#if PCITRACER
    outpw (TRPORT, 0xDEA0 | (npA->IDEChannel));
#endif
  if (!(rcCheck = METHOD(npA).CheckIRQ (npA))) {
#if PCITRACER
    outpw (TRPORT, 0xDEAA);
#endif
    return (Claimed);
  }

  if (((NPUSHORT)&(npA->SuspendIRQaddr))[1]) {
    Claimed = ~npA->SuspendIRQaddr();
  } else {
    USHORT TimerHandle;

    TimerHandle = saveXCHG (&(npA->IRQTimerHandle), 0);
    if (Claimed = rcCheck) DevHelp_EOI (npA->IRQLevel);

    if (TimerHandle) {
      ADD_CancelTimer (TimerHandle);
      StartSM (npA);
    }
  }
#if PCITRACER
    outpw (TRPORT, 0xDEF0 | (npA->IDEChannel));
#endif
  return (Claimed);
}

USHORT NEAR CatchInterrupt (NPA npA)
{
  USHORT Claimed = 0;

  if (METHOD(npA).CheckIRQ (npA)) {
    DevHelp_EOI (npA->IRQLevel);
    Claimed = 1;
  }
  return (Claimed);
}

#undef PCITRACER
#define PCITRACER 0

#define outpdelay(Port,Data) OutBd (Port,Data,npA->IODelayCount)

/*
** StartState()
**
** Route the current IORB on the ACB's IORB queue to the appropriate
** IORB handler.
**
*/
VOID NEAR StartState (NPA npA)
{
  PIORB  pIORB;
  USHORT Cmd;
  NPU	 npU;

#define CmdCode (Cmd >> 8)
#define CmdModifier (Cmd & 0xFF)

  DISABLE
  if (NextIORB (npA)) {
    /*--------------------------------------------------------*/
    /* No more IORBs so go to sleep, stay in ACBS_START, and  */
    /* mark the state machine as inactive.		      */
    /*--------------------------------------------------------*/
    npA->State = ACBS_START | ACBS_WAIT | ACBS_SUSPENDED;
    ENABLE
    return;
  }
  ENABLE

#if PCITRACER
  outpw (TRPORT, 0xDEB0);
#endif
  pIORB = npA->pIORB;

  Cmd = REQ (pIORB->CommandCode, pIORB->CommandModifier);

#if PCITRACER
  outpw (TRPORT, Cmd);
#endif

  npU = npA->npU = (NPU)pIORB->UnitHandle;

  // All I/O operations should be started with:
  //   - retrys enabled
  //   - multiple mode disabled
  // The controlling bits for these behaviors can only be changed
  // from within the state machine.
  //
  // Reset Flags bits (next 3 lines) moved to here from
  // InitACBRequest().

  npA->Flags &= ~(ACBF_DISABLERETRY | ACBF_MULTIPLEMODE);
  npU->Flags &= ~UCBF_DISABLERESET;
  npA->State = ACBS_DONE; // default to done

  switch (Cmd) {
    case REQ (IOCC_UNIT_CONTROL, IOCM_ALLOCATE_UNIT):	AllocateUnit (npA); return;
    case REQ (IOCC_DEVICE_CONTROL, IOCM_SUSPEND):	Suspend (npA);      return;
  }
  
  /*------------------------------------------------*/
  /* Check that unit is allocated for IORB commands */
  /* which require an allocated unit		    */
  /*------------------------------------------------*/
  if (!(Cmd == REQ (IOCC_ADAPTER_PASSTHRU, IOCM_EXECUTE_ATA))) {
    if (!(npU->Flags & UCBF_ALLOCATED) && !(InitActive || (pIORB->RequestControl & 0x8000))) {
      /* Unit not allocated */
      Error (npA, IOERR_UNIT_NOT_ALLOCATED);
      return;
    }
  }

#if PCITRACER
  outpw (TRPORT, 0xDEB1);
#endif
  /*-------------------------------------------*/
  /* Route the iorb to the appropriate handler */
  /*-------------------------------------------*/

  switch (Cmd) {
    case REQ (IOCC_UNIT_CONTROL, IOCM_DEALLOCATE_UNIT): DeallocateUnit (npA);  break;
    case REQ (IOCC_UNIT_CONTROL, IOCM_CHANGE_UNITINFO): ChangeUnitInfo (npA);  break;

    case REQ (IOCC_GEOMETRY, IOCM_SET_MEDIA_GEOMETRY):	SetMediaGeometry (npA);break;
    case REQ (IOCC_GEOMETRY, IOCM_GET_MEDIA_GEOMETRY):
    case REQ (IOCC_GEOMETRY, IOCM_GET_DEVICE_GEOMETRY):

    case REQ (IOCC_EXECUTE_IO, IOCM_NO_OPERATION):
    case REQ (IOCC_EXECUTE_IO, IOCM_READ):
    case REQ (IOCC_EXECUTE_IO, IOCM_READ_VERIFY):
    case REQ (IOCC_EXECUTE_IO, IOCM_WRITE):
    case REQ (IOCC_EXECUTE_IO, IOCM_WRITE_VERIFY):

    case REQ (IOCC_ADAPTER_PASSTHRU, IOCM_EXECUTE_ATA):

    case REQ (IOCC_UNIT_STATUS, IOCM_GET_UNIT_STATUS):	    StartIO (npA);	 break;
    case REQ (IOCC_UNIT_STATUS, IOCM_GET_LOCK_STATUS):	    GetLockStatus (npA); break;
    case REQ (IOCC_UNIT_STATUS, IOCM_GET_CHANGELINE_STATE): GetChangeLine (npA); break;

    default:						    Error (npA, IOERR_CMD_NOT_SUPPORTED); break;
  }
}


/*-------------------------------------------------*/
/*  StartIO()					   */
/*						   */
/*  Called from StartState() to begin processing   */
/*  an I/O operation which requires HW set-up and  */
/*  interrupt processing.			   */
/*-------------------------------------------------*/

VOID NEAR StartIO (NPA npA)
{
  NPU npU;

#if PCITRACER
  outpw (TRPORT, 0xDEB2);
#endif
  npU = (NPU)npA->npU;

  saveCLR32 (&(npA->ElapsedTime));

  if (npU->Flags & UCBF_NOTPRESENT) {
    Error (npA, IOERR_UNIT_NOT_READY);
    return;
  }

  if (METHOD(npA).StartStop) METHOD(npA).StartStop (npA, ACBS_START);

  if (!InitACBRequest (npA)) {
    if (npA->ReqFlags & ACBR_OTHERIO)
      StartOtherIO (npA);
    else if (npA->ReqFlags & ACBR_BLOCKIO)
      StartBlockIO (npA);
  }
}

/*---------------------------------------------*/
/* RetryState				       */
/* ----------				       */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/

VOID NEAR RetryState(NPA npA)
{
  if (npA->ReqFlags & ACBR_PASSTHRU)
    SetupFromATA (npA, npA->npU);

  if (npA->ReqFlags & ACBR_OTHERIO)
    StartOtherIO (npA);
  else if (npA->ReqFlags & ACBR_BLOCKIO)
    StartBlockIO (npA);
}

/*---------------------------------------------*/
/* InitACBRequest			       */
/* --------------			       */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/

USHORT NEAR InitACBRequest (NPA npA)
{
  PIORB pIORB;
  UCHAR CmdCod, CmdMod;
  NPU	npU;

  pIORB  = npA->pIORB;
  CmdCod = pIORB->CommandCode;
  CmdMod = pIORB->CommandModifier;

  npU = npA->npU;
  npU->Flags &= ~UCBF_DIAG_FAILED;

  *(NPUSHORT)&FEAT = 0;
  *(NPUSHORT)&LBA0 = 0;
  *(NPUSHORT)&LBA2 = 0;
	      LBA3 = 0;
  *(NPUSHORT)&LBA4 = 0;

  memset (&(npA->ReqFlags), 0,
	  offsetof (struct _A, cResets) + 1 - offsetof (struct _A, ReqFlags));
  memset (&(npA->IOSGPtrs.numTotalBytes), 0, 16);

  if (pIORB->RequestControl & IORB_DISABLE_RETRY)
    npA->Flags |= ACBF_DISABLERETRY;

  /*------------------------------------------------------*/
  /* Translate IORB Command Code/Modifier to ACBR_* flags */
  /*------------------------------------------------------*/
  if (CmdCod == IOCC_EXECUTE_IO) {
    if ((npU->Flags & (UCBF_REMOVABLE | UCBF_READY)) == UCBF_REMOVABLE) {
      Error (npA, IOERR_UNIT_NOT_READY);
      return (TRUE);
    }

    if ((CmdMod == IOCM_WRITE) || (CmdMod == IOCM_WRITE_VERIFY)) {
      if (npU->WriteProtect) {
	if ((npU->WriteProtect > 1) ||
	    ((((PIORB_EXECUTEIO)pIORB)->RBA + ((PIORB_EXECUTEIO)pIORB)->BlockCount) < npU->LogGeom.SectorsPerTrack)) {
	  npA->IORBStatus &= ~IORB_RECOV_ERROR;
	  npA->IORBStatus |= IORB_DONE;
	  npA->IORBError = 0;
	  return (TRUE);
	}
      }

      npA->IOSGPtrs.Mode = SGLIST_TO_PORT;

      ++npU->DeviceCounters.TotalWriteOperations;
      npU->DeviceCounters.TotalSectorsWritten +=
	(ULONG)(((PIORB_EXECUTEIO)pIORB)->BlockCount);

      if (npU->CmdSupported & UCBS_WCACHE) {
	if (npU->FlushCount >= 0) npU->FlushCount--;
	if ((npU->FlushCount == 0) && !(npU->Flags & UCBF_WCACHEENABLED))
	  npU->ReqFlags |= UCBR_ENABLEWCACHE;
      }
    } else { // read operation

      npA->IOSGPtrs.Mode = PORT_TO_SGLIST;

      ++npU->DeviceCounters.TotalReadOperations;
      npU->DeviceCounters.TotalSectorsRead +=
	(ULONG)(((PIORB_EXECUTEIO)pIORB)->BlockCount);
    }

    switch (CmdMod) {
      case IOCM_READ:
	npA->ReqFlags |= npU->ReqFlags | ACBR_READ;
	break;

      case IOCM_WRITE:
	npA->ReqFlags |= npU->ReqFlags | ACBR_WRITE;
	break;

      case IOCM_WRITE_VERIFY:
	npA->ReqFlags |= npU->ReqFlags | (ACBR_WRITEV | ACBR_WRITE);
	break;

      case IOCM_READ_VERIFY:
	if (npU->CmdSupported & UCBS_WCACHE) {
	  if (npU->Flags & UCBF_WCACHEENABLED) {
	    npU->ReqFlags |=  UCBR_DISABLEWCACHE;
	    npU->ReqFlags &= ~UCBR_ENABLEWCACHE;
	  }
	  npU->FlushCount = 10;
	}
	npA->ReqFlags |= npU->ReqFlags | ACBR_VERIFY;
	break;

      case IOCM_NO_OPERATION:
	npA->ReqFlags |= npU->ReqFlags;
	break;
    }
  } else if (CmdCod == IOCC_ADAPTER_PASSTHRU) {
    if (!SetupFromATA (npA, npU)) {
      //
      // unsupported passthru command
      //
      Error (npA, IOERR_CMD_NOT_SUPPORTED);
      return (TRUE);
    }
  } else if (CmdCod == IOCC_UNIT_STATUS) {
    switch (CmdMod) {
      case IOCM_GET_UNIT_STATUS:
	/*
	** Setup a seek to see if there is a drive out there.
	*/
	SetupSeek (npA, npU);
	break;
    }
  } else if (CmdCod == IOCC_GEOMETRY) {
    switch (CmdMod) {
      case IOCM_GET_MEDIA_GEOMETRY:
      case IOCM_GET_DEVICE_GEOMETRY:
	if ((npU->Flags & (UCBF_REMOVABLE | UCBF_ATAPIDEVICE)) == UCBF_REMOVABLE) {
	  SetupIdentify (npA, npU);
	  break;
	} else {
	  return (TRUE);
	}
    }
  }

  /*-----------------------------------------------*/
  /* Set the S/G pointers for Block IO operations  */
  /*-----------------------------------------------*/
  if (npA->ReqFlags & ACBR_BLOCKIO)
    InitBlockIO (npA);

  return (FALSE);
}


BOOL NEAR SetupFromATA (NPA npA, NPU npU)
{
  PPassThruATA		 picp;
  PIORB_ADAPTER_PASSTHRU piorb_pt;
  USHORT		 Map;

  /* Setup pointers to IORB and IssueCommandParameters Structures */

  piorb_pt = (PIORB_ADAPTER_PASSTHRU) npA->pIORB;
  picp = (PPassThruATA) piorb_pt->pControllerCmd;

  Map = picp->RegisterMapW;
  if (!(Map & RTM_COMMAND)) {
    return (FALSE);
  } else if (picp->TaskFileIn.Command == FX_SMARTCMD) {
    if (picp->TaskFileIn.Features == 0xD7) {
      return (FALSE);
    }
  }

  npA->IOSGPtrs.cSGList = piorb_pt->cSGList;
  npA->IOSGPtrs.pSGList = piorb_pt->pSGList;

  // copy caller supplied task file register info

  if (Map & RTM_FEATURES) FEAT	 = picp->TaskFileIn.Features;
  if (Map & RTM_SECCNT	) SECCNT = picp->TaskFileIn.SectorCount;
  if (Map & RTM_LBA0	) LBA0	 = picp->TaskFileIn.Lba0_SecNum;
  if (Map & RTM_LBA1	) LBA1	 = picp->TaskFileIn.Lba1_CylLo;
  if (Map & RTM_LBA2	) LBA2	 = picp->TaskFileIn.Lba2_CylHi;
  if (Map & RTM_LBA3	) LBA3	 = picp->TaskFileIn.Lba3;
  if (Map & RTM_LBA4	) LBA4	 = picp->TaskFileIn.Lba4;
  if (Map & RTM_LBA5	) LBA5	 = picp->TaskFileIn.Lba5;
			 COMMAND = picp->TaskFileIn.Command;
  DRVHD   = npU->DriveHead;
  npA->IOPendingMask = Map | FM_PDRHD;

  // initialize mode, don't know yet
  npA->IOSGPtrs.Mode = 0;

  // if we have a scatter/gather list this is a DATA command
  if (piorb_pt->cSGList) {
    // scatter gather list is in SECTOR format

    npA->SecRemain	 = piorb_pt->cSGList;
    npA->SecToGo	 = piorb_pt->cSGList;
    npA->SecReq 	 = npA->SecToGo;
    npA->BytesToTransfer = piorb_pt->cSGList * 512;
    npA->SecPerInt	 = 1;

    if (piorb_pt->Flags & PT_DIRECTION_IN) {
      // if the IROB says inbound then set this as a READ command

      npA->IOSGPtrs.Mode = PORT_TO_SGLIST;
      if ((picp->TaskFileIn.Command == FX_IDENTIFY) ||
	  (picp->TaskFileIn.Command == FX_ATAPI_IDENTIFY))
	npA->IOSGPtrs.Mode |= 0x40; // slow!

      npA->ReqFlags |= npU->ReqFlags | ACBR_READ | ACBR_PASSTHRU;
    } else {
      // IORB doesn't say IN, so MUST be OUT (with scatter gather list set)

      npA->IOSGPtrs.Mode = SGLIST_TO_PORT;
      npA->ReqFlags |= npU->ReqFlags | ACBR_WRITE | ACBR_PASSTHRU;
    }
  } else {
    npA->ReqFlags |= npU->ReqFlags | ACBR_NONDATA | ACBR_PASSTHRU;
  }

  return (TRUE);
}


/*
** Setup to perform a seek to the first sector on the drive.
*/
BOOL NEAR SetupSeek (NPA npA, NPU npU)
{
  npA->IOSGPtrs.cSGList = 0;
  npA->IOSGPtrs.pSGList = NULL;

  SECTOR	     = 1;
  COMMAND	     = FX_SEEK;
  npA->IOPendingMask = (FM_PSECCNT | FM_PCYLL | FM_PCYLH | FM_PDRHD | FM_PCMD);

  npA->ReqFlags = ACBR_NONDATA;
  npA->Flags |= ACBF_DISABLERETRY;
  npU->Flags |= UCBF_DISABLERESET;

  return (TRUE);
}


/*
** Setup to identify device command
*/
BOOL NEAR SetupIdentify (NPA npA, NPU npU)
{
  npA->IdentifySGList.ppXferBuf  = ppDataSeg + (USHORT)&(npA->IdentifyBuf);
  npA->IdentifySGList.XferBufLen = 512;
  npA->IOSGPtrs.pSGList = &(npA->IdentifySGList);
  npA->IOSGPtrs.cSGList = 1;
  npA->SecRemain	= 1;
  npA->SecToGo		= 1;
  npA->SecReq		= 1;
  npA->SecPerInt	= 1;
  npA->BytesToTransfer	= 512;

  DRVHD   = npU->DriveHead;
  COMMAND = FX_IDENTIFY;
  npA->IOPendingMask = FM_PDRHD | FM_PCMD;

  npA->IOSGPtrs.Mode = PORT_TO_SGLIST | 0x40; // slow!
  npA->ReqFlags |= ACBR_READ | ACBR_NONPASSTHRU;

  return (TRUE);
}


/*---------------------------------------------*/
/* InitBlockIO				       */
/* ------------ 			       */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/

VOID NEAR InitBlockIO (NPA npA)
{
  PIORB_EXECUTEIO pIORB = (PIORB_EXECUTEIO) npA->pIORB;

#if PCITRACER
  outpw (TRPORT, 0xDEB3);
#endif

  // if NOT a Passthru command
  // then setup
  // PASSTHRU requests are setup already
  if (!(npA->ReqFlags & ACBR_NONBLOCKIO)) {
    npA->IOSGPtrs.cSGList = pIORB->cSGList;
    npA->IOSGPtrs.pSGList = pIORB->pSGList;

    npA->RBA = pIORB->RBA;
    if (npA->npU->Flags & UCBF_ONTRACK) npA->RBA += ONTRACK_SECTOR_OFFSET;

    npA->SecRemain = pIORB->BlockCount;

    /* Calculate total bytes in transfer request */

    npA->BytesToTransfer = (ULONG) pIORB->BlockCount * (ULONG) pIORB->BlockSize;
    pIORB->BlocksXferred   = 0;
  }
}

/*---------------------------------------------*/
/* StartOtherIO 			       */
/* ------------ 			       */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/

USHORT NEAR StartOtherIO (NPA npA)
{
  USHORT ReqFlags;  // USHORT is ok here!
  NPU	 npU = npA->npU;
  USHORT Data;
  UCHAR  longWait = FALSE;
  USHORT i;

#if PCITRACER
  outpw (TRPORT, 0xDEB4);
  outpw (TRPORT, npA->ReqFlags);
#endif
  if (!(npA->ReqFlags & ACBR_PASSTHRU)) {
    FEAT = 0;
    npA->IOPendingMask |= FM_PFEAT | FM_PDRHD | FM_PCMD;
  } else {
    npA->IOPendingMask |= FM_PDRHD | FM_PCMD;
    if (COMMAND == FX_EJECT_MEDIA) {
      longWait = TRUE;
      npU->Flags &= ~UCBF_READY;
      npU->Flags |= UCBF_BECOMING_READY;
    }
  }

  DRVHD    = npU->DriveHead;
  ReqFlags = npA->ReqFlags;

  if (ReqFlags & ACBR_RESETCONTROLLER) {
    npA->ReqMask = ACBR_RESETCONTROLLER;
    DoReset (npA);
    goto StartOtherIOExit;
  }

  if (ReqFlags & ACBR_READMAX) {
    COMMAND		= (npU->CmdSupported >> 16) & FX_LBA48SUPPORTED ?
			  FX_READMAXEXT : FX_READMAX;
    npA->ReqMask	= ACBR_READMAX;

  } else if (ReqFlags & ACBR_SETMAXNATIVE) {
    SECCNT		= 0;
    if ((npU->CmdSupported >> 16) & FX_LBA48SUPPORTED) {
      COMMAND		= FX_SETMAXEXT;
      npA->IOPendingMask |= FM_HIGH;
    } else {
      COMMAND		= FX_SETMAX;
      DRVHD	       |= LBA3 & 0x0F;
      npA->IOPendingMask &= ~FM_LBA3;
    }
    npA->IOPendingMask |= FM_PSECCNT | FM_LBA0 | FM_LBA1 | FM_LBA2;
    npA->ReqMask	= ACBR_SETMAXNATIVE;

  } else if (ReqFlags & ACBR_SETPARAM) {
    outpdelay (DEVCTLREG, DEVCTL);
    COMMAND		= FX_SETP;
    SECCNT		= npU->PhysGeom.SectorsPerTrack;
    DRVHD	       |= 0x0F & (npU->PhysGeom.NumHeads - 1);
    npA->IOPendingMask |= FM_PSECCNT;
    npA->ReqMask	= ACBR_SETPARAM;

  } else if (ReqFlags & ACBR_SETMULTIPLE) {
    COMMAND		= FX_SETMUL;
    SECCNT		= (UCHAR) npU->SecPerBlk;
    npA->IOPendingMask |= FM_PSECCNT;
    npA->ReqMask	= ACBR_SETMULTIPLE;

  } else if (ReqFlags & ACBR_SETPIOMODE) {
    COMMAND		= FX_SETFEAT;
    FEAT		= FX_SETXFERMODE;
    if (npU->CurPIOMode)
      Data = npU->CurPIOMode | FX_PIOMODEX;
    else
      Data = npU->Flags & UCBF_DISABLEIORDY ? FX_PIOMODE0 : FX_PIOMODE0IORDY;
    SECCNT		= Data;
    npA->IOPendingMask |= FM_PFEAT | FM_PSECCNT;
    npA->ReqMask	= ACBR_SETPIOMODE;

  } else if (ReqFlags & ACBR_SETDMAMODE) {
    COMMAND		= FX_SETFEAT;
    FEAT		= FX_SETXFERMODE;
    Data = npU->CurDMAMode;
    if (Data >= 3)
      Data = (npU->UltraDMAMode - 1) | FX_ULTRADMAMODEX;
    else
      Data |= FX_MWDMAMODEX;
    SECCNT		= Data;
    npA->IOPendingMask |= FM_PFEAT | FM_PSECCNT;
    npA->ReqMask	= ACBR_SETDMAMODE;

  } else if (ReqFlags & ACBR_SETIDLETIM) {
    COMMAND		= FX_SETIDLE;
    SECCNT		= (UCHAR)(~(npU->IdleTime));
    npA->IOPendingMask |= FM_PSECCNT;
    npA->ReqMask	= ACBR_SETIDLETIM;

  } else if (ReqFlags & ACBR_ENABLEWCACHE) {
    COMMAND		= FX_SETFEAT;
    FEAT		= FX_ENABLE_WCACHE;
    npA->IOPendingMask |= FM_PFEAT;
    npU->Flags	       |= UCBF_WCACHEENABLED;
    npA->ReqMask	= ACBR_ENABLEWCACHE;

  } else if (ReqFlags & ACBR_DISABLEWCACHE) {
    COMMAND		= FX_SETFEAT;
    FEAT		= FX_DISABLE_WCACHE;
    npA->IOPendingMask |= FM_PFEAT;
    npU->Flags	       &=~UCBF_WCACHEENABLED;
    npA->ReqMask	= ACBR_DISABLEWCACHE;

  } else if (ReqFlags & ACBR_ENABLERAHEAD) {
    COMMAND		= FX_SETFEAT;
    FEAT		= FX_ENABLE_RAHEAD;
    npA->IOPendingMask |= FM_PFEAT;
    npA->ReqMask	= ACBR_ENABLERAHEAD;

  } else if (ReqFlags & ACBR_FREEZELOCK) {
    COMMAND		= FX_FREEZELOCK;
    npA->ReqMask	= ACBR_FREEZELOCK;

  } else if (ReqFlags & ACBR_NONDATA) {
    npA->ReqMask = ACBR_NONDATA;
  }

  /*--------------------------------------*/
  /* Set the Next State fields in the ACB */
  /*--------------------------------------*/
  npA->Flags &= ~ACBF_BMINT_SEEN;
  npA->State  = ACBS_INTERRUPT | ACBS_WAIT;

  /*-----------------------------------------*/
  /* Start the IRQ timer and output the copy */
  /* of the TASK File Regs in the ACB to the */
  /* controller.			     */
  /*-----------------------------------------*/
  longWait |= (npU->Flags & (UCBF_REMOVABLE | UCBF_READY | UCBF_BECOMING_READY)) == (UCBF_REMOVABLE | UCBF_BECOMING_READY);
  ADD_StartTimerMS (&npA->IRQTimerHandle,
		    (ULONG)((longWait || npU->LongTimeout) ? npA->IRQLongTimeOut
							   : npA->IRQTimeOut),
		    (PFN)IRQTimer, npA);

  SendCmdPacket (npA);

StartOtherIOExit:
  ;
}


/*---------------------------------------------*/
/* StartBlockIO 			       */
/* ------------ 			       */
/*					       */
/* Starts a Read/Write/Verify type operation   */
/*					       */
/*---------------------------------------------*/

USHORT NEAR StartBlockIO (NPA npA)
{
  NPU npU = npA->npU;

  /*--------------------------------------*/
  /* Set address and Sector Count  in ACB.  */
  /*--------------------------------------*/
  if (!(npA->ReqFlags & ACBR_NONBLOCKIO))
    SetIOAddress (npA);

  /*--------------------------------------*/
  /* Set the Next State fields in the ACB */
  /*--------------------------------------*/
  npA->State = ACBS_INTERRUPT;

  /*-----------------------------------------*/
  /* Start the IRQ timer and output the copy */
  /* of the TASK File Regs in the ACB to the */
  /* controller.			     */
  /*-----------------------------------------*/

  if ((!(npA->ReqFlags & ACBR_WRITE)) || (npA->ReqFlags & ACBR_DMAIO)) {
    UCHAR longWait;
    npA->State |= ACBS_WAIT;
    npA->Flags &= ~ACBF_BMINT_SEEN;

    longWait = (npU->Flags & (UCBF_REMOVABLE | UCBF_READY | UCBF_BECOMING_READY)) == (UCBF_REMOVABLE | UCBF_BECOMING_READY);
    ADD_StartTimerMS (&npA->IRQTimerHandle,
		      (ULONG)((longWait || npU->LongTimeout) ? npA->IRQLongTimeOut
							     : npA->IRQTimeOut),
		      (PFN)IRQTimer, npA);

    if (!(npA->ReqFlags & ACBR_NONBLOCKIO)) {
      npU->IdleCounter = npU->IdleCountInit;
      if (npU->LongTimeout > 0)
	npU->LongTimeout = FALSE;
    }
  }

  if (!SendCmdPacket (npA)) {
    /*-----------------------------------------------*/
    /* For a 'Class 2' operation WRITE, etc. we need */
    /* to 'prime the pump' by writing the first      */
    /* block to the controller. 		     */
    /*-----------------------------------------------*/
    if (!(npA->ReqFlags & ACBR_DMAIO)) {
      if (npA->ReqFlags & ACBR_WRITE)
	DoBlockIO (npA, 0);
    }
  }
}

/*---------------------------------------------*/
/* SetupDMAdefault			       */
/*---------------------------------------------*/

VOID NEAR GenericSetupDMA (NPA npA)
{
  outpdelay (BMSTATUSREG, (UCHAR)(BMISTA_ERROR |  /* clear Error Bit	   */
			      BMISTA_INTERRUPT |  /* clear INTR flag	   */
			      npA->BMStatus));

  OutD (npA->BMIDTP, npA->ppSGL);
}

/*---------------------------------------------*/
/* SetIOAddress 			       */
/* ------------ 			       */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/

VOID NEAR SetIOAddress (NPA npA)
{
  NPU	npU = npA->npU;
  UCHAR CmdIdx;

#define Mult (npU->Flags & UCBF_MULTIPLEMODE)

  /*-------------------------------------*/
  /* Save S/G List pointers at start	 */
  /* of operation.			 */
  /*-------------------------------------*/
  npA->IOSGPtrs.iSGListStart  = npA->IOSGPtrs.iSGList;
  npA->IOSGPtrs.SGOffsetStart = npA->IOSGPtrs.SGOffset;

  /*-------------------------------------*/
  /* Check for controller transfer limit */
  /* possibly different per unit!	 */
  /*-------------------------------------*/
  npA->SecToGo = (npA->SecRemain >= npU->MaxXSectors) ? npU->MaxXSectors : npA->SecRemain;

  /*--------------------------------------------------*/
  /* For READ/WRITE operation check for MULTIPLE MODE */
  /* and calculate appropriate SecPerInt value.       */
  /*--------------------------------------------------*/
  npA->SecPerInt = 1;

  if ((npA->ReqFlags & ACBR_MULTIPLEMODE) && Mult) {
    npA->SecPerInt = (npA->SecToGo > npU->SecPerBlk) ? npU->SecPerBlk
						     : npA->SecToGo;
  }

  /*---------------------------------------------------*/
  /* For READ VERIFY operations there is one interrupt */
  /* when the operation is complete.		       */
  /*---------------------------------------------------*/
  else if (npA->ReqFlags & ACBR_VERIFY) {
    npA->SecPerInt = npA->SecToGo;
  }

  npA->SecReq = npA->SecToGo;

  SECCNT = (UCHAR) npA->SecToGo;
  FEAT	 = 0;

  CmdIdx = ((((NPCH)&(npA->ReqFlags))[3] & ((ACBR_READ | ACBR_WRITE | ACBR_VERIFY) >> 24)) >> 1)
	 | (((NPCH)&(npU->Flags))[0] & UCBF_MULTIPLEMODE);

  if ((npA->RBA & 0xF0000000) &&	   // if LBA48 addressing required
     !(npA->Cap & CHIPCAP_LBA48DMA))	   // but no LBA48 DMA possible
    npA->ReqFlags |= ACBR_BM_DMA_FORCEPIO; // then do PIO

  /*------------------------------------------------*/
  /* Check for DMA Mode for this Unit		    */
  /*						    */
  /*------------------------------------------------*/

  if ((InitActive == BIOSActive) &&
      (npU->Flags & UCBF_BM_DMA) && (npA->ReqFlags & (ACBR_READ | ACBR_WRITE))) {
    /* Check forced PIO mode flag and try to create scatter/gather list */
    if (!(npA->ReqFlags & ACBR_BM_DMA_FORCEPIO) && !(CreateBMSGList (npA))) {
      /* Shut down Bus Master DMA controller if active */

      CmdIdx = (CmdIdx & 1) | 8;
      if (CmdIdx & 1) {
	npA->BM_CommandCode = npA->BM_StrtMask | BMICOM_START;
	++npU->DeviceCounters.TotalBMWriteOperations;
      } else {
	npA->BM_CommandCode = npA->BM_StrtMask | BMICOM_RD | BMICOM_START;
	++npU->DeviceCounters.TotalBMReadOperations;
      }

      BMSTATUS = 0;

      METHOD(npA).SetupDMA (npA);

      /*
       * Bus Master DMA is now ready for the transfer.	It must be started after the
       * command is issued to the drive, to avoid data corruption in case a spurious
       * interrupt occured at the bginning of the xfer and the drive is in an unknown
       * state.
       */

      npA->ReqFlags |= ACBR_DMAIO;
    }
  }

  if (npA->RBA & 0xF0000000) {	// LBA48 addressing required
    *(ULONG *)&SECTOR = npA->RBA & 0x00FFFFFFUL;
    LBA3 = npA->RBA >> 24;
    CmdIdx += sizeof (CmdTable) / 2;
    npA->IOPendingMask |= FM_HIGH;
  } else {
    *(ULONG *)&SECTOR = npA->RBA & 0x0FFFFFFFUL;
  }

  DRVHD  |= npU->DriveHead;
  COMMAND = CmdTable[CmdIdx];
  npA->IOPendingMask |= (FM_PCMD | FM_PFEAT | FM_PSECNUM | FM_PSECCNT |
			 FM_PCYLL | FM_PCYLH | FM_PDRHD);
}

/*---------------------------------------------*/
/* InterruptState			       */
/* --------------			       */
/*					       */
/* Continues an ongoing I/O operation.	       */
/*					       */
/*---------------------------------------------*/

VOID NEAR InterruptState (NPA npA)
{
  UCHAR        Status, Error = 0;
  USHORT       rc = 0;		 // must initialize
  ULONG        Port;
  ULONG        XferPtr;
  register int loop;
  NPU	       npU = npA->npU;

  if ((npA->ReqFlags & ACBR_DMAIO) && (npU->Flags & UCBF_BM_DMA)) {
    /* Wait for BM DMA active to go away for a while. */

    if (BMSTATUSREG && ((BMSTATUS & (BMISTA_INTERRUPT | BMISTA_ACTIVE)) != BMISTA_INTERRUPT)) {
      if ((npU->Flags & UCBF_IGNORE_BM_ACTV)) {
	IODly (Delay500 * 8);
	METHOD(npA).StopDMA (npA);
	BMSTATUS = InB (BMSTATUSREG);
      } else {
	IODly (npA->IRQDelayCount);
	Port = BMSTATUSREG;
	for (loop = 1024 ; --loop != 0;) {
	  BMSTATUS = InB (Port);
	  if (!(BMSTATUS & BMISTA_ACTIVE)) break;
	  _asm { rep nop };
	}
      }
    }

    /*********************************************************/
    /* See page 80 of the 82371FB PCI ISA IDE Accelerator HW */
    /* reference manual for a description of the error	     */
    /* conditions detected below.			     */
    /*********************************************************/

    if (BMSTATUS & BMISTA_ACTIVE) {
      ++npU->DeviceCounters.TotalBMStatus;
      if (!(npU->Features & 4)) {
	npA->DataErrorCnt++;
	rc = 1;
      }
    } else if (!((BMSTATUS & BMISTA_INTERRUPT) || (npA->Flags & ACBF_BMINT_SEEN))) {
      ++npU->DeviceCounters.TotalBMStatus2;
      if (!(npU->Features & 8))
	rc = 1;
    }

    METHOD(npA).StopDMA (npA);
    /* Fix so that Bus Master errors will definitely result in retries */
    if (BMSTATUS & BMISTA_ERROR) {
      USHORT PCIStatus;

      PciGetReg (npA->PCIInfo.PCIAddr, PCIREG_STATUS, (PULONG)&PCIStatus, 2);
      if (PCIStatus & PCI_STATUS_MASTER_ABORT) {
	PciSetReg (npA->PCIInfo.PCIAddr, PCIREG_STATUS, PCI_STATUS_MASTER_ABORT, 2);
      }
      ++npU->DeviceCounters.TotalBMErrors;
      rc = 1;
    }
  }

  /*----------------------------------------*/
  /* Check the STATUS Reg for the ERROR bit */
  /*----------------------------------------*/
  if (npA->FlagsT & ATBF_LATE_INT) STATUS = InB (STATUSREG);
  Status = STATUS;
  if (Status & FX_BUSY) {
    CheckBusy (npA);
    Status = STATUS;
  }
  if (Status & FX_ERROR) {
    Error = ERROR = InB (ERRORREG);
    rc = 1;
    if (npA->ReqMask & ACBR_OTHERIO) {
      rc = !(Error & FX_ABORT);
    }
    if (npU->Flags & (UCBF_REMOVABLE | UCBF_ATAPIDEVICE)) {
      IODelay();
      if (!Error || (Error == FX_MC)) {
	rc = 0;
	Status &= ~FX_ERROR;
      }
    }
  }

  if (rc) {
    npA->State = ACBS_ERROR;
  } else {
    USHORT Cmd = REQ (npA->pIORB->CommandCode, npA->pIORB->CommandModifier);

    if (!InitActive && (npU->Flags & UCBF_REMOVABLE)) {
      /*
      ** The current command succeeded and the previous command
      ** failed so if the current command actually accessed the
      ** media then assume the media is back and inform the caller
      ** of this event by turning on the media changed bit.  Do
      ** not notify the caller until the current operation is an
      ** operation which actually accesses the media.
      */
      DISABLE
      if (!(npU->Flags & UCBF_READY) || (npU->Flags & UCBF_BECOMING_READY)) {
	UCHAR MediaChanged = FALSE;

	switch (CmdCode) {
	  case IOCC_EXECUTE_IO:
	    switch (CmdModifier) {
	      case IOCM_READ:
	      case IOCM_READ_VERIFY:
	      case IOCM_WRITE:
	      case IOCM_WRITE_VERIFY:
		if (!(npU->Flags & UCBF_READY)) {
		  npU->Flags |= UCBF_READY;
		  MediaChanged = TRUE;
		}
		npU->Flags &= ~UCBF_BECOMING_READY;
	    }
	    break;
	  case IOCC_UNIT_STATUS:
	    switch (CmdModifier) {
	      case IOCM_GET_UNIT_STATUS:
		npU->Flags |= UCBF_READY;
		MediaChanged = TRUE;
	    }
	    break;
	  default:
	    break;
	}
	if (MediaChanged) {
	  UCHAR MErr = GetMediaError (npU);
	  if ((MErr | Error) & FX_MC) {
	    SendAckMediaChange (npA);
	  } else if (MErr & FX_NM) {
	    Status	 |= FX_ERROR;
	    npU->Flags &= ~UCBF_READY;
	    npU->Flags |= UCBF_BECOMING_READY;
	  }
	}
      }
      ENABLE
    }

    /*--------------------------------------------*/
    /* Handle Other Operations			  */
    /*	    Recal, SetParam, Identify		  */
    /*--------------------------------------------*/
    if (npA->ReqFlags & ACBR_OTHERIO) {
      DoOtherIO (npA);

      if (Cmd == REQ (IOCC_UNIT_STATUS, IOCM_GET_UNIT_STATUS))
	GetUnitStatus (npA, Status);
    }

    /*----------------------*/
    /* Handle Block I/O Ops */
    /*----------------------*/
    else if (npA->ReqFlags & ACBR_BLOCKIO) {

      /*    Modified to add Bus Master DMA shutdown when command completes */
      /*    Interrupt routine now supports two separate shutdown procedures */
      /*    for slave and Bus Master DMA */

      if (npA->ReqFlags & ACBR_DMAIO) {
	npA->SecToGo = 0;
	npA->ReqFlags &= ~ACBR_DMAIO; /* reset DMA XFER in progress */
	rc = 0;
      } else
	rc = DoBlockIO (npA, npA->SecPerInt);

      if (!rc && !npA->SecToGo) {
	npA->SecRemain -= npA->SecReq;
	npA->RBA       += npA->SecReq;

	/*----------------------------------------------------*/
	/* If the current operation is complete but there     */
	/* are additional sectors remaining. Start the	      */
	/* next part of the operation.			      */
	/*----------------------------------------------------*/

	if (npA->SecRemain) {
	  StartBlockIO (npA);
	} else {
	  /*--------------------------------------------*/
	  /* For WRITE VERIFY operations, start the	*/
	  /* verify part of the operation.		*/
	  /*--------------------------------------------*/

	  if (npA->ReqFlags & ACBR_WRITEV) {
	    InitBlockIO (npA);
	    npA->ReqFlags &= ~(ACBR_WRITE | ACBR_WRITEV);
	    npA->ReqFlags |=	ACBR_VERIFY;
	    npA->State	   =	ACBS_RETRY;
	  } else {
	    if (!(npA->ReqFlags & ACBR_NONBLOCKIO))
	      ((PIORB_EXECUTEIO)npA->pIORB)->BlocksXferred = ((PIORB_EXECUTEIO)npA->pIORB)->BlockCount;

	    npA->State = ACBS_DONE;
	  }
	}
      }
    }
  }
}


/*---------------------------------------------*/
/* DoBlockIO				       */
/* ---------				       */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/

USHORT NEAR DoBlockIO (NPA npA, USHORT cSec)
{
  USHORT rc = 0;
  NPU	 npU = npA->npU;

  npA->SecToGo -= cSec;

  /*---------------------------------------------------*/
  /* Servicing of DRQ is not needed for READ Verify or */
  /* the last interrupt of a Write operation.	       */
  /*---------------------------------------------------*/

  if (!((npA->ReqFlags & ACBR_VERIFY) ||
       ((npA->ReqFlags & ACBR_WRITE) && !npA->SecToGo))) {
    if (!WaitDRQ (npA)) {
      /*----------------------------------------------------*/
      /* If there will be additional interrupts after	    */
      /* this one, start the IRQ timer and set the IRQ flag */
      /*----------------------------------------------------*/
      if (npA->SecToGo) {
	UCHAR longWait = (npU->Flags & (UCBF_REMOVABLE | UCBF_READY | UCBF_BECOMING_READY)) == (UCBF_REMOVABLE | UCBF_BECOMING_READY);
	ADD_StartTimerMS (&npA->IRQTimerHandle,
			  (ULONG)((longWait || npU->LongTimeout) ? npA->IRQLongTimeOut
								 : npA->IRQTimeOut),
			  (PFN)IRQTimer, npA);
	npA->State |= ACBS_WAIT;
	npA->Flags &= ~ACBF_BMINT_SEEN;
      } else {
	if (npU->Flags & UCBF_ATAPIDEVICE)
	  npA->Flags |= ACBF_INTERRUPT;
      }

      if (npA->ReqFlags & ACBR_WRITE)
	if (npA->SecToGo < npA->SecPerInt)
	  npA->SecPerInt = npA->SecToGo;

      npA->IOSGPtrs.numTotalBytes = ((ULONG) npA->SecPerInt << SECTORSHIFT);

      if (npU->Flags & UCBF_PIO32) npA->IOSGPtrs.Mode |= 0x80;

#if PCITRACER
  outpw (TRPORT, 0xDC00);
#endif

      npA->IOSGPtrs.iPortAddress = DATAREG;

#if 0 //PCITRACER
  outpw (TRPORT, npA->IOSGPtrs.Mode);
  outpw (TRPORT, npA->IOSGPtrs.cSGList);
  OutD	(TRPORT, (ULONG)(npA->IOSGPtrs.pSGList));
  OutD	(TRPORT, npA->IOSGPtrs.iPortAddress);
  OutD	(TRPORT, npA->IOSGPtrs.numTotalBytes);
  outpw (TRPORT, npA->IOSGPtrs.iSGList);
  OutD	(TRPORT, npA->IOSGPtrs.SGOffset);
  outpw (TRPORT, npA->IOSGPtrs.iSGListStart);
  OutD	(TRPORT, npA->IOSGPtrs.SGOffsetStart);
#endif

      ADD_XferIO (&npA->IOSGPtrs);

#if PCITRACER
  outpw (TRPORT, 0xDC01);
#endif

      if (npA->ReqFlags & ACBR_READ) {
	if (npA->SecToGo < npA->SecPerInt)
	  npA->SecPerInt = npA->SecToGo;

	if ((npU->Flags & UCBF_ATAPIDEVICE) ||
	    (npA->HardwareType == PromiseMIO)) {
	  CheckBusy (npA);
	  InB (INTRSNREG);
	}
      }
    } else {
      /*----------------------------------------------------*/
      /* DRQ Timeout					    */
      /*						    */
      /* WaitDRQ will has set the new state and error info. */
      /* This just returns a flag to the caller to stop     */
      /* processing.					    */
      /*----------------------------------------------------*/
      if (COMMAND == FX_SMARTCMD) {
	Error (npA, IOERR_DEVICE_REQ_NOT_SUPPORTED);
	npA->State = ACBS_DONE;
      } else
	rc = 1;
    }
  }
  return (rc);
}


/*---------------------------------------------*/
/* DoOtherIO				       */
/* ---------				       */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/

VOID NEAR DoOtherIO (NPA npA)
{
  NPU npU = npA->npU;

  (USHORT)(npA->ReqFlags) &= ~npA->ReqMask;
  npU->ReqFlags &= ~npA->ReqMask;

  npA->State = (npA->ReqFlags & ~(ACBR_NONDATA | ACBR_NONBLOCKIO)) ? ACBS_RETRY : ACBS_DONE;

  if ((npA->ReqMask == ACBR_SETMULTIPLE) && (npU->Flags & UCBF_SMSENABLED))
    npU->Flags |= UCBF_MULTIPLEMODE;

  if (npA->ReqMask & (ACBR_SETPIOMODE | ACBR_SETDMAMODE)) {
    if (METHOD(npA).PostInitUnit) METHOD(npA).PostInitUnit (npU);

  } else if (npA->ReqMask & ACBR_READMAX) {
    if ((npU->CmdSupported >> 16) & FX_LBA48SUPPORTED) {
      OutB (DEVCTLREG, (UCHAR)(DEVCTL | FX_HOB));
      LBA5 = InB (LBA5REG);
      LBA4 = InB (LBA4REG);
      LBA3 = InB (LBA3REG);
      OutB (DEVCTLREG, DEVCTL);
    } else {
      LBA3 = InB (DRVHDREG) & 0x0F;
    }
    LBA0   = InB (LBA0REG);
    LBA1   = InB (LBA1REG);
    LBA2   = InB (LBA2REG);
  }
}

/*---------------------------------------------*/
/* DoneState				       */
/* ---------				       */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/

VOID NEAR DoneState (NPA npA)
{
  NPU	npU   = npA->npU;
  PIORB pIORB = npA->pIORB;

  if (npA->ReqFlags & ACBR_PASSTHRU) {
    USHORT	 i, IOMask;
    PBYTE	 preg;
    PPassThruATA picp;

    npA->ReqFlags = npU->ReqFlags = 0;		  /* don't run this code again */
    npA->ReqMask = 0;				  /* don't run this code again */

    picp = (PPassThruATA)((PIORB_ADAPTER_PASSTHRU)pIORB)->pControllerCmd;

    if (HIUSHORT(picp)) {
      IOMask = picp->RegisterMapR;
      preg = (PBYTE) &(picp->TaskFileOut);

      if (IOMask & (RTM_LBA4 | RTM_LBA5)) {
	OutB (DEVCTLREG, (UCHAR)(DEVCTL | FX_HOB));
	if (IOMask & RTM_LBA5) picp->TaskFileOut.Lba5 = InB (LBA5REG);
	if (IOMask & RTM_LBA4) picp->TaskFileOut.Lba4 = InB (LBA4REG);
	if (IOMask & RTM_LBA3) picp->TaskFileOut.Lba3 = InB (LBA3REG);
	OutB (DEVCTLREG, DEVCTL);
	IOMask &= FM_LOW;
      }
      if (IOMask & RTM_LBA3)   picp->TaskFileOut.Lba3 = InB (DRVHDREG) & 0x0F;
      if (IOMask & RTM_LBA2)   picp->TaskFileOut.Lba2_CylHi  = InB (LBA2REG);
      if (IOMask & RTM_LBA1)   picp->TaskFileOut.Lba1_CylLo  = InB (LBA1REG);
      if (IOMask & RTM_LBA0)   picp->TaskFileOut.Lba0_SecNum = InB (LBA0REG);
      if (IOMask & RTM_SECCNT) picp->TaskFileOut.SectorCount = InB (SECCNTREG);
      if (IOMask & RTM_ERROR)  picp->TaskFileOut.Error	     = InB (ERRORREG);
      if (IOMask & RTM_STATUS) picp->TaskFileOut.Status      = InB (STATUSREG);
    }
  }

  if ((pIORB->CommandCode == IOCC_GEOMETRY) &&
      ((pIORB->CommandModifier == IOCM_GET_MEDIA_GEOMETRY)  ||
       (pIORB->CommandModifier == IOCM_GET_DEVICE_GEOMETRY))) {
    PGEOMETRY pGEO = ((PIORB_GEOMETRY)pIORB)->pGeometry;

    if ((npU->Flags & (UCBF_REMOVABLE | UCBF_READY)) == (UCBF_REMOVABLE | UCBF_READY)) {
      npU->Flags &= ~UCBF_CHANGELINE;
      // Get the physical geometry from the identify data.
      IDEGeomExtract (&(npU->PhysGeom), &(npA->IdentifyBuf));
      LogGeomCalculate (&(npU->LogGeom), &(npU->PhysGeom));
      if (npU->LogGeom.TotalCylinders > npU->MaxTotalCylinders)
	npU->MaxTotalCylinders = npU->LogGeom.TotalCylinders;
      if (npU->LogGeom.TotalSectors > npU->MaxTotalSectors)
	npU->MaxTotalSectors = npU->LogGeom.TotalSectors;
    }

    pGEO->TotalSectors	  = npU->LogGeom.TotalSectors;
    pGEO->BytesPerSector  = 512;
    pGEO->Reserved	  = 0;
    pGEO->NumHeads	  = npU->LogGeom.NumHeads;
    pGEO->TotalCylinders  = npU->LogGeom.TotalCylinders;
    pGEO->SectorsPerTrack = npU->LogGeom.SectorsPerTrack;

    if (pIORB->CommandModifier == IOCM_GET_DEVICE_GEOMETRY) {
      pGEO->TotalSectors   = npU->MaxTotalSectors;
      pGEO->TotalCylinders = npU->MaxTotalCylinders;
    }
  }

  if (METHOD(npA).StartStop) METHOD(npA).StartStop (npA, ACBS_DONE);

  IORBDone (npA);
  npA->State = ACBS_START;
}

/*---------------------------------------------*/
/* ErrorState				       */
/* ----------				       */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/
VOID NEAR ErrorState (NPA npA)
{
  NPU	 npU = npA->npU;
  UCHAR  Reset = 0;
  UCHAR  RemovableNotReady = 0;
  USHORT TimerHandle;

  TimerHandle = saveXCHG (&(npA->IRQTimerHandle), 0);
  if (TimerHandle) {
    ADD_CancelTimer (TimerHandle);
    npA->Flags &= ~ACBF_MULTIPLEMODE;
  }

  npA->UseCount = 1;

  npA->IORBStatus |= IORB_RECOV_ERROR;
  npA->IORBError   = MapError (npA);

  if (npA->TimerFlags & ACBT_IRQ) {
    ++npU->DeviceCounters.TotalIRQsLost;
    Reset = 1;
#if PCITRACER
    outpw (TRPORT, 0xDCE0);
    outpw (TRPORT, npA->TimerFlags);
#endif
  } else if (npA->TimerFlags & ACBT_DRQ) {
    ++npU->DeviceCounters.TotalDRQsLost;
    Reset = 1;
#if PCITRACER
    outpw (TRPORT, 0xDCE4);
    outpw (TRPORT, npA->TimerFlags);
#endif
  } else if (npA->TimerFlags & ACBT_RESETFAIL) {
    Reset = 1;
#if PCITRACER
    outpw (TRPORT, 0xDCE5);
    outpw (TRPORT, npA->TimerFlags);
#endif
  } else if (npA->TimerFlags & ACBT_BUSY) {
    ++npU->DeviceCounters.TotalBusyErrors;
    if (!(npA->BusyTimeoutCnt % MAX_BUSY_TIMEOUTS)) {
      Reset = 1;
#if PCITRACER
    outpw (TRPORT, 0xDCE1);
    outpw (TRPORT, npA->TimerFlags);
#endif
    }
  } else if (!(STATUS & FX_DRDY) && !(npU->Flags & UCBF_ATAPIDEVICE)) {
    Reset = 1;
#if PCITRACER
    outpw (TRPORT, 0xDCE2);
#endif
  } else if (!(npA->DataErrorCnt % 10)) {
    Reset = 1;
#if PCITRACER
    outpw (TRPORT, 0xDCE3);
#endif
  }

  RemovableNotReady = (npU->Flags & UCBF_REMOVABLE)
		   && (npA->IORBError == IOERR_UNIT_NOT_READY);

  if ((npA->ReqFlags & ACBR_DMAIO) && (npU->Flags & UCBF_BM_DMA)) {
    if ((npA->DataErrorCnt == 1) && (npA->IORBError == IOERR_DEVICE_ULTRA_CRC)) {
      /**********************************************/
      /* Retry the Ultra DMA operation one (1) time */
      /**********************************************/
#if PCITRACER
  outpw (TRPORT, 0xDCCC);
#endif
      if ((Beeps > 0) && (npA->HardwareType != ALi)) {
	DevHelp_Beep (3000, 10);
#if PCITRACER
  outpw (TRPORT+1, 0xBEEB);
#endif
      }
    } else {
      /* the VIA BM state machine is lying on us here sometimes */
      /* so, since the machine should have stopped here anyway, */
      /* it's save to stop it again                             */

      METHOD(npA).StopDMA (npA);  /* controller is locked, Clear Active bit */
      METHOD(npA).ErrorDMA (npA);

      if (!RemovableNotReady) {
	if (Beeps > 0) {
	  DevHelp_Beep (1000, 10);
  #if PCITRACER
    outpw (TRPORT, 0xDCCE);
    outpw (TRPORT+1, 0xBEEB);
  #endif
	}

	/*******************************************************/
	/* Changed DMA_FORCEPIO flag from an ACB Flags option  */
	/* to an ACB ReqFlags option as this mechanism should  */
	/* remain on for the remainder of the I/O operation but*/
	/* be reset at the beginning of the next.  ReqFlags is */
	/* always reset at the beginning of each request.      */
	/*******************************************************/

	/***************************/
	/* force retry in PIO mode */
	/***************************/

	npA->ReqFlags |= ACBR_SETMULTIPLE | ACBR_BM_DMA_FORCEPIO;
      }
      npA->ReqFlags &= ~ACBR_DMAIO;	    /* clear DMA IO flag */
    }
  }

  npA->TimerFlags = 0;

  if (Reset) {
    if (InitActive == 1) {
      npA->IORBStatus = IORB_ERROR;
      npA->State = ACBS_DONE;
      return;
    } else if (Beeps > 0) {
#if PCITRACER
  outpw (TRPORT+1, 0xBEEB);
#endif
      DevHelp_Beep (300, 10);
    }
  }

  if (Reset && !(npU->Flags & UCBF_DISABLERESET))
    DoReset (npA);

  else if (RemovableNotReady) {
    npA->IORBStatus = IORB_ERROR;
    npA->State = ACBS_DONE;

  } else if ((npU->Flags & UCBF_REMOVABLE) &&
	     (npA->IORBError == IOERR_MEDIA_CHANGED))
    npA->State = ACBS_DONE;

  else
    SetRetryState (npA);
}


/*---------------------------------------------*/
/*					       */
/* SetRetryState			       */
/* -------------			       */
/*					       */
/* This routine determines the next state      */
/* after we handled an error or completed      */
/* a reset.				       */
/*					       */
/*---------------------------------------------*/

VOID NEAR SetRetryState(NPA npA)
{
  ULONG ElapsedTime;
  NPU npU = npA->npU;

  if (!npA->ReqFlags) {
    npA->State = ACBS_DONE;
    return;
  }

  if (npU->Flags & UCBF_MULTIPLEMODE) npA->ReqFlags |= ACBR_SETMULTIPLE;

  /*---------------------------------*/
  /* Complete this request if:	     */
  /*				     */
  /*  - Retries disabled	     */
  /*  - Max elapsed time expired     */
  /*  - The unit failed after reset  */
  /*---------------------------------*/
  ElapsedTime = saveGET32 (&(npA->ElapsedTime));

  if (npA->Flags & ACBF_DISABLERETRY	      ||
      ElapsedTime > npA->TimeOut	      ||
      npA->DataErrorCnt > MAX_DATA_ERRORS     ||
      npU->Flags & UCBF_DIAG_FAILED) {
    npA->IORBStatus &= ~IORB_RECOV_ERROR;
    npA->IORBStatus |=	IORB_ERROR;
    npA->State	     =	ACBS_DONE;
  }

  /*-----------------------------------*/
  /* Otherwise			       */
  /*				       */
  /*  -Reset the S/G buffer pointers   */
  /*   to where they were at the start */
  /*   of the operation 	       */
  /*				       */
  /*  -Schedule a retry 	       */
  /*-----------------------------------*/

  else {
    if (npA->ReqFlags & ACBR_BLOCKIO) {
      npA->IOSGPtrs.iSGList  = npA->IOSGPtrs.iSGListStart;
      npA->IOSGPtrs.SGOffset = npA->IOSGPtrs.SGOffsetStart;
    }

    ADD_StartTimerMS (&npA->RetryTimerHandle, npA->DelayedRetryInterval,
		      (PFN)DelayedRetry, npA);
    npA->State |= ACBS_WAIT;
  }
}

/*---------------------------------------------*/
/* DoReset				       */
/*---------------------------------------------*/

VOID NEAR DoReset (NPA npA)
{
  NPU npU;

  if (npA->ResetTimerHandle) return;

#if PCITRACER
  outpw (TRPORT, 0xDEB5);
#endif

  npA->Flags &= ~ACBF_MULTIPLEMODE;

  /*---------------------------------------------*/
  /* Set the RECAL/SETPARAM request flags in all */
  /* UCBs.					 */
  /*---------------------------------------------*/

  for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->cUnits); npU++) {
    ReInitUnit (npU);
  }

  npU = npA->npU;
  npA->ReqFlags |= npU->ReqFlags & ACBR_SETUP;
  npA->ReqFlags &= ~ACBR_RESETCONTROLLER;
  npU->ReqFlags &= ~ACBR_RESETCONTROLLER;

  npA->cResets++;
  npA->DelayedResetCtr = (DELAYED_RESET_MAX / DELAYED_RESET_INTERVAL);

  /*------------------------------------*/
  /* Wait for the controller to recover */
  /* and check the diagnostic code.	*/
  /*					*/
  /* Disable drives which failed to	*/
  /* recover.				*/
  /*------------------------------------*/
  npA->State = ACBS_PATARESET;

  if (METHOD(npA).PreReset) METHOD(npA).PreReset (npA);

#if PCITRACER
  outpw (TRPORT, 0xDEF6);
  outpw (TRPORT, npA);
  outpw (TRPORT, DEVCTLREG);
#endif
  OutBd (DEVCTLREG, FX_DCRRes | FX_SRST, IODelayCount * RESET_ASSERT_TIME); // assert SRST
  OutBd (DEVCTLREG, DEVCTL = FX_DCRRes,  IODelayCount * 5); // deassert SRST for at least 5us
}

/*---------------------------------------------*/
/*					       */
/* ResetCheck				       */
/* ----------				       */
/*					       */
/* This routine is called to check if a reset  */
/* has completed and determines what to do     */
/* next.				       */
/*					       */
/*---------------------------------------------*/

VOID NEAR ResetCheck (NPA npA)
{
  NPU	npU;
  UCHAR connected = TRUE;
  UCHAR ResetComplete = TRUE;

  /*-----------------------------------*/
  /* See if we should continue	       */
  /* waiting for the reset to complete */
  /*-----------------------------------*/
  if (npA->DelayedResetCtr) {
    npA->DelayedResetCtr--;

    for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->cUnits); npU++) {
      if (npU->Flags & UCBF_NOTPRESENT) continue;

      if ((npA->Cap & CHIPCAP_SATA) && SSTATUS) {
	if ((InD (SSTATUS) & SSTAT_DET) != SSTAT_COM_OK) {
	  connected = FALSE;
	  npU->Flags &= ~UCBF_PHYREADY;
	} else {
	  OutD (SERROR, InD (SERROR));	      // clear SERROR
	  npU->Flags |= UCBF_PHYREADY;
	}
      }
    }

    if (connected) {
      for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->cUnits); npU++) {
	if (npU->Flags & UCBF_NOTPRESENT) continue;

	SelectUnit (npU);
	if ((GetStatusReg (npA) & ~FX_BUSY) == ~FX_BUSY) {
	  // Controller is gone, more retries are not going to help.
	  // So stop retrying and report the error.
	  npA->DelayedResetCtr = 0;
	  npA->TimerFlags |= ACBT_RESETFAIL;
	  npA->State = ACBS_DONE;

	} else if (STATUS & FX_BUSY) {
	  ResetComplete = FALSE;
	} else {
	  ERROR = InBd (ERRORREG, npA->IODelayCount);
	  if ((ERROR & ~FX_DIAG_DRIVE1) != FX_DIAG_PASSED) {
	    npU->Flags	    |= UCBF_DIAG_FAILED;
	    npA->TimerFlags |= ACBT_RESETFAIL;
	  }
	}
      }
    } else {
      ResetComplete = FALSE;
    }

    /*-----------------------------------------*/
    /* If diagnostic results are not available */
    /* schedule another retry		       */
    /*-----------------------------------------*/
    if (!ResetComplete && npA->DelayedResetCtr) {
      ADD_StartTimerMS (&npA->ResetTimerHandle, npA->DelayedResetInterval,
			(PFN)DelayedReset, npA);
      npA->State |= ACBS_WAIT;
    }
  } else {
    npA->TimerFlags |= ACBT_RESETFAIL;
  }

  /*---------------------------------------------------------*/
  /* If the RESET is complete but failed:		     */
  /*   Do the reset again until the reset count is exhausted */
  /*   Otherwise retry or fail the original request	     */
  /*---------------------------------------------------------*/
  if (ResetComplete) {
    npA->ReqFlags &= ~ACBR_RESETCONTROLLER;

    if ((npA->TimerFlags & ACBT_RESETFAIL) &&
	(npA->cResets < MAX_RESET_RETRY) && !InitActive) {
      npA->State = ACBS_ERROR;

    } else {
      // reinitialize some hardware

      if (METHOD(npA).PostReset) METHOD(npA).PostReset (npA);
      SetRetryState (npA);
    }
  }
}

#if 0
/*----------------------------------------------------*/
/*						      */
/* GetDiagResults				      */
/* --------------				      */
/*						      */
/* This routine is called after a controller	      */
/* reset is started to collect diagnostic	      */
/* results from the controller			      */
/*						      */
/* Return 0 if reset and diagnostics are complete.    */
/* Return 1 if reset and diagnostics are incomplete.  */
/*						      */
/* Always return the contents of the status register. */
/*						      */
/*----------------------------------------------------*/

USHORT NEAR GetDiagResults (NPA npA, PUCHAR Status)
{
  USHORT rc = 0;
  USHORT DiagCode;
  NPU	 npU;

  /*------------------------------------*/
  /* If the controller is ready to talk */
  /* to us!				*/
  /*------------------------------------*/
  *Status = GetStatusReg (npA);

  // Some controllers with no devices attached report all 1's in
  // in status register.  If this is the case, the device is not
  // going to come ready or report any meaningful status
  if (!(*Status & FX_BUSY)) {
    npU = &npA->UnitCB[0];
    DiagCode = GetErrorReg (npU);

    /*------------------------------------------------*/
    /* Error code indicates one or both drives failed */
    /*------------------------------------------------*/
    if (DiagCode != FX_DIAG_PASSED) {
      /*-------------------------------------*/
      /* Error code indicates Drive 0 failed */
      /*-------------------------------------*/
      if ((DiagCode & 0x0f) != FX_DIAG_PASSED) {
	npU->Flags |= UCBF_DIAG_FAILED;
	npA->TimerFlags |= ACBT_RESETFAIL;
      }

      /*-------------------------------------*/
      /* Error code indicates Drive 1 failed */
      /*-------------------------------------*/
      if (DiagCode & FX_DIAG_DRIVE1) {
	npU = &npA->UnitCB[1];
	DiagCode = GetErrorReg (npU);

	if (DiagCode != FX_DIAG_PASSED) {
	  npU->Flags |= UCBF_DIAG_FAILED;
	  /*---------------------------------------*/
	  /* Connor drives used in IBM L40SX mark  */
	  /* non-installed Unit 1 as defective in  */
	  /* diagnostic results.		   */
	  /*---------------------------------------*/
	  if (npA->cUnits > 1) npA->TimerFlags |= ACBT_RESETFAIL;
	}
      }
    }
  } else {
    rc = 1;
  }

  return (rc);
}
#endif

/*---------------------------------------------*/
/* SendCmdPacket			       */
/*---------------------------------------------*/

USHORT NEAR SendCmdPacket (NPA npA)
{
  NPU npU = npA->npU;

#if PCITRACER
  outpw (TRPORT, 0xDEB6);
#endif
  if (npA->IOPendingMask & FM_PDRHD) {
    if (npU->Flags & UCBF_PCMCIA) DRVHD &= ~0x10;
    outpdelay (DRVHDREG, DRVHD);

    if (npU->Flags & UCBF_ATAPIDEVICE) {
      /* ATAPI Devices are not ready until they recieve an ATAPI */
      /* command						 */
      if (CheckBusy (npA)) goto Send_Error;
    } else {
      if (CheckReady (npA)) goto Send_Error;
    }
  }

  /*
  ** About to write the command register which will sooner
  ** or later cause an interrupt.  If we are currently running
  ** on the interrupt stack, disable interrupts and return to
  ** to kernel.  The kernel will enable interrupts at the RETI.
  ** This prevents an interrupt stack overflow caused by the
  ** HW re-interrupting before the driver and kernel have done
  ** the RETI.
  */
  if (npA->atInterrupt > 0) DISABLE;

  METHOD(npA).SetupTF (npA);
  METHOD(npA).StartDMA (npA);
  return (0);

Send_Error:
  npA->State = ACBS_ERROR;
  return (1);
}

/*---------------------------------------------*/
/* WaitDRQ				       */
/* -------				       */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/

USHORT NEAR WaitDRQ (NPA npA)
{
  UCHAR   Status;
  SHORT   Stop;
  USHORT  Count = 0;

  npA->TimerFlags &= ~(ACBT_DRQ | ACBT_BUSY);
  Stop = *msTimer + MAX_WAIT_READY;

  while ((--Count != 0) && ((*msTimer - Stop) < 0)) {
    Status = GetStatusReg (npA);
    if (!(Status & FX_BUSY))
      if (Status & (FX_ERROR | FX_DF | FX_DRQ)) {
	if (Status & (FX_ERROR | FX_DF)) {
	  npA->State = ACBS_ERROR;
	  return (1);
	} else {
	  return (0);
	}
      }
    IODly (2 * IODelayCount);
  }

  npA->TimerFlags |= ((Status & FX_BUSY) ? ACBT_BUSY : ACBT_DRQ);
  npA->State = ACBS_ERROR;
  return (1);
}

/*---------------------------------------------*/
/* CheckReady				       */
/* ----------				       */
/*					       */
/*					       */
/*					       */
/*---------------------------------------------*/

USHORT NEAR CheckWorker (NPA npA, UCHAR Mask, UCHAR Value)
{
  UCHAR Status;
  SHORT Stop;
  USHORT Count = 0;

  npA->TimerFlags &= ~ACBT_BUSY;
  Stop = *msTimer + MAX_WAIT_READY;

  while ((--Count != 0) && ((*msTimer - Stop) < 0)) {
    Status = GetStatusReg (npA);
    if ((Status & Mask) == Value) break;
    IODelay();
  }

  if (Status & FX_BUSY) npA->TimerFlags |= ACBT_BUSY;

  return ((Status & Mask) != Value);
}

USHORT NEAR CheckReady (NPA npA)
{
  return CheckWorker (npA, FX_BUSY | FX_DRDY, FX_DRDY);
}

USHORT NEAR CheckBusy (NPA npA)
{
  return CheckWorker (npA, FX_BUSY, 0);
}


/*------------------------------------*/
/*				      */
/*				      */
/*				      */
/*------------------------------------*/

VOID NEAR SelectUnit (NPU npU)
{
  NPA npA = npU->npA;

  OutBd (DRVHDREG, DRVHD = npU->DriveHead, npA->IODelayCount);
}


USHORT NEAR GetStatusReg (NPA npA)
{
  USHORT rc = STATUS = InBd (STATUSREG, npA->IODelayCount);
  return (rc);
}


/*--------------------------------------------------------------*/
/*  SetMediaGeometry()						*/
/*								*/
/*								*/
/*--------------------------------------------------------------*/

VOID NEAR SetMediaGeometry (NPA npA)
{ }


/*--------------------------------------------------------------*/
/*  GetChangeLine()						*/
/*								*/
/*								*/
/*--------------------------------------------------------------*/

VOID NEAR GetChangeLine (NPA npA)
{
  PIORB_UNIT_STATUS pIORB;

  pIORB = (PIORB_UNIT_STATUS) npA->pIORB;
  pIORB->UnitStatus = npA->npU->Flags & UCBF_CHANGELINE ? US_CHANGELINE_ACTIVE : 0;
}


/*--------------------------------------------------------------*/
/*  GetLockStatus()						*/
/*								*/
/*								*/
/*--------------------------------------------------------------*/

VOID NEAR GetLockStatus (NPA npA)
{
  PIORB_UNIT_STATUS pIORB;

  pIORB = (PIORB_UNIT_STATUS) npA->pIORB;
  pIORB->UnitStatus = 0;
}


/*--------------------------------------------------------------*/
/*  GetUnitStatus()						*/
/*								*/
/*								*/
/*--------------------------------------------------------------*/

VOID NEAR GetUnitStatus (NPA npA, USHORT Status)
{
  PIORB_UNIT_STATUS pIORB;
  NPU npU;

  pIORB = (PIORB_UNIT_STATUS) npA->pIORB;
  npU = npA->npU;

#if 0
  if (!LastAccessedUnit) LastAccessedUnit = npA->npU;
#endif

  /*
  ** Set the ready flag for removable media.
  */
  pIORB->UnitStatus = US_POWER;
  if (Status & FX_ERROR) {
    npU->Flags &= ~UCBF_READY;
    npU->Flags |= UCBF_BECOMING_READY;
  } else {
    npU->Flags |= UCBF_READY;
    pIORB->UnitStatus |= US_READY;
  }

  // If this is being faked, it is not ready and not powered
  if (npU->Flags & UCBF_NOTPRESENT)
    pIORB->UnitStatus = 0;
  else if (Status & FX_ERROR) {
    /*
    ** Interpret the device's current status register, if the
    ** preceding operation failed then interpret that to mean
    ** the media is not present.
    */
    pIORB->UnitStatus = US_MEDIA_UNKNOWN;
    npA->IORBStatus |= IORB_ERROR;
    npA->IORBError = MapError (npA);
    if ((npU->Flags & UCBF_REMOVABLE) && (npA->IORBError == IOERR_UNIT_NOT_READY)) {
      npA->IORBStatus = IORB_ERROR;
    } else {
      pIORB->UnitStatus |= US_DEFECTIVE;
    }
  }
  npA->State = ACBS_DONE;
}


/*----------------------------------------------------------*/
/*  SendAckMediaChange()				    */
/*							    */
/*  Send the drive a Media Change Acknowledge command to    */
/*  clear the error bit (ERR) in the Status Register and    */
/*  clear media changed (MC) bit in the Error Register.     */
/*							    */
/*  This is done as a synchronious operation (with	    */
/*  interrupts disabled) because the command is vendor	    */
/*  specific and is also, hopefully, a relatively  rare     */
/*  command.						    */
/*							    */
/*----------------------------------------------------------*/
VOID NEAR SendAckMediaChange (NPA npA)
{
  UCHAR Data;

  CheckReady (npA);

  /* Turn off interrupts, temporarily */
  OutBdms (DEVCTLREG, DEVCTL = FX_nIEN);

  /* Write the Acknowledge Media Change command */
  OutBd (COMMANDREG, COMMAND = FX_ACK_MEDIA_CHANGE, 8 * IODelayCount);

  CheckReady (npA);
  Data = InB (ERRORREG);

  /* Turn interrupts back on */
  OutBdms (DEVCTLREG, DEVCTL = FX_DCRRes);
}


/*----------------------------------------------------------*/
/*  GetMediaError()					    */
/*							    */
/*  Read the Removable Media Status error.  Called only for */
/*  removable media devices.				    */
/*							    */
/*----------------------------------------------------------*/
USHORT NEAR GetMediaError (NPU npU)
{
  UCHAR Data = 0;
  NPA npA = npU->npA;

  CheckReady (npA);

  /* Turn off interrupts, temporarily */
  OutBdms (DEVCTLREG, DEVCTL = FX_nIEN);

  /* Write the Get Media Status command */
  OutBd (COMMANDREG, COMMAND = FX_GET_MEDIA_STATUS, 8 * IODelayCount);

  /* Wait for INTRQ */
  CheckReady (npA);
  Data = InB (ERRORREG);
  if (Data & FX_ABORT) Data = 0;

  ERROR = Data;

  /* Turn interrupts back on */
  OutBdms (DEVCTLREG, DEVCTL = FX_DCRRes);

  return (Data);
}

/*------------------------------------*/
/*				      */
/*				      */
/*				      */
/*------------------------------------*/

USHORT NEAR GetErrorReg (NPU npU)
{
  NPA npA = npU->npA;

  OutBd (DRVHDREG, npU->DriveHead, npA->IODelayCount);
  ERROR = InBd (ERRORREG, npA->IODelayCount);
  OutBd (DRVHDREG, DRVHD, npA->IODelayCount);
  return (ERROR);
}

/*------------------------------------*/
/*				      */
/* MapError			      */
/*				      */
/*------------------------------------*/

USHORT NEAR MapError (NPA npA)
{
  USHORT IORBError = IOERR_DEVICE_NONSPECIFIC;
  UCHAR  ErrCode;
  NPU	 npU = npA->npU;

  if (npA->TimerFlags & (ACBT_DRQ | ACBT_READY)) {
    IORBError = IOERR_ADAPTER_TIMEOUT;

  } else if (npA->TimerFlags & ACBT_BUSY) {
    npA->BusyTimeoutCnt++;
    IORBError = IOERR_ADAPTER_TIMEOUT;

  } else if (npA->TimerFlags & ACBT_IRQ) {
    IORBError = IOERR_ADAPTER_DEVICE_TIMEOUT;

  } else if (npA->TimerFlags & ACBT_RESETFAIL) {
    IORBError = IOERR_ADAPTER_DIAGFAIL;

  } else if (!(npU->Flags & UCBF_ATAPIDEVICE) && !(STATUS & FX_DRDY)) {
    IORBError = IOERR_UNIT_NOT_READY;

  } else if (STATUS & FX_ERROR) {
    ErrCode = GetErrorReg (npU);

    npA->DataErrorCnt++;

    if (ErrCode & FX_AMNF) {
      ++npU->DeviceCounters.TotalReadErrors;
      ++npU->DeviceCounters.ReadErrors[0];
      IORBError = IOERR_RBA_ADDRESSING_ERROR;

    } else if (ErrCode & FX_IDNF) {
      ++npU->DeviceCounters.TotalSeekErrors;
      ++npU->DeviceCounters.SeekErrors[0];
      IORBError = IOERR_RBA_ADDRESSING_ERROR;

    } else if (ErrCode & FX_ECCERROR) {
      IORBError = IOERR_RBA_CRC_ERROR;

      npA->DataErrorCnt = -1;		     /* Terminate retries */
      ++npU->DeviceCounters.TotalReadErrors;
      ++npU->DeviceCounters.ReadErrors[1];

      // if device is removable media
      if (npU->Flags & UCBF_REMOVABLE) {
	if (ErrCode = GetMediaError (npU)) {
	  // Media Status reported some error, so disable further retries.

	  npA->Flags |= ACBF_DISABLERETRY;

	  if (ErrCode & FX_WRT_PRT) {
	    IORBError = IOERR_MEDIA_WRITE_PROTECT;
	  } else if (ErrCode & FX_MC) {
	     /* Handled below */
	  } else if (ErrCode & FX_NM) {
	    IORBError = IOERR_MEDIA_NOT_PRESENT;
	    npU->Flags &= ~UCBF_READY;
	  } else {
	    IORBError = IOERR_DEVICE_NONSPECIFIC;
	  }
	}
      }

    } else if (ErrCode & FX_ICRC) {
      if (ErrCode & FX_ABORT) {
	/*************************************/
	/*	  Ultra DMA CRC error	     */
	/*************************************/

	IORBError = IOERR_DEVICE_ULTRA_CRC;

	/*************************************/
	/*	  Error Counters	     */
	/*************************************/

	if (npU->ReqFlags & ACBR_WRITE) {
	  ++npU->DeviceCounters.TotalWriteErrors;
	  ++npU->DeviceCounters.WriteErrors[0];
	} else {
	  ++npU->DeviceCounters.TotalReadErrors;
	  ++npU->DeviceCounters.ReadErrors[2];
	}
      } else {
	/***************************/
	/*  (ErrCode & FX_BADBLK) */
	/***************************/

	npA->DataErrorCnt = -1; 	       /* Terminate retries */
	IORBError = IOERR_RBA_CRC_ERROR;

	if (npU->ReqFlags & ACBR_WRITE) {
	  ++npU->DeviceCounters.TotalWriteErrors;
	  ++npU->DeviceCounters.WriteErrors[1];
	} else {
	  ++npU->DeviceCounters.TotalReadErrors;
	  ++npU->DeviceCounters.ReadErrors[3];
	}
      }

    } else if (ErrCode & FX_TRK0) {
      if (npU->Flags & UCBF_REMOVABLE) {
	/* Media is not present, map this to a not ready error. */

	IORBError = IOERR_UNIT_NOT_READY;
	npU->Flags &= ~UCBF_READY;
      } else {
	IORBError = IOERR_DEVICE_NONSPECIFIC;
	++npU->DeviceCounters.TotalSeekErrors;
	++npU->DeviceCounters.SeekErrors[1];
      }

    } else if (ErrCode & FX_ABORT) {
      IORBError = IOERR_DEVICE_REQ_NOT_SUPPORTED;

      if (npU->ReqFlags & ACBR_WRITE) {
	++npU->DeviceCounters.TotalWriteErrors;
      } else {
	if ((npU->Flags & (UCBF_REMOVABLE | UCBF_READY)) != UCBF_REMOVABLE)
	  ++npU->DeviceCounters.TotalReadErrors;
      }

      // if device is removable media
      if (npU->Flags & UCBF_REMOVABLE) {
	if (ErrCode = GetMediaError (npU)) {
	  // Media Status reported some error, so disable further retries.

	  npA->Flags |= ACBF_DISABLERETRY;
	  if (ErrCode & FX_WRT_PRT) {
	    IORBError = IOERR_MEDIA_WRITE_PROTECT;
	  } else if (ErrCode & FX_MC) {
	    /* Handled below */
	  } else if (ErrCode & FX_NM) {
	    IORBError = IOERR_MEDIA_NOT_PRESENT;
	    npU->Flags &= ~UCBF_READY;
	  } else {
	    IORBError = IOERR_DEVICE_NONSPECIFIC;
	  }
	}
      }
    }

    /*
    ** The spec is not clear on whether the Media Changed bit
    ** always goes active by itself or not.  Always acknowledge
    ** the media change and just in case the bit can be set with
    ** other error bits, check it seperately.
    */
    if (ErrCode & FX_MC) {
      if (npU->Flags & UCBF_REMOVABLE) {
	/* Media Changed */
	SendAckMediaChange (npA);
	IORBError = IOERR_MEDIA_CHANGED;
	npU->Flags |= UCBF_READY;
      }
    }
  }
#if PCITRACER
  OutD (TRPORT, npU->Flags);
#endif
  return (IORBError);
}

/*--------------------------------------------*/
/* CreateBMSGList			      */
/* --------------			      */
/*					      */
/* Arguments:				      */
/*					      */
/*					      */
/* Actions:				      */
/*	Takes OS/2 scatter/gather list and    */
/*	builds SFF-8038i compatible list for  */
/*	DMA controller. 		      */
/*					      */
/*					      */
/* Returns:				      */
/*	0 if successful 		      */
/*					      */
/*					      */
/*--------------------------------------------*/

BOOL NEAR CreateBMSGList (NPA npA)
{
  if (BuildSGList (npA->pSGL, &(npA->IOSGPtrs), npA->SecReq * 512UL, npA->SGAlign)) {
    return (0); 			 /* finished building sg list */
  } else {
    ++npA->npU->DeviceCounters.ByteMisalignedBuffers;
    return (1); 		    /* fail conversion */
  }
}

