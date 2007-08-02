/**************************************************************************
 *
 * SOURCE FILE NAME = PARSECMD.C
 *
 * DESCRIPTIVE NAME =  DaniATAPI.FLT - Filter Driver for ATAPI
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : Parser routine that calls the command parser
 *
 ****************************************************************************/
#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#define INCL_INITRP_ONLY
#include "os2.h"
#include "dskinit.h"

#include "iorb.h"
#include "addcalls.h"
#include "dhcalls.h"
#include "reqpkt.h"

#include "scsi.h"
#include "cdbscsi.h"

#include "cmdpdefs.h"
#include "cmdpext.h"
#include "atapicon.h"
#include "atapireg.h"
#include "atapityp.h"
#include "atapiext.h"
#include "atapipro.h"

#pragma optimize(OPTIMIZE, on)

/*------------------------------------*/
/*				      */
/*				      */
/*				      */
/*------------------------------------*/

#define PCF_ADAPTER0	0x01
#define PCF_ADAPTER1	0x02
#define PCF_ADAPTER2	0x04
#define PCF_ADAPTER3	0x08
#define PCF_ADAPTER4	0x10
#define PCF_ADAPTER5	0x20
#define PCF_ADAPTER6	0x40
#define PCF_ADAPTER7	0x80

#define PCF_UNIT0	0x1
#define PCF_UNIT1	0x2

#define PCF_CLEAR_ADAPTER (PCF_UNIT0  | PCF_UNIT1)

/*อออออออออออออออออออออออออออออออออป
บ				   บ
บ				   บ
ศอออออออออออออออออออออออออออออออออ*/
USHORT NEAR ParseCmdLine (PSZ pCmdLine)
{
  CC	 cc;
  NPSZ	 pOutBuf;
  USHORT rc = 0;

  NPA	 npA;
  NPU	 npU = NULL;

  USHORT Length, Value;
  UCHAR  AFlags = 0, UFlags = 0;
  UCHAR  Mask;

  for (npA = AdapterTable; npA < (AdapterTable + MAX_ADAPTERS); npA++) {
    npA->UnitCB[0].Flags |= UCBF_SCSI;
    npA->UnitCB[1].Flags |= UCBF_SCSI;
  }
  npA = NULL;
  pOutBuf = poutbuf;

  /*อออออออออออออออออออออออออออออออออออออออออออออป
  บ  Parse the command Line			 บ
  ศอออออออออออออออออออออออออออออออออออออออออออออ*/

  cc = Command_Parser (pCmdLine, &opttable, pOutBuf, outbuf_len);

  if (cc.ret_code == NO_ERR) {
    while (!rc && pOutBuf[1] != (UCHAR) TOK_ID_END) {

      Length = pOutBuf[0];
      Value  = *(NPUSHORT)(pOutBuf + 2);

      switch (pOutBuf[1]) {
	 /*-------------------------------------------*/
	 /* Verbose Mode - /V			      */
	 /*					      */
	 /*-------------------------------------------*/

	case TOK_ID_V:

	  Verbose = TRUE;
	  break;

	 /*-------------------------------------------*/
	 /* NOT Verbose Mode - /!V		      */
	 /*					      */
	 /*-------------------------------------------*/

	case TOK_ID_NOT_V:

	  Verbose = FALSE;
	  break;

	 /*-------------------------------------------*/
	 /* RSJ special  - /RSJ 		      */
	 /*					      */
	 /*-------------------------------------------*/

	case TOK_ID_RSJ:

	  if (npU)
	    npU->Flags |= UCBF_RSJ;
	  else if (npA) {
	    npA->UnitCB[0].Flags |= UCBF_RSJ;
	    npA->UnitCB[1].Flags |= UCBF_RSJ;
	  } else {
	    for (npA = AdapterTable; npA < (AdapterTable + MAX_ADAPTERS); npA++) {
	      npA->UnitCB[0].Flags |= UCBF_RSJ;
	      npA->UnitCB[1].Flags |= UCBF_RSJ;
	    }
	    npA = NULL;
	  }

	  break;

	 /*-------------------------------------------*/
	 /* SCSI export  - /SCSI		      */
	 /*					      */
	 /*-------------------------------------------*/

	case TOK_ID_SCSI:

	  if (npU)
	    npU->Flags |= UCBF_SCSI;
	  else if (npA) {
	    npA->UnitCB[0].Flags |= UCBF_SCSI;
	    npA->UnitCB[1].Flags |= UCBF_SCSI;
	  }
	  break;

	 /*-------------------------------------------*/
	 /* NOT SCSI export  - /!SCSI		      */
	 /*					      */
	 /*-------------------------------------------*/

	case TOK_ID_NOT_SCSI:

	  if (npU)
	    npU->Flags &= ~UCBF_SCSI;
	  else if (npA) {
	    npA->UnitCB[0].Flags &= ~UCBF_SCSI;
	    npA->UnitCB[1].Flags &= ~UCBF_SCSI;
	  } else {
	    for (npA = AdapterTable; npA < (AdapterTable + MAX_ADAPTERS); npA++) {
	      npA->UnitCB[0].Flags &= ~UCBF_SCSI;
	      npA->UnitCB[1].Flags &= ~UCBF_SCSI;
	    }
	    npA = NULL;
	  }
	  break;

	 /*-------------------------------------------*/
	 /* shutdown eject   - /EJ		      */
	 /*					      */
	 /*-------------------------------------------*/

	case TOK_ID_EJ:

	  if (npU)
	    npU->Flags |= UCBF_EJECT;
	  else if (npA) {
	    npA->UnitCB[0].Flags |= UCBF_EJECT;
	    npA->UnitCB[1].Flags |= UCBF_EJECT;
	  } else {
	    for (npA = AdapterTable; npA < (AdapterTable + MAX_ADAPTERS); npA++) {
	      npA->UnitCB[0].Flags |= UCBF_EJECT;
	      npA->UnitCB[1].Flags |= UCBF_EJECT;
	    }
	    npA = NULL;
	  }
	  break;

	 /*-------------------------------------------*/
	 /* ZIP to x: - /ZIP:			      */
	 /*					      */
	 /*-------------------------------------------*/

	case TOK_ID_ZIPA:

	  ZIPtoA = 1;
	  break;

	case TOK_ID_ZIPB:

	  ZIPtoA = 2;
	  break;

	 /*-------------------------------------------*/
	 /* Adapter - /A:n			      */
	 /*					      */
	 /*-------------------------------------------*/

	case TOK_ID_ADAPTER:

	  Value &= 0x0F;
	  Mask = PCF_ADAPTER0 << Value;
	  if ((Value >= MAX_ADAPTERS) || (AFlags & Mask)) { rc = 1; break; }
	  AFlags |= Mask;
	  UFlags = 0;
	  npA = AdapterTable + Value;
	  npU = NULL;	// reset unit pointer
	  break;

       /*-------------------------------------------*/
       /* Unit Number - /U:0, /U:1		    */
       /*					    */
       /*-------------------------------------------*/

	case TOK_ID_UNIT:

	  Value &= 0x0F;
	  Mask = (Value) ? PCF_UNIT1 : PCF_UNIT0;
	  if ((Value >= MAX_UNITS) || (UFlags & Mask)) { rc = 1; break; }
	  UFlags |= Mask;
	  npU	  = &npA->UnitCB[Value];
	  break;

	 /*-------------------------------------------*/
	 /* forced types /TYPE: 		      */
	 /*					      */
	 /*-------------------------------------------*/

	case TOK_ID_TYPE:

	  npU->Types = Value;
	  break;

	 /*-------------------------------------------*/
	 /* disable removability /!RMV		      */
	 /*					      */
	 /*-------------------------------------------*/

	case TOK_ID_NOTREMOVBL:

	  npU->Flags |= UCBF_NOTRMV;
	  break;

	 /*-------------------------------------------*/
	 /* Large Floppy - /LF			      */
	 /*					      */
	 /*-------------------------------------------*/

	case TOK_ID_LF:

	  npU->Flags |= UCBF_LARGE_FLOPPY;
	  break;

	 /*-------------------------------------------*/
	 /* NOT Large Floppy - /!LF		      */
	 /*					      */
	 /*-------------------------------------------*/

	case TOK_ID_NOT_LF:

	  npU->Flags &= ~UCBF_LARGE_FLOPPY;
	  break;

       /*-------------------------------------------*/
       /* Ignore Adapter - /I			    */
       /*					    */
       /*-------------------------------------------*/

       case TOK_ID_IGNORE:

	 if (npU) {
	   npU->Flags  |= UCBF_IGNORE;
	   npU->Status	= UTS_SKIPPED;
	 } else if (npA) {
	   npA->FlagsT |= ATBF_IGNORE;
	   npA->UnitCB[0].Status = UTS_SKIPPED;
	   npA->UnitCB[1].Status = UTS_SKIPPED;
	 }
	 break;

	 /*-------------------------------------------*/
	 /* Unknown Switch			      */
	 /*					      */
	 /*-------------------------------------------*/

	default:
	  rc = 1;
	  break;
      }

      if (!rc) pOutBuf += Length;
    }
  } else if (cc.ret_code != NO_OPTIONS_FND_ERR) {
    rc = 1;
  }

  return (rc);
}

