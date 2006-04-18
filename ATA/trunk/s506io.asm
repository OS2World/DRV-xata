;**************************************************************************
;*
;* SOURCE FILE NAME = S506IO.asm
;*
;* DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
;*
;* Copyright : COPYRIGHT Daniela Engert 1999-2006
;*
;* DESCRIPTION : IO routines
;*
;* Each IO function has an 32bit IO port identifier (IOPI) as parameter
;* 'Location' which describes the actual port location:
;*
;*   1) IO space:   0x0000pppp	pppp = 16 bit IO port address
;*   2) MEM space:  0xnmmmmmmm	(n >= 2) 32 bit flat kernel space address
;*   3) PCI config: 0x10ccbbdf	cc = 8 bit register offset,
;*				bb = 8 bit PCI bus
;*				d  = 5 bit PCI device
;*				f  = 3 bit PCI function
;*
;****************************************************************************/
		JUMPS
		SMART
		LOCALS

		EXTRN	DOSIODELAYCNT:ABS

		.386p

_TEXT		SEGMENT DWORD USE16 PUBLIC 'CODE'
		ASSUME CS:_TEXT

Argument	EQU	ESP + 4
CFGAddr 	=	0CF8h
CFGData 	=	0CFCh

PreparePCICfg	MACRO
		PUSHF
		CLI
		CALL	PCIPrep
		ENDM

; VOID FAR OutB (ULONG Location, UCHAR Value)
;	Location: register dx:ax
;	Value:	  register bx

		PUBLIC	@OutB
		PUBLIC	@OutB2
@OutB		PROC FAR
@OutB2:
		TEST	DH, 0E0h	; any bit set means MEMORY
		XCHG	AX, DX		; AX = hi/arg2, DX = lo/arg1
		JZ	@@IOorPCI
		SHL	EDX, 16
		SHLD	EAX, EDX, 16	; EAX = linear address now
		MOV	BYTE PTR GS:[EAX], BL

		RET
@@IOorPCI:
		TEST	AH, 10h 	; bit set means PCI
		JNZ	@@PCI
		MOV	AL, BL
		OUT	DX, AL

		RET
@@PCI:					; AL = reg, DX = bus:8/dev:5/fnc:3
		PreparePCICfg
		OUT	DX, AL
		XOR	EAX, EAX
		SUB	DL, CL
		OUT	DX, EAX
		POPF
		RET

@OutB		ENDP

; VOID FAR OutBdms (ULONG Location, UCHAR Value)
;	Location: register dx:ax
;	Value:	  register bx
;	Delay:	  1 æs

		PUBLIC	@OutBdms
@OutBdms	PROC FAR
		CALL	@OutB
		MOV	AX, DOSIODELAYCNT
		SHL	AX, 1
OutBdmsLoop:	DEC	AX
		JNZ	OutBdmsLoop
		RET
@OutBdms	ENDP


; VOID FAR OutBd (ULONG Location, UCHAR Value, USHORT Delay)
;	Location: register dx:ax
;	Value:	  register bx
;	Delay:	  [+ 4]

		PUBLIC	@OutBd
@OutBd		PROC FAR
		CALL	@OutB
		MOVZX	ESP, SP
		MOV	AX, WORD PTR [Argument]
		INC	AX
OutBdLoop:	DEC	AX
		JNZ	OutBdLoop
		RET	2
@OutBd		ENDP

; VOID FAR OutW (ULONG Location, USHORT Value)
;	Location: register dx:ax
;	Value:	  register bx

		PUBLIC	@OutW
		PUBLIC	@OutW2
@OutW		PROC FAR
@OutW2:
		TEST	DH, 0E0h	; any bit set means MEMORY
		XCHG	AX, DX		; AX = hi/arg2, DX = lo/arg1
		JZ	@@IOorPCI
		SHL	EDX, 16
		SHLD	EAX, EDX, 16	; EAX = linear address now
		MOV	WORD PTR GS:[EAX], BX

		RET
@@IOorPCI:
		TEST	AH, 10h 	; bit set means PCI
		JNZ	@@PCI
		MOV	AX, BX
		OUT	DX, AX

		RET
@@PCI:					; AL = reg, DX = bus:8/dev:5/fnc:3
		PreparePCICfg
		OUT	DX, AX
		XOR	EAX, EAX
		SUB	DL, CL
		OUT	DX, EAX
		POPF
		RET

@OutW		ENDP

; VOID FAR OutD (ULONG Location, ULONG Value)
;	Location: register dx:ax
;	Value:	  [+ 4]

		PUBLIC	@OutD
		PUBLIC	@OutD2
@OutD		PROC FAR
@OutD2:
		MOVZX	ESP, SP
		MOV	EBX, DWORD PTR [Argument]

		TEST	DH, 0E0h	; any bit set means MEMORY
		XCHG	AX, DX		; AX = hi/arg2, DX = lo/arg1
		JZ	@@IOorPCI
		SHL	EDX, 16
		SHLD	EAX, EDX, 16	; EAX = linear address now
		MOV	DWORD PTR GS:[EAX], EBX

		RET	4
@@IOorPCI:
		TEST	AH, 10h 	; bit set means PCI
		JNZ	@@PCI
		MOV	EAX, EBX
		OUT	DX, EAX

		RET	4
@@PCI:					; AL = reg, DX = bus:8/dev:5/fnc:3
		PreparePCICfg
		OUT	DX, EAX
		XOR	EAX, EAX
		SUB	DL, CL
		OUT	DX, EAX
		POPF
		RET	4

@OutD		ENDP

PCIPrep 	PROC	NEAR		; AL = reg, DX = bus:8/dev:5/fnc:3
		MOV	AH, AL
		MOV	AL, 80h
		SHL	EAX, 16
		SHLD	EDX, EAX, 16
		ROR	EDX, 8		; EDX = 80/bus:8/dev:5/fnc:3/reg:8
		MOV	EAX, EDX
		MOV	CL, DL
		AND	CL, 3
		ADD	CL, CFGData - CFGAddr
		AND	AL, NOT 3
		MOV	DX, CFGAddr
		OUT	DX, EAX
		ADD	DL, CL
		MOV	EAX, EBX
		RET
PCIPrep 	ENDP

; UCHAR FAR InB (ULONG Location)
;	Location: register dx:ax

		PUBLIC	@InB
		PUBLIC	@InB2
@InB		PROC FAR
@InB2:
		TEST	DH, 0E0h	; any bit set means MEMORY
		XCHG	AX, DX		; AX = hi/arg2, DX = lo/arg1
		JZ	@@IOorPCI
		SHL	EDX, 16
		SHLD	EAX, EDX, 16	; EAX = linear address now
		MOV	AL, BYTE PTR GS:[EAX]

		RET
@@IOorPCI:
		TEST	AH, 10h 	; bit set means PCI
		JNZ	@@PCI
		IN	AL, DX

		RET
@@PCI:					; AL = reg, DX = bus:8/dev:5/fnc:3
		PreparePCICfg
		IN	AL, DX
		XCHG	AX, BX
		XOR	EAX, EAX
		SUB	DL, CL
		OUT	DX, EAX
		POPF
		XCHG	AX, BX

		RET

@InB		ENDP

; UCHAR FAR InBdms (ULONG Location)
;	Location: register dx:ax
;	Delay:	  1 æs

		PUBLIC	@InBdms
@InBdms 	PROC FAR
		MOV	BX, DOSIODELAYCNT
		SHL	BX, 1
@InBdms 	ENDP

; UCHAR FAR InBd (ULONG Location, USHORT Delay)
;	Location: register dx:ax
;	Delay:	  register bx

		PUBLIC	@InBd
@InBd		PROC FAR
		PUSH	BX
		CALL	@InB
InBdelay:	POP	BX
		INC	BX
InBdLoop:	DEC	BX
		JNZ	InBdLoop
		RET
@InBd		ENDP

; USHORT FAR InW (ULONG Location)
;	Location: register dx:ax

		PUBLIC	@InW
		PUBLIC	@InW2
@InW		PROC FAR
@InW2:
		TEST	DH, 0E0h	; any bit set means MEMORY
		XCHG	AX, DX		; AX = hi/arg2, DX = lo/arg1
		JZ	@@IOorPCI
		SHL	EDX, 16
		SHLD	EAX, EDX, 16	; EAX = linear address now
		MOV	AX, WORD PTR GS:[EAX]

		RET
@@IOorPCI:
		TEST	AH, 10h 	; bit set means PCI
		JNZ	@@PCI
		IN	AX, DX

		RET
@@PCI:					; AL = reg, DX = bus:8/dev:5/fnc:3
		PreparePCICfg
		IN	AX, DX
		XCHG	AX, BX
		XOR	EAX, EAX
		SUB	DL, CL
		OUT	DX, EAX
		POPF
		XCHG	AX, BX

		RET

@InW		ENDP

; USHORT FAR InWdms (ULONG Location)
;	Location: register dx:ax
;	Delay:	  1 æs

		PUBLIC	@InWdms
@InWdms 	PROC FAR
		MOV	BX, DOSIODELAYCNT
		SHL	BX, 1
@InWdms 	ENDP

; UCHAR FAR InWd (ULONG Location, USHORT Delay)
;	Location: register dx:ax
;	Delay:	  register bx

		PUBLIC	@InWd
@InWd		PROC FAR
		PUSH	BX
		CALL	@InW
		JMP	InBdelay
@InWd		ENDP

; ULONG FAR InD (ULONG Location)
;	Location: register dx:ax

		PUBLIC	@InD
		PUBLIC	@InD2
@InD		PROC FAR
@InD2:
		TEST	DH, 0E0h	; any bit set means MEMORY
		XCHG	AX, DX		; AX = hi/arg2, DX = lo/arg1
		JZ	@@IOorPCI
		SHL	EDX, 16
		SHLD	EAX, EDX, 16	; EAX = linear address now
		MOV	EAX, DWORD PTR GS:[EAX]
		SHLD	EDX, EAX, 16

		RET
@@IOorPCI:
		TEST	AH, 10h 	; bit set means PCI
		JNZ	@@PCI
		IN	EAX, DX
		SHLD	EDX, EAX, 16

		RET
@@PCI:					; AL = reg, DX = bus:8/dev:5/fnc:3
		PreparePCICfg
		IN	EAX, DX
		XCHG	EAX, EBX
		XOR	EAX, EAX
		SUB	DL, CL
		OUT	DX, EAX
		POPF
		XCHG	EAX, EBX
		SHLD	EDX, EAX, 16

		RET

@InD		ENDP

; USHORT FAR InDdms (ULONG Location)
;	Location: register dx:ax
;	Delay:	  1 æs

		PUBLIC	@InDdms
@InDdms 	PROC FAR
		MOV	BX, DOSIODELAYCNT
		SHL	BX, 1
@InDdms 	ENDP

; UCHAR FAR InDd (ULONG Location, USHORT Delay)
;	Location: register dx:ax
;	Delay:	  register bx

		PUBLIC	@InDd
@InDd		PROC FAR
		PUSH	BX
		CALL	@InD
		JMP	InBdelay
@InDd		ENDP

_TEXT		ENDS
		END

