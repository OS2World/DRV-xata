;**************************************************************************
;*
;* SOURCE FILE NAME = S506PIO.asm
;*
;* DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
;*
;* Copyright : COPYRIGHT Daniela Engert 1999-2009
;* distributed under the terms of the GNU Lesser General Public License
;*
;* DESCRIPTION : PIO core routine
;****************************************************************************/

		JUMPS
		LOCALS

		.386p

		EXTRN	_Device_Help:DWORD
		EXTRN	_Delay500:WORD

_TEXT		SEGMENT DWORD USE16 PUBLIC 'CODE'
		ASSUME CS:_TEXT

SCATGATENTRY	STRUC
  ppXferBuf	DD ?	; + 0	  Physical pointer to transfer buffer
  XferBufLen	DD ?	; + 4	  Length of transfer buffer
SCATGATENTRY	ENDS

ADD_X		STRUC
  TMode 	DW ?	 ; + 0			Direction of xferdata
  cSGList	DW ?	 ; + 2			Count of S/G list elements
  pSGList	DD ?	 ; + 4 PSCATGATENTRY	Far pointer to S/G List
  iPortAddress	DD ?	 ; + 8			I/O Port Address
  numTotalBytes DD ?	 ; + 12 		Total bytes to copy
  iSGList	DW ?	 ; + 16 		Current index of S/G List
  SGOffset	DD ?	 ; + 18 		Current offset
  iSGListStart	DW ?	 ; + 22 		Save area - S/G List Index
  SGOffsetStart DD ?	 ; + 24 		Save area - S/G List Offset
  Selector	DW ?	 ; + 28 		temp selector for PIO
ADD_X		ENDS

		PUBLIC	ADD_XFERIO

npADDX		EQU BP + 4
ByteHeld	EQU BP-1	; 1
ByteMiss	EQU BP-2	; 1
Mode		EQU BP-3	; 1  ; 7/s: 32, 6/z: slow, 2/p: mem, 0/c: out
MapSel		EQU BP-6	; 2
Port		EQU BP-10	; 4
Data		EQU BP-14	; 4
BufRem		EQU BP-18	; 4
ppSrcDst	EQU BP-22	; 4
pSGE		EQU BP-26	; 4
XferLen 	EQU BP-30	; 4
XferRem 	EQU BP-34	; 4
XferCur 	EQU BP-38	; 4

ADD_XFERIO	PROC	NEAR

		ENTER	38, 0
		PUSH	DI
		PUSH	SI

		SUB	AL, AL
		MOV	[ByteHeld], AL
		MOV	[ByteMiss], AL
		MOV	BX, [npADDX]
		MOV	AX, [BX + iSGList]
		SHL	AX, 3
		MOVZX	EAX, AX
		ADD	EAX, [BX + pSGList]
		MOV	[pSGE], EAX
		MOV	AX, [BX + Selector]
		MOV	[MapSel], AX
		MOV	EAX, [BX + iPortAddress]
		MOV	[Port], EAX
		TEST	EAX, 0E0000000h 	; test for MMIO
		SETNZ	AL
		SHL	AL, 2
		OR	AL, BYTE Ptr [BX + TMode]
		MOV	[Mode], AL
		MOV	EAX, [BX + numTotalBytes]
		MOV	[BufRem], EAX
;--------------------------------------------------------------------
OuterLoop:
		MOV	BX, [npADDX]
		MOV	ECX, [BX + SGOffset]
		MOV	EDX, ECX
		LES	BX, [pSGE]
		ADD	ECX, ES:[BX + ppXferBuf]
		MOV	[ppSrcDst], ECX
		MOV	ECX, ES:[BX + XferBufLen]
		SUB	ECX, EDX

		CMP	ECX, EAX
		JBE	L6DE
		MOV	ECX, EAX
L6DE:		MOV	[XferLen], ECX
		MOV	[XferRem], ECX
;--------------------------------------------------------------------
InnerLoop:
		MOV	EAX, 10000h
		CMP	ECX, EAX
		JBE	L702
		MOV	ECX, EAX
L702:		MOV	[XferCur], ECX

		PUSH	DS
		MOV	AX, [ppSrcDst + 2]
		MOV	BX, [ppSrcDst]
		MOV	SI, [MapSel]
		MOV	DL, 2EH		; PhysToGDTSelector
		CALL	[_Device_Help]
		ADD	[ppSrcDst], ECX
		MOV	EDX, [Port]
		CLD

		TEST	Byte Ptr [Mode], 1
		JNZ	Output
;--------------------------------------------------------------------
		MOV	ES, [MapSel]
		XOR	EBX, EBX
		MOV	DI, BX
		XCHG	BL, [ByteHeld]
		AND	BL, 3
		JZ	In0
		SUB	ECX, EBX
		MOV	EAX, [Data]
InLoop0:	STOSB
		SHR	EAX, 8
		DEC	BL
		JNZ	InLoop0

In0:		MOV	AH, [Mode]    ; 7/s: 32, 6/z: slow, 2/p: mem, 0/c: out
		SAHF
		JPO	InP
InM:
		JZ	InMslow
		JS	InM32
;--------------------------------------------------------------------
InM16:
		SHR	ECX, 1
		JZ	InM16LoopEnd
InM16Loop:
		MOV	AX, WORD PTR GS:[EDX]
		STOSW
		LOOP	InM16Loop
InM16LoopEnd:	JNC	InEnd
InM16End:	MOV	AX, WORD PTR GS:[EDX]
		JMP	In16End
;--------------------------------------------------------------------
InMslow:	ROR	ECX, 1
		JCXZ	InMsLoop0End
InMsLoop0:	MOV	AX, WORD PTR GS:[EDX]
		STOSW
		MOV	AX, [_Delay500]
		SHL	AX, 3
InMsLoop1:	DEC	AX
		JNZ	InMsLoop1
		LOOP	InMsLoop0
InMsLoop0End:	RCL	ECX, 1
		JMP	InM16LoopEnd
;--------------------------------------------------------------------
InM32:		ROR	ECX, 2
		JCXZ	InM32LoopEnd
InM32Loop:	MOV	EAX, DWORD PTR GS:[EDX]
		STOSD
		LOOP	InM32Loop
InM32LoopEnd:	ROL	ECX, 2
		AND	CX, 3
		JZ	InEnd
		MOV	EAX, DWORD PTR GS:[EDX]
		JMP	In32End
;--------------------------------------------------------------------
InP:
		JZ	InPslow
		JS	InP32
InP16:
		SHR	ECX, 1
		REP INSW
InP16End:	JNC	InEnd
		IN	AX, DX
In16End:	MOV	BYTE PTR [ByteHeld], 01
		STOSB
		MOV	[Data], AH
		JMP	InEnd
;--------------------------------------------------------------------
InPslow:	ROR	ECX, 1
		JCXZ	InPsLoop0End
InPsLoop0:	INSW
		MOV	AX, [_Delay500]
		SHL	AX, 3
InPsLoop1:	DEC	AX
		JNZ	InPsLoop1
		LOOP	InPsLoop0
InPsLoop0End:	RCL	ECX, 1
		JMP	InP16End
;--------------------------------------------------------------------
InP32:		ROR	ECX, 2
		REP INSD
		ROL	ECX, 2
		AND	CX, 3
		JZ	InEnd
		IN	EAX, DX
;--------------------------------------------------------------------
In32End:
		MOV	BL, CL
		NEG	BL
		AND	BL, 3
		MOV	[ByteHeld], BL
In32Loop:	STOSB
		SHR	EAX, 8
		LOOP	In32Loop
		MOV	[Data], EAX
InEnd:		JMP	EndInnerLoop

;--------------------------------------------------------------------
Output:
		MOV	DS, [MapSel]
		XOR	EBX, EBX
		MOV	SI, BX
		XCHG	BL, [ByteMiss]
		AND	BL, 03
		JZ	Out0
		SUB	ECX, EBX
		MOV	EAX, [Data]
OutLoop0:	LODSB
		ROR	EAX, 8
		DEC	BL
		JNZ	OutLoop0

		MOV	BL, BYTE PTR [Mode]
		TEST	BL, 1 SHL 2
		JZ	OutPa
		TEST	BL, 80h
		JNZ	OutM32a
		JMP	OutM16a

Out0:		MOV	BL, BYTE PTR [Mode]
		TEST	BL, 1 SHL 2
		JZ	OutP
		TEST	BL, 80h
		JNZ	OutM32
		JMP	OutM16
;--------------------------------------------------------------------
OutM16a:	ROR	EAX, 16
		MOV	WORD PTR GS:[EDX], AX
OutM16: 	MOV	DI, CX
		MOV	BX, 1
		SHR	ECX, 1
		JZ	OutM16LoopEnd
OutM16Loop:
		LODSW
		MOV	WORD PTR GS:[EDX], AX
		LOOP	OutM16Loop
OutM16LoopEnd:	MOV	AX, DI
		JMP	OutEnd
;--------------------------------------------------------------------
OutM32a:	MOV	DWORD PTR GS:[EDX], EAX
OutM32: 	MOV	DI, CX
		MOV	BX, 3
		SHR	ECX, 2
		JZ	OutM32LoopEnd
OutM32Loop:
		LODSD
		MOV	DWORD PTR GS:[EDX], EAX
		LOOP	OutM32Loop
OutM32LoopEnd:	MOV	AX, DI
		JMP	OutEnd
;--------------------------------------------------------------------
OutP:		TEST	BL, 80h
		JNZ	OutP32
		JMP	OutP16

OutP16a:	ROR	EAX, 16
		OUT	DX, AX
OutP16: 	MOV	AX, CX
		MOV	BX, 1
		SHR	ECX, 1
		REP OUTSW
		JMP	OutEnd

OutPa:		TEST	BL, 80h
		JZ	OutP16a
;--------------------------------------------------------------------
OutP32a:	OUT	DX, EAX
OutP32: 	MOV	AX, CX
		MOV	BX, 3
		SHR	ECX, 2
		REP OUTSD

OutEnd: 	AND	AX, BX
		MOV	CX, AX
		JCXZ	EndInnerLoop
		NEG	AL
		AND	AL, BL
		MOV	[ByteMiss], AL
OutEndLoop:	LODSB
		ROR	EAX, 8
		LOOP	OutEndLoop
		MOV	[Data], EAX
;--------------------------------------------------------------------
EndInnerLoop:
		POP	DS
		MOV	ECX, [XferRem]
		SUB	ECX, [XferCur]
		MOV	[XferRem], ECX
		JNZ	InnerLoop
;--------------------------------------------------------------------
		MOV	EAX, [XferLen]
		SUB	[BufRem], EAX
		MOV	BX, [npADDX]
		ADD	EAX, [BX + SGOffset]
		LES	SI, [pSGE]
		CMP	ES:[SI + XferBufLen], EAX
		JNE	EndOuterLoop
		INC	WORD PTR [BX + iSGList]
		ADD	WORD PTR [pSGE], SIZE SCATGATENTRY
		SUB	EAX, EAX
EndOuterLoop:
		MOV	[BX + SGOffset], EAX
		MOV	EAX, [BufRem]
		OR	EAX, EAX
		JNZ	OuterLoop
;--------------------------------------------------------------------
		POP	SI
		POP	DI
		LEAVE
		RET	2

ADD_XFERIO	ENDP

_TEXT		ENDS
		END

