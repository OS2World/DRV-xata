/**************************************************************************
 *
 * SOURCE FILE NAME = ACPI.C
 *
 * DESCRIPTIVE NAME =
 *
 * Copyright : COPYRIGHT Daniela Engert 2006
 *
 ****************************************************************************/

 #define INCL_NOPMAPI
 #define INCL_DOSINFOSEG
 #define INCL_NO_SCB
 #define INCL_DOSERRORS
 #include "os2.h"
#include "bseerr.h"
#include "dskinit.h"
#include "strat2.h"

#include "iorb.h"
#include "reqpkt.h"
#include "dhcalls.h"
#include "addcalls.h"

#include "s506cons.h"
#include "s506type.h"
#include "s506regs.h"
#include "s506ext.h"
#include "s506pro.h"

#define __OS2_16__
#include <acpi.h>

#pragma optimize(OPTIMIZE, on)

BOOL FAR ACPISetup (VOID)
{
  if (SELECTOROF (ACPIIDC.ProtIDCEntry) != NULL)
    return (FALSE);	/* already initialized */

  return (DevHelp_AttachDD (ACPICA_DDName, (NPBYTE)&ACPIIDC));
}


