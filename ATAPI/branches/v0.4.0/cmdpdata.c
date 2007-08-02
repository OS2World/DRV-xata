/**************************************************************************
 *
 * SOURCE FILE NAME = CMDPARSE.C
 *
 * DESCRIPTIVE NAME =  DaniS506I.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       Daniela Engert 1999, 2006
 *
 * DESCRIPTION : ADD CONFIG.SYS Command Line Parser Helper Routine
 *
 * Purpose: This module consists of the Command_Parser Function and
 *	    its associated local routines.  For detailed description
 *	    of the Command_Parser interface refer to the CMDPARSE.H
 *	    file.
 *
 ****************************************************************************/


#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#define INCL_NO_SCB
#define INCL_INITRP_ONLY
#include "os2.h"

#include "iorb.h"
#include "reqpkt.h"
#include "dhcalls.h"
#include "addcalls.h"
#include "dskinit.h"

#include "atapicon.h"
#include "atapityp.h"
#include "atapipro.h"

#include "cmdparse.h"
#include "cmdpdefs.h"

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
/*							    /T: 	      */

OPT OPT_VERBOSE =     {TOK_ID_V,	  "V",     TYPE_0,     {0, 1, 2, 3}};
OPT OPT_NOT_VERBOSE = {TOK_ID_NOT_V,	  "!V",    TYPE_0,     {0, 1, 2, 3}};
OPT OPT_ADAPTER =     {TOK_ID_ADAPTER,	  "A",     TYPE_D,     {1, 1, 1, 1}};
OPT OPT_EJ     =      {TOK_ID_EJ,	  "EJ",    TYPE_0,     {0, 1, 2, 3}};
OPT OPT_IGNORE =      {TOK_ID_IGNORE,	  "I",     TYPE_0,     {E, 1, 2, 3}};
OPT OPT_U    =	      {TOK_ID_UNIT,	  "U",     TYPE_D,     {E, 3, E, 3}};
OPT OPT_UNIT =	      {TOK_ID_UNIT,	  "UNIT",  TYPE_D,     {E, 3, E, 3}};
OPT OPT_LF =	      {TOK_ID_LF,	  "LF",    TYPE_0,     {E, E, E, 3}};
OPT OPT_NOT_LF =      {TOK_ID_NOT_LF,	  "!LF",   TYPE_0,     {E, E, E, 3}};
OPT OPT_ZIPA  =       {TOK_ID_ZIPA,	  "ZA",    TYPE_0,     {0, 1, 2, 3}};
OPT OPT_ZIPB  =       {TOK_ID_ZIPB,	  "ZB",    TYPE_0,     {0, 1, 2, 3}};
OPT OPT_RSJ  =	      {TOK_ID_RSJ,	  "RSJ",   TYPE_0,     {0, 1, 2, 3}};
OPT OPT_SCSI  =       {TOK_ID_SCSI,	  "SCSI",  TYPE_0,     {E, 1, 2, 3}};
OPT OPT_NOT_SCSI  =   {TOK_ID_NOT_SCSI,   "!SCSI", TYPE_0,     {0, 1, 2, 3}};
OPT OPT_TYPE =	      {TOK_ID_TYPE,	  "TYPE",  TYPE_ATAPI, {E, E, E, 3}};
OPT OPT_NOTREMOVBL =  {TOK_ID_NOTREMOVBL, "!RMV",  TYPE_0,     {E, E, E, 3}};
OPT OPT_END =	      {TOK_ID_END,	  "\0",    TYPE_0,     {O, O, O, O}};

/*									*/
/*   The following is a generic OPTIONTABLE for ADDs which support disk */
/*   devices.								*/
/*									*/
OPTIONTABLE  opttable =

{   ENTRY_STATE, MAX_STATES,
    { (POPT) &OPT_VERBOSE,
      (POPT) &OPT_NOT_VERBOSE,
      (POPT) &OPT_NOT_LF,
      (POPT) &OPT_ADAPTER,
      (POPT) &OPT_EJ,
      (POPT) &OPT_IGNORE,
      (POPT) &OPT_LF,
      (POPT) &OPT_U,
      (POPT) &OPT_UNIT,
      (POPT) &OPT_RSJ,
      (POPT) &OPT_NOTREMOVBL,
      (POPT) &OPT_SCSI,
      (POPT) &OPT_NOT_SCSI,
      (POPT) &OPT_TYPE,
      (POPT) &OPT_ZIPA,
      (POPT) &OPT_ZIPB,
      (POPT) &OPT_END
    }
};
