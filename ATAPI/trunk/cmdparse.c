/**************************************************************************
 *
 * SOURCE FILE NAME = CMDPARSE.C
 *
 * DESCRIPTIVE NAME =  DaniS506I.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
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

#include "cmdproto.h"
#include "cmdpext.h"

#define TOKVBUF_LEN 255
#define UNDEFINED   -1

#pragma optimize(OPTIMIZE, on)

/*******************************************************************************
*									       *
*   Command_Parser -  external entry point into this module		       *
*									       *
*******************************************************************************/

CC NEAR Command_Parser(PSZ pCmdLine, NPOPTIONTABLE pOptTable, NPBYTE pOutBuf, USHORT OutBuf_Len)
{
  USHORT j, end_index;

  if (OutBuf_Len < (TOKL_ID_END + TOK_MIN_LENGTH)) {
    cc.ret_code = BUF_TOO_SMALL_ERR;
    cc.err_index = 0;
    return (cc);
  }

  poutbuf_end = pOutBuf + OutBuf_Len;

  poutbuf1 = pOutBuf;
  for (poutbuf1 = pOutBuf; poutbuf1 < poutbuf_end;  poutbuf1++)
    *poutbuf1 = 0;
  poutbuf1 = pOutBuf;

  Insert_End_Token();

  /*-------------------------------------------------------------------*/
  /*  Locate the last entry in the Option Table. This special entry    */
  /*  defines whether or not an option is required based on the index  */
  /*  in the state table.					       */
  /*-------------------------------------------------------------------*/

  for (end_index = 0; pOptTable->poption[end_index]->id != TOK_ID_END; end_index++);
  pend_option = pOptTable->poption[end_index];

  /*-------------------------------------------------*/
  /*  Setup the initial index into the state table.  */
  /*-------------------------------------------------*/

  state_index = pOptTable->entry_state;
  if (!Validate_State_Index(pOptTable->max_states))
    return (cc);

  /*--------------------------------------------------------*/
  /*  On return from Locate_First_Slash call pcmdline_slash */
  /*  contains the ptr to the / in the command line.	    */
  /*--------------------------------------------------------*/

  pcmdline_start = pCmdLine;
  pcmdline1 = pCmdLine;

  if (!Locate_First_Slash()) return (cc);

  for (j = 0; j < end_index; j++) {
    /*--------------------------------------------------------------------*/
    /*	Locate valid options in Option Table, based on state table index. */
    /*--------------------------------------------------------------------*/

    if (pOptTable->poption[j]->state[state_index] != E) {
      /*-----------------------------------------------------------*/
      /*  Found a valid option. Check to see if this is the option */
      /*  entered at this point in command line.		   */
      /*-----------------------------------------------------------*/

      ptable_option = pOptTable->poption[j];
      length = strlen (ptable_option->string);
      if (_strncmp (pcmdline_slash, ptable_option->string, length) == TRUE) {
	/*--------------------------------------------------------*/
	/* Found the command line option.  Now, syntax check its  */
	/* associated value.					  */
	/*--------------------------------------------------------*/

	if (!Parse_Option_Value()) return (cc);

	/*----------------------------------------------------------*/
	/* No syntax err detected.  Now, insert the option and its  */
	/* associated value into the output buffer in token format. */
	/*----------------------------------------------------------*/

	if (!Insert_Token()) return (cc);

	/*-----------------------------------------*/
	/*  Setup next index into the state table. */
	/*-----------------------------------------*/

	state_index = ptable_option->state[state_index];
	if (!Validate_State_Index(pOptTable->max_states)) return (cc);

	/*-----------------------------------------------------------*/
	/*  Setup cmdline_slash to point the the next / (option) in  */
	/*  the command line.					     */
	/*  Parsing stops once either an invalid character is	     */
	/*  found on the command line or the end of the command line */
	/*  is detected.					     */
	/*-----------------------------------------------------------*/

	if (!Locate_Next_Slash()) return (cc);

	/*----------------------------------------------------------------*/
	/* Setup for option search. Point to the top of the Option Table. */
	/*----------------------------------------------------------------*/

	j = -1;
      } /* endif */
    } /* endif */
  } /* endfor */

  if (pend_option->state[state_index] == R) {
  /*------------------------------------------------------*/
  /* A required option was not found on the command line. */
  /*------------------------------------------------------*/

    cc.ret_code = REQ_OPT_ERR;
  } else {
  /*--------------------------------------------------------------------*/
  /* Characters on the command line are not defined in the Option Table */
  /* as a valid option.  All options must start with a / character.	*/
  /*--------------------------------------------------------------------*/

    cc.ret_code = INVALID_OPT_ERR;
  }
  cc.err_index = pcmdline_slash-pCmdLine;
  return (cc);
}

/*******************************************************************************
*									       *
*   FUNCTION: Insert the end of token marker into the output buffer	       *
*									       *
*******************************************************************************/
STATIC VOID NEAR Insert_End_Token()
{
  *poutbuf1 = TOKL_ID_END;
  *(poutbuf1+1) = TOK_ID_END;
  return;
}

/*******************************************************************************
*									       *
*   FUNCTION: Locate the / on the command line.  All characters entered prior  *
*	      to the first / are ignored.  This allows the parser to bypass    *
*	      the BASEDEV = xxxxxxxx.xxx portion of the command line.	       *
*									       *
*******************************************************************************/
STATIC BOOL NEAR Locate_First_Slash()
{
  char c;

  while (((c = *pcmdline1) != '\0') && (c != '\n') && (c != '\r')) {
    pcmdline1++;
    if (c == '/') {
      pcmdline_slash = pcmdline1;
      return (TRUE);
    }
  }

  cc.err_index = 0;
  cc.ret_code = NO_OPTIONS_FND_ERR;
  if (pend_option->state[state_index] == R)
    cc.ret_code = REQ_OPT_ERR;

  return (FALSE);
}

/*******************************************************************************
*									       *
*   FUNCTION: Parse the command line for the value assigned to located option  *
*									       *
*******************************************************************************/
STATIC BOOL NEAR Parse_Option_Value()
{
  pcmdline1 = pcmdline_slash + length;

  if (ptable_option->type != TYPE_0)
    if ((*pcmdline1 != ':') && (*pcmdline1 != '=')) {
      cc.ret_code = INVALID_OPT_ERR;
      cc.err_index = pcmdline1 - pcmdline_start;
      return (FALSE);
    } else {
      pcmdline1++;
    }

  Skip_Over_Blanks();

  for (tokv_index = 0; tokv_index <= TOKVBUF_LEN; tokv_index++)
       tokvbuf[tokv_index].byte_value= 0;

  tokv_index = UNDEFINED;

  cc.ret_code = NO_ERR;
  cc.err_index = 0;

  switch (ptable_option->type) {
      case TYPE_0:
	   break;

//	case TYPE_CHAR:
//	     char_parser();
//	     break;

      case TYPE_D:
	   d_parser();
	   break;

//	case TYPE_DD:
//	     dd_parser();
//	     break;

//	case TYPE_DDDD:
//	     dddd_parser();
//	     break;

//	case TYPE_HH:
//	     hh_parser();
//	     break;

//	case TYPE_HHHH:
//	     hhhh_parser();
//	     break;
#ifdef BBR
      case TYPE_BBR:
	   bad_block_parser();
	   break;
#endif

//	case TYPE_FORMAT:
//	     format_parser();
//	     break;

//	case TYPE_SCSI_ID:
//	     scsi_id_parser();
//	     break;

//	case TYPE_GEOMETRY:
//	     geometry_parser();
//	     break;

//	case TYPE_CHGLINE:
//	     chgline_parser();
//	     break;

      case TYPE_ATAPI:
	   atapi_parser();
	   break;

      default:
	   cc.ret_code = UNDEFINED_TYPE_ERR;

  } /* endswitch */

  if (cc.ret_code != NO_ERR) {
    cc.err_index = pcmdline1 - pcmdline_start;
    return (FALSE);
  }

  return (TRUE);
}


/*******************************************************************************
*									       *
*   FUNCTION: Skip over all the blank and tab characters		       *
*									       *
*******************************************************************************/
STATIC VOID NEAR Skip_Over_Blanks()
{
  while ((*pcmdline1 == ' ') || (*pcmdline1 == '\t'))
    pcmdline1++;
  return;
}

/*******************************************************************************
*									       *
*   FUNCTION: TYPE_D option parser - one digit decimal number (d)	       *
*									       *
*******************************************************************************/
STATIC VOID NEAR d_parser()
{
  char c;

  if (((c = *pcmdline1) >= '0') && (c <= '9')) {
    tokv_index++;
    tokvbuf[tokv_index].byte_value = *pcmdline1 - '0';
    pcmdline1++;
  } else {
    cc.ret_code = SYNTAX_ERR;
  }
  return;
}


/*******************************************************************************
*									       *
*   FUNCTION: TYPE_ATAPI option parser - ATAPI type C/L/Z/O		       *
*									       *
*******************************************************************************/
STATIC VOID NEAR atapi_parser()
{
   USHORT num = 0;
   UCHAR flag = 1;

   while (flag) {
     switch (*pcmdline1) {
       case 'c':
       case 'C': num |= ATAPITYPE_CDROM; break;
       case 'w':
       case 'W': num |= ATAPITYPE_CDWRITER; break;
       case 'z':
       case 'Z': num |= ATAPITYPE_ZIP; break;
       case 'l':
       case 'L': num |= ATAPITYPE_LS120; break;
       case 'm':
       case 'M': num |= ATAPITYPE_MO; break;
       case 'o':
       case 'O': num |= ATAPITYPE_OTHER; break;
       case '\0':
       case '\n':
       case '\r':
       case ' ': flag = 0; break;
     }
     if (flag) pcmdline1++;
   }

   tokv_index++;
   tokvbuf[tokv_index].byte_value = (UCHAR)num;
   tokv_index++;
   tokvbuf[tokv_index].byte_value = (UCHAR) (num >> 8);
}


/*******************************************************************************
*									       *
*   FUNCTION: Insert the parsed option (token) into the output buffer.	       *
*									       *
*******************************************************************************/
STATIC BOOL NEAR Insert_Token()
{
    USHORT t, tok_size;

    tok_size = TOK_MIN_LENGTH + tokv_index;
    if ((poutbuf1 + tok_size + TOKL_ID_END) >= poutbuf_end)
    {
       cc.err_index = pcmdline_slash-pcmdline_start;
       cc.ret_code = BUF_TOO_SMALL_ERR;
       return (FALSE);
    }

    *poutbuf1 = tok_size + 1;
    poutbuf1++;
    *poutbuf1 = ptable_option->id;
    poutbuf1++;

    if (tokv_index != UNDEFINED)
    {
       for (t=0;t <= tokv_index;t++)
       {
	   *poutbuf1 = tokvbuf[t].byte_value;
	    poutbuf1++;
       }
    }

    Insert_End_Token();

    return (TRUE);
}

/*******************************************************************************
*									       *
*   FUNCTION: Locate the next / char.					       *
*									       *
*******************************************************************************/
BOOL NEAR Locate_Next_Slash()
{
  char c;

  while (((c = *pcmdline1) != '\0') && (c != '\n') && (c != '\r')) {
    pcmdline1++;
    if ((c != ' ') && (c != '\t')) {
      if (c == '/') {
	pcmdline_slash = pcmdline1;
	return (TRUE);
      } else {
	cc.ret_code = INVALID_OPT_ERR;
	cc.err_index = pcmdline1-pcmdline_start;
	return (FALSE);
      }
    }
  } /* endwhile */

  if (pend_option->state[state_index] == R) {
    cc.ret_code = REQ_OPT_ERR;
    cc.err_index = pcmdline1-pcmdline_start;
  } else {
    cc.ret_code = NO_ERR;
    cc.err_index = 0;
  }
  return (FALSE);
}


/*******************************************************************************
*									       *
*   FUNCTION: Validate the State Index					       *
*									       *
*******************************************************************************/
STATIC BOOL NEAR Validate_State_Index (USHORT maxstate)
{
  if ((state_index > maxstate) ||
      (state_index < 0)) {
    cc.ret_code = UNDEFINED_STATE_ERR;
    cc.err_index = 0;
    return (FALSE);
  }

  return (TRUE);
}

