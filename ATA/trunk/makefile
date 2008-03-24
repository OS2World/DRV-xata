#/***********************************************************************/
#/*									*/
#/* Driver Name: DANIS506.ADD - Adapter Driver for PATA/SATA DASD	*/
#/*		 ------------------------------------------------	*/
#/*									*/
#/* Source File Name: MAKEFILE						*/
#/*									*/
#/*---------------------------------------------------------------------*/
#/*									*/
#/* Copyright (C) 1999-2007 Daniela Engert				*/
#/*									*/
#/***********************************************************************/

_VERSION = 1.8
_FIXLEVEL = 2a

#****************************************************************************
#  Dot directive definition area (usually just suffixes)
#****************************************************************************

.SUFFIXES:
.SUFFIXES: .com .add .exe .obj .asm .inc .def .lnk .crf .ref
.SUFFIXES: .lst .sym .map .c .h .lib

#****************************************************************************
#  Environment Setup for the component(s).
#****************************************************************************

#
# Conditional Setup Area and User Defined Macros
#

#
# Compiler Location w/ includes, libs and tools
#

INC	  = ..\..\..\..\..\inc
H	  = ..\..\..\..\..\h
LIB	  = ..\..\..\..\..\lib
TOOLS	  = ..\..\..\..\..\tools
DISKH	  = ..\..\diskh
RMLIB	  = ..\..\..\resource\rmcalls
APM	  = ..\..\..\thinkpad\dockii\apmcalls
ACPI	  = Acpi

#
# Since the compiler/linker and other tools use environment
# variables ( INCLUDE, LIB, etc ) in order to get the location of files,
# the following line will check the environment for the LIFE of the
# makefile and will be specific to this set of instructions. All MAKEFILES
# are requested to use this format to insure that they are using the correct
# level of files and tools.
#

!if [set INCLUDE=.;$(H);$(DISKH);$(ACPI)] || \
    [set PATH=$(TOOLS);$(PATH)]
!endif


#
# Compiler/tools Macros
#

#AS=masm
AS=tasm
CC=cl
LINK=link
MAPSYM=mapsym
PACK=nelite

#
# Set compiler and Linker Options
#
#  To create ICAT debug version use:
#     nmake icat=on
#

AFLAGS = -MX -T -Z -M
AINC   = -I. -I$(INC)
CFLAGS = /c /Zp /G2sr /Answ /nologo /Oi
LFLAGS = /map /nod /packd /a:2 /far
PACKFLAGS = /P:255 /A:0 /S /E+ /O-

!ifdef icat
CFLAGS = $(CFLAGS) /Zi /Od
LFLAGS = $(LFLAGS) /co
!endif

!ifdef notraces
CFLAGS = $(CFLAGS) /DTRACES=0
!else
CFLAGS = $(CFLAGS) /DTRACES=1
!endif

!if [set CL=$(CFLAGS)]
!endif

CFLAGS=

LIBS  =  $(APM)\apmcalls.lib $(RMLIB)\rmcalls.lib $(LIB)\libh.lib $(LIB)\os2286p.lib

#****************************************************************************
# Set up Macros that will contain all the different dependencies for the
# executables and dlls etc. that are generated.
#****************************************************************************

#
# H files
#
#
HDR = cmdparse.h cmdpdefs.h cmdphdr.h cmdproto.h cs.h cs_help.h \
      s506cons.h s506ext.h s506oem.h s506pro.h s506regs.h s506type.h \
      dhcalls.h iorb.h addcalls.h apmcalls.h ata.h pcmcia.h trace.h

#
# OBJ files
#
#
OBJ1 =	S506SEGS.OBJ S506DH.OBJ   ADDSERVA.OBJ S506PIO.OBJ  S506DMA.OBJ  S506IO.OBJ
OBJ2 =	S506DATA.OBJ S506RTE.OBJ  S506SM.OBJ   S506IORB.OBJ S506APM.OBJ  CS_HELP.OBJ
OBJ3 =	S506TIMR.OBJ S506CSUB.OBJ S506OSM2.OBJ S506OEM.OBJ  S506MATH.OBJ S506MPIF.OBJ CS.OBJ
OBJ4 =	S506INIT.OBJ CMDPARSE.OBJ S506PRTF.OBJ S506OEMI.OBJ S506GEOM.OBJ S506I13.OBJ
OBJ5 =	S506COMN.OBJ S506PIIX.OBJ S506VIA.OBJ  S506GNRC.OBJ S506ALI.OBJ  S506SIS.OBJ
OBJ6 =	S506CMD.OBJ  S506PDC.OBJ  S506CX.OBJ   S506HPT.OBJ  S506AEC.OBJ  S506SW.oBJ
OBJ7 =	S506OPTI.OBJ S506SII.OBJ  S506ITE.OBJ  S506JMB.OBJ  S506NTIO.OBJ ACPI.OBJ
OBJL =	..\..\LibObj\anfaulsh.obj ..\..\LibObj\anfauldi.obj ..\..\LibObj\anfaulre.obj

OBJS = $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) $(OBJ6) $(OBJ7)

#****************************************************************************
#   Setup the inference rules for compiling and assembling source code to
#   obejct code.
#****************************************************************************


.asm.obj:
	$(AS) $(AFLAGS) $(AINC) $*.asm;

.asm.lst:
	$(AS) -l -n $(AFLAGS) $(AINC) $*.asm;

.c.obj:
	$(CC) $(CFLAGS) $(CINC) $*.c

.c.i:
	$(CC) $(CFLAGS) $(CINC) -P $*.c

.c.lst:
	$(CC) $(CFLAGS) /Fc $(CINC) $*.c
	copy $*.cod $*.lst
	del $*.cod


#****************************************************************************
#   Target Information
#****************************************************************************

_VENDOR  = DANI
FILEVER = @^#$(_VENDOR):$(_VERSION)^#@^#^#1^#^# $(TIMESTAMP)::::$(_FIXLEVEL)::@@
CFLAGS = $(CFLAGS) /DVERSION=\"$(_VERSION).$(_FIXLEVEL)\"

#
# Should be the default tag for all general processing
#

all:   DaniS506.add

clean:
	if exist *.lnk	del *.lnk
	if exist *.obj	del *.obj
	if exist *.map	del *.map
	if exist *.lst	del *.lst
	if exist *.sym	del *.sym
	if exist *.add	del *.add
	if exist *.def	del *.def
	if exist *.mak	del *.mak


#*****************************************************************************
#   Specific Description Block Information
#*****************************************************************************

DaniS506.add:  $(OBJS) $(LIBS) $(HDR) makefile
	Rem Create DEF file <<$(@B).def
LIBRARY $(@B)

DESCRIPTION "$(FILEVER)  Adapter Driver for PATA/SATA DASD"

PROTMODE

SEGMENTS
 DDHeader	CLASS 'DATA'
 LIBDATA	CLASS 'DATA'
 _DATA		CLASS 'DATA'
 CONST		CLASS 'CONST'
 _BSS		CLASS 'BSS'
 'Code'         CLASS 'CODE' IOPL
 LIBCODE	CLASS 'CODE' IOPL
 _TEXT		CLASS 'CODE' IOPL
 InitCode	CLASS 'CODE' IOPL
 EndCode	CLASS 'CODE' IOPL
 Messages	CLASS 'MSGS' IOPL SWAPPABLE
 CSCode 	CLASS 'CODE' IOPL
 ACPICode	CLASS 'CODE' IOPL
 FCode		CLASS 'CODE' IOPL
 RMCode 	CLASS 'CODE' IOPL

<<keep
	$(LINK) $(LFLAGS) /NOE @<<$(@B).lnk
$(OBJ1) +
$(OBJ2) +
$(OBJ3) +
$(OBJL) +
$(OBJ4) +
$(OBJ5) +
$(OBJ6) +
$(OBJ7)
$*.add
$*.map
$(LIBS)
$(@B).def
<<keep
	     $(MAPSYM) $*.map
!ifndef icat
	     $(PACK) $@ $@ $(PACKFLAGS)
!endif
########     trcust $*.tsf

#****************************************************************************
#  Dependency generation and Checking
#****************************************************************************

depend.mak:  dephold
	touch depchk
	includes -e -sobj -llst -I. -I$(H) -I$(DOSINC) -I$(DISKH) -I$(ADDLIB) -I$(APM_H) -I$(PCM_H) -I$(INIT) -I$(TASK) -I$(INC) -P$$(H)=$(H) *.c *.asm >$@
	-del depchk

dephold:
	touch $@

$(OBJS): $(HDR)

