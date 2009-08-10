;**************************************************************************
;*
;* SOURCE FILE NAME = S506MATH.asm
;*
;* DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
;*
;* Copyright : COPYRIGHT Daniela Engert 1999-2009
;* distributed under the terms of the GNU Lesser General Public License
;*
;* DESCRIPTION : Math routines
;****************************************************************************/

		.386p

_TEXT		SEGMENT DWORD USE16 PUBLIC 'CODE'
		ASSUME CS:_TEXT

		PUBLIC	__aNlshl

__aNlshl	PROC	NEAR
		XOR	CH, CH
		JCXZ	ENlshl
LNlshl: 	SHL	AX, 1
		RCL	DX, 1
		LOOP	LNlshl
ENlshl: 	RET
__aNlshl	ENDP

		PUBLIC	__aFlshl

__aFlshl	PROC	FAR
		XOR	CH, CH
		JCXZ	EFlshl
LFlshl: 	SHL	AX, 1
		RCL	DX, 1
		LOOP	LFlshl
EFlshl: 	RET
__aFlshl	ENDP

		PUBLIC	__aNulshr

__aNulshr	PROC	NEAR
		XOR	CH, CH
		JCXZ	ENulshr
LNulshr:	SHR	DX, 1
		RCR	AX, 1
		LOOP	LNulshr
ENulshr:	RET
__aNulshr	ENDP

		PUBLIC	__aFulshr

__aFulshr	PROC	FAR
		XOR	CH, CH
		JCXZ	EFulshr
LFulshr:	SHR	DX, 1
		RCR	AX, 1
		LOOP	LFulshr
EFulshr:	RET
__aFulshr	ENDP

		PUBLIC	__aNulmul
		PUBLIC	__aNlmul
__aNulmul	PROC	NEAR
__aNlmul:	PUSH	BP
		MOV	BP, SP
		MOV	EAX, DWord Ptr [BP+4]
		MUL	DWord Ptr [BP+8]
		SHLD	EDX, EAX, 16
		LEAVE
		RET	8
__aNulmul	ENDP

		PUBLIC	__aNNaulmul
		PUBLIC	__aNNalmul
__aNNaulmul	PROC	NEAR
__aNNalmul:	PUSH	BP
		MOV	BP, SP
		MOV	BX, Word Ptr [BP+4]
		MOV	EAX, DWord Ptr [BP+6]
		MUL	DWord Ptr [BX]
		MOV	DWord Ptr [BX], EAX
		SHLD	EDX, EAX, 16
		LEAVE
		RET	6
__aNNaulmul	ENDP

		PUBLIC	__aFulmul
		PUBLIC	__aFlmul
__aFulmul	PROC	FAR
__aFlmul:	PUSH	BP
		MOV	BP, SP
		MOV	EAX, DWord Ptr [BP+6]
		MUL	DWord Ptr [BP+10]
		SHLD	EDX, EAX, 16
		LEAVE
		RET	8
__aFulmul	ENDP

		PUBLIC	__aNuldiv
__aNuldiv	PROC	NEAR
		PUSH	BP
		MOV	BP, SP
		XOR	EDX, EDX
		MOV	EAX, DWord Ptr [BP+4]
		DIV	DWord Ptr [BP+8]
		SHLD	EDX, EAX, 16
		LEAVE
		RET	8
__aNuldiv	ENDP

		PUBLIC	__aFuldiv
__aFuldiv	PROC	FAR
		PUSH	BP
		MOV	BP, SP
		XOR	EDX, EDX
		MOV	EAX, DWord Ptr [BP+6]
		DIV	DWord Ptr [BP+10]
		SHLD	EDX, EAX, 16
		LEAVE
		RET	8
__aFuldiv	ENDP

		PUBLIC	__aNulrem
__aNulrem	PROC	NEAR
		PUSH	BP
		MOV	BP, SP
		XOR	EDX, EDX
		MOV	EAX, DWord Ptr [BP+4]
		DIV	DWord Ptr [BP+8]
		XCHG	EAX, EDX
		SHLD	EDX, EAX, 16
		LEAVE
		RET	8
__aNulrem	ENDP

		PUBLIC	__aFulrem
__aFulrem	PROC	FAR
		PUSH	BP
		MOV	BP, SP
		XOR	EDX, EDX
		MOV	EAX, DWord Ptr [BP+6]
		DIV	DWord Ptr [BP+10]
		XCHG	EAX, EDX
		SHLD	EDX, EAX, 16
		LEAVE
		RET	8
__aFulrem	ENDP

		PUBLIC	_CalculateLoss
_CalculateLoss	PROC	FAR
		PUSH	BP
		MOV	BP, SP
		MOV	EAX, DWord Ptr [BP+6]	; Physical Total
		SUB	EAX, DWord Ptr [BP+10]	; Logical Total
		MOV	EDX, 10000
		JZ	_CalculateLssE
		MUL	EDX
		DIV	DWord Ptr [BP+6]
		MOV	DX, 9999
		SUB	DX, AX
_CalculateLssE: XCHG	AX, DX
		LEAVE
		RET
_CalculateLoss	ENDP

_TEXT		ENDS
		END
