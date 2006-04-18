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

USHORT MYENTRY DevHelp_AllocGDTSelector (PSEL	Selectors,
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

USHORT MYENTRY DevHelp_AttachDD (NPSZ	DDName,
				 NPBYTE IDCTable) {
  _asm {
    MOV  DI, AX
    MOV  DL, 0x2A
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    AND  AX, DX
  }
}

USHORT MYENTRY DevHelp_Beep (USHORT Frequency,
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

USHORT MYENTRY DevHelp_RegisterDeviceClassLite (NPSZ	 DeviceString,
						NPFN	 DriverEP,
						NPUSHORT DeviceHandle) {
  _asm {
    MOV  SI, BX
    MOV  BX, AX
    MOV  AX, CS
    XOR  DI, DI
    MOV  CX, 1
    MOV  DL, 0x43
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    MOV  BX, [DeviceHandle]
    MOV  [BX], AX
    AND  AX, DX
  }
}

USHORT MYENTRY DevHelp_CreateInt13VDM (PBYTE VDMInt13CtrlBlk) {
  _asm {
    MOV  SI, WORD PTR [VDMInt13CtrlBlk + 0]
    MOV  AX, WORD PTR [VDMInt13CtrlBlk + 2]
    MOV  DL, 0x6E
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    AND  AX, DX
  }
}

USHORT MYENTRY DevHelp_SetIRQ (NPFN   IRQHandler,
			       USHORT IRQLevel,
			       USHORT SharedFlag) {
  _asm {
    XCHG AX, BX
    MOV  DH, DL
    MOV  DL, 0x1B
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    AND  AX, DX
  }
}

USHORT MYENTRY DevHelp_UnSetIRQ (USHORT IRQLevel) {
  _asm {
    MOV  BX, AX
    MOV  DL, 0x1C
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    AND  AX, DX
  }
}

USHORT MYENTRY DevHelp_EOI (USHORT IRQLevel) {
  _asm {
    MOV  DL, 0x31
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    AND  AX, DX
  }
}

USHORT MYENTRY DevHelp_GetDOSVar (USHORT VarNumber,
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

USHORT MYENTRY DevHelp_Save_Message (NPBYTE MsgTable) {
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

USHORT MYENTRY DevHelp_VerifyAccess (SEL    MemSelector,
				     USHORT Length,
				     USHORT MemOffset,
				     UCHAR  AccessFlag) {
  _asm {
    MOV  CX, DX
    MOV  DI, BX
    MOV  DH, [AccessFlag]
    MOV  DL, 0x27
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    AND  AX, DX
  }
}

USHORT MYENTRY DevHelp_VirtToPhys (PVOID  SelOffset,
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

USHORT MYENTRY DevHelp_PhysToVirt (ULONG   PhysAddr,
				   USHORT  usLength,
				   PVOID   SelOffset) {
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

USHORT MYENTRY DevHelp_ProcRun (ULONG	EventId,
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

USHORT MYENTRY DevHelp_ProcBlock (ULONG  EventId,
				  ULONG  WaitTime,
				  USHORT IntWaitFlag) {
  _asm {
    XCHG AX, DX
    XCHG BX, DX
    MOV  DH, DL
    MOV  CX, WORD PTR [WaitTime + 0]
    MOV  DI, WORD PTR [WaitTime + 2]
    MOV  DL, 0x04
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    AND  AX, DX
    AND  DH, 0x80
    OR	 AH, DH
  }
}

USHORT MYENTRY DevHelp_AllocateCtxHook (NPFN   HookHandler,
					PULONG HookHandle) {
  _asm {
    _emit 0x66
    XOR  AX, AX
    MOV  AX, BX
    _emit 0x66
    XOR  BX, BX
    _emit 0x66
    DEC  BX
    MOV  DL, 0x63
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    LES  BX, [HookHandle]
    _emit 0x66
    MOV  ES:[BX], AX
    SBB  BX, BX
    AND  AX, BX
  }
}

USHORT MYENTRY DevHelp_FreeCtxHook (ULONG HookHandle) {
  _asm {
    _emit 0x66
    SHL  DX, 16
    MOV  DX, AX
    _emit 0x66
    MOV  AX, DX
    _emit 0x66
    MOV  DL, 0x64
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
  }
}

USHORT MYENTRY DevHelp_ArmCtxHook (ULONG HookData,
				   ULONG HookHandle) {
  _asm {
    _emit 0x66
    SHL  DX, 16
    MOV  DX, AX
    _emit 0x66
    MOV  AX, DX
    _emit 0x66
    MOV  BX, WORD PTR [HookHandle]
    _emit 0x66
    XOR  CX, CX
    _emit 0x66
    DEC  CX
    MOV  DL, 0x65
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  BX, BX
    AND  AX, BX
  }
}

USHORT MYENTRY DevHelp_VirtToLin (SEL	Selector,
				  ULONG Ofs,
				  PLIN	LinearAddr) {
  _asm {
    _emit 0x66
    MOV  SI, WORD PTR [Ofs]
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

USHORT MYENTRY DevHelp_VMAllocLite (USHORT  Flags,
				    USHORT  Size,
				    ULONG   PhysAddr,
				    PLIN    LinearAddr) {
  _asm {
    _emit 0x66
    XOR  CX, CX
    MOV  CX, AX
    _emit 0x66
    MOV  AX, CX
    MOV  CX, DX
    _emit 0x66
    MOV  DI, WORD PTR [PhysAddr]
    MOV  DL, 0x57
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    LES  BX, [LinearAddr]
    _emit 0x66
    MOV  ES:[BX], AX
    SBB  DX, DX
    AND  AX, DX
  }
}

USHORT MYENTRY DevHelp_LockLite (SEL Selector,
				 USHORT Flags) {
  _asm {
    MOV  BX, DX
    MOV  DL, 0x13
    MOV  DS, CS:[DSSel]
    CALL [Device_Help]
    SBB  DX, DX
    AND  AX, DX
  }
}


