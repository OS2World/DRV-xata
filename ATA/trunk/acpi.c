/**************************************************************************
 *
 * SOURCE FILE NAME = ACPI.C
 *
 * DESCRIPTIVE NAME =  DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * COPYRIGHT Daniela Engert 2006-2009
 * Portions copyright (c) 2009, 2010 Steven H. Levine
 * Distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION: ACPI.PSD interface
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

#include <acpi.h>
#include <ipdc.h>
#include <acpiapi.h>

#pragma optimize(OPTIMIZE, on)

extern RP_GENIOCTL IOCtlACPI;

BOOL FAR ACPISetup (VOID)
{
  if (isACPIPresent) return (FALSE);	 /* already initialized */

  if (DevHelp_AttachDD (ACPICA_DDName, (NPBYTE)&ACPIIDC))
    return (1);     /* couldn't find ACPICA IDC entry point */	// 07 Jun 10 SHL comment was missing trailing */

  if (!isACPIPresent|| ACPIIDC.ProtIDC_DS == NULL)	// 07 Jun 10 SHL correct syntax error hidden by prior nested comment
    return (1);     /* Bad Entry Point or Data Segment */

  APICRewire = 0;
}

#define CallAcpi(P,PS,F)	       \
  IOCtlACPI.Function   = (F),	       \
  IOCtlACPI.DataPacket = (PUCHAR)(P),  \
  IOCtlACPI.DataLen    = (PS),	       \
  CallAcpiCA ((PRPH)&IOCtlACPI)

static KNOWNDEVICE Dev = { 0 };

USHORT FAR ACPIGetPCIIRQs (USHORT PCIAddr)
{
  Dev.PciId.Segment  = 0;
  Dev.PciId.Bus      = (PCIAddr >> 8);
  Dev.PciId.Device   = (PCIAddr >> 3) & 31;
  Dev.PciId.Function = (PCIAddr & 7);
  Dev.PicIrq  = 0;
  Dev.ApicIrq = 0;

  CallAcpi (&Dev, sizeof (KNOWNDEVICE), FIND_PCI_DEVICE);

  return ((Dev.ApicIrq << 8) | Dev.PicIrq);
}

#if 0 // 17 Aug 10 SHL fixme to be gone or used

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

#endif // 17 Aug 10 SHL fixme to be gone or used

