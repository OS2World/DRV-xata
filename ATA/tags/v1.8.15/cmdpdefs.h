/**************************************************************************
 *
 * SOURCE FILE NAME =  CMDPDEFS.H
 *
 * DESCRIPTIVE NAME =  DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : Token IDs
 *
 ****************************************************************************/

/*							      */
/* TOKEN IDs  - opt.id definitions			      */
/*							      */
/* note:  - Assign a unique token id (1 - 255) for each valid */
/*	    option.					      */
/*							      */

#define TOK_ADAPTER	   1
#define TOK_PORT	   2
#define TOK_UNIT	   3
#define TOK_PCILOC	   4
#define TOK_IRQ 	   5
#define TOK_DM		   6
#define TOK_NOT_DM	   7
#define TOK_SM		   8
#define TOK_NOT_SM	   9
#define TOK_SMS 	  11
#define TOK_NOT_SMS	  12
#define TOK_TIMEOUT	  13
#define TOK_IGNORE	  14
#define TOK_V		  15
#define TOK_NOT_V	  16
#define TOK_PCMCIA	  23

#define TOK_BM_DMA	  25	    /* Bus Master DMA enable */
#define TOK_NOT_BM_DMA	  26	    /* Bus Master DMA disable */

#define TOK_VPAUSE	  27	    /* /W  Pause Verbose */
#define TOK_NOT_VPAUSE	  28	    /* /!W Pause Verbose */
#define TOK_IDLETIMER	  29	    /* /IT: Idle Timer	 */
#define TOK_MAXRATE	  30	    /* /MR: Limit Max Rate */
#define TOK_GBM 	  31	    /* /GBM  */
#define TOK_FORCEGBM	  32	    /* /FORCEGBM */
#define TOK_PCICLOCK	  33	    /* /PCLK: */
#define TOK_DEBUG	  34	    /* /DEBUG: */
#define TOK_REMOVABLE	  35
#define TOK_NOTREMOVABLE  36
#define TOK_NOTBEEP	  37	    /* /!AA   */
#define TOK_80WIRE	  39	    /* /80WIRE*/
#define TOK_NOSHTDWN	  40	    /* /!SHUTDWON */
#define TOK_SHTDWN	  41	    /* /SHUTDWON */
#define TOK_IRQTIMEOUT	  42	    /* /TO: */
#define TOK_PF		  43	    /* /PF:	*/
#define TOK_NOISE	  44	    /* /NL:	*/
#define TOK_VL		  45	    /* /VL	*/
#define TOK_VLL 	  46	    /* /VLL	*/
#define TOK_VPAUSEL	  47	    /* /WL	*/
#define TOK_VPAUSELL	  48	    /* /WLL	*/
#define TOK_NOTSETMAX	  49	    /* /!SETMAX */
#define TOK_NOT_BIOS	  50	    /* /!BIOS	*/
#define TOK_FIXES	  51	    /* /FIXES:	*/
#define TOK_WP		  52	    /* /WP:	*/
#define TOK_LATENCY	  53	    /* /LAT:	*/
#define TOK_BAY 	  54	    /* /BAY:	*/
#define TOK_APM 	  55	    /* /APM:	*/
#define TOK_LPM 	  56	    /* /LPM:	*/

#ifdef BBR
#define TOK_BBRINT3	  98	    // BBR with an int 3
#define TOK_BBR 	  99	    // BBR without an int 3
#endif

