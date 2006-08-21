/**************************************************************************
 *
 * SOURCE FILE NAME = ATAPICON.H
 *
 * DESCRIPTION : Locally defined equates.
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION :
 ****************************************************************************/

/*---------------------------------------*/
/* Maximum Adapters/Units		 */
/*---------------------------------------*/
#define MAX_ADAPTERS	8
#define MAX_UNITS	2

/*---------------------------------------*/
/* DASD Device Type			 */
/*---------------------------------------*/
#define LS_120		0x0020		     /* UIB_TYPE_LS120	*/
#define ZIP_DRIVE	0x0021		     /* UIB_TYPE_ZIP_DRIVE*/

/*---------------------------------------*/
/* TAPE Device Type			 */
/*---------------------------------------*/
#define ONSTREAM_TAPE	0x0030

#define LS120ID_STRGSIZE     7
#define ZIPID_STRGSIZE	    12
#define ONSTREAMID_STRGSIZE 15

/*------------------------*/
/* Media Validity Values  */			  /* MediaInvalid4Drive()   */
/*------------------------*/
#define  MV_VALID	0x00			  /* Media Valid - Continue */
#define  MV_INVALID	0x01			  /* Media Invalid  */

/*---------------------------------------*/
/* Setup and Execute commands		 */
/*---------------------------------------*/
#define SAE_TESTUNITREADY      1
#define SAE_INTERNAL_IDENTIFY  2
#define SAE_READ	       3
#define SAE_READ_VERIFY        4
#define SAE_WRITE	       5
#define SAE_WRITE_VERIFY       6
#define SAE_REQUESTSENSE       7
#define SAE_GETMEDIASTATUS     8
#define SAE_LOCK_MEDIA	       9
#define SAE_UNLOCK_MEDIA      10
#define SAE_EJECT_MEDIA       11
#define SAE_RESET	      12
#define SAE_EXECUTE_CDB       13
#define SAE_FORMAT_TRACK      14
#define SAE_READ_CAPACITY     15
#define SAE_EXECUTE_ATA       16

#define MAX_ATAPI_PKT_SIZE    16

#define LOCKUNLOCKEJECT       10

/*-----------------------------*/
/* Sense Data defines	       */
/*-----------------------------*/
#define  ASC_WRITE_PROTECTED_MEDIA   0x27
#define  ASC_MEDIA_CHANGED	     0x28
#define  ASC_BUS_DEVICE_RESET	     0x29
#define  ASC_MEDIUM_NOT_PRESENT      0x3A

#define  BYTECHK	 0x02	   /* Verify bit for media verification */

/*--------------------*/
/* START/STOP OPTIONS */
/*--------------------*/
#define   STOP_THE_MEDIA  0x00
#define  EJECT_THE_MEDIA  0x02
#define  EJECT_THE_MEDIA_ONSTREAM  0x04
#define  START_THE_MEDIA  0x01

#define  IDENTIFY_DATA_BYTES  512

/*-------------------*/
/* TimeOut Equates   */
/*-------------------*/

/*---------------------------------------*/
/* TimeOut for RESET to complete (30s)	 */
/*					 */
/* After a RESET the drive is checked	 */
/* every 200ms. 			 */
/*---------------------------------------*/
#define DELAYED_RESET_MAX	(30*1000)
#define DELAYED_RESET_INTERVAL	     200
#define ATAPI_RESET_DELAY	      20
#define ATAPI_NUM_RETRIES	       5

/*----------------------------------------------*/
/* TimeOut for IRQ from last interrupt (5s)	*/
/*----------------------------------------------*/
#define IRQ_TIMEOUT_INTERVAL	(5*1000)

/*----------------------------------------------*/
/* Retry Delay interval (200ms) 		*/
/*----------------------------------------------*/
#define DELAYED_RETRY_INTERVAL	200

/*---------------------------------------*/
/* Minimum User Timeout (5s)		 */
/*---------------------------------------*/
#define MIN_USER_TIMEOUT	(IRQ_TIMEOUT_INTERVAL / 1000L)

/*---------------------------------------*/
/* Default User Timeout (30s)		 */
/*---------------------------------------*/
#define DEFAULT_USER_TIMEOUT	30

/*---------------------------------------*/
/* Maximum Cmd/Data Error Retries	 */
/*---------------------------------------*/
#define MAX_DATA_ERRORS 	25

/*---------------------------------------*/
/* Initialization unit timeouts 	 */
/*---------------------------------------*/
#define INIT_TIMEOUT_LONG	(10*1000L)
#define INIT_TIMEOUT_SHORT	(    500L)
#define IRQ_TIMEOUT_LENGTH	( 5*1000l)

/*---------------------------------------*/
/* Elapsed Timer Interval (200ms)	 */
/*---------------------------------------*/
#define ELAPSED_TIMER_INTERVAL	200

/*---------------------------------------*/
/* Maximum PIO Delays (us)		 */
/*---------------------------------------*/
#define MAX_WAIT_DRQ			30
#define MAX_WAIT_BSY		      2000
#define MAX_WAIT_READY		      1000

/*---------------------------------------*/
/* Maximum Transfer - MultiBlock Mode	 */
/*---------------------------------------*/
#define MAX_MULTMODE_BLK	      16

/*---------------------------------------*/
/* Maximum Transfer - Bytes		 */
/*---------------------------------------*/
//#define MAX_XFER_BYTES_PER_INTERRUPT	 8192
#define MAX_XFER_BYTES_PER_INTERRUPT   0xFFFE

/*---------------------------------------*/
/* Maximum Transfer - Overall		 */
/*---------------------------------------*/
#define MAX_XFER_SEC		      256

/*---------------------------------------*/
/* ATA command back-off time  æs	 */
/*---------------------------------------*/
#define ATA_BACKOFF		    65000

/* Miscellaneous constants */

#define MR_64K_LIMIT	0x00010000L	    /* 64K memory limit */
#define MR_4K_LIMIT	0x00001000L	    /* 4K memory limit */
#define MR_2K_LIMIT	0x00000800L	    /* 2K memory limit */

#define SECTORSHIFT		9

#define SCRATCH_BUF_SIZE     1024
#define SENSE_DATA_BYTES       18
#define DEVICE_TABLE_SIZE     512

#define TIMERS_PER_ACB		2

#define TIMER_POOL_SIZE 	(sizeof(ADD_TIMER_POOL) +		      \
				      ((TIMERS_PER_ACB * MAX_ADAPTERS))       \
				* sizeof(ADD_TIMER_DATA))

#define ACB_POOL_SIZE		(MAX_ADAPTERS * (MAX_UNITS * (sizeof(UCB))))

#define UNITINFO_POOL_SIZE	(sizeof(UNITINFO) * MAX_ADAPTERS * MAX_UNITS)

#define SG_POOL_SIZE		((MAX_ADAPTERS+1)*MR_2K_LIMIT-1)

/*-------------------------------*/
/* Miscellaneous		 */
/*-------------------------------*/
#if 1
#define ENABLE	_asm{ sti }
#define DISABLE _asm{ cli }
#else
#define ENABLE	_enable();
#define DISABLE _disable();
#endif
#define INT3   _asm{ int 3 }

#define      MAXRESETS	       4

#define  LEN_MODE_SENSE_10    24

#define BEEP(f,t) DevHelp_Beep(f,t);DevHelp_ProcBlock(0xC0FEBABEul,2*t,0)

