;**************************************************************************
;*
;* SOURCE FILE NAME = cs_help.asm
;*
;* DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
;*
;* Copyright : COPYRIGHT Daniela Engert 1999-2009
;* distributed under the terms of the GNU Lesser General Public License
;*
;* DESCRIPTION : card services stubs
;****************************************************************************/

IDCTable	STRUC
  reserved	DW	3 DUP (?)
  Entry 	DD	FAR PTR ?
  DSeg		DW	?
IDCTable	ENDS

IDC_PACKET	STRUC
  function	DB	?
  handle	DW	?
  pointer	DD	FAR PTR ?
  arglength	DW	?
  argpointer	DD	FAR PTR ?
IDC_PACKET	ENDS


_DATA	SEGMENT WORD PUBLIC 'DATA'
EXTRN	_CSIDC:WORD
_DATA	ENDS

_TEXT	SEGMENT WORD PUBLIC 'CODE'
EXTRN	_DSSel:WORD
_TEXT	ENDS

CSCode	SEGMENT WORD PUBLIC 'CODE'
CSCode	ENDS

_TEXT	SEGMENT WORD PUBLIC 'CODE'
	ASSUME CS:_TEXT, DS:_DATA, ES:NOTHING, SS:NOTHING
	.286p

EXTRN	_CSCallbackHandler:NEAR

;/***************************************************************************
;*
;* FUNCTION NAME = CSCallbackStub
;*
;* VOID FAR  _cdecl CSCallbackStub();
;* VOID NEAR _cdecl CSCallbackHandler (USHORT Socket, USHORT Event, PUCHAR Buffer)
;*
;**************************************************************************/

	PUBLIC	_CSCallbackStub

_CSCallbackStub PROC FAR

	PUSHF
	PUSHA
	PUSH	DS

	PUSH	ES		; setup buffer pointer
	PUSH	BX
	SUB	AH, AH
	PUSH	AX		; set up event number,
	PUSH	CX		; socket number and data segment
	MOV	DS, CS:[_DSSel] ; - service the call back
	CALL	_CSCallbackHandler
	ADD	SP, 3*2

	POP	ES
	POP	DS
	POPA
	POPF
	RET

_CSCallbackStub ENDP

_TEXT	ENDS

CSCode	SEGMENT WORD PUBLIC 'CODE'
	ASSUME cs:CSCode, ds:_DATA, es:NOTHING, ss:NOTHING
	.386

;/***************************************************************************
;*
;* FUNCTION NAME = _CallCardServices
;*
;* USHORT FAR _cdecl CallCardServices (IDC_PACKET FAR * IDCPacket)
;*
;**************************************************************************/

	PUBLIC	_CallCardServices

_CallCardServices PROC FAR

	PUSH	BP
	MOV	BP, SP
	PUSH	SI
	PUSH	DI
	PUSH	CX
	PUSH	ES
	PUSH	BX
	PUSH	DS

	LES	BX, DWORD PTR [BP+6]	; set up function argument

	MOV	AL, BYTE PTR ES:[BX+function]
	MOV	AH, 0AFH		; set up static value

	MOV	DX, WORD PTR ES:[BX+handle]

	MOV	DI, WORD PTR ES:[BX+pointer+2]
	MOV	SI, WORD PTR ES:[BX+pointer+0]

	MOV	CX, WORD PTR ES:[BX+arglength]
	LES	BX, DWORD PTR ES:[BX+argpointer]

	PUSH	_CSIDC.Entry
	MOV	DS, _CSIDC.DSeg

	CALL	DWORD PTR SS:[BP-16]	; make the idc request to card services
	POP	BX
	POP	BX
					; error if carry flag set
	JC	SHORT Done		;   - return code in ax

	LES	BX, DWORD PTR [BP+6]	; set up handle
	MOV	WORD PTR ES:[BX+handle], DX

	MOV	AX, 0			; call successful

Done:	POP	DS
	POP	BX
	POP	ES
	POP	CX
	POP	DI
	POP	SI
	MOV	SP, BP
	POP	BP
	RET

_CallCardServices ENDP

CSCode	ENDS

	END
