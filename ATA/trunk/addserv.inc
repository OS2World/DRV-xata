;*DDK*************************************************************************/
;
; Copyright:   COPYRIGHT Daniela Engert 1999-2009
; COPYRIGHT    Copyright (C) 1995 IBM Corporation
;
;    The following IBM OS/2 WARP source code is provided to you solely for
;    the purpose of assisting you in your development of OS/2 WARP device
;    drivers. You may use this code in accordance with the IBM License
;    Agreement provided in the IBM Device Driver Source Kit for OS/2. This
;    Copyright statement may not be removed.;
;
;* distributed under the terms of the GNU Lesser General Public License
;*****************************************************************************/

;**************************************************************************
;*
;* SOURCE FILE NAME = ADDSERV.INC
;*
;* DESCRIPTIVE NAME = ADD/DM Function Library routine
;*
;*
;*
;* VERSION = V2.0
;*
;* DATE
;*
;* DESCRIPTION :
;*
;*
;* CHANGE ACTIVITY =
;*   DATE      FLAG	   APAR   CHANGE DESCRIPTION
;*   --------  ----------  -----  --------------------------------------
;*   mm/dd/yy  @Vnnnnn	   XXXXX  XXXXXXX
;*
;*
;****************************************************************************/


;*************************************
;* _XferBuffData		     *
;*************************************

Xfer_Data     struc		  ; Xfer buffer data parameters structure

  X_Mode	    dw	    ?	    ; Direction of xferdata
  X_cSGList	    dw	    ?	    ; Count of S/G list element
  X_pSGList	    dd	    ?	    ; Far pointer to S/G list
  X_pBuffer	    dd	    ?	    ; Far pointer  to buffer
  X_numTotalBytes   dd	    ?	    ; Total bytes to copy
  X_iSGList	    dw	    ?	    ; Current index of S/G list
  X_SGOffset	    dd	    ?	    ; Current offset

Xfer_Data     ends


X_SGLIST_TO_BUFFER  equ     1	    ; From S/G list to the buffer
X_BUFFER_TO_SGLIST  equ     2	    ; From the buffer to S/G list


ScatGat_Entry struc

  X_ppXferBuf	    dd	    ?	    ; Physical pointer to transfer buffer
  X_XferBufLen	    dd	    ?	    ; Length of transfer buffer

ScatGat_Entry ends


Far_Pointer   struc

  off		  dw	  ?	  ; offset
  sel		  dw	  ?	  ; selector

Far_Pointer   ends

X_SG_LENGTH	    equ     size ScatGat_Entry	 ; Length ScatGat list


ADD_ERROR	    equ     1	    ;
ADD_SUCCESS	    equ     0	    ;


;*************************************
;* _ConvRBStoCHS		     *
;*************************************


CHS_Addr      struc

  C_Sector	    db	    ?
  C_Cylinder	    dw	    ?
  C_Head	    db	    ?

CHS_Addr      ends


Geometry      struc

  C_TotalSectors    dd	    ?
  C_BytesPerSector  dw	    ?
  C_Reserved	    dw	    ?
  C_NumHeads	    dw	    ?
  C_TotalCylinders  dd	    ?
  C_SectorsPerTrack dw	    ?

Geometry      ends

Geo	      struc

  G_NumHeads	    db	    ?
  G_SectorsPerTrack db	    ?
  G_TotalCylinders  dw	    ?
  G_TotalSectors    dd	    ?

Geo	      ends


;*************************************
;* _DMASetup			     *
;*************************************


Far_Pointer   struc

  D_low 	    dw	    ?	    ; low-word
  D_high	    dw	    ?	    ; high-word

Far_Pointer   ends


ENABLE	macro
	jmp  $+2		; timing delay
	sti			; enable processor external interrupts
	endm

ENAB	macro
	sti			; enable processor external interrupts
	endm

DISABLE macro
	cli			; disable processor external interrupts
	endm

DEVHLP	macro	function
	mov	dl,function
	call	dword ptr [_Device_Help]
	endm

IOWait	macro
	local	l
	push	ax
	mov	ax,10
l	label	near
	dec	ax
	jnz	l
	pop	ax
	endm




;*************************************
;* ADD Timer Services		     *
;*************************************

Timer_Pool    struc

  MTick 	 dw   ?      ;
  Entry_start	 db   ?

Timer_Pool    ends

Timer_Data    struc

  Interval	 dd   ?      ; Timer interval in millisecond
  BackupInterval dd   ?      ; Timer interval in millisecond for backup
  NotifyEntry	 dd   ?      ; Notify address
  Parm1 	 dd   ?      ; Parameter which ADD wants when timer os expired

Timer_Data    ends

lnTimerData   equ  size Timer_Data	   ; the size of Timer Data structure


