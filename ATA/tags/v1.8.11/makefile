# danis506 makefile
# $Id$

# 2009-03-19 SHL generalize
# 2010-06-07 SHL Clone from older msc build
# 2010-06-07 SHL Sync with LGPL sources
# 2011-03-04 SHL Comments and bldlevel tweaks
# 2011-06-07 SHL Correct icat debug build

#/************************************************************************/
#/*									 */
#/* Driver Name: DANIS506.ADD - Adapter Driver for PATA/SATA DASD	 */
#/*		 ------------------------------------------------	 */
#/*									 */
#/* Source File Name: MAKEFILE						 */
#/*									 */
#/* Copyright (C) 1999-2009 Daniela Engert				 */
#/* Portions Copyright (c) 2009, 2011 Steven H. Levine			 */
#/*									 */
#/* Distributed under the terms of the GNU Lesser General Public License */
#/*									 */
#/************************************************************************/

# Build options that can be set in enviroment or on nmake command line
#  VERSION - build version (i.e. 1.8) - see makefile for current default
#  FIXLEVEL - fix level (i.e 11 for 1.8.11) - see makefile for current default
#  VENDOR - builder (i.e. slainc, defaults to unknown)
#  BUILDMACH - build machine (i.e. slamain, defaults to unknown)
#  BUILDTIME - build time (yyyy/mm/dd hh:mm:ss) - make fails if not set
#  LIBOBJ - points to prebuild runtime .obj file directory (defaults to LibObj)
#  ACPI - points to ACPI toolkit header directory (defaults to ACPI)

# Build options that can be set on nmake command line
#  icat - generate ICAT symbolic debug data (default is no ICAT data)
#  notraces - suppress tracing logic - use for compressed build (default includes tracing logic)

# Allow external define
!ifdef VERSION
_VERSION = $(VERSION)
!else
_VERSION = 1.8
!endif

# Allow external define
!ifdef FIXLEVEL
_FIXLEVEL = $(FIXLEVEL)
!else
_FIXLEVEL = 11
!endif

# Allow override
!ifdef VENDOR
_VENDOR = $(VENDOR)
!else
_VENDOR = unknown
!endif

!ifndef BUILDMACH
BUILDMACH = unknown
!endif

# Spacing and width matters to bldlevel
# Define BUILDTIME as YYYY/MM/DD HH:MM:SS
# Bldlevel requires TIMESTAMP and trailing spaces to be exactly 25 characters
# BUILDTIME is 19 characters so we use 6 spaces here
!ifdef BUILDTIME
TIMESTAMP = $(BUILDTIME)      $(BUILDMACH)
!else
!error BUILDTIME must be defined as YYYY/MM/DD HH:MM:SS in environment
!endif

#****************************************************************************
#  Dot directive definition area (usually just suffixes)
#****************************************************************************

.SUFFIXES:
.SUFFIXES: .com .add .exe .obj .asm .inc .def .lnk .crf .ref
.SUFFIXES: .lst .sym .map .c .h .lib

#****************************************************************************
#  Environment Setup for the component(s).
#****************************************************************************

# Conditional Setup Area and User Defined Macros

# Compiler Location w/ includes, libs and tools

INC	  = ..\..\..\..\..\inc
H	  = ..\..\..\..\..\h
LIB	  = ..\..\..\..\..\lib
TOOLS	  = ..\..\..\..\..\tools
DISKH	  = ..\..\diskh
RMLIB	  = ..\..\..\resource\rmcalls
APM	  = ..\..\..\thinkpad\dockii\apmcalls

!ifndef ACPI
ACPI	  = Acpi
!endif

# Point at assumed object library directory unless defined externally
!ifndef LIBOBJ
LIBOBJ    = LibObj
!endif

# Since the compiler/linker and other tools use environment
# variables ( INCLUDE, LIB, etc ) in order to get the location of files,
# the following line will check the environment for the LIFE of the
# makefile and will be specific to this set of instructions. All MAKEFILES
# are requested to use this format to insure that they are using the correct
# level of files and tools.

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

AFLAGS = -MX -T -Z -M
AINC   = -I. -I$(INC)

# /Zp pack 1
# /G2sr 286, no stack probes, fastcall
# /Answ near data, small code, ss != ds, ds not reloaded
# /Oi intrinsics
# Define __OS2__ because code needs it; undefine MSDOS for safety
# CFLAGS = /c /Zp /G2sr /Answ /nologo /Oi
CFLAGS = /c /Zp /G2sr /Answ /nologo /Oi /D__OS2__ /UMSDOS
LFLAGS = /map /nod /packd /a:2 /far

PACKFLAGS = /P:255 /A:0 /S /E+ /O-

!ifdef icat
# Generate source level codeview debug data
CFLAGS = $(CFLAGS) /Zi /Od /Oi
LFLAGS = $(LFLAGS) /co
!endif

!ifdef notraces
CFLAGS = $(CFLAGS) /DTRACES=0
!else
CFLAGS = $(CFLAGS) /DTRACES=1
!endif

# Enable this to generate .cod listing
# CFLAGS = $(CFLAGS) /Fc

# Define in CL variable to keep command lines shorter
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

HDR = cmdparse.h cmdpdefs.h cmdphdr.h cmdproto.h cs.h cs_help.h \
      s506cons.h s506ext.h s506oem.h s506pro.h s506regs.h s506type.h \
      dhcalls.h iorb.h addcalls.h apmcalls.h ata.h pcmcia.h trace.h

#
# OBJ files
#

OBJ1 =	S506SEGS.OBJ S506DH.OBJ   ADDSERVA.OBJ S506PIO.OBJ  S506DMA.OBJ  S506IO.OBJ
OBJ2 =	S506DATA.OBJ S506RTE.OBJ  S506SM.OBJ   S506IORB.OBJ S506APM.OBJ  CS_HELP.OBJ
OBJ3 =	S506TIMR.OBJ S506CSUB.OBJ S506OSM2.OBJ S506OEM.OBJ  S506MATH.OBJ S506MPIF.OBJ CS.OBJ
OBJ4 =	S506INIT.OBJ CMDPARSE.OBJ S506PRTF.OBJ S506OEMI.OBJ S506GEOM.OBJ S506I13.OBJ
OBJ5 =	S506COMN.OBJ S506PIIX.OBJ S506VIA.OBJ  S506GNRC.OBJ S506ALI.OBJ  S506SIS.OBJ
OBJ6 =	S506CMD.OBJ  S506PDC.OBJ  S506CX.OBJ   S506HPT.OBJ  S506AEC.OBJ  S506SW.oBJ
OBJ7 =	S506OPTI.OBJ S506SII.OBJ  S506ITE.OBJ  S506JMB.OBJ  S506NTIO.OBJ ACPI.OBJ
OBJL =	$(LIBOBJ)\anfaulsh.obj $(LIBOBJ)\anfauldi.obj $(LIBOBJ)\anfaulre.obj

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
	echo CL=%%CL
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

# nmake requires #s to be quoted with ^
FILEVER = @^#$(_VENDOR):$(_VERSION).$(_FIXLEVEL)^#@^#^#1^#^# $(TIMESTAMP)::::$(_FIXLEVEL)::@@
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
	@echo Creating DaniS506.def <<$(@B).def
; Created by makefile

LIBRARY $(@B)

DESCRIPTION "$(FILEVER)Adapter Driver for PATA/SATA DASD"

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
 CSCode	CLASS 'CODE' IOPL
 ACPICode	CLASS 'CODE' IOPL
 FCode		CLASS 'CODE' IOPL
 RMCode	CLASS 'CODE' IOPL

<<keep
	@echo Linking DaniS506.add
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
