	page	,132

;**************************************************************************
;*
;* SOURCE FILE NAME = S506SEGS.ASM
;*
;* DESCRIPTIVE NAME = DaniS506.ADD - Adapter Driver for PATA/SATA DASD
;*		      CODE/DATA segment declarations.
;*
;* Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
;*	       COPYRIGHT Daniela Engert, 2000-2006
;*
;****************************************************************************

		INCLUDE devhdr.inc
		INCLUDE devcmd.inc

AUTOMOUNT	=	0

EXTRN		DOSIODELAYCNT:ABS
EXTRN		DOS32FLATDS:ABS

if AUTOMOUNT
EXTRN		SAS_SEL:ABS
endif

DDHeader	SEGMENT DWORD PUBLIC 'DATA'
		PUBLIC	_DiskDDHeader

;_DiskDDHeader	 DW	 OFFSET _DiskDDHeader2, SEG _DiskDDHeader2
_DiskDDHeader	DD	-1
		DW	DEVLEV_3 + DEV_CHAR_DEV + DEV_30
		DW	_S506Str1
		DW	0
		DB	"IBMS506$"
		DW	0, 0, 0, 0
		DD	DEV_ADAPTER_DD + DEV_INITCOMPLETE + DEV_IOCTL2 ;+ DEV_SAVERESTORE

_DiskDDHeader2	DD	-1
		DW	DEVLEV_3 + DEV_CHAR_DEV + DEV_30 + DEV_SHARE
		DW	_MPIfStr1
		DW	0
		DB	"ATAMIF$ "
		DW	0, 0, 0, 0
		DD	0

DDHeader	ENDS


LIBDATA 	SEGMENT DWORD PUBLIC 'DATA'
LIBDATA 	ENDS


		PUBLIC	_Delay500

_DATA		SEGMENT DWORD PUBLIC 'DATA'
_Delay500	DW	DOSIODELAYCNT

if AUTOMOUNT
		PUBLIC	_SelSAS
_SelSAS 	DW	SAS_SEL
endif

EXTRN		_IHdr:WORD
EXTRN		_MemTop:WORD
EXTRN		_OemHlpIDC:WORD
EXTRN		_ACPIIDC:WORD

_DATA		ENDS

_BSS		SEGMENT DWORD PUBLIC 'BSS'
_BSS		ENDS

_TEXT		SEGMENT DWORD PUBLIC 'CODE'
		ASSUME CS:_TEXT, DS:_DATA

		EXTRN  _S506Str1:NEAR
		EXTRN  _MPIfIOCtl:NEAR
		EXTRN  @CardInsertionDeferred:NEAR

		.486p

		PUBLIC	@saveXCHG
@saveXCHG	PROC	NEAR
		XCHG	[BX], AX
		RET
@saveXCHG	ENDP

		PUBLIC	@saveGET32
@saveGET32	PROC	NEAR
		MOV	EAX, [BX]
		SHLD	EDX, EAX, 16
		RET
@saveGET32	ENDP

		PUBLIC	@saveCLR32
@saveCLR32	PROC	NEAR
		XOR	EAX, EAX
		XCHG	EAX, [BX]
		RET
@saveCLR32	ENDP

		PUBLIC	@saveDEC
@saveDEC	PROC	NEAR
		LOCK DEC BYTE PTR [BX]
		SETNZ	AL
		RET
@saveDEC	ENDP

		PUBLIC	@saveINC
@saveINC	PROC	NEAR
		MOV	AL, 1
		LOCK XADD BYTE PTR [BX], AL
		RET
@saveINC	ENDP

		PUBLIC @FixedIRQ0
@FixedIRQ0	PROC	NEAR
		MOV	BL, 0 * 6	; 2 bytes
		DB	0BEh		; MOV SI, imm16 = 3 bytes
@FixedIRQ0	ENDP

		PUBLIC @FixedIRQ1
@FixedIRQ1	PROC	NEAR
		MOV	BL, 1 * 6	; 2 bytes
		DB	0BEh		; MOV SI, imm16 = 3 bytes
@FixedIRQ1	ENDP

		PUBLIC @FixedIRQ2
@FixedIRQ2	PROC	NEAR
		MOV	BL, 2 * 6	; 2 bytes
		DB	0BEh		; MOV SI, imm16 = 3 bytes
@FixedIRQ2	ENDP

		PUBLIC @FixedIRQ3
@FixedIRQ3	PROC	NEAR
		MOV	BL, 3 * 6	; 2 bytes
		DB	0BEh		; MOV SI, imm16 = 3 bytes
@FixedIRQ3	ENDP

		PUBLIC @FixedIRQ4
@FixedIRQ4	PROC	NEAR
		MOV	BL, 4 * 6	; 2 bytes
		DB	0BEh		; MOV SI, imm16 = 3 bytes
@FixedIRQ4	ENDP

		PUBLIC @FixedIRQ5
@FixedIRQ5	PROC	NEAR
		MOV	BL, 5 * 6	; 2 bytes
		DB	0BEh		; MOV SI, imm16 = 3 bytes
@FixedIRQ5	ENDP

		PUBLIC @FixedIRQ6
@FixedIRQ6	PROC	NEAR
		MOV	BL, 6 * 6	; 2 bytes
		DB	0BEh		; MOV SI, imm16 = 3 bytes
@FixedIRQ6	ENDP

		PUBLIC @FixedIRQ7
@FixedIRQ7	PROC	NEAR
		MOV	BL, 7 * 6	; 2 bytes
@FixedIRQ7	ENDP

		PUBLIC @HandleInterrupts
@HandleInterrupts PROC FAR
		MOV	GS, CS:[_FlatSel]
		MOV	BH, 0
		XOR	DI, DI
		MOV	SI, [_IHdr+BX]
__LOOP:
		OR	SI, SI
		JZ	__EXIT
		LOCK INC BYTE PTR [SI+2]
		MOV	BX, SI
		CALL	WORD PTR [SI+4]
		OR	DI, AX
__NEXT:
		LOCK DEC BYTE PTR [SI+2]
		MOV	SI, WORD PTR [SI]
		JMP	__LOOP
__EXIT:
		SHR	DI, 1
		CMC
		RET
@HandleInterrupts ENDP

		PUBLIC	_CSHookHandler
_CSHookHandler	PROC FAR

		PUSH GS
		PUSH ES
		PUSH DS
		PUSHAD

		MOV	DS, CS:[_DSSel]
		MOV	GS, CS:[_FlatSel]
		MOV	BX, AX
		CALL	@CardInsertionDeferred

		POPAD
		POP  DS
		POP  ES
		POP  GS
		RET

_CSHookHandler	ENDP

		PUBLIC	_LoadFlatGS
_LoadFlatGS	PROC	NEAR
		MOV	GS, CS:[_FlatSel]
		RET
_LoadFlatGS	ENDP

		.286p

_MPIfStr1	PROC	FAR
		MOV	AL, ES:[BX + 2] ; command
		MOV	WORD PTR ES:[BX + 3], 8103h    ; error unknown command
		CMP	AL, CMDInit
		JE	Short _MPInit
		CMP	AL, CMDInitBase
		JE	Short _MPInit
		CMP	AL, CMDINPUT
		JE	Short _MPReadWrite
		CMP	AL, CMDOUTPUT
		JE	Short _MPReadWrite
		CMP	AL, CMDOpen
		JE	Short _MPDone
		CMP	AL, CMDClose
		JE	Short _MPDone
		CMP	AL, CMDGenIOCTL
		JNE	Short _MPRet
		CMP	BYTE PTR ES:[BX + 13], 0F0h    ; category ATAMIF
		JNE	Short _MPRet
		PUSH	ES
		PUSH	BX
		CALL	_MPIfIOCtl
		POP	BX
		POP	ES
		RET
_MPReadWrite:
		MOV	WORD PTR ES:[BX +18], 0000h    ; count = 0
_MPDone:
		MOV	WORD PTR ES:[BX + 3], 0100h    ; done
_MPRet:
		RET
_MPInit:
		MOV	DX, OFFSET __CodeEnd
		MOV	AX, [_MemTop]
		SUB	AX, 1
		SBB	CX, CX
		NOT	CX
		AND	DX, CX
		AND	AX, CX
		MOV	WORD PTR ES:[BX +14], DX
		MOV	WORD PTR ES:[BX +16], AX
		JCXZ	Short _MPFailSilent
		JMP	_MPDone
_MPFailSilent:
		MOV	WORD PTR ES:[BX + 3], 8115h
		RET

_MPIfStr1	ENDP

_TEXT		ENDS

		PUBLIC	_DSSel
		PUBLIC	_CSSel
		PUBLIC	_FlatSel

Code		SEGMENT DWORD PUBLIC 'CODE'
		.386p
		MOV	EBP, [ESP]	; retrieve RetIP
		INT 3			; honey pot trap
		INT 3
		INT 3

		.286p
_DSSel		DW	SEG _DATA
_CSSel		DW	SEG _TEXT
_FlatSel	DW	DOS32FLATDS
Code		ENDS

LIBCODE 	SEGMENT DWORD PUBLIC 'CODE'
LIBCODE 	ENDS

RMCode		SEGMENT DWORD PUBLIC 'CODE'
		.386p
		MOV	EBP, [ESP]	; retrieve RetIP
		INT 3			; honey pot trap
		INT 3
		INT 3

		.286p
RMCode		ENDS

CSCode		SEGMENT DWORD PUBLIC 'CODE'
CSCode		ENDS

FCode		SEGMENT DWORD PUBLIC 'CODE'
		EXTRN	_VDMInt13CallBack:NEAR
FCode		ENDS

InitCode	SEGMENT DWORD PUBLIC 'CODE'
InitCode	ENDS

EndCode 	SEGMENT BYTE PUBLIC 'CODE'

		ASSUME	CS:EndCode
		PUBLIC	__CodeEnd
__CodeEnd:
EndCode 	ENDS


		PUBLIC _MsgBuffer
		PUBLIC _MsgBufferEnd
Messages	SEGMENT DWORD PUBLIC 'MSGS'
_MsgBuffer	DB	65535 DUP (?)
_MsgBufferEnd	LABEL	BYTE
Messages	ENDS

DGROUP		GROUP	_BSS, DDHeader, LIBDATA, _DATA
StaticGroup	GROUP	Code, LIBCODE, _TEXT, InitCode, EndCode
StaticGroup2	GROUP	RMCode, CSCode, FCode

IDCTable	struc
  reserved	DW	3 DUP (?)
  Entry 	DD	FAR PTR ?
  DSeg		DW	?
IDCTable	ends

		.386p
FCode		SEGMENT

		ASSUME	CS:FCode,DS:DGROUP

		PUBLIC _StubVDMInt13CallBack
_StubVDMInt13CallBack PROC NEAR

		PUSH  ES
		CALL  _VDMInt13CallBack
		POP   ES

		DB     66h
		RETF
_StubVDMInt13CallBack ENDP

		public	@CallOEMHlp
@CallOEMHlp	PROC	FAR

		PUSH	BP
		MOV	BP, SP
		PUSH	SI
		PUSH	DI

		LES	BX, DWORD PTR [BP+6]
		TEST	WORD PTR [_OemHlpIDC.Entry+2], -1
		JNZ	DoOEMHlp

		MOV	AX, 8100h
		MOV	WORD PTR ES:[BX+3], AX
		JMP	SHORT CallOEMHlpEnd
DoOEMHlp:
		PUSH	[_OemHlpIDC.Entry]
		PUSH	DS
		MOV	DS, [_OemHlpIDC.DSeg]
		CALL	DWORD PTR [BP-8]
		POP	DS
		ADD	SP, 4
		MOV	AX, WORD PTR ES:[BX+3]
CallOEMHlpEnd:
		AND	AX, 8000h
		POP	DI
		POP	SI
		LEAVE
		RET	4

@CallOEMHlp	ENDP

		public	@CallAcpiCA
@CallAcpiCA	PROC	FAR

		PUSH	BP
		MOV	BP, SP
		PUSH	SI
		PUSH	DI
		PUSH	GS

		LES	BX, DWORD PTR [BP+6]
		TEST	WORD PTR [_ACPIIDC.Entry+2], -1
		JNZ	DoAcpiCA

		MOV	AX, 8100h
		MOV	WORD PTR ES:[BX+3], AX
		JMP	SHORT CallAcpiCAEnd
DoAcpiCA:
		PUSH	[_ACPIIDC.Entry]
		PUSH	DS
		MOV	DS, [_ACPIIDC.DSeg]
		CALL	DWORD PTR [BP-8]
		POP	DS
		ADD	SP, 4
		MOV	AX, WORD PTR ES:[BX+3]
CallAcpiCAEnd:
		AND	AX, 8000h
		POP	GS
		POP	DI
		POP	SI
		LEAVE
		RET	4

@CallAcpiCA	ENDP

FCode		ENDS

		END
