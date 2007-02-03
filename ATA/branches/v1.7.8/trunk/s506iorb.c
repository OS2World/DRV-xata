/**************************************************************************
 *
 * SOURCE FILE NAME = S506IORB.C
 *
 * DESCRIPTIVE NAME = DaniS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : IORB Routine/Processing
 ****************************************************************************/


#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#define INCL_INITRP_ONLY

#include "os2.h"
#include "dskinit.h"
#include "devclass.h"

#include "iorb.h"
#include "reqpkt.h"
#include "dhcalls.h"
#include "addcalls.h"

#include "s506cons.h"
#include "s506type.h"
#include "s506ext.h"
#include "s506pro.h"
#include "s506regs.h"

#include "trace.h"

#pragma optimize(OPTIMIZE, on)

#define PCITRACER 0
#define TRPORT 0xD020

#ifdef CHECK_HANDLES
VOID NEAR Trap3 (USHORT Code, void *value)
{
  _asm int 3
}
#endif

/*------------------------------*/
/* ADDEntryPoint		*/
/* -------------		*/
/*				*/
/* ADD IORB command entry.	*/
/*				*/
/*------------------------------*/


VOID FAR _loadds _cdecl ADDEntryPoint (PIORBH pIORB)
{
  PIORBH pFirstIORB;
  PIORBH pLastIORB;
  NPA  npA;
  NPU  npU;

#if PCITRACER
  outpw (TRPORT, 0xDEDE);
#endif

  LoadFlatGS();

  pFirstIORB = pIORB;

  /*------------------------------------------*/
  /* Queue IORB commands which dont address a */
  /* specific unit to ACB 0 Unit 0	      */
  /*------------------------------------------*/
  if (pFirstIORB->CommandCode == IOCC_CONFIGURATION) {
    pFirstIORB->UnitHandle = (USHORT)ACBPtrs[0]->UnitCB;
  }

  npU = (NPU)pFirstIORB->UnitHandle;
#if PCITRACER
  outpw (TRPORT+2, npU);
  outpw (TRPORT+2, REQ (pFirstIORB->CommandCode, pFirstIORB->CommandModifier));
  OutD (TRPORT, npU->Flags);
#endif

#ifdef CHECK_HANDLES
  {
    // Since this driver uses a lot of near pointers, it is possible
    // to get a bad UnitHandle and use it for a while before trapping
    // or worse continuing.  This check and the one below for the ACB
    // confirm the resulting pointer are referencing valid internal
    // structures.
    // Validate the unit handle.
    int a;

    if (npU) {
      for (a = 0; a < cAdapters; a++) {
	if (npU == (NPU)(&(ACBPtrs[a]->UnitCB[0]))) goto goodUCB;
	if (npU == (NPU)(&(ACBPtrs[a]->UnitCB[1]))) goto goodUCB;
      }
    }

    // This is an invalid UCB.
    Trap3 (1, npU);

goodUCB:

    npA = npU->npA;

    // Validate the ACB pointer.
    if (npA) {
      if (npA == ACBPtrs[a]) {
outpw (TRPORT+2, npA);
outp  (TRPORT+2, a);
	goto goodACB;
      }
    }

    // This is an invalid ACB.
    Trap3 (2, npA);

goodACB:
    ;
  }
#else
  npA = npU->npA;
#endif

  if (npA->FlagsT & ATBF_PCMCIA) LastAccessedPCCardUnit = npA;
  pLastIORB = PreProcessIORBs (npA, npU, &pFirstIORB);

  DISABLE
  if (pFirstIORB) {
    /*------------------------------------------------*/
    /* Add non-preprocessed IORBs to end of the ACB Q */
    /*------------------------------------------------*/
    if (!npA->pHeadIORB)
      npA->pHeadIORB = pFirstIORB;
    else
      npA->pFootIORB->pNxtIORB = pFirstIORB;

    /*--------------------------------------*/
    /* Point to the last IORB on the ACB Q  */
    /*--------------------------------------*/
    npA->pFootIORB = pLastIORB;
  }

  /*
  ** Check if there is any work on the queue, try to restart the
  ** state machine.
  ** Work could have been added to the current ACB only by adding
  ** onto the normal work queue and/or onto the suspend queue,
  ** check both to see if we should restart the state machine.
  */
  if (npA->pHeadIORB || npA->pSuspendHead) {
    /*
    ** Restart the state machine.
    */
    if (!(npA->Flags & ACBF_SM_ACTIVE)) {
      npA->Flags |= ACBF_SM_ACTIVE;
      ENABLE
      StartSM (npA);
    }
  }

#if PCITRACER
  outpw (TRPORT, 0xDEDF);
#endif
  ENABLE
}


/*
** NextIORB
**
** Called from StartState() with interrupts disabled so additional,
** synchronized processing may be done in StartState() upon return.
** Prepair the ACB to process the next IORB on its queue.
**
** Return 0 if there is another IORB to process.
** Return 1 if the queue is empty.
*/
USHORT NEAR NextIORB (NPA npA)
{
  /*---------------------------------------------------------------*/
  /* Get the next IORB from the HWResoure queue of suspended IORBs */
  /* or from the ACB's IORB queue of waiting requests.  First try  */
  /* to get a suspend IORB and then try for one on the ACB queue.  */
  /*
  /* If there are any IORBs on the suspend queue and if there are  */
  /* no IORBs on the ACB's queue or if the suspend count is        */
  /* counted down then grant the suspend request.  Otherwise	   */
  /* decrement the suspend count.				   */
  /*								   */
  /* If there are no suspend IORBs to process then process an IORB */
  /* from the ACB's queue.                                         */
  /*---------------------------------------------------------------*/

  if (npA->pSuspendHead) {

    if (!npA->pHeadIORB)	     npA->CountUntilSuspend = 0;
    else if (npA->CountUntilSuspend) npA->CountUntilSuspend--;

    if (!npA->CountUntilSuspend) {
      npA->pIORB = npA->pSuspendHead;
      if (!(npA->pSuspendHead = (npA->pSuspendHead)->pNxtIORB)) {
	npA->pSuspendFoot = 0;
      } else { /* Setup defered count for next suspend request */
	npA->CountUntilSuspend =
	  (((PIORB_DEVICE_CONTROL)npA->pSuspendHead)->Flags & DC_SUSPEND_IMMEDIATE) ?
	  IMMEDIATE_COUNT : DEFERRED_COUNT;
      }
    } else {
      /*------------------------------------------*/
      /* Process an IORB from the ACB's Queue     */
      /*------------------------------------------*/
      if (npA->pIORB = npA->pHeadIORB) {
	if (!(npA->pHeadIORB = npA->pIORB->pNxtIORB))
	  npA->pFootIORB = 0;
      }
    }
  } else {
    /*------------------------------------------*/
    /* Process an IORB from the ACB's Queue     */
    /*------------------------------------------*/
    if (npA->pIORB = npA->pHeadIORB) {
      if (!(npA->pHeadIORB = npA->pIORB->pNxtIORB))
	npA->pFootIORB = 0;
    }
  }

  if (npA->pIORB) {
    npA->IORBError  = 0;
    npA->IORBStatus = 0;
    return (FALSE);
  } else {
    return (TRUE);
  }
}

/*--------------------------------------------------------------*/
/*  GetDeviceTable()						*/
/*								*/
/*  Handles an IOCM_GET_DEVICETABLE request			*/
/*								*/
/*  Called from PreProcessIORBs(); this IORB is not placed on	*/
/*  the ACB's IORB queue.                                       */
/*--------------------------------------------------------------*/

#define doSCSI DriveId

static BOOL NEAR isSCSI (NPU npU) {
  return (doSCSI && (npU->Flags & UCBF_ATAPIDEVICE) &&
	  (npU->pUnitInfo != 0) && (npU->pUnitInfo->UnitSCSITargetID < 3));
}

static USHORT NEAR GetVirtualAdapters (NPA npA, UCHAR findSCSI)
{
  UCHAR count = 0;
  NPU	npU, npUF;

  for (npU = npA->UnitCB; npU < (npA->UnitCB + npA->cUnits); npU++) {
    npUF = (NPU)((USHORT)npU + 4);
    if (findSCSI == isSCSI (npU))
      count++;

    if (npUF->Flags & UCBF_FAKE) {
      if (findSCSI == isSCSI (npUF))
	count++;
    }
  }
  return (count);
}

void NEAR FillUnitInfo (PUNITINFO pUI, NPU npU, UCHAR AIndex, UCHAR UIndex)
{
#if PCITRACER
  outpw (TRPORT, 0xDC02);
#endif

  if (!SELECTOROF(npU->pUnitInfo)) { /* If unit info was not changed */
    UCHAR fake = (npU->Flags & UCBF_FAKE) ? 1 : 0;

    fclrmem (pUI, sizeof(UNITINFO));

    pUI->AdapterIndex	 = AIndex;
    pUI->UnitHandle	 = (USHORT)npU;
    pUI->FilterADDHandle = 0;
    pUI->QueuingCount	 = 3;
    pUI->UnitFlags	 = UF_NOSCSI_SUPT;

    /*
    ** This is for docking/swapping support.  Transfer the
    ** FORCE and NOTPRESENT flags only if the unit's
    ** presense is being forced from the command line.
    */
    if (fake) {
      npU = (NPU)((USHORT)npU - 4);
      pUI->UnitType = UIB_TYPE_ATAPI;
    } else {
      if (npU->Flags & UCBF_FORCE)
	pUI->UnitType = UIB_TYPE_DISK;
      else
	pUI->UnitType = npU->Flags & UCBF_ATAPIDEVICE ? UIB_TYPE_ATAPI
						      : UIB_TYPE_DISK;
    }

    pUI->UnitIndex	  = npU->UnitIndex;
    pUI->UnitSCSITargetID = 0x80 + npU->UnitIndex;

    if (npU->Flags & UCBF_PCMCIA) {
      pUI->UnitFlags |= UF_CHANGELINE;
      if (npU->Flags & UCBF_NODASDSUPPORT) {
					   pUI->UnitFlags |= UF_NODASD_SUPT;
      } else {
	if (npU->Flags & UCBF_FORCE)	   pUI->UnitFlags |= UF_FORCE;
	if (npU->Flags & UCBF_NOTPRESENT)  pUI->UnitFlags |= UF_NOTPRESENT;
      }
    } else {
      if (npU->Flags & UCBF_NODASDSUPPORT) pUI->UnitFlags |= UF_NODASD_SUPT;
      if (npU->Flags & UCBF_FORCE) {
					   pUI->UnitFlags |= UF_FORCE;
					   pUI->UnitFlags |= UF_CHANGELINE;
	if (npU->Flags & UCBF_NOTPRESENT) {
					   pUI->UnitFlags |= UF_NOTPRESENT;
	} else {
	  if (((npU->Flags & UCBF_ATAPIDEVICE) && !fake) ||
	      (!(npU->Flags & UCBF_ATAPIDEVICE) && fake))
					   pUI->UnitFlags |= UF_NOTPRESENT;
	}
      } else {
	NPA npA = npU->npA;

	if (npU->Flags & UCBF_NOTPRESENT)  pUI->UnitFlags |= UF_NOTPRESENT;
	if ((npU->Flags & UCBF_ATAPIDEVICE) &&
	    (DATAREG >= 0x10000))	   pUI->UnitFlags |= UF_NOTPRESENT;
      }
    }
    if (npU->Flags & UCBF_REMOVABLE)	   pUI->UnitFlags |= UF_REMOVABLE;

#if PCITRACER
  outpw (TRPORT, pUI->UnitFlags);
  outp	(TRPORT, pUI->UnitType);
#endif
  } else { /* Pass back the changed unit info */
    *pUI = *(npU->pUnitInfo);
#if PCITRACER
  outpw (TRPORT, 0xDCFF);
#endif
  }
}

VOID NEAR GetDeviceTable (PIORB pIORB)
{
  USHORT       LengthNeeded;
  UCHAR        NumAdapters, NumUnits;
  UCHAR        i, j, k;
  PADAPTERINFO pADPT;
  PUNITINFO    pUI;
  PDEVICETABLE pDT;
  PULONG       pSCSIMGR;
  NPA	       npA;
  NPU	       npU, npUF;
  UCHAR        CurrentAdapter, CurrentUnit;
  UCHAR        PCCard;

#define pIORBc ((PIORB_CONFIGURATION)pIORB)

  pSCSIMGR = (PULONG)pIORB;
  OFFSETOF (pSCSIMGR) = 10;
  doSCSI = (pSCSIMGR[0] == 0x49534353ul) && (pSCSIMGR[1] == 0x2452474Dul);
  pDT = pIORBc->pDeviceTable;
  NumAdapters = 0;
  NumUnits = 0;

  for (i = 0; i < cAdapters; i++) {
    npA = ACBPtrs[i];
    if ((j = GetVirtualAdapters (npA, 0)) != 0) {
      NumAdapters++;
      NumUnits += j;
    }
    if ((k = GetVirtualAdapters (npA, 1)) != 0) {
      NumAdapters++;
      NumUnits += k;
    }
  }

//DevHelp_Beep (1000, 50);
//DevHelp_ProcBlock ((ULONG)(PVOID)&CompleteInit, 1000UL, 0);

  LengthNeeded = sizeof(DEVICETABLE)
	       + (NumAdapters - 1) * sizeof(NPADAPTERINFO)
	       + NumAdapters * sizeof(ADAPTERINFO)
	       + (NumUnits - NumAdapters) * sizeof(UNITINFO);
#if PCITRACER
  outpw (TRPORT, 0xDB01);
  outp (TRPORT, NumAdapters);
  outp (TRPORT+1, NumUnits);
  outpw (TRPORT, LengthNeeded);
  outpw (TRPORT, pIORBc->DeviceTableLen);
#endif

  if (pIORBc->DeviceTableLen < LengthNeeded) {
    pIORBc->iorbh.ErrorCode = IOERR_CMD_SYNTAX;
    pIORBc->iorbh.Status   |= IORB_ERROR;
    return;
  }

  pDT->ADDLevelMajor = ADD_LEVEL_MAJOR;
  pDT->ADDLevelMinor = ADD_LEVEL_MINOR;
  pDT->ADDHandle     = ADDHandle;
  pDT->TotalAdapters = NumAdapters;

  pADPT = (PADAPTERINFO)&(pDT->pAdapter[NumAdapters]);

  CurrentAdapter = 0;
  for (i = 0; i < cAdapters; i++) {
    npA = ACBPtrs[i];

#if PCITRACER
  outpw (TRPORT, 0xDC00);
  outpw (TRPORT, i | (CurrentAdapter << 8));
#endif

    for (k = 0; k < 2; k++) {
#if PCITRACER
  outp (TRPORT+1, k);
#endif

      if (!GetVirtualAdapters (npA, k)) continue;

      pDT->pAdapter[CurrentAdapter] = (NPADAPTERINFO)pADPT;

      PCCard = (k == 0) && CSHandle && (npA->FlagsT & ATBF_PCMCIA);
      memcpy (pADPT->AdapterName, PCCard ? AdapterNamePCCrd :
					   AdapterNameATAPI, sizeof (AdapterNameATAPI));
      pADPT->Reserved		  = 0;
      pADPT->AdapterIOAccess	  = AI_IOACCESS_PIO;
      pADPT->AdapterHostBus	  = AI_HOSTBUS_OTHER | AI_BUSWIDTH_32BIT;
      pADPT->AdapterSCSILUN	  = 0;
      pADPT->AdapterFlags	  = AF_16M | AF_HW_SCATGAT;
      pADPT->MaxHWSGList	  = 0;
      pADPT->MaxCDBTransferLength = 0L;
      if (k == 0) {
	pADPT->AdapterDevBus	   = AI_DEVBUS_ST506 | AI_DEVBUS_16BIT;
	pADPT->AdapterSCSITargetID = 0;
      } else {
	pADPT->AdapterDevBus	   = AI_DEVBUS_SCSI_2 | AI_DEVBUS_16BIT;
	pADPT->AdapterSCSITargetID = 7;
	pADPT->AdapterFlags	  |= AF_ASSOCIATED_DEVBUS;
      }

      CurrentUnit = 0;
      for (j = 0, npU = npA->UnitCB; j < npA->cUnits; j++, npU++) {
	npUF = (NPU)((USHORT)npU + 4);

#if PCITRACER
  outpw (TRPORT, 0xDC01);
  outpw (TRPORT, j | (CurrentUnit << 8));
  OutD (TRPORT, npU->Flags);
  OutD (TRPORT, npUF->Flags);
#endif

	if (k == isSCSI (npU)) {
	  pUI = &pADPT->UnitInfo[CurrentUnit];
	  FillUnitInfo (pUI, npU, CurrentAdapter, CurrentUnit);
	  CurrentUnit++;
	}
	if ((npUF->Flags & UCBF_FAKE) && (k == isSCSI (npUF))) {
	  pUI = &pADPT->UnitInfo[CurrentUnit];
	  FillUnitInfo (pUI, npUF, CurrentAdapter, CurrentUnit);
	  CurrentUnit++;
	}
      }

      pADPT->AdapterUnits = CurrentUnit;
      CurrentAdapter++;
      pADPT = (PADAPTERINFO)&(pADPT->UnitInfo[CurrentUnit]);
    }
  }

#if PCITRACER
  outpw (TRPORT, 0xDC03);
  outp (TRPORT, CurrentAdapter);
#endif
//DevHelp_Beep (1600, 50);
//DevHelp_ProcBlock ((ULONG)(PVOID)&CompleteInit, 1000UL, 0);
}

/*--------------------------------------------------------------*/
/*  CompleteInit()						*/
/*								*/
/*  Called from PreProcessIORBs(); this IORB is not placed on	*/
/*  the ACB's IORB queue.                                       */
/*								*/
/*--------------------------------------------------------------*/

VOID NEAR CompleteInit (NPA pA)
{
  CHAR	Adapter, Unit;
  UCHAR rc;
  NPC	npC;
  NPA	npA;
  NPU	npU;

//DevHelp_Beep (2000, 50);
//DevHelp_ProcBlock ((ULONG)(PVOID)&CompleteInit, 1000UL, 0);

//outpw (TRPORT+0x10, 0xDEDE);

  BIOSActive = 0;

  for (npC = ChipTable; npC < (ChipTable + MAX_ADAPTERS); npC++) {
    if ((USHORT)npC->npA[0] | (USHORT)npC->npA[1])
      ConfigurePCI (npC, PCIC_INIT_COMPLETE);
  }

  for (Adapter = 0; Adapter < cAdapters; Adapter++) {
    npA = ACBPtrs[Adapter];

    for (npU = npA->UnitCB, Unit = npA->cUnits; --Unit >= 0; npU++) {
      if ((npU->Flags & UCBF_NOTPRESENT) ||
	  (npA->FlagsT & ATBF_BIOSDEFAULTS))
	continue;

      ReInitUnit (npU);
      if (npU->Flags & UCBF_ATAPIDEVICE) NoOp (npU); // issue settings
    }
  }

  /* start idle ticker */
  ADD_StartTimerMS (&IdleTickerHandle, 30000UL, (PFN)IdleTicker, 0);

//DevHelp_Beep (1600, 50);
//DevHelp_ProcBlock ((ULONG)(PVOID)&CompleteInit, 1000UL, 0);
}


/*------------------------------------------*/
/*  AllocateUnit()			    */
/*					    */
/*  Mark a drive as allocated		    */
/*					    */
/*------------------------------------------*/

VOID NEAR AllocateUnit (NPA npA)
{
  NPU npU = npA->npU;

  if (npU->Flags & UCBF_ALLOCATED) {
    Error (npA, IOERR_UNIT_ALLOCATED);
  } else {
    npU->Flags |= UCBF_ALLOCATED;
  }
}


/*------------------------------------------*/
/*  DeallocateUnit()			    */
/*					    */
/*  Deallocate a previously allocated drive */
/*					    */
/*------------------------------------------*/

VOID NEAR DeallocateUnit (NPA npA)
{
  NPU npU = npA->npU;

  if (!(npU->Flags & UCBF_ALLOCATED)) {
    Error (npA, IOERR_UNIT_NOT_ALLOCATED);
  } else {
    npU->Flags &= ~UCBF_ALLOCATED;
  }
}


/*------------------------------------------*/
/*  ChangeUnitInfo()			    */
/*					    */
/*  Store pointer to new UNITINFO structure */
/*					    */
/*------------------------------------------*/

VOID NEAR ChangeUnitInfo (NPA npA)
{
  NPU npU = npA->npU;

  if (!npU->Flags & UCBF_ALLOCATED) {
    Error (npA, IOERR_UNIT_NOT_ALLOCATED);
  } else {
    /* Save the Unit Info pointer */
    npU->pUnitInfo = ((PIORB_UNIT_CONTROL)(npA->pIORB))->pUnitInfo;
  }
}


/*--------------------------------------------------------------*/
/*  GetQueueStatus()						*/
/*								*/
/*  Called from PreProcessIORBs(); this IORB is not placed on	*/
/*  the ACB's IORB queue.                                       */
/*--------------------------------------------------------------*/

VOID NEAR GetQueueStatus (NPA npA, PIORB pIORB)
{
  ((PIORB_DEVICE_CONTROL)pIORB)->QueueStatus = (ULONG)npA->pHeadIORB;
}

/*--------------------------------------------------------------*/
/*  GetUnitResources()						*/
/*								*/
/*  Called from PreProcessIORBs(); this IORB is not placed on	*/
/*  the ACB's IORB queue.                                       */
/*--------------------------------------------------------------*/

VOID NEAR GetUnitResources (NPA npA, PIORB pIORBR)
{
  PIORB_RESOURCE  pIORB   = (PIORB_RESOURCE)pIORBR;
  PRESOURCE_ENTRY pREHead = pIORB->pResourceEntry;
  PBYTE 	  pRE;
  USHORT	  ReqLength;
  USHORT	  fix;
  PIRQ_ENTRY	  pIRQ;
  PPORT_ENTRY	  pPort;

  ReqLength = sizeof (RESOURCE_ENTRY) + sizeof (IRQ_ENTRY) + 2 * sizeof (PORT_ENTRY);

  if (ReqLength <= pIORB->ResourceEntryLen) { /* Valid request */
    fclrmem (pREHead, pIORB->ResourceEntryLen);

    /* The resource entry structure contains pointers to variable length
       arrays.	Because of this, the called driver must manage the structure
       and its contents. This is accomplished by using the pRE pointer.
       It initially points to the first non-used byte in the Resource Entry
       buffer.	Each time a resource entry is added, the it is incremented
       by the size of the latest entry.
    */

    /*point directly after resource entry structure */
    pRE = (PBYTE) (pREHead+1);

    /* Update Resource Entry structure	*/
    pREHead->cIRQ_Entries = 1;
    pREHead->npIRQ_Entry = (NPIRQ_ENTRY)pRE;

    pIRQ = (PIRQ_ENTRY) pRE;
    pRE  = (PBYTE)	(pIRQ+1);	    /* point past current structure */

    if (npA->FlagsT & ATBF_INTSHARED)
      pIRQ->IRQ_Flags = RE_SYSTEM_RESOURCE | IRQ_LOW_LEVEL_TRIGGERED;
    else
      pIRQ->IRQ_Flags = RE_ADAPTER_RESOURCE | IRQ_RISING_EDGE_TRIGGERED;
    pIRQ->IRQ_Value = npA->IRQLevel;

    /* Update Resource Entry structure	*/
    pREHead->cPort_Entries = 2;
    pREHead->npPort_Entry = (NPPORT_ENTRY)pRE;

    pPort = (PPORT_ENTRY) pRE;
    pRE   = (PBYTE)	 (pPort+1);	   /* point past current structure */

    pPort->Port_Flags = RE_ADAPTER_RESOURCE;
    if (npA->UnitCB[0].Flags & UCBF_PIO32) pPort->Port_Flags |= 0x100;
    if (npA->UnitCB[1].Flags & UCBF_PIO32) pPort->Port_Flags |= 0x200;
    pPort->StartPort  = DATAREG;
    pPort->cPorts     = 8;

    pPort = (PPORT_ENTRY) pRE;
    pRE   = (PBYTE)	 (pPort+1);	   /* point past current structure */

    pPort->Port_Flags = RE_ADAPTER_RESOURCE | (npA->HardwareType & 0xFF);
    pPort->StartPort  = DEVCTLREG;
    pPort->cPorts     = 2;

    ReqLength += sizeof(PORT_ENTRY);	   /* one more port entry */
    if ((npA->pSGL) && (ReqLength <= pIORB->ResourceEntryLen)) {
      pREHead->cPort_Entries++; 	  /* one more port entry */

      pPort = (PPORT_ENTRY) pRE;
      pRE   = (PBYTE)	   (pPort+1);	 /* point past current structure */

      if (npA->FlagsT & ATBF_BM_DMA)
	pPort->Port_Flags = RE_ADAPTER_RESOURCE | 2
			  | (npA->UnitCB[0].Flags & UCBF_BM_DMA ? 4 : 0)
			  | (npA->UnitCB[1].Flags & UCBF_BM_DMA ? 8 : 0);
      else
	pPort->Port_Flags = RE_ADAPTER_RESOURCE | 1;

     if (npA->Cap & CHIPCAP_SATA)
	pPort->Port_Flags |= 16;

      pPort->StartPort = npA->BMICOM;	/* command port is base */
      pPort->cPorts    = 8;		/* 8 ports per channel */
    }

    pREHead->Max_Resource_Entry = RE_PORT; /* Establish last table entry
					  position			    */
  } else {
    Error (npA, IOERR_CMD_SYNTAX);
  }
}


VOID NEAR Error (NPA npA, USHORT ErrorCode)
{
  npA->IORBStatus &= ~IORB_RECOV_ERROR;
  npA->IORBStatus |= IORB_ERROR;
  npA->IORBError   = ErrorCode;
}

/*--------------------------------------------------------------*/
/*  IORBDone()							*/
/*								*/
/*								*/
/*--------------------------------------------------------------*/

VOID NEAR IORBDone (NPA npA)
{
  PIORB pIORB = npA->pIORB;

  /*
  ** If a removable media has changed then ignore any error that
  ** may have occured and report the changed media error.
  ** Report media changed only once.
  */
  DISABLE

  pIORB->ErrorCode = npA->IORBError;
  pIORB->Status    = npA->IORBStatus | IORB_DONE;

#if PCITRACER
outpw (TRPORT+2, 0x9999);
outpw (TRPORT+2, pIORB->Status);
outpw (TRPORT+2, pIORB->ErrorCode);
#endif

  npA->pIORB = 0;
  ENABLE

  if (pIORB->RequestControl & IORB_ASYNC_POST) pIORB->NotifyAddress (pIORB);

#if PCITRACER
  outpw (TRPORT, 0xDEEE);
#endif
}

