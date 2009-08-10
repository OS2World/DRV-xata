/**************************************************************************
 *
 * SOURCE FILE NAME = S506DATA.C
 *
 * DESCRIPTIVE NAME = DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert, 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 *****************************************************************************/

 #define INCL_NOPMAPI
 #define INCL_NO_SCB
 #define INCL_DOSINFOSEG
 #include "os2.h"
 #include "dskinit.h"

 #include "iorb.h"
 #include "strat2.h"
 #include "reqpkt.h"
 #include "dhcalls.h"
 #include "addcalls.h"
 #include "pcmcia.h"
 #include <stddef.h>

 #include "s506cons.h"
 #include "s506type.h"
 #include "s506regs.h"
 #include "s506ext.h"
 #include "s506pro.h"

#define YEAR  2009
#define MONTH 5
#define DAY   25
#define PCMCIAVERSION 0x186

/*-------------------------------------------------------------------*/
/*								     */
/*	Static Data						     */
/*								     */
/*								     */
/*-------------------------------------------------------------------*/

NPA	       ACBPtrs[MAX_ADAPTERS] = { 0 };
NPA	       SocketPtrs[MAX_ADAPTERS] = { 0 };
NPA	       PCCardPtrs[MAX_ADAPTERS] = { 0 };
IHDRS	       IHdr[MAX_IRQS]	     = { { 0, &FixedIRQ0 },
					 { 0, &FixedIRQ1 },
					 { 0, &FixedIRQ2 },
					 { 0, &FixedIRQ3 },
					 { 0, &FixedIRQ4 },
					 { 0, &FixedIRQ5 },
					 { 0, &FixedIRQ6 },
					 { 0, &FixedIRQ7 } };

PFN	       Device_Help	      = 0L;
PFN	       RM_Help0 	      = 0l;
PFN	       RM_Help3 	      = 0l;
ULONG	       RMFlags		      = 0;
ULONG	       ppDataSeg	      = 0L;
HDRIVER        hDriver		      = 0L;
USHORT	       ADDHandle	      = 0;
UCHAR	       cChips		      = 0;
UCHAR	       cAdapters	      = 0;
UCHAR	       cUnits		      = 0;
UCHAR	       cSockets 	      = 0;
UCHAR	       DriveId		      = 0;
UCHAR	       InitActive	      = 0;
UCHAR	       InitIOComplete	      = 0;
UCHAR	       InitComplete	      = 0;
UCHAR	       BIOSActive	      = 1;
UCHAR	       BIOSInt13	      = 1;
UCHAR	       EzDrivePresent	      = 0;
PGINFOSEG      pGlobal		      = 0;
volatile SHORT _far *msTimer	      = 0;
struct DevClassTableStruc _far *pDriverTable = NULL;
USHORT	       IODelayCount	      = 0;
USHORT	       ElapsedTimerHandle     = 0;
USHORT	       IdleTickerHandle       = 0;
UCHAR	       PCInumBuses	      = 1;
UCHAR	       PCIClock 	      = 33;
UCHAR	       PCIClockIndex	      = 0;
USHORT	       Debug		      = 0;
PCHAR	       WritePtr 	      = 0;
USHORT	       WritePtrRestart	      = 0;
USHORT	       WritePtrErr	      = 0;
PCHAR	       ReadPtr		      = 0;
CHAR	       Beeps		      = -1;
UCHAR	       LinesPrinted	      = 0;
UCHAR	       TracerEnabled	      = 0;
UCHAR	       TraceErrors	      = 0;
UCHAR	       NotShutdown	      = 1;
UCHAR	       PowerState	      = 0;
USHORT	       Fixes		      = 0;
NPPCI_INFO     CurPCIInfo	      = 0;
UCHAR	       CurLevel 	      = 0;
UCHAR	       RegOffset	      = 0;

NPU	       LastAccessedUnit       = NULL;
NPA	       LastAccessedPCCardUnit = NULL;
#if AUTOMOUNT
PStrat1Entry   OS2LVM		      = NULL;
#endif
USHORT	       UStatus		      = 0;
struct GTD_P   GetTuple 	      = { 0 };

UCHAR AdapterNameATAPI[17] = "ATA_ATAPI       "; /* Adapter Name ASCIIZ string */
UCHAR AdapterNamePCCrd[17] = "PCMCIA ATA drive"; /* Adapter Name ASCIIZ string */

UCHAR CmdTable[2*12] = {
  FX_CREAD	, FX_CWRITE	 , FX_VERIFY	, FX_VERIFY,
  FX_CREADMUL	, FX_CWRITEMUL	 , FX_VERIFY	, FX_VERIFY,
  FX_DMAREAD	, FX_DMAWRITE	 , FX_DMAREAD	, FX_DMAWRITE,
  FX_CREADEXT	, FX_CWRITEEXT	 , FX_VERIFYEXT , FX_VERIFYEXT,
  FX_CREADMULEXT, FX_CWRITEMULEXT, FX_VERIFYEXT , FX_VERIFYEXT,
  FX_DMAREADEXT , FX_DMAWRITEEXT , FX_DMAREADEXT, FX_DMAWRITEEXT
};

UCHAR ALI_ModeSets[4][3] = {
  { 0x0C, 0x33, 0x31 },   /* 30/33 Mhz PCI clock timings */
  { 0x07, 0x32, 0x21 },   /* 25 Mhz PCI clock timings */
  { 0x0F, 0x43, 0x32 },   /* 37 Mhz PCI clock timings */
  { 0x00, 0x44, 0x32 }	  /* 41 Mhz PCI clock timings */
};

UCHAR ALI_UltraModeSets[4][5] = {
  { 0x04, 0x03, 0x02, 0x01, 0x00 }, /* 30/33 Mhz PCI clock timings */
  { 0x03, 0x05, 0x01, 0x01, 0x00 }, /* 25 Mhz PCI clock timings */
  { 0x06, 0x04, 0x05, 0x02, 0x01 }, /* 37 Mhz PCI clock timings */
  { 0x06, 0x04, 0x05, 0x02, 0x01 }  /* 41 Mhz PCI clock timings */
};

UCHAR ALI_Ultra100ModeSets[7] = {
  0x04, 0x03, 0x02, 0x01, 0x00, 0x07, 0x05  /* 30/33 Mhz PCI clock timings */
};

UCHAR CMD_ModeSets[4][3] = {
  { 0x8B, 0x32, 0x3F },   /* 30/33 Mhz PCI clock timings */
  { 0x68, 0x31, 0x2F },   /* 25 Mhz PCI clock timings */
  { 0x9D, 0x42, 0x31 },   /* 37 Mhz PCI clock timings */
  { 0xAE, 0x43, 0x31 }	  /* 41 Mhz PCI clock timings */
};

UCHAR CMD_UltraModeSets[4][6] = {
  { 0x30, 0x20, 0x10, 0x20, 0x10, 0x00 }, /* 30/33 Mhz PCI clock timings */
  { 0x20, 0x20, 0x10, 0x20, 0x10, 0x00 }, /* 25 Mhz PCI clock timings */
  { 0x30, 0x30, 0x20, 0x30, 0x20, 0x10 }, /* 37 Mhz PCI clock timings */
  { 0x30, 0x30, 0x20, 0x30, 0x20, 0x10 }  /* 41 Mhz PCI clock timings */
};

ULONG CX_PIOModeSets[4][3] = {
  { 0x00009172, 0x00032010, 0x00040010 },   /* 30/33 Mhz PCI clock timings */
  { 0x00006151, 0x00031020, 0x00040000 },   /* 25 Mhz PCI clock timings */
  { 0x00009292, 0x00032111, 0x00041010 },   /* 37 Mhz PCI clock timings */
  { 0x0000A2A2, 0x00033111, 0x00041010 }    /* 41 Mhz PCI clock timings */
};

ULONG CX_DMAModeSets[4][5] = {
  { 0x00012121, 0x00002020, 0x00921250, 0x00911140, 0x00911030 }, /* 30/33 Mhz PCI clock timings */
  { 0x00011110, 0x00002010, 0x00911130, 0x00911030, 0x00901020 }, /* 25 Mhz PCI clock timings */
  { 0x00022222, 0x00012120, 0x00921350, 0x00911140, 0x00911130 }, /* 37 Mhz PCI clock timings */
  { 0x00023232, 0x00012120, 0x00921360, 0x00911250, 0x00911140 }  /* 41 Mhz PCI clock timings */
};

ULONG HPT_ModeSets[3][3] = {
  { 0x00D0A7AA, 0x00C8A742, 0x00C8A731 },   /* 30/33 Mhz PCI clock timings */
  { 0x00D08585, 0x00CA8532, 0x00CA8521 },   /* 25 Mhz PCI clock timings */
  { 0x0018D9D9, 0x0010D974, 0x0008D963 }    /* 40 Mhz PCI clock timings */
};

UCHAR HPT_UltraModeSets[3][6] = {
  { 0, 3, 2, 7, 1, 1 },      /* 30/33 Mhz PCI clock timings */
  { 3, 3, 7, 7, 1, 1 },      /* 25 Mhz PCI clock timings */
  { 0, 0, 3, 2, 7, 7 }	     /* 40 Mhz PCI clock timings */
};

ULONG HPT37x_ModeSets[3][3] = {
  { 0x06814EA7, 0x06414E42, 0x06414E31 },  /* 30/33 Mhz PCI clock timings */
  { 0x0AC1F48A, 0x0A81F443, 0x0A81F442 },  /* 50 Mhz PCI clock timings */
  { 0x0D029D5E, 0x0C829C84, 0x0C829C62 }   /* 66 Mhz PCI clock timings */
};

UCHAR HPT37x_UltraModeSets[3][7] = {
  { 4, 3, 2,8+3,1, 1, 1 },    /* 30/33 Mhz PCI clock timings */
  { 6, 5, 3, 3,8+3,1, 1 },    /* 50 Mhz PCI clock timings */
  { 0, 6, 4, 3, 2, 2, 1 }     /* 66 Mhz PCI clock timings */
};

USHORT PDC_ModeSets[3]	  = { 0x1309, 0x0602, 0x0401 };
USHORT PDC_100ModeSets[3] = { 0x1FF3, 0x0925, 0x0622 };
USHORT PDC_133ModeSets[3] = { 0x2BFB, 0x0D27, 0x0923 };

USHORT PDC_DMAModeSets[5]    = { 0x0460, 0x0360, 0x0360, 0x0240, 0x0120 };
USHORT PDC_DMA66ModeSets[8]  = { 0x0460, 0x0360, 0x07E0, 0x05A0, 0x0360, 0x0240, 0x0120, 0x0120 };
USHORT PDC_DMA100ModeSets[8] = { 0x2568, 0x2347, 0xB04B, 0xAC37, 0xAA25, 0xAA14, 0xAA12, 0xA811 };
USHORT PDC_DMA133ModeSets[9] = { 0x276B, 0x2569, 0xD54F, 0xD03A, 0xCD27, 0xCD15, 0xCD13, 0xCB12, 0xCB11 };

USHORT SII_ModeSets[3]	   = { 0x328A, 0x10C3, 0x10C1 };
USHORT SII_MDMAModeSets[2] = { 0x10C2, 0x10C1 };

UCHAR AEC62X0_ModeSets[3] = { 0x7A, 0x33, 0x31 };

UCHAR PIIX_ModeSets[4][3] = {
  { 0x80, 0xA1, 0xA3 },   /* 30/33 Mhz PCI clock timings */
  { 0x80, 0xA2, 0xB3 },   /* 25 Mhz PCI clock timings */
  { 0x80, 0x91, 0xA2 },   /* 37 Mhz PCI clock timings */
  { 0x80, 0x90, 0xA2 }	  /* 41 Mhz PCI clock timings */
};

UCHAR PIIX_UltraModeSets[4][7] = {
  { 0x00, 0x01, 0x02, 0x11, 0x12, 0x21, 0x22 },   /* 30/33 Mhz PCI clock timings */
  { 0x01, 0x01, 0x02, 0x11, 0x12, 0x21, 0x22 },   /* 25 Mhz PCI clock timings */
  { 0x00, 0x00, 0x01, 0x11, 0x11, 0x21, 0x22 },   /* 37 Mhz PCI clock timings */
  { 0x00, 0x00, 0x01, 0x11, 0x11, 0x21, 0x22 }	  /* 41 Mhz PCI clock timings */
};

UCHAR SMSC_UltraModeSets[4][7] = {
  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x04 },   /* 30/33 Mhz PCI clock timings */
  { 0x01, 0x01, 0x02, 0x03, 0x04, 0x04, 0x04 },   /* 25 Mhz PCI clock timings */
  { 0x00, 0x00, 0x01, 0x02, 0x03, 0x03, 0x03 },   /* 37 Mhz PCI clock timings */
  { 0x00, 0x00, 0x01, 0x02, 0x03, 0x03, 0x03 }	  /* 41 Mhz PCI clock timings */
};

USHORT SIS_ModeSets[4][3] = {
  { 0x000C, 0x0333, 0x0331 },	/* 30/33 Mhz PCI clock timings */
  { 0x0669, 0x0332, 0x0221 },	/* 25 Mhz PCI clock timings */
  { 0x077B, 0x0443, 0x0332 },	/* 37 Mhz PCI clock timings */
  { 0x077C, 0x0444, 0x0332 }	/* 41 Mhz PCI clock timings */
};

UCHAR SIS_UltraModeSets[4][6] = {
  { 0xE0, 0xC0, 0xA0, 0xA0, 0xA0, 0xA0 },   /* 30/33 Mhz PCI clock timings */
  { 0xC0, 0xC0, 0xA0, 0xA0, 0xA0, 0xA0 },   /* 25 Mhz PCI clock timings */
  { 0xE0, 0xE0, 0xC0, 0xC0, 0xC0, 0xC0 },   /* 37 Mhz PCI clock timings */
  { 0xE0, 0xE0, 0xC0, 0xC0, 0xC0, 0xC0 }    /* 41 Mhz PCI clock timings */
};

UCHAR SIS_Ultra66ModeSets[4][6] = {
  { 0xF0, 0xD0, 0xB0, 0xA0, 0x90, 0x80 }, /* 30/33 Mhz PCI clock timings */
  { 0xD0, 0xC0, 0xA0, 0xA0, 0x90, 0x80 }, /* 25 Mhz PCI clock timings */
  { 0xF0, 0xE0, 0xE0, 0xB0, 0xA0, 0x90 }, /* 37 Mhz PCI clock timings */
  { 0xF0, 0xF0, 0xE0, 0xB0, 0xA0, 0x90 }  /* 41 Mhz PCI clock timings */
};

UCHAR SIS_Ultra100ModeSets[6] = {
    0x8B, 0x87, 0x85, 0x84, 0x82, 0x81	      /* common timings */
};

UCHAR SIS_Ultra133ModeSets[7] = {
    0x8F, 0x8B, 0x87, 0x85, 0x83, 0x82, 0x81  /* common timings */
};

ULONG SIS2_100ModeSets[3] = {
    0x1E1C6000, 0x09072002, 0x04062002	      /* 100 MHz timings */
};

ULONG SIS2_133ModeSets[3] = {
    0x28269008, 0x0C0A300A, 0x0509300A	      /* 133 MHz timings */
};

USHORT SIS2_UltraModeSets[7] = {
    0x6B4, 0x474, 0x354, 0x144, 0x124, 0x114, 0x21C  /* common timings */
};

UCHAR VIA_ModeSets[4][3] = {
  { 0xA8, 0x22, 0x20 },   /* 30/33 Mhz PCI clock timings */
  { 0xA8, 0x21, 0x10 },   /* 25 Mhz PCI clock timings */
  { 0xC9, 0x32, 0x21 },   /* 37 Mhz PCI clock timings */
  { 0xDA, 0x33, 0x21 }	  /* 41 Mhz PCI clock timings */
};

UCHAR VIA_UltraModeSets[4][7] = {
  { 0xC2, 0xC1, 0xC0, 0xC4, 0xC5, 0xC6, 0xC7 },   /* 30/33 Mhz PCI clock timings */
  { 0xC1, 0xC1, 0xC0, 0xC4, 0xC5, 0xC6, 0xC7 },   /* 25 Mhz PCI clock timings */
  { 0xC3, 0xC2, 0xC1, 0xC0, 0xC4, 0xC5, 0xC5 },   /* 37 Mhz PCI clock timings */
  { 0xC3, 0xC2, 0xC1, 0xC0, 0xC4, 0xC5, 0xC5 }	  /* 41 Mhz PCI clock timings */
};

UCHAR VIA_Ultra66ModeSets[4][7] = {
  { 0xC6, 0xC4, 0xC2, 0xC1, 0xC0, 0xC0, 0xC0 },   /* 30/33 Mhz PCI clock timings */
  { 0xC4, 0xC3, 0xC1, 0xC1, 0xC0, 0xC0, 0xC0 },   /* 25 Mhz PCI clock timings */
  { 0xC7, 0xC5, 0xC3, 0xC2, 0xC1, 0xC1, 0xC1 },   /* 37 Mhz PCI clock timings */
  { 0xC7, 0xC6, 0xC3, 0xC2, 0xC1, 0xC1, 0xC1 }	  /* 41 Mhz PCI clock timings */
};

UCHAR VIA_Ultra100ModeSets[4][7] = {
  { 0xC7, 0xC6, 0xC4, 0xC2, 0xC1, 0xC0, 0xC0 },   /* 30/33 Mhz PCI clock timings */
  { 0xC7, 0xC5, 0xC3, 0xC2, 0xC1, 0xC0, 0xC0 },   /* 25 Mhz PCI clock timings */
  { 0xC7, 0xC7, 0xC5, 0xC3, 0xC2, 0xC1, 0xC1 },   /* 37 Mhz PCI clock timings */
  { 0xC7, 0xC7, 0xC6, 0xC4, 0xC2, 0xC1, 0xC1 }	  /* 41 Mhz PCI clock timings */
};

UCHAR VIA_Ultra133ModeSets[4][7] = {
  { 0xCE, 0xC8, 0xC6, 0xC4, 0xC2, 0xC1, 0xC0 },   /* 30/33 Mhz PCI clock timings */
  { 0xCA, 0xC6, 0xC4, 0xC3, 0xC1, 0xC0, 0xC0 },   /* 25 Mhz PCI clock timings */
  { 0xCF, 0xCA, 0xC7, 0xC5, 0xC3, 0xC1, 0xC1 },   /* 37 Mhz PCI clock timings */
  { 0xCF, 0xCC, 0xC8, 0xC6, 0xC3, 0xC2, 0xC1 }	  /* 41 Mhz PCI clock timings */
};

UCHAR OSB_ModeSets[4][3] = {
  { 0x5D, 0x22, 0x20 },   /* 30/33 Mhz PCI clock timings */
  { 0x49, 0x21, 0x10 },   /* 25 Mhz PCI clock timings */
  { 0x6F, 0x32, 0x21 },   /* 37 Mhz PCI clock timings */
  { 0x8F, 0x33, 0x21 }	  /* 41 Mhz PCI clock timings */
};

UCHAR OSB_UltraModeSets[4][7] = {
  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 },   /* 30/33 Mhz PCI clock timings */
  { 0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 },   /* 25 Mhz PCI clock timings */
  { 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 },   /* 37 Mhz PCI clock timings */
  { 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 }	  /* 41 Mhz PCI clock timings */
};

USHORT OPTI_ModeSets[2][3] = {
  { 0x2059, 0x1020, 0x0020 },	/* 30/33 Mhz PCI clock timings */
  { 0x1046, 0x0020, 0x0010 },	/* 25 Mhz PCI clock timings */
};

UCHAR ITE_ModeSets[2][3] = {
  { 0xAA, 0x33, 0x31 },   /* 66 Mhz timings */
  { 0x88, 0x32, 0x21 },   /* 50 Mhz timings */
};

UCHAR ITE_UltraModeSets[2][7] = {
  { 0x44, 0x42, 0x31, 0x21, 0x11, 0xA2, 0x91 },   /* 66 Mhz timings */
  { 0x33, 0x31, 0x21, 0x21, 0x11, 0x91, 0x91 },   /* 50 Mhz timings */
};


/*-------------------------------------------------------------------*/
/*								     */
/*	PCI Configuration Operations				     */
/*								     */
/*								     */
/*-------------------------------------------------------------------*/

USHORT CfgICH[]     = {0x254};
USHORT CfgPIIX4[]   = {0x148, 0x24A};
USHORT CfgPIIX3[]   = {0x144};
USHORT CfgPIIX[]    = {0x440};
USHORT CfgGeneric[] = {0x204, 0x20C, 0x8404, 0x840C};
USHORT CfgNull[]    = {0};
USHORT CfgVIA[]     = {0x204, 0x20C, 0x8404, 0x840C, 0x440, 0x448, 0x44C, 0x450, 0x444, 0}; //44 needs to be last!
USHORT CfgALI[]     = {0x204, 0x20C, 0x8404, 0x840C, 0x24A, 0x153, 0x454, 0x458, 0x45C, 0};
USHORT CfgPDC[]     = {0x204, 0x20C, 0x8404, 0x840C, 0x460, 0x464, 0x468, 0x46C, 0x8111, 0x821A, 0x811F, 0};
USHORT CfgHPT37x[]  = {0x254, 0x25A};
USHORT CfgHPT36x[]  = {0x44C, 0x250};
USHORT CfgSIS[]     = {0x204, 0x20C, 0x8404, 0x840C, 0x440, 0x444, 0x448, 0};
USHORT CfgSIS2[]    = {0x204, 0x20C, 0x8404, 0x840C, 0x450, 0x157, 0x470, 0x474, 0x478, 0x47C, 0};
USHORT CfgCMD[]     = {0x204, 0x20C, 0x8404, 0x840C, 0x450, 0x454, 0x458, 0x171, 0x173, 0x179, 0x17B, 0};
USHORT CfgCX[]	    = {0x204, 0x20C, 0x8404, 0x840C, 0x8420, 0x8424, 0x8428, 0x842C, 0x8430, 0x8434, 0x8438, 0x843C, 0};
USHORT CfgOSB[]     = {0x248, 0x24A, 0x256};
USHORT CfgAEC[]     = {0x204, 0x20C, 0x8404, 0x840C, 0x440, 0x444, 0x154, 0};
USHORT CfgOPTI[]    = {0x204, 0x20C, 0x8404, 0x840C, 0x440, 0x246, 0};
USHORT CfgNFC[]     = {0x204, 0x20C, 0x8404, 0x840C, 0x450, 0x458, 0x45C, 0x460, 0};
USHORT CfgSII[]     = {0x204, 0x20C, 0x8404, 0x840C, 0x180, 0x18A, 0x2A2, 0x4A4, 0x4A8, 0x04AC, 0x2B2, 0x4B4, 0x4B8, 0x04BC, 0};
USHORT CfgITE[]     = {0x204, 0x20C, 0x8404, 0x840C, 0x240, 0x150, 0x454, 0x458, 0};

USHORT CListNULL[] =  { 0 };
USHORT CListCIntel[] = {
	0x7199, 0x7111,     // PIIX4 -> PIIX4
	0x7601, 0x2411,     // ICHx  -> ICH
	0x244A, 0x244B,     // ICH2M -> ICH2
	0x245B, 0x244B,     // C-ICH -> ICH2
	0x248A, 0x248B,     // ICH3M -> ICH3
	0x24CA, 0x24CB,     // ICH4M -> ICH4
	0x24C1, 0x24CB,     // ICH4L -> ICH4
	0x25A2, 0x24DB,     // ICH6300 -> ICH5
	0x269E, 0x266F,     // ICH631x -> ICH6
	0x24DF, 0x24D1,     // ICH5R SATA -> ICH5 SATA
	0x25A3, 0x24D1,     // ICH5? SATA -> ICH5 SATA
	0x25B0, 0x24D1,     // ICH5?R SATA -> ICH5 SATA
	0x2652, 0x2651,     // ICH6R SATA -> ICH6 SATA
	0x2653, 0x2651,     // ICH6M SATA -> ICH6 SATA
	0x2680, 0x2651,     // ICH631x SATA -> ICH6 SATA
	0x2681, 0x2651,     // ICH631x AHCI -> ICH6 SATA
	0x2682, 0x2651,     // ICH631x RAID -> ICH6 SATA
	0x2683, 0x2651,     // ICH631x RAID -> ICH6 SATA
	0x27C1, 0x27C0,     // ICH7  AHCI -> ICH7 SATA
	0x27C3, 0x27C0,     // ICH7R SATA -> ICH7 SATA
	0x27C4, 0x27C0,     // ICH7M SATA -> ICH7 SATA
	0x27C5, 0x27C0,     // ICH7M AHCI -> ICH7 SATA
	0x27C6, 0x27C0,     // ICH7MR AHCI -> ICH7 SATA
	0x2821, 0x2820,     // ICH8  AHCI -> ICH8 SATA
	0x2822, 0x2820,     // ICH8  RAID -> ICH8 SATA
	0x2824, 0x2820,     // ICH8  AHCI -> ICH8 SATA
	0x2828, 0x2820,     // ICH8M SATA -> ICH8 SATA
	0x2829, 0x2820,     // ICH8M AHCI -> ICH8 SATA
	0x282A, 0x2820,     // ICH8M RAID -> ICH8 SATA
	0x2921, 0x2920,     // ICH9 SATA2 2p -> ICH9 SATA
	0x2922, 0x2920,     // ICH9 AHCI  -> ICH9 SATA
	0x2923, 0x2920,     // ICH9 AHCI2 -> ICH9 SATA
	0x2924, 0x2920,     // ICH9 AHCI3 -> ICH9 SATA
	0x2925, 0x2920,     // ICH9 AHCI4 -> ICH9 SATA
	0x2926, 0x2920,     // ICH9 SATA3 2p -> ICH9 SATA
	0x2927, 0x2920,     // ICH9 AHCI5 -> ICH9 SATA
	0x2928, 0x2920,     // ICH9MSATA  2p -> ICH9 SATA
	0x2929, 0x2920,     // ICH9MAHCI  -> ICH9 SATA
	0x292A, 0x2920,     // ICH9MAHCI2 -> ICH9 SATA
	0x292B, 0x2920,     // ICH9MAHCI3 -> ICH9 SATA
	0x292D, 0x2920,     // ICH9MSATA2 2p -> ICH9 SATA
	0x292E, 0x2920,     // ICH9MSATA3 1p -> ICH9 SATA
	0x292F, 0x2920,     // ICH9MAHCI4 -> ICH9 SATA
	0x294D, 0x2920,     // ICH9 AHCI6 -> ICH9 SATA
	0x294E, 0x2920,     // ICH9MAHCI5 -> ICH9 SATA
	0x2996, 0x2986,     // IDER -> IDER
	0x29A6, 0x2986,     // IDER -> IDER
	0x29B6, 0x2986,     // IDER -> IDER
	0x29C6, 0x2986,     // IDER -> IDER
	0x29D6, 0x2986,     // IDER -> IDER
	0x29E6, 0x2986,     // IDER -> IDER
	0x29F6, 0x2986,     // IDER -> IDER
	0x2A06, 0x2986,     // IDER -> IDER
	0x2A16, 0x2986,     // IDER -> IDER
	0x2A52, 0x2986,     // IDER -> IDER
	0x2E06, 0x2986,     // IDER -> IDER
	0x5028, 0x2920,     // Tolapai SATA  -> ICH9 SATA
	0x502A, 0x2920,     // Tolapai AHCI  -> ICH9 SATA
	0x502B, 0x2920,     // Tolapai AHCI2 -> ICH9 SATA
	0x3A20, 0x3A00,     // ICH10 SATA2 4p -> ICH10 SATA 4p
	0x3A26, 0x3A06,     // ICH10 SATA2 2p -> ICH10 SATA 2p
	0x3A05, 0x3A00,     // ICH10 AHCI  -> ICH10 SATA
	0x3A25, 0x3A00,     // ICH10 AHCI2 -> ICH10 SATA
	0x3B20, 0x3A00,     // PCH SATA  4p -> ICH10 SATA 4p
	0x3B21, 0x3A06,     // PCH SATA  2p -> ICH10 SATA 2p
	0x3B24, 0x3A00,     // PCH SATA RAID-> ICH10 SATA 4p
	0x3B25, 0x3A00,     // PCH SATA RAID-> ICH10 SATA 4p
	0x3B26, 0x3A06,     // PCH SATA  2p -> ICH10 SATA 2p
	0x3B28, 0x3A00,     // PCH SATA  4p -> ICH10 SATA 4p
	0x3B2B, 0x3A00,     // PCH SATA RAID-> ICH10 SATA 4p
	0x3B2C, 0x3A00,     // PCH SATA RAID-> ICH10 SATA 4p
	0x3B2D, 0x3A06,     // PCH SATA2 2p -> ICH10 SATA 2p
	0x3B2E, 0x3A00,     // PCH SATA2 4p -> ICH10 SATA 4p
		0 };
USHORT CListFIntel[] = {
		0x1230,     // PIIX	  82371FB
		0x7010,     // PIIX3	  82371SB
		0x7111,     // PIIX4	  82371AB/EB/MB
		0x2411,     // ICH	  82801AA
		0x2421,     // ICH0	  82801AB
		0x244B,     // ICH2	  82801BA
		0x248B,     // ICH3	  82801CA
		0x24CB,     // ICH4	  82801DB
		0x24DB,     // ICH5	  82801EB
		0x24D1,     // ICH5 SATA  82801EB
		0x266F,     // ICH6	  82801FB
		0x2651,     // ICH6 SATA  82801FB
		0x27DF,     // ICH7	  82801GB
		0x27C0,     // ICH7 SATA  82801GB
		0x2820,     // ICH8 SATA  82801HB 4 ports
		0x2825,     // ICH8 SATA2 82801HB 2 ports
		0x2850,     // ICH8M	  82801HBM
		0x2920,     // ICH9 SATA	  4p
		0x2986,     // IDE Redirection
		0x3A00,     // ICH10 SATA	  4p
		0x3A06,     // ICH10 SATA	  2p
		0 };
USHORT CListFiSCH[] = {
		0x811A,     // SCH
		0 };
USHORT CListCVia[] = {
	0x4149, 0x3149,     // VT6420	-> VT8237 SATA
	0x0581, 0x3149,     // CX/VX700 -> VT8237 SATA
	0x0591, 0x3149,     // VT8237A	-> VT8237 SATA
	0x5287, 0x3149,     // VT8237?	-> VT8237 SATA
	0x5337, 0x3149,     // VT8237A	-> VT8237 SATA
	0x5372, 0x3149,     // VT8237?	-> VT8237 SATA
	0x7353, 0x3149,     // CX/VX800 -> VT8237?SATA
	0x7372, 0x3149,     // VT8237S	-> VT8237 SATA
	0x5287, 0x3349,     // VT8251	-> VT8251 SATA ?
	0x6287, 0x3349,     // VT8251	-> VT8251 SATA
	0x7800, 0x3349,     // SB900	-> VT8251 SATA
	0x9000, 0x3349,     // VT8261	-> VT8251 SATA
	0x9040, 0x3349,     // VT8261	-> VT8251 SATA
		0 };
USHORT CListFVia[] = {
		0x0571,     // VIA 571
		0x1571,     // old VIA
		0x3149,     // VT8237 SATA
		0x3164,     // VT6410 PATA
		0x3249,     // VT6421 SATA/PATA
		0x3349,     // VT8251 SATA
		0x5324,     // CX700  PATA
		0xC409,     // VX855/875 PATA
		0 };
USHORT CListCALi[] = {
	0x5288, 0x5289,     // ALi M5288 -> ALi M5289
		0 };
USHORT CListFALi[] = {
		0x5229,     // ALi M5229
		0x5228,     // ALi M5228
		0x5281,     // ALi SATA
		0x5287,     // ALi SATA
		0x5289,     // ALi SATA
		0 };
USHORT CListCSiS[] = {
	0x0181, 0x0180,     // SiS 181	-> SiS 180
	0x1182, 0x0182,     // SiS 1182 -> SiS 182
	0x0183, 0x0182,     // SiS 183	-> SiS 182
	0x1183, 0x0182,     // SiS 1183 -> SiS 182
	0x1185, 0x1184,     // SiS 1185 -> SiS 1184
		0 };
USHORT CListFSiS[] = {
		0x5513,     // SiS 5513
		0x5517,     // SiS 5517
		0x5518,     // SiS 5518
		0x0180,     // SiS 180 SATA
		0x0182,     // SiS 182 SATA
		0x1180,     // SiS 1180 SATA
		0x1184,     // SiS 1184 AHCI
		0 };
USHORT CListFCMD[] = {
		0x0640,     // CMD640
		0x0643,     // CMD643
		0x0646,     // CMD646
		0x0648,     // CMD648
		0x0649,     // CMD649
		0 };
USHORT CListCPromise[] = {
	0x0D38, 0x4D38,     // Promise Ultra66	     20263
	0x0D30, 0x4D30,     // Promise Ultra100      20265
		0 };
USHORT CListFPromise[] = {
		0x4D33,     // Promise Ultra33	     20246
		0x4D38,     // Promise Ultra66	     20262
		0x4D30,     // Promise Ultra100      20267
		0 };
USHORT CListCPromiseTX[] = {
	0x6268, 0x4D68,     // Promise Ultra100 TX2  20270
	0x6269, 0x4D69,     // Promise Ultra133      20271
	0x1275, 0x4D69,     // Promise Ultra133      20275
	0x5275, 0x4D69,     // Promise Ultra133      20276
	0x7275, 0x4D69,     // Promise Ultra133      20277
		0 };
USHORT CListFPromiseTX[] = {
		0x4D68,     // Promise Ultra100 TX2  20268
		0x4D69,     // Promise Ultra133 TX2  20269
		0 };
USHORT CListCPromiseS[] = {
	0x3319, 0x3318,     // Promise SATA150 TX4   20319
	0x3375, 0x3371,     // Promise SATA150 TX2+  20375
	0x3376, 0x3371,     // Promise SATA150 376   20376
	0x3377, 0x3371,     // Promise SATA150 377   20377
	0x3373, 0x3371,     // Promise SATA150 378   20378
	0x3372, 0x3371,     // Promise SATA150 379   20379
	0x3515, 0x3519,     // Promise SATA300 719   40719
	0x3570, 0x3571,     // Promise SATA150 771   20771
	0x3577, 0x3D18,     // Promise SATA150 779   20779
	0x3574, 0x3D18,     // Promise SATA150 579   20579
	0x3D17, 0x3D18,     // Promise SATA300 718   40718
	0x3D73, 0x3D75,     // Promise SATA300 775   40775
	0x6626, 0x6617,     // Promise Ultra 617     20617
	0x6629, 0x6617,     // Promise Ultra 619     20619
	0x6620, 0x6617,     // Promise Ultra 620     20620
		0 };
USHORT CListFPromiseS[] = {
		0x6617,     // Promise Ultra 6xx     20617  PATA
		0x3318,     // Promise S150 TX4      20375  SATA
		0x3371,     // Promise S150 TX2+     20371  SATA  + PATA
		0x3519,     // Promise FastTrack TX4 20519  SATA2
		0x3571,     // Promise FastTrack TX2 20571  SATA2 + PATA
		0x3D18,     // Promise S300	     20518  SATA2 (ATAPI)
		0x3D75,     // Promise S300	     20575  SATA2 + PATA (ATAPI)
		0 };
USHORT CListFCyrix[] = {
		0x0102,     // Cyrix 5530
		0 };
USHORT CListFHighPoint[] = {
		0x0004,     // HPT366/368/370
		0x0005,     // HPT372A
		0x0006,     // HPT302
		0x0007,     // HPT371
		0x0008,     // HPT374
		0x0009,     // HPT372N
		0 };
USHORT CListFAMD[] = {
		0x7401,     // AMD751
		0x7409,     // AMD756
		0x7411,     // AMD766
		0x7441,     // AMD768
		0x209A,     // AMD5536 (GeodeLX)
		0x7469,     // AMD8111
		0 };
USHORT CListCAEC[] = {
	0x0007, 0x0006,     // AEC6260M
		0 };
USHORT CListFAEC[] = {
		0x0005,     // AEC6210
		0x0006,     // AEC6260
		0x0009,     // AEC6280/6880
		0 };
USHORT CListFAEC2[] = {
		0x000A,     // AEC6886/6896
		0 };
USHORT CListFSMSC[] = {
		0x9130,     // SMSC SLC90E66
		0 };
USHORT CListFServerWorks[] = {
		0x0211,     // ServerWorks OSB4
		0x0212,     // ServerWorks CSB5
		0x0213,     // ServerWorks CSB6
		0x0214,     // ServerWorks HT1000 aka BCM5785
		0 };
USHORT CListCBCMSata[] = {
	0x024B, 0x024A,     // Broadcom BCM5785 -> BCM5785
		0 };
USHORT CListFBCMSata[] = {
		0x024A,     // Broadcom BCM5785 aka ServerWorks HT1000
		0 };
USHORT CListCOpti[] = {
	0xC558, 0xD568,     // Opti Viper
	0xD721, 0xD768,     // Opti Vendetta
		0 };
USHORT CListFOpti[] = {
		0xC621,     // Opti Viper
		0xD568,     // Opti Viper
		0xD768,     // Opti Vendetta
		0 };
USHORT CListCNvidia[] = {
	0x0085, 0x0065,     // nForce2 ultra -> nForce2
	0x00E5, 0x00D5,     // nForce3 CK8 -> nForce3
	0x0053, 0x0035,     // nForce4 C04 -> nForce4 M04
	0x036E, 0x0265,     // nForce5 M55 -> nForce5 M51
	0x0448, 0x03EC,     // nForce6 M65 -> nForce6 M61
	0x0560, 0x03EC,     // nForce6 M67 -> nForce6 M61
	0x0759, 0x056C,     // nForce7 M77 -> nForce7 M73
	0x00E3, 0x008E,     // nForce3 CK8 SATA  -> nForce2 SATA
	0x00EE, 0x008E,     // nForce3 CK8 SATA2 -> nForce2 SATA
	0x003E, 0x0036,     // nForce4 M04 SATA2 -> nForce4 M04 SATA
	0x0054, 0x0036,     // nForce4 C04 SATA  -> nForce4 M04 SATA
	0x0055, 0x0036,     // nForce4 C04 SATA2 -> nForce4 M04 SATA
	0x0267, 0x0266,     // nForce5 M51 SATA2 -> nForce5 M51 SATA
	0x037E, 0x0266,     // nForce5 M55 SATA  -> nForce5 M51 SATA
	0x037F, 0x0266,     // nForce5 M55 SATA2 -> nForce5 M51 SATA
	0x03F6, 0x0266,     // nForce6 M61 SATA  -> nForce5 M51 SATA
	0x03F7, 0x0266,     // nForce6 M61 SATA1 -> nForce5 M51 SATA
	0x03E7, 0x0266,     // nForce6 M61 SATA2 -> nForce5 M51 SATA
	0x044D, 0x044C,     // nForce6 M65 AHCI2 -> nForce6 M65 SATA
	0x044E, 0x044C,     // nForce6 M65 AHCI3 -> nForce6 M65 SATA
	0x044F, 0x044C,     // nForce6 M65 AHCI4 -> nForce6 M65 SATA
	0x045C, 0x044C,     // nForce6 M65 SATA  -> nForce6 M65 SATA
	0x045D, 0x044C,     // nForce6 M65 SATA2 -> nForce6 M65 SATA
	0x045E, 0x044C,     // nForce6 M65 SATA3 -> nForce6 M65 SATA
	0x045F, 0x044C,     // nForce6 M65 SATA4 -> nForce6 M65 SATA
	0x0550, 0x044C,     // nForce6 M67 SATA  -> nForce6 M65 SATA
	0x0551, 0x044C,     // nForce6 M67 SATA2 -> nForce6 M65 SATA
	0x0552, 0x044C,     // nForce6 M67 SATA3 -> nForce6 M65 SATA
	0x0553, 0x044C,     // nForce6 M67 SATA4 -> nForce6 M65 SATA
	0x07F0, 0x044C,     // nForce7 M73 SATA  -> nForce6 M65 SATA
	0x07F1, 0x044C,     // nForce7 M73 SATA2 -> nForce6 M65 SATA
	0x07F2, 0x044C,     // nForce7 M73 SATA3 -> nForce6 M65 SATA
	0x07F3, 0x044C,     // nForce7 M73 SATA4 -> nForce6 M65 SATA
	0x07F4, 0x044C,     // nForce7 M73 SATA5 -> nForce6 M65 SATA
	0x07F5, 0x044C,     // nForce7 M73 SATA6 -> nForce6 M65 SATA
	0x07F6, 0x044C,     // nForce7 M73 SATA7 -> nForce6 M65 SATA
	0x07F7, 0x044C,     // nForce7 M73 SATA8 -> nForce6 M65 SATA
	0x07F8, 0x044C,     // nForce7 M73 SATA9 -> nForce6 M65 SATA
	0x07F9, 0x044C,     // nForce7 M73 SATA10 -> nForce6 M65 SATA
	0x07FA, 0x044C,     // nForce7 M73 SATA11 -> nForce6 M65 SATA
	0x07FB, 0x044C,     // nForce7 M73 SATA12 -> nForce6 M65 SATA
	0x0AD0, 0x044C,     // nForce7 M77 SATA  -> nForce6 M65 SATA
	0x0AD1, 0x044C,     // nForce7 M77 SATA2 -> nForce6 M65 SATA
	0x0AD2, 0x044C,     // nForce7 M77 SATA3 -> nForce6 M65 SATA
	0x0AD3, 0x044C,     // nForce7 M77 SATA4 -> nForce6 M65 SATA
	0x0AB4, 0x044C,     // nForce7 M79 SATA  -> nForce6 M65 SATA
	0x0AB5, 0x044C,     // nForce7 M79 SATA2 -> nForce6 M65 SATA
	0x0AB6, 0x044C,     // nForce7 M79 SATA3 -> nForce6 M65 SATA
	0x0AB7, 0x044C,     // nForce7 M79 SATA4 -> nForce6 M65 SATA
	0x0AB8, 0x044C,     // nForce7 M79 SATA4 -> nForce6 M65 SATA
	0x0AB9, 0x044C,     // nForce7 M79 SATA4 -> nForce6 M65 SATA
	0x0ABA, 0x044C,     // nForce7 M79 SATA4 -> nForce6 M65 SATA
	0x0ABB, 0x044C,     // nForce7 M79 SATA4 -> nForce6 M65 SATA
	0x0ABC, 0x044C,     // nForce7 M79 SATA4 -> nForce6 M65 SATA
	0x0ABD, 0x044C,     // nForce7 M79 SATA4 -> nForce6 M65 SATA
	0x0ABE, 0x044C,     // nForce7 M79 SATA4 -> nForce6 M65 SATA
	0x0ABF, 0x044C,     // nForce7 M79 SATA4 -> nForce6 M65 SATA
	0x0D84, 0x044C,     // nForce7 M89 SATA  -> nForce6 M65 SATA
	0x0D85, 0x044C,     // nForce7 M89 SATA  -> nForce6 M65 SATA
	0x0D86, 0x044C,     // nForce7 M89 SATA  -> nForce6 M65 SATA
	0x0D87, 0x044C,     // nForce7 M89 SATA  -> nForce6 M65 SATA
	0x0D88, 0x044C,     // nForce7 M89 SATA  -> nForce6 M65 SATA
	0x0D89, 0x044C,     // nForce7 M89 SATA  -> nForce6 M65 SATA
	0x0D8A, 0x044C,     // nForce7 M89 SATA  -> nForce6 M65 SATA
	0x0D8B, 0x044C,     // nForce7 M89 SATA  -> nForce6 M65 SATA
	0x0D8C, 0x044C,     // nForce7 M89 SATA  -> nForce6 M65 SATA
	0x0D8D, 0x044C,     // nForce7 M89 SATA  -> nForce6 M65 SATA
	0x0D8E, 0x044C,     // nForce7 M89 SATA  -> nForce6 M65 SATA
	0x0D8F, 0x044C,     // nForce7 M89 SATA  -> nForce6 M65 SATA
		0 };
USHORT CListFNvidia[] = {
		0x008E,     // Nvidia nForce SATA
		0x01BC,     // Nvidia nForce
		0x0065,     // Nvidia nForce2
		0x00D5,     // Nvidia nForce3
		0x0035,     // Nvidia nForce4 M04
		0x0265,     // Nvidia nForce5 M51
		0x03EC,     // Nvidia nForce6 M61
		0x056C,     // Nvidia nForce7 M73
		0x0036,     // Nvidia nForce4 SATA
		0x0266,     // Nvidia nForce5 SATA (ADMA)
		0x044C,     // Nvidia nForce6 SATA (AHCI)
		0 };
USHORT CListFNatSemi[] = {
		0x0502,     // NS Geode SCx200
		0 };
USHORT CListCSiI[] = {
	0x3512, 0x3112,     // SiI3512 SATA -> SiI3112
	0x0240, 0x3112,     // Adaptec SATA -> SiI3112
		0 };
USHORT CListFSiI[] = {
		0x0680,     // SiI680
		0x3112,     // SiI3112 (SATA)
		0x3114,     // SiI3114 (SATA)
		0 };
USHORT CListFITE[] = {
		0x8212,     // ITE 8212
		0x8211,     // ITE 8211
		0 };
USHORT CListFITE2[] = {
		0x8213,     // ITE 8213
		0 };
USHORT CListCIXP[] = {
	0x436E, 0x4369,     // IXP320 PATA -> IXP300 PATA
	0x4376, 0x4369,     // IXP400 PATA -> IXP300 PATA
	0x438C, 0x4369,     // IXP600 PATA -> IXP300 PATA
	0x439C, 0x4369,     // IXP700 PATA -> IXP300 PATA
		0 };
USHORT CListFIXP[] = {
		0x4349,     // ATI IXP200
		0x4369,     // ATI IXP300
		0 };
USHORT CListCIXPS[] = {
	0x4379, 0x436E,     // IXP400 SATA -> IXP300 SATA
	0x437A, 0x436E,     // IXP400 SATA -> IXP300 SATA
		0 };
USHORT CListFIXPS[] = {
		0x436E,     // ATI IXP300 SATA
		0 };
USHORT CListCIXPA[] = {
	0x4381, 0x4380,     // IXP600 SATA -> IXP600 SATA
	0x4390, 0x4380,     // IXP700/800 SATA -> IXP600 SATA
	0x4391, 0x4380,     // IXP700/800 AHCI -> IXP600 SATA
	0x4392, 0x4380,     // IXP700/800 RAID -> IXP600 SATA
	0x4393, 0x4380,     // IXP700/800 RAID5-> IXP600 SATA
	0x4394, 0x4380,     // IXP700/800 SATA -> IXP600 SATA
	0x4395, 0x4380,     // IXP700/800 SATA -> IXP600 SATA
		0 };
USHORT CListFIXPA[] = {
		0x4380,     // ATI IXP600 SATA
		0 };
USHORT CListFVSB[] = {
		0x780c,     // VIA SB900
		0 };
USHORT CListNetCell[] = {
		0x0044,     // SR510x/310x
		0 };
USHORT CListJMicron[] = {
		0x2360,     // JMB360 1xSATA
		0x2361,     // JMB361 1xSATA & 1xPATA
		0x2363,     // JMB363 2xSATA & 1xPATA
		0x2365,     // JMB365 ?xSATA & ?xPATA
		0x2366,     // JMB366 2xSATA & 2xPATA
		0x2368,     // JMB368 2xPATA
		0 };
USHORT CListCMarvell[] = {
	0x6111, 0x6145,     // Marvell 6111 -> Marvell 6145
	0x6120, 0x6145,     // Marvell 6120 -> Marvell 6145
	0x6121, 0x6145,     // Marvell 6121 -> Marvell 6145
	0x6122, 0x6145,     // Marvell 6122 -> Marvell 6145
	0x6123, 0x6145,     // Marvell 6123 -> Marvell 6145
	0x6140, 0x6145,     // Marvell 6140 -> Marvell 6145
	0x6141, 0x6145,     // Marvell 6141 -> Marvell 6145
		0 };
USHORT CListMarvell[] = {
		0x6101,     // Marvell 61xx PATA
		0x6145,     // Marvell 61xx PATA & SATA
		0 };
USHORT CListCInitio[] = {
	0x1623, 0x1622,     // 1623 -> 162x
		0 };
USHORT CListInitio[] = {
		0x1622,     // Initio 162x
		0 };

/*
** PCIDevice is an array of adapter specific information.  Each
** element identifies one supported PCI device.  The indexes into
** this array are defined in s506oem.h
*/
PCI_DEVICE PCIDevice[] =
{/*   Device, Vendor, Index, Revision			       */
 /*   PCIFunc_Init(),	PCIFunc_InitComplete(), - functions    */
 /*   npDeviceMsg, FirstConfigRec			       */
  { { 0x1230, 0x8086, Intel, MODE_NATIVE_OK, TR_PIO_EQ_DMA | TR_PIO0_WITH_DMA, 2,
      AcceptPIIX, CfgICH,
      NULL, NonsharedCheckIRQ,
      GetPIIXPio, SetupCommon,
      CalculateAdapterTiming, PIIXTimingValue, ProgramPIIXChip
#if ENABLEBUS
      , EnableBusPIIX
#endif
      },
      CListCIntel, CListFIntel,
      0x41, 0x43, 0x77, 0xF8, 0x20,
      NULL },
  { { 0x0571, 0x1106, Via, MODE_NATIVE_OK, TR_PIO_EQ_DMA, 2,
      AcceptVIA, CfgVIA,
      NULL, NonsharedCheckIRQ,
      GetVIAPio, SetupCommon,
      CalculateAdapterTiming, VIATimingValue, ProgramVIAChip},
      CListCVia, CListFVia,
      0x40, 0x40, 0x01, 0xF8, 0x10,
      NULL },
  { { 0x5229, 0x10B9, ALi, MODE_NATIVE_OK | SIMPLEX_DIS, TR_PIO_EQ_DMA, 2,
      AcceptALI, CfgALI,
      GenericInitComplete, NonsharedCheckIRQ,
      GetALIPio, SetupALI,
      CalculateAdapterTiming, ALITimingValue, ProgramALIChip},
      CListCALi, CListFALi,
      0x04, 0x04, 0x00, 0x07, 0x10,
      NULL },
  { { 0x5513, 0x1039, SiS, MODE_NATIVE_OK, TR_PIO_EQ_DMA, 2,
      AcceptSIS, CfgSIS,
      NULL, NonsharedCheckIRQ,
      GetSISPio, SetupCommon,
      CalculateAdapterTiming, SISTimingValue, ProgramSISChip},
      CListCSiS, CListFSiS,
      0x04, 0x04, 0x00, 0x00, 0x80,
      NULL },
  { { 0x0640, 0x1095, CMD64x, MODE_NATIVE_OK | SIMPLEX_DIS, TR_PIO_EQ_DMA, 2,
      AcceptCMD, CfgCMD,
      NULL, NonsharedCheckIRQ,
      GetCMDPio, SetupCommon,
      CalculateAdapterTiming, CMDTimingValue, ProgramCMDChip},
      CListNULL, CListFCMD,
      0x04, 0x04, 0x00, 0x00, 0x10,
      NULL },
  { { 0x4D33, 0x105A, Promise, MODE_NATIVE_OK, 0, 2,
      AcceptPDC, CfgPDC,
      PromiseInitComplete, PromiseCheckIRQ,
      GetPDCPio, SetupCommon,
      CalculateAdapterTiming, PDCTimingValue, ProgramPDCChip,
      NULL, NULL, NULL,
      PDCSetupDMA, PDCStartOp, NULL, NULL,
      PDCPreReset, PDCPostReset, NULL,
    },
      CListCPromise, CListFPromise,
      0x04, 0x04, 0x00, 0x00, 0x10,
      NULL },
  { { 0x4D68, 0x105A, PromiseTX, MODE_NATIVE_OK, 0, 2,
      AcceptPDCTX2, CfgGeneric,
      NULL, PromiseTX2CheckIRQ,
      GetGenericPio, SetupPDCTX2,
      NULL, PDCTimingValue, NULL,
      NULL, NULL, NULL,
      NULL, NULL, NULL, NULL,
      NULL, NULL, ProgramPDCIO,
      },
      CListCPromiseTX, CListFPromiseTX,
      0x04, 0x04, 0x00, 0x00, 0x10,
      NULL },
  { { 0x0102, 0x1078, Cyrix, MODE_COMPAT_ONLY, 0, 16,
      AcceptCx, CfgCX,
      NULL, NonsharedCheckIRQ,
      GetCxPio, SetupCommon,
      CalculateAdapterTiming, CxTimingValue, ProgramCxChip},
      CListNULL, CListFCyrix,
      0x04, 0x04, 0x00, 0x00, 0x10,
      Cx5530Msgtxt },
  { { 0x0004, 0x1103, HPT37x, MODE_NATIVE_OK, TR_PIO_EQ_DMA, 4,
      AcceptHPT, CfgHPT37x,
      NULL, HPTCheckIRQ,
      GetHPTPio, SetupCommon,
      CalculateAdapterTiming, HPTTimingValue, ProgramHPTChip,
      HPTEnableInt, NULL, NULL,
      HPT37xSetupDMA, NULL, NULL, HPT37xErrorDMA,
      HPT37xPreReset,
      },
      CListNULL, CListFHighPoint,
      0x04, 0x04, 0x00, 0x00, 0x78 | 1,
      NULL },
  { { 0x7409, 0x1022, AMD, MODE_COMPAT_ONLY | SIMPLEX_DIS, TR_PIO_EQ_DMA, 2,
      AcceptAMD, CfgVIA,
      NULL, NonsharedCheckIRQ,
      GetVIAPio, SetupCommon,
      CalculateAdapterTiming, VIATimingValue, ProgramVIAChip},
      CListNULL, CListFAMD,
      0x40, 0x40, 0x01, 0x00, 0x10,
      NULL },
  { { 0x0005, 0x1191, AEC, MODE_NATIVE_OK, TR_PIO_EQ_DMA, 2,
      AcceptAEC, CfgAEC,
      GenericInitComplete, BMCheckIRQ,
      GetAECPio, SetupCommon,
      CalculateAdapterTiming, AECTimingValue, ProgramAECChip},
      CListCAEC, CListFAEC,
      0x4A, 0x4A, 0x21, 0x00, 0x10,
      NULL },
  { { 0x9130, 0x1055, Intel, MODE_COMPAT_ONLY, TR_PIO_EQ_DMA | TR_PIO0_WITH_DMA, 2,
      AcceptSMSC, CfgPIIX4,
      NULL, NonsharedCheckIRQ,
      GetPIIXPio, SetupCommon,
      CalculateAdapterTiming, PIIXTimingValue, ProgramPIIXChip},
      CListNULL, CListFSMSC,
      0x41, 0x43, 0x77, 0x00, 0x10,
      SLC90E66Msgtxt },
  { { 0x0211, 0x1166, ServerWorks, MODE_COMPAT_ONLY | SIMPLEX_DIS, TR_SAME_DMA_TYPE, 2,
      AcceptOSB, CfgOSB,
      NULL, NonsharedCheckIRQ,
      GetOSBPio, SetupCommon,
      CalculateAdapterTiming, OSBTimingValue, ProgramOSBChip},
      CListNULL, CListFServerWorks,
      0x04, 0x04, 0x00, 0x00, 0x40,
      NULL },
  { { 0xC621, 0x1045, Opti, MODE_COMPAT_ONLY, TR_PIO_EQ_DMA, 2,
      AcceptOPTI, CfgOPTI,
      NULL, NonsharedCheckIRQ,
      GetOPTIPio, SetupCommon,
      CalculateAdapterTiming, OPTITimingValue, ProgramOPTIChip},
      CListCOpti, CListFOpti,
      0x04, 0x40, 0xC0, 0x01, 0x10,
      OPTIMsgtxt },
  { { 0x01BC, 0x10DE, NVidia, MODE_NATIVE_OK, TR_PIO_EQ_DMA, 2,
      AcceptNVidia, CfgNFC,
      NULL, NonsharedCheckIRQ,
      GetVIAPio, SetupCommon,
      CalculateAdapterTiming, VIATimingValue, ProgramVIAChip},
      CListCNvidia, CListFNvidia,
      0x50, 0x50, 0x01, 0x00, 0x10,
      NULL },
  { { 0x0502, 0x100B, Cyrix, MODE_COMPAT_ONLY, 0, 16,
      AcceptCx, CfgCX,
      NULL, NonsharedCheckIRQ,
      GetCxPio, SetupCommon,
      CalculateAdapterTiming, CxTimingValue, ProgramCxChip},
      CListNULL, CListFNatSemi,
      0x04, 0x04, 0x00, 0x00, 0x10,
      NsSCxMsgtxt },
  { { 0x0680, 0x1095, SiI68x, MODE_NATIVE_OK, 0, 2,
      AcceptSII, CfgSII,
      NULL, BMCheckIRQ,
      GetSIIPio, SetupCommon,
      CalculateAdapterTiming, SIITimingValue, ProgramSIIChip},
      CListCSiI, CListFSiI,
      0xA1, 0xB1, 0x00, 0x00, 0x10,
      NULL },
  { { 0x8212, 0x1283, ITE, MODE_NATIVE_OK, TR_PIO_EQ_DMA | TR_PIO_SHARED | TR_UDMA_SHARED, 2,
      AcceptITE, CfgITE,
      NULL, BMCheckIRQ,
      GetGenericPio, SetupCommon,
      CalculateAdapterTiming, ITETimingValue, ProgramITEChip},
      CListNULL, CListFITE,
      0x41, 0x41, 0x57, 0x00, 0x01,
      NULL },
  { { 0, 0x1191, AEC, MODE_NATIVE_OK | NONSTANDARD_HOST, TR_PIO_EQ_DMA, 2,
      AcceptAEC2, CfgGeneric,
      NULL, BMCheckIRQ,
      GetAEC2Pio, SetupCommon,
      CalculateAdapterTiming, AEC2TimingValue, ProgramAEC2Chip},
      CListNULL, CListFAEC2,
      0x40, 0x40, 0x00, 0x00, 0x10,
      NULL },
  { { 0, 0x169C, generic, MODE_NATIVE_OK | SIMPLEX_DIS, 0, 2,
      AcceptNetCell, CfgNull,
      NULL, BMCheckIRQ,
      GetNetCellPio, SetupCommon,
      CalculateGenericAdapterTiming, NULL, NULL},
      CListNULL, CListNetCell,
      0x04, 0x04, 0x00, 0x00, 0x10,
      NetCellMsgtxt },
  { { 0, 0x1002, ATI, MODE_COMPAT_ONLY, TR_PIO_EQ_DMA | TR_SAME_DMA_TYPE, 2,
      AcceptIXP, CfgOSB,
      NULL, NonsharedCheckIRQ,
      GetOSBPio, SetupCommon,
      CalculateAdapterTiming, OSBTimingValue, ProgramOSBChip},
      CListCIXP, CListFIXP,
      0x04, 0x04, 0x00, 0x00, 0x40,
      IXPMsgtxt },
  { { 0, 0x1002, SiISata, MODE_NATIVE_OK, 0, 2,
      AcceptIXPSATA, CfgGeneric,
      NULL, SIICheckIRQ,
      GetGenericPio, SetupCommon,
      CalculateGenericAdapterTiming, NULL, NULL},
      CListCIXPS, CListFIXPS,
      0xA1, 0xB1, 0x00, 0x00, 0x10,
      IXPMsgtxt },
  { { 0, 0x197B, JMicron, MODE_NATIVE_OK, 0, 2,
      AcceptJMB, CfgGeneric,
      NULL, BMCheckIRQ,
      GetJMBPio, SetupCommon,
      CalculateAdapterTiming, NULL, NULL},
      CListNULL, CListJMicron,
      0x04, 0x04, 0x00, 0x00, 0x10,
      NULL },
  { { 0, 0x105A, PromiseMIO, MODE_NATIVE_OK | NONSTANDARD_HOST, 0, 2,
      AcceptPDCMIO, CfgNull,
      NULL, PromiseMCheckIRQ,
      GetGenericPio, SetupCommon,
      CalculateGenericAdapterTiming, NULL, NULL,
      NULL, NULL, NULL,
      PromiseMSetupDMA, PromiseMStartOp, PromiseMStopDMA, PromiseMErrorDMA},
      CListCPromiseS, CListFPromiseS,
      0x04, 0x04, 0x00, 0x00, 0x10,
      NULL },
  { { 0, 0x11AB, Marvell, MODE_NATIVE_OK, 0, 2,
      AcceptMarvell, CfgGeneric,
      NULL, BMCheckIRQ,
      NULL, SetupCommon,
      CalculateAdapterTiming, NULL, NULL},
      CListCMarvell, CListMarvell,
      0x04, 0x04, 0x00, 0x00, 0x10,
      NULL },
  { { 0, 0x1002, AHCI, MODE_NATIVE_OK, 0, 2,
      AcceptAHCI, CfgNull,
      NULL, BMCheckIRQ,
      NULL, SetupCommon,
      CalculateAdapterTiming, NULL, NULL},
      CListCIXPA, CListFIXPA,
      0x04, 0x04, 0x00, 0x00, 0x10,
      IXPMsgtxt },
  { { 0, 0x1101, Initio, MODE_NATIVE_OK, 0, 2,
      AcceptInitio, CfgNull,
      NULL, InitioCheckIRQ,
      NULL, SetupCommon,
      CalculateGenericAdapterTiming, NULL, NULL,
      NULL, NULL, NULL,
      InitioSetupDMA, InitioStartOp, InitioStopDMA, InitioErrorDMA},
      CListCInitio, CListInitio,
      0x04, 0x04, 0x00, 0x00, 0x10,
      NULL },
  { { 0, 0x1166, BcmSata, MODE_NATIVE_OK | NONSTANDARD_HOST, 0, 2,
      AcceptBCM, CfgGeneric,
      NULL, BMCheckIRQ,
      GetGenericPio, SetupCommon,
      CalculateAdapterTiming, NULL, NULL},
      CListCBCMSata, CListFBCMSata,
      0x04, 0x04, 0x00, 0x00, 0x10,
      BroadcomMsgtxt },
  { { 0, 0x8086, Intel, MODE_COMPAT_ONLY, 0, 1,
      AcceptSCH, CfgNull,
      NULL, NonsharedCheckIRQ,
      GetSCHPio, SetupCommon,
      CalculateAdapterTiming, NULL, ProgramSCH},
      CListNULL, CListFiSCH,
      0x04, 0x04, 0x00, 0x00, 0x10,
      SCHMsgtxt },
  { { 0x8213, 0x1283, Intel, MODE_NATIVE_OK, TR_PIO_EQ_DMA | TR_PIO0_WITH_DMA, 2,
      AcceptITE8213, CfgPIIX4,
      NULL, BMCheckIRQ,
      GetPIIXPio, SetupCommon,
      CalculateAdapterTiming, PIIXTimingValue, ProgramPIIXChip},
      CListNULL, CListFITE2,
      0x41, 0x43, 0x77, 0x00, 0x10,
      NULL },
  { { 0, 0x1106, ATI, MODE_COMPAT_ONLY, TR_PIO_EQ_DMA | TR_SAME_DMA_TYPE, 2,
      AcceptIXP, CfgOSB,
      NULL, NonsharedCheckIRQ,
      GetOSBPio, SetupCommon,
      CalculateAdapterTiming, OSBTimingValue, ProgramOSBChip},
      CListNULL, CListFVSB,
      0x04, 0x04, 0x00, 0x00, 0x40,
      VIASBMsgtxt },
  { { 0, 0, generic, MODE_NATIVE_OK, 0, 2,
      AcceptGeneric, CfgGeneric,
      GenericInitComplete, NonsharedCheckIRQ,
      GetGenericPio, SetupCommon,
      CalculateGenericAdapterTiming, NULL, NULL},
      CListNULL, CListNULL,
      0x04, 0x04, 0x00, 0x00, 0x10,
      GenericMsgtxt }
};

SCATGATENTRY   ScratchSGList	    = { 0, 512 };
//SCATGATENTRY	 ScratchSGList1       = { 0, 512 };
ULONG	       Queue = 0L;

UCHAR	       ScratchBuf[SCRATCH_BUF_SIZE] = { 0 };

USHORT	       ReqCount = 0;
UCHAR	       TimerPool[TIMER_POOL_SIZE] = { 0 };
CHAR	       OEMHLP_DDName[9] = "OEMHLP$ ";
CHAR	       PCMCIA_DDName[9] = "PCMCIA$ ";
CHAR	       ACPICA_DDName[9] = "ACPICA$ ";
IDCTABLE       OemHlpIDC =  { 0 };
IDCTABLE       CSIDC = { 0 };
IDCTABLE       ACPIIDC = { 0 };
USHORT	       CSHandle = 0;
UCHAR	       CSRegistrationComplete = 0;
UCHAR	       NumSockets = 0;
UCHAR	       MaxNumSockets = 0;
UCHAR	       FirstSocket = 0;
UCHAR	       Inserting = 0;
UCHAR	       IRQprefLevel = 0;
USHORT	       BasePortpref = 0;
ULONG	       CSCtxHook = 0;
USHORT	       CSIrqAvail = 0xCEB8; // 0xDEB8;
struct CI_Info CIInfo = {
  0, sizeof (CIInfo),
  ATB_IOClient | ATB_Insert4Sharable | ATB_Insert4Exclusive, PCMCIAVERSION, 0,
  ((YEAR - 1980) << 9) | (MONTH << 5) | DAY,
  offsetof (struct CI_Info, CI_Name), sizeof (CIInfo.CI_Name),
  offsetof (struct CI_Info, CI_Vendor), sizeof (CIInfo.CI_Vendor),
  "DaniS506 EIDE Driver",
  "Copyright Daniela Engert 2009, all rights reserved"
};

CCB	       ChipTable[MAX_ADAPTERS] = { 0 };
ACB	       AdapterTable[MAX_ADAPTERS] =
{
 /*----------------------------------------------------*/
 /* BasePort  IRQLevel	Flags  Status  Socket	       */
 /*						       */
 /*----------------------------------------------------*/
  {0,0,0,0,-1, 0	, 0	   , 0	       },
  {0,0,0,0,-1, 0	, 0	   , 0	       },
  {0,0,0,0,-1, 0	, 0	   , 0	       },
  {0,0,0,0,-1, 0	, 0	   , 0	       },
  {0,0,0,0,-1, 0	, 0	   , 0	       },
  {0,0,0,0,-1, 0	, 0	   , 0	       },
  {0,0,0,0,-1, 0	, 0	   , 0	       },
  {0,0,0,0,-1, 0	, 0	   , 0	       }
};

// Scratch buffers used to get the BIOS Parameter Block
BOOT_RECORD	      BootRecord	  = { 0 };
PARTITION_BOOT_RECORD PartitionBootRecord = { 0 };

RP_GENIOCTL    IOCtlRP	 = {{sizeof (IOCtlRP), 0, 0x10}, 0x80, PCI_FUNC};
RP_GENIOCTL    IOCtlACPI = {{sizeof (IOCtlACPI), 0, 0x10}, 0x88};
IORB_EXECUTEIO InitIORB 		  = { 0 };
CHAR	       DTMem[(MAX_ADAPTERS+1)*MR_1K_LIMIT-1] = { 0 };

/*-------------------------------------------------------------------*/
/*								     */
/*	Initialization Data					     */
/*								     */
/*								     */
/*-------------------------------------------------------------------*/

NPVOID	       MemTop;
NPCH	       npTrace		    = NULL;
CHAR	       TraceBuffer[1000]    = { 0 };
UCHAR	       Verbose		    = 0;
UCHAR	       ATAPISlaveChk	    = 0;
PDDD_PARM_LIST pDDD_Parm_List	    = { 0 };
UCHAR	       GenericBM	    = 0;
USHORT	       xBridgePCIAddr	    = 0;
USHORT	       xBridgeDevice	    = 0;
UCHAR	       xBridgeRevision	    = 0;
UCHAR	       PCILatency	    = 0;

#define MSG_REPLACEMENT_STRING	1178

MSGTABLE  InitMsg = { MSG_REPLACEMENT_STRING, 1, 0 };

NPSZ AdptMsgs[] = {    "OK",
		       "Adapter not detected",
		       "Ignored - Previously Initialized",
		       "IRQ Level Not Available",
		       "IRQ Resource Not Available",
		       "IO Port Resource Not Available",
		       "No devices found",
		       "PCCard inserted",
		       "PCCard not inserted",
		       "DeviceBay"
		  };

NPSZ UnitMsgs[] = {    "OK",
		       "[Unit not detected]",
		       "[Read Sector 0 failed]",
		       "[Unit not ready]",
		       "No Master Device",
		       "[Initialization deferred]"
		  };

NPSZ  MsgSMS	     = " SMS:%d";
NPSZ  MsgLBAon	     = " LBA";
NPSZ  MsgAML	     = " NL:%d";
NPSZ  MsgONTrackOn   = " Ontrack";
NPSZ  MsgEzDriveFBP  = " EzDrive FBP";

NPSZ  MsgBMOn	     = " BusMaster Scatter/Gather";
NPSZ  MsgTypeMDMAOn  = " BusMaster";

NPSZ  ATAPIMsg	     = " ATAPI";
NPSZ  MsgNull	     = "";

/*
** Verbose mode PCI Device identification messages.
*/
UCHAR  SII68xMsgtxt[]  = "SiI %X";
UCHAR  CMD64xMsgtxt[]  = "CMD %X";
UCHAR  PIIXxMsgtxt[]   = "Intel PIIX%c";
UCHAR  VIA571Msgtxt[]  = "VIA %s";
UCHAR  SiS5513Msgtxt[] = "SiS %X";
UCHAR  ALI5229Msgtxt[] = "ALi %X";
UCHAR  PDCxMsgtxt[]    = "Promise Ultra%d";
UCHAR  ICHxMsgtxt[]    = "Intel ICH%d";
UCHAR  Cx5530Msgtxt[]  = "Cyrix 5530";
UCHAR  HPT36xMsgtxt[]  = "HPT %d";
UCHAR  AMDxMsgtxt[]    = "AMD %d";
UCHAR  AEC62xMsgtxt[]  = "ATP%d";
UCHAR  SLC90E66Msgtxt[]= "SLC 90E66";
UCHAR  OSBMsgtxt[]     = "ServerWorks SB%d";
UCHAR  OPTIMsgtxt[]    = "Opti c621";
UCHAR  NForceMsgtxt[]  = "NVidia nForce%s";
UCHAR  ITEMsgtxt[]     = "ITE %X";
UCHAR  IXPMsgtxt[]     = "ATI IXP";
UCHAR  VIASBMsgtxt[]   = "VIA SB900";
UCHAR  NsSCxMsgtxt[]   = "NS Geode SCx200";
UCHAR  NetCellMsgtxt[] = "NetCell SyncRAID";
UCHAR  JMMsgtxt[]      = "JMicron JMB%x";
UCHAR  PromiseMIOtxt[] = "Promise %dxx";
UCHAR  MarvellMsgtxt[] = "Marvell %X";
UCHAR  InitioMsgtxt[]  = "Initio INIC-%x";
UCHAR  BroadcomMsgtxt[]= "BCM ";
UCHAR  IDERMsgtxt[]    = "IDE Redirection";
UCHAR  SCHMsgtxt[]     = "Intel SCH";
UCHAR  GenericMsgtxt[] = "Generic";
UCHAR ParmErrMsg[] = " Warning: DANIS506.ADD - Invalid CONFIG.SYS parameters near pos %d";
UCHAR VersionMsg[] = "            Daniela's Bus Master IDE Driver for OS/2 Version "VERSION;

/*-------------------------------------------------------------------*/
/*								     */
/*	Command Line Parser Data				     */
/*								     */
/*								     */
/*-------------------------------------------------------------------*/

#define TOKVBUF_LEN 255

PSZ	   pcmdline1		  = 0;
PSZ	   pcmdline_slash	  = 0;
PSZ	   pcmdline_start	  = 0;
INT	   tokv_index		  = 0;
INT	   state_index		  = 0;
INT	   length		  = 0;
CHARBYTE   tokvbuf[TOKVBUF_LEN]   = { 0 };
NPOPT	   pend_option		  = 0;
NPOPT	   ptable_option	  = 0;
BYTE	   *poutbuf1		  = 0;
BYTE	   *poutbuf_end 	  = 0;
CC	   cc			  = { 0, 0 };

#define OUTBUF_LEN	 255

BYTE	 outbuf[OUTBUF_LEN] = {0};
USHORT	 outbuf_len	    = OUTBUF_LEN + 1;
PBYTE	 poutbuf	    = outbuf;

#define ENTRY_STATE	   0
#define MAX_STATES	   3

/*				    opt.state[] initialization definitions    */
/*									      */
/*						____ entry state	      */
/*						|		    previous  */
/*						v		      opt |   */
/*  ----Command Line Option --------	       ----- STATE TABLE -----	  |   */
/*  token id	      string   type		0   1	2   3		  |   */
/*									  |   */
/*						*  /A:	/I  /U: <-----------  */
/*							    /GEO:	      */
/*							    /T: 	      */

OPT OPT_NOTBEEP =      {TOK_NOTBEEP,	 "!AA",      TYPE_0,       {0, 1, 2, 3}};
OPT OPT_NOT_BIOS =     {TOK_NOT_BIOS,	 "!BIOS",    TYPE_0,       {0, 1, 2, 3}};
OPT OPT_NOT_BM_DMA  =  {TOK_NOT_BM_DMA,  "!BM",      TYPE_0,       {0, 1, 2, 3}};
OPT OPT_NOT_DM =       {TOK_NOT_DM,	 "!DM",      TYPE_0,       {E, 1, 2, 3}};
OPT OPT_NOTREMOVABLE = {TOK_NOTREMOVABLE,"!RMV",     TYPE_0,       {E, E, E, 3}};
OPT OPT_NOT_SETMAX =   {TOK_NOTSETMAX,	 "!SETMAX",  TYPE_0,       {0, 1, 2, 3}};
OPT OPT_NOSHTDWN =     {TOK_NOSHTDWN,	 "!SHUTDOWN",TYPE_0,       {0, 1, 2, 3}};
OPT OPT_NOT_SMS =      {TOK_NOT_SMS,	 "!SMS",     TYPE_0,       {E, E, E, 3}};
OPT OPT_NOT_VERBOSE =  {TOK_NOT_V,	 "!V",       TYPE_0,       {0, 1, 2, 3}};
OPT OPT_NOT_VPAUSE =   {TOK_NOT_VPAUSE,  "!W",       TYPE_0,       {0, 1, 2, 3}};
OPT OPT_80WIRE =       {TOK_80WIRE,	 "80WIRE",   TYPE_0,       {E, 1, 2, E}};
OPT OPT_ADAPTER =      {TOK_ADAPTER,	 "A",        TYPE_DD,      {1, 1, 1, 1}};
OPT OPT_APM =	       {TOK_APM,	 "APM",      TYPE_DDDD,    {0, 1, 2, 3}};
OPT OPT_BAY  =	       {TOK_BAY,	 "BAY",      TYPE_0,       {E, 1, 2, E}};
OPT OPT_BM_DMA	=      {TOK_BM_DMA,	 "BM",       TYPE_0,       {0, 1, 2, 3}};
OPT OPT_DEBUG =        {TOK_DEBUG,	 "DEBUG",    TYPE_DDDD,    {0, 1, 2, 3}};
OPT OPT_DM =	       {TOK_DM, 	 "DM",       TYPE_0,       {E, 1, 2, 3}};
OPT OPT_FIXES =        {TOK_FIXES,	 "FIXES",    TYPE_HHHH,    {0, 1, 2, 3}};
OPT OPT_FORCEGBM =     {TOK_FORCEGBM,	 "FORCEGBM", TYPE_0,       {0, 1, 2, 3}};
OPT OPT_GBM =	       {TOK_GBM,	 "GBM",      TYPE_0,       {0, 1, 2, 3}};
OPT OPT_IRQ  =	       {TOK_IRQ,	 "IRQ",      TYPE_DD,      {0, 1, 2, E}};
OPT OPT_IDLETIMER =    {TOK_IDLETIMER,	 "IT",       TYPE_DDDD,    {0, 1, 2, 3}};
OPT OPT_IGNORE =       {TOK_IGNORE,	 "I",        TYPE_0,       {E, 1, 2, 3}};
OPT OPT_LATENCY =      {TOK_LATENCY,	 "LAT",      TYPE_DDDD,    {0, 1, 2, 3}};
OPT OPT_LOC =	       {TOK_PCILOC,	 "LOC",      TYPE_PCILOC,  {E, 1, 2, E}};
OPT OPT_LPM =	       {TOK_LPM,	 "LPM",      TYPE_DD,      {0, 1, 2, 3}};
OPT OPT_MAXRATE =      {TOK_MAXRATE,	 "MR",       TYPE_HHHH,    {E, E, E, 3}};
OPT OPT_NOISE =        {TOK_NOISE,	 "NL",       TYPE_DDDD,    {E, E, E, 3}};
OPT OPT_PCMCIA =       {TOK_PCMCIA,	 "PCS",      TYPE_O,       {0, 1, 2, 3}};
OPT OPT_PCICLOCK =     {TOK_PCICLOCK,	 "PCLK",     TYPE_DDDD,    {0, 1, 2, 3}};
OPT OPT_PORT =	       {TOK_PORT,	 "PORT",     TYPE_HHHH,    {0, 1, 2, E}};
OPT OPT_PF   =	       {TOK_PF, 	 "PF",       TYPE_HHHH,    {0, 1, 2, 3}};
OPT OPT_P    =	       {TOK_PORT,	 "P",        TYPE_HHHH,    {E, 1, 2, E}};
OPT OPT_REMOVABLE =    {TOK_REMOVABLE,	 "RMV",      TYPE_0,       {E, E, E, 3}};
OPT OPT_SHTDWN =       {TOK_SHTDWN,	 "SHUTDOWN", TYPE_0,       {0, 1, 2, 3}};
OPT OPT_SMS =	       {TOK_SMS,	 "SMS",      TYPE_DDDD,    {E, E, E, 3}};
OPT OPT_IRQTIMEOUT =   {TOK_IRQTIMEOUT,  "TO",       TYPE_DDDD,    {E, 1, 2, E}};
OPT OPT_T =	       {TOK_TIMEOUT,	 "T",        TYPE_DDDD,    {E, E, E, 3}};
OPT OPT_UNIT =	       {TOK_UNIT,	 "UNIT",     TYPE_D,       {E, 3, 3, 3}};
OPT OPT_U    =	       {TOK_UNIT,	 "U",        TYPE_D,       {E, 3, 3, 3}};
OPT OPT_VERBOSE =      {TOK_V,		 "V",        TYPE_0,       {0, 1, 2, 3}};
OPT OPT_VPAUSE =       {TOK_VPAUSE,	 "W",        TYPE_0,       {0, 1, 2, 3}};
OPT OPT_VERBOSEL =     {TOK_VL, 	 "VL",       TYPE_0,       {0, 1, 2, 3}};
OPT OPT_VPAUSEL =      {TOK_VPAUSEL,	 "WL",       TYPE_0,       {0, 1, 2, 3}};
OPT OPT_VERBOSELL =    {TOK_VLL,	 "VLL",      TYPE_0,       {0, 1, 2, 3}};
OPT OPT_VPAUSELL =     {TOK_VPAUSELL,	 "WLL",      TYPE_0,       {0, 1, 2, 3}};
OPT OPT_WP =	       {TOK_WP, 	 "WP",       TYPE_O,       {E, E, E, 3}};
OPT OPT_END =	       {TOK_END,	 "",         TYPE_0,       {O, O, O, O}};


/*									*/
/*   The following is a generic OPTIONTABLE for ADDs which support disk */
/*   devices.								*/
/*									*/
OPTIONTABLE  opttable =

{   ENTRY_STATE, MAX_STATES,
    { (NPOPT) &OPT_NOTBEEP,
      (NPOPT) &OPT_NOT_BIOS,
      (NPOPT) &OPT_NOT_BM_DMA,
      (NPOPT) &OPT_NOT_DM,
      (NPOPT) &OPT_NOTREMOVABLE,
      (NPOPT) &OPT_NOT_SETMAX,
      (NPOPT) &OPT_NOSHTDWN,
      (NPOPT) &OPT_NOT_SMS,
      (NPOPT) &OPT_NOT_VERBOSE,
      (NPOPT) &OPT_NOT_VPAUSE,
      (NPOPT) &OPT_80WIRE,
      (NPOPT) &OPT_APM,
      (NPOPT) &OPT_ADAPTER,
      (NPOPT) &OPT_BAY,
      (NPOPT) &OPT_BM_DMA,
      (NPOPT) &OPT_DEBUG,
      (NPOPT) &OPT_DM,
      (NPOPT) &OPT_FIXES,
      (NPOPT) &OPT_FORCEGBM,
      (NPOPT) &OPT_GBM,
      (NPOPT) &OPT_IRQ,
      (NPOPT) &OPT_IDLETIMER,
      (NPOPT) &OPT_IGNORE,
      (NPOPT) &OPT_LATENCY,
      (NPOPT) &OPT_LOC,
      (NPOPT) &OPT_LPM,
      (NPOPT) &OPT_MAXRATE,
      (NPOPT) &OPT_NOISE,
      (NPOPT) &OPT_PCMCIA,
      (NPOPT) &OPT_PCICLOCK,
      (NPOPT) &OPT_PORT,
      (NPOPT) &OPT_PF,
      (NPOPT) &OPT_P,
      (NPOPT) &OPT_REMOVABLE,
      (NPOPT) &OPT_SHTDWN,
      (NPOPT) &OPT_SMS,
      (NPOPT) &OPT_IRQTIMEOUT,
      (NPOPT) &OPT_T,
      (NPOPT) &OPT_UNIT,
      (NPOPT) &OPT_U,
      (NPOPT) &OPT_WP,
      (NPOPT) &OPT_VERBOSELL,
      (NPOPT) &OPT_VERBOSEL,
      (NPOPT) &OPT_VERBOSE,
      (NPOPT) &OPT_VPAUSELL,
      (NPOPT) &OPT_VPAUSEL,
      (NPOPT) &OPT_VPAUSE,
      (NPOPT) &OPT_END
    }
};

/*----------------------------------------------*/
/* Driver Description				*/
/*----------------------------------------------*/

/* by declaring the storage as arrays and intializing the array, we force  */
/* the compiler to store the ASCII strings in this location rather than in */
/* the text area at the beginning of the segment.  Therefore we can throw  */
/* these strings away.							   */



UCHAR DrvrNameTxt[]	= "DANIS506.ADD";
USHORT DrvrNameSize	= sizeof(DrvrNameTxt);
UCHAR DrvrDescriptTxt[] = "DMA Adapter Driver for PATA/SATA DASD";
UCHAR VendorNameTxt[]	= "Dani";



DRIVERSTRUCT DriverStruct =
{
   DrvrNameTxt, 			     /* DrvrName		*/
   DrvrDescriptTxt,			     /* DrvrDescript		*/
   VendorNameTxt,			     /* VendorName		*/
   CMVERSION_MAJOR,			     /* MajorVer		*/
   CMVERSION_MINOR,			     /* MinorVer		*/
   YEAR,MONTH,DAY,			     /* Date			*/
   DRF_STATIC,				     /* DrvrFlags		*/
   DRT_ADDDM,				     /* DrvrType		*/
   DRS_ADD,				     /* DrvrSubType		*/
   NULL 				     /* DrvrCallback		*/
};


/*----------------------------------------------*/
/* Adapter Description				*/
/*----------------------------------------------*/
UCHAR AdaptDescriptNameTxt[] = "IDE_0 xATA Controller";

ADAPTERSTRUCT AdapterStruct =
{
  AdaptDescriptNameTxt, 	     /* AdaptDescriptName; */
  AS_NO16MB_ADDRESS_LIMIT,	     /* AdaptFlags;	   */
  AS_BASE_MSD,			     /* BaseType;	   */
  AS_SUB_IDE,			     /* SubType;	   */
  AS_INTF_GENERIC,		     /* InterfaceType;	   */
  AS_HOSTBUS_PCI,		     /* HostBusType;	   */
  AS_BUSWIDTH_32BIT,		     /* HostBusWidth;	   */
  NULL, 			     /* pAdjunctList;	   */
  NULL				     /* reserved	   */
};

/*----------------------------------------------*/
/* Device Description				*/
/*----------------------------------------------*/
UCHAR DevDescriptNameTxt[] = "HD_0 Hard Drive";

DEVICESTRUCT DevStruct =
{
   DevDescriptNameTxt,	 /* DevDescriptName; */
   DS_FIXED_LOGICALNAME, /* DevFlags;	  */
   DS_TYPE_DISK 	 /* DevFlags;	     */
};

/*----------------------------------------------------------------*/
/*								  */
/*	Mini-VDM Data						  */
/*								  */
/*								  */
/*----------------------------------------------------------------*/
BOOL	   VDMInt13Created = FALSE;
BOOL	   VDMInt13Active  = FALSE;
VDMINT13CB VDMInt13	   = { 0 };
ULONG	   VDMDskBlkID	   = (ULONG)&ACBPtrs[0];
ULONG	   VDMInt13BlkID   = (ULONG)&VDMInt13;



/*-------------------------------------------------------------------*/
/*								     */
/*	Verbose Data						     */
/*								     */
/*								     */
/*-------------------------------------------------------------------*/

UCHAR VPCIInfo[] = "  %s %cATA host (%04x:%04x rev:%02x) on PCI %d:%d.%d#%d";
UCHAR VPCardInfo[] = "  %s PCCard PATA host";
UCHAR VControllerInfo[] = "Controller:%1d  Port:%04lx IRQ:%02x  Status:%s%s";
UCHAR VUnitInfo1[] = " Unit:%1d Status:%s%s%s%s%s";
UCHAR VUnitInfo2[] = " Unit:%1d Status:%s%s%s%s%s%s%s%s";
UCHAR VModelInfo[] = "  Model:%s";
UCHAR VModelUnknown[] = "  <Unknown>";
UCHAR VBPBInfo[]   = " BPB";
UCHAR VI13Info[]   = " BIOS";
UCHAR VIDEInfo[]   = " Identify";
UCHAR VGeomInfo0[] = "   OS2:log   phys   BPB/BIOS   IDE:log   phys     Total Sectors";
UCHAR VGeomInfo1[] = "  C %6u %6u               %6u %6u   Avail %10lu";
UCHAR VGeomInfo2[] = "  H %6u %6u     %6u    %6u %6u   OS2   %10lu";
UCHAR VGeomInfo3[] = "  S %6u %6u     %6u    %6u %6u   %% Used    %3u.%02u";
UCHAR VBlankLine[] = " ";


UCHAR VPauseVerbose0[] = "SYS    : Pausing DANIS506.ADD output...";
UCHAR VPauseVerbose1[] = "SYS    : End of DANIS506.ADD output.";
UCHAR VString[] 	= "%s";
UCHAR VSata0[] = " no speed";
UCHAR VSata1[] = " 1.5GBit/s";
UCHAR VSata2[] = " 3.0GBit/s";
UCHAR VULTRADMAString[] = " UltraDMA%d/PIO%d";
UCHAR VDMAString[] = " MWordDMA%d/PIO%d";
UCHAR VPIOString[] = " PIO%d";

