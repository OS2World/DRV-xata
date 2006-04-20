/**************************************************************************
 *
 * SOURCE FILE NAME = S506DH.c
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : Device Helper library
 ****************************************************************************/

 #define INCL_NOPMAPI
 #define INCL_DOSINFOSEG
 #define INCL_NO_SCB
 #define INCL_INITRP_ONLY
 #define INCL_DOSERRORS
 #include "os2.h"
 #include "dhcalls.h"

extern USHORT (FAR * FAR Device_Help)();
extern USHORT DSSel;

USHORT FAR _fastcall DevHelp_AllocGDTSelector (PSEL   Selectors,
					       USHORT Count) {
  _asm {
    LES  DI, [Selectors]
    MOV  CX, AX
    MOV  DL, 0x2D
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DI, DI
    AND  AX, DI
  }
}

USHORT FAR _fastcall DevHelp_Beep (USHORT Frequency,
				   USHORT DurationMS) {
  _asm {
    MOV  BX, AX
    MOV  CX, DX
    MOV  DL, 0x52
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    AND  AX, DX
  }
}

USHORT FAR _fastcall DevHelp_RegisterDeviceClass (NPSZ	  DeviceString,
						  PFN	  DriverEP,
						  USHORT  DeviceFlags,
						  USHORT  DeviceClass,
						  PUSHORT DeviceHandle) {
  _asm {
    MOV  SI, BX
    MOV  DI, AX
    MOV  CX, DX
    MOV  BX, [DriverEP + 0]
    MOV  AX, [DriverEP + 2]
    MOV  DL, 0x43
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    LES  DI, [DeviceHandle]
    MOV  ES:[DI], AX
    AND  AX, DX
  }
}

USHORT FAR _fastcall DevHelp_GetDOSVar (USHORT VarNumber,
					USHORT VarMember,
					PPVOID KernelVar) {
  _asm {
    MOV  CX, DX
    MOV  DL, 0x24
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    MOV  CX, BX
    LES  BX, [KernelVar]
    MOV  ES:[BX + 0], CX
    MOV  ES:[BX + 2], AX
    AND  AX, DX
  }
}

USHORT FAR _fastcall DevHelp_EOI( USHORT IRQLevel ) {
  _asm {
    MOV  DL, 0x31
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    AND  AX, DX
  }
}

USHORT FAR _fastcall DevHelp_Save_Message (NPBYTE MsgTable) {
  _asm {
    MOV  SI, BX
    XOR  BX, BX
    MOV  DL, 0x3D
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    AND  AX, DX
  }
}

USHORT FAR _fastcall DevHelp_VirtToPhys (PVOID	SelOffset,
					 PULONG PhysAddr) {
  _asm {
    PUSH DS
    POP  ES
    LDS  SI, [SelOffset]
    MOV  DL, 0x16
    MOV  ES, CS:[DSSel]
    CALL ES:[Device_Help]
    SBB  DX, DX
    OR	 SI, SI
    PUSH ES
    POP  DS
    LES  SI, [PhysAddr]
    MOV  ES:[SI + 0], BX
    MOV  ES:[SI + 2], AX
    AND  AX, DX
  }
}

USHORT FAR _fastcall DevHelp_PhysToVirt (ULONG	 PhysAddr,
					 USHORT  usLength,
					 PVOID	 SelOffset) {
  _asm {
    MOV  CX, BX
    MOV  BX, AX
    MOV  AX, DX
    MOV  DH, 1
    MOV  DL, 0x15
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    PUSH ES
    PUSH DI
    LES  DI, [SelOffset]
    POP  ES:[DI + 0]
    POP  ES:[DI + 2]
    AND  AX, DX
  }
}

USHORT FAR _fastcall DevHelp_ProcRun (ULONG   EventId,
				      PUSHORT AwakeCount) {
  _asm {
    XCHG AX, DX
    XCHG BX, DX
    MOV  DL, 0x05
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    LES  BX, [AwakeCount]
    MOV  ES:[BX], AX
    AND  AX, DX
  }
}

USHORT FAR _fastcall DevHelp_ProcBlock (ULONG  EventId,
					ULONG  WaitTime,
					USHORT IntWaitFlag) {
  _asm {
    XCHG AX, DX
    XCHG BX, DX
    MOV  DH, DL
    MOV  CX, [WaitTime + 0]
    MOV  DI, [WaitTime + 2]
    MOV  DL, 0x04
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    AND  AX, DX
    AND  DH, 0x80
    OR	 AH, DH
  }
}

USHORT FAR _fastcall DevHelp_VirtToLin (SEL   Selector,
					ULONG Ofs,
					PLIN  LinearAddr) {
  _asm {
    _emit 0x66
    MOV  SI, [Ofs]
    MOV  DL, 0x5B
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    LES  BX, [LinearAddr]
    _emit 0x66
    MOV  ES:[BX], AX
    SBB  DX, DX
    AND  AX, DX
  }
}

USHORT FAR _fastcall DevHelp_PageListToGDTSelector (SEL    Selector,
						    ULONG  lSize,
						    USHORT Access,
						    LIN    pPageList) {
  _asm {
    _emit 0x66
    MOV  CX, [lSize]
    _emit 0x66
    MOV  DI, [pPageList]
    MOV  DH, DL
    MOV  DL, 0x60
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    AND  AX, DX
  }
}

