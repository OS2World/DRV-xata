/**************************************************************************
 *
 * SOURCE FILE NAME = S506APM.C
 *
 * DESCRIPTIVE NAME = DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2008
 *
 * DESCRIPTION : Adapter Driver APM callback routines
 ****************************************************************************/

#define INCL_BASEAPI
#define INCL_NOPMAPI
#define INCL_NO_SCB
#define INCL_DOSERRORS
#include "os2.h"
#include "dskinit.h"

#include "iorb.h"
#include "strat2.h"
#include "reqpkt.h"
#include "dhcalls.h"
#include "addcalls.h"
#include "pcmcia.h"

#include "s506cons.h"
#include "s506type.h"
#include "s506regs.h"
#include "s506ext.h"
#include "s506pro.h"

#include "cs.h"
#include "cs_help.h"

extern RP_GENIOCTL IOCtlRP;

#pragma optimize(OPTIMIZE, on)

#define PCITRACER 0
#define TRPORT 0x9012

USHORT NEAR NotifyFLT (NPU npU, UCHAR CardEvent)
{
  NPIORB_CHANGE_UNITSTATUS npTI = (NPIORB_CHANGE_UNITSTATUS)&InitIORB;
  NPA	    npA = npU->npA;
  PUNITINFO pUI;

  pUI = npU->pUnitInfoF;
  if (SELECTOROF(pUI) == NULL) return (1);

  clrmem (npTI, sizeof(*npTI));

  npTI->iorbh.Length	      = sizeof(IORB_CHANGE_UNITSTATUS);
  npTI->iorbh.UnitHandle      = (USHORT)(pUI->UnitHandle);
  npTI->iorbh.CommandCode     = IOCC_UNIT_CONTROL;
  npTI->iorbh.CommandModifier = IOCM_CHANGE_UNITSTATUS;
  npTI->iorbh.RequestControl  = IORB_ASYNC_POST | IORB_DISABLE_RETRY;
  npTI->UnitStatus	      = (npU->Flags & UCBF_NOTPRESENT ? 0 : US_PRESENT)
			      | (npU->Flags & UCBF_BM_DMA ? US_BM_READ | US_BM_WRITE : 0)
			      | (CardEvent ? US_PORT_UPDATE : 0);
  npTI->pIdentifyData	      = (NPIDENTIFYDATA)ScratchBuf + npU->UnitIndex;
  npTI->BasePort	      = DATAREG;
  npTI->StatusPort	      = DEVCTLREG;
  npTI->IRQLevel	      = npA->IRQLevel;

  return (Execute ((UCHAR)pUI->FilterADDHandle, (NPIORB)npTI));
}

VOID NEAR Delay (USHORT ms)
{
  SHORT Stop;

  Stop = *msTimer + ms;
  while ((*msTimer - Stop) < 0);
}

/*------------------------------------*/
/* APM Suspend/Resume Support	      */
/* Reinitializes adapters and units   */
/* following a resume event.	      */
/*------------------------------------*/

USHORT FAR _cdecl APMEventHandler (PAPMEVENT Event)
{
  USHORT Message = (USHORT)Event->ulParm1;
  USHORT PwrState;

  if (Message == APM_SETPWRSTATE) {
    PowerState = PwrState = (USHORT)(Event->ulParm2 >> 16);
    if (PwrState != APM_PWRSTATEREADY)
      return (APMSuspend (PwrState));
  } else if ((Message == APM_CRITRESUMEEVENT) ||
	     (Message == APM_STBYRESUMEEVENT)) {
    PowerState = 0;
    return (APMResume());
  }
  return 0;
}

void NEAR InitUnitSync (NPU npU) {
  npU->Flags &= ~(UCBF_MULTIPLEMODE | UCBF_DIAG_FAILED);
				       npU->ReqFlags |= UCBR_SETPIOMODE;
  if (npU->Flags & UCBF_BM_DMA)        npU->ReqFlags |= UCBR_SETDMAMODE;
  if (npU->IdleTime != 0)	       npU->ReqFlags |= UCBR_SETIDLETIM;
  if (npU->Flags & UCBF_ATAPIDEVICE) return;
  if (npU->CmdSupported & UCBS_WCACHE)	 npU->ReqFlags |= UCBR_ENABLEWCACHE;
  if (npU->CmdSupported & UCBS_RAHEAD)	 npU->ReqFlags |= UCBR_ENABLERAHEAD;
  if (npU->CmdSupported & UCBS_SECURITY) npU->ReqFlags |= UCBR_FREEZELOCK;
  if (npU->Flags & UCBF_SMSENABLED)	 npU->ReqFlags |= UCBR_SETMULTIPLE;
  if (npU->SATACmdSupported & FX_SSPSUPPORTED)	npU->ReqFlags |= UCBR_ENABLESSP;
  if (npU->SATACmdSupported & FX_DIPMSUPPORTED) npU->ReqFlags |= UCBR_ENABLEDIPM;
}

void NEAR ReInitUnit (NPU npU) {
  if (npU->Flags & UCBF_NOTPRESENT) return;

  InitUnitSync (npU);
  if (npU->CmdEnabled & UCBS_HPROT) npU->ReqFlags |= UCBR_READMAX | UCBR_SETMAXNATIVE;
}

void NEAR UnitFlush (NPU npU) {
  npU->LongTimeout = TRUE;
  IssueOneByte (npU, (UCHAR)(((npU->CmdSupported >> 16) & FX_FLUSHXSUPPORTED) ?
		FX_FLUSHEXT : FX_FLUSH));
}

USHORT NEAR APMSuspend (USHORT PowerState)
{
  NPA	npA;
  NPU	npU;
  UCHAR Adapter, Unit;

  if (PowerState == APM_PWRSTATESUSPEND) {
    for (Adapter = 0; Adapter < cAdapters; Adapter++) {
      npA = ACBPtrs[Adapter];
      npU = npA->UnitCB;

      for (Unit = 0; Unit < npA->cUnits; Unit++, npU++) {
	if (npU->Flags & (UCBF_ATAPIDEVICE | UCBF_NOTPRESENT | UCBF_CFA)) continue;

	UnitFlush (npU);
	if ((Adapter | Unit) == 0) {
	  if (npU->CmdEnabled & UCBS_HPROT) {
	    IssueSetMax (npU, (PULONG)&(npU->FoundLBASize), TRUE);
	  }
	}
      }
#if ENABLEBUS
      if ((npA->FlagsT & (ATBF_PCMCIA | ATBF_BAY | ATBF_DISABLED)) == ATBF_BAY) {
	if (METHOD(npA).EnableBus) METHOD(npA).EnableBus (npA, 0);
      }
#endif
    }
  }
  return 0;
}

USHORT NEAR APMResume()
{
  NPA  npA;
  NPU  npU;
  NPC  npC;
  CHAR Adapter, Unit;

DevHelp_Beep (200, 30);

  for (npC = ChipTable; npC < (ChipTable + MAX_ADAPTERS); npC++) {
    if ((USHORT)npC->npA[0] | (USHORT)npC->npA[1])
      ConfigurePCI (npC, PCIC_RESUME);
  }

  for (Adapter = 0; Adapter < cAdapters; Adapter++) {
    npA = ACBPtrs[Adapter];

#if ENABLEBUS
    if ((npA->FlagsT & (ATBF_PCMCIA | ATBF_BAY | ATBF_DISABLED)) == ATBF_BAY) {
      if (METHOD(npA).EnableBus) METHOD(npA).EnableBus (npA, 1);
    }
#endif

    if ((npA->FlagsT & (ATBF_PCMCIA | ATBF_BAY | ATBF_DISABLED)) == ATBF_BAY) {
      // Drive Bay: try to rediscover possibly changed units
      CardRemoval (npA);
      METHOD(npA).Setup (npA);
      StartDetect (npA);
    } else if ((npA->FlagsT & ATBF_ON) && (npA->cUnits > 0)) {

      METHOD(npA).Setup (npA);

      // de-bunk some ATAPI devices...
      { USHORT i;
	for (i = ATA_BACKOFF /2 ; i > 0; i--) IODly (2 * IODelayCount);
      }
      // static adapter: wake up units

      npU = npA->UnitCB;
      for (npU = npA->UnitCB, Unit = npA->cUnits; --Unit >= 0; npU++) {
	CHAR k;

	if (npU->Flags & UCBF_NOTPRESENT) continue;

	SelectUnit (npU);
	for (k = 20; --k >= 0;)
	  if (!CheckBusy (npA)) break;

	npU->LongTimeout = TRUE;
	ReInitUnit (npU);
	NoOp (npU); // issue settings
      }
    }
  }
DevHelp_Beep (2000, 30);
  return 0;
}


USHORT NEAR ReConfigureUnit (NPU npU)
{
  NPA	npA;
  UCHAR ConfigChange = FALSE;

  npA = npU->npA;

  /*----------------------------------------*/
  /* Select unit and check status for READY */
  /*----------------------------------------*/

  if (npU->Flags & UCBF_FORCE) {
    UCHAR PIO, DMA, UDMA;
    NPIDENTIFYDATA npID = ((NPIDENTIFYDATA)ScratchBuf) + npU->UnitIndex;

#if PCITRACER
  outpw (TRPORT, 0xDF00);
  OutD	(TRPORT-2, npU->Flags);
  outpw (TRPORT-2, npA->FlagsT);
  outpw (TRPORT-2, npA->Cap);
#endif
    IdentifyDevice (npU, npID);
#if PCITRACER
  outpw (TRPORT, 0xDF01);
#endif

    PIO  = npU->CurPIOMode;
    DMA  = npU->CurDMAMode;
    UDMA = npU->UltraDMAMode;

    ConfigureUnit (npU);

    ConfigChange = ((PIO != npU->CurPIOMode) ||
		    (DMA != npU->CurDMAMode) ||
		   (UDMA != npU->UltraDMAMode) ||
		    (npU->Flags & UCBF_BM_DMA));
    npU->Flags |= UCBF_READY;
#if PCITRACER
  OutD (TRPORT-2, npU->Flags);
#endif
  }

  return (ConfigChange);
}

VOID NEAR CardRemoval (NPA npA) {
  CHAR Unit;
  NPU  npU;

  for (npU = npA->UnitCB, Unit = npA->cUnits; --Unit >= 0; npU++) {
    if (npU->Flags & UCBF_FORCE) {
      npU->CurPIOMode = 0;
      npU->Flags &= ~(UCBF_BM_DMA | UCBF_READY);
      npU->Flags |= UCBF_NOTPRESENT | UCBF_CHANGELINE;
      if (npU->FlagsF & UCBF_FAKE) NotifyFLT (npU, TRUE);
    }
  }
}

VOID NEAR CardInsertion (NPA npA) {
  UCHAR i;

  Inserting = TRUE;
  METHOD(npA).CheckIRQ = NonsharedCheckIRQ;
  CollectPorts (npA);
//DevHelp_Beep (200, 30);
  StartDetect (npA);
}

VOID NEAR StartDetect (NPA npA) {
  DRVHD = 0xA0;

  OutBdms (DRVHDREG, DRVHD);
  OutBd (DEVCTLREG, (DEVCTL = FX_DCRRes | FX_SRST | FX_nIEN), IODelayCount * RESET_ASSERT_TIME); // assert SRST
  OutBdms (DEVCTLREG, (DEVCTL = FX_DCRRes | FX_nIEN));

  *(NPUSHORT)(npA->Controller) = 0;
  npA->InsertState = ACBI_SATA_WAITPhy1;
  npA->State  = ACBS_START | ACBS_SUSPENDED;

  ADD_StartTimerMS (&npA->RetryTimerHandle, 31UL, (PFN)InsertHandler, npA);
}

VOID NEAR IssueSATAReset (NPU npU) {
  ULONG temp;
  UCHAR loop;

//  DISABLE
  temp = InD (SCONTROL) & ~0xF;
  OutD (SCONTROL, temp);	      // deassert COMRESET
  OutD (SCONTROL, temp | 1);	      // issue COMRESET
//  ENABLE
  InDd (SSTATUS, SATA_RESET_ASSERT_TIME * IODelayCount);  // flush write, wait
  OutD (SCONTROL, temp);	      // deassert COMRESET
  InDd (SSTATUS, SATA_RESET_ASSERT_TIME * IODelayCount);  // flush write, wait
  OutD (SERROR, InD (SERROR));				  // clear SERROR
  InDd (SSTATUS, IODelayCount); 			  // flush write, wait
  for (loop = 10; --loop > 0;) {
    if (!(temp = InD (SERROR))) break;
    OutD (SERROR, temp);				  // clear SERROR
    InDd (SSTATUS, IODelayCount);			  // flush write, wait
  }
}

VOID NEAR SATARemoval (NPU npU)
{
  DevHelp_Beep (300, 50);
  IssueSATAReset (npU);

  if (npU->Flags & UCBF_FORCE) {
    npU->CurPIOMode = 0;
//    npU->Flags &= UCBF_READY;
    npU->Flags &= ~(UCBF_BM_DMA | UCBF_READY | UCBF_BECOMING_READY);
    npU->Flags |= UCBF_NOTPRESENT | UCBF_CHANGELINE;
    if (npU->FlagsF & UCBF_FAKE) NotifyFLT (npU, TRUE);
  }
}

VOID NEAR StartDetectSATA (NPU npU)
{
  NPA npA = npU->npA;
  npA->npU = npU;

  DevHelp_Beep (3000, 50);
  IssueSATAReset (npU);

  DRVHD = 0xA0;
  *(NPUSHORT)(npA->Controller) = 0;
  npA->InsertState = ACBI_SATA_WAITPhy1;
  npA->State  = ACBS_START | ACBS_SUSPENDED;

  ADD_StartTimerMS (&npA->RetryTimerHandle, 31UL, (PFN)InsertHandler, npA);
}

#define TICKS(x) ((x+30)/31)

VOID FAR _cdecl InsertHandler (USHORT hTimer, PACB pA, ULONG Unused)
{
  NPA npA = (NPA)pA;
  NPU npU = npA->npU;

  if ((npA->FlagsT & ATBF_PCMCIA) && (npA->Socket == (UCHAR)-1)) // card removed in between
    npA->InsertState = ACBI_STOP;

  do {
//DevHelp_Beep ((npA->InsertState + 1) * 500, 20);

    switch (npA->InsertState) {
      case ACBI_SATA_WAITPhy1:
	npA->InsertState = ACBI_SATA_WAITPhy2;
	break;

      case ACBI_SATA_WAITPhy2:
	npA->Retries = TICKS (1000);
	npA->InsertState = npA->Cap & CHIPCAP_SATA ? ACBI_SATA_WAITPhyReady
						   : ACBI_WAITReset;
	break;

      case ACBI_SATA_WAITPhyReady:
	if ((InD (SSTATUS) & SSTAT_DET) != SSTAT_COM_OK) {
	  if (--npA->Retries == 0) {
	    npU->Flags &= ~UCBF_PHYREADY;
	    npA->InsertState = ACBI_STOP;    // give up
	  }
	} else {
	  OutD (SERROR, InD (SERROR));	      // clear SERROR
	  npU->Flags |= UCBF_PHYREADY;
	  npA->InsertState = ACBI_WAITReset;
	}
	break;

      case ACBI_WAITReset:
	npA->Retries = TICKS (30000);
	npA->InsertState = ACBI_WAITCtrlr;
	break;

      case ACBI_WAITCtrlr:
	if (TESTBUSY) {
	  if (--npA->Retries == 0) npA->InsertState = ACBI_STOP;    // give up
	} else {
	  UCHAR Controller = CtNone;

	  npA->npU = npU = &npA->UnitCB[0];

	  OutBdms (DRVHDREG , DRVHD);
	  OutBdms (DEVCTLREG, (DEVCTL = FX_DCRRes | FX_nIEN));

	  if (InBdms (DRVHDREG) == DRVHD) {
	    UCHAR Data1 = InBdms (CYLLREG);
	    if (((Data1 ==  ATAPISIGL) && (InBdms (CYLHREG) ==	ATAPISIGH)) ||
		((Data1 == SATAPISIGL) && (InBdms (CYLHREG) == SATAPISIGH))) {
	      Controller = CtATAPI;
	      npU->Flags |= UCBF_ATAPIDEVICE;
	    } else {
	      Controller = CtATA;
	      npU->Flags &= ~UCBF_ATAPIDEVICE;
	    }
	  } else {
	    break;
	  }
	  npA->Controller[0] = Controller;

	  if (!Controller || !(npU->Flags & UCBF_FORCE)) {
	    npA->InsertState = ACBI_STOP;
	  } else {
	    OutBdms (DRVHDREG, npU->DriveHead);
	    OutB (COMMANDREG, FX_CFA_GET_EXT_ERROR);
	    npA->Retries = TICKS (200);
	    npA->InsertState = ACBI_WAITCmd1;
	  }
	}
	break;

      case ACBI_WAITCmd1:
	if (TESTBUSY) {
	  if (--npA->Retries == 0) npA->InsertState = ACBI_STOP;    // give up
	} else {
	  OutBdms (FEATREG, FX_ENABLE_CFA_POWER1);
	  OutB (COMMANDREG, FX_SETFEAT);
	  npA->Retries = TICKS (200);
	  npA->InsertState = ACBI_WAITCmd2;
	}
	break;

      case ACBI_WAITCmd2:
	if (TESTBUSY) {
	  if (--npA->Retries == 0) npA->InsertState = ACBI_STOP;    // give up
	} else {
	  OutBdms (COMMANDREG, (UCHAR)(
		(npA->Controller[0] == CtATAPI) ? FX_ATAPI_IDENTIFY : FX_IDENTIFY));
	  npA->Retries = TICKS (30000);
	  npA->InsertState = ACBI_WAITIdent;
	}
	break;

      case ACBI_WAITIdent:
	if (TESTDRDY) {
	  if (--npA->Retries == 0) npA->InsertState = ACBI_STOP;    // give up
	} else {
	  UCHAR i = 0;

	  ADD_CancelTimer (hTimer);

	  do {
	    InWdms (DATAREG);
	  } while (--i != 0);
#if 1
	  npA->InsertState = ACBI_ExecDefer;
	  DevHelp_ArmCtxHook (0, CSCtxHook);
#else
	  npA->InsertState = ACBI_STOP;
#endif
	}
	break;

      case ACBI_STOP:
	npA->InsertState = -1;
	ADD_CancelTimer (hTimer);
	if (npA->FlagsT & ATBF_PCMCIA) CSUnconfigure (npA);
#if ENABLEBUS
	if ((npA->FlagsT & (ATBF_PCMCIA | ATBF_BAY | ATBF_DISABLED)) == ATBF_BAY) {
	  if (METHOD(npA).EnableBus) METHOD(npA).EnableBus (npA, 0);
	}
#endif
	return;
    }
  } while (npA->InsertState == ACBI_STOP);
}

VOID NEAR _fastcall CardInsertionDeferred (VOID) {
  USHORT rc;
  NPA	 npA;
  NPU	 npU;

  for (npA = AdapterTable; npA < (AdapterTable + MAX_ADAPTERS); npA++) {
    if (!(npA->FlagsT & ATBF_FORCE) || (npA->InsertState != ACBI_ExecDefer)) continue;

    npA->InsertState = -1;
    npU = npA->UnitCB + 0;
    npU->Flags &= ~(UCBF_BM_DMA | UCBF_NOTPRESENT);
    npU->Flags |= UCBF_CHANGELINE;
    npU->LongTimeout = -1;

    if (npA->FlagsT & ATBF_BAY) {
      OutB (DEVCTLREG, DEVCTL = FX_DCRRes);
    Reconfigure:
      if (ReConfigureUnit (npU)) METHOD(npA).Setup (npA);
#if PCITRACER
  OutD (TRPORT-2, npU->Flags);
#endif
      npU->LongTimeout = 1;
      ReInitUnit (npU);
      IssueOneByte (npU, FX_IDLEIMM);
#if PCITRACER
  outpw (TRPORT, 0xDF0F);
#endif

      if (npU->Flags & UCBF_FORCE) NotifyFLT (npU, TRUE);

    } else if (npA->FlagsT & ATBF_PCMCIA) {
      if (NULL == HookIRQ (npA)) goto Fail;

      npA->IntHandler = FixedInterrupt;
      OutB (DEVCTLREG, DEVCTL = FX_DCRRes);
      rc = IssueOneByte (npU, FX_IDLEIMM);

      if (!rc || (rc == IOERR_DEVICE_REQ_NOT_SUPPORTED)) {
	DevHelp_Beep (3000, 50);
	goto Reconfigure;
      } else {
	Fail:
	DevHelp_Beep (300, 50);
	DevHelp_ProcBlock ((ULONG)(PVOID)&CardInsertionDeferred, 100UL, 0);
	DevHelp_Beep (300, 50);
	CSUnconfigure (npA);
      }
    }
  }

  if (Inserting) {
    Inserting = FALSE;
#if AUTOMOUNT
    if (SELECTOROF (OS2LVM)) {

      UStatus = 0;
      IOCtlRP.Category = 9;
      IOCtlRP.Function = 0x6A;
      IOCtlRP.ParmPacket = (PUCHAR)&UStatus + 1;
      IOCtlRP.DataPacket = (PUCHAR)&UStatus + 0;

      _asm { push ds
	     pop es
	     mov bx, offset IOCtlRP }
      OS2LVM();
    }
#endif
    DevHelp_ProcRun ((ULONG)(PVOID)&CardInsertion, &rc);
  }
}
