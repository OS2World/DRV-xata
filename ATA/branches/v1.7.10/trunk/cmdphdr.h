/**************************************************************************
 *
 * SOURCE FILE NAME =  CMDPHDR.H
 *
 * DESCRIPTIVE NAME =  DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : Command Parser Top Level Include file
 *
 ****************************************************************************/

#include "cmdparse.h"
#include "cmdpdefs.h"

/*						    */
/*  external references resolved in related C file. */
/*						    */

extern	 CC	       cc;
extern	 USHORT        outbuf_len;
extern	 OPTIONTABLE   opttable;
extern	 PBYTE	       poutbuf;
