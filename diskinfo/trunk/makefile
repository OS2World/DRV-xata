# Makefile for building diskinfo
# $Id: $

# This is an OpenWatcom makefile

# Required environment variables
# WATCOM
# VENDOR
# BUILDVER
# BUILDMACH
# BUILDTIME

#DEBUG=1

!ifdef %WATCOM
WATCOM=$(%WATCOM)
!else
!error WATCOM not defined
!endif

# yyyy/mm/dd hh:mm:ss
!ifndef %BUILDTIME
!error BUILDTIME not defined
!endif

!ifndef %BUILDVER
!error BUILDVER not defined
!endif

!ifndef %BUILDMACH
!error BUILDMACH not defined
!endif

!ifndef %VENDOR
!error VENDOR not defined
!endif

NAME = diskinfo
DESC = Diskinfo utility (c)2006 Daniela Engert, Copyright (c)2012 Steven Levine and Associates, Inc. All rights reserved
EXENAME = $(NAME)
MAKEFILE = makefile

.ERASE

.SUFFIXES:
.SUFFIXES: .sys .exe .obj .asm .inc .def .lrf .lst .sym .map .c .h .lib .dbg .dll .itl .hlp

CC=wcc386 -q
CFLAGS=-bt=os2 -bm -5s -s -olirs -zp4 -d2 -wx
!ifdef DEBUG
CFLAGS += -dDEBUG
!endif

ASM=wasm
AFLAGS=-q -6p -bt=os2 -wx -d1

LINK=wlink
%INCLUDE=.;..\h;$(WATCOM)\H;$(WATCOM)\H\OS2;$(ACPITK)\h;
# %LIB=$(ACPITK)\lib;

OBJS = DiskInfo.obj Identify.obj Dump16.obj

all: DiskInfo.exe

# Inference rules
.c.obj: .AUTODEPEND
     $(CC) $(CFLAGS) $*.c

.asm.obj: .AUTODEPEND
     $(ASM) $(AFLAGS) $*.asm

# Avoid lost spaces in @%write
FILL = xxxx

DiskInfo.exe: $(OBJS) $(MAKEFILE)
	@%create $^*.lrf
        @%append $^*.lrf op desc '@$#$(%VENDOR):$(%BUILDVER)$#@$#$#1$#$# $(%BUILDTIME) $(FILL:x= ) $(%BUILDMACH)::EN:US:0:U:@@$(DESC)'
	@%append $^*.lrf system os2v2
	@%append $^*.lrf option quiet,caseexact
!ifdef DEBUG
	@%append $^*.lrf debug all
!endif	
	@%append $^*.lrf option map
	@%append $^*.lrf name $^.
        @for %f in ($(OBJS)) do @%append $^*.lrf file %f
        @%append $^*.lrf IMPORT DOSIODELAYCNT DOSCALLS.427
	$(LINK) @$^*.lrf
#	@%erase $^*.lrf

clean: .symbolic
    @!rm -f *.exe *.map *.obj *.lst *.lst
