/**************************************************************************
 *
 * SOURCE FILE NAME = S506INIT.C
 * $Id$
 *
 * DESCRIPTIVE NAME = DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : Adapter Driver initialization routines.
 *
 *****************************************************************************/

#define INCL_NOPMAPI
#define INCL_DOSINFOSEG
#define INCL_NO_SCB
#define INCL_INITRP_ONLY
#define INCL_DOSERRORS
#include "os2.h"
#include "dskinit.h"
#include <stddef.h>

#include "devhdr.h"
#include "iorb.h"
#include "reqpkt.h"
#include "dhcalls.h"
#include "addcalls.h"
#include "devclass.h"

#include "s506cons.h"
#include "s506type.h"
#include "s506regs.h"
#include "s506ext.h"
#include "s506pro.h"

#include "Trace.h"

#pragma optimize(OPTIMIZE, on)

#define PCITRACER 0
#define TRPORT 0xA810

/*------------------------------------*/
/*				      */
/*				      */
/*				      */
/*------------------------------------*/

VOID FAR _cdecl _loadds NotifyIORBDone (PIORB pIORB)
{
  USHORT AwakeCount;

  if (pIORB->Status & IORB_DONE)
    DevHelp_ProcRun ((ULONG) pIORB, &AwakeCount);
}

UCHAR NEAR TestStatus50us (NPA npA)
{
  USHORT Stop;
  UCHAR  Data;

  Stop = 50;
  while (--Stop != 0) { // wait for BUSY | DRDY | ERR within 50us
    Data = InBdms (STATUSREG);
    if (Data & (FX_BUSY | FX_DRQ | FX_ERROR)) break;
  }
TS(",%X",Data);
  return (Data);
}

UCHAR NEAR WaitNotBusy (NPA npA)
{
  USHORT Stop;
  UCHAR  Data;

  for (Stop = 0; --Stop != 0;) {
    Data = InBdms (STATUSREG);
    if (!(Data & FX_BUSY) || ((Data & 0x7F) == 0x7F)) break;
    IODly (-1);
  }
TS(",%X",Data);
  return ((Data & FX_BUSY) || (Data == 0x7F));
}

UCHAR NEAR WaitNotBusyDRQ (NPA npA)
{
  USHORT Stop;
  UCHAR  Data;

  for (Stop = 0; --Stop != 0;) {
    Data = InBdms (STATUSREG);
    if (!(Data & (FX_BUSY | FX_DRQ))) break;
    IODly (-1);
  }
TS(",%X",Data);
  return ((Data & (FX_BUSY | FX_DRQ)) || (Data == 0x7F));
}

UCHAR NEAR TryCommand (NPA npA, UCHAR Command)
{
  UCHAR Data;

  OutBdms (FEATREG, 0);
  OutBdms (COMMANDREG, Command);
  Data = TestStatus50us (npA);
  if (!(Data & (FX_BUSY | FX_DRQ | FX_ERROR))) return (1);
  Data = InBdms (STATUSREG);
  if (Data & (FX_DF | FX_CORR)) return (1); // remains from unterminated dangling unused cable
  if (Data & FX_BUSY) {
    if (WaitNotBusy (npA)) return (1);
  }
  return (0);
}

USHORT NEAR TestChannel (NPA npA, UCHAR UnitId)
{
  UCHAR  Data0, Data1;
  USHORT Stop;
  UCHAR  Controller = CtNone;

  Data0 = UnitId ? 0xB0 : 0xA0;
  DISABLE
  OutBdms (DRVHDREG, Data0); /* Select Unit */
  OutBd (DEVCTLREG, FX_DCRRes | FX_nIEN, 10 * IODelayCount);
  for (Stop = 0; Stop < 100; Stop++) {
    Data1 = InBdms (DRVHDREG);
    if (!((Data1 ^ Data0) & 0x10)) break;
    OutBdms (DRVHDREG, Data0); /* Select Unit */
  }
  Data0 = InBdms (STATUSREG);
  ENABLE

  TS("%02X:",Data1);
  TS("%02X",Data0);

  if (Data0 & FX_DRQ) { // probably remains from write to non-existent unit
    for (Stop = 1; Stop <= 4096; Stop++) {
      InWdms (DATAREG);
      Data0 = InBdms (STATUSREG);
      if (!(Data0 & FX_DRQ)) break;
    }
    if (Data0 & FX_DRQ) goto Fault;

    if (BMSTATUSREG) OutB (BMSTATUSREG, InB (BMSTATUSREG));

    TS("r%d",Stop)
    TS(":%02X",Data0);
  }

  if (WaitNotBusy (npA)) {
    Data0 = InBdms (STATUSREG);
    if ((Data0 & FX_DRDY) || (Data0 == FX_BUSY)) {
      OutBdms (COMMANDREG, FX_ATAPI_RESET);
      if (WaitNotBusy (npA)) goto Fault;
    } else {
      USHORT j;

  Fault:
      for (j = ATA_BACKOFF/2; j > 0; j--) IODly (2*IODelayCount);

      Controller = CtNone;
      goto MakeDecision;
    }
  }

  Controller = CtATA;

  if (TryCommand (npA, FX_IDENTIFY)) goto Fault;

  Data1 = InBdms (STATUSREG);
  if (Data1 & FX_ERROR) {  // possibly ATAPI
    UCHAR haveSig = FALSE;

    Controller = CtATAPI;
    Data1 = InBdms (CYLLREG);
    if (((Data1 ==  ATAPISIGL) && (InBdms (CYLHREG) ==	ATAPISIGH)) ||
	((Data1 == SATAPISIGL) && (InBdms (CYLHREG) == SATAPISIGH))) {
T('a')
      haveSig = TRUE;
      OutBdms (CYLLREG, 0);
    }

    if (TryCommand (npA, FX_ATAPI_IDENTIFY)) goto Fault;
    Data1 = InBdms (STATUSREG);
    if (Data1 & FX_ERROR) {
      if (!haveSig) {
	goto Fault;
      } else {
	TryCommand (npA, FX_ATAPI_RESET);
	OutBdms (CYLLREG, 0);	// clear signature from ATAPI_RESET
	if (TryCommand (npA, FX_ATAPI_IDENTIFY)) goto Fault;
	Data1 = InBdms (STATUSREG);
      }
    }

    if ((Controller == CtATAPI) && !(Data1 & FX_DRQ)) goto MakeDecision; //broken device
  }

  if (Data1 & FX_DRQ) {
    UCHAR i = 0;
    USHORT j;

    do {
      InWdms (DATAREG);
    } while (--i != 0);

    if (BMSTATUSREG) OutB (BMSTATUSREG, InB (BMSTATUSREG));

    for (j = ATA_BACKOFF/2; j > 0; j--) IODly (2*IODelayCount);

    if (WaitNotBusyDRQ (npA)) goto Fault;
  } else {
    goto Fault;
  }
MakeDecision:

  OutB (FEATREG, 0);

  npA->Controller[UnitId] = Controller;
#if TRACES
  if (!Controller) {
T('-')
  }
TS("'%d", Controller)
#endif

  return (!Controller);
}

USHORT NEAR CheckController (NPA npA)
{
  USHORT rc;

T('R') T('(')

  npA->Controller[0] = CtNone;
  npA->Controller[1] = CtNone;

  if (!(npA->UnitCB[0].FlagsT & UTBF_DISABLED)) TestChannel (npA, 0);
  rc = !(npA->Controller[0]);

  if (npA->FlagsT & (ATBF_PCMCIA | ATBF_BAY)) {
    //npA->Controller[1] = CtATA;
  } else {
    if (npA->maxUnits >= 2) {
      if (!(npA->UnitCB[1].FlagsT & UTBF_DISABLED)) TestChannel (npA, 1);
      rc &= !(npA->Controller[1]);
    }
  }

  OutBdms (DRVHDREG, (UCHAR)(((npA->Controller[0] == CtNone) && (npA->Controller[1] != CtNone)) ? 0xB0 : 0xA0));

T(')')

  return (rc);
}


USHORT FAR Execute (UCHAR ADDHandle, NPIORB npIORB)
{
  npIORB->Status	= 0;
  npIORB->NotifyAddress = (PIORBNotify)NotifyIORBDone;

  if (ADDHandle)
    (*(PADDEntry FAR *)&(pDriverTable->DCTableEntries[ADDHandle-1].DCOffset))((PIORB)(npIORB));
  else
    ADDEntryPoint ((PIORB)npIORB);

  DISABLE
  while (!(npIORB->Status & IORB_DONE)) {
    DevHelp_ProcBlock ((ULONG)(PIORB)npIORB, -1, WAIT_IS_INTERRUPTABLE);
    DISABLE
  }
  ENABLE

  return ((npIORB->Status & IORB_ERROR) ? npIORB->ErrorCode : 0);
}

/*------------------------------------*/
/*				      */
/* NoOp()			      */
/*				      */
/*------------------------------------*/
USHORT NEAR NoOp (NPU npU)
{
  NPIORB_EXECUTEIO npTI = (NPIORB_EXECUTEIO)&(npU->npA->PTIORB);

  if (!(npU->ReqFlags)) return (0);

  clrmem (npTI, sizeof (IORB_EXECUTEIO));

  npTI->iorbh.Length	      = sizeof (IORB_EXECUTEIO);
  npTI->iorbh.CommandCode     = IOCC_EXECUTE_IO;
  npTI->iorbh.UnitHandle      = (USHORT) npU;
  npTI->iorbh.CommandModifier = IOCM_NO_OPERATION;
  npTI->iorbh.RequestControl  = IORB_ASYNC_POST | IORB_DISABLE_RETRY;

  return (Execute (0, (NPIORB)npTI));
}


NPIHDRS FAR HookIRQ (NPA npA) {
  NPIHDRS p;
  USHORT  rc;

  for (p = IHdr; p < (IHdr + MAX_IRQS); p++) {
    if (p->IRQLevel == 0) {
      p->IRQLevel  = npA->IRQLevel;
      p->IRQShared = (npA->FlagsT & ATBF_INTSHARED) ? 1 : 0;
      rc = DevHelp_SetIRQ ((NPFN)p->IRQHandler, p->IRQLevel, p->IRQShared);
      if (rc && p->IRQShared) {
	p->IRQShared = 0;
	rc = DevHelp_SetIRQ ((NPFN)p->IRQHandler, p->IRQLevel, p->IRQShared);
	if (rc) {
	  p->IRQLevel = 0;
	  return (NULL);
	}
      }
    }
    if (p->IRQLevel == npA->IRQLevel) {
      npA->npIHdr    = p;
      npA->npIntNext = p->npA;
      p->npA	     = npA;
      return (p);
    }
  }
  return (NULL);
}

VOID FAR UnhookIRQ (NPA npA) {
  NPIHDRS p = npA->npIHdr;

  if (p) {
    if ((p->npA = npA->npIntNext) == NULL) {
      DevHelp_UnSetIRQ (p->IRQLevel);
      p->IRQLevel = 0;
    }
  }
}

VOID FAR RehookIRQ (NPA npA) {
  NPIHDRS p = npA->npIHdr;

  if (p) {
    DISABLE
    DevHelp_UnSetIRQ (p->IRQLevel);
    DevHelp_SetIRQ ((NPFN)p->IRQHandler, p->IRQLevel, p->IRQShared);
    ENABLE
  }
}


void NEAR SetupPCMCIA (void);
void NEAR SetupPCI (void);
void AssignPCIChips (void);
void ScanForUnits (void);
void ReadVerifyPIO (void);
void VerifyInitPost (void);
void AssignSGList (void);

UCHAR counted = 0;

#define NOTCOUNTED(x) (!(counted & (1 << (x))))
#define SETCOUNTED(x) (counted |= 1 << (x))

/*------------------------------------*/
/*				      */
/* DriveInit()			      */
/*				      */
/*------------------------------------*/

VOID NEAR DriveInit (PRPINITIN pRPH)
{
  NPA	 npA;
  NPC	 npC;
  BOOL	 rc = TRUE;
  PSEL	 p;
  PSZ	 pCmdLine;
  USHORT BusInfo;

#if PCITRACER
  outpw (TRPORT+2, 0xDEDE);
#endif

  Device_Help = pRPH->DevHlpEP;
  DevHelp_GetDOSVar (DHGETDOSV_SYSINFOSEG, 0, (PPVOID)&p);
  SELECTOROF (pGlobal) = *p;
  msTimer = (PSHORT)&(pGlobal->msecs);
  pDDD_Parm_List = (PDDD_PARM_LIST)((PRPINITIN)pRPH)->InitArgs;
  WritePtr = (PCHAR)PciGetReg;
  DevHelp_LockLite (SELECTOROF (WritePtr), LOCKTYPE_LONG_HIGHMEM << 8);
  WritePtr = (PCHAR)&WritePtr;
  DevHelp_VirtToPhys (MAKEP (SELECTOROF (WritePtr), 0), (PULONG)&ppDataSeg);
  WritePtr = MsgBuffer;
  BusInfo = ((PMACHINE_CONFIG_INFO)MAKEP (SELECTOROF (pDDD_Parm_List),
					  pDDD_Parm_List->machine_config_table))->BusInfo;
  if (BusInfo & (BUSINFO_EISA | BUSINFO_MCA))
    goto S506_Deinstall;

  pCmdLine = MAKEP (SELECTOROF (pDDD_Parm_List), (USHORT)pDDD_Parm_List->cmd_line_args);
  IODelayCount = Delay500 * 2;
  DevHelp_AllocateCtxHook ((NPFN)&CSHookHandler, (PULONG)&CSCtxHook);
  RMCreateDriver (&DriverStruct, &hDriver);

//DevHelp_Beep (1000, 50);
//DevHelp_ProcBlock ((ULONG)(PVOID)&CompleteInit, 1000UL, 0);

TINIT
#if TRACES
TotalTime();
#endif

  InitActive = 2;

  if (ParseCmdLine (pCmdLine)) {
    sprntf (TraceBuffer, ParmErrMsg, (USHORT)pcmdline1 - (USHORT)pDDD_Parm_List->cmd_line_args);
    TTYWrite (0, TraceBuffer);
    TTYWrite (0, VPauseVerbose0);
    LinesPrinted = 0;
  }

  {
    PSZ   p, pCmd;
#if TRACES
    NPSZ  q = "R"VERSION;
#else
    NPSZ  q = "u"VERSION" (!!)";
#endif
    UCHAR c;

    pCmd = p = pCmdLine;
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

  if (!(Debug & 0x200)) AssignPCIChips ();
  if (!cChips) CheckLegacyPorts ();

  ADD_InitTimer (TimerPool, sizeof(TimerPool));

  /*----------------------------------------*/
  /* Setup each Adapter. Scan each adapter  */
  /* for drives (units).		    */
  /*----------------------------------------*/
  DriveId = ScanForOtherADDs() + 0x80;

  if (!(Debug & 0x400)) {
    FindHotPCCard();
    SetupPCMCIA();
  }

  ADD_StartTimerMS (&ElapsedTimerHandle, ELAPSED_TIMER_INTERVAL, (PFN)ElapsedTimer, 0);
TINIT
  ScanForUnits();
TEND

  for (npC = ChipTable; npC < (ChipTable + MAX_ADAPTERS); npC++) {
    if ((USHORT)npC->npA[0] | (USHORT)npC->npA[1])
      ConfigurePCI (npC, PCIC_START);
  }

#if PCITRACER
outpw (TRPORT+2, 0xDEDC);
#endif

  InitIOComplete = 1;

TINIT

#if PCITRACER
outpw (TRPORT+2, 0xDEDD);
#endif

  SetupPCI();
TTIME TWRITE(1)

  PrintInfo (AdapterTable);

  if (!cUnits && !CSHandle && !ADDHandle && !(Debug & 0x8000)) goto S506_Deinstall;

  MemTop = DTMem;
  AssignSGList();

#if PCITRACER
outpw (TRPORT+2, 0xDEDB);
#endif

  InitActive = 1;

TINIT
  VerifyInitPost();
TWRITE(1)

S506_Install:

npTrace=TraceBuffer;DiffTime();TraceTTime();

TWRITE(1)
TEND

#define pRPO ((PRPINITOUT)pRPH)

  *(WritePtr++) = 0x1A;
  pRPO->CodeEnd    = (USHORT) &_CodeEnd;
  pRPO->DataEnd    = (USHORT) MemTop - 1;
  pRPO->rph.Status = STDON;

  /*
  ** Put any PCI Configuration Registers back to bios' state.
  */

  for (npC = ChipTable; npC < (ChipTable + MAX_ADAPTERS); npC++) {
    if ((USHORT)npC->npA[0] | (USHORT)npC->npA[1]) {
      ConfigurePCI (npC, PCIC_START_COMPLETE);

      memcpy (WritePtr, npC->CfgTblBIOS, 2 * sizeof (npC->CfgTblBIOS));
      WritePtr += 2 * sizeof (npC->CfgTblBIOS);
    }
  }

  rc = FALSE;

  // de-bunk some ATAPI devices...
  { USHORT i;
    for (i = ATA_BACKOFF; i > 0; i--) IODly (IODelayCount);
  }

S506_Deinstall:

  if (rc) {
#if PCITRACER
outpw (TRPORT+2, 0xDEEE);
#endif
    ADD_DeInstallTimer();

    pRPO->CodeEnd = pRPO->DataEnd = (USHORT)MemTop = 0;
    pRPO->rph.Status = STDON + ERROR_I24_QUIET_INIT_FAIL;

    RMDestroyDriver (hDriver);
    DevHelp_FreeCtxHook (CSCtxHook);
  }

  WritePtrRestart = (USHORT)WritePtr;
  Debug = 0;
  InitActive = 0;
  Beeps = -Beeps;

//DevHelp_Beep (1600, 50);
//DevHelp_ProcBlock ((ULONG)(PVOID)&CompleteInit, 1000UL, 0);

#if PCITRACER
  outpw (TRPORT+2, 0xDEDE);
#endif
}

void NEAR SetupPCMCIA (void) {
  NPA	npA;
  UCHAR numS;

  /*
  ** Register this driver with PCMCIA socket services if there
  ** were any PCMICA adapters specified on the command line.
  */

  if (MaxNumSockets > 0) PCMCIASetup();

  for (numS = MaxNumSockets; numS > 0; numS--) {
    npA = LocateATEntry (0, 0, 0);
    if (npA) {
      npA->Cap = 0;
      npA->FlagsT |= (ATBF_ON | ATBF_PCMCIA | ATBF_BIOSDEFAULTS);
      MEMBER(npA).Vendor = 0;
      npA->maxUnits = 1;
    }
  }
}

void NEAR SetupPCI (void) {
  NPA npA;

  if (PCIClock < 20) PCIClock = 33;

  for (npA = AdapterTable; npA < (AdapterTable + MAX_ADAPTERS); npA++) {
    if ((npA->FlagsT & ATBF_ON) && (npA->cUnits > 0)) {
      GetChipPIOMode (npA);
      METHOD(npA).Setup (npA);
    }
  }
}

void AssignPCIChips (void) {
  UCHAR Count;

  ACPISetup();
  Count = EnumPCIDevices();

TWRITE(8)
}

void NoMaster (NPA npA) {
  npA->UnitCB[0].Status = UTS_NO_MASTER;
  npA->UnitCB[0].Flags |= (UCBF_NODASDSUPPORT | UCBF_NOTPRESENT);
  if (NOTCOUNTED(0)) {
    npA->cUnits++;
    cUnits++;
    SETCOUNTED(0);
  }
}

void NEAR AddUnit (NPU npU, UCHAR Type) {
  NPA npA = npU->npA;

  npU->Status  = UTS_OK;
  npU->Flags  &= ~UCBF_ATAPIDEVICE;
  if (Type == CtATAPI)
    npU->Flags |= UCBF_ATAPIDEVICE;

  npA->FlagsT |= ATBF_ON;
  if (NOTCOUNTED(npU->UnitIndex)) {
    npA->cUnits++;
    cUnits++;
    SETCOUNTED(npU->UnitIndex);
  }
}

void ScanForUnits (void) {
  NPA	npA;
  NPU	npU;
  UCHAR TimeOut, rc;

  for (npA = AdapterTable; npA < (AdapterTable + MAX_ADAPTERS); npA++) {
    if (npA->FlagsT & ATBF_DISABLED) continue;
    if (!DATAREG && !(npA->FlagsT & ATBF_PCMCIA)) continue;

    if (!ConfigureController (npA)) {
      for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->maxUnits); npU++) {
	rc = ConfigureUnit (npU);
	if (!rc && (npU == &npA->UnitCB[1]) && NOTCOUNTED(0))
	  NoMaster (npA); // Have found a slave device with no master.	To keep
			  // the Unit and UnitCB structures in sync with nUnits
			  // must count the master unit but mark it as not present.

	if ((TimeOut = npU->TimeOut) < MIN_USER_TIMEOUT)
	  TimeOut = DEFAULT_USER_TIMEOUT;

	npA->TimeOut = 1000L * TimeOut;
      }
      AssignAdapterResources (npA);
    }

#   if TRACES
    if (Debug & 1) TraceStr ("S:%d[%d/%d]", npA->Status, npA->cUnits, cUnits);
    TWRITE(1)
#   endif
  } // for
}

void AssignSGList (void) {
  NPA	 npA;
  USHORT p;
  UCHAR  i, j;

  p = (USHORT)ppDataSeg & (MR_1K_LIMIT - 1);
  p = (((USHORT)MemTop + p + (MR_1K_LIMIT - 1)) & ~(MR_1K_LIMIT - 1)) - p;
  for (i = 0; i < cAdapters; i++) {
    BOOL busmaster = FALSE;

    npA = ACBPtrs[i];
    if ((npA->FlagsT & (ATBF_BM_DMA | ATBF_BAY)) == (ATBF_BM_DMA | ATBF_BAY))
      busmaster = TRUE;
    for (j = 0; j < npA->cUnits; j++)
      if (npA->UnitCB[j].Flags & UCBF_BM_DMA)
	busmaster = TRUE;

    if (busmaster) {
      npA->pSGL  = (NPPRD)p;
      npA->ppSGL = ppDataSeg + p;
      p += MR_1K_LIMIT;
    }
  }
  MemTop = (NPVOID)p;
}

void VerifyInitPost (void) {
  NPA	npA;
  NPU	npU;
  UCHAR i;

  for (i = 0; i < cAdapters; i++) {
    T('V') T('[')
    npA = ACBPtrs[i];
    RehookIRQ (npA);

    for (npU = npA->UnitCB; npU < (npA->UnitCB + MAX_UNITS); npU++) {
      if (!(npU->Flags & UCBF_NOTPRESENT)) {
	NPIDENTIFYDATA npID = (NPIDENTIFYDATA) ScratchBuf;
T('a')
	IdentifyDevice (npU, npID);
TTIME
      }

      memset (&(npU->DeviceCounters), 0, sizeof (DeviceCountersData));
      npU->DeviceCounters.wRevisionNumber = 1;
    }

  npA->IRQTimeOut = (npA->IRQTimeOutSet > 0) ? 100 * npA->IRQTimeOutSet
					     : IRQ_TIMEOUT_INTERVAL;

  SelectUnit ((npA->UnitCB[0].Flags & UCBF_NOTPRESENT) ? npA->UnitCB + 1 : npA->UnitCB);
  T(']')
  }
}

VOID NEAR ConfigureACB (NPA npA)
{
  UCHAR i;
  NPU	npU;

  ACBPtrs[cAdapters] = npA;

  npA->BM_StopMask = npA->HardwareType == ALi ? 0 : BMICOM_RD;

  DEVCTL = FX_DCRRes;

  npA->TimeOut		    = INIT_TIMEOUT_SHORT;
  npA->IRQTimeOut	    = 200;			    // enable IRQ polling
  npA->IRQLongTimeOut	    = IRQ_LONG_TIMEOUT_INTERVAL;
  npA->DelayedRetryInterval = DELAYED_RETRY_INTERVAL;
  npA->DelayedResetInterval = DELAYED_RESET_INTERVAL;
  npA->npIntNext	    = NULL;
  npA->IntHandler	    = CatchInterrupt;

  DevHelp_AllocGDTSelector (&(npA->IOSGPtrs.Selector), 1);
  if (npA->IRQLevel) {
    HookIRQ (npA);
  }

  for (i = 0, npU = npA->UnitCB; npU < (npA->UnitCB + MAX_UNITS); i++, npU++) {
    npU->UnitIndex = i;
    npU->UnitId    = i;
    npU->DriveHead = i ? 0xF0 : 0xE0;
    npU->ModelNum  = npA->ModelNum[i];
    npU->npA = npU->npAF = npA;

    npU->MaxXSectors = MAX_XFER_SEC;	// initialize to default
  }

  /*
  ** Move any PCI Configuration data to the ACB.
  */
  npA->IODelayCount = npA->IRQDelayCount = IODelayCount;
  if (MEMBER(npA).Vendor) {
    npA->IRQDelayCount *= npA->IRQDelay;

  } else {
    memcpy (&npA->PCIInfo.Ident, &PCIDevice[PCID_Generic].Ident, sizeof (PCI_IDENT));
    npA->HardwareType = generic;
    if (!npA->maxUnits) npA->maxUnits = 2;
  }
  if (!METHOD(npA).GetPIOMode) METHOD(npA).GetPIOMode = GetGenericPio;
  if (!METHOD(npA).SetTF)      METHOD(npA).SetTF      = GenericSetTF;
  if (!METHOD(npA).GetTF)      METHOD(npA).GetTF      = GenericGetTF;
  if (!METHOD(npA).SetupDMA)   METHOD(npA).SetupDMA   = GenericSetupDMA;
  if (!METHOD(npA).StartDMA)   METHOD(npA).StartDMA   = GenericStartOp;
  if (!METHOD(npA).StopDMA)    METHOD(npA).StopDMA    = GenericStopDMA;
  if (!METHOD(npA).ErrorDMA)   METHOD(npA).ErrorDMA   = GenericErrorDMA;
  if (!METHOD(npA).Setup || (npA->FlagsT & ATBF_BIOSDEFAULTS)) {
    METHOD(npA).Setup		= SetupCommon;
    METHOD(npA).CalculateTiming = CalculateGenericAdapterTiming;
    METHOD(npA).ProgramChip	= NULL;
  }

  /*---------------------------------------------*/
  /* Initialize the ACB's state machine to       */
  /* process requests.				 */
  /*---------------------------------------------*/
  npA->State = ACBS_START | ACBS_SUSPENDED;
}

VOID NEAR ConfigurePCCard (NPA npA)
{
  static UCHAR Socket = 0;

  PCCardPtrs[cSockets++] = npA;
  if (Socket == 0) Socket = FirstSocket;
  while (Socket < (FirstSocket + NumSockets)) {
    if (0 == CSCardPresent (Socket)) {
TTIME
      SocketPtrs[Socket] = npA;
      npA->npPCIDeviceMsg = npA->PCIDeviceMsg;
      memcpy (npA->PCIDeviceMsg, GetTuple.GTD_TupleData, 20);
      npA->PCIDeviceMsg[19] = '\0';
      if (CSConfigure (npA, Socket) == 0) {
	break;
      }
      SocketPtrs[Socket] = NULL;
    }
    Socket++;
  }
}

VOID FAR CollectPorts (NPA npA)
{
  UCHAR i;

  for (i = FI_PFEAT; i <= FI_PCMD; i++)
    npA->IOPorts[i] = DATAREG + i;

  if (!DEVCTLREG) DEVCTLREG = npA->IOPorts[0] + 14; // default offset

  if (BMCMDREG < 0x100) BMCMDREG = 0;
  if (BMCMDREG) {
    BMSTATUSREG = BMCMDREG + 2;
    BMDTPREG	= BMCMDREG + 4;
  }
}

/**
 * Configure controller
 *
 *  Activate this controller's ACB:
 *  - Move any initialization data required to persist from
 *   the Adapter Table (npA) to the Adapter Control Block (npA).
 *  - Hook the IRQ.
 * @return 0 if OK else error code
 */

USHORT NEAR ConfigureController (NPA npA)
{
  NPU	 npU;
  UCHAR  rc;
  UCHAR  Unit, *pC;
  NPIDENTIFYDATA npID;

  TS("CF:%X[",npA->FlagsT)

  counted = 0xFC;

  if (npA->FlagsT & ATBF_PCMCIA) ConfigurePCCard (npA);
  if (!STATUSREG) CollectPorts (npA);

  if (AllocAdapterResources (npA)) {
    /* Adapter probably controlled by another ADD */
    npA->FlagsT |= ATBF_DISABLED;
    npA->Status = ATS_NOT_PRESENT;
    goto ConfigureControllerExit;
  }

  ConfigureACB (npA);
  if (npA->Status != ATS_OK)
    goto ConfigureControllerExit;

  *(NPUSHORT)(npA->Controller) = 0;
  clrmem (ScratchBuf, sizeof(ScratchBuf));

  npA->UnitCB[0].Status = npA->UnitCB[1].Status = -1;
  npA->UnitCB[0].Flags |= UCBF_NOTPRESENT;
  npA->UnitCB[1].Flags |= UCBF_NOTPRESENT;

  rc = 0;
  if (DATAREG)
    rc = CheckController (npA);

  pC = npA->Controller;
  if (npA->FlagsT & ATBF_FORCE) {

    for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->maxUnits); npU++, pC++) {
      if (npA->FlagsT & ATBF_PCMCIA) npU->Flags |= UCBF_PCMCIA;
      npU->Flags |= UCBF_FORCE | UCBF_CHANGELINE | UCBF_REMOVABLE;
      npU->FlagsF = npU->Flags | UCBF_ATAPIDEVICE | UCBF_FAKE | UCBF_NODASDSUPPORT;
      if (*pC) {
	npU->Status = 0;
	npU->Flags &= ~UCBF_NOTPRESENT;
	npU->Flags |= UCBF_READY;
      } else {
	npU->Status = (npA->Socket != (UCHAR)-1) ? UTS_NOT_INITIALIZED : UTS_NOT_PRESENT;
      }
      AddUnit (npU, *pC);
    }

  } else {

    for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->maxUnits); npU++, pC++) {
      if (*pC) {
	AddUnit (npU, *pC);
	npU->Flags &= ~UCBF_NOTPRESENT;
      }
    }
    if (rc) {
      npA->Status = ATS_NOT_PRESENT;
      goto ConfigureControllerExit;
    }
  }

  cAdapters++;

  if (!DATAREG) {
    T('p')
    goto ConfigureControllerExit;
  }

  /*---------------------------------------------*/
  /* Activate the ACB				 */
  /*						 */
  /* This Hooks the requested IRQ level if not	 */
  /* already hooked				 */
  /*---------------------------------------------*/

  RehookIRQ (npA);
  npA->IntHandler = FixedInterrupt;

  CSIrqAvail &= ~(1 << npA->IRQLevel);

  OutBdms (DEVCTLREG, DEVCTL);
  RehookIRQ (npA);

TSTR("F:%X/%X", npA->Flags, npA->FlagsT);

  for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->maxUnits); npU++) {
    npID = ((NPIDENTIFYDATA)ScratchBuf) + npU->UnitIndex;

    TSTR("u%X/%lX", npU->FlagsT, npU->Flags);

    if (npU->Flags & UCBF_NOTPRESENT) continue;
    IdentifyDevice (npU, npID);

    TS("H:%X,", npID->HardwareTestResult)
    T('i')
    TTIME
  }

  if (METHOD(npA).EnableInterrupts) METHOD(npA).EnableInterrupts (npA);

ConfigureControllerExit:
  if (npA->Status)
    UnhookIRQ (npA);
  else
    RehookIRQ (npA);
  if ((npA->Status) && !(npA->FlagsT & ATBF_FORCE)) {
    DeallocAdapterResources (npA);
    T('h')
  }
  T(']')
  TTIME
  return (npA->Status);
}


/*------------------------------------*/
/*				      */
/* ConfigureUnit()		      */
/*				      */
/*------------------------------------*/
USHORT ConfigureUnit (NPU npU)
{
  TTIME
  T('U') T('(')
  //  if ((npU->Flags & (UCBF_FORCE | UCBF_ATAPIDEVICE)) == UCBF_FORCE) { // ATA only
  if (npU->Flags & UCBF_FORCE) {
    npU->Flags |= UCBF_REMOVABLE;
    if (npU->Flags & UCBF_PCMCIA) npU->Flags |= UCBF_CFA;

    // set a faked hardware reported Physical geometry.
    npU->IDEPhysGeom.SectorsPerTrack = 32;
    npU->IDEPhysGeom.NumHeads	     = 4;
    npU->IDEPhysGeom.TotalCylinders  = 16;
    npU->IDEPhysGeom.TotalSectors    = 16 * 4 * 32;
    npU->FlagsT |= UTBF_IDEGEOMETRYVALID;
  }

  if (npU->Flags & UCBF_NOTPRESENT) {
    npU->Status = UTS_NOT_PRESENT;
  } else {
    // Get the hardware reported Physical geometry.
    // Increment the BIOS DriveID if an HDD was found
    if (!IdentifyGeometryGet (npU) && InitActive) npU->DriveId = DriveId++;
    npU->Status = UTS_OK;
  }

  if ((npU->Flags & (UCBF_ATAPIDEVICE | UCBF_FORCE)) != UCBF_ATAPIDEVICE)
    DetermineUnitGeometry (npU);

TTIME
TSTR("F:%X/%lXS:%d)", npU->FlagsT, npU->Flags, npU->Status);
  return (npU->Status);
}


/*------------------------------------*/
/*				      */
/* ReadDrive()			      */
/*				      */
/*------------------------------------*/
USHORT ReadDrive (NPU npU, ULONG RBA, USHORT Retry, NPBYTE pBuf)
{
  NPIORB_EXECUTEIO npTI = &InitIORB;
  USHORT rc;
  ULONG UCBFlags;

  if (RBA > npU->PhysGeom.TotalSectors) return (IOERR_RBA_LIMIT);

  UCBFlags = npU->Flags;

  clrmem (npTI, sizeof(IORB_EXECUTEIO));

  if (!pBuf) {
    npTI->iorbh.CommandModifier = IOCM_READ_VERIFY;
  } else {
    npTI->iorbh.CommandModifier = IOCM_READ;

    ScratchSGList.ppXferBuf = ppDataSeg + (USHORT)pBuf;
    npTI->cSGList	    = 1;
    npTI->pSGList	    = &ScratchSGList;
  }

  npTI->iorbh.Length	     = sizeof (IORB_EXECUTEIO);
  npTI->iorbh.CommandCode    = IOCC_EXECUTE_IO;
  npTI->iorbh.UnitHandle     = (USHORT)npU;
  npTI->iorbh.RequestControl = Retry ? IORB_ASYNC_POST : IORB_ASYNC_POST | IORB_DISABLE_RETRY;
  npTI->RBA		     = RBA;
  npTI->BlockCount	     = 1;
  npTI->BlocksXferred	     = 0;
  npTI->BlockSize	     = 512;

  rc = Execute (0, (NPIORB)npTI);

  npU->Flags = UCBFlags;
  return (rc);
}

/*------------------------------------*/
/*				      */
/*				      */
/*				      */
/*------------------------------------*/
USHORT FAR ScanForOtherADDs()
{
  NPIORB_CONFIGURATION npIORB;
  DEVICETABLE	      *npDeviceTable;
  NPADAPTERINFO        npAdapterInfo;

  UCHAR  i, j, k;
  USHORT UnitFlags;
  USHORT UnitType;
  USHORT DriveCount = 0;

  DevHelp_GetDOSVar(DHGETDOSV_DEVICECLASSTABLE, 1, (PPVOID)&pDriverTable);
  npDeviceTable = (DEVICETABLE *)ScratchBuf;

  for (i = 1; i <= pDriverTable->DCCount; i++) {
    npIORB = (NPIORB_CONFIGURATION) &InitIORB;
    npIORB->iorbh.Length	  = sizeof (IORB_CONFIGURATION);
    npIORB->iorbh.CommandCode	  = IOCC_CONFIGURATION;
    npIORB->iorbh.CommandModifier = IOCM_GET_DEVICE_TABLE;
    npIORB->iorbh.RequestControl  = IORB_ASYNC_POST;
    npIORB->pDeviceTable	  = (PVOID)ScratchBuf;
    npIORB->DeviceTableLen	  = sizeof (ScratchBuf);

    clrmem (ScratchBuf, sizeof(ScratchBuf));

    if (Execute (i, (NPIORB)npIORB)) continue;

    for (j = 0; j < npDeviceTable->TotalAdapters; j++) {
      npAdapterInfo = npDeviceTable->pAdapter[j];

      for (k = 0; k < npAdapterInfo->AdapterUnits; k++) {
	UnitFlags = npAdapterInfo->UnitInfo[k].UnitFlags;
	UnitType  = npAdapterInfo->UnitInfo[k].UnitType;
	if (!(UnitFlags & (UF_NODASD_SUPT | UF_DEFECTIVE))) {
	  if ((UnitType == UIB_TYPE_DISK) && !(UnitFlags & UF_REMOVABLE))
	    DriveCount++;
	}
      }
    }
  }

  return (DriveCount);
}

VOID NEAR SetOption (NPU npU, NPA npA, USHORT Ofs, USHORT Value)
{
  if (npU) {
    // Apply to specific unit
    *(NPUSHORT)((NPCH)npU + Ofs) |= Value;
  } else if (npA) {
    // Apply to all units on specific adapter
    *(NPUSHORT)((NPCH)(npA->UnitCB + 0) + Ofs) |= Value;
    *(NPUSHORT)((NPCH)(npA->UnitCB + 1) + Ofs) |= Value;
  } else {
    // Apply to all units on all adapters
    for (npA = AdapterTable; npA < (AdapterTable + MAX_ADAPTERS); npA++) {
      *(NPUSHORT)((NPCH)(npA->UnitCB + 0) + Ofs) |= Value;
      *(NPUSHORT)((NPCH)(npA->UnitCB + 1) + Ofs) |= Value;
    }
  }
}

#define PCF_ADAPTER0	0x01
#define PCF_ADAPTER1	0x02
#define PCF_ADAPTER2	0x04
#define PCF_ADAPTER3	0x08
#define PCF_ADAPTER4	0x10
#define PCF_ADAPTER5	0x20
#define PCF_ADAPTER6	0x40
#define PCF_ADAPTER7	0x80

#define PCF_UNIT0	0x1
#define PCF_UNIT1	0x2

#define PCF_CLEAR_ADAPTER (PCF_UNIT0  | PCF_UNIT1)
#define CHKFLG(x) // if (Flags&x){rc=1;break;}else{Flags|=x;}

/**
 * Parse commaond line
 * @param pCmdLine points to nul terminated command line argument string
 * @return 0 if OK or error code
 * @note C4203 warning is expected
 */

USHORT FAR ParseCmdLine (PSZ pCmdLine)
{
  CC	cc;
  NPSZ	pOutBuf;
  UCHAR rc = 0;

  NPA	npA = NULL;
  NPU	npU = NULL;

  USHORT Value;
  UCHAR  UFlags = 0;
  USHORT AFlags = 0;
  USHORT Mask;
  UCHAR  Length;

  pOutBuf = poutbuf;
  cc = Command_Parser (pCmdLine, &opttable, pOutBuf, outbuf_len);

  if (cc.ret_code == NO_ERR) {
    while (!rc && pOutBuf[1] != (UCHAR) TOK_END) {

      Length = pOutBuf[0];
      Value = *(NPUSHORT)(pOutBuf+2);

      switch (pOutBuf[1]) {
	case TOK_DM:
	  break;

	/*-------------------------------------------*/
	/* Quite/Verbose Mode - /!V /V /VL /VLL      */
	/* Pause Verbose Mode - /!W /W /WL /WLL      */
	/*-------------------------------------------*/

	case TOK_V:	   Verbose++;
	case TOK_VL:	   Verbose++;
	case TOK_VLL:	   Verbose++;
			   break;

	case TOK_VPAUSE:   Verbose++;
	case TOK_VPAUSEL:  Verbose++;
	case TOK_VPAUSELL: Verbose++;
			   Verbose |= 0x80;
			   break;

	case TOK_NOT_VPAUSE:
	case TOK_NOT_V:    Verbose &= ~0x8F;
			   break;

	/*-------------------------------------------*/
	/* Enable/Force Generic /GBM /GBM!	     */
	/*-------------------------------------------*/

	case TOK_GBM:

	  GenericBM = 1;
	  break;

	case TOK_FORCEGBM:

	  if (npA) {
	    npA->FlagsT |= ATBF_BIOSDEFAULTS;
	  } else {
	    NPA pA;
	    for (pA = AdapterTable; pA < (AdapterTable + MAX_ADAPTERS); pA++)
	      pA->FlagsT |= ATBF_BIOSDEFAULTS;
	  }
	  break;

	/*-------------------------------------------*/
	/* Set Debug Level /DEBUG:nnnn		     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_DEBUG:

	  Debug = Value;
	  if (Debug & 0x8000) Debug = 0x8000 | (USHORT)(-Debug);
	  break;

	/*-------------------------------------------*/
	/* Set chipset fixes /FIXES:hhhh	     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_FIXES:

	  Fixes = Value;
	  break;

	/*-------------------------------------------*/
	/* Set PCI Latency  /LAT:nnnn		     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_LATENCY:

	  PCILatency = Value;
	  break;

	/*-------------------------------------------*/
	/* No Audible Alerts  - /!AA		     */
	/*-------------------------------------------*/

	case TOK_NOTBEEP:

	  Beeps = 0;
	  break;

	/*-------------------------------------------*/
	/* No device shutdown  - /!SHUTDOWN	     */
	/*-------------------------------------------*/

	case TOK_NOSHTDWN:
	  // no op
	  break;

	/*------------------------------------------*/
	/* No device shutdown  - /SHUTDOWN	    */
	/*------------------------------------------*/

	case TOK_SHTDWN:

	  NotShutdown = 0;
	  break;

	/*-------------------------------------------*/
	/* don't call BIOS INT13          /BIOS      */
	/*					     */
	/*-------------------------------------------*/

	case TOK_NOT_BIOS:
	  BIOSInt13 = 0;
	  break;

	/*-------------------------------------------*/
	/* Adapter Id - /A:0, /A:1, /A:2 or /A:3     */
	/*					     */
	/* Check for  correct adapter index. Address */
	/* ADAPTERTABLE structure.		     */
	/*-------------------------------------------*/

	case TOK_ADAPTER:

	  Value &= 0x0F;
	  Mask = PCF_ADAPTER0 << Value;

	  if ((Value >= MAX_ADAPTERS) || (AFlags & Mask)) { rc = 1; break; }
	  AFlags |= Mask;
	  UFlags &= ~PCF_CLEAR_ADAPTER;
	  npA = AdapterTable + Value;
	  npU = NULL;

	  break;

	/*-------------------------------------------*/
	/* Ignore Adapter - /I			     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_IGNORE:

	  if (npU) {
	    npU->FlagsT |= UTBF_DISABLED;
	    if ((npA->UnitCB[0].FlagsT & npA->UnitCB[1].FlagsT) & UTBF_DISABLED)
	      goto DisableAdapter;
	  } else {
	   DisableAdapter:
	    npA->FlagsT |= ATBF_DISABLED;
	  }
	  break;


	/*-------------------------------------------*/
	/* PCI location - /LOC:			     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_PCILOC:

	  npA->LocatePCI     = Value;
	  npA->LocateChannel = pOutBuf[4];
	  break;

	/*-------------------------------------------*/
	/* 80wire forced present	  /80WIRE    */
	/*					     */
	/*-------------------------------------------*/

	case TOK_80WIRE:

	  npA->FlagsT |= ATBF_80WIRE;
	  break;

	/*-------------------------------------------*/
	/* Adapter IRQ TimeOut -  /TO:nn (sec)	     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_IRQTIMEOUT:

	  npA->IRQTimeOutSet = Value;
	  break;

	/*-------------------------------------------*/
	/* DASD Manager Support - /!DM, /DM	     */
	/*					     */
	/* If this is specified before /U (Unit Id)  */
	/* then this switch applies to all units.    */
	/* Otherwise, this switch applies to the     */
	/* current unit.			     */
	/*-------------------------------------------*/

	case TOK_NOT_DM:
	  SetOption (npU, npA, offsetof (UCB, Flags) + 2, (USHORT)(UCBF_NODASDSUPPORT >> 16));
	  break;


	/*-------------------------------------------*/
	/* Base Port - /P:hhhh			     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_PORT:
	  if (npA) {
	    NPA pA;

	    for (pA = AdapterTable; pA < (AdapterTable + MAX_ADAPTERS); pA++)
	      if (pA->IOPorts[0] == Value) pA->IOPorts[0] = 0;

	    DATAREG = Value;

	    switch (Value) {
	      case PRIMARY_P:
		DEVCTLREG     = PRIMARY_C;
		npA->IRQLevel = PRIMARY_I;
		break;
	      case SECNDRY_P:
		DEVCTLREG     = SECNDRY_C;
		npA->IRQLevel = SECNDRY_I;
		break;
	      default:
		DEVCTLREG = DATAREG + 14; // default offset
	    }
	  } else {
	    BasePortpref = Value;
	  }
	  break;

	/*-------------------------------------------*/
	/* IRQ Level - /IRQ:nn			     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_IRQ:
	  if (npA)
	    npA->IRQLevel = Value;
	  else
	    IRQprefLevel = Value;
	  break;

	case TOK_PCMCIA:

	  MaxNumSockets = Value & 0x0F;
	  npA = NULL;
	  break;

	case TOK_BAY:

	  npA->FlagsT |= ATBF_BAY;
	  break;
	/*-------------------------------------------*/
	/* Unit Number - /U:0, /U:1		     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_UNIT:

	  Value &= 0x0F;
	  Mask = Value ? PCF_UNIT1 : PCF_UNIT0;
	  if ((Value >= MAX_UNITS) || (UFlags & Mask)) { rc = 1; break; }
	  UFlags |= Mask;
	  npU	 = &npA->UnitCB[Value];

	  break;


	/*-------------------------------------------*/
	/* Unit TimeOut -  /T:nn (sec)		     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_TIMEOUT:

	  npU->TimeOut = Value;
	  break;


	/*-------------------------------------------*/
	/* Unit Idle Time  -  /IT:nnnn (min)	     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_IDLETIMER:

	  SetOption (npU, npA, offsetof (UCB, IdleTime), (UCHAR)(~Value));
	  break;

	/*-------------------------------------------*/
	/* Limit Data Rate -  /MR:0hhh		     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_MAXRATE:

	  npU->MaxRate = ~Value;
	  break;

	/*-------------------------------------------*/
	/* Set Noise Level -  /NL:d		     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_NOISE:

	  npU->AMLevel = ~Value;
	  break;

	/*-------------------------------------------*/
	/* Set APM Level -  /APM:d		     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_APM:

	  SetOption (npU, npA, offsetof (UCB, APMLevel), Value);
	  break;

	/*-------------------------------------------*/
	/* Set Link Power Management Level -  /LPM:d */
	/*					     */
	/*-------------------------------------------*/

	case TOK_LPM:

	  SetOption (npU, npA, offsetof (UCB, LPMLevel), (UCHAR)(~Value));
	  break;

	/*-------------------------------------------*/
	/* Protect MBR/disk	-  /WP:0/1	     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_WP:

	  npU->WriteProtect = (Value & 0x7F) + 1;
	  break;

	/*-------------------------------------------*/
	/* Set PCI Clock Rate /PCLK:nn		     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_PCICLOCK:

	  PCIClock = Value;
	  break;

	/*-------------------------------------------*/
	/* not/REMOVABLE - RMV, !RMV		     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_REMOVABLE:
	case TOK_NOTREMOVABLE:
	  break;

	/*-------------------------------------------*/
	/* Disable HPA unlock - /!SETMAX	     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_NOTSETMAX:

	  SetOption (npU, npA, offsetof (UCB, FlagsT), UTBF_NOTUNLOCKHPA);
	  break;

	/*-------------------------------------------*/
	/* Enable Set Multiple Support		     */
	/*					     */
	/*-------------------------------------------*/

	case TOK_SMS:

	  npU->SMSCount = ~Value;
	  break;

	case TOK_NOT_SMS:

	  npU->SMSCount = ~0;
	  break;

	case TOK_PF:
	  SetOption (npU, npA, offsetof (UCB, Features), Value);
	  break;

	/*-------------------------------------------*/
	/*  Bus Master DMA Enable/Disable	     */
	/*					     */
	/* If this switch occurs after the /UNIT:x   */
	/* switch then the disable action applies    */
	/* only towards that specific unit.	     */
	/*					     */
	/* If it appears after the /A:x switch then  */
	/* it applies to all units of the adapter.   */
	/* Any other use of these switches is not    */
	/* valid.				     */
	/*-------------------------------------------*/

	case TOK_BM_DMA:
	  SetOption (npU, npA, offsetof (UCB, FlagsT), UTBF_SET_BM);
	  break;

	case TOK_NOT_BM_DMA:
	  SetOption (npU, npA, offsetof (UCB, FlagsT), UTBF_NOT_BM);
	  break;


	/*-------------------------------------------*/
	/* Unknown Switch			     */
	/*					     */
	/*-------------------------------------------*/

	default:

	  rc = 1;
	  break;
      }

      if (!rc) pOutBuf += Length;
    }
  } else if (cc.ret_code != NO_OPTIONS_FND_ERR) {
    rc = 1;
  }
  return (rc);
}

ULONG FAR PortToPhys (ULONG Port, NPA npA)
{
  NPBAR npB;

  for (npB = npA->npC->BAR + 0; npB <= npA->npC->BAR + 5; npB++) {
    if ((Port >= npB->Addr) && (Port < (npB->Addr + npB->Size)))
      return (Port - npB->Addr + npB->phyA);
  }
  return (Port);
}

VOID NEAR PrintAdapterInfo (NPA npA) {
  UCHAR Msg = npA->Status;
  ULONG Port;
  UCHAR IRQ;

  Port = PortToPhys (DATAREG, npA);
  IRQ  = APICRewire ? npA->npC->IrqAPIC : 0;
  if (!IRQ) IRQ = npA->IRQLevel;

  if (npA->FlagsT & ATBF_PCMCIA) {
    if (npA->Socket != (UCHAR)-1) {
      Msg  = ATS_PCCARD_INSERTED;
    } else {
      Port = BasePortpref;
      IRQ  = IRQprefLevel;
      Msg  = ATS_PCCARD_NOT_INS;
    }
    Msg = (npA->Socket != (UCHAR)-1) ? ATS_PCCARD_INSERTED : ATS_PCCARD_NOT_INS;
  } else if (npA->FlagsT & ATBF_BAY) {
    if (npA->Status <= ATS_NOT_PRESENT) Msg = ATS_BAY;
  } else {
    if ((MEMBER(npA).Vendor) && (npA->Status == ATS_NOT_PRESENT))
      Msg = ATS_NO_DEVICES_FOUND;
  }

  sprntf (TraceBuffer, VControllerInfo, npA - AdapterTable, Port, IRQ, AdptMsgs[Msg],
	  ((npA->FlagsT & ATBF_BM_DMA) && !(npA->Status)) ? MsgBMOn : MsgNull);
  TTYWrite (2, TraceBuffer);

  if (MEMBER(npA).Vendor) {
    sprntf (TraceBuffer, VPCIInfo, npA->npPCIDeviceMsg,
	    npA->Cap & CHIPCAP_SATA ? 'S' : 'P',
	    MEMBER(npA).Vendor, MEMBER(npA).Device, MEMBER(npA).Revision,
	    (UCHAR)(npA->PCIInfo.PCIAddr >> 8),
	    (UCHAR)((npA->PCIInfo.PCIAddr >> 3) & 0x1F),
	    (UCHAR)(npA->PCIInfo.PCIAddr & 7),
	    npA->IDEChannel);
    TTYWrite (3, TraceBuffer);
  } else if (Msg == ATS_PCCARD_INSERTED) {
    sprntf (TraceBuffer, VPCardInfo, npA->npPCIDeviceMsg);
    TTYWrite (3, TraceBuffer);
  }
}

/*------------------------------------*/
/*				      */
/* PrintInfo()			      */
/*				      */
/*------------------------------------*/
extern USHORT _cdecl FAR CalculateLoss (ULONG PTotal, ULONG LTotal);

VOID FAR PrintInfo (NPA npA)
{
  NPU	npU;
  NPCH	s = TraceBuffer;
  NPCH	s1 = s + 128;
  NPCH	s2 = s1 + 32;
  NPCH	s3 = s2 + 32;
  NPCH	s4 = s3 + 32;
  NPCH	t1, t2;
  UCHAR v1, v2;

  TTYWrite (1, VersionMsg);

  for (npA = AdapterTable; npA < (AdapterTable + MAX_ADAPTERS); npA++) {
    if (npA->FlagsT & ATBF_ON) {
      PrintAdapterInfo (npA);

      // Pause the output between adapters so verbose info does
      // not roll off the screen.

      if ((npA != AdapterTable) && (Verbose & 0x80) && (LinesPrinted > 10)) {
	TTYWrite (0, VPauseVerbose0);
	LinesPrinted = 0;
      }

      for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->cUnits); npU++) {
	if (npU->Status == UTS_NO_MASTER) {
	  sprntf (s, VUnitInfo1, npU->UnitIndex, UnitMsgs[npU->Status], MsgNull, MsgNull, MsgNull, MsgNull);

	  TTYWrite (3, s);

	} else {
	  if (npU->Flags & UCBF_NOTPRESENT) continue;

	  /*****************************/
	  /* Process Strings s2 and s3 */
	  /*****************************/

	  ProcessVerboseTimings (npU, npA, s2, s3);

	  if (npU->Flags & UCBF_ATAPIDEVICE) {
	    sprntf (s, VUnitInfo1, npU->UnitIndex, UnitMsgs[npU->Status],
		    (npU->Flags & UCBF_PCMCIA ? MsgNull : ATAPIMsg),
		    MsgNull,
		    s2, s3);

	    TTYWrite (2, s);

	    if (npU->ModelNum[0]) {
	      sprntf (s, VModelInfo, npU->ModelNum);
	      TTYWrite (1, s);
	    }
	  } else {
	    USHORT percentUsed[2];

	    t1 = MsgNull;

	    if (npU->Flags & UCBF_ONTRACK) {		     /* Disk Manager */
	      t1 = MsgONTrackOn;
	    } else if (npU->Flags & UCBF_EZDRIVEFBP) {	  /* EZDrive */
	      t1 = MsgEzDriveFBP;
	    }

	    if (npU->FlagsT & UTBF_BPBGEOMETRYVALID) {
	      t2 = VBPBInfo;
	      v1 = npU->BPBLogGeom.NumHeads;
	      v2 = npU->BPBLogGeom.SectorsPerTrack;

	    } else if (npU->FlagsT & UTBF_I13GEOMETRYVALID) {
	      t2 = VI13Info;
	      v1 = npU->I13LogGeom.NumHeads;
	      v2 = npU->I13LogGeom.SectorsPerTrack;

	    } else if (npU->FlagsT & UTBF_IDEGEOMETRYVALID) {
	      t2 = VIDEInfo;
	      v1 = v2 = 0;
	    }
	    sprntf (s1, npU->Flags & UCBF_SMSENABLED ? MsgSMS : MsgNull,
		    npU->SecPerBlk);

	    sprntf (s4, npU->CmdSupported & UCBS_ACOUSTIC ? MsgAML : MsgNull,
		    (npU->AMLevel ? ~npU->AMLevel : npU->FoundAMLevel) & 0x7F);

	    sprntf (s, VUnitInfo2, npU->UnitIndex, UnitMsgs[npU->Status], s1,
		    MsgLBAOn, s4, t1, s2, s3, t2);
	    TTYWrite (2, s);

	    if (npU->ModelNum[0]) {
	      sprntf (s, VModelInfo, npU->ModelNum);
	      TTYWrite (1, s);
	    }

	    TTYWrite (3, VGeomInfo0);

	    sprntf (s, VGeomInfo1,
		    npU->LogGeom.TotalCylinders,
		    npU->PhysGeom.TotalCylinders,
		    npU->IDELogGeom.TotalCylinders,
		    npU->IDEPhysGeom.TotalCylinders,
		    npU->IDEPhysGeom.TotalSectors);
	    TTYWrite (3, s);

	    sprntf (s, VGeomInfo2,
		    npU->LogGeom.NumHeads,
		    npU->PhysGeom.NumHeads,
		    v1,
		    npU->IDELogGeom.NumHeads,
		    npU->IDEPhysGeom.NumHeads,
		    npU->LogGeom.TotalSectors);
	    TTYWrite (3, s);

	    // Compute % of drive available to the OS.
	    if (npU->IDEPhysGeom.TotalSectors > 0) {
	      USHORT Used;

	      Used = CalculateLoss (npU->IDEPhysGeom.TotalSectors, npU->LogGeom.TotalSectors);
	      percentUsed[0] = Used / 100;
	      percentUsed[1] = Used - 100 * percentUsed[0];
	    }

	    sprntf (s, VGeomInfo3,
		    npU->LogGeom.SectorsPerTrack,
		    npU->PhysGeom.SectorsPerTrack,
		    v2,
		    npU->IDELogGeom.SectorsPerTrack,
		    npU->IDEPhysGeom.SectorsPerTrack,
		    percentUsed[0],
		    percentUsed[1]);

	    TTYWrite (3, s);
	  }
	}
      }
    }
  }

  // Pause the output at the end of verbose messages so info does
  // not roll off the screen.
  if (Verbose & 0x80) TTYWrite (0, VPauseVerbose1);

  TTYWrite (3, "\r\n");
}

#if TRACES
VOID NEAR TraceCh (CHAR c)
{
  if (InitActive) *(npTrace++) = c;
}

VOID FAR TraceStr (NPSZ fmt, ...)
{
  if (InitActive) {
    prntf (npTrace, fmt, (PUCHAR)&fmt + sizeof(fmt));
    while (*npTrace) npTrace++;
  }
}

VOID NEAR TraceTime (VOID)
{
  if (InitActive) {
    USHORT t = DiffTime();
    if (t > 0) {
      sprntf (npTrace, "%d", t);
      while (*npTrace) npTrace++;
    }
  }
}

VOID TraceTTime (VOID)
{
  if (InitActive) {
    USHORT t = TotalTime();
    if (t > 0) {
      sprntf (npTrace, "Time: %d.%ds", t/10, t%10);
      while (*npTrace) npTrace++;
    }
  }
}

/*-------------------------------*/
/*				 */
/* DiffTime()			 */
/*				 */
/*-------------------------------*/
USHORT NEAR DiffTime  (VOID)
{
  static USHORT LastTime;
  USHORT CurrentTime, Delta;

  CurrentTime = *msTimer;
  Delta = CurrentTime - LastTime;
  LastTime = CurrentTime;
  return ((Delta + 500) / 1000);
}

USHORT TotalTime (VOID)
{
  static USHORT LastTime;
  USHORT CurrentTime, Delta;

  CurrentTime = *msTimer;
  Delta = CurrentTime - LastTime;
  LastTime = CurrentTime;
  return ((Delta + 50) / 100);
}

VOID NEAR TWrite (USHORT DbgLevel)
{
  *npTrace = '\0';
  if (Debug & DbgLevel)
    if (Verbose)
      TTYWrite (99, TraceBuffer);
    else
      SaveMsg (TraceBuffer);
  npTrace = TraceBuffer;
}

#else
USHORT NEAR DiffTime (VOID) {}
VOID TraceTTime (VOID) {}
#endif

/*-------------------------------*/
/*				 */
/* SaveMsg ()			 */
/*				 */
/*-------------------------------*/
VOID FAR SaveMsg  (PSZ Buf)
{
  if (InitActive) {
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
}

/*-------------------------------*/
/*				 */
/* TTYWrite()			 */
/*				 */
/*-------------------------------*/
VOID FAR TTYWrite (UCHAR Level, NPSZ Buf)
{
  if (InitActive) {
    if ((Verbose & 0xF) >= Level) {
      InitMsg.MsgStrings[0] = Buf;
      DevHelp_Save_Message ((NPBYTE)&InitMsg);
      LinesPrinted++;
    }
    SaveMsg (Buf);
  }
}

/*-------------------------------*/
/*				 */
/* ProcessVerboseTimings()	 */
/*				 */
/*-------------------------------*/
VOID NEAR ProcessVerboseTimings (NPU npU, NPA npA, NPCH s2, NPCH s3)
{
  *s2 = 0;					/* start with empty strings */

  if (SSTATUS) {
    UCHAR Sspeed = ((UCHAR)InD (SSTATUS) & SSTAT_SPD) >> 4;
    switch (Sspeed) {
      default	      : sprntf (s3, VSata0); break;
      case SSTAT_SPD_1: sprntf (s3, VSata1); break;
      case SSTAT_SPD_2: sprntf (s3, VSata2); break;
    }
    if (npU->Flags & UCBF_BM_DMA)
      sprntf (s2, VString, MsgTypeMDMAOn);
  } else if (npU->Flags & UCBF_BM_DMA) {
    sprntf (s2, VString, MsgTypeMDMAOn);
    if (npU->CurDMAMode > 2)		     /* show UltraDMA and PIO modes */
      sprntf (s3, VULTRADMAString, npU->CurDMAMode - 3, npU->CurPIOMode);
    else					  /* show DMA and PIO modes */
      sprntf (s3, VDMAString, npU->CurDMAMode, npU->CurPIOMode);
  } else {					      /* show only PIO mode */
    sprntf (s3, VPIOString, npU->CurPIOMode);
  }
}

NPHRESOURCE NEAR AllocResource (NPHRESOURCE nphRes, ULONG Addr, USHORT Len, UCHAR shared)
{
  RESOURCESTRUCT Resource;

  if (Addr & 0xFFFF0000) {  // memory resource
    Resource.ResourceType	       = RS_TYPE_MEM;
    Resource.MEMResource.MemBase       = Addr;
    Resource.MEMResource.MemSize       = Len;
    Resource.MEMResource.MemFlags      = shared ? RS_MEM_SHARED : RS_MEM_EXCLUSIVE;
  } else { // IO resource
    if ((USHORT)Addr < 0x100) return (nphRes);

    Resource.ResourceType	       = RS_TYPE_IO;
    Resource.IOResource.BaseIOPort     = Addr;
    Resource.IOResource.NumIOPorts     = Len;
    Resource.IOResource.IOFlags        = shared ? RS_IO_SHARED : RS_IO_EXCLUSIVE;
    Resource.IOResource.IOAddressLines = 16;
  }

  if (!RMAllocResource (hDriver, nphRes, &Resource)) nphRes++;
  return (nphRes);
}

/**
 * Allocate adapter resources
 * @param npA points to adapter descriptor
 * @return 0 if allocate OK else error code
 */

USHORT FAR AllocAdapterResources (NPA npA)
{
  RESOURCESTRUCT Resource;
  NPHRESOURCE	 nphRes;
  UCHAR		 IRQ;

# define npResourceList ((NPAHRESOURCE)(npA->ResourceBuf))

  if (npA->FlagsT & ATBF_PCMCIA) return 0;		// Nothing to allocate

  clrmem (npResourceList, sizeof (npA->ResourceBuf));
  nphRes = &(npResourceList->hResource[0]);

  /*-----------------------*/
  /* AllocIRQResource	   */
  /*-----------------------*/

  IRQ  = APICRewire ? npA->npC->IrqAPIC : 0;
  if (!IRQ) IRQ = npA->IRQLevel;

  Resource.ResourceType		 = RS_TYPE_IRQ;
  Resource.IRQResource.IRQLevel  = IRQ;
  Resource.IRQResource.PCIIrqPin = npA->FlagsT & ATBF_INTSHARED ?
				     RS_PCI_INT_A : RS_PCI_INT_NONE;
  Resource.IRQResource.IRQFlags  = npA->FlagsT & ATBF_INTSHARED ?
				     RS_IRQ_SHARED : RS_IRQ_EXCLUSIVE;

  if (!RMAllocResource (hDriver, nphRes, &Resource)) nphRes++;

  nphRes = AllocResource (nphRes, npA->npC->BAR[5].phyA, npA->npC->BAR[5].Size, TRUE);
  nphRes = AllocResource (nphRes, PortToPhys (DATAREG, npA), 8, FALSE);
  nphRes = AllocResource (nphRes, PortToPhys (DEVCTLREG, npA), 1, FALSE);
  nphRes = AllocResource (nphRes, PortToPhys (npA->BMICOM, npA), npA->BMSize, FALSE);
  nphRes = AllocResource (nphRes, PortToPhys (npA->ExtraPort, npA), npA->ExtraSize, TRUE);

  npResourceList->NumResource = nphRes - &(npResourceList->hResource[0]);

  /*************************************************/
  /* Check at least two resources registered       */
  /* (one for the IRQ and at least one I/O port)   */
  /* otherwise return error code                   */
  /*************************************************/
  return (npResourceList->NumResource >= 2 ? 0 : 1);
}

/*-------------------------------*/
/*				 */
/* DeallocAdapterResources()	 */
/*				 */
/*-------------------------------*/
VOID FAR DeallocAdapterResources (NPA npA)
{
  UCHAR i;
  NPHRESOURCE nphRes;

  nphRes = &(npResourceList->hResource[0]);
  for (i = 6; i > 0; i--) {
    if (*nphRes) {
      RMDeallocResource (hDriver, *nphRes);
      *(nphRes++) = 0;
    }
  }
}

#undef npResourceList

VOID FAR RegisterADD (void)
{
  if (!ADDHandle)
    DevHelp_RegisterDeviceClassLite (AdapterNameATAPI, (NPFN)&ADDEntryPoint, (NPUSHORT)&ADDHandle);
}

/*-------------------------------*/
/*				 */
/* AssignAdapterResources()	 */
/*				 */
/*-------------------------------*/
VOID FAR AssignAdapterResources (NPA npA)
{
  NPU	   npU;
  UCHAR    i;
  ADJUNCT  Adjunct;
  HDEVICE  hDevice;
  HADAPTER hAdapter;

  RegisterADD();

  Adjunct.pNextAdj  = 0;
  Adjunct.AdjLength = sizeof(ADJUNCT);
  Adjunct.AdjType   = ADJ_ADD_UNIT;
  Adjunct.Add_Unit.ADDHandle = ADDHandle;

  /*---------------------------------------------------*/
  /* increment controller number in adapter string:    */
  /* Original String is "IDE_0 PATA/SATA Controller"   */
  /*---------------------------------------------------*/

  AdapterStruct.AdaptDescriptName[4] = '0' + (npA - AdapterTable);

  /*
  ** There is adapter object information available to
  ** identify individual PCI adapters.	Only a global is
  ** is available to disern other bus types.  Default host
  ** host bus type is AS_HOSTBUS_ISA.
  */
  AdapterStruct.HostBusType = AS_HOSTBUS_ISA;
  AdapterStruct.HostBusWidth= AS_BUSWIDTH_16BIT;
  if (MEMBER(npA).Vendor) {
    AdapterStruct.HostBusType = AS_HOSTBUS_PCI;
    AdapterStruct.HostBusWidth= AS_BUSWIDTH_32BIT;
  } else if (npA->FlagsT & ATBF_PCMCIA)
    AdapterStruct.HostBusType = AS_HOSTBUS_PCMCIA;

  RMCreateAdapter (hDriver, &hAdapter, &AdapterStruct, NULL, (PAHRESOURCE)(npA->ResourceBuf));

  for (npU = npA->UnitCB, i = 0; i < npA->cUnits; npU++, i++) {
    Adjunct.Add_Unit.UnitHandle = (USHORT)npU;
    DevStruct.pAdjunctList = &Adjunct;

    if (npU->Status == UTS_NO_MASTER) continue;

    if (!(npU->Flags & UCBF_ATAPIDEVICE)) {
      /*-----------------------------------------*/
      /* Copy Device Description to local buffer */
      /*-----------------------------------------*/

      memcpy (ScratchBuf, DevDescriptNameTxt, sizeof (DevDescriptNameTxt));

      /*---------------------------------------------------*/
      /* increment device number in buffer string:	   */
      /* Original String is "HD_0 Hard Drive"              */
      /*---------------------------------------------------*/

      ScratchBuf[3] += i;

      /*--------------------------------------------------*/
      /* if a model number is exists, copy the portion of */
      /* string that will fit in the local buffer	  */
      /*--------------------------------------------------*/

      if (npU->ModelNum[0])
	_strncpy (ScratchBuf + 5, npU->ModelNum, 42 - 5);

      DevStruct.DevDescriptName = ScratchBuf;
      DevStruct.DevFlags = (npU->Flags & UCBF_REMOVABLE) ? DS_REMOVEABLE_MEDIA : DS_FIXED_LOGICALNAME;

      RMCreateDevice (hDriver, &hDevice, &DevStruct, hAdapter, NULL);
    }
  }
}


/*-------------------------------*/
/*				 */
/* FindHotPCCard()		 */
/*				 */
/*-------------------------------*/
VOID FAR FindHotPCCard (void)
{
  #define EXCA_PORT 0x3E0   // default PCCard bridge port
  UCHAR Slot, Reg;
  NPA	npA;
  struct {
    UCHAR  Id, Status, r2, IntCtl, r4, r5, WinEnable, r7;
    USHORT IOWin0S, IOWin0E, IOWin1S, IOWin1E;
  } ExCA;
  PUCHAR p;

  for (Slot = 0; Slot < (2 * 0x40); Slot += 0x40) {
    p = (PUCHAR)&ExCA;
    DISABLE;
    for (Reg = Slot; Reg < (Slot + sizeof (ExCA)); Reg++) {
      outp (EXCA_PORT, Reg);
      *(p++) = inp (EXCA_PORT + 1);
    }
    ENABLE

    if (ExCA.Id == 0xFF) continue;		// Slot not present
    if (!(ExCA.Status & 0x4C)) continue;	// Slot powered off or no card detected
    if ((ExCA.IntCtl & 0x2F) < 0x23) continue;	// card not IO card or bad IRQ value
    ExCA.IntCtl &= 0x0F;			// IRQ Level
    ExCA.IOWin0E -= ExCA.IOWin0S - 1;		// size of first IO window
    ExCA.WinEnable &= 0xC0;
    if (0xC0 == ExCA.WinEnable) {		// 2 windows
      if (ExCA.IOWin0E != 8) continue;		// size != 8
    } else if (0x40 == ExCA.WinEnable) {	// only first window
      if (ExCA.IOWin0E < 15) continue;		// size too small
      ExCA.IOWin1S = ExCA.IOWin0S + 6;
    } else continue;

    npA = LocateATEntry (ExCA.IOWin0S, 0, 0);
    if (npA) {
      clrmem (npA, sizeof (*npA));

      DATAREG	    = ExCA.IOWin0S;
      DEVCTLREG     = ExCA.IOWin1S;
      npA->IRQLevel = ExCA.IntCtl;
      npA->Socket   = 1 + Slot / 0x40;
      npA->FlagsT  |=  ATBF_BIOSDEFAULTS | ATBF_ON;
    }
  }
  return;
}


/*-------------------------------*/
/*				 */
/* LocateATEntry()		 */
/*				 */
/*-------------------------------*/
NPA FAR LocateATEntry (USHORT BasePort, USHORT PCIAddr, UCHAR Channel)
{
  NPA npA, npAfree = NULL;

//if (Debug & 8) TraceStr ("[%04X,%X#%d-", BasePort, PCIAddr, Channel);

  for (npA = AdapterTable; npA < (AdapterTable + MAX_ADAPTERS); npA++) {
    if (npA->FlagsT & (ATBF_ON | ATBF_PCMCIA | ATBF_POPULATED)) continue;
//if (Debug & 8) TraceStr ("<%04X>(%04X,%X#%d)", npA,(USHORT)DATAREG, npA->LocatePCI, npA->LocateChannel);
    if (!DATAREG && !npA->LocatePCI && !npAfree) npAfree = npA;
    // if the ports match or slot is empty
    if ((DATAREG == BasePort) ||
	(PCIAddr && ((PCIAddr == npA->LocatePCI) && (Channel == npA->LocateChannel)))) {
      npAfree = npA;
      break;
    }
  }

  if (npAfree) {
    npAfree->IOPorts[0] = BasePort;
  }

//if (Debug & 8) TraceStr("=%04X]", npAfree);
  return (npAfree);
}

/*-------------------------------*/
/*				 */
/* UCBSetupDMAPIO()		 */
/*				 */
/* For ATA or ATAPI devices	 */
/*				 */
/*-------------------------------*/
VOID NEAR UCBSetupDMAPIO (NPU npU, NPIDENTIFYDATA npID)
{
  NPA	 npA = npU->npA;
  USHORT Delay;

TS("D<F:%X", npU->Features)

  Delay = npU->Features & 3;
  if (npA->Cap & CHIPCAP_ATA33) Delay = 3;
  if (Delay == 3)
    Delay = 0;
  else
    Delay = IODelayCount >> Delay;

  if ((npA->UnitCB[0].Flags & UCBF_NOTPRESENT) || (npU->UnitIndex == 0))
    npA->IODelayCount = Delay;
  else
    if (npA->IODelayCount < Delay) npA->IODelayCount = Delay;

  TSTR ("S:%d,C:%X,F:%X;",npU->ATAVersion,npID->IDECapabilities,npA->FlagsT);

  if ((npID->IDECapabilities & (FX_IORDYSUPPORTED | FX_DISIORDYSUPPORTED)) == (FX_IORDYSUPPORTED | FX_DISIORDYSUPPORTED))
    npU->Flags |= UCBF_DISABLEIORDY;

  if (npID->IDECapabilities & FX_DMASUPPORTED) {
T('a')
    npU->Flags |= UCBF_BM_DMA;

    if (npID->AdditionalWordsValid & FX_WORD88_VALID)
      GetDeviceULTRAMode (npU, npID);

    GetDeviceDMAMode (npU, npID);
  }

  /*********************************/
  /*	 Get Device PIO Mode	   */
  /*********************************/

  if (npID->AdditionalWordsValid & FX_WORDS64_70VALID)
    GetDevicePIOMode (npU, npID);

  TSTR ("U%d/D%d/P%d,BP%d,F:%lX>", npU->UltraDMAMode,
	 npU->CurDMAMode,npU->CurPIOMode,npU->BestPIOMode,npU->Flags);
}

UCHAR CountBits (USHORT x) {
  UCHAR i;
  for (i = 0; x; x >>= 1)
    i++;
  return (i);
}

/*-------------------------------*/
/*				 */
/* GetDeviceULTRAMode()		 */
/*				 */
/* For ATA or ATAPI devices	 */
/*				 */
/*-------------------------------*/
VOID GetDeviceULTRAMode (NPU npU, NPIDENTIFYDATA npID)
{
  NPA	 npA = npU->npA;
  USHORT MaxUDMA;
  UCHAR  Cable80Pin = FALSE;

  if (((npID->HardwareTestResult != 0) &&
       (npID->HardwareTestResult != 0xFFFF) &&
       (npID->HardwareTestResult & 0x2000))
    || (npA->Cap & CHIPCAP_SATA))
    Cable80Pin = TRUE;

  npU->FoundUDMAMode = CountBits (npID->UltraDMAModes >> 8);

  MaxUDMA = (~npU->MaxRate & 0x0F00) >> 8;

  if ((npA->Cap & CHIPCAP_SATA) &&
      ((npID->SATACapabilities == 0) ||
       (npID->SATACapabilities == 0xFFFF) ||
      !(npID->SATACapabilities & 0xF) ||
       (npID->HardwareTestResult != 0))) {
    if (MaxUDMA > (5+1)) MaxUDMA = 5+1;
    npU->SATAGeneration = 0;
  } else {
    npU->SATAGeneration = (npID->SATACapabilities & 0xF) >> 1;
  }

  TSTR ("M%d,C%d,I:%X,S%x,", MaxUDMA, Cable80Pin, npID->UltraDMAModes, npU->SATAGeneration);

  if (npA->FlagsT & ATBF_BIOSDEFAULTS) {
    npU->UltraDMAMode = npU->FoundUDMAMode;
  } else {
    if ((MaxUDMA > 3) && !(Cable80Pin || (npA->FlagsT & ATBF_80WIRE)))
      MaxUDMA = 3;
    MaxUDMA = (1 << MaxUDMA) - 1;
    npU->UltraDMAMode = CountBits (npID->UltraDMAModes & ULTRADMAMASK & MaxUDMA);
  }

  TSTR ("U%d/F%d;", npU->UltraDMAMode, npU->FoundUDMAMode);
}

/*-------------------------------*/
/*				 */
/* GetDeviceDMAMode()		 */
/*				 */
/* For ATA or ATAPI devices	 */
/*				 */
/*-------------------------------*/
VOID GetDeviceDMAMode (NPU npU, NPIDENTIFYDATA npID)
{
  USHORT dmaMWMode;
  USHORT MaxDMA;

  MaxDMA = (~npU->MaxRate & 0x0070) >> 4;

 TSTR ("M%d,I:%X,", MaxDMA, npID->DMAMWordFlags);

  MaxDMA = (1 << MaxDMA) - 1;

  if (!(npID->AdditionalWordsValid & FX_WORDS64_70VALID) &&
       (npID->DMAMWordFlags & 0xF8F8))
    npID->DMAMWordFlags = 0;

  npU->FoundDMAMode = npID->DMAMWordFlags >> 8;

  if (npU->npA->FlagsT & ATBF_BIOSDEFAULTS)
    dmaMWMode = npU->FoundDMAMode;
  else {
    if (npID->MinMWDMACycleTime > 0) {
      if (npID->MinMWDMACycleTime <= 185)
	npID->DMAMWordFlags |= 2;
      if (npID->MinMWDMACycleTime <= 125)
	npID->DMAMWordFlags |= 4;
    }
    /************************************************************/
    /* If older WDC drive then use word 52 to decide DMA timing */
    /************************************************************/
    if (!npID->MinMWDMACycleTime && (npID->DMAMode <= 0x2FF))
      npID->DMAMWordFlags |= (2 << (npID->DMAMode >> 8)) - 1;
    dmaMWMode = npID->DMAMWordFlags & MaxDMA;
  }

  npU->CurDMAMode   = CountBits ((dmaMWMode & 7) >> 1);
  npU->FoundDMAMode = CountBits ((npU->FoundDMAMode & 7) >> 1);

 TSTR ("D%d/F%d;", npU->CurDMAMode, npU->FoundDMAMode);
}

/*-------------------------------*/
/*				 */
/* GetDevicePIOMode()		 */
/*				 */
/* For ATA or ATAPI devices	 */
/*				 */
/*-------------------------------*/
USHORT GetDevicePIOMode (NPU npU, NPIDENTIFYDATA npID)
{
  UCHAR PIO;

  npU->BestPIOMode = 0;

  // PIO mode is only in the lower 2 bits, so ignore higher bits.
  PIO = npID->AdvancedPIOModes & 3;
  if (PIO != 0) {
    /*************************************************/
    /* Find highest PIO mode the unit can operate in */
    /*************************************************/

    npU->BestPIOMode = 2 + CountBits (PIO);
  }

  PIO = ~npU->MaxRate & 0x0007;
  if (PIO < 3) PIO = 0;

  npU->CurPIOMode = npU->BestPIOMode;
  if (npU->CurPIOMode > PIO) npU->CurPIOMode = PIO;

  if (npU->FoundDMAMode && (MEMBER(npU->npA).TModes & TR_PIO_EQ_DMA))
    npU->FoundPIOMode = npU->FoundDMAMode + 2;

  TSTR ("M%d,P%d/B%d/F%d;", PIO, npU->CurPIOMode, npU->BestPIOMode, npU->FoundPIOMode);

  return (npU->BestPIOMode);
}

USHORT NEAR GetChipPIOMode (NPA npA) {
  NPU	 npU;
  UCHAR  Unit, PIO;
  USHORT Clocks = 0, ns;

  npU = npA->UnitCB;
  for (Unit = 0; Unit < MAX_UNITS; Unit++, npU++) {
    if ((npU->Flags & UCBF_NOTPRESENT) || (npU->FoundPIOMode)) continue;

    Clocks = METHOD(npA).GetPIOMode (npA, Unit);
    PIO = 0;
    if (Clocks) {
      ns = (1000 / PCIClock) * Clocks;
      if (ns <= 150) PIO = 4;
      else if (ns <= 240) PIO = 3;
    }
    npU->FoundPIOMode = PIO;

  TSTR ("FP%d/%d;", Clocks, npU->FoundPIOMode);
  }
}

/*-----------------------------------*/
/* IdentifyDevice()		     */
/*				     */
/* For ATA or ATAPI devices	     */
/*-----------------------------------*/

VOID NEAR IdentifyDevice (NPU npU, NPIDENTIFYDATA npID)
{
  NPA	npA    = npU->npA;
  UCHAR UnitId = npU->UnitIndex;

#define npTI (&npA->PTIORB)
#define npicp (&npA->icp)

  TS("I(U%d", UnitId)

  Retry:

  ClearIORB (npA);

  npicp->TaskFileIn.Command  = (npU->Flags & UCBF_ATAPIDEVICE) ? FX_ATAPI_IDENTIFY : FX_IDENTIFY;
  npicp->RegisterMapW = RTM_COMMAND | RTM_FEATURES;
  npicp->RegisterMapR = RTM_STATUS  | RTM_ERROR;

  ScratchSGList.ppXferBuf  = ppDataSeg + (USHORT)npID;
  ScratchSGList.XferBufLen = 512;

  npU->ReqFlags = 0;
  npTI->cSGList = 1;
  npTI->pSGList = &ScratchSGList;

  IssueCommand (npU);

#if TRACES
  if (Debug & 8) {
    TraceStr ("s%X/%X", npicp->TaskFileOut.Status,npTI->iorbh.ErrorCode);
  }
#endif

  T(')')

  if (npTI->iorbh.Status & IORB_ERROR) return;	     /* ERROR */

  if (npID->GeneralConfig.Bits.Incomplete) {
    if ((npID->SpecificConfig == 0x37C8) || (npID->SpecificConfig == 0x738C))
      IssueSetFeatures (npU, FX_DEVICE_SPIN_UP, 0);
    if (++npA->Retries < 2) goto Retry;
  }

  npA->Retries = 0;

  if (npU->CmdSupported) return; // don't gather feature bits twice!!

  if ((npID->CommandSetSupported[1] & 0xC000) == 0x4000)
    npU->CmdSupported = *(ULONG *)(npID->CommandSetSupported);
  npU->CmdEnabled = *(ULONG *)(npID->CommandSetEnabled) & npU->CmdSupported;
  if ((npID->IDECapabilities & FX_POWERSUPPORTED) ||
      (npU->CmdEnabled & ((ULONG)FX_POWERSUPPORTED3 << 16)))
    npU->CmdEnabled |= FX_POWERSUPPORTED2;

  if ((npID->SATAFeatSupported != 0) && (~npID->SATAFeatSupported != 0)) {
    npU->SATACmdSupported = npID->SATAFeatSupported;
    npU->SATACmdEnabled   = npID->SATAFeatEnabled;
  }

#if TRACES
  if (Debug & 8) {
    TraceStr ("C%lX/%lX,%X/%X", npU->CmdSupported, npU->CmdEnabled, npU->SATACmdSupported, npU->SATACmdEnabled);
  }
#endif

  npU->ATAVersion = CountBits (npID->MajorVersion) - 1;
  if (npU->ATAVersion > 15) npU->ATAVersion = 0;
  if (!npU->ATAVersion)
    if (npID->UltraDMAModes) npU->ATAVersion = 4;
    else if (npID->ATAPIMinorVersion >= 9) npU->ATAVersion = 2;
}

