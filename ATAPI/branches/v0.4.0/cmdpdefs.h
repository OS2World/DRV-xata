/**************************************************************************
 *
 * SOURCE FILE NAME =  CMDPDEFS.H
 *
 * DESCRIPTIVE NAME =  DaniATAPI.FLT - Filter Driver for ATAPI
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 ****************************************************************************/
/*							      */
/* TOKEN IDs  - opt.id definitions			      */
/*							      */
/* note:  - Assign a unique token id (1 - 255) for each valid */
/*	    option.					      */
/*							      */

#define TOK_ID_ADAPTER	      1
#define TOK_ID_PORT	      2
#define TOK_ID_UNIT	      3
#define TOK_ID_DMA	      4
#define TOK_ID_IRQ	      5
#define TOK_ID_AHS	      6
#define TOK_ID_NOT_AHS	      7
#define TOK_ID_FORMAT	      8
#define TOK_ID_S_BYTES	      9
#define TOK_ID_MCA	     10
#define TOK_ID_PS2	     TOK_ID_MCA
#define TOK_ID_SLOT	     11
#define TOK_ID_DM	     12
#define TOK_ID_NOT_DM	     13
#define TOK_ID_SM	     14
#define TOK_ID_NOT_SM	     15
#define TOK_ID_HCW	     16
#define TOK_ID_NOT_HCW	     17
#define TOK_ID_HCR	     18
#define TOK_ID_NOT_HCR	     19
#define TOK_ID_DEV0	     20
#define TOK_ID_CAM	     21
#define TOK_ID_NOT_CAM	     22
#define TOK_ID_FDT	     23
#define TOK_ID_NOT_FDT	     24
#define TOK_ID_GEO	     25
#define TOK_ID_SMS	     26
#define TOK_ID_NOT_SMS	     27
#define TOK_ID_SN	     28
#define TOK_ID_NOT_SN	     29
#define TOK_ID_ET	     30
#define TOK_ID_NOT_ET	     31
#define TOK_ID_CHGLINE	     32
#define TOK_ID_TIMEOUT	     33
#define TOK_ID_IGNORE	     34
#define TOK_ID_V	     35
#define TOK_ID_NOT_V	     36
#define TOK_ID_LBAMODE	     37
#define TOK_ID_RESET	     38
#define TOK_ID_NORESET	     39
#define TOK_ID_ATAPI	     40
#define TOK_ID_NOT_ATAPI     41
#define TOK_ID_FORCE	     42
#define TOK_ID_PCMCIA	     43
/* Incremented token ids for DMA (below) by 1 */

#define TOK_ID_DMATYPE	     44
#define TOK_ID_DMACHNL	     45
#define TOK_ID_DMASGP	     46
#define TOK_ID_NOT_DMASG     47
#define TOK_ID_NEC	     48

#define TOK_ID_BM_DMA	     49        /* Bus Master DMA enable */
#define TOK_ID_NOT_BM_DMA    50        /* Bus Master DMA disable */
#define TOK_ID_DMASG	     51        /* DMA scatter/gather enable */

#define TOK_ID_PS2LED	     52        /* PS2, force drive light control */

#define TOK_ID_NOT_LF	     53        /*  NOT Large Floppy	 */
#define TOK_ID_LF	     54        /*  Large Floppy (LS-120) */
#define TOK_ID_ZIPA	     55        /*  ZIP to A: */
#define TOK_ID_TYPE	     56        /*  forced types */
#define TOK_ID_NOT_SCSI      57        /*  disable SCSI export */
#define TOK_ID_SCSI	     58        /*  enable SCSI export */
#define TOK_ID_EJ	     59        /*  enable shutdown eject */
#define TOK_ID_RSJ	     60        /*  RSJ special		 */
#define TOK_ID_NOTREMOVBL    61        /*  disable removable opt */
#define TOK_ID_ZIPB	     62        /*  ZIP to B: */

