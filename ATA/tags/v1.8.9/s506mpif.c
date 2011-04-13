/**************************************************************************
 *
 * SOURCE FILE NAME = S506MPIF.C
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 2002-2009
 * distributed under the terms of the GNU Lesser General Public License
 *
 * DESCRIPTION : PCCard Director interface
 ****************************************************************************/

 #define INCL_NOPMAPI
 #define INCL_NO_SCB
 #define INCL_DOSINFOSEG
 #define INCL_DOSERRORS
 #include "os2.h"
 #include "devcmd.h"
 #include "dskinit.h"

 #include "iorb.h"
 #include "strat2.h"
 #include "reqpkt.h"
 #include "dhcalls.h"
 #include "addcalls.h"

 #include "s506cons.h"
 #include "s506type.h"
 #include "s506ext.h"
 #include "s506pro.h"
 #include "s506regs.h"

#include "Trace.h"

#pragma optimize(OPTIMIZE, on)

#define StatusError(pRPH,ErrorCode) ((PRPH)pRPH)->Status = (ErrorCode)

#define  MY_CATEGORY	     0xF0
#define  CLEAR_ACCESSED_FLAG 0x40
#define  SET_FIRSTDRIVEID    0x43
#define  GET_NUMOFUNITS      0x60
#define  GET_UNITIDLIST      0x61
#define  QUERY_ACCUNITID     0x62
#define  GET_DRIVEATTR	     0x63
#define  GET_FIRSTDRIVEID    0x67

VOID NEAR _cdecl _loadds MPIfIOCtl (PRP_GENIOCTL pRP)
{
  PUSHORT p = ((PUSHORT)pRP->DataPacket);
  UCHAR UnitId;

  switch (pRP->Function) {
    case CLEAR_ACCESSED_FLAG:	      /* Function 40h			   */
      LastAccessedPCCardUnit = NULL;
      StatusError (pRP, STDON);
      break;

    case SET_FIRSTDRIVEID:	      /* Function 43h			   */
      StatusError (pRP, STDON);
      break;

    case GET_NUMOFUNITS:	      /* Function 60h			   */
      p[0] = cSockets;
      StatusError (pRP, STDON);
      break;

    case GET_UNITIDLIST:	      /* Function 61h			   */
      memcpy (pRP->DataPacket, PCCardPtrs, cSockets * sizeof (NPA));
      DISABLE
      while (Inserting) {
	DevHelp_ProcBlock ((ULONG)(PVOID)&CardInsertion, 30000UL, 0);
	DISABLE
      }
      ENABLE
      StatusError (pRP, STDON);
      break;

    case QUERY_ACCUNITID:	      /* Function 62h			   */
      p[0] = (USHORT)LastAccessedPCCardUnit;
      StatusError (pRP, STDON);
      break;

    case GET_DRIVEATTR: 	      /* Function 63h			   */
      UnitId = (UCHAR)*(pRP->ParmPacket);
      if (UnitId >= cSockets) {
	StatusError (pRP, STERR | STDON | ERROR_I24_INVALID_PARAMETER);
	return;
      }
      p[0] = 0;
      p[1] = PCCardPtrs[UnitId]->Socket - FirstSocket;
      p[2] = p[3] = 0;
      StatusError (pRP, STDON);
      break;

    case GET_FIRSTDRIVEID:	      /* Function 67h			   */
      p[0] = 0;
      StatusError (pRP, STDON);
      break;
  }
}

