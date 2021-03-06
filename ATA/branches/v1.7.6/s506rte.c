/**************************************************************************
 *
 * SOURCE FILE NAME = S506RTE.C
 *
 * DESCRIPTIVE NAME = DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : Strategy 1 Entry Point
 *
 ****************************************************************************/

 #define INCL_NOPMAPI
 #define INCL_NO_SCB
 #define INCL_DOSINFOSEG
 #define INCL_DOSERRORS
 #include "os2.h"
 #include "devcmd.h"
 #include "sas.h"
 #include "dskinit.h"

 #include "iorb.h"
 #include "strat2.h"
 #include "reqpkt.h"
 #include "dhcalls.h"
 #include "addcalls.h"

 #include "s506cons.h"
 #include "s506type.h"
 #include "s506ext.h"
 #include "s506pro.h"
 #include "s506regs.h"

#include "Trace.h"

#pragma optimize(OPTIMIZE, on)

#define PCITRACER 0
#define TRPORT 0xD020

void NEAR DoIOCTLs (PRP_GENIOCTL);

#define StatusError(pRPH,ErrorCode) (pRPH)->Status = (ErrorCode)

/*------------------------------------------------------------------------*/
/* OS/2 Strategy Request Router 					  */
/* ---------------------------- 					  */
/* This routine receives the OS/2 Initialization Request Packet. Any	  */
/* other request types are rejected.					  */
/*------------------------------------------------------------------------*/

VOID FAR _cdecl S506Str1 (VOID)
{
  PRPH pRPH;	      /* Pointer to RPH (Request Packet Header)      */

  _asm {
    mov word ptr pRPH[0], bx	   /*  pRPH is initialize to		      */
    mov word ptr pRPH[2], es	   /*  ES:BX passed from the kernel	      */
  }

  StatusError (pRPH, STATUS_DONE);  // default to done

  switch (pRPH->Cmd) {
    case CMDInitBase :
      LoadFlatGS();
      DriveInit ((PRPINITIN) pRPH);
      break;

    case CMDGenIOCTL : {
      USHORT AwakeCount;
      USHORT BlockRet;

      DISABLE
      if (ReqCount >= 1) {
	PUCHAR DataPacketSave;

	++ReqCount;
	DataPacketSave = ((PRP_GENIOCTL)pRPH)->DataPacket;  /* save data packet addr */

	/* setup up queue and wait for wakeup */

	((PRP_GENIOCTL)pRPH)->DataPacket = (PUCHAR)(struct _ILockEntry FAR*)Queue;
	Queue = (ULONG) pRPH;
	pRPH->Status |= STBUI;
	while (pRPH->Status & STBUI) {
	  BlockRet = DevHelp_ProcBlock ((ULONG) pRPH, 5000, WAIT_IS_INTERRUPTABLE);
	  DISABLE
	}

	/* woke up, clean up and run command now */

	((PRP_GENIOCTL)pRPH)->DataPacket = DataPacketSave;  /* restore data packet addr */

	if (BlockRet == WAIT_TIMED_OUT) {
	  StatusError (pRPH, STATUS_DONE | STERR);
	  goto ExitIOCtl;
	}
      } else {
	++ReqCount;			/* now processing a command */
	Queue = 0L;			/* force queue empty */
      }

      ENABLE
      DoIOCTLs ((PRP_GENIOCTL)pRPH);  /* process IOCTL subfunctions */

      DISABLE
      --ReqCount;			/* decrement request count */
      if (ReqCount) {
	/* pick request packet for process to wake up and remove from chain */

	pRPH = (PRPH)Queue;
	Queue = (ULONG)(((PRP_GENIOCTL)pRPH)->DataPacket);
	pRPH->Status &= ~STBUI;
	DevHelp_ProcRun ((ULONG)pRPH, &AwakeCount); /* run blocked process */
      }
    ExitIOCtl:

      ENABLE
      break;
    }

    case CMDInit :
      #define pRPO ((PRPINITOUT)pRPH)
      pRPO->CodeEnd = 0;
      pRPO->DataEnd = 0;
      StatusError (pRPH, STATUS_DONE | STATUS_ERR_UNKCMD);
      break;

    case CMDInitComplete : {
      NPA   npA;
      NPU   npU;
      CHAR  Adapter, Unit;
      UCHAR noAPM;
      PVOID p;

      InitComplete = 1;
      // attach to APM
      if (!(noAPM = APMAttach())) {
	// if attached, register for suspend and resume
	APMRegister ((PAPMHANDLER)APMEventHandler,
		     APM_NOTIFYSETPWR | APM_NOTIFYNORMRESUME |
		     APM_NOTIFYCRITRESUME | APM_NOTIFYSTBYRESUME,
		     0);
      }

      for (Adapter = 0; Adapter < cAdapters; Adapter++) {
	npA = ACBPtrs[Adapter];

	for (npU = npA->UnitCB, Unit = npA->cUnits; --Unit >= 0; npU++) {
	  if (noAPM) {
	    if (npU->IdleCountInit != 0)
	      npU->LongTimeout = -1;
	    else {
	      npU->IdleTime = ~0;
	      npU->ReqFlags |= UCBR_SETIDLETIM;
	    }
	  }
	  if (!(npU->Flags & (UCBF_NOTPRESENT | UCBF_ATAPIDEVICE | UCBF_CFA)))
	    NoOp (npU);
	}
      }

#if AUTOMOUNT
      // search OS2LVM.DMD

      p = MAKEP (SelSAS, 0);
      OFFSETOF (p) = ((struct SAS _far *)p)->SAS_dd_data;
      OFFSETOF (p) = ((struct SAS_dd_section _far *)p)->SAS_dd_bimodal_chain;
      while (OFFSETOF (p) != (USHORT)-1) {
	if (*(PULONG)(((struct SysDev _far *)p)->SDevName + 1) == 0x204D564Cul) {
	  OS2LVM = (PStrat1Entry)MAKEP (((struct SysDev _far *)p)->SDevProtCS,
					((struct SysDev _far *)p)->SDevStrat);
	  break;
	}
	p = (PVOID)(((struct SysDev _far *)p)->SDevNext);
      }
#endif
      break;
    }

    case CMDShutdown : {
      NPA  npA;
      NPU  npU;
      CHAR Adapter, Unit;

      if (((PRPSAVERESTORE)pRPH)->FuncCode) {
	Beeps = 0;

	for (Adapter = 0; Adapter < cAdapters; Adapter++) {
	  npA = ACBPtrs[Adapter];

	  for (npU = npA->UnitCB, Unit = npA->cUnits; --Unit >= 0; npU++) {
	    if (!(npU->Flags & (UCBF_NOTPRESENT | UCBF_ATAPIDEVICE | UCBF_CFA))) {
	      npU->LongTimeout = -1;
	      IssueOneByte (npU, (UCHAR)(((npU->CmdSupported >> 16) & FX_FLUSHXSUPPORTED) ?
			    FX_FLUSHEXT : FX_FLUSH));
	      if (npU->CmdEnabled & UCBS_HPROT) {
		IssueSetMax (npU, (PULONG)&(npU->FoundLBASize), TRUE);
	      }
	      if (NotShutdown)
//		  IssueOneByte (npU, FX_IDLEIMM);
		IssueSetIdle (npU, 0, 0);    // Marvell can't do IDLE_IMMEDIATE
	      else
		IssueSetIdle (npU, 1, 1);
	    }
	  }
	}
	Delay (500);
      }
      break;
    }
    case CMDOpen :
      if (SELECTOROF (ReadPtr) == NULL)
	ReadPtr = MsgBuffer;
      break;

    case CMDClose :
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
      StatusError (pRPH, STATUS_DONE | STATUS_ERR_UNKCMD);
  }
}


int NEAR IssueCommand (NPU npU)
{
  NPA	 npA = npU->npA;
  USHORT rc;
  NPIORB_ADAPTER_PASSTHRU npIORB = &(npA->PTIORB);

  npIORB->iorbh.Length		= sizeof (IORB_ADAPTER_PASSTHRU);
  npIORB->iorbh.UnitHandle	= (USHORT)npU;
  npIORB->iorbh.CommandCode	= IOCC_ADAPTER_PASSTHRU;
  npIORB->iorbh.CommandModifier = IOCM_EXECUTE_ATA;
  npIORB->iorbh.RequestControl	= IORB_ASYNC_POST | IORB_DISABLE_RETRY;

  npIORB->ControllerCmdLen	= sizeof (npA->icp);
  npIORB->pControllerCmd	= (PBYTE)&npA->icp;
  npIORB->Flags 		= PT_DIRECTION_IN;

  rc = Execute (0, (NPIORB)npIORB);

  if (npA->icp.TaskFileOut.Status & FX_ERROR) {
    npIORB->iorbh.Status |= IORB_ERROR;
    rc = npIORB->iorbh.ErrorCode = IOERR_DEVICE_REQ_NOT_SUPPORTED;
  }
  return (rc);
}

int NEAR IssueCommandTO (NPU npU, USHORT TimeOut) {
  NPA	 npA = npU->npA;
  USHORT rc;
  ULONG  save = npA->IRQTimeOut;

  npA->IRQTimeOut = TimeOut;
  rc = IssueCommand (npU);
  npA->IRQTimeOut = save;
  return (rc);
}

VOID NEAR ClearIORB (NPA npA) {
  fclrmem (&npA->PTIORB, sizeof (npA->PTIORB));
  clrmem (&npA->icp, sizeof (npA->icp));
}

/*---------------------------------------------------------------------------*
 * Process SMART Commands						     *
 * ----------------------						     *
 *									     *
 *									     *
 *---------------------------------------------------------------------------*/

int NEAR DoSMART (NPU npU, BYTE bySubFunction, BYTE byParameter, PVOID pBuffer)
{
  NPA		npA = npU->npA;
  NPPassThruATA npicp;
  USHORT	rc;

  ClearIORB (npA);

  npicp = &npA->icp;
  npicp->TaskFileIn.CylinderLow  = 0x4F;	/* magic key for SMART */
  npicp->TaskFileIn.CylinderHigh = 0xC2;
  npicp->TaskFileIn.SectorCount  = byParameter;
  npicp->TaskFileIn.SectorNumber = byParameter;
  npicp->TaskFileIn.Features	 = bySubFunction;
  npicp->TaskFileIn.Command	 = FX_SMARTCMD;

  npicp->RegisterTransferMap = PTA_RTMWR_FEATURES | PTA_RTMWR_SECTORCOUNT |
			       PTA_RTMWR_CYLINDERLOW | PTA_RTMWR_CYLINDERHIGH |
			       PTA_RTMWR_SECTORNUMBER | PTA_RTMWR_COMMAND |
			       PTA_RTMRD_ERROR | PTA_RTMRD_STATUS;

  if (pBuffer) {			     /* if involving data transfer */
    ScratchSGList.ppXferBuf  = ppDataSeg + (USHORT)ScratchBuf;
    ScratchSGList.XferBufLen = 512;
    npA->PTIORB.pSGList      = &ScratchSGList;
    npA->PTIORB.cSGList      = 1;
    npicp->TaskFileIn.SectorCount = 1;
  }

  rc = IssueCommand (npU);

  if (pBuffer) {
    /* Move data from the local ScratchBuf to caller's buffer */
    memcpy (pBuffer, ScratchBuf, 512);
  }

  return (rc);
}


/*---------------------------------------------------------------------------*
 * Get SMART Status							     *
 * ----------------							     *
 *									     *
 *									     *
 *---------------------------------------------------------------------------*/

int NEAR GetSmartStatus (NPU npU, PDWORD pData)
{
  NPA npA = npU->npA;
  NPPassThruATA npicp;

  ClearIORB (npA);

  npicp = &npA->icp;
  npicp->TaskFileIn.CylinderLow  = 0x4F;	/* magic key for SMART */
  npicp->TaskFileIn.CylinderHigh = 0xC2;
  npicp->TaskFileIn.Features	 = FX_SMART_STATUS;
  npicp->TaskFileIn.Command	 = FX_SMARTCMD;

  npicp->RegisterTransferMap = PTA_RTMWR_FEATURES | PTA_RTMWR_SECTORCOUNT |
			       PTA_RTMWR_CYLINDERLOW | PTA_RTMWR_CYLINDERHIGH |
			       PTA_RTMWR_COMMAND |
			       PTA_RTMRD_CYLINDERLOW | PTA_RTMRD_CYLINDERHIGH |
			       PTA_RTMRD_ERROR | PTA_RTMRD_STATUS;

  IssueCommand (npU);

  npA->PTIORB.iorbh.Status &= ~IORB_ERROR;
  if ((npicp->TaskFileOut.Status & FX_ERROR) &&
      (npicp->TaskFileOut.Error & FX_ABORT)) {
    npA->PTIORB.iorbh.Status |= IORB_ERROR;
    npA->PTIORB.iorbh.ErrorCode = IOERR_DEVICE_REQ_NOT_SUPPORTED;
    *pData = -1;
  } else switch (*(NPUSHORT)&(npicp->TaskFileOut.CylinderLow)) {
    case 0xC24F : *pData = 0; break;
    case 0xF42C : *pData = 1; break;
  }
  return ((npA->PTIORB.iorbh.Status & IORB_ERROR) ? npA->PTIORB.iorbh.ErrorCode : 0);
}

/*---------------------------------------------------------------------------*
 * Read Sector								     *
 *---------------------------------------------------------------------------*/

int NEAR IOCtlRead (NPU npU, PBYTE pBuffer, USHORT Buffersize, ULONG RBA)
{
  NPA npA = npU->npA;
  NPIORB_EXECUTEIO npIO = (NPIORB_EXECUTEIO)&(npA->PTIORB);
  USHORT rc;

  ClearIORB (npA);

#if 1
  ScratchSGList.ppXferBuf  = ppDataSeg + (USHORT)ScratchBuf;
  ScratchSGList.XferBufLen = 512;
  npIO->pSGList 	   = &ScratchSGList;
  npIO->cSGList 	   = 1;
#else
  ScratchSGList.ppXferBuf   = ppDataSeg + (USHORT)ScratchBuf;
  ScratchSGList.XferBufLen  = 1;
  ScratchSGList1.ppXferBuf  = ScratchSGList.ppXferBuf + ScratchSGList.XferBufLen;
  ScratchSGList1.XferBufLen = 512 - ScratchSGList.XferBufLen;
  npIO->pSGList 	    = &ScratchSGList;
  npIO->cSGList 	    = 2;
#endif
  npIO->iorbh.Length	      = sizeof (IORB_EXECUTEIO);
  npIO->iorbh.UnitHandle      = (USHORT)npU;
  npIO->iorbh.CommandCode     = IOCC_EXECUTE_IO;
  npIO->iorbh.CommandModifier = IOCM_READ;
  npIO->iorbh.RequestControl  = IORB_ASYNC_POST | IORB_DISABLE_RETRY | 0x8000;
  npIO->RBA		      = RBA;
  npIO->BlockCount	      = 1;
  npIO->BlockSize	      = 512;
  npIO->BlocksXferred	      = 0;
  npIO->Flags		      = 0;

  rc = Execute (0, (NPIORB)npIO);

  if (rc) {
    fclrmem (pBuffer, Buffersize);
  } else {
    memcpy (pBuffer, ScratchBuf, Buffersize);
  }
  return (rc);
}


/*---------------------------------------------------------------------------*
 * Get Inquiry Data							     *
 * ----------------							     *
 *									     *
 *									     *
 *---------------------------------------------------------------------------*/

int NEAR GetInquiryData (NPU npU, PBYTE pBuffer, USHORT Buffersize)
{
  NPA	 npA = npU->npA;
  USHORT rc;

  ClearIORB (npA);

  npA->icp.TaskFileIn.Command = (npU->Flags & UCBF_ATAPIDEVICE) ?
				 FX_ATAPI_IDENTIFY : FX_IDENTIFY;

  npA->icp.RegisterTransferMap = PTA_RTMWR_COMMAND | PTA_RTMWR_FEATURES |
				 PTA_RTMRD_ERROR | PTA_RTMRD_STATUS;

  ScratchSGList.ppXferBuf  = ppDataSeg + (USHORT)ScratchBuf;
  ScratchSGList.XferBufLen = 512;
  npA->PTIORB.pSGList	   = &ScratchSGList;
  npA->PTIORB.cSGList	   = 1;

  rc = IssueCommand (npU);

  if (rc) {
    fclrmem (pBuffer, Buffersize);
  } else {
    memcpy (pBuffer, ScratchBuf, Buffersize);
  }
  return (rc);
}


int NEAR IssueOneByte (NPU npU, BYTE Cmd)
{
  NPA npA = npU->npA;

  ClearIORB (npA);

  npA->icp.TaskFileIn.Command  = Cmd;
  npA->icp.RegisterTransferMap = PTA_RTMWR_COMMAND;

  return (IssueCommandTO (npU, 200));
}

int NEAR IssueSetMax (NPU npU, PULONG numSectors, SHORT set)
{
  NPPassThruATA npicp;
  NPA	 npA = npU->npA;
  USHORT rc;
  ULONG  numNative;

  ClearIORB (npA);

  npicp = &npA->icp;
  npicp->TaskFileIn.Command   = FX_READMAX;
  npicp->TaskFileIn.DriveHead = npU->UnitId ? 0x50 : 0x40;
  npicp->RegisterTransferMap = PTA_RTMWR_COMMAND | PTA_RTMWR_DRIVEHEAD |
			       PTA_RTMRD_CYLINDERLOW | PTA_RTMRD_CYLINDERHIGH |
			       PTA_RTMRD_SECTORNUMBER | PTA_RTMRD_DRIVEHEAD |
			       PTA_RTMRD_ERROR;
  rc = IssueCommandTO (npU, 200);
  if (rc) return (rc);

  npicp->TaskFileOut.DriveHead &= 0x0F;
  numNative = 1 + *(ULONG *)&(npicp->TaskFileOut.SectorNumber);

  if (set > 0) {
    *(ULONG *)&(npicp->TaskFileIn.SectorNumber) |=
					 ((*numSectors - 1)& 0x0FFFFFFFUL);
  } else {
    *numSectors = numNative;
    return (0);
  }

  fclrmem (&npA->PTIORB, sizeof (npA->PTIORB));
  npicp->TaskFileIn.Command	= FX_SETMAX;
  npicp->TaskFileIn.Features	= 0;
  npicp->TaskFileIn.SectorCount = 0;
  if (set & 2) {
    npicp->TaskFileIn.SectorCount = 1;
    npicp->TaskFileIn.DriveHead  |= 0x40;   // LBA mode
  }

  npicp->RegisterTransferMap = PTA_RTMWR_COMMAND | PTA_RTMWR_FEATURES |
			       PTA_RTMWR_SECTORCOUNT | PTA_RTMWR_DRIVEHEAD |
			       PTA_RTMRD_CYLINDERLOW | PTA_RTMRD_CYLINDERHIGH |
			       PTA_RTMRD_SECTORNUMBER | PTA_RTMRD_DRIVEHEAD |
			       PTA_RTMRD_ERROR;
  if (set)
    npicp->RegisterTransferMap |= PTA_RTMWR_SECTORNUMBER | PTA_RTMWR_CYLINDERLOW |
				  PTA_RTMWR_CYLINDERHIGH;

  return (IssueCommandTO (npU, 200));
}

/*---------------------------------------------------------------------------*
 * Issue Set Features							     *
 * ------------------							     *
 *									     *
 *									     *
 *---------------------------------------------------------------------------*/

int NEAR IssueSetFeatures (NPU npU, BYTE SubCommand, BYTE Arg)
{
  NPA npA = npU->npA;

  ClearIORB (npA);

  npA->icp.TaskFileIn.Command = FX_SETFEAT;
  npA->icp.TaskFileIn.Features = SubCommand;
  npA->icp.TaskFileIn.SectorCount = Arg;

  npA->icp.RegisterTransferMap = PTA_RTMWR_COMMAND | PTA_RTMWR_FEATURES |
				 PTA_RTMWR_SECTORCOUNT |
				 PTA_RTMRD_CYLINDERLOW | PTA_RTMRD_CYLINDERHIGH |
				 PTA_RTMRD_SECTORNUMBER | PTA_RTMRD_ERROR;

  return (IssueCommand (npU));
}

/*---------------------------------------------------------------------------*
 * Issue Get Power Status						     *
 * ----------------------						     *
 *									     *
 *									     *
 *---------------------------------------------------------------------------*/

int NEAR IssueGetPowerStatus (NPU npU)
{
  NPA npA = npU->npA;

  ClearIORB (npA);

  npA->icp.TaskFileIn.Command  = FX_CPWRMODE;
  npA->icp.RegisterTransferMap = PTA_RTMWR_COMMAND |
				 PTA_RTMRD_SECTORCOUNT | PTA_RTMRD_STATUS;

  return (IssueCommand (npU));
}

/*---------------------------------------------------------------------------*
 * Issue Set Idle							     *
 * ---------------							     *
 *									     *
 *									     *
 *---------------------------------------------------------------------------*/
int NEAR IssueSetIdle (NPU npU, UCHAR Cmd, BYTE Arg)
{
  NPA npA = npU->npA;

  ClearIORB (npA);

  npA->icp.TaskFileIn.Command = Cmd ? FX_SETSTANDBY : FX_SETIDLE;
  npA->icp.TaskFileIn.SectorCount = Arg;

  npA->icp.RegisterTransferMap = PTA_RTMWR_COMMAND | PTA_RTMWR_SECTORCOUNT |
				 PTA_RTMRD_ERROR | PTA_RTMRD_STATUS;

  return (IssueCommandTO (npU, 100));
}

/*---------------------------------------------------------------------------*
 * Issue GetMediaStatus 						     *
 * ------------------							     *
 *									     *
 *									     *
 *---------------------------------------------------------------------------*/

int NEAR IssueGetMediaStatus (NPU npU)
{
  NPA	 npA = npU->npA;
  USHORT rc;

  ClearIORB (npA);

  npA->icp.TaskFileIn.Command  = FX_GET_MEDIA_STATUS;
  npA->icp.RegisterTransferMap = PTA_RTMWR_COMMAND |
				 PTA_RTMRD_ERROR | PTA_RTMRD_STATUS;

  rc = IssueCommand (npU);

  if (rc) {
    if (npA->icp.TaskFileOut.Error & FX_ABORT) {
      rc = IOERR_CMD_ABORTED;
    } else {
      // signal the status has changed
      rc = IOERR_DEVICE_NONSPECIFIC;
      npU->MediaStatus = npA->icp.TaskFileOut.Error;
    }
  }
  return (rc);
}

/*---------------------------------------------------------------------------*
 * ProcessLockUnlockEject						     *
 * ------------------							     *
 *									     *
 *									     *
 *---------------------------------------------------------------------------*/

VOID NEAR ProcessLockUnlockEject (NPU npU, PIORB pIORB, UCHAR Function)
{
  NPPassThruATA npicp;
  NPA	 npA = npU->npA;
  USHORT rc;

  ClearIORB (npA);

  npicp = &npA->icp;
  switch (Function) {
    case IOCM_EJECT_MEDIA:
      npicp->TaskFileIn.Command = FX_EJECT_MEDIA;
      if (npU->Flags & (UCBF_PCMCIA | UCBF_CFA)) {
	IssueOneByte (npU, FX_STANDBYIMM);
	DevHelp_Beep (2000, 100);
      }
      break;
    case IOCM_LOCK_MEDIA:
      npicp->TaskFileIn.Command = FX_LOCK_DOOR;
      break;
    case IOCM_UNLOCK_MEDIA:
      npicp->TaskFileIn.Command = FX_UNLOCK_DOOR;
      break;
  }

  if (npU->Flags & UCBF_PCMCIA) return;

  npicp->RegisterTransferMap = PTA_RTMWR_COMMAND | PTA_RTMRD_ERROR | PTA_RTMRD_STATUS;

  rc = IssueCommand (npU);

  if (rc) {
    if (npicp->TaskFileOut.Error & FX_ABORT) {
      pIORB->ErrorCode = IOERR_CMD_ABORTED;
    } else {
      // signal the status has changed
      if (npicp->TaskFileOut.Error & FX_MCR) {
	pIORB->ErrorCode = IOERR_MEDIA_CHANGED;
      } else {
	pIORB->ErrorCode = IOERR_DEVICE_NONSPECIFIC;
      }
      if (npicp->TaskFileOut.Error & FX_NM) {
	npU->Flags &= ~UCBF_READY;
	npU->Flags |=  UCBF_BECOMING_READY;
      } else {
	npU->Flags |=  UCBF_READY;
      }
    }
  }
}

#ifdef ENABLE_SET_TIMINGS

/*---------------------------------------------------------------------------*
 * Set Timing Modes							     *
 * ----------------							     *
 *									     *
 *									     *
 *---------------------------------------------------------------------------*/

static int SetTimingModes (PConfigureUnitParameters pCUP)
{
  NPA	 npA;
  USHORT UnitId;

  // Verify the input physical unit translates to an active
  // and valid ACB.
  if (pCUP->byPhysicalUnit/2 >= cAdapters)
  {
     return IOERR_UNIT_NOT_READY;
  }

  npA = ACBPtrs[pCUP->byPhysicalUnit/2];
  UnitId = pCUP->byPhysicalUnit % 2;

  if  ((UnitId >= npA->cUnits) ||
       (npA->UnitCB[UnitId].Flags & UCBF_NODASDSUPPORT))
  {
    npA->PTIORB.iorbh.Status |= IORB_ERROR;
    npA->PTIORB.iorbh.ErrorCode = IOERR_UNIT_NOT_READY;
    return ((npA->PTIORB.iorbh.Status & IORB_ERROR) ? npA->PTIORB.iorbh.ErrorCode : 0);
  }

  /* handle setting of Bus Master DMA enable switches */

  if  (pCUP->BusMasterDMA.CommandBits & CUPCB_BUSMASTERENABLE)
  {
    if	(pCUP->BusMasterDMA.OnOff)	   /* if nonzero, turn on DMA */
    {
      npA->UnitCB[UnitId].Flags |= UCBF_BM_DMA;
    }
    else				    /* otherwise, turn it off */
    {
      npA->UnitCB[UnitId].Flags &= ~UCBF_BM_DMA;
    }
  }

  /* handle PIO mode selection */

  if  (pCUP->BusMasterDMA.CommandBits & CUPCB_SETPIOMODE)
  {
    npA->UnitCB[UnitId].CurPIOMode = pCUP->BusMasterDMA.PIO_Mode;
  }

  /* handle DMA mode selection */
  if  (pCUP->BusMasterDMA.CommandBits & CUPCB_SETDMAMODE)
  {
    npA->UnitCB[UnitId].CurDMAMode = pCUP->BusMasterDMA.DMA_Mode;
  }

  /* Calculate adapter timing based on new setting for unit. */

  CalculateAdapterTiming (npA, piix_Level,
			  npA->BasePort] == FX_PRIMARY);

  /* Issue required set features commands */

  if  (npA->UnitCB[UnitId].CurPIOMode)
  {
    IssueSetFeatures (npA, UnitId, FX_SETXFERMODE,
		      (BYTE)(FX_PIOMODEX |
		      npA->UnitCB[UnitId].CurPIOMode));
  }
  else
  {
    IssueSetFeatures (npA, UnitId, FX_SETXFERMODE, FX_PIOMODE0);
  }

  if  (npA->UnitCB[UnitId].Flags & UCBF_BM_DMA)
  {
    IssueSetFeatures (npA, UnitId, FX_SETXFERMODE,
		      (BYTE)(FX_MWDMAMODEX |
		      npA->UnitCB[UnitId].CurDMAMode));
  }
}

#endif

/*---------------------------------------------------------------------------*
 * Process Generic IOCTL Calls						     *
 * ---------------------------						     *
 *									     *
 *									     *
 *---------------------------------------------------------------------------*/

NPU NEAR MapUnit (UCHAR PhUnit, PUSHORT error) {
  NPU  npU;
  NPA  npA;
  CHAR Adapter, Unit;

  *error = IOERR_UNIT_NOT_READY;
  if (PhUnit < 0x80) {
    for (Adapter = 0; Adapter < cAdapters; Adapter++) {
      npA = ACBPtrs[Adapter];

      for (npU = npA->UnitCB, Unit = npA->cUnits; --Unit >= 0; npU++) {
	if (npU->Flags & (UCBF_NODASDSUPPORT | UCBF_ATAPIDEVICE)) continue;
	if (PhUnit-- == 0) {
	  *error = 0;
	  return (npU);
	}
      }
    }
  } else {
    UCHAR UnitId = PhUnit % 2;

    PhUnit = (PhUnit & 0x7F) / 2;
    if (PhUnit >= cAdapters) {
      *error = IOERR_ADAPTER_REQ_NOT_SUPPORTED;
      return (NULL);
    }
    npA = ACBPtrs[PhUnit];
    if (UnitId >= npA->cUnits)
      return (NULL);
    *error = 0;
    return (&(npA->UnitCB[UnitId]));
  }
  return (NULL);
}

void NEAR DoIOCTLs (PRP_GENIOCTL pRP_IOCTL)
{
  PDSKSP_CommandParameters pcp;
  USHORT		   status = STATUS_DONE;
  USHORT		   error, rc = IOERR_DEVICE_REQ_NOT_SUPPORTED;
  UCHAR 		   PhUnit;
  NPA			   npA;
  NPU			   npU;

  pcp = (PDSKSP_CommandParameters) pRP_IOCTL->ParmPacket;

  if (DevHelp_VerifyAccess (SELECTOROF (pcp), sizeof (PDSKSP_CommandParameters),
			    OFFSETOF (pcp), VERIFY_READONLY)) {
    StatusError ((PRPH) pRP_IOCTL, STATUS_DONE | STERR | STATUS_ERR_INVPAR);
    return;
  }

  PhUnit = pcp->byPhysicalUnit;

#if PCITRACER
  outpw (TRPORT+2, 0xDE90);
  outpw (TRPORT, (pRP_IOCTL->Category << 8) | pRP_IOCTL->Function);
  outp	(TRPORT, PhUnit);
#endif

  if (pRP_IOCTL->Category < DSKSP_CAT_PCMCIA) {
    npU = MapUnit (PhUnit, &error);
    if (!npU ||
       ((npU->Flags & UCBF_NOTPRESENT) &&
	!((pRP_IOCTL->Category == DSKSP_CAT_GENERIC) &&
	 (pRP_IOCTL->Function == DSKSP_GET_UNIT_INFORMATION)))) {
      if (!error) error = IOERR_UNIT_NOT_READY;
      rc = error;
      goto Exit;
    }
    npA = npU->npA;
  }
#if PCITRACER
  outpw (TRPORT+2, npU);
  outpw (TRPORT+2, npA);
#endif

  if (pRP_IOCTL->Category == DSKSP_CAT_SMART) {
    UCHAR  Parameter;
    USHORT Size   = sizeof (BYTE);
    UCHAR  Access = VERIFY_READONLY;

    if (npU->Flags & UCBF_ATAPIDEVICE) goto Exit;

    switch (pRP_IOCTL->Function) {
      case DSKSP_SMART_SAVE:		Size   = 0; break;
      case DSKSP_SMART_GETSTATUS:	Size   = sizeof (DWORD);
					Access = VERIFY_READWRITE; break;
      case DSKSP_SMART_GET_ATTRIBUTES:
      case DSKSP_SMART_GET_THRESHOLDS:
      case DSKSP_SMART_GET_LOG: 	Size   = 512 * sizeof (BYTE);
					Access = VERIFY_READWRITE; break;
    }
    if (Size) {
      if (DevHelp_VerifyAccess (SELECTOROF (pRP_IOCTL->DataPacket), Size,
				OFFSETOF (pRP_IOCTL->DataPacket), Access)) {
	StatusError ((PRPH) pRP_IOCTL, STATUS_DONE | STERR | STATUS_ERR_INVPAR);
	return;
      } else {
	Parameter = pRP_IOCTL->DataPacket[0];
      }
    }

    switch (pRP_IOCTL->Function) {
      case DSKSP_SMART_ONOFF:
	rc = DoSMART (npU, (BYTE)(Parameter ? FX_SMART_ENABLE : FX_SMART_DISABLE), 0, NULL);
	break;

      case DSKSP_SMART_AUTOSAVE_ONOFF:
	rc = DoSMART (npU, FX_SMART_AUTOSAVE, (BYTE)(Parameter ? 0xF1 : 0), NULL);
	break;

      case DSKSP_SMART_AUTO_OFFLINE:
	rc = DoSMART (npU, FX_SMART_AUTO_OFFLINE, Parameter, NULL);
	break;

      case DSKSP_SMART_EXEC_OFFLINE:
	rc = DoSMART (npU, FX_SMART_IMMEDIATE_OFFLINE, Parameter, NULL);
	break;

      case DSKSP_SMART_SAVE:
	rc = DoSMART (npU, FX_SMART_SAVE, 0, NULL);
	break;

      case DSKSP_SMART_GETSTATUS:
	rc = GetSmartStatus (npU, (PDWORD) pRP_IOCTL->DataPacket);
	break;

      case DSKSP_SMART_GET_ATTRIBUTES:
	rc = DoSMART (npU, FX_SMART_READ_VALUES, 0, pRP_IOCTL->DataPacket);
	break;

      case DSKSP_SMART_GET_THRESHOLDS:
	rc = DoSMART (npU, FX_SMART_READ_THRESHOLDS, 0, pRP_IOCTL->DataPacket);
	break;

      case DSKSP_SMART_GET_LOG:
	rc = DoSMART (npU, FX_SMART_READ_LOG, Parameter, pRP_IOCTL->DataPacket);
	break;
    }
  } else if (pRP_IOCTL->Category == DSKSP_CAT_GENERIC) {
    UCHAR  Parameter;
    USHORT Size   = 0;
    UCHAR  Access = VERIFY_READWRITE;

    rc = 0;

    switch (pRP_IOCTL->Function) {
      case DSKSP_GEN_GET_COUNTERS:     Size = pRP_IOCTL->DataLen;
				       if ((Size == 0) ||
					   (Size > sizeof (DeviceCountersData)))
					 Size = sizeof (DeviceCountersData);
				       break;
      case DSKSP_GET_UNIT_INFORMATION: Size = sizeof (UnitInformationData); break;
      case DSKSP_GET_INQUIRY_DATA:     Size = sizeof (IDENTIFYDATA); break;
      case DSKSP_GET_NATIVE_MAX:       Size = 4; break;
      case DSKSP_SET_NATIVE_MAX:       Size = 5; break;
      case DSKSP_SET_PROTECT_MBR:      Size = 1;
				       Access = VERIFY_READONLY; break;
      case DSKSP_READ_SECTOR:	       Size = 512; break;
      case DSKSP_GET_DEVICETABLE:      Size = pRP_IOCTL->DataLen;
				       if (Size == 0) Size = 512; break;
      case DSKSP_POWER: 	       Size = 1;
#if 0
      case DSKSP_TEST_LASTACCESSED:    Size = 1; break;
#endif
    }

    if (Size) {
      if (DevHelp_VerifyAccess (SELECTOROF (pRP_IOCTL->DataPacket), Size,
				OFFSETOF (pRP_IOCTL->DataPacket), Access)) {
	StatusError ((PRPH) pRP_IOCTL, STATUS_DONE | STERR | STATUS_ERR_INVPAR);
	return;
      } else {
	Parameter = pRP_IOCTL->DataPacket[0];
      }
    }

    switch (pRP_IOCTL->Function) {
      case DSKSP_GEN_GET_COUNTERS:
	memcpy (pRP_IOCTL->DataPacket, (PBYTE)&npU->DeviceCounters, Size);
	break;

      case DSKSP_GET_UNIT_INFORMATION: {
	PUnitInformationData ui = (PUnitInformationData)pRP_IOCTL->DataPacket;
	ULONG TPort = PortToPhys (DATAREG, npA);
	ULONG CPort = PortToPhys (DEVCTLREG, npA);

	if (IS_MMIO (DATAREG)) {
	  ui->wRevisionNumber = 1;
	  ui->dTFBase	      = TPort;
	} else {
	  ui->wRevisionNumber = 0;
	  ui->rev0.wTFBase    = TPort;
	  ui->rev0.wDevCtl    = CPort;
	}
	ui->wIRQ    = npA->IRQLevel;
	ui->wFlags  = UIF_VALID;
	ui->wFlags |= UIF_TIMINGS_VALID;
	ui->wFlags |= (npU->Flags & UCBF_BM_DMA) ? UIF_RUNNING_BMDMA : 0;
	ui->wFlags |= (npU->UnitId) ? UIF_SLAVE : 0;
	ui->wFlags |= (npU->Flags & UCBF_ATAPIDEVICE) ? UIF_ATAPI : 0;
	ui->wFlags |= (npA->Cap & CHIPCAP_SATA) ? UIF_SATA : 0;
	ui->byPIO_Mode = npU->CurPIOMode;
	ui->byDMA_Mode = npU->CurDMAMode;
	ui->UnitFlags1 = npU->Flags;
	ui->UnitFlags2 = (USHORT)UStatus;
	break;
      }
      case DSKSP_GET_INQUIRY_DATA:
	rc = GetInquiryData (npU, pRP_IOCTL->DataPacket, 512);
	if (rc) {
	  *(PUSHORT)(pRP_IOCTL->DataPacket) = rc;
	  ((PUCHAR)(pRP_IOCTL->DataPacket))[2] = npA->icp.TaskFileOut.Status;
	  ((PUCHAR)(pRP_IOCTL->DataPacket))[3] = npA->icp.TaskFileOut.Error;
	}
	break;

      case DSKSP_GET_NATIVE_MAX:
	rc = IssueSetMax (npU, (PULONG)(pRP_IOCTL->DataPacket), -1);
	break;

      case DSKSP_SET_NATIVE_MAX:
	if (Parameter > 2) {
	  StatusError ((PRPH) pRP_IOCTL, STATUS_DONE | STERR | STATUS_ERR_INVPAR);
	  return;
	}

	rc = IssueSetMax (npU, (PULONG)(pRP_IOCTL->DataPacket + 1), Parameter + 1);
	break;

      case DSKSP_SET_PROTECT_MBR:
	npU->WriteProtect = Parameter;
	break;

      case DSKSP_READ_SECTOR:
	rc = IOCtlRead (npU, pRP_IOCTL->DataPacket, 512,
			((PReadWriteSectorParameters)pcp)->RBA);
	if (rc) {
	  *(PUSHORT)(pRP_IOCTL->DataPacket) = rc;
	  ((PUCHAR)(pRP_IOCTL->DataPacket))[2] = npA->icp.TaskFileOut.Status;
	  ((PUCHAR)(pRP_IOCTL->DataPacket))[3] = npA->icp.TaskFileOut.Error;
	}
	break;

      case DSKSP_GET_DEVICETABLE: {
	NPIORB_CONFIGURATION pIORB = (NPIORB_CONFIGURATION)ScratchBuf;

	pIORB->pDeviceTable   = (PDEVICETABLE)(pRP_IOCTL->DataPacket);
	pIORB->DeviceTableLen = pRP_IOCTL->DataLen;

	GetDeviceTable ((PIORB)pIORB);
	break;
      }

      case DSKSP_RESET:
	npU->ReqFlags |= ACBR_RESETCONTROLLER;
	NoOp (npU);
	break;

      case DSKSP_POWER: {
	PowerState = Parameter;
	if (0 == PowerState) {
	  APMResume();
	} else {
	  APMSuspend (PowerState);
	}
	break;
      }

#if 0
      case DSKSP_TEST_LASTACCESSED:
	if (LastAccessedUnit == npU) {
	  LastAccessedUnit = NULL;
	  pRP_IOCTL->DataPacket[0] = 1;
	} else {
	  pRP_IOCTL->DataPacket[0] = 0;
	}
	break;
#endif
    }

#ifdef ENABLE_SET_TIMINGS
  } else if (pRP_IOCTL->Category == DSKSP_CAT_TEST_CALLS) {
    switch (pRP_IOCTL->Function) {
      case DSKSP_TEST_CONFIGURE_UNIT:

	if (DevHelp_VerifyAccess (SELECTOROF (pcp),
				  sizeof (ConfigureUnitParameters),
				  OFFSETOF (pcp),
				  VERIFY_READONLY)) {
	  StatusError ((PRPH) pRP_IOCTL, STATUS_DONE |	STERR | STATUS_ERR_INVPAR);
	  return;
	}

	rc = SetTimingModes ((PConfigureUnitParameters) pcp);
	break;
    }
#endif
  } else if (pRP_IOCTL->Category == DSKSP_CAT_PCMCIA) {
    rc = DoIOCtlPCMCIA (pRP_IOCTL);
    status = STATUS_DONE | (rc & 0xFF);
    if (rc) status |= STERR;
    rc = FALSE;
  }

Exit:
  if (rc) {
    if (rc == IOERR_UNIT_NOT_READY) {
      status |= STERR | 0x0002;   /* unit not ready error */
    } else {
      status |= STATUS_ERR_UNKCMD;
    }
  }

  StatusError ((PRPH) pRP_IOCTL, status);
}

