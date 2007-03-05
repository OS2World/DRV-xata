;*DDK*************************************************************************/
;
; COPYRIGHT    Copyright (C) 1995 IBM Corporation
;
;    The following IBM OS/2 WARP source code is provided to you solely for
;    the purpose of assisting you in your development of OS/2 WARP device
;    drivers. You may use this code in accordance with the IBM License
;    Agreement provided in the IBM Device Driver Source Kit for OS/2. This
;    Copyright statement may not be removed.;
;*****************************************************************************/

;**************************************************************************
;*
;* SOURCE FILE NAME = ADDMACS.INC
;*
;* DESCRIPTIVE NAME = ADD/DM Function Library
;*
;*
;*
;* VERSION = V2.0
;*
;* DATE
;*
;* DESCRIPTION : Private macros for ADDCALLS.LIB
;*
;*
;* CHANGE ACTIVITY =
;*   DATE      FLAG	   APAR   CHANGE DESCRIPTION
;*   --------  ----------  -----  --------------------------------------
;*   mm/dd/yy  @Vnnnnn	   XXXXX  XXXXXXX
;*
;*
;****************************************************************************/

;
;----------------------------------------------------------------------------:
;									     ;
;  ADD Common Services Library Macros						     ;
;									     ;
;----------------------------------------------------------------------------;
;
;
; To define a Device Help Thunk:
;
;
;   ADDDef   Name
;
;   ADDArgs  Type, ArgName1
;     .
;     .
;     .
;   ADDArgs  Type, ArgNameN
;
;   ADDCall
;
;   ADDRet1
;
;

;
;
; Macro ADDDef
;
;
ADDDef	 Macro	 ADDName, Code
;
	public	ADD_&ADDName
ADD_&ADDName  label   near

ADDCode  =	ADD_&ADDName
Frame	=	 0
Disp	=	-4
	push	bp
	mov	bp, sp
;
	Endm

;
;
; Macro ADDDef_F
;
;
ADDDef_F   Macro   ADDName, Code
;
	public	f_ADD_&ADDName
f_ADD_&ADDName	label	far

ADDCode  =	f_ADD_&ADDName
Frame	=	 0
Disp	=	-6
	push	bp
	mov	bp, sp
;
	Endm
;
;
; Macro ADDArgs
;
;
ADDArgs  Macro	Type,ArgName
;

Field = 0
IFIDN <&Type>,<PADD_XFER_DATA>
  Field = 4
ENDIF
IFIDN <&TYPE>,<ULONG>
  Field = 4
ENDIF
IFIDN <&TYPE>,<PGEOMETRY>
  Field = 4
ENDIF
IFIDN <&TYPE>,<NPGEO>
  Field = 2
ENDIF
IFIDN <&TYPE>,<PCHS_ADDR>
  Field = 4
ENDIF
IFIDN <&TYPE>,<USHORT>
  Field = 2
ENDIF
IFIDN <&TYPE>,<PBYTE>
  Field = 4
ENDIF
IFIDN <&TYPE>,<PFN>
  Field = 4
ENDIF
IFIDN <&TYPE>,<PULONG>
  Field = 4
ENDIF
IFIDN <&TYPE>,<PUSHORT>
  Field = 4
ENDIF
IFIDN <&TYPE>,<PVOID>
  Field = 4
ENDIF
IFE Field
  %OUT Unknown Argument: &Type
  .ERR
ENDIF

Disp  = Disp+Field
&ArgName = -Disp		   ; Offset to variable from FRAME
Frame = Frame+Field
;
	Endm
;
;
; Macro ADDRet1
;
;
ADDRet1  Macro

	 ADDRet2  %Frame

	 Endm
;
;
; Macro ADDRet2
;
;
ADDRet2  Macro	 FrameSize

	pop	bp
	ret	FrameSize

	Endm

;
;
; Macro ADDRetF1
;
;
ADDRetF1  Macro

	 ADDRetF2  %Frame

	 Endm
;
;
; Macro ADDRetF2
;
;
ADDRetF2  Macro   FrameSize

	pop	bp
	retf	FrameSize

	Endm



Stk	equ	<[BP].Frame>
