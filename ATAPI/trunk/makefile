#/****************************************************************************/
#/*									     */
#/* Driver Name: DaniATAPI.FLT - Filter Driver for ATAPI units		     */
#/*		 ------------------------------------------------	     */
#/*									     */
#/* Source File Name: MAKEFILE						     */
#/*									     */
#/*--------------------------------------------------------------------------*/
#/*									     */
#/* COPYRIGHT (c) Daniela Engert 2000-2007				     */
#/*									     */
#/****************************************************************************/

_VERSION  = 0.4
_FIXLEVEL = 0wip

#****************************************************************************
#  Dot directive definition area (usually just suffixes)
#****************************************************************************

.SUFFIXES:
.SUFFIXES: .com .flt .exe .obj .asm .inc .def .lnk .lrf .crf .ref
.SUFFIXES: .lst .sym .map .c .h .lib .cod .flt

#****************************************************************************
#  Environment Setup for the component(s).
#****************************************************************************

#
# Conditional Setup Area and User Defined Macros
#

#
# Compiler Location w/ includes, libs and tools
#

INC    = ..\..\..\..\..\inc
H      = ..\..\..\..\..\h
LIB    = ..\..\..\..\..\lib
TOOLS  = ..\..\..\..\..\tools
DISKH  = ..\..\diskh
RMLIB  = ..\..\..\resource\rmcalls
APM    = ..\..\..\thinkpad\dockii\apmcalls
CDROM  = ..\..\cdrom\os2cdrom

#
# Since the compiler/linker and other tools use environment
# variables ( INCLUDE, LIB, etc ) in order to get the location of files,
# the following line will check the environment for the LIFE of the
# makefile and will be specific to this set of instructions. All MAKEFILES
# are requested to use this format to insure that they are using the correct
# level of files and tools.
#

!if [set INCLUDE=.;$(H);$(DISKH);$(CDROM)] || \
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
LFLAGS = /map /nod /noe /packd /a:2 /far
PACKFLAGS = /P:255 /A:0 /S /E+ /O-

!ifdef icat
CFLAGS = $(CFLAGS) /Zi /Od
LFLAGS = $(LFLAGS) /co
!endif

LIBS  =  $(LIB)\libh.lib $(RMLIB)\rmcalls.lib $(APM)\apmcalls.lib $(LIB)\os2286p.lib

#****************************************************************************
# Set up Macros that will contain all the different dependencies for the
# executables and dlls etc. that are generated.
#****************************************************************************

HDR = addcalls.h apmcalls.h atapicon.h atapiext.h atapipro.h atapireg.h \
      atapityp.h cmdparse.h cmdpdefs.h cmdpext.h cmdproto.h dhcalls.h iorb.h

#
# OBJ files
#
#
OBJ1 =	ATAPISEG.OBJ S506DH.OBJ ADDSERVA.OBJ ADDSERVB.OBJ S506PIO.OBJ S506DMA.OBJ S506MATH.OBJ ATAPIISM.OBJ ATAPIOSM.OBJ
OBJ2 =	ATAPIGEN.OBJ ATAPIORB.OBJ ATAPINIT.OBJ ATAPIDAT.OBJ S506PRTF.OBJ S506CSUB.OBJ
OBJ4 =	PARSECMD.OBJ CMDPARSE.OBJ CMDPDATA.OBJ

OBJS = $(OBJ1) $(OBJ4) $(OBJ2)

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

all:   DaniATAPI.FLT

clean:
	if exist *.lnk	del *.lnk
	if exist *.obj	del *.obj
	if exist *.map	del *.map
	if exist *.lst	del *.lst
	if exist *.sym	del *.sym
	if exist *.flt	del *.flt
	if exist *.def	del *.def
	if exist *.mak	del *.mak


#*****************************************************************************
#   Specific Description Block Information
#*****************************************************************************

DaniATAPI.FLT:	$(OBJS) $(LIBS) makefile
	rem Create DEF file <<$(@B).def
LIBRARY $(@B)

DESCRIPTION "$(FILEVER)  Filter Driver for ATAPI devices"

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
 'RMCode'       CLASS 'CODE' IOPL
 EndCode	CLASS 'CODE' IOPL
 Messages	CLASS 'MSGS' IOPL SWAPPABLE

<<keep
	$(LINK) $(LFLAGS) @<<$(@B).lnk
$(OBJ1) +
$(OBJ2) +
$(OBJ4)
$*.FLT
$*.map
$(LIBS)
$(@B).def
<<keep
	     $(MAPSYM) $*.map
!ifndef icat
	     $(PACK) $@ $@ $(PACKFLAGS)
!endif

#****************************************************************************
#  Dependency generation and Checking
#****************************************************************************

#depend.mak:  dephold
#	 touch depchk
#	 includes -e -sobj -llst -I. -I$(H) -I$(DISKH) -I$(CDROM) -I$(RSM_H) -I$(ADDLIB) -I$(APMLIB) -I$(DOCLIB) -I$(DOSINC) -I$(INIT) -I$(TASK) -I$(INC) \
#				 -P$$(H)=$(H) *.c *.asm >$@
#	 -del depchk
#
#dephold:
#	 touch $@
#
#!include depend.mak

$(OBJS): $(HDR)

