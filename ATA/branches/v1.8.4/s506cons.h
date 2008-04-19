/**************************************************************************
 *
 * SOURCE FILE NAME =  S506CONS.H
 *
 * DESCRIPTIVE NAME =  DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2008
 *
 * DESCRIPTION : Locally defined equates.
 ****************************************************************************/

#define ENABLEBUS 0
#define AUTOMOUNT 0

/*---------------------------------------*/
/* Maximum Adapters/Units		 */
/*---------------------------------------*/
#define MAX_IRQS	8
#define MAX_ADAPTERS   16
#define MAX_CHANNELS	4
#define MAX_UNITS	2
#define MAX_SOCKETS	8

/*-------------------*/
/* TimeOut Equates   */
/*-------------------*/

/*---------------------------------------*/
/* TimeOut for RESET to complete (30s)	 */
/*					 */
/* After a RESET the drive is checked	 */
/* every 200ms. 			 */
/*---------------------------------------*/
#define DELAYED_RESET_MAX	(30*1000L)
#define DELAYED_RESET_INTERVAL	      200L

#define RESET_ASSERT_TIME		50 // microseconds
#define SATA_RESET_ASSERT_TIME		 5 // microseconds

/*------------------------------------------*/
/* TimeOut for IRQ from last interrupt (5s) */
/*------------------------------------------*/
#define IRQ_TIMEOUT_INTERVAL	 (5*1000L)

/*---------------------------------------*/
/* TimeOut for IRQ after spin-down (30s) */
/*---------------------------------------*/
#define IRQ_LONG_TIMEOUT_INTERVAL (30*1000L)

/*---------------------------------------*/
/* Retry Delay interval (200ms) 	 */
/*---------------------------------------*/
#define DELAYED_RETRY_INTERVAL	      200L

/*---------------------------------------*/
/* Minimum User Timeout (5s)		 */
/*---------------------------------------*/
#define MIN_USER_TIMEOUT (IRQ_TIMEOUT_INTERVAL / 1000L)

/*---------------------------------------*/
/* Default User Timeout (30s)		 */
/*---------------------------------------*/
#define DEFAULT_USER_TIMEOUT		30

/*---------------------------------------*/
/* Maximum Cmd/Data Error Retries	 */
/*---------------------------------------*/
#define MAX_BUSY_TIMEOUTS		50

/*---------------------------------------*/
/* Maximum Cmd/Data Error Retries	 */
/*---------------------------------------*/
#define MAX_DATA_ERRORS 		25

/*---------------------------------------*/
/* Maximum Reset Retries		 */
/*---------------------------------------*/
#define MAX_RESET_RETRY 		10

/*---------------------------------------*/
/* Initialization unit timeouts 	 */
/*---------------------------------------*/
#define INIT_TIMEOUT_LONG	(10*1000L)
#define INIT_TIMEOUT_SHORT	( 2*1000L)

/*---------------------------------------*/
/* Elapsed Timer Interval (200ms)	 */
/*---------------------------------------*/
#define ELAPSED_TIMER_INTERVAL	200

/*---------------------------------------*/
/* Calibrate Loop Counter		 */
/*---------------------------------------*/
#define CALIBRATE_LOOP_COUNT	      1024
#define CALIBRATE_TIMER_INTERVAL    (3*31L)

/*---------------------------------------*/
/* Maximum PIO Delays (ms)		 */
/*---------------------------------------*/
#define MAX_WAIT_DRQ			100
#define MAX_WAIT_READY			100

/*---------------------------------------*/
/* Maximum Transfer - MultiBlock Mode	 */
/*---------------------------------------*/
#define MAX_MULTMODE_BLK	       128

/*---------------------------------------*/
/* Maximum Transfer - Overall		 */
/*---------------------------------------*/
#define MAX_XFER_SEC		       256

/*---------------------------------------*/
/* Suspend Count			 */
/*---------------------------------------*/
#define DEFERRED_COUNT			 5
#define IMMEDIATE_COUNT 		 1

/*---------------------------------------*/
/* ATAPI: ATA_BACKOFF æs		 */
/*---------------------------------------*/
#define ATA_BACKOFF		    10000

/*-------------------------------*/
/* Miscellaneous		 */
/*-------------------------------*/
#define ENABLE	_enable();
#define DISABLE _disable();

#ifdef P4_LOOP
#define P4_REP_NOP _asm { rep nop }
#else
#define P4_REP_NOP
#endif

#define BEEP(f,t) DevHelp_Beep(f,t);DevHelp_ProcBlock(0xC0FEBABEul,2*t,0)

#define IODelay() IODly(npA->IODelayCount)

#define SECTORSHIFT		9

#define SCRATCH_BUF_SIZE	1024

#define ATAPI_RECOVERY		30
/* CHL
 * Increase the retry value, this is the worst case for some CDROMs to
 * post its signature. In most case, it exits the loop much earlier, so
 * there is no performance problem.
 * Don't decrease the value, if you do so, it will cause the problem for TOSHIBA
 * CDROM.
 */
#define MAX_ATAPI_RECOVERY_TRYS  32768
#define MAX_ATAPI_CONFIGURE_TRYS 5

#define MAX_SPINUP_RETRIES	 200

#define ON			1
#define OFF			0
#define INDETERMINANT		-1

/*---------------------------------------------*/
/* Dummy IORB CommandModifier		       */
/*					       */
/* To pass internal requests to state machine  */
/*---------------------------------------------*/
#define IOCM_NO_OPERATION	0

#define REQ(x,y) (((x) << 8) | (UCHAR)(y))

#define PCI_CMD_IO		0x0001	    /* IO  */
#define PCI_CMD_MEM		0x0002	    /* MEM */
#define PCI_CMD_BM		0x0004	    /* Bus Master Enable */
#define PCI_CMD_BME		0x0005	    /* Bus Master Enable & IO */
#define PCI_CMD_BME_MEM 	0x0007	    /* Bus Master Enable & IO & Mem */

/* Ultra DMA Mode Supported Identify Word 88 *//*xxxxx210xxxxx210*/

#define ULTRADMA_7ACTIV  0x8000 	    /*	   Active Modes   */
#define ULTRADMA_6ACTIV  0x4000 	    /*	   Active Modes   */
#define ULTRADMA_5ACTIV  0x2000 	    /*	   Active Modes   */
#define ULTRADMA_4ACTIV  0x1000 	    /*	   Active Modes   */
#define ULTRADMA_3ACTIV  0x0800
#define ULTRADMA_2ACTIV  0x0400
#define ULTRADMA_1ACTIV  0x0200
#define ULTRADMA_0ACTIV  0x0100

#define ULTRADMAMASK	 (1 << ULTRADMAMODE_6) - 1  /* Supported Modes */
#define ULTRADMAMODE_6	 0x0007
#define ULTRADMAMODE_5	 0x0006
#define ULTRADMAMODE_4	 0x0005
#define ULTRADMAMODE_3	 0x0004
#define ULTRADMAMODE_2	 0x0003
#define ULTRADMAMODE_1	 0x0002
#define ULTRADMAMODE_0	 0x0001


/* Miscellaneous constants */

#define MR_64K_LIMIT	0x00010000L	    /* 64K memory limit */
#define MR_4K_LIMIT	0x00001000L	    /* 4K memory limit */
#define MR_2K_LIMIT	0x00000800L	    /* 2K memory limit */
#define MR_1K_LIMIT	0x00000400L	    /* 1K memory limit */

/* IdentifyDevice Constants */

#define  ATA_DEVICE    0		   /* ATA Device   */
#define  ATAPI_DEVICE  1		   /* ATAPI Device */

// BIOS Geometry Limits
#define BIOS_MAX_CYLINDERS		1024l
#define BIOS_MAX_NUMHEADS		 255
#define BIOS_MAX_SECTORSPERTRACK	  63
#define BIOS_MAX_TOTALSECTORS	    16450560l // 1024 * 255 * 63

// ATA Geometry Limits
#define ATA_MAX_CYLINDERS	       65536l
#define ATA_MAX_NUMHEADS		  16
#define ATA_MAX_SECTORSPERTRACK 	 255
#define ATA_MAX_TOTALSECTORS	   267386880l // 65536 * 16 * 255
#define ATA_MAX_UNTRANSLATED	    16514064l // 16383 * 16 * 63

// Legacy Geometry Limits
#define LIMITED_MAX_CYLINDERS		1024l
#define LIMITED_MAX_NUMHEADS		  16
#define LIMITED_MAX_SECTORSPERTRACK	  63
#define LIMITED_MAX_TOTALSECTORS     1032192l // 1024 * 16 * 63

#define TRANSLATED_MAX_TOTALSECTORS 15481935l // 16383 * 15 * 63
