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

#if 0
#define __OS2_16__
#include <acpi.h>
#include <ipdc.h>
#include <amlresrc.h>
#endif

#pragma optimize(OPTIMIZE, on)

extern RP_GENIOCTL IOCtlACPI;

BOOL FAR ACPISetup (VOID)
{
  if (SELECTOROF (ACPIIDC.ProtIDCEntry) != NULL)
    return (FALSE);	/* already initialized */

  if (DevHelp_AttachDD (ACPICA_DDName, (NPBYTE)&ACPIIDC))
    return (1);     /* couldn't find ACPICA IDC entry point

  if ((SELECTOROF(ACPIIDC.ProtIDCEntry) == NULL) ||
      (ACPIIDC.ProtIDC_DS == NULL))
    return (1);     /* Bad Entry Point or Data Segment */
}

#if 0
#define CallAcpi(P,PS,F)	       \
  IOCtlACPI.Function   = (F),	       \
  IOCtlACPI.DataPacket = (PUCHAR)(P),  \
  IOCtlACPI.DataLen    = (PS),	       \
  CallAcpiCA ((PRPH)&IOCtlACPI)

ACPI_STATUS ACPIENTRY AcpiGetType (
  ACPI_HANDLE		Object,
  ACPI_OBJECT_TYPE FAR *OutType)
{
  GTyPar Packet = {Object, AcpiPointer (OutType), AE_ERROR};

  CallAcpi (&Packet, sizeof (Packet), GETTYPE_FUNCTION);
  return (Packet.Status);
}

ACPI_STATUS ACPIENTRY AcpiGetObjectInfo (
  ACPI_HANDLE	   Handle,
  ACPI_BUFFER FAR *ReturnBuffer)
{
  GOIPar Packet = {Handle, AcpiPointer (ReturnBuffer), AE_ERROR};

  CallAcpi (&Packet, sizeof (Packet), GETOBJECTINFO_FUNCTION);
  return (Packet.Status);
}

ACPI_STATUS ACPIENTRY AcpiGetNextObject (
  ACPI_OBJECT_TYPE Type,
  ACPI_HANDLE	   Parent,
  ACPI_HANDLE	   Child,
  ACPI_HANDLE FAR *OutHandle)
{
  GNOPar Packet = {Type, Parent, Child, AcpiPointer (OutHandle), AE_ERROR};

  CallAcpi (&Packet, sizeof (Packet), GETNEXTOBJECT_FUNCTION);
  return (Packet.Status);
}

ACPI_STATUS ACPIENTRY AcpiGetParent (
  ACPI_HANDLE  Object,
  ACPI_HANDLE *OutHandle)
{
  GTpPar Packet = {Object, (ACPI_HANDLE *)AcpiPointer (OutHandle), AE_ERROR};

  CallAcpi (&Packet, sizeof (Packet), GETPARENT_FUNCTION);
  return (Packet.Status);
}

ACPI_STATUS ACPIENTRY AcpiGetName (
  ACPI_HANDLE  Handle,
  UINT32       NameType,
  ACPI_BUFFER *RetPathPtr)
{
  GNPar Packet = {Handle, NameType, AcpiPointer (RetPathPtr), AE_ERROR};
  CallAcpi (&Packet, sizeof (Packet), GETNAME_FUNCTION);
  return (Packet.Status);
}

ACPI_STATUS ACPIENTRY AcpiEvaluateObject (
  ACPI_HANDLE	    Object,
  ACPI_STRING	    Pathname,
  ACPI_OBJECT_LIST *ParameterObjects,
  ACPI_BUFFER	   *ReturnObjectBuffer)
{
  EvaPar Packet = {Object, Pathname, AcpiPointer (ParameterObjects), AcpiPointer (ReturnObjectBuffer), AE_ERROR};
  CallAcpi (&Packet, sizeof (Packet), EVALUATE_FUNCTION);
  return (Packet.Status);
}

LIN FAR AcpiPointer (PVOID p) {
  LIN lp;
  DevHelp_VirtToLin (SELECTOROF (p), OFFSETOF (p), &lp);
  return (void *)(lp);
}

#endif

