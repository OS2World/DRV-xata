;**************************************************************************
;*
;* SOURCE FILE NAME = S506DMA.asm
;*
;* DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
;*
;* Copyright : COPYRIGHT Daniela Engert 1999-2009
;* distributed under the terms of the GNU Lesser General Public License
;*
;* DESCRIPTION : DMA core routine
;****************************************************************************/

		.386p

_TEXT		SEGMENT DWORD USE16 PUBLIC 'CODE'
		ASSUME CS:_TEXT

SCATGATENTRY	STRUC
  BufAdr	DD ?	; + 0	  Physical pointer to transfer buffer
  BufLen	DD ?	; + 4	  Length of transfer buffer
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


BytesLeft	EQU BP + 4
SGOff		EQU BP - 8
npSGPtrs	EQU BP - 10
npSGOutM1	EQU BP - 12
SGAlign 	EQU BP - 14

; USHORT NEAR BuildSGList (NPPRD npSGOut, NPADD_X npSGPtrs, ULONG BytesLeft, UCHAR SGAlign) {
;	register bx = npSGOut
;	register dx = SGAlign
;	register ax = npSGPtrs
;	BytesLeft = bp + 4

		PUBLIC	@BuildSGList
@BuildSGList	PROC NEAR
		PUSH	BP
		MOV	BP,SP
		PUSH	DI
		PUSH	SI

		SUB	BX, 8			   ; npSGOut--

		MOV	SI, AX
		MOV	EDI, DWORD PTR [SI + SGOffset] ; SGOff
		NEG	EDI
		PUSH	EDI

		PUSH	SI			   ; save npSGPtrs
		PUSH	BX			   ; save npSGOut - 1
		PUSH	DX			   ; save SGAlign

		MOV	EDI, [BytesLeft]	   ; BytesLeft

		ROR	EBX, 16
		LES	BX, DWORD PTR [SI + pSGList]
		MOV	CX, [SI + cSGList]
		SHL	CX, 3
		ADD	CX, BX			   ; pSGEnd
		MOV	DX, [SI + iSGList]
		SHL	DX, 3
		ADD	BX, DX
		ROR	EBX, 16

OuterLoop:
		ROR	EBX, 16
		CMP	BX, CX
		JAE	Done

		MOV	EAX, DWORD PTR [SGOff]	   ; Length
		MOV	EDX, DWORD PTR ES:[BX+BufAdr]
		MOV	ESI, EDX		   ; CurPhysEnd
		SUB	EDX, EAX		   ; PhysicalAddress
		MOV	DWORD PTR [SGOff], 0

		TEST	DL,BYTE PTR [SGAlign]
		JNZ	NotAligned

LenLoop:
		ADD	EAX, DWORD PTR ES:[BX+BufLen]
		ADD	ESI, DWORD PTR ES:[BX+BufLen]
		CMP	EAX, EDI
		JA	Short LenTooLong
		ADD	BX, 8			   ; pSGList++
		CMP	BX, CX
		JAE	Short LenDone
		CMP	ESI, DWORD PTR ES:[BX+BufAdr]
		JE	LenLoop
		JMP	Short LenDone
LenTooLong:
		SUB	EAX, EDI
		SUB	EAX, DWORD PTR ES:[BX+BufLen]
		NEG	EAX
		MOV	DWORD PTR [SGOff], EAX
		MOV	EAX, EDI
LenDone:
		TEST	AL, BYTE PTR [SGAlign]
		JNZ	Short NotAligned

		ROR	EBX, 16
BldLoop:
		ADD	BX, 8			   ; npSGOut++
		MOV	DWORD PTR [BX+BufAdr], EDX ; npSGOut->Address = PhysicalAddress

		MOV	SI, DX			   ; calculate remainder in 64K
		NOT	SI
		MOVZX	ESI, SI
		INC	ESI

		CMP	EAX, ESI		   ; limit to remainder
		JA	Short $L1
		MOV	ESI, EAX
$L1:						   ; TempLength in ESI now!
		ADD	EDX, ESI		   ; PhysicalAddress += TempLength
		SUB	EAX, ESI		   ; Length -= TempLength
		SUB	EDI, ESI		   ; BytesLeft -= TempLength

		MOVZX	ESI, SI
		MOV	DWORD PTR [BX+BufLen], ESI ; npSGOut->ByteCountAndEOT = TempLength

		JZ	SHORT BldLoopDone

		OR	EAX, EAX
		JNZ	BldLoop 		   ; loop 'til no more bytes left
		JMP	OuterLoop
BldLoopDone:
		OR	BYTE PTR [BX+BufLen+3],128 ; npSGOut->ByteCountAndEOT |= PRD_EOT_FLAG
Done:
		POP	AX			   ; SGAlign

		MOV	AX, BX
		POP	BX			   ; npSGOutM1
		SUB	AX, BX
		SHR	AX, 3			   ; return ((npSGOut - npSGsave) + 1)

		ROR	EBX, 16
		POP	SI			   ; npSGPtrs
		SUB	BX, WORD PTR [SI + pSGList]
		SHR	BX, 3
		MOV	[SI + iSGList], BX
		POP	DWORD PTR [SI + SGOffset]  ; SGOffset

Exit:
		POP	SI
		POP	DI
		LEAVE
		RET	4

NotAligned:
		ADD	SP, 10
		SUB	AX, AX
		JMP	Exit

@BuildSGList	ENDP

_TEXT		ENDS
		END

