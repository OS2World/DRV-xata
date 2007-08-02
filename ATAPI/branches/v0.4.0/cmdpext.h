/**************************************************************************
 *
 * SOURCE FILE NAME =  CMDPEXT.H
 *
 * DESCRIPTIVE NAME =  DaniATAPI.FLT - Filter Driver for ATAPI
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : C externs for Command Parser internal functions
 ****************************************************************************/

 #include "cmdparse.h"

/*-------------------------------------------------------------------*/
/*								     */
/*	Command Line Parser Data				     */
/*								     */
/*-------------------------------------------------------------------*/

 extern PSZ	     pcmdline1;
 extern PSZ	     pcmdline_slash;
 extern PSZ	     pcmdline_start;
 extern INT	     tokv_index;
 extern INT	     state_index;
 extern INT	     length;
 extern CHARBYTE     tokvbuf[];
 extern NPOPT	     pend_option;
 extern NPOPT	     ptable_option;
 extern BYTE	     *poutbuf1;
 extern BYTE	     *poutbuf_end;

 extern CC	     cc;
 extern USHORT	     outbuf_len;
 extern PBYTE	     poutbuf;
 extern OPTIONTABLE  opttable;
