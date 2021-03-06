;**************************************************************************
;*
;* SOURCE FILE NAME = S506IO.asm
;*
;* DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
;*
;* Copyright : COPYRIGHT Daniela Engert 1999-2007
;*
;* DESCRIPTION : IO routines
;*
;* Each IO function has an 32bit IO port identifier (IOPI) as parameter
;* 'Location' which describes the actual port location:
;*
;*   1) IO space:   0x0000pppp	pppp = 16 bit IO port address
;*   2) MEM space:  0xnmmmmmmm	(n > 2) 32 bit flat kernel space address
;*   3) PCI config: 0x10ccbbdf	cc = 8 (possibly 12) bit register offset,
;*				bb = 8 bit PCI bus
;*				d  = 5 bit PCI device
;*				f  = 3 bit PCI function
;*   4) indexed IO: 0x2iiipppp	pppp = 16 bit IO port address (index port)
;*				 iii = 12 bit index value
;*				       data port = idx port + 4
;*
;****************************************************************************/
		JUMPS
		SMART
		LOCALS

		EXTRN	DOSIODELAYCNT:ABS

		.386p

_TEXT		SEGMENT DWORD USE16 PUBLIC 'CODE'
		ASSUME CS:_TEXT; GS:KernelFlatDS

Argument	EQU	ESP + 4
CFGAddr 	=	0CF8h
CFGData 	=	0CFCh

PreparePCICfg	MACRO
		PUSHF
		CLI
		CALL	PCIPrep
		ENDM

; VOID FAR _fastcall OutB (ULONG Location, UCHAR Value)
; VOID FAR _fastcall OutB2 (USHORT Addr1, USHORT Addr2, UCHAR Value);
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

; VOID FAR _fastcall OutBdms (ULONG Location, UCHAR Value)
;	Location: register dx:ax
;	Value:	  register bx
;	Delay:	  1 �s

		PUBLIC	@OutBdms
@OutBdms	PROC FAR
		CALL	@OutB
		MOV	AX, DOSIODELAYCNT
		SHL	AX, 1
OutBdmsLoop:	DEC	AX
		JNZ	OutBdmsLoop
		RET
@OutBdms	ENDP


; VOID FAR _fastcall OutBd (ULONG Location, UCHAR Value, USHORT Delay)
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

; VOID FAR _fastcall OutW (ULONG Location, USHORT Value)
; VOID FAR _fastcall OutW2 (USHORT Addr1, USHORT Addr2, USHORT Value);
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

; VOID FAR _fastcall OutD (ULONG Location, ULONG Value)
; VOID FAR _fastcall OutD2 (USHORT Addr1, USHORT Addr2, ULONG Value);
;	Location: register dx:ax
;	Value:	  [+ 4]

		PUBLIC	@OutD
		PUBLIC	@OutD2
@OutD		PROC FAR
@OutD2:
		MOVZX	ESP, SP
		MOV	EBX, DWORD PTR [Argument]

		TEST	DH, 0C0h	; any bit set means MEMORY
		XCHG	AX, DX		; AX = hi/arg2, DX = lo/arg1
		JZ	@@IOorPCI
		SHL	EDX, 16
		SHLD	EAX, EDX, 16	; EAX = linear address now
		MOV	DWORD PTR GS:[EAX], EBX

		RET	4
@@IOorPCI:
		TEST	AH, 30h 	; bit set means PCI or Indexed
		JNZ	@@PCIorIdx
@@IO:
		MOV	EAX, EBX
		OUT	DX, EAX

		RET	4
@@PCIorIdx:
		TEST	AH, 10h 	; bit set means PCI
		JNZ	@@PCI
		AND	AX, 0FFFh
		MOVZX	EAX, AX 	; AX = index, DX = Port
		OUT	DX, EAX
		ADD	DX, 4
		JMP	@@IO
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

; UCHAR FAR _fastcall InB (ULONG Location)
; UCHAR FAR _fastcall InB2 (USHORT Addr1, USHORT Addr2);
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

; UCHAR FAR _fastcall InBdms (ULONG Location)
;	Location: register dx:ax
;	Delay:	  1 �s

		PUBLIC	@InBdms
@InBdms 	PROC FAR
		MOV	BX, DOSIODELAYCNT
		SHL	BX, 1
@InBdms 	ENDP

; UCHAR FAR _fastcall InBd (ULONG Location, USHORT Delay)
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

; USHORT FAR _fastcall InW (ULONG Location)
; USHORT FAR _fastcall InW2 (USHORT Addr1, USHORT Addr2);
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

; USHORT FAR _fastcall InWdms (ULONG Location)
;	Location: register dx:ax
;	Delay:	  1 �s

		PUBLIC	@InWdms
@InWdms 	PROC FAR
		MOV	BX, DOSIODELAYCNT
		SHL	BX, 1
@InWdms 	ENDP

; UCHAR FAR _fastcall InWd (ULONG Location, USHORT Delay)
;	Location: register dx:ax
;	Delay:	  register bx

		PUBLIC	@InWd
@InWd		PROC FAR
		PUSH	BX
		CALL	@InW
		JMP	InBdelay
@InWd		ENDP

; ULONG FAR _fastcall InD (ULONG Location)
; ULONG FAR _fastcall InD2 (USHORT Addr1, USHORT Addr2);
;	Location: register dx:ax

		PUBLIC	@InD
		PUBLIC	@InD2
@InD		PROC FAR
@InD2:
		TEST	DH, 0C0h	; any bit set means MEMORY
		XCHG	AX, DX		; AX = hi/arg2, DX = lo/arg1
		JZ	@@IOorPCI
		SHL	EDX, 16
		SHLD	EAX, EDX, 16	; EAX = linear address now
		MOV	EAX, DWORD PTR GS:[EAX]
		SHLD	EDX, EAX, 16

		RET
@@IOorPCI:
		TEST	AH, 30h 	; bit set means PCI or Indexed
		JNZ	@@PCIorIdx
@@IO:
		IN	EAX, DX
		SHLD	EDX, EAX, 16

		RET
@@PCIorIdx:
		TEST	AH, 10h 	; bit set means PCI
		JNZ	@@PCI
		AND	AX, 0FFFh
		MOVZX	EAX, AX 	; AX = index, DX = Port
		OUT	DX, EAX
		ADD	DX, 4
		JMP	@@IO
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

; USHORT FAR _fastcall InDdms (ULONG Location)
;	Location: register dx:ax
;	Delay:	  1 �s

		PUBLIC	@InDdms
@InDdms 	PROC FAR
		MOV	BX, DOSIODELAYCNT
		SHL	BX, 1
@InDdms 	ENDP

; UCHAR FAR _fastcall InDd (ULONG Location, USHORT Delay)
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

