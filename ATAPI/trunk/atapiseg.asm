;**************************************************************************
;*
;* SOURCE FILE NAME = ATAPIseg.asm
;*
;* DESCRIPTIVE NAME = DaniATAPI.FLT - Filter Driver for ATAPI
;*
;* Copyright : COPYRIGHT IBM CORPORATION, 1993, 1994
;*	       LICENSED MATERIAL - PROGRAM PROPERTY OF IBM
;*	       COPYRIGHT Daniela Engert 1999-2006
;****************************************************************************

	include devhdr.inc

EXTRN	DOSIODELAYCNT:ABS

;/*-------------------------------------*/
;/* Assembler Helper to order segments	*/
;/*-------------------------------------*/

DDHeader	SEGMENT DWORD PUBLIC 'DATA'

		PUBLIC _DiskDDHeader

_DiskDDHeader	DD	-1
		DW	DEVLEV_3 + DEV_CHAR_DEV + DEV_30
		DW	_IDECDStr
		DW	0
		DB	"ATAPI$  "
		DW	0, 0, 0, 0
		DD	DEV_ADAPTER_DD + DEV_INITCOMPLETE + DEV_IOCTL2 ;+ DEV_SAVERESTORE

DDHeader	ENDS

LIBDATA 	SEGMENT DWORD PUBLIC 'DATA'
LIBDATA 	ENDS

		PUBLIC _Delay500
_DATA		SEGMENT DWORD PUBLIC 'DATA'
_Delay500	DW	DOSIODELAYCNT
_DATA		ENDS

_BSS		SEGMENT DWORD PUBLIC 'BSS'
_BSS		ENDS

_TEXT		SEGMENT DWORD PUBLIC 'CODE'
		EXTRN	_IDECDStr:near
_TEXT		ENDS

		PUBLIC _DSSel
		PUBLIC _CSSel
Code		SEGMENT DWORD PUBLIC 'CODE'
		ASSUME	CS:Code

		.486p
		MOV	BP, SP
		MOV	EBP, [BP]	; retrieve RetIP
		INT 3			; honey pot trap

_DSSel		DW	SEG _DATA
_CSSel		DW	SEG _TEXT

		PUBLIC	@saveXCHG
@saveXCHG	PROC	NEAR
		XCHG	[BX], AX
		RET
@saveXCHG	ENDP

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

		.286p
Code		ENDS

LIBCODE 	SEGMENT DWORD PUBLIC 'CODE'
LIBCODE 	ENDS

RMCode		SEGMENT DWORD PUBLIC 'CODE'
RMCode		ENDS

EndCode 	SEGMENT BYTE PUBLIC 'CODE'
		ASSUME	CS:EndCode
		PUBLIC	__CodeEnd
__CodeEnd:
EndCode 	ENDS

		PUBLIC _MsgBuffer
		PUBLIC	_MsgBufferEnd
Messages	SEGMENT DWORD PUBLIC 'MSGS'
_MsgBuffer	DB	8191 DUP (?)
_MsgBufferEnd	DB	?
Messages	ENDS

DGROUP		GROUP	_BSS, DDHeader, LIBDATA, _DATA
StaticGroup	GROUP	Code, LIBCODE, _TEXT, RMCode, EndCode

		END
