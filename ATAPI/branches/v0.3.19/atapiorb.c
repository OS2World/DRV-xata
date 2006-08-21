/**************************************************************************
 *
 * SOURCE FILE NAME =	  ATAPIORB.C
 *
 * DESCRIPTIVE NAME =	  ATAPI IORB Interface to State Machine
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       COPYRIGHT Daniela Engert 1999-2006
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

#include "atapicon.h"
#include "atapireg.h"
#include "atapityp.h"
#include "atapiext.h"
#include "atapipro.h"

#define CHECK_HANDLES 0

#define PCITRACER 0
#define TRPORT 0x9014

#if PCITRACER
#define TR(x) outp (TRPORT,(x));
#else
#define TR(x)
#endif

#pragma optimize(OPTIMIZE, on)

VOID _cdecl _loadds FAR ATAPIADDEntry (PIORBH pNewIORB)
{
  PIORBH pNewsTailIORB;  /* The tail of the iorb chain for pNewIORB */
  NPA	 npA;
  NPU	 npU;

  /*ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
  ³ Queue IORBs which don't address ³
  ³ a specific unit to ACB 0 Unit 0 ³
  ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ*/

#if PCITRACER
  TR(0xB2);
  outpw (TRPORT, (pNewIORB->CommandCode << 8) | pNewIORB->CommandModifier);
#endif

  npU = (NPU) pNewIORB->UnitHandle;
  if (pNewIORB->CommandCode == IOCC_CONFIGURATION)
    pNewIORB->UnitHandle = (USHORT) (npU = ACBPtrs[0].npA->npU);

  npA = npU->npA;

#if CHECK_HANDLES
  if ((npA < ACBPtrs[0].npA) || ((USHORT)npU < (USHORT)npA)) {
    pNewIORB->ErrorCode = IOERR_UNIT_NOT_ALLOCATED;
    goto PreProcessed;
  }
#endif

  if (pNewIORB->CommandCode == IOCC_UNIT_CONTROL) {
    switch (pNewIORB->CommandModifier) {
      case IOCM_ALLOCATE_UNIT :
	if (npU->Flags & UCBF_ALLOCATED) {
	  pNewIORB->ErrorCode = IOERR_UNIT_ALLOCATED;
	} else {
	  npU->Flags |= UCBF_ALLOCATED;
	}
	goto PreProcessed;

      case IOCM_DEALLOCATE_UNIT :
	if (npU->Flags & UCBF_ALLOCATED) {
	  npU->Flags &= ~UCBF_ALLOCATED;
	} else {
	  pNewIORB->ErrorCode = IOERR_UNIT_NOT_ALLOCATED;
	}
	goto PreProcessed;

      case IOCM_CHANGE_UNITINFO :
	if (npU->Flags & UCBF_PROXY) {
	  if (!(npU->Flags & (UCBF_ALLOCATED | UCBF_CDWRITER))) {
	    pNewIORB->ErrorCode = IOERR_UNIT_NOT_ALLOCATED;
	  } else {
	    npU->pUI = ((PIORB_UNIT_CONTROL)pNewIORB)->pUnitInfo;
	  }
	} else {
	  AllocDeallocChangeUnit (npA->SharedhADD, npU->hUnit, IOCM_CHANGE_UNITINFO,
				  ((PIORB_UNIT_CONTROL)pNewIORB)->pUnitInfo);
	}
	goto PreProcessed;

      case IOCM_CHANGE_UNITSTATUS:
	ChangeUnitStatus (npU, (PIORB_CHANGE_UNITSTATUS)pNewIORB);

      PreProcessed:
#if PCITRACER
  TR(0xC0);
  outpw (TRPORT, npU);
  outpw (TRPORT, pNewIORB->ErrorCode);
#endif
	pNewIORB->Status = pNewIORB->ErrorCode ? IORB_DONE | IORB_ERROR : IORB_DONE;
	if (pNewIORB->RequestControl & IORB_ASYNC_POST)
	  pNewIORB->NotifyAddress (pNewIORB);
	return;
    }

  }

  DISABLE

  /*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    ³ Put new IORB on end of Queue ³
    ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/
  if (!npA->pHeadIORB)
    npA->pHeadIORB = pNewIORB;
  else
    npA->pFootIORB->pNxtIORB = pNewIORB;

  /*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿*/
  /*³Find end of chained request³*/
  /*ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/
  for (pNewsTailIORB = pNewIORB;
       pNewsTailIORB->RequestControl & IORB_CHAIN;
       pNewsTailIORB = pNewsTailIORB->pNxtIORB)
    ;

  /*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿*/
  /*³ Point the foot of the IORB Queue to the ³*/
  /*³ tail of the new IORB chain.	      ³*/
  /*ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/
  npA->pFootIORB = pNewsTailIORB;

  /*ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿*/
  /*³Make sure pNxtIORB for the for the  ³*/
  /*³Last IORB on our Q is clear.	 ³*/
  /*ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ*/
  npA->pFootIORB->pNxtIORB = 0;

  TR(0xB3)

  if (!(npA->OSMFlags & ACBOF_SM_ACTIVE)) {

  TR(0xB4)

    npA->OSMFlags |= ACBOF_SM_ACTIVE;

    NextIORB(npA);

    npA->OSMState	= ACBOS_START_IORB;
    npA->IORBStatus	= 0;
    npA->IORBError	= 0;
    npA->cResetRequests = 0;
    npA->npCmdIO	= &npA->ExternalCmd;

    if (!(npA->ResourceFlags & ACBRF_CURR_OWNER) &&
	!(pNewIORB->CommandCode == IOCC_CONFIGURATION) &&
	!(pNewIORB->CommandCode == IOCC_UNIT_CONTROL)  &&
	!(pNewIORB->CommandCode == IOCC_RESOURCE)) {

  TR(0xB5)

      npA->OSMState = ACBOS_SUSPEND_COMPLETE;
      ENABLE
      SuspendResumeOtherAdd (npA, IOCM_SUSPEND);

    } else {
      ENABLE
      StartOSM (npA);
    }
  }
  ENABLE
}

/***************************************************************************
*
* FUNCTION NAME = x2BCD
*
* DESCRIPTION	= Convert binary byte to BCD byte  (2 digits)
*
*		  PSEUDOCODE:
*		      return  ((n / 10 << 4) | (n % 10));
*
* INPUT 	= binary byte
*
* OUTPUT	=
*
* RETURN-NORMAL = al = BCD
*
* RETURN-ERROR	= none
*
**************************************************************************/

UCHAR NEAR _fastcall x2BCD (UCHAR Data)
{
  _asm {
    xor     ah, ah
    mov     cl, 10
    div     cl
    xchg    ah, al
    shl     ah, 4
    or	    al, ah
  }
}

VOID NEAR Convert_17B_to_ATAPI (ULONG pPhysData)
{
   struct translatetable
   {
      ULONG OldValue;
      ULONG NewValue;
   };

   PBYTE  pData;
   ULONG  Capabilities_17B = *pData+4;
   PBYTE  pCaps 	   =  pData+4;
   UCHAR  i, tsize;

   static struct translatetable table[] =
   {
      { CP_AUDIO_PLAY , 		CP_AUDIO_PLAY },
      { REV_17B_CP_MODE2_FORM1, 	CP_MODE2_FORM1 },
      { REV_17B_CP_MODE2_FORM2, 	CP_MODE2_FORM2 },
      { REV_17B_CP_READ_CDDA,		CP_CDDA },
      { REV_17B_CP_PHOTO_CD,		CP_MULTISESSION },
      { REV_17B_CP_EJECT,		CP_EJECT },
      { REV_17B_CP_LOCK,		CP_LOCK },
      { REV_17B_CP_UPC, 		CP_UPC },
      { REV_17B_CP_ISRC,		CP_ISRC },
      { REV_17B_CP_INDEPENDENT_VOL_LEV, CP_INDEPENDENT_VOLUME_LEVELS },
      { REV_17B_CP_SEPARATE_CH_MUTE,	CP_SEPARATE_CHANNEL_MUTE },
      { REV_17B_CP_TRAY_LOADER, 	CP_TRAY_LOADER },
      { REV_17B_CP_POPUP_LOADER,	CP_POPUP_LOADER },
      { REV_17B_CP_RESERVED_LOADER,	CP_RESERVED_LOADER },
      { REV_17B_CP_PREVENT_JUMPER,	CP_PREVENT_JUMPER },
      { REV_17B_CP_LOCK_STATE,		CP_LOCK_STATE },
      { REV_17B_CP_CDDA_ACCURATE,	CP_CDDA_ACCURATE },
      { REV_17B_CP_C2_POINTERS, 	CP_C2_POINTERS }
   };

   tsize = sizeof(table) / sizeof(struct translatetable);

   DevHelp_PhysToVirt (pPhysData, 1, (PVOID)&pData);

   Capabilities_17B = *pData+4;
   pCaps	    =  pData+4;

   *pCaps = 0;

   for (i = 0; i < tsize; i++) {
      if  (Capabilities_17B & table[i].OldValue)
      {
	 *pCaps |= table[i].NewValue;
      }
   }
}

