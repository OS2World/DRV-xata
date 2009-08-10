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

	page	,132

;**************************************************************************
;*
;* SOURCE FILE NAME = ADDSERVA.ASM
;*
;* DESCRIPTIVE NAME = ADDCALLS common service routines
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
;****************************************************************************/

	title	ADDSERVA - OS/2 2.0 ADD Common Service Routine
	name	ADDSERVA


	.xlist
	include basemaca.inc
	include ABINT13.inc		;
	include devhlp.inc		; device helper definitions
	include infoseg.inc		; information segment definitions
	include addserv.inc		; ADD common internal include file
	include addmacs.inc		; ADD common internal macro file
	.list


EXTRN	DOS32FLATDS:ABS

LIBDATA segment dword public 'DATA'

	extrn	_Device_Help:dword

	TimerPool_ptr	 dd  0		; save area of The Timer Pool address
	Number_of_Timers dw  0		; save area of # of timer elements
			 dw  0		; pad

LIBDATA ends


LIBCODE segment dword public 'CODE'
	assume	CS:LIBCODE,DS:LIBDATA

	.486p

;******************************************************************************
;*
;*  SUBROUTINE NAME:   f_ADD_InitTimer
;*
;*  DESCRIPTIVE NAME:  Initialize Timer
;*
;*  FUNCTION:	       Initializes the timer service
;*
;*  LINKAGE:	       call far
;*
;*  PARAMETERS:        Far pointer to the timer data pool
;*		       The size of the timer pool in bytes
;*
;*  EXIT-NORMAL:       AX = 0	( timer handle )
;*
;*  EXIT-ERROR:        AX != 0
;*
;*  PSEUDOCODE:
;*
;******************************************************************************
;******************************************************************************
;*
;*  SUBROUTINE NAME:   ADD_InitTimer
;*
;*  DESCRIPTIVE NAME:  Initialize Timer
;*
;*  FUNCTION:	       Initializes the timer service
;*
;*  LINKAGE:	       call near
;*
;******************************************************************************

TimerPool    equ   es:[di]

	 ADDDef   InitTimer
	 ADDArgs  PBYTE,     pTimerPool
	 ADDArgs  USHORT,    lnTimerPool

	     SaveReg <es,di,si,bx,cx,dx>
					      ;
	     mov     cx,Stk.lnTimerPool       ; get timer pool length
	     xor     bx,bx		      ;
	     les     di,Stk.pTimerPool	      ; set pointer to timer pool
					      ;
TI_loop1:				      ; clear timer pool
	     mov     byte ptr TimerPool[bx],0 ;
	     inc     bx 		      ;
	     loop    TI_loop1		      ;
					      ;
	     mov     al,DHGETDOSV_SYSINFOSEG  ; get system global infoseg

	     SaveReg <es,ds,si,cx,dx>
	     DEVHLP  DevHlp_GetDOSVar	      ;
	     RestoreReg <dx,cx,si,ds,es>

	     jc      TI_error_exit	      ; if carry on => error
					      ;
	     push    es 		      ;
	     mov     es,ax		      ; ax:bx => sysinfoseg
	     mov     es,word ptr es:[bx]      ;
	     xor     bx,bx		      ; es:bx => addr/sysinfo
	     mov     ax,es:[bx].SIS_ClkIntrvl ; get timer interval
					      ;
	     xor     dx,dx		      ; convert MS/TICK
	     mov     cx,10		      ;
	     div     cx 		      ;
					      ;
	     pop     es 		      ;
	     mov     TimerPool.MTick,ax       ; save MS/TICK
					      ;
	     mov     ax,offset f_ADD_TimerHandler ; ax <= Timer Handler offset
					      ;
	     SaveReg <es,gs,di,si,bx,cx>
	     cli			      ;
	     DEVHLP  DevHlp_SetTimer	      ; Set timer handler to be
	     sti			      ;    called on every timer tick
	     RestoreReg <cx,bx,si,di,gs,es>

	     jc      TI_error_exit	      ; if carry on => error
	     mov     eax,Stk.pTimerPool       ; save ptr to Pool
	     mov     dword ptr TimerPool_ptr,eax  ; save ptr to Pool
	     xor     dx,dx		      ;
	     mov     ax,Stk.lnTimerPool       ; culc. how many timer elements
	     mov     cx,lnTimerData	      ;
	     div     cx 		      ;
	     mov     Number_of_Timers,ax      ; save number of elements
	     mov     ax,ADD_SUCCESS	      ; set return code
	     jmp     short TI_exit	      ;
					      ;
TI_error_exit:				      ;
	     mov     ax,ADD_ERROR	      ; set return code and go to exit

TI_exit:
	     RestoreReg <dx,cx,bx,si,di,es>

	     ADDRET1			      ;
					      ;

;******************************************************************************
;*
;*  SUBROUTINE NAME:   f_ADD_DeInstallTimer
;*
;*  DESCRIPTIVE NAME:  Deinstall Timer Support
;*
;*  FUNCTION:
;*
;*  LINKAGE:	       call far
;*
;*  PARAMETERS:
;*
;*
;*  EXIT-NORMAL:
;*
;*  EXIT-ERROR:
;*
;*  PSEUDOCODE:
;*
;******************************************************************************
;******************************************************************************
;*
;*  SUBROUTINE NAME:   ADD_DeInstallTimer
;*
;*  DESCRIPTIVE NAME:  Disable Timer Support
;*
;*  FUNCTION:
;*
;*  LINKAGE:	       call near
;*
;******************************************************************************

	 ADDDef   DeInstallTimer

	     mov     ax,offset f_ADD_TimerHandler ; ax <= Timer Handler offset

	     DEVHLP  DevHlp_ResetTimer

	     mov     Number_of_Timers, 0

	     ADDRET1			      ;


;******************************************************************************
;*
;*  SUBROUTINE NAME:   f_ADD_StartTimerMS
;*
;*  DESCRIPTIVE NAME:  Start Timer
;*
;*  FUNCTION:	       This function set the interval of the timer
;*
;*  LINKAGE:	       call far
;*
;*  PARAMETERS:        Far pointer to timer handle
;*		       Timer interval
;*		       Notify address
;*		       parameters which ADD wants when the timer is expired
;*
;*  EXIT-NORMAL:       AX = 0
;*
;*  EXIT-ERROR:        AX != 0
;*
;*  PSEUDOCODE:
;*
;******************************************************************************
;******************************************************************************
;*
;*  SUBROUTINE NAME:   ADD_StartTimerMS
;*
;*  DESCRIPTIVE NAME:  Start Timer
;*
;*  FUNCTION:	       This function set the interval of the timer
;*
;*  LINKAGE:	       call near
;*
;******************************************************************************

TimerData  equ	 es:[bx]			;
						;
	ADDDef	 StartTimerMS			;
	ADDArgs  PUSHORT,    phTimer		;
	ADDArgs  ULONG,      vTimer		;
	ADDArgs  PFN,	     addrNotify 	;
	ADDArgs  PVOID,      parm_1		;
						;
	     SaveReg <es,di,ebx,ecx,edx>	;

	     les     di,TimerPool_ptr		; set timer pool address
	     lea     bx, [di+2] 		; add "MTick" length
	     mov     cx,Number_of_Timers	;
						;
	     mov     edx,Stk.addrNotify 	;
TS_loop1:					;
	     xor     eax, eax
	     cmpxchg TimerData.NotifyEntry,edx	; look for available entry
	     je      TS_entry_available 	;
	     add     bx,lnTimerData		;
	     loop    TS_loop1			;
	     jmp     short TS_error_exit	;
						;
TS_entry_available:				;
	     mov     edx,Stk.parm_1		;
	     mov     TimerData.Parm1,edx	; save parameter ADD wants
						;
	     mov     eax,Stk.vTimer		;
	     xor     edx,edx			; convert to tick
	     movzx   ecx,TimerPool.MTick	;
	     div     ecx			;
	     or      edx,edx			; if dx!=0 => raising value
	     jnz     TS_increment_value 	;
	     or      eax,eax			; if value is 0 => one tick
	     jnz     TS_not_increment		;
						;
TS_increment_value:				;
	     inc     ax 			;
						;
TS_not_increment:				;
	     inc     ax 			; partial timer tick
	     mov     TimerData.Interval,eax	; save timer interval
	     mov     TimerData.BackupInterval,eax ; save timer interval for backup

	     les     di,Stk.phTimer		;
	     mov     word ptr es:[di],bx	; set timer handle
						;
	     mov     ax,ADD_SUCCESS		; set return code
	     jmp     short TS_exit		;	and go to exit
						;
TS_error_exit:					;
	     mov     ax,ADD_ERROR		; set return code
						;
TS_exit:					;
	     RestoreReg <edx,ecx,ebx,di,es>	;

	ADDRET1 				;


;******************************************************************************
;*
;*  SUBROUTINE NAME:   f_ADD_Cancel Timer
;*
;*  DESCRIPTIVE NAME:  Cancel Timer
;*
;*  FUNCTION:	       This function cancels the timer, so the
;*		       timer handle won't be effective any more.
;*
;*  LINKAGE:	       call far
;*
;*  PARAMETERS:        Timer handle
;*
;*  EXIT-NORMAL:       AX = 0
;*
;*  EXIT-ERROR:        none
;*
;*  PSEUDOCODE:
;*
;******************************************************************************
;******************************************************************************
;*
;*  SUBROUTINE NAME:   ADD_Cancel Timer
;*
;*  DESCRIPTIVE NAME:  Cancel Timer
;*
;*  FUNCTION:	       This function calces the timer, so the
;*		       timer handle won't be effective any more.
;*
;*  LINKAGE:	       call near
;*
;******************************************************************************

	ADDDef	 CancelTimer			;
	ADDArgs  USHORT      hTimer		;
						;
	     SaveReg <es,bx>			;
						;
	     les     bx,TimerPool_ptr		; get timer pool address
	     mov     bx,Stk.hTimer		;
						;
	     xor     eax, eax
	     mov     TimerData.NotifyEntry,eax	; clear notify address
	     mov     TimerData.Interval,eax	; clear interval value
	     mov     TimerData.BackupInterval,eax; clear interval for backup
						;
	     RestoreReg <bx,es> 		;

	ADDRET1 				;
						;

;******************************************************************************
;*
;*  SUBROUTINE NAME:   TimerHandler
;*
;*  DESCRIPTIVE NAME:  TimerHandler
;*
;*  FUNCTION:	       this routiner is called on every timer tick.
;*
;*  LINKAGE:	       call far
;*
;*  PARAMETERS:        None
;*
;*  EXIT-NORMAL:       Always
;*
;*  EXIT-ERROR:        None
;*
;*  PSEUDOCODE:        call the routine of timer requester
;*
;******************************************************************************

						;
	ADDDef_F TimerHandler			;
						;
	     DISABLE

	     SaveReg <gs,es,bx,cx>		;
						;
	     les     bx,TimerPool_ptr		;
	     inc     bx
	     inc     bx
	     mov     cx,Number_of_Timers	;
	     mov     ax,DOS32FLATDS
	     mov     gs,ax
						;
TH_loop1:					; check each entry.....
	     cmp     TimerData.NotifyEntry,0	;
	     je      TH_skip_entry		;
	     dec     TimerData.Interval 	; decrease every tick count.
	     jnz     TH_skip_entry		; if zero => WAKE UP

	     mov     eax,TimerData.BackupInterval ; set interval for the next
	     mov     TimerData.Interval,eax	;

	     SaveReg <es,bx,cx> 		;
	     push    TimerData.Parm1		;
	     push    bx 			; set parameters

	     call    TimerData.NotifyEntry	; call notify address of ADD

	     add     sp,6
	     RestoreReg <cx,bx,es>
						;

TH_skip_entry:					;
	     add     bx,lnTimerData		; set the next entry pointer
	     loop    TH_loop1			;
						;
	     RestoreReg <cx,bx,es,gs>

	     ENABLE
	     ADDRETF1


LIBCODE 	ends

		end
