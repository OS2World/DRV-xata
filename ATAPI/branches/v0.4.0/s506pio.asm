;**************************************************************************
;*
;* SOURCE FILE NAME = S506PIO.asm
;*
;* DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
;*
;* Copyright : COPYRIGHT Daniela Engert 1999-2006
;*
;* DESCRIPTION : PIO core routine
;****************************************************************************/

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
  iPortAddress	DW ?	 ; + 8			I/O Port Address
  numTotalBytes DD ?	 ; + 10 		Total bytes to copy
  iSGList	DW ?	 ; + 14 		Current index of S/G List
  SGOffset	DD ?	 ; + 16 		Current offset
  iSGListStart	DW ?	 ; + 20 		Save area - S/G List Index
  SGOffsetStart DD ?	 ; + 22 		Save area - S/G List Offset
  Selector	DW ?	 ; + 26 		temp selector for PIO
ADD_X		ENDS

		PUBLIC	ADD_XFERIO

npADDX		EQU BP + 4
ByteHeld	EQU BP-1	; 1
ByteMiss	EQU BP-2	; 1
Mode		EQU BP-3	; 1
MapSel		EQU BP-6	; 2
Port		EQU BP-8	; 2
Data		EQU BP-12	; 4
BufRem		EQU BP-16	; 4
ppSrcDst	EQU BP-20	; 4
pSGE		EQU BP-24	; 4
XferLen 	EQU BP-28	; 4
XferRem 	EQU BP-32	; 4
XferCur 	EQU BP-36	; 4

ADD_XFERIO	PROC	NEAR

		ENTER	36, 0
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
		MOV	AX, [BX + iPortAddress]
		MOV	[Port], AX
		MOV	AX, [BX + Selector]
		MOV	[MapSel], AX
		MOV	AL, BYTE Ptr [BX + TMode]
		MOV	[Mode], AL
		MOV	EAX, [BX + numTotalBytes]
		MOV	[BufRem], EAX
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
		JBE	Short L6DE
		MOV	ECX, EAX
L6DE:		MOV	[XferLen], ECX
		MOV	[XferRem], ECX
InnerLoop:
		MOV	EAX, 10000h
		CMP	ECX, EAX
		JBE	Short L702
		MOV	ECX, EAX
L702:		MOV	[XferCur], ECX

		PUSH	DS
		MOV	AX, [ppSrcDst + 2]
		MOV	BX, [ppSrcDst]
		MOV	SI, [MapSel]
		MOV	DL, 2EH
		CALL	[_Device_Help]
		ADD	[ppSrcDst], ECX
		MOV	DX, [Port]
		CLD

		TEST	Byte Ptr [Mode], 1
		JNZ	Output

		MOV	ES, [MapSel]
		XOR	EBX, EBX
		MOV	DI, BX
		XCHG	BL, [ByteHeld]
		AND	BL, 3
		JZ	Short L74D
		SUB	ECX, EBX
		MOV	EAX, [Data]
L744:		STOSB
		SHR	EAX, 8
		DEC	BL
		JNZ	L744
L74D:		MOV	AH, [Mode]
		SAHF
		JZ	Short L768
		JS	Short L77C
		SHR	ECX, 1
		REP INSW
		JNC	Short L7A2
L75C:		MOV	BYTE PTR [ByteHeld], 01
		IN	AX, DX
		STOSB
		MOV	[Data], AH
		JMP	Short L7A2

L768:		SHR	ECX, 1
		RCL	BL, 1
L76D:		INSW
		MOV	AX, [_Delay500]
		SHL	AX, 3
L771:		DEC	AX
		JNZ	L771
		LOOP	L76D
		RCR	BL, 1
		JNC	Short L7A2
		JMP	L75C
L77C:		MOV	AX, CX
		SHR	ECX, 2
		REP INSD
		MOV	CX, AX
		MOV	BX, 3
		AND	CX, BX
		JZ	Short L7A2
		NEG	AL
		AND	AL, BL
		MOV	[ByteHeld], AL
		IN	EAX, DX
L797:		STOSB
		SHR	EAX, 8
		LOOP	L797
		MOV	[Data], EAX
L7A2:		JMP	Short EndInnerLoop

Output:
		MOV	DS, [MapSel]
		XOR	EBX, EBX
		MOV	SI, BX
		XCHG	BL, [ByteMiss]
		AND	BL, 03
		JZ	Short L7CC
		SUB	ECX, EBX
		MOV	EAX, [Data]
L7BB:		LODSB
		ROR	EAX, 8
		DEC	BL
		JNZ	L7BB
		TEST	BYTE PTR [Mode], 80h
		JNZ	Short L7E6
		JMP	Short L7D4
L7CC:		TEST	BYTE PTR [Mode], 80h
		JNZ	Short L7E8
		JMP	Short L7D9
L7D4:		ROR	EAX, 16
		OUT	DX, AX
L7D9:		MOV	AX, CX
		MOV	BX, 1
		SHR	ECX, 1
		REP OUTSW
		JMP	Short L7F4

L7E6:		OUT	DX, EAX
L7E8:		MOV	AX, CX
		MOV	BX, 3
		SHR	ECX, 2
		REP OUTSD
L7F4:		AND	AX, BX
		MOV	CX, AX
		JCXZ	EndInnerLoop
		NEG	AL
		AND	AL, BL
		MOV	[ByteMiss], AL
L801:		LODSB
		ROR	EAX, 8
		LOOP	L801
		MOV	[Data], EAX
EndInnerLoop:
		POP	DS
		MOV	ECX, [XferRem]
		SUB	ECX, [XferCur]
		MOV	[XferRem], ECX
		JNZ	InnerLoop

		MOV	EAX, [XferLen]
		SUB	[BufRem], EAX
		MOV	BX, [npADDX]
		ADD	EAX, [BX + SGOffset]
		LES	SI, [pSGE]
		CMP	ES:[SI + XferBufLen], EAX
		JNE	Short EndOuterLoop
		INC	WORD PTR [BX + iSGList]
		ADD	WORD PTR [pSGE], SIZE SCATGATENTRY
		SUB	EAX, EAX
EndOuterLoop:
		MOV	[BX + SGOffset], EAX
		MOV	EAX, [BufRem]
		OR	EAX, EAX
		JNZ	OuterLoop

L860:		POP	SI
		POP	DI
		LEAVE
		RET	2

ADD_XFERIO	ENDP

_TEXT		ENDS
		END

