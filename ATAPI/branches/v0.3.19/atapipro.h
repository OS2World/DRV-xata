/**************************************************************************
 *
 * SOURCE FILE NAME = ATAPIPRO.H
 *
 * DESCRIPTIVE NAME = Proto-types for ATAPI driver
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 ****************************************************************************/

#include "apmcalls.h"

#define OPTIMIZE "sacegln"  // MSC 6

USHORT NEAR _fastcall saveXCHG (NPUSHORT location, USHORT value);
UCHAR  NEAR _fastcall saveINC (NPBYTE counter);
UCHAR  NEAR _fastcall saveDEC (NPBYTE counter);

/*----------------------------------------------------------------------------*
 *	S506CSUB.C Utility Functions					      *
 *----------------------------------------------------------------------------*/
VOID   FAR  clrmem (NPVOID d, USHORT len);
VOID   FAR  fclrmem (PVOID d, USHORT len);
VOID   NEAR strnswap (NPSZ d, NPSZ s, USHORT n);
BOOL   NEAR strncmp (NPCH s1, NPCH s2, USHORT n);
BOOL   NEAR _strncmp (PUCHAR s1, PUCHAR s2, USHORT n);
VOID   NEAR _strncpy (NPSZ d, NPSZ s, USHORT n);

/* ATAPIISM.C */

VOID NEAR StartOSMRequest	    (NPA npA);
VOID NEAR StartISM		    (NPA npA);
VOID NEAR ISMStartState 	    (NPA npA);
VOID NEAR InterruptState	    (NPA npA);
VOID NEAR PutByteInSGList	    (NPA npA, USHORT Data);
VOID NEAR StartATA_ATAPICmd	    (NPA npA);
VOID NEAR SetATAPICmd		    (NPA npA);
VOID NEAR SetATACmd		    (NPA npA);
VOID NEAR WriteATAPIPkt 	    (NPA npA);
VOID NEAR InitializeISM 	    (NPA npA);
VOID FAR _cdecl IRQTimeOutHandler   (USHORT TimerHandle, PA pA);

VOID   NEAR PerfBMComplete	    (NPA npA , UCHAR immediate);
USHORT NEAR FakeATA		    (NPA npA);
USHORT NEAR FakeATAPI		    (NPA npA);

IRQEntry AdapterIRQ0;
IRQEntry AdapterIRQ1;
IRQEntry AdapterIRQ2;
IRQEntry AdapterIRQ3;
IRQEntry AdapterIRQ4;
IRQEntry AdapterIRQ5;
IRQEntry AdapterIRQ6;
IRQEntry AdapterIRQ7;
USHORT NEAR AdptInterrupt	    (NPA npA);
USHORT NEAR GetRegister 	    (NPA npA, USHORT Register);

BOOL   NEAR CreateBMSGList	    (NPA npA);
USHORT NEAR BuildSGList (NPPRD npSGOut, NPADD_XFER_IO_X npSGPtrs, ULONG Bytes, UCHAR SGAlign);


/* ATAPIOSM.C */

OSMEntry    StartOSM;
VOID   NEAR StartIORB		   (NPA npA);
VOID   NEAR CompleteIORB	   (NPA npA);
VOID   NEAR FreeHWResources	   (NPA npA);
USHORT NEAR ImmediateSuspendCheck  (NPA npA);
VOID   NEAR GetDeviceTable	   (NPA npA);
VOID   NEAR CompleteInit	   (VOID);
VOID   NEAR Suspend_Complete	   (NPA npA);
VOID   NEAR Resume_Complete	   (NPA npA);
USHORT NEAR NextIORB		   (NPA npA);
VOID   NEAR GetMediaGeometry	   (NPA npA);
VOID   NEAR GetDeviceGeometry	   (NPA npA);
VOID   NEAR UnitStatusMediaSense   (NPA npA, UCHAR command);
VOID   NEAR ChangeUnitStatus	   (NPU npU, PIORB_CHANGE_UNITSTATUS pIORB);
VOID   NEAR GetChangeLineState	   (NPA npA);
VOID   NEAR JustGo2CompleteIORB    (NPA npA);
VOID   NEAR ISMComplete 	   (NPA npA);
VOID   NEAR ValidateAndSend	   (NPA npA, UCHAR command);
VOID   NEAR SetupAndExecute	   (NPA npA, UCHAR command);
VOID   NEAR ProcessTURSense	   (NPA npA);
VOID   NEAR ProcessSenseData	   (NPA npA);
UCHAR  NEAR SetMediaFlags	   (NPA npA, UCHAR ASC);
VOID   NEAR SetIORBError	   (NPA npA, UCHAR ASC);
VOID   NEAR ProcessIdentify	   (NPA npA);
VOID   NEAR RecalculateGeometry    (NPGEOMETRY npGEO);
VOID   NEAR ProcessCapacity	   (NPA npA);
VOID   NEAR ProcessCDBModeSense    (NPA npA);
VOID   NEAR ProcessCDBInquiry	   (NPA npA);
USHORT NEAR MediaInvalid4Drive	   (NPA npA);
VOID   NEAR ExtractMediaGeometry   (NPA npA);
VOID   NEAR ClearCurrentMediaGEO   (NPA npA);
VOID   NEAR SetCmdError 	   (NPA npA);
VOID   NEAR Format_Track	   (NPA npA);
VOID   NEAR SetMediaGeometry	   (NPA npA);
VOID   NEAR GetLockStatus	   (NPA npA);
VOID   NEAR ChangeUnitInfo	   (NPA npA);
ULONG  NEAR _fastcall Swap32	   (ULONG x);
USHORT NEAR _fastcall Swap16	   (SHORT x);

/* ATAPINIT.C */
VOID   NEAR DriverInit		   (PRPINITIN  pRPH);
NPBYTE NEAR InitAllocate	   (USHORT Size);
VOID   NEAR InitDeAllocate	   (USHORT Size);
VOID   NEAR PrintInfo		   (NPA npA);
VOID   NEAR DisplayInfo 	   (NPA npA);
VOID   NEAR TTYWrite		   (NPSZ Buf);
USHORT NEAR ParseCmdLine	   (PSZ pCmdLine);
USHORT NEAR ClaimUnit		   (NPA npA, NPUNITINFO npOldUnitInfo);
USHORT NEAR Issue_ReportResources  (NPRESOURCE_ENTRY pRE, USHORT UnitHandle, UCHAR hADD);
VOID   NEAR ConfigureACB	   (NPA npA);
USHORT NEAR ATAPIIdentify	   (NPA npA);
VOID   NEAR ATAPIReset		   (NPA npA);
NPU    NEAR ConfigureUnit	   (NPA npA, NPIDENTIFYDATA NpID, UCHAR Reconfigure);
VOID   NEAR GetATAPIUnits	   (void);
VOID   NEAR GetCDBTranslation	   (NPU npU);
VOID   NEAR GetCDCapabilities	   (NPU npU);
VOID   NEAR GetGeometry 	   (NPU npU, ULONG DefaultSize);
VOID   NEAR InitOnstream	   (NPU npU);
USHORT NEAR CheckController	   (NPA npA);
USHORT NEAR CheckSectorReg	   (NPA npA);
USHORT NEAR CheckReady		   (NPA npA);
USHORT NEAR InitSuspendResumeOtherAdd (NPA npA, USHORT function);
IORBNotifyFnc NotifyIORBDone;
USHORT NEAR GetIdentifyData	   (NPU npU, NPIDENTIFYDATA npID);
USHORT NEAR GetSCSIInquiry	   (NPU npU);
VOID   NEAR GetLUNMode		   (NPU npU, USHORT LastLUN);
NPCH   NEAR PrepBuffer		   (NPU npU, USHORT Len);
NPIORB NEAR InitSendCDB 	   (NPU npU, UCHAR CmdLen, NPCH Cmd);
VOID   NEAR ScanForLegacyFloppies  (void);
BOOL   NEAR Remove_DriveFlag	   (USHORT Unit, NPUNITINFO pUnitInfo);
VOID   NEAR SetupSGlist 	   (void);

VOID   FAR  _cdecl ElapsedTimer    (USHORT hElapsedTimer, ULONG Unused1,
							  ULONG Unused2);
VOID   NEAR ResetController	   (NPA npA);
USHORT NEAR MapSGList		   (NPA npA);


/* ATAPIGEN.C */

VOID   NEAR IODelay			(void);
USHORT NEAR AllocDeallocChangeUnit	(UCHAR hADD, USHORT UnitHandle,
					 UCHAR function, PUNITINFO pUI);
USHORT NEAR SuspendResumeOtherAdd	(NPA npA, UCHAR function);
ULONG  NEAR QueryOtherAdd		(NPA npA);
IORBNotifyFnc NotifySuspendResumeDone;
USHORT NEAR Execute			(UCHAR ADDHandle, NPIORB npIORB);
VOID   NEAR _cdecl IDECDSt		();
USHORT NEAR DRQ_AND_BSY_CLEAR_WAIT	(NPA npA);
USHORT NEAR BSY_CLR_DRQ_SET_WAIT	(NPA npA);
USHORT NEAR DRQ_CLEAR_WAIT		(NPA npA);
USHORT NEAR BSYWAIT			(NPA npA);

VOID   NEAR ATAPICompleteInit		(NPA npA);
USHORT FAR  _loadds _cdecl APMEventHandler (PAPMEVENT);
USHORT NEAR APMSuspend			(void);
USHORT NEAR APMResume			(void);
void   NEAR _cdecl outpd		(USHORT port, ULONG val);
ULONG  NEAR inpd			(USHORT port);

/* ATAPIORB.C */
typedef PIORB FAR *PPIORB;

ADDEntry ATAPIADDEntry;
UCHAR  NEAR _fastcall x2BCD  (UCHAR Data);
VOID   NEAR Convert_17B_to_ATAPI (ULONG pPhysData);

/* PRINTF.C */
VOID   FAR   sprntf (NPSZ s, NPSZ fmt, ...);

