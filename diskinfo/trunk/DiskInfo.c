#define INCL_DOS
#include <OS2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "HDParm.h"

ULONG _Optlink ReadTF (USHORT Sel, USHORT Port, PUCHAR Data);
ULONG _Optlink WriteTF (USHORT Sel, USHORT Port, PUCHAR Data);

#define MAX_ADAPTERS 16

#pragma pack(1)

#define DSKSP_CAT_SMART 	    0x80  /* SMART IOCTL category */

#define DSKSP_SMART_ONOFF	    0x20  /* turn SMART on or off */
#define DSKSP_SMART_AUTOSAVE_ONOFF  0x21  /* turn SMART autosave on or off */
#define DSKSP_SMART_SAVE	    0x22  /* force save of SMART data */
#define DSKSP_SMART_GETSTATUS	    0x23  /* get SMART status (pass/fail) */
#define DSKSP_SMART_GET_ATTRIBUTES  0x24  /* get SMART attributes table */
#define DSKSP_SMART_GET_THRESHOLDS  0x25  /* get SMART thresholds table */

#define SMART_CMD_ON	  1		  /* on value for related SMART functions */
#define SMART_CMD_OFF	  0		  /* off value for related SMART functions */

#define DSKSP_CAT_GENERIC	    0x90  /* generic IOCTL category */
#define DSKSP_GEN_GET_COUNTERS	    0x40  /* get general counter values table */
#define DSKSP_GET_UNIT_INFORMATION  0x41  /* get unit configuration and BM DMA c*/
#define DSKSP_GET_INQUIRY_DATA	    0x42  /* get ATA/ATAPI inquiry data */
#define DSKSP_GET_NATIVE_MAX	    0x43  /* get native capacity */
#define DSKSP_SET_NATIVE_MAX	    0x44  /* set report capacity */
#define DSKSP_SET_PROTECT_MBR	    0x45  /* set protection of sector 0 */
#define DSKSP_READ_SECTOR	    0x46  /* read one sector */
#define DSKSP_GET_DEVICETABLE	    0x47  /* query devicetable */

typedef struct _DSKSP_CommandParameters {
  BYTE byPhysicalUnit;		   /* physical unit number 0-n */
				   /* 0 = 1st disk, 1 = 2nd disk, ...*/
				   /* 0x80 = Pri/Mas, 0x81=Pri/Sla, 0x82=Sec/Mas,*/
} DSKSP_CommandParameters, *PDSKSP_CommandParameters;

/*
 * Parameters for SMART and generic commands
 */

/*
 * SMART Attribute table item
 */

typedef struct _S_Attribute
{
  BYTE	      byAttribID;		  /* attribute ID number */
  USHORT      wFlags;			  /* flags */
  BYTE	      byValue;			  /* attribute value */
  BYTE	      byVendorSpecific[8];	  /* vendor specific data */
} S_Attribute;

/*
 * SMART Attribute table structure
 */

typedef struct _DeviceAttributesData
{
  USHORT      wRevisionNumber;		  /* revision number of attribute table */
  S_Attribute Attribute[30];		  /* attribute table */
  BYTE	      byReserved[6];		  /* reserved bytes */
  USHORT      wSMART_Capability;	  /* capabilities word */
  BYTE	      byReserved2[16];		  /* reserved bytes */
  BYTE	      byVendorSpecific[125];	  /* vendor specific data */
  BYTE	      byCheckSum;		  /* checksum of data in this structure */
} DeviceAttributesData, NEAR *NPDeviceAttributesData, FAR *PDeviceAttributesData;

/*
 * SMART Device Threshold table item
 */

typedef struct _S_Threshold
{
  BYTE	      byAttributeID;		  /* attribute ID number */
  BYTE	      byValue;			  /* threshold value */
  BYTE	      byReserved[10];		  /* reserved bytes */
} S_Threshold;

/*
 * SMART Device Threshold table
 */

typedef struct _DeviceThresholdsData
{
  USHORT      wRevisionNumber;		  /* table revision number */
  S_Threshold Threshold[30];		  /* threshold table */
  BYTE	      byReserved[18];		  /* reserved bytes */
  BYTE	      VendorSpecific[131];	  /* vendor specific data */
  BYTE	      byCheckSum;		  /* checksum of data in this structure */
} DeviceThresholdsData, NEAR *NPDeviceThresholdsData, FAR *PDeviceThresholdsData;

/*
 * Unit Configuration and Counters
 */

typedef struct _UnitInformationData
{
  USHORT      wRevisionNumber;		  /* structure revision number */
  union {
    struct {
      USHORT  wTFBase;			  /* task file register base addr */
      USHORT  wDevCtl;			  /* device control register addr */
    } rev0;
    ULONG     dTFBase;			  /* task file register base addr */
  };
  USHORT      wIRQ;			  /* interrupt request level */
  USHORT      wFlags;			  /* flags */
  BYTE	      byPIO_Mode;		  /* PIO transfer mode programmed */
  BYTE	      byDMA_Mode;		  /* DMA transfer mode programmed */
  ULONG       UnitFlags1;
  USHORT      UnitFlags2;
} UnitInformationData, *PUnitInformationData;

/*
 * Unit Information Flags Definitions
 */

#define UIF_VALID	    0x8000	  /* unit information valid */
#define UIF_TIMINGS_VALID   0x4000	  /* timing information valid */
#define UIF_RUNNING_BMDMA   0x2000	  /* running Bus Master DMA on unit */
#define UIF_RUNNING_DMA     0x1000	  /* running slave DMA on unit */
#define UIF_SATA	    0x0004	  /* SATA     channel */
#define UIF_SLAVE	    0x0002	  /* slave on channel */
#define UIF_ATAPI	    0x0001	  /* ATAPI device if 1, ATA otherwise */

typedef struct _DeviceCountersData
{
  USHORT      wRevisionNumber;		  /* counter structure revision */
  ULONG       TotalReadOperations;	  /* total read operations performed */
  ULONG       TotalWriteOperations;	  /* total write operations performed */
  ULONG       TotalWriteErrors; 	  /* total write errors encountered */
  ULONG       TotalReadErrors;		  /* total read errors encountered */
  ULONG       TotalSeekErrors;		  /* total seek errors encountered */
  ULONG       TotalSectorsRead; 	  /* total number of sectors read */
  ULONG       TotalSectorsWritten;	  /* total number of sectors written */

  ULONG       TotalBMReadOperations;	  /* total bus master DMA read operations */
  ULONG       TotalBMWriteOperations;	  /* total bus master DMA write operations */
  ULONG       ByteMisalignedBuffers;	  /* total buffers on odd byte boundary */
  ULONG       TransfersAcross64K;	  /* total buffers crossing a 64K page boundary */
  USHORT      TotalBMStatus;		  /* total bad busmaster status */
  USHORT      TotalBMErrors;		  /* total bad busmaster error */
  ULONG       TotalIRQsLost;		  /* total lost interrupts */
  USHORT      TotalDRQsLost;		  /* total lost data transfer requests */
  USHORT      TotalBusyErrors;		  /* total device busy timeouts        */
  USHORT      TotalBMStatus2;		  /* total bad busmaster status */
  USHORT      TotalChipStatus;		  /* total bad chip status */
  USHORT      ReadErrors[4];
  USHORT      WriteErrors[2];
  USHORT      SeekErrors[2];
} DeviceCountersData, *PDeviceCountersData;

/* Identify Data */

typedef struct _IDENTIFYDATA  *PIDENTIFYDATA;

typedef struct _IDENTIFYDATA
{
  USHORT	GeneralConfig;		/*  0 General configuration bits      */
  USHORT	TotalCylinders; 	/*  1 Default Translation - Num cyl   */
  USHORT	Reserved;		/*  2 Reserved			      */
  USHORT	NumHeads;		/*  3			  - Num heads */
  USHORT	NumUnformattedbpt;	/*  4 Unformatted Bytes   - Per track */
  USHORT	NumUnformattedbps;	/*  5			  - Per sector*/
  USHORT	SectorsPerTrack;	/*  6 Default Translation - Sec/Trk   */
  USHORT	NumBytesISG;		/*  7 Byte Len - inter-sector gap     */
  USHORT	NumBytesSync;		/*  8	       - sync field	      */
  USHORT	NumWordsVUS;		/*  9 Len - Vendor Unique Info	      */
  CHAR		SerialNum[20];		/* 10 Serial number		      */
  USHORT	CtrlType;		/* 20 Controller Type		      */
  USHORT	CtrlBufferSize; 	/* 21 Ctrl buffer size - Sectors      */
  USHORT	NumECCBytes;		/* 21 ECC bytes -  read/write long    */
  CHAR		FirmwareRN[8];		/* 23 Firmware Revision 	      */
  CHAR		ModelNum[40];		/* 27 Model number		      */
  USHORT	NumSectorsPerInt;	/* 47 Multiple Mode - Sec/Blk	      */
  USHORT	DoubleWordIO;		/* 48 Double Word IO Flag	*/
  USHORT	IDECapabilities;	/* 49 Capability Flags Word	*/
  USHORT	Reserved2;		/* 50				 */
  USHORT	PIOCycleTime;		/* 51 Transfer Cycle Timing - PIO     */
  USHORT	DMACycleTime;		/* 52			    - DMA     */
  USHORT	AdditionalWordsValid;	/* 53 Additional Words valid	*/
  USHORT	LogNumCyl;		/* 54 Current Translation - Num Cyl   */
  USHORT	LogNumHeads;		/* 55			    Num Heads */
  USHORT	LogSectorsPerTrack;	/* 56			    Sec/Trk   */
  ULONG 	LogTotalSectors;	/* 57			    Total Sec */
  USHORT	LogNumSectorsPerInt;	/* 59				      */
  ULONG 	LBATotalSectors;	/* 60 LBA Mode - Sectors	      */
  USHORT	DMASWordFlags;		/* 62				      */
  USHORT	DMAMWordFlags;		/* 63				      */
  USHORT	AdvancedPIOModes;	/* 64 Advanced PIO modes supported */
  USHORT	MinMWDMACycleTime;	/* 65 Minimum multiWord DMA cycle time */
  USHORT	RecMWDMACycleTime;	/* 66 Recommended MW DMA cycle time */
  USHORT	MinPIOCycleTimeWOFC;	/* 67 Minimum PIO cycle time without IORDY */
  USHORT	MinPIOCycleTime;	/* 68 Minimum PIO cycle time	*/
  USHORT	Reserved3[82-69];	/* 69			 */
  USHORT	CommandSetSupported[3]; /* 82			 */
  USHORT	CommandSetEnabled[3];	/* 85			 */
  USHORT	UltraDMAModes;		/* 88 Ultra DMA Modes	 */
  USHORT	Reserved4[93-89];	/* 89			 */
  USHORT	HardwareTestResult;	/* 93 hardware test result*/
  USHORT	Reserved5[127-94];	/* 94			  */
  USHORT	MediaStatusWord;	/* 127 media status Word  */
  USHORT	Reserved6[256-128];	/*			  */
} IDENTIFYDATA;

typedef struct _getNativeMax {
  ULONG numSectors;
} GetNativeMax, *pGetNativeMax;

typedef struct _setNativeMax {
  BYTE	mode;				  /* 0: LBA volatile, 1: LBA, 2: CHS */
  union {
    ULONG numLBASectors;
    struct {
      BYTE   numSectors;
      USHORT numCylinders;
      BYTE   numHeads;
    } CHS;
  } value;
} SetNativeMax, *pSetNativeMax;

typedef struct _setProtMBR {
  BOOL	enable;
} SetProtectMBR, NEAR *NPSetProtectMBR, FAR *PSetProtectMBR;

typedef struct _ReadWriteSectorParameters {
  BYTE	      byPhysicalUnit;		  /* physical unit number 0-n */
					  /* 0 = Pri/Mas, 1=Pri/Sla, 2=Sec/Mas, etc. */
  ULONG       RBA;
} ReadWriteSectorParameters, NEAR *NPReadWriteSectorParameters, FAR *PReadWriteSectorParameters;

int main (int argc, char *argv[]) {
  APIRET rc;
  HFILE hDevice;
  ULONG ActionTaken;
  ULONG Options = 0;
  int i, j;
  DSKSP_CommandParameters Parms;
  ULONG PLen = 1;
  ULONG IDLen = 512;
  IDENTIFYDATA Id;
  ULONG UnitInfoLen = sizeof (UnitInformationData);
  UnitInformationData UnitInfo;
  int UnitLow = 0;
  int UnitHigh = 2 * MAX_ADAPTERS - 1;
  UCHAR Enable = FALSE;
  ULONG setNativeMax = 0;
  ULONG setNativeMaxC = 0;
  char Model[41], FirmWare[9], SerialNo[21];
  ULONG LBA = 0;
  UCHAR Cmd, Feat = 0, Opt = 0;
  UCHAR Sector[512];

  {
    PCHAR p;

    if ((p = strrchr (argv[0], '.')) != NULL)
      *p = '\0';
    if ((p = strrchr (argv[0], '\\')) != NULL)
      p++;
    else
      if ((p = strrchr (argv[0], ':')) != NULL)
	p++;
    argv[0] = p;
  }

  if (argc > 1) {
    for (i = 1; i < argc; i++) {
      if ((i == 1) && isdigit (argv[1][0])) {
	char c = atoi (argv[1]);
	char d = argv[1][strlen (argv[1]) - 1];

	if ((c >= 0) && (c < MAX_ADAPTERS)) {
	  if ((d == 'm') || (d == 'M'))
	    UnitLow = UnitHigh = c * 2;
	  else if ((d == 's') || (d == 'S'))
	    UnitLow = UnitHigh = c * 2 + 1;
	  continue;
	}
      }
      if (strchr (argv[i], 'i')) Options |= 4;
      if (strchr (argv[i], 'I')) Options |= 256;
      if (strchr (argv[i], 'c')) Options |= 2;
      if (strchr (argv[i], 'C')) Options |= 2 | 0x80000000;
      if (strchr (argv[i], 'v')) Options |= 1;
      if (strchr (argv[i], 's')) {
	Options |= 8;
	if ((i + 1) < argc)
	  if (!stricmp (argv[i+1], "1")) {
	    Enable = 1;
	    Options |= 32;
	    i += 1;
	    continue;
	  } else  if (!stricmp (argv[i+1], "0")) {
	    Enable = 0;
	    Options |= 32;
	    i += 1;
	    continue;
	  }
      }
      if (strchr (argv[i], 'm')) {
	Options |= 16;
	j = i + 1;
	if ((j < argc) && (UnitLow == UnitHigh)) {
	  if (!stricmp (argv[j], "32G")) {
	    setNativeMaxC = 65532;
	  } else if (!stricmp (argv[j], "16G")) {
	    setNativeMaxC = 32766;
	  } else if (!stricmp (argv[j], "8G")) {
	    setNativeMaxC = 16383;
	  } else if (!stricmp (argv[j], "4G")) {
	    setNativeMaxC = 8192;
	  } else if (!stricmp (argv[j], "2G")) {
	    setNativeMaxC = 4096;
	  } else if (!stricmp (argv[j], "512M")) {
	    setNativeMaxC = 1024;
	  } else if (!strnicmp (argv[j], "max", 3)) {
	    setNativeMax = (ULONG)-1;
	  } else {
	    setNativeMax = strtoul (argv[j], NULL, 10);
	  }
	  if (setNativeMax || setNativeMaxC) {
	    Options |= 32;
	    i = j;
	    continue;
	  }
	}
      }
      if (strchr (argv[i], 'p')) {
	Options |= 64;
	j = i + 1;
	if ((j < argc) && (UnitLow == UnitHigh)) {
	  Enable = atoi (argv[j]);
	  i = j;
	  continue;
	}
      }
      if (strchr (argv[i], 'r')) {
	Options |= 128;
	j = i + 1;
	if ((j < argc) && (UnitLow == UnitHigh)) {
	  LBA = strtoul (argv[j], NULL, 10);
	  i = j;
	  continue;
	}
      }
      if (strchr (argv[i], '=')) {
	if (UnitLow == UnitHigh)
	  Options |= 32768 | 0x80000000;
      }
      if (strchr (argv[i], '*')) {
	Options = 512;
      }
      if (strchr (argv[i], '!')) {
	j = i + 1;
	if ((j < argc) && (UnitLow == UnitHigh)) {
	  Cmd = strtoul (argv[j], NULL, 16);
	  Options |= 16384 | 32768 | 0x80000000;

	  if ((j + 1) < argc) {
	    j++;
	    Feat = strtoul (argv[j], NULL, 16);
	  }
	  if ((j + 1) < argc) {
	    j++;
	    Opt = strtoul (argv[j], NULL, 16);
	  }
	  i = j;
	  continue;
	}
      }
    }
    if (Options == 0) {
      printf ("Usage: %s {i}{I}{c}{v}{s}{m}{p}\n", argv[0]);
      printf ("       %s <unit> {i}{I}{c}{v}{s}{m}{p}\n", argv[0]);
      printf ("       %s <unit> s {0|1} (SMART off/on)\n", argv[0]);
      printf ("       %s <unit> m <new capacity>\n", argv[0]);
      printf ("       %s <unit> p {0|1|2} (write protect off/track 0/disk)\n", argv[0]);
      printf ("       %s <unit> r <sector number> (read sector)\n", argv[0]);
      printf ("       <unit>         = [0|1|2|3|4||5|6|7|8][m|s], e.g. '0s'\n");
      printf ("       <new capacity> = [32G|16G|8G|4G|2G|512M|any number of sectors]\n");
      exit (1);
    }
  }

  rc = DosOpen ("\\DEV\\IBMS506$", &hDevice, &ActionTaken, 0,  FILE_SYSTEM,
		 OPEN_ACTION_OPEN_IF_EXISTS, OPEN_SHARE_DENYNONE |
		 OPEN_FLAGS_NOINHERIT | OPEN_ACCESS_READONLY, NULL);
  if (rc) exit (rc);

  if (Options & 512) {
    int k;
    UCHAR *p;
    ULONG DLen = sizeof (Sector);

    printf ("DEVICE TABLE\n");

    memset (&Sector, 0, sizeof (Sector));

    rc = DosDevIOCtl (hDevice, DSKSP_CAT_GENERIC, DSKSP_GET_DEVICETABLE,
		      (PVOID)&Parms, PLen, &PLen, (PVOID)&Sector, DLen, &DLen);
    if (!rc) {
      p = Sector;
      for (j = 0; j < 512; j += 16) {
	printf ("% 4d/%03X: ", j, j);
	for (k = 0; k < 16; k++) {
	  printf ("%02X ", *(p++));
	  if ((k & 3) == 3)
	    printf (" ");
	}
	printf ("\n");
      }
      printf ("\n");

      if (argv[2] && argv[2][0]) {
	FILE *f = fopen (argv[2], "wb");
	if (f) {
	  fwrite (Sector, 1, sizeof (Sector), f);
	  fclose (f);
	}
      }
    }
    goto Close;
  }

  for (i = UnitLow; i <= UnitHigh; i++) {
    Parms.byPhysicalUnit = 0x80 + i;
    memset (&Id, 0, 512);

    rc = DosDevIOCtl (hDevice, DSKSP_CAT_GENERIC, DSKSP_GET_UNIT_INFORMATION,
		      (PVOID)&Parms, PLen, &PLen, (PVOID)&UnitInfo, UnitInfoLen, &UnitInfoLen);
    if (rc == 0xFF03) break;

    printf ("%d/%c: ", i / 2, i % 2 ? 's' : 'm');
    if (rc == 0xFF02) {
      printf ("(not present)\n");
      continue;
    }

    if (!(Options & 0x80000000)) {
      rc = DosDevIOCtl (hDevice, DSKSP_CAT_GENERIC, DSKSP_GET_INQUIRY_DATA,
			(PVOID)&Parms, PLen, &PLen, (PVOID)&Id, IDLen, &IDLen);
      if (rc == 0xFF02) {
	printf ("(not present)\n");
	continue;
      } else if (rc == 0xFF03) {
	printf ("IO error %04X Status:%02X Error:%02X\n", *(USHORT *)&Id, ((UCHAR *)&Id)[2], ((UCHAR *)&Id)[3]);
      } else {
	for (j = 0; j < 40; j++)
	  Model[j] = Id.ModelNum[j ^ 1];
	Model[40] = '\0';
	for (j = 0; j < 8; j++)
	  FirmWare[j] = Id.FirmwareRN[j ^ 1];
	FirmWare[9] = '\0';
	for (j = 0; j < 20; j++)
	  SerialNo[j] = Id.SerialNum[j ^ 1];
	SerialNo[21] = '\0';
	printf ("%-40s %-8s %-20s\n", Model, FirmWare, SerialNo);
      }
    }

    if (Options & 32768) {
      UCHAR Regs[8];

      if (Options & 16384) {
	Regs[1] = Feat;
	Regs[2] = Opt;
	Regs[7] = Cmd;
	WriteTF (i % 2 ? 0xB0 : 0xA0, UnitInfo.rev0.wTFBase, Regs + 1);
      } else {
	ReadTF (i % 2 ? 0xB0 : 0xA0, UnitInfo.rev0.wTFBase, Regs + 1);
      }
      printf ("St:%02X  Er:%02X  Sc:%02X  Sn:%02X  CL:%02X  CH:%02X  Dr:%02X",
	       Regs[7], Regs[1], Regs[2], Regs [3], Regs[4], Regs[5], Regs[6]);
      printf (" [%lX,%X]\n", UnitInfo.UnitFlags1, UnitInfo.UnitFlags2);
    }

    if (Options & 1) {
      switch (UnitInfo.wRevisionNumber) {
	case 0:
	  printf ("Port %4X/%4X, Irq %d",
		  UnitInfo.rev0.wTFBase, UnitInfo.rev0.wDevCtl, UnitInfo.wIRQ);
	  break;
	case 1:
	  printf ("Port %8lX, Irq %d", UnitInfo.dTFBase, UnitInfo.wIRQ);
	  break;
      }
      if (UnitInfo.wFlags & UIF_SATA) printf (", SATA");
      if (UnitInfo.wFlags & UIF_ATAPI) printf (", ATAPI");
      if (UnitInfo.wFlags & UIF_RUNNING_BMDMA) printf (", Busmaster");
      printf (", PIO%d", UnitInfo.byPIO_Mode);
      if (UnitInfo.wFlags & UIF_RUNNING_BMDMA)
	if (UnitInfo.byDMA_Mode > 2)
	  printf (", Ultra DMA%d", UnitInfo.byDMA_Mode - 3);
	else
	  printf (", MWord DMA%d", UnitInfo.byDMA_Mode);

      printf ("\n\n");
    }

    if ((Options & 8) && !(UnitInfo.wFlags & UIF_ATAPI)) {
      ULONG value = 0;
      ULONG DataLen;

      if (Options & 32) {
	rc = DosDevIOCtl (hDevice, DSKSP_CAT_SMART, DSKSP_SMART_ONOFF,
			  (PVOID)&Parms, PLen, &PLen, (PVOID)&Enable, 1, &DataLen);
      }

      rc = DosDevIOCtl (hDevice, DSKSP_CAT_SMART, DSKSP_SMART_GETSTATUS,
			(PVOID)&Parms, PLen, &PLen, (PVOID)&value, sizeof (value), &DataLen);
      printf ("SMART: ");
      if (rc == 0xFF03)
	printf ("not supported or disabled\n\n");
      else
	printf ("device is%s reliable\n\n", value ? " NOT" : "");
    }

    if ((Options & 2) && !(UnitInfo.wFlags & UIF_ATAPI)) {
      ULONG CountersLen = sizeof (DeviceCountersData);
      DeviceCountersData Counters;

      rc = DosDevIOCtl (hDevice, DSKSP_CAT_GENERIC, DSKSP_GEN_GET_COUNTERS,
			(PVOID)&Parms, PLen, &PLen, (PVOID)&Counters, CountersLen, &CountersLen);

      printf ("Device counters\n");
      printf ("Total operations    : %8u reads, %8u writes\n", Counters.TotalReadOperations, Counters.TotalWriteOperations);
      printf ("Total sectors       : %8u reads, %8u writes\n", Counters.TotalSectorsRead, Counters.TotalSectorsWritten);
      printf ("Busmaster operations: %8u reads, %8u writes, %8u misaligned\n",
	Counters.TotalBMReadOperations, Counters.TotalBMWriteOperations, Counters.ByteMisalignedBuffers);
      printf ("Total errors        : %5u reads, %5u writes, %5u seeks\n", Counters.TotalReadErrors, Counters.TotalWriteErrors, Counters.TotalSeekErrors);
      printf ("Total lost states   : %5u IRQs,  %5u DRQs,   %5u BUSY\n", Counters.TotalIRQsLost, Counters.TotalDRQsLost, Counters.TotalBusyErrors);
      printf ("Total bad states    : %5u BMSTA, %5u BMSTA2, %5u BMERR, %5u Chip\n", Counters.TotalBMStatus, Counters.TotalBMStatus2, Counters.TotalBMErrors, Counters.TotalChipStatus);
      printf ("Subtotal errors     : %u r0, %u r1, %u r2, %u r3, %u w0, %u w1, %u s0, %u s1\n", Counters.ReadErrors[0], Counters.ReadErrors[1], Counters.ReadErrors[2], Counters.ReadErrors[3], Counters.WriteErrors[0], Counters.WriteErrors[1], Counters.SeekErrors[0], Counters.SeekErrors[1]);
      printf ("\n");
    }

    if ((Options & 16) && !(UnitInfo.wFlags & UIF_ATAPI) && (Id.CommandSetSupported[0] & 0x0400)) {
      ULONG MaxSectors = (ULONG)-1;
      ULONG DLen = sizeof (MaxSectors);

      rc = DosDevIOCtl (hDevice, DSKSP_CAT_GENERIC, DSKSP_GET_NATIVE_MAX,
			(PVOID)&Parms, PLen, &PLen, (PVOID)&MaxSectors, DLen, &DLen);

      if (!rc && (MaxSectors != (ULONG)-1))
	printf ("Disk native capacity:        %ld sectors\nCurrently reported capacity: %ld sectors\n", MaxSectors, Id.LBATotalSectors);

      if (Options & 32) {
	SetNativeMax set;
	IDENTIFYDATA newId;

	if (setNativeMaxC)
	  setNativeMax = setNativeMaxC * Id.NumHeads * Id.SectorsPerTrack;
	if (setNativeMax > MaxSectors) setNativeMax = MaxSectors;

	printf ("Set new reported capacity to %ld sectors", setNativeMax);
	set.mode = 1;
	set.value.numLBASectors = setNativeMax;
	DLen = sizeof (set);

	rc = DosDevIOCtl (hDevice, DSKSP_CAT_GENERIC, DSKSP_SET_NATIVE_MAX,
			  (PVOID)&Parms, PLen, &PLen, (PVOID)&set, DLen, &DLen);

	DosDevIOCtl (hDevice, DSKSP_CAT_GENERIC, DSKSP_GET_INQUIRY_DATA,
		    (PVOID)&Parms, PLen, &PLen, (PVOID)&newId, IDLen, &IDLen);

	if (!rc) rc = setNativeMax != newId.LBATotalSectors;
	if (rc)
	  printf (" - failed!\n");
	else
	  printf (" - done\n");

	set.mode = 0;
	set.value.numLBASectors = Id.LBATotalSectors;
	rc = DosDevIOCtl (hDevice, DSKSP_CAT_GENERIC, DSKSP_SET_NATIVE_MAX,
			  (PVOID)&Parms, PLen, &PLen, (PVOID)&set, DLen, &DLen);

	rc = DosDevIOCtl (hDevice, DSKSP_CAT_GENERIC, DSKSP_GET_INQUIRY_DATA,
			  (PVOID)&Parms, PLen, &PLen, (PVOID)&newId, IDLen, &IDLen);
      }

      printf ("\n");
    }

    if (Options & 64) {
      ULONG DataLen = sizeof (Enable);

      rc = DosDevIOCtl (hDevice, DSKSP_CAT_GENERIC, DSKSP_SET_PROTECT_MBR,
			(PVOID)&Parms, PLen, &PLen, (PVOID)&Enable, DataLen, &DataLen);
      printf ("Write protection: %s\n\n", Enable ? (Enable > 1 ? "disk" : "track 0") : "off");
    }

    if (Options & 4) {
      int k;
      USHORT *p;

      printf ("%sIDENTIFY response\n", (UnitInfo.wFlags & UIF_ATAPI) ? "ATAPI " : "");

      p = (USHORT *)&Id;
      for (j = 0; j < 256; j += 8) {
	printf ("% 4d/%03X: ", j, j);
	for (k = 0; k < 8; k++) {
	  printf ("%04X ", *(p++));
	  if (k == 3)
	    printf (" ");
	}
	printf ("\n");
      }
      printf ("\n");
    }

    if (Options & 256) {
//	printf ("%sIDENTIFY response\n", (UnitInfo.wFlags & UIF_ATAPI) ? "ATAPI " : "");
      Identify ((USHORT *)&Id);
      printf ("\n");
    }

    if (Options & 128) {
      int k;
      UCHAR *p;
      ULONG DLen = sizeof (Sector);
      ReadWriteSectorParameters RParms;
      ULONG RLen = sizeof (RParms);

      printf ("Sector %lu/%lXh data\n", LBA, LBA);

      RParms.byPhysicalUnit = Parms.byPhysicalUnit;
      RParms.RBA = LBA;

      rc = DosDevIOCtl (hDevice, DSKSP_CAT_GENERIC, DSKSP_READ_SECTOR,
			(PVOID)&RParms, RLen, &RLen, (PVOID)&Sector, DLen, &DLen);
      if (!rc) {
	p = Sector;
	for (j = 0; j < 512; j += 16) {
	  printf ("% 4d/%03X: ", j, j);
	  for (k = 0; k < 16; k++) {
	    printf ("%02X ", *(p++));
	    if ((k & 3) == 3)
	      printf (" ");
	  }
	  printf ("\n");
	}
	printf ("\n");
      } else {
	printf ("IO error %04X Status:%02X Error:%02X\n", *(USHORT *)Sector, Sector[2], Sector[3]);
      }
    }

  }

Close:
  DosClose (hDevice);

  if (Options == 0)
    printf ("\nmore info with %s {?}{i}{I}{c}{v}{s}{m}{p}\n", argv[0]);

  return (rc);
}
