/**************************************************************************
 *
 * SOURCE FILE NAME = S506NTIO.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 2006
 *
 * DESCRIPTION : Adapter Driver Initio routines.
 ****************************************************************************/

#define INCL_NOPMAPI
#define INCL_DOSINFOSEG
#define INCL_NO_SCB
#define INCL_INITRP_ONLY
#define INCL_DOSERRORS
#include "os2.h"
#include "dskinit.h"

#include "devhdr.h"
#include "iorb.h"
#include "reqpkt.h"
#include "dhcalls.h"
#include "addcalls.h"
#include "devclass.h"

#include "s506cons.h"
#include "s506type.h"
#include "s506regs.h"
#include "s506ext.h"
#include "s506pro.h"

#include "Trace.h"

#pragma optimize(OPTIMIZE, on)


/*----------------------------------------------------------------*/
/* Defines for IN_level 	    PCI Config Device IDs	  */
/*					  func0  func1		  */
/*----------------------------------------------------------------*/
#define NOIN		   0
#define IN		   1   /*			     */

/* remap some structure members */

#define PciInfo (&npA->PCIInfo)

BOOL NEAR AcceptInitio (NPA npA)
{
  NPU npU = npA->UnitCB;
  NPC npC = npA->npC;
  ULONG BAR5 = npC->BAR[5].Addr;
  ULONG Adr;

  if (!BAR5) return (FALSE);

  if (0 == npA->IDEChannel) {
    OutW (BAR5 | 0x7C, 0x2000 | (~0x8104 & InW (BAR5 | 0x7C)));
    DevHelp_ProcBlock ((ULONG)(PVOID)&AcceptInitio, 200, 0);
  }

  Adr = BAR5 + npA->IDEChannel * 0x40;
//  for (i = FI_PDAT; i <= FI_PCMD; i++)
//    npA->IOPorts[i] = Adr + (i << 2);
//  DEVCTLREG = Adr + 0x38;
  BMCMDREG  = Adr + 0x0B;
  BMDTPREG  = Adr + 0x0C;
  BMSTATUSREG = 0;
  SSTATUS   = Adr + 0x20;
  ISRPORT   = Adr + 0x09;

  OutB (Adr | 0x0A, 0x04); // mask off most interrupt sources
InD (Adr | 0x14);
  OutW (Adr | 0x14, 0);    // switch to ATA mode;

  GenericSATA (npA);
  npA->Cap |= CHIPCAP_ATA66 | CHIPCAP_ATA100 | CHIPCAP_ATA133;
  npA->Cap &= ~CHIPCAP_PIO32;

  sprntf (npA->PCIDeviceMsg, InitioMsgtxt, MEMBER(npA).Device);
  return (TRUE);
}

VOID NEAR InitioSetupDMA (NPA npA)
{
  OutD (BMDTPREG, npA->ppSGL);
}

VOID NEAR InitioStartOp (NPA npA)
{
  if (npA->BM_CommandCode & BMICOM_START) {
    OutB (COMMANDREG, COMMAND);
    OutB (BMCMDREG, (UCHAR)(0x80 | npA->BM_CommandCode));
  } else {
    OutB (COMMANDREG, COMMAND);
  }
}

VOID NEAR InitioStopDMA (NPA npA)
{
  if (npA->BM_CommandCode) {
    OutB (BMCMDREG, (UCHAR)(npA->BM_CommandCode & ~0x81)); /* turn OFF Start bit */
    npA->BM_CommandCode = 0;
  }
}

VOID NEAR InitioErrorDMA (NPA npA)
{
}

int NEAR InitioCheckIRQ (NPA npA) {

  DISABLE
  if (InB (ISRPORT) & 0x80) {
    npA->Flags |= ACBF_BMINT_SEEN;
    BMSTATUS = 0;
    STATUS = InB (STATUSREG);
    ENABLE
    return (1);
  }
  ENABLE
  return (0);
}


