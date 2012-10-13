;**************************************************************************
; Dump16.asm
; $Id: $
;
; Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
;	       COPYRIGHT Daniela Engert 1999-2009
; Portions Copyright (c) 2010, 2011 Steven H. Levine
; distributed under the terms of the GNU Lesser General Public License
;*****************************************************************************/

                .386
                .387
;                .MODEL  USE32 OS2 FLAT
;                IDEAL
;                SMART
;                JUMPS
;                LOCALS

DATA32 SEGMENT  DWORD USE32 PUBLIC 'DATA'
DATA32 ENDS

CFGAddr         =       0CF8h
CFGData         =       0CFCh
EXTRN           DOSIODELAYCNT:ABS

PushFar MACRO   Loc
                DB      68h     ; PUSH imm32
                DW      SMALL OFFSET Loc, SEG Loc
ENDM

JMPTiled MACRO  Loc
                PUSH    word ptr 0                 ; zero upper half of target CS
                PUSH    EAX                     ; dummy push 32 bits
                MOV     EAX, OFFSET Loc         ; create 16 bit offset of
                PUSH    AX                      ; tiled target
                PUSHF
                MOV     EAX, ESP                ; build tiled SS:SP
                PUSH    0       ; 32 bits!      ; on stack
                PUSH    AX                      ; from current flat ESP
                SHR     EAX, 16-3               ; and make sure
                OR      AL, 7                   ; top 16 bits of ESP
                MOV     [ESP + 4], AX           ; are all zero!
                LSS     ESP, [ESP]              ; switch stack flat -> tiled
                MOV     EAX, OFFSET Loc         ; calculate tiled
                SHR     EAX, 16-3               ; target address from
                OR      AL, 7                   ; 32bit flat address
                SHL     EAX, 16                 ; and save it
                XCHG    EAX, [ESP + 6]          ; on stack
                POPF
                RETF                            ; jump to tiled target, clean stack
ENDM


CODE32 SEGMENT  DWORD USE32 PUBLIC 'CODE'
                ASSUME  CS:FLAT, DS:FLAT, SS:FLAT, ES:FLAT
                PUBLIC  ReadPortPDC
                PUBLIC  ReadPortOpti
                PUBLIC  WritePortOpti
                PUBLIC  ReadTF
                PUBLIC  WriteTF
                PUBLIC  SMAPI
                PUBLIC  ReadCMOS
                PUBLIC  ReadPCIcfgB
                PUBLIC  ReadPCIcfgW
                PUBLIC  ReadPCIcfgD
                PUBLIC  WritePCIcfgB
                PUBLIC  WritePCIcfgW
                PUBLIC  WritePCIcfgD


Call16 PROC NEAR
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP
Call16Common:                           ; Arguments go here
                JMPTiled Call16T
Call16 ENDP

Call16T PROC  FAR             ; we are in tiled USE 16 mode now!
                DB      0FFh,5Eh,10h    ; CALL SMALL [DWORD SS:BP+10h]
                DB      66h,0EAh        ; JMP  FAR Call16L
                DF      Call16L         ; jump back to flat USE32 mode again!
Call16T ENDP

Call16L PROC NEAR
                PUSH    DS              ; restore stack
                POP     SS              ; to FLAT:32
                LEAVE                   ; and return
                POP     EDI
                POP     ESI
                POP     EBX
                POP     ECX
                RET
Call16L ENDP

ReadPortPDC PROC NEAR
                PushFar PortIO
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP

                XCHG    EDX, EAX
                MOV     BL, 2
                JMP     Call16Common
ReadPortPDC ENDP

ReadPortOpti Proc NEAR
                PushFar PortIO
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP

                MOV     EDX, EAX
                MOV     BL, 3
                JMP     Call16Common
ReadPortOpti Endp

WritePortOpti Proc NEAR
                PushFar PortIO
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP

                XCHG    EDX, EAX
                MOV     BL, 4
                JMP     Call16Common
WritePortOpti Endp

ReadTF Proc  NEAR
                PushFar PortIO
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP

                MOV     BL, 5
                JMP     Call16Common
ReadTF Endp

WriteTF Proc NEAR
                PushFar PortIO
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP

                MOV     BL, 6
                JMP     Call16Common
WriteTF Endp

SMAPI Proc  NEAR
                PushFar PortIO
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP

                MOV     BL, 7
                JMP     Call16Common
SMAPI Endp

ReadCMOS Proc NEAR
                PushFar PortIO
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP

                MOV     BL, 8
                JMP     Call16Common
ReadCMOS Endp

ReadPCIcfgB Proc NEAR
                PushFar PortIO
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP

                MOV     BL, 9
                JMP     Call16Common
ReadPCIcfgB Endp

ReadPCIcfgW Proc NEAR
                PushFar PortIO
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP

                MOV     BL, 10
                JMP     Call16Common
ReadPCIcfgW Endp

ReadPCIcfgD Proc NEAR
                PushFar PortIO
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP

                MOV     BL, 11
                JMP     Call16Common
ReadPCIcfgD Endp

WritePCIcfgB Proc NEAR
                PushFar PortIO
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP

                MOV     BL, 12
                JMP     Call16Common
WritePCIcfgB Endp

WritePCIcfgW Proc NEAR
                PushFar PortIO
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP

                MOV     BL, 13
                JMP     Call16Common
WritePCIcfgW Endp

WritePCIcfgD Proc NEAR
                PushFar PortIO
                PUSH    EBX
                PUSH    ESI
                PUSH    EDI
                PUSH    EBP
                MOV     EBP,ESP

                MOV     BL, 14
                JMP     Call16Common
WritePCIcfgD Endp

CODE32 ENDS

IOCode SEGMENT  PAGE USE16 PUBLIC 'CODE'
                ASSUME  CS:IOCode, DS:FLAT, SS:FLAT, ES:FLAT

                PUBLIC  PortIO

PortIO Proc FAR
                CMP     BL, 0           ; ReadPort
                JNE     @@1
                IN      EAX, DX
                JMP     @@Exit
@@1:
                CMP     BL, 1           ; WritePort
                JNE     @@2
                OUT     DX, EAX
                JMP     @@Exit
@@2:
                CMP     BL, 2           ; ReadPortPromise
                JNE     @@3
                CLI
                OUT     DX, AL
                CALL    IODelay
                ADD     DX, 2
                XOR     EAX, EAX
                IN      AL, DX
                STI
                JMP     @@Exit
@@3:
                CMP     BL, 3           ; ReadPortOpti
                JNE     @@4
                CLI
                MOV     BX, DX
                AND     DX, 0FFF0h
                INC     DX
                IN      AX, DX
                CALL    IODelay
                IN      AX, DX
                CALL    IODelay
                MOV     AL, 03h
                INC     DX
                OUT     DX, AL
                CALL    IODelay
                XCHG    BX, DX
                IN      AL, DX
                CALL    IODelay
                MOV     DX, BX
                MOV     BL, AL
                MOV     AL, 83h
                OUT     DX, AL
                STI
                XOR     EAX, EAX
                MOV     AL, BL
                JMP     @@Exit
@@4:
                CMP     BL, 4           ; WritePortOpti
                JNE     @@5
                PUSH    AX
                CLI
                MOV     BX, DX
                AND     DX, 0FFF0h
                INC     DX
                IN      AX, DX
                CALL    IODelay
                IN      AX, DX
                CALL    IODelay
                MOV     AL, 03h
                INC     DX
                OUT     DX, AL
                CALL    IODelay
                XCHG    BX, DX
                POP     AX
                OUT     DX, AL
                CALL    IODelay
                MOV     DX, BX
                MOV     AL, 83h
                OUT     DX, AL
                STI
                JMP     @@Exit
@@5:
                CMP     BL, 5           ; ReadTaskFile
                JNE     @@6
                CLI
                ADD     DX, 6
                OUT     DX, AL
                CALL    IODelay
                SUB     DX, 5
                MOV     BL, 7
@@5L:
                IN      AL, DX
                INC     DX
                MOV     [ECX], AL
                INC     ECX
                CALL    IODelay
                DEC     BL
                JNZ     @@5L
                STI
                JMP     @@Exit
@@6:
                CMP     BL, 6           ; WriteTaskFile
                JNE     @@7
                CLI
                ADD     DX, 6
                OUT     DX, AL
                CALL    IODelay
                SUB     DX, 5
                MOV     AL, [ECX]
                OUT     DX, AL
                CALL    IODelay
                INC     DX
                MOV     AL, [ECX+1]
                OUT     DX, AL
                CALL    IODelay
                ADD     DX, 5
                MOV     AL, [ECX+6]
                OUT     DX, AL
                CALL    IODelay
@@6W:
                IN      AL, DX
                CALL    IODelay
                TEST    AL, 80h
                JNZ     @@6W

                SUB     DX, 6
                MOV     BL, 7
@@6L:
                IN      AL, DX
                INC     DX
                MOV     [ECX], AL
                INC     ECX
                CALL    IODelay
                DEC     BL
                JNZ     @@6L
                STI
                JMP     @@Exit
@@7:
                CMP     BL, 7           ; SMAPI
                JNE     @@8
                CLI
                PUSH    EAX
                MOV     BX, [EAX]
                MOV     CX, [EAX+2]
                MOV     SI, [EAX+4]
                MOV     AX, 5380h
                OUT     DX, AL
                OUT     4Fh, AL
                POP     EBX
                MOV     [EBX+2], CX
                MOV     [EBX+4], SI
                MOVZX   EAX, AH
                STI
                JMP     @@Exit
@@8:
                CMP     BL, 8           ; ReadCMOS
                JNE     @@9
                CLI
                OR      AL, 80h
                OUT     70h, AL
                OUT     4Fh, AL
                IN      AL, 71h
                MOV     AH, AL
                MOV     AL, 7
                OUT     70h, AL
                OUT     4Fh, AL
                IN      AL, 71h
                MOVZX   EAX, AH
                STI
                JMP     @@Exit
@@9:
                CMP     BL, 9           ; ReadPCICfgB
                JNE     @@10
                CLI
                CALL    PCIPrep
                IN      AL, DX
                XCHG    AX, BX
                XOR     EAX, EAX
                SUB     DL, CL
                OUT     DX, EAX
                STI
                XCHG    AX, BX
                JMP     @@Exit
@@10:
                CMP     BL, 10           ; ReadPCICfgW
                JNE     @@11
                CLI
                CALL    PCIPrep
                IN      AX, DX
                XCHG    AX, BX
                XOR     EAX, EAX
                SUB     DL, CL
                OUT     DX, EAX
                STI
                XCHG    AX, BX
                JMP     @@Exit
@@11:
                CMP     BL, 11           ; ReadPCICfgD
                JNE     @@12
                CLI
                CALL    PCIPrep
                IN      EAX, DX
                XCHG    EAX, EBX
                XOR     EAX, EAX
                SUB     DL, CL
                OUT     DX, EAX
                STI
                XCHG    EAX, EBX
                JMP     @@Exit
@@12:
                CMP     BL, 12           ; WritePCICfgB
                JNE     @@13
                CLI
                CALL    PCIPrep
                OUT     DX, AL
                XOR     EAX, EAX
                SUB     DL, CL
                OUT     DX, EAX
                STI
                JMP     @@Exit
@@13:
                CMP     BL, 13           ; WritePCICfgW
                JNE     @@14
                CLI
                CALL    PCIPrep
                OUT     DX, AX
                XOR     EAX, EAX
                SUB     DL, CL
                OUT     DX, EAX
                STI
                JMP     @@Exit
@@14:
                CMP     BL, 14           ; WritePCICfgD
                JNE     @@Exit
                CLI
                CALL    PCIPrep
                OUT     DX, EAX
                XOR     EAX, EAX
                SUB     DL, CL
                OUT     DX, EAX
                STI
                JMP     @@Exit
@@Exit:
                CALL    IODelay
                RET
PortIO Endp

PCIPrep Proc NEAR            ; EAX = 00/bus:8/dev:5/fnc:3/Reg:8
                OR      EAX, 80000000h  ; EAX = 80/bus:8/dev:5/fnc:3/reg:8
                MOV     EBX, EDX
                MOV     CL, AL
                AND     CL, 3
                ADD     CL, CFGData - CFGAddr
                AND     AL, NOT 3
                MOV     DX, CFGAddr
                OUT     DX, EAX
                ADD     DL, CL
                MOV     EAX, EBX
                RET
PCIPrep Endp

IODelay Proc NEAR
                PUSH    CX
                MOV     CX, DOSIODELAYCNT
                SHL     CX, 1
@@Wait:         DEC     CX
                JNZ     @@Wait
                POP     CX
                RET
IODelay Endp

IOCode ENDS

       END

