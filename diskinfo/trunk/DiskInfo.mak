STATIC_LINK = defined
OPTIMIZE = defined
#DEBUG = defined
#PROFILE = defined

.INCLUDE .IGNORE: "$(MAKESTARTUP:d)STANDARD.MK"

all: DiskInfo.exe

DiskInfo.exe: DiskInfo.obj Identify.obj Dump16.obj DiskInfo.def
DiskInfo.obj: DiskInfo.c HDParm.h DiskInfo.mak
Identify.obj: Identify.c HDParm.h DiskInfo.mak

