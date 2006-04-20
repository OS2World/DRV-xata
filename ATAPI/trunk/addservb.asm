;*DDK*************************************************************************/
;
; Copyright:   COPYRIGHT Daniela Engert 1999-2006
; COPYRIGHT    Copyright (C) 1995 IBM Corporation
;
;    The following IBM OS/2 WARP source code is provided to you solely for
;    the purpose of assisting you in your development of OS/2 WARP device
;    drivers. You may use this code in accordance with the IBM License
;    Agreement provided in the IBM Device Driver Source Kit for OS/2. This
;    Copyright statement may not be removed.;
;*****************************************************************************/

	page	,132

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


LIBDATA segment dword public 'DATA'
LIBDATA ends


LIBCODE segment dword public 'CODE'
	assume	CS:LIBCODE,DS:LIBDATA

	CPUMODE 386
;******************************************************************************
;*
;*  SUBROUTINE NAME:   f_ADD_ConvRBAtoCHS
;*
;*  DESCRIPTIVE NAME: Conversion from RBA to CHS
;*
;*  FUNCTION:	 Converts a Relative Block Address to a Cylinder / Head /
;*		 Sector.
;*
;*  EXTRY POINT: f_ADD_ConvRBAtoCHS
;*
;*  LINKAGE:	 Call far
;*
;*  INPUT:	 ULONG	    RBA
;*		 NPGEOMETRY Near pointer to Geometry control block
;*		 NPCHS_ADDR Near pointer to CHS output.
;*
;*		 GEOMETRY and CHS_ADDR structures are defined in IORB.H.
;*
;*  OUTPUT:	 Exit normal:	 AX = 0
;*				 CHS_ADDR will be filled up.
;*
;*		 Exit error:	 AX != 0
;*				  - invalid parameter
;*
;*  PSEUDOCODE:  ( RBA ) / ( SectorsPerTrack )
;*		       =>  Remainder + 1  = Sector#
;*
;*		 ( Quatient of the above function ) / ( NumberOfHeads )
;*		       =>  Quarient  = Cylinder#
;*		       =>  Remainder = Head#
;*
;******************************************************************************
;******************************************************************************
;*
;*  SUBROUTINE NAME:   ADD_ConvRBAtoCHS
;*
;*  DESCRIPTIVE NAME: Conversion from RBA to CHS
;*
;*  FUNCTION:	 Converts a Relative Block Address to a Cylinder / Head /
;*		 Sector.
;*
;*  EXTRY POINT: ADD_ConvRBAtoCHS
;*
;*  LINKAGE:	 Call near
;******************************************************************************

sCHS	     equ    es:[di]
sGeo	     equ    [si]

	ADDDef	 ConvRBAtoCHS
	ADDArgs  ULONG,      RBA
	ADDArgs  NPGEO,      npGeometry
	ADDArgs  PCHS_ADDR,  pCHS

	     SaveReg <ds,es,di,si,ebx,dx>

	     mov     si,Stk.npGeometry		  ; Geometry offset
	     les     di,Stk.pCHS		  ; CHS offset
						  ;
	     mov     eax,Stk.RBA		  ; get RBA
	     xor     edx,edx			  ;
	     movzx   ebx,sGeo.G_SectorsPerTrack   ; get sectors per track
	     or      bl,bl			  ;  if 0 => error
	     jz      short C_error_exit 	  ; ( to avoid divided by zero)
						  ;
	     div     ebx			  ; RBA / Sectors per track
						  ;
	     inc     dl 			  ; Remainder + 1
	     mov     sCHS.C_Sector,dl		  ; set Sector number
						  ;
	     xor     edx,edx			  ;
	     movzx   ebx,sGeo.G_NumHeads	  ; get the number of heads
	     or      bl,bl			  ; if 0 => error
	     jz      short C_error_exit 	  ; ( to avoid divided by zero)
	     cmp     bl, 16			  ; if > 16 => error
	     ja      short C_error_exit 	  ;
						  ;
	     div     ebx			  ; Quitient / Number of heads
						  ;
	     mov     sCHS.C_Cylinder,ax 	  ; set cylinder number
	     shr     eax,16			  ;  if more than 2 bytes
	     jnz     short C_error_exit 	  ;	    => error
						  ;
	     and     dl,0fh			  ; Head field is nibble
	     mov     SCHS.C_Head,dl		  ; set head number
						  ;
	     mov     ax,ADD_SUCCESS		  ; set return code
	     jmp     short C_conv_exit		  ;	and goto exit
						  ;
C_error_exit:					  ;
	     mov     ax,ADD_ERROR		  ; set return code
						  ;
C_conv_exit:					  ;
	     RestoreReg <dx,ebx,si,di,es,ds>	  ;

	     ADDRET1				  ;

LIBCODE 	ends

		end
