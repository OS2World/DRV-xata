/**************************************************************************
 *
 * SOURCE FILE NAME =  S506PRO.H
 *
 * DESCRIPTIVE NAME =  DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : C Function Prototype statements
 *
 *****************************************************************************/

#include "APMCALLS.H"

#define OPTIMIZE "sacegln"

/*--------------------*/
/* Strategy 1 Router  */
/*--------------------*/

VOID FAR  _cdecl S506Str1 (VOID);
VOID NEAR StatusError (PRPH pRPH, USHORT ErrorCode);
VOID NEAR ProcessLockUnlockEject (NPU npU, PIORB pIORB, UCHAR Function);
VOID NEAR ClearIORB (NPA npA);
int  NEAR IssueCommand (NPU npU);
int  NEAR IssueOneByte (NPU npU, BYTE Cmd);
int  NEAR IssueSetMax (NPU npU, PULONG numSectors, SHORT set);
int  NEAR IssueSetFeatures (NPU npU, BYTE SubCommand, BYTE Arg);
int  NEAR IssueGetMediaStatus (NPU npU);
int  NEAR IssueGetPowerStatus (NPU npU);
int  NEAR IssueSetIdle (NPU npU, UCHAR Cmd, BYTE Arg);
int  NEAR IOCtlRead (NPU npU, PBYTE pBuffer, USHORT Buffersize, ULONG RBA);
int  FAR  DoIOCtlPCMCIA (PVOID);

/*----------------------------------------------------------------------------*
 * S506IORB.C - IORB Queueing Entry Point				      *
 *----------------------------------------------------------------------------*/
VOID NEAR GetDeviceTable (PIORB pIORB);
VOID NEAR CompleteInit (NPA npA);
VOID NEAR AllocateUnit (NPA npA);
VOID NEAR DeallocateUnit (NPA npA);
VOID NEAR ChangeUnitInfo (NPA npA);
VOID NEAR GetGeometry (NPA npA);
VOID NEAR GetUnitStatus (NPA npA, USHORT Status);
VOID NEAR GetLockStatus (NPA npA);
VOID NEAR GetQueueStatus (NPA npA, PIORB pIORB);
VOID NEAR GetUnitResources (NPA npA, PIORB pIORB);
VOID NEAR SetMediaGeometry (NPA npA);
VOID NEAR Error (NPA npA, USHORT ErrorCode);
USHORT NEAR NextIORB (NPA npA);
VOID FAR  _loadds _cdecl ADDEntryPoint (PIORBH pNewIORB);
VOID NEAR IORBDone(NPA npA);
VOID NEAR PreProcessedIORBDone (PIORB pIORB);
VOID NEAR GetChangeLine (NPA npA);
VOID NEAR GetLockStatus (NPA npA);
VOID NEAR LockMedia (NPA npA);
VOID NEAR UnLockMedia (NPA npA);
VOID NEAR EjectMedia (NPA npA);

/*------------------------*/
/* Interrupt Entry Points */
/*------------------------*/

VOID   NEAR FixedIRQ0 (VOID);
VOID   NEAR FixedIRQ1 (VOID);
VOID   NEAR FixedIRQ2 (VOID);
VOID   NEAR FixedIRQ3 (VOID);
VOID   NEAR FixedIRQ4 (VOID);
VOID   NEAR FixedIRQ5 (VOID);
VOID   NEAR FixedIRQ6 (VOID);
VOID   NEAR FixedIRQ7 (VOID);
USHORT NEAR HandleInterrupts (NPA npA);
USHORT NEAR FixedInterrupt (NPA npA);
USHORT NEAR CatchInterrupt (NPA npA);

VOID   NEAR _cdecl LoadFlatGS (void);
ULONG  NEAR _fastcall saveCLR32 (ULONG *location);
ULONG  NEAR _fastcall saveGET32 (ULONG *location);
USHORT NEAR _fastcall saveXCHG (NPUSHORT location, USHORT value);
UCHAR  NEAR _fastcall saveINC (NPBYTE counter);
UCHAR  NEAR _fastcall saveDEC (NPBYTE counter);


/*----------------------------------------------------------------------------*
 *	S506TIMR.C Timer Routines					      *
 *----------------------------------------------------------------------------*/
VOID   FAR _cdecl IRQTimer (USHORT TimerHandle, PACB pA);
VOID   FAR _cdecl DelayedRetry(USHORT hRetryTimer, PACB pA);
VOID   FAR _cdecl DelayedReset(USHORT hResetTimer, PACB pA);
VOID   FAR _cdecl ElapsedTimer (USHORT hElapsedTimer, ULONG Unused);
VOID   FAR _cdecl IdleTicker (USHORT hTimer, ULONG Unused);
VOID   NEAR _fastcall IODly (USHORT count);
VOID   FAR  _fastcall IODlyFar (USHORT count);

/*----------------------------------------------------------------------------*
 *	S506IO.C IO Routines						      *
 *----------------------------------------------------------------------------*/

VOID   FAR  _fastcall OutB (ULONG Location, UCHAR Value);
VOID   FAR  _fastcall OutW (ULONG Location, USHORT Value);
VOID   FAR  _fastcall OutD (ULONG Location, ULONG Value);
UCHAR  FAR  _fastcall InB (ULONG Location);
USHORT FAR  _fastcall InW (ULONG Location);
ULONG  FAR  _fastcall InD (ULONG Location);
VOID   FAR  _fastcall OutBd (ULONG Location, UCHAR Value, USHORT Delay);
UCHAR  FAR  _fastcall InBd (ULONG Location, USHORT Delay);
UCHAR  FAR  _fastcall InWd (ULONG Location, USHORT Delay);
UCHAR  FAR  _fastcall InDd (ULONG Location, USHORT Delay);
VOID   FAR  _fastcall OutBdms (ULONG Location, UCHAR Value);
UCHAR  FAR  _fastcall InBdms (ULONG Location);
UCHAR  FAR  _fastcall InWdms (ULONG Location);
UCHAR  FAR  _fastcall InDdms (ULONG Location);
VOID   FAR  _fastcall OutB2 (USHORT Addr1, USHORT Addr2, UCHAR Value);
VOID   FAR  _fastcall OutW2 (USHORT Addr1, USHORT Addr2, USHORT Value);
VOID   FAR  _fastcall OutD2 (USHORT Addr1, USHORT Addr2, ULONG Value);
UCHAR  FAR  _fastcall InB2 (USHORT Addr1, USHORT Addr2);
USHORT FAR  _fastcall InW2 (USHORT Addr1, USHORT Addr2);
ULONG  FAR  _fastcall InD2 (USHORT Addr1, USHORT Addr2);
// Addr2:
#define IOPORT	0
#define PCICONFIG(x) (0x1000 | (x))
#define IS_MMIO(x) ((x) & 0xE0000000)

/*----------------------------------------------------------------------------*
 *	S506SM.C Procedures (State Machine)				      *
 *----------------------------------------------------------------------------*/
VOID   NEAR StartSM(NPA npA);
VOID   NEAR StartState(NPA npA);
VOID   NEAR StartIO(NPA npA);
VOID   NEAR RetryState(NPA npA);
VOID   NEAR InterruptState(NPA npA);
VOID   NEAR DoneState(NPA npA);
VOID   NEAR ErrorState(NPA npA);
USHORT NEAR StartOtherIO(NPA npA);
USHORT NEAR InitACBRequest(NPA npA);
BOOL   NEAR SetupFromATA(NPA npA, NPU npU);
BOOL   NEAR SetupSeek (NPA npA, NPU npU);
BOOL   NEAR SetupIdentify (NPA npA, NPU npU);
VOID   NEAR InitBlockIO(NPA npA);
USHORT NEAR StartBlockIO(NPA npA);
USHORT NEAR DoBlockIO(NPA npA, USHORT cSec);
VOID   NEAR DoOtherIO(NPA npA);
VOID   NEAR SetIOAddress(NPA npA);
VOID   NEAR SetRetryState(NPA npA);
VOID   NEAR DoReset(NPA npA);
VOID   NEAR ResetCheck (NPA npA);
USHORT NEAR GetDiagResults (NPA npA, PUCHAR Status);

USHORT NEAR CheckReady(NPA npA);
USHORT NEAR CheckBusy(NPA npA);
USHORT NEAR WaitDRQ(NPA npA);
VOID   NEAR UpdateBlockIOPtrs(NPA npA);
USHORT NEAR SendCmdPacket(NPA npA);
USHORT NEAR SendReset(NPA npA);
USHORT NEAR GetErrorReg(NPU npU);
VOID   NEAR SelectUnit (NPU npU);
USHORT NEAR GetStatusReg (NPA npA);
VOID   NEAR SendAckMediaChange (NPA npA);
USHORT NEAR GetMediaError (NPU npU);
USHORT NEAR MapError(NPA npA);
VOID   NEAR StopBMDMA (NPA npA);
BOOL   NEAR CreateBMSGList (NPA npA);
USHORT NEAR BuildSGList (NPPRD npSGOut, NPADD_XFER_IO_X npSGPtrs, ULONG Bytes, UCHAR SGAlign);

/*----------------------------------------------------------------------------*
 *	S506CSUB.C Utility Functions					      *
 *----------------------------------------------------------------------------*/
VOID   FAR clrmem (NPVOID d, USHORT len);
VOID   FAR fclrmem (PVOID d, USHORT len);
VOID   NEAR strnswap (NPSZ d, NPSZ s, USHORT n);
BOOL   FAR _strncmp (PUCHAR s1, PUCHAR s2, USHORT n);
USHORT FAR _strlen (PSZ s);

/*----------------------------------------------------------------------------*
 *	S506INIT.C Procedures						      *
 *----------------------------------------------------------------------------*/

VOID   NEAR DriveInit(PRPINITIN pRPH);
USHORT NEAR ConfigureController (NPA npA);
USHORT NEAR CheckController (NPA npA);
USHORT NEAR CheckReg (NPA npA, UCHAR UnitId);
USHORT NEAR ConfigureUnit (NPU npU);
USHORT NEAR GetReg (NPA npA, USHORT Reg);
USHORT NEAR CheckForATAPISignature (NPU npU);
USHORT NEAR DoATAPIPresenseCheck (NPU npU);
USHORT NEAR ATAPIReset (NPA npA);
USHORT NEAR ReadDrive (NPU npU, ULONG RBA, USHORT Retry, NPBYTE pBuf);
USHORT NEAR NoOp (NPU npU);
USHORT FAR  Execute (UCHAR ADDHandle, NPIORB npIORB);
VOID   FAR _cdecl _loadds NotifyIORBDone (PIORB pIORB);
USHORT FAR  ScanForOtherADDs(VOID);
VOID   NEAR CheckACBViable (NPA npA);
VOID   NEAR SetOption (NPU npU, NPA npA, USHORT Ofs, USHORT Value);
USHORT FAR  ParseCmdLine1st (PSZ pCmdLine);
USHORT FAR  ParseCmdLine (PSZ pCmdLine);
USHORT NEAR ParseGeom (NPU npU, NPCH pOutBuf);
VOID   FAR  PrintInfo (NPA Buf);
VOID   NEAR PrintAdapterInfo (NPA Buf);
VOID   FAR  SaveMsg (PSZ Buf);
VOID   FAR  TTYWrite (UCHAR Level, NPSZ Buf);
NPHRESOURCE NEAR AllocResource (NPHRESOURCE nphRes, ULONG Addr, USHORT Len, UCHAR shared);
VOID   FAR  AllocAdapterResources (NPA npA);
VOID   FAR  DeallocAdapterResources (NPA npA);
VOID   FAR  AssignAdapterResources (NPA npA);
NPA    FAR  LocateATEntry (USHORT BasePort, USHORT PCIAddr, UCHAR Channel);
VOID   NEAR UpdateATTable (NPRESOURCELIST npResourceList);
VOID   FAR  FindDetected (UCHAR SearchType);
USHORT NEAR ReadExCAByte (UCHAR Idx);
USHORT NEAR ReadExCAWord (UCHAR Idx);
VOID   FAR  FindHotPCCard (VOID);
VOID   NEAR ProcessVerboseTimings (NPU npU, NPA npA, NPCH s2, NPCH s3);
USHORT NEAR GetChipPIOMode (NPA npA);
VOID   NEAR GetDeviceULTRAMode (NPU npU, NPIDENTIFYDATA npID);
VOID   NEAR GetDeviceDMAMode (NPU npU, NPIDENTIFYDATA npID);
USHORT NEAR GetDevicePIOMode (NPU npU, NPIDENTIFYDATA npID);
VOID   NEAR UCBSetupDMAPIO (NPU npU, NPIDENTIFYDATA npID);
VOID   NEAR IdentifyDevice (NPU npU, NPIDENTIFYDATA npID, UCHAR ATAPIDevice);
VOID   FAR  CollectPorts (NPA npA);
ULONG  FAR  PortToPhys (ULONG Port, NPA npA);

#if 1
#pragma alloc_text (FCode, SetOption)
#pragma alloc_text (FCode, ParseCmdLine1st)
#pragma alloc_text (FCode, ParseCmdLine)
#pragma alloc_text (FCode, ParseGeom)
#pragma alloc_text (FCode, PrintInfo)
#pragma alloc_text (FCode, PrintAdapterInfo)
#pragma alloc_text (FCode, ProcessVerboseTimings)
#pragma alloc_text (FCode, TTYWrite)
#pragma alloc_text (FCode, SaveMsg)
#pragma alloc_text (FCode, ScanForOtherADDs)
#pragma alloc_text (FCode, LocateATEntry)
#pragma alloc_text (FCode, UpdateATTable)
#pragma alloc_text (FCode, PortToPhys)
#pragma alloc_text (FCode, FindDetected)
#pragma alloc_text (FCode, FindHotPCCard)
#pragma alloc_text (FCode, ReadExCAByte)
#pragma alloc_text (FCode, ReadExCAWord)
#pragma alloc_text (FCode, AllocAdapterResources)
#pragma alloc_text (FCode, AllocResource)
#pragma alloc_text (FCode, DeallocAdapterResources)
#pragma alloc_text (FCode, AssignAdapterResources)
#endif
/*----------------------------------------------------------------------------*
 *	S506PRTF.C Procedures						      *
 *----------------------------------------------------------------------------*/

VOID   FAR   sprntf (NPSZ s, NPSZ fmt, ...);
USHORT FAR   prntf (NPSZ s, NPSZ fmt, PUCHAR arg_ptr);
VOID   FAR   _strncpy (NPSZ d, NPSZ s, USHORT n);

/*----------------------------------------------------------------------------*
 *	S506OSM2.C Procedures						      *
 *----------------------------------------------------------------------------*/

typedef PIORB FAR *PPIORB;

VOID   NEAR SuspendState (NPA npA);
VOID   NEAR Suspend (NPA npA);
VOID   NEAR SuspendIORBReq (NPA npA, PIORB pIORB);
VOID   NEAR ResumeIORBReq (NPA npA, PIORB pIORB);
NPIHDRS FAR HookIRQ (NPA npA);
VOID   FAR  UnhookIRQ (NPA npA);
VOID   FAR  RehookIRQ (NPA npA);
USHORT FAR  ActivateACB (NPA npA);
VOID   FAR  DeactivateACB (NPA npA);
PIORB  NEAR PreProcessIORBs (NPA npA, NPU npU, PPIORB ppFirstIORB);
VOID   NEAR RemoveIORB (PIORB pIORB, PIORB pIORBPrev,
			PPIORB pIORBFirst);
USHORT NEAR AllocateHWResources (NPA npA);
NPA    NEAR FreeHWResources (NPA npA);

/*----------------------------------------------------------------------------*
 *	S506APM.C Procedures						      *
 *----------------------------------------------------------------------------*/
USHORT FAR _cdecl APMEventHandler (PAPMEVENT Event);
USHORT NEAR APMSuspend (USHORT PowerState);
USHORT NEAR APMResume (VOID);
USHORT NEAR NotifyFLT (NPU npU, UCHAR CardEvent);
USHORT NEAR ReConfigureController (NPA npA);
USHORT NEAR ReConfigureUnit (NPU npU);
void   NEAR ReInitUnit (NPU npU);
void   NEAR InitUnitSync (NPU npU);
void   NEAR UnitFlush (NPU npU);
VOID   NEAR CardInsertion (NPA npA);
VOID   NEAR _fastcall CardInsertionDeferred (NPU npU);
VOID   NEAR CardRemoval (NPA npA);
VOID   NEAR StartDetect (NPA npA);
VOID   NEAR StartDetectSATA (NPU npU);
VOID   NEAR SATARemoval (NPU npU);
VOID   NEAR IssueSATAReset (NPU npU);
VOID   FAR _cdecl InsertHandler (USHORT hTimer, PACB pA, ULONG Unused);
VOID   FAR _cdecl CSHookHandler (void);
VOID   NEAR Delay (USHORT ms);

/*----------------------------------------------------------------------------*
 *	CS.C Procedures 						      *
 *----------------------------------------------------------------------------*/
USHORT FAR  PCMCIASetup (VOID);
USHORT FAR  CSCardPresent (USHORT);
USHORT NEAR CallCS (USHORT Function, PVOID Arg, USHORT ArgLen);
USHORT NEAR TryIRQ (struct IRQ_P _far *SetIRQ, USHORT Filter);
USHORT NEAR TryIO (struct IO_P _far *SetIO, USHORT Port);
PUCHAR NEAR GetVolt (PUCHAR p, PUCHAR Volt);
USHORT FAR  CSConfigure (NPA npA, USHORT Socket);
VOID   FAR  CSReconfigure (NPA npA);
USHORT FAR  CSUnconfigure (NPA npA);
USHORT NEAR CSRegisterClient (void);
BOOL   NEAR CardServicesPresent (void);
VOID   NEAR _cdecl CSCallbackHandler (USHORT Socket, USHORT Event, PUCHAR Buffer);
NPA    NEAR FindAdapterForSocket (UCHAR Socket);

#pragma alloc_text (CSCode, PCMCIASetup)
#pragma alloc_text (CSCode, CardServicesPresent)
#pragma alloc_text (CSCode, CSCardPresent)
#pragma alloc_text (CSCode, CSRegisterClient)
#pragma alloc_text (CSCode, CallCS)
#pragma alloc_text (CSCode, TryIRQ)
#pragma alloc_text (CSCode, TryIO)
#pragma alloc_text (CSCode, GetVolt)
#pragma alloc_text (CSCode, CSConfigure)
#pragma alloc_text (CSCode, CSReconfigure)
#pragma alloc_text (CSCode, CSUnconfigure)
//#pragma alloc_text (CSCode, FindAdapterForSocket)

/*----------------------------------------------------------------------------*
 *	S506OEM.C Procedures						      *
 *----------------------------------------------------------------------------*/

int    NEAR BMCheckIRQ (NPA npA);

/*---------------------------------------------*/
/* PCI Prototypes			       */
/*---------------------------------------------*/

UCHAR  FAR  SetupOEMHlp(VOID);
USHORT FAR  _fastcall CallOEMHlp (PRPH pRPH);
BOOL   FAR  ReadPCIConfigSpace (NPPCI_INFO npPCIInfo, UCHAR ConfigReg,
				PULONG Data, UCHAR Size);
USHORT NEAR ReadConfigB (NPPCI_INFO npPCIInfo, USHORT ConfigReg);
USHORT NEAR ReadConfigW (NPPCI_INFO npPCIInfo, UCHAR ConfigReg);
ULONG  NEAR ReadConfigD (NPPCI_INFO npPCIInfo, UCHAR ConfigReg);
USHORT NEAR RConfigB (UCHAR ConfigReg);
USHORT NEAR RConfigW (UCHAR ConfigReg);
BOOL   FAR  WritePCIConfigSpace (NPPCI_INFO npPCIInfo, UCHAR ConfigReg,
				 ULONG Data, UCHAR Size);
VOID   NEAR WriteConfigB (NPPCI_INFO npPCIInfo, USHORT ConfigReg, UCHAR Data);
VOID   NEAR WriteConfigBI(NPPCI_INFO npPCIInfo, UCHAR ConfigReg, UCHAR Data);
VOID   NEAR WriteConfigW (NPPCI_INFO npPCIInfo, UCHAR ConfigReg, USHORT Data);
VOID   NEAR WriteConfigWI(NPPCI_INFO npPCIInfo, UCHAR ConfigReg, USHORT Data);
VOID   NEAR WriteConfigD (NPPCI_INFO npPCIInfo, UCHAR ConfigReg, ULONG Data);
VOID   NEAR WriteConfigDI(NPPCI_INFO npPCIInfo, UCHAR ConfigReg, ULONG Data);
VOID   NEAR WConfigB (UCHAR ConfigReg, UCHAR Data);
VOID   NEAR WConfigBI (UCHAR ConfigReg, UCHAR Data);
VOID   NEAR WConfigW (UCHAR ConfigReg, USHORT Data);
VOID   NEAR WConfigWI (UCHAR ConfigReg, USHORT Data);
VOID   NEAR WConfigD (UCHAR ConfigReg, ULONG Data);
BOOL   FAR  PciSetReg(USHORT PCIAddr, UCHAR ConfigReg, ULONG val,UCHAR size);
BOOL   FAR  PciGetReg(USHORT PCIAddr, UCHAR ConfigReg, PULONG val,UCHAR size);
BOOL   NEAR PciGetRegMech1 (USHORT regmask, UCHAR index, PULONG val, UCHAR size);
BOOL   NEAR PciGetRegMech2 (USHORT regmask, UCHAR index, PULONG val, UCHAR size);
BOOL   NEAR PciSetRegMech2 (USHORT regmask, UCHAR index, ULONG val, UCHAR size);
BOOL   NEAR PciSetRegMech1 (USHORT regmask, UCHAR index, ULONG val, UCHAR size);
USHORT NEAR GetRegB (USHORT PCIAddr, UCHAR ConfigReg);
USHORT NEAR GetRegW (USHORT PCIAddr, UCHAR ConfigReg);
ULONG  NEAR GetRegD (USHORT PCIAddr, UCHAR ConfigReg);
VOID   NEAR SetRegB (USHORT PCIAddr, UCHAR ConfigReg, UCHAR val);
VOID   NEAR SetRegW (USHORT PCIAddr, UCHAR ConfigReg, USHORT val);
VOID   NEAR SetRegD (USHORT PCIAddr, UCHAR ConfigReg, ULONG val);
VOID   FAR  ConfigurePCI (NPC npC, USHORT InPhase);
BOOL   NEAR CheckWellknown (NPPCI_INFO npPCIInfo);
VOID   NEAR SetLatency (NPA npA, UCHAR Lat);
VOID   NEAR GetBAR (NPBAR npB, USHORT PCIAddr, UCHAR BAR);
USHORT NEAR GetPCIBuses (VOID);
UCHAR  NEAR CollectSCRPorts (NPU npU);
UCHAR  NEAR CheckSATAPhy (NPU npU);
VOID   NEAR SetupT13StandardController (NPA npA);
USHORT FAR  EnumPCIDevices (VOID);
VOID   FAR  FixAddr (NPA npA);
UCHAR  NEAR ProbeChannel (NPA npA);
UCHAR  NEAR HandleFoundAdapter (NPA npA, NPPCI_DEVICE npDev);

#pragma alloc_text (FCode, SetupOEMHlp)
#pragma alloc_text (FCode, CallOEMHlp)
#pragma alloc_text (FCode, ReadPCIConfigSpace)
#pragma alloc_text (FCode, WritePCIConfigSpace)
#pragma alloc_text (FCode, ConfigurePCI)
#pragma alloc_text (FCode, GetBAR)
#pragma alloc_text (FCode, CollectSCRPorts)
#pragma alloc_text (FCode, CheckSATAPhy)
#pragma alloc_text (FCode, SetupT13StandardController)
#pragma alloc_text (FCode, EnumPCIDevices)
#pragma alloc_text (FCode, HandleFoundAdapter)
#pragma alloc_text (FCode, SetLatency)
#pragma alloc_text (FCode, ProbeChannel)
#pragma alloc_text (FCode, GetPCIBuses)
#pragma alloc_text (FCode, CheckWellknown)
#pragma alloc_text (FCode, FixAddr)
#pragma alloc_text (FCode, PciSetReg)
#pragma alloc_text (FCode, PciGetReg)
#pragma alloc_text (FCode, PciGetRegMech1)
#pragma alloc_text (FCode, PciGetRegMech2)
#pragma alloc_text (FCode, PciSetRegMech1)
#pragma alloc_text (FCode, PciSetRegMech2)
#pragma alloc_text (FCode, GetRegB)
#pragma alloc_text (FCode, GetRegW)
#pragma alloc_text (FCode, GetRegD)
#pragma alloc_text (FCode, SetRegB)
#pragma alloc_text (FCode, SetRegW)
#pragma alloc_text (FCode, SetRegD)


int GetInquiryData (NPU npU, PBYTE pBuffer, USHORT Buffersize);

/*----------------------------------------------------------------*/
/*	S506COMN.C Procedures					  */
/*----------------------------------------------------------------*/
VOID   NEAR SetupDefault (NPA npA);
VOID   NEAR SetupCommon (NPA npA);
USHORT NEAR SetupCommonPre (NPA npA);
VOID   NEAR SetupCommonPost (NPA npA);
VOID   NEAR CalculateAdapterTiming (NPA npA);
VOID   NEAR PickClockIndex (void);
VOID   NEAR DeviceTimingMode (NPU npU, UCHAR Restrictions);
VOID   NEAR EnablePCIBM (NPA npA);
VOID   NEAR SetDeviceDMAMode (NPU npU);
VOID   NEAR ReInitDeviceDMAMode (NPU npU);
VOID   NEAR SetDevicePIOMode (NPU npU);
VOID   NEAR SetDMAcapableBits (NPA npA);
VOID   NEAR DisableAdapterBMDMA (NPA npA);
int    NEAR NonsharedCheckIRQ (NPA npA);

/*----------------------------------------------------------------*/
/*	S506PIIX.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptPIIX (NPA npA);
BOOL   NEAR AcceptSMSC (NPA npA);
USHORT NEAR GetPIIXPio (NPA npA, UCHAR Unit);
VOID   NEAR PIIXTimingValue (NPU npU);
VOID   NEAR ProgramPIIXChip (NPA npA);
VOID   NEAR EnableBusPIIX (NPA npA, UCHAR enable);

#pragma alloc_text (FCode, AcceptPIIX)
#pragma alloc_text (FCode, AcceptSMSC)

/*----------------------------------------------------------------*/
/*	S506VIA.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptVIA (NPA npA);
BOOL   NEAR AcceptAMD (NPA npA);
BOOL   NEAR AcceptNVidia (NPA npA);
USHORT NEAR GetVIAPio (NPA npA, UCHAR Unit);
VOID   NEAR VIATimingValue (NPU npU);
VOID   NEAR ProgramVIAChip (NPA npA);

#pragma alloc_text (FCode, AcceptVIA)
#pragma alloc_text (FCode, AcceptAMD)
#pragma alloc_text (FCode, AcceptNVidia)

/*----------------------------------------------------------------*/
/*	S506SIS.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptSIS (NPA npA);
USHORT NEAR GetSISPio (NPA npA, UCHAR Unit);
VOID   NEAR SISTimingValue (NPU npU);
VOID   NEAR ProgramSISChip (NPA npA);
VOID   NEAR SIS2TimingValue (NPU npU);
VOID   NEAR ProgramSIS2Chip (NPA npA);

#pragma alloc_text (FCode, AcceptSIS)

/*----------------------------------------------------------------*/
/*	S506ALI.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptALI (NPA npA);
USHORT NEAR GetALIPio (NPA npA, UCHAR Unit);
VOID   NEAR SetupALI (NPA npA);
VOID   NEAR ALITimingValue (NPU npU);
VOID   NEAR ProgramALIChip (NPA npA);

#pragma alloc_text (FCode, AcceptALI)

/*----------------------------------------------------------------*/
/*	S506GNRC.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptGeneric (NPA npA);
BOOL   NEAR AcceptNetCell (NPA npA);
BOOL   NEAR AcceptJM (NPA npA);
BOOL   NEAR AcceptMarvell (NPA npA);
USHORT NEAR GetGenericPio (NPA npA, UCHAR Unit);
VOID   NEAR SetupGeneric (NPA npA);
VOID   NEAR GenericInitComplete (NPA npA);
VOID   NEAR CalculateGenericAdapterTiming (NPA npA);
VOID   NEAR ProgramGenericChip (NPA npA);
VOID   NEAR GenericSetupTF (NPA npA);
VOID   NEAR GenericSetupDMA (NPA npA);
VOID   NEAR GenericStartOp (NPA npA);
VOID   NEAR GenericStopDMA (NPA npA);
VOID   NEAR GenericErrorDMA (NPA npA);
VOID   NEAR GenericSATA (NPA npA);
ULONG  NEAR GetAHCISCR (NPA npA, USHORT Port);

#pragma alloc_text (FCode, AcceptGeneric)
#pragma alloc_text (FCode, AcceptNetCell)
#pragma alloc_text (FCode, AcceptJM)
#pragma alloc_text (FCode, AcceptMarvell)
#pragma alloc_text (FCode, GenericInitComplete)
#pragma alloc_text (FCode, GenericSATA)
#pragma alloc_text (FCode, GetAHCISCR)

/*----------------------------------------------------------------*/
/*	S506CMD.C Procedures
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptCMD (NPA npA);
USHORT NEAR GetCMDPio (NPA npA, UCHAR Unit);
int    NEAR CMDCheckIRQ (NPA npA);
VOID   NEAR CMDTimingValue (NPU npU);
VOID   NEAR ProgramCMDChip (NPA npA);

#pragma alloc_text (FCode, AcceptCMD)

/*----------------------------------------------------------------*/
/*	S506PDC.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptPDC (NPA npA);
BOOL   NEAR AcceptPDCTX2 (NPA npA);
BOOL   NEAR AcceptPDCMIO (NPA npA);
USHORT NEAR GetPDCPio (NPA npA, UCHAR Unit);
USHORT NEAR GetPDCPioTX2 (NPA npA, UCHAR Unit);
VOID   NEAR SetupPDCTX2 (NPA npA);
VOID   NEAR PromiseInitComplete (NPA npA);
int    NEAR PromiseCheckIRQ (NPA npA);
int    NEAR PromiseTX2CheckIRQ (NPA npA);
int    NEAR PromiseMCheckIRQ (NPA npA);
VOID   NEAR PDCTimingValue (NPU npU);
VOID   NEAR ProgramPDCChip (NPA npA);
VOID   NEAR ProgramPDCChipTX2 (NPA npA);
VOID   NEAR ProgramPDCIO (NPU npU);
VOID   NEAR PDCSetupDMA (NPA npA);
VOID   NEAR PDCStartOp (NPA npA);
VOID   NEAR PDCPreReset (NPA npA);
VOID   NEAR PDCPostReset (NPA npA);
VOID   NEAR PromiseMStartOp (NPA npA);
VOID   NEAR PromiseMSetupDMA (NPA npA);
VOID   NEAR PromiseMStopDMA (NPA npA);
VOID   NEAR PromiseMErrorDMA (NPA npA);

#pragma alloc_text (FCode, AcceptPDC)
#pragma alloc_text (FCode, AcceptPDCTX2)
#pragma alloc_text (FCode, AcceptPDCMIO)
#pragma alloc_text (FCode, PromiseInitComplete)

/*----------------------------------------------------------------*/
/*	S506CX.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptCx (NPA npA);
USHORT NEAR GetCxPio (NPA npA, UCHAR Unit);
VOID   NEAR CxTimingValue (NPU npU);
VOID   NEAR ProgramCxChip (NPA npA);

#pragma alloc_text (FCode, AcceptCx)

/*----------------------------------------------------------------*/
/*	S506HPT.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptHPT (NPA npA);
UCHAR  NEAR SetupHPTPLL (NPA npA);
USHORT NEAR GetHPTPio (NPA npA, UCHAR Unit);
int    NEAR HPTCheckIRQ (NPA npA);
VOID   NEAR HPTTimingValue (NPU npU);
VOID   NEAR ProgramHPTChip (NPA npA);
VOID   NEAR HPTEnableInt (NPA npA);
VOID   NEAR HPT36xSetupDMA (NPA npA);
VOID   NEAR HPT37xSetupDMA (NPA npA);
VOID   NEAR HPT36xErrorDMA (NPA npA);
VOID   NEAR HPT37xErrorDMA (NPA npA);
VOID   NEAR HPT36xPreReset (NPA npA);
VOID   NEAR HPT37xPreReset (NPA npA);

#pragma alloc_text (FCode, AcceptHPT)
#pragma alloc_text (FCode, SetupHPTPLL)

/*----------------------------------------------------------------*/
/*	S506AEC.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptAEC (NPA npA);
BOOL   NEAR AcceptAEC2 (NPA npA);
USHORT NEAR GetAECPio (NPA npA, UCHAR Unit);
VOID   NEAR AECTimingValue (NPU npU);
VOID   NEAR ProgramAECChip (NPA npA);
BOOL   NEAR AcceptAEC2 (NPA npA);
USHORT NEAR GetAEC2Pio (NPA npA, UCHAR Unit);
VOID   NEAR AEC2TimingValue (NPU npU);
VOID   NEAR ProgramAEC2Chip (NPA npA);

#pragma alloc_text (FCode, AcceptAEC)
#pragma alloc_text (FCode, AcceptAEC2)

/*----------------------------------------------------------------*/
/*	S506SW.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptOSB (NPA npA);
BOOL   NEAR AcceptIXP (NPA npA);
USHORT NEAR GetOSBPio (NPA npA, UCHAR Unit);
VOID   NEAR OSBTimingValue (NPU npU);
VOID   NEAR ProgramOSBChip (NPA npA);

#pragma alloc_text (FCode, AcceptOSB)
#pragma alloc_text (FCode, AcceptIXP)

/*----------------------------------------------------------------*/
/*	S506OPTI.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptOPTI (NPA npA);
USHORT NEAR GetOPTIPio (NPA npA, UCHAR Unit);
VOID   NEAR OPTITimingValue (NPU npU);
VOID   NEAR ProgramOPTIChip (NPA npA);

#pragma alloc_text (FCode, AcceptOPTI)

/*----------------------------------------------------------------*/
/*	S506SII.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptSII (NPA npA);
BOOL   NEAR AcceptIXPSATA (NPA npA);
VOID   NEAR SiISATA (NPA npA);
USHORT NEAR GetSIIPio (NPA npA, UCHAR Unit);
USHORT NEAR GetSIISATAPio (NPA npA, UCHAR Unit);
VOID   NEAR SIITimingValue (NPU npU);
VOID   NEAR ProgramSIIChip (NPA npA);
VOID   NEAR ProgramSIISATAChip (NPA npA);
VOID   NEAR SIISetupTF (NPA npA);
VOID   NEAR SIIStartOp (NPA npA);
VOID   NEAR SIIStartStop (NPA npA, UCHAR State);
int    NEAR SIICheckIRQ (NPA npA);

#pragma alloc_text (FCode, AcceptSII)
#pragma alloc_text (FCode, AcceptIXPSATA)
#pragma alloc_text (FCode, SiISATA)

/*----------------------------------------------------------------*/
/*	S506ITE.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptITE (NPA npA);
USHORT NEAR GetITEPio (NPA npA, UCHAR Unit);
VOID   NEAR ITETimingValue (NPU npU);
VOID   NEAR ProgramITEChip (NPA npA);

#pragma alloc_text (FCode, AcceptITE)

/*----------------------------------------------------------------*/
/*	S506NTIO.C Procedures					  */
/*----------------------------------------------------------------*/
BOOL   NEAR AcceptInitio (NPA npA);
int    NEAR InitioCheckIRQ (NPA npA);
VOID   NEAR InitioStartOp (NPA npA);
VOID   NEAR InitioSetupDMA (NPA npA);
VOID   NEAR InitioStopDMA (NPA npA);
VOID   NEAR InitioErrorDMA (NPA npA);

#pragma alloc_text (FCode, AcceptInitio)

/*---------------------------------------------------------------*
 *	S506GEOM.C Procedures					 *
 *---------------------------------------------------------------*/

VOID   NEAR DetermineUnitGeometry (NPU npU);
UCHAR  NEAR AdjustCylinders (NPGEO npGEO, ULONG TotalSectors);
VOID   NEAR UnlockMax (NPU npU, NPIDENTIFYDATA npId);
BOOL   NEAR IdentifyGeometryGet (NPU npU);
VOID   NEAR IDEGeomExtract (NPGEO npIDEGeom, NPIDENTIFYDATA npId);
VOID   NEAR LogGeomCalculate (NPGEO npLogGeom, NPGEO npPhysGeom);
VOID   NEAR LogGeomCalculateLBAAssist (NPGEO1 npLogGeom, ULONG TotalSectors);
VOID   NEAR LogGeomCalculateBitShift (NPGEO1 npLogGeom, NPGEO1 npPhysGeom);
BOOL   NEAR Int13GeometryGet (NPU npU);
BOOL   NEAR BPBGeometryGet (NPU npU);
BOOL   NEAR ReadAndExtractBPB (NPU npU, ULONG partitionBootRecordSectorNum, NPGEO2 npLogGeom);
USHORT NEAR VerifyEndOfMedia (NPU npU, NPGEO npGEOC, PULONG MaxSec);
BOOL   NEAR PhysGeomValidate (NPGEO1 npGeom);
BOOL   FAR  LogGeomValidate (NPGEO1 npGeom);
VOID   NEAR IDStringsExtract (NPCH s, NPIDENTIFYDATA npID);
VOID   NEAR MediaStatusEnable (NPU npU, NPIDENTIFYDATA npID);
VOID   NEAR MultipleModeEnable (NPU npU, NPIDENTIFYDATA npID);
VOID   NEAR AcousticEnable (NPU npU, NPIDENTIFYDATA npID);
VOID   NEAR IdleTimerEnable (NPU npU, NPIDENTIFYDATA npID);
USHORT NEAR DriveTypeToGeometry (USHORT DriveType, NPGEO1 npGEO);

#pragma alloc_text (FCode, DriveTypeToGeometry)

/*---------------------------------------------------------------*
 *	S506I13.C Procedures					 *
 *---------------------------------------------------------------*/

BOOL NEAR VDMInt13Create (VOID);
BOOL FAR  Int13Fun08GetLogicalGeom (NPU npU);
BOOL NEAR Int13DoRequest (VOID);
VOID NEAR _loadds _cdecl VDMInt13CallBack (VOID);
VOID FAR  _cdecl StubVDMInt13CallBack (VOID);

#pragma alloc_text (FCode, VDMInt13Create)
#pragma alloc_text (FCode, Int13Fun08GetLogicalGeom)
#pragma alloc_text (FCode, Int13DoRequest)
#pragma alloc_text (FCode, VDMInt13CallBack)

/*---------------------------------------------------------------*
 *	ACPI.C Procedures					 *
 *---------------------------------------------------------------*/

BOOL FAR ACPISetup (VOID);
USHORT FAR _fastcall CallAcpiCA (PRPH pRPH);
LIN FAR AcpiPointer (PVOID p);

#pragma alloc_text (ACPICode, ACPISetup)
#pragma alloc_text (ACPICode, AcpiPointer)


