/**************************************************************************
 *
 * SOURCE FILE NAME =  S506CSUB.C
 *
 * DESCRIPTIVE NAME =  DaniS506I.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : Adapter Driver miscellaneous routines.
 *
 ****************************************************************************/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#define INCL_NO_SCB
#define INCL_INITRP_ONLY
#define INCL_DOSERRORS
#include "os2.h"
#include "dskinit.h"

#include "iorb.h"
#include "reqpkt.h"
#include "dhcalls.h"
#include "addcalls.h"

#include "atapicon.h"
#include "atapityp.h"
#include "atapipro.h"


#pragma optimize(OPTIMIZE, on)

/*------------------------------------*/
/*				      */
/*				      */
/*				      */
/*------------------------------------*/

VOID FAR clrmem (NPCH d, USHORT len)
{
  memset (d, 0, len);
}

VOID FAR fclrmem (PUCHAR d, USHORT len)
{
  memset (d, 0, len);
}


/****************************/
/* Copy & Swap String Bytes */
/****************************/

VOID NEAR strnswap (NPSZ d, NPSZ s, USHORT n)
{
  USHORT j = 1;

  while (n && s[j]) { *d++ = s[j]; j ^= 1; s += j * 2; n--; }
  while (n) { *d++ = 0; n--; }
}



/*******************************************************************************
*									       *
*   FUNCTION: Compare n number of characters in 2 strings, return TRUE if =    *
*	      If s1 is in lower case, convert to upper prior to comparing.     *
*									       *
*******************************************************************************/
BOOL NEAR _strncmp (PUCHAR s1, PUCHAR s2, USHORT n)
{
  for (; n > 0; n--)
    if ((*(s1++) ^ *(s2++)) & ~0x20)
      return (FALSE);
  return (TRUE);
}

BOOL NEAR strncmp (NPCH s1, NPCH s2, USHORT n)
{
  for (; n > 0; n--)
    if ((*(s1++) ^ *(s2++)) & ~0x20)
      return (FALSE);
  return (TRUE);
}

