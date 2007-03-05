/****************************************************************************
 *									    *
 *			     OCO Source Materials			    *
 *			       IBM Confidential 			    *
 *									    *
 *		      Copyright (c) IBM Corporation 1998		    *
 *			     All Rights Reserved			    *
 * Copyright : COPYRIGHT Daniela Engert 1999-2006			    *
 *									    *
 ****************************************************************************/

#define INCL_NOBASEAPI
#define INCL_NOPMAPI
#define INCL_DOSINFOSEG
#define INCL_NO_SCB
#define INCL_INITRP_ONLY
#define INCL_DOSERRORS
#include <os2.h>
#include <dos.h>
#include <dskinit.h>

#include <iorb.h>
#include <reqpkt.h>
#include <dhcalls.h>
#include <addcalls.h>

#include "s506cons.h"
#include "s506type.h"
#include "s506pro.h"
#include "s506ext.h"

#include "Trace.h"

#pragma optimize(OPTIMIZE, on)

/**@internal VDMInt13Create()
 *  Set up the DiskDD to INT 13 VDM communications area and pass it to
 * DevHelp_CreateInt13VDM.  Verify the VDM was created successfully.
 *
 * @return
 *  FALSE - the mini-VDM was successfully created.
 * @return
 *  TRUE - creation of the mini-VDM failed.
 */
BOOL NEAR VDMInt13Create (VOID)
{
  USHORT rc = 0;
  PUCHAR fnc = (PUCHAR)StubVDMInt13CallBack;

  // Create a mini-VDM to get int 13 (BIOS) data.
  if (!BIOSInt13)      return (VDMInt13Created = -1);
  if (VDMInt13Created) return (VDMInt13Created != 1);

  // Setup Service routine addresses
  // Setup VDM<->diskDD communication area

  (USHORT)VDMInt13.Int13Functions[INT13_FCN_DISKDDCALLOUT+0] = FP_OFF (fnc);
  (USHORT)VDMInt13.Int13Functions[INT13_FCN_DISKDDCALLOUT+1] = FP_SEG (fnc);
  VDMInt13.Int13State	 = VDMINT13_IDLE;
  VDMInt13.Int13PriClass = 0x03;
  VDMInt13.Int13PriLevel = 0x1F;
  VDMInt13.Int13CreateRc = 0;

  if (!(rc = DevHelp_CreateInt13VDM ((char far *)&VDMInt13))) {
    DevHelp_ProcBlock ((ULONG)VDMDskBlkID,	 // Wait for VDM to run
		       (ULONG)-1, 0);
  }

  VDMInt13Created = 1;
  if (rc || VDMInt13.Int13CreateRc)
    VDMInt13Created = -1;
  return (VDMInt13Created != 1);
}



//
//   Int13Fun08GetLogicalGeom()
//
// Get the logical geometry from the legacy BIOS interface.
//
BOOL FAR Int13Fun08GetLogicalGeom (NPU npU)
{
  if (VDMInt13Create()) return (TRUE);
//if (npU->DriveId != 0x81) return TRUE;

  // AH = function number.
  VDMInt13.Int13Regs.R_EAX = BIOS_GET_LEGACY_DRIVE_PARAM << 8;
  // DL = BIOS driver number.
  VDMInt13.Int13Regs.R_EDX = npU->DriveId;

  // Do the Int 13 function.
  if (!Int13DoRequest()) {
    if (!(VDMInt13.Int13Regs.R_EFLAGS & EFLAGS_CF)) {
      npU->I13LogGeom.NumHeads	      = ((VDMInt13.Int13Regs.R_EDX & 0x0000ff00l) >> 8) + 1;
      npU->I13LogGeom.SectorsPerTrack = (VDMInt13.Int13Regs.R_ECX & 0x0000003fl);
      return (FALSE);
    }
  }

  return (TRUE);
}


/*--------------------------------------------------------------------*
 *								      *
 *  Subroutine Name: Int13DoRequest()				      *
 *								      *
 *  This routine unblocks the INT 13 worker thread and blocks until   *
 *  the thread completes its request.				      *
 *								      *
 *--------------------------------------------------------------------*/
BOOL NEAR Int13DoRequest (VOID)
{
  USHORT  rc = FALSE;
  USHORT  AwakeCount;

  VDMInt13Active = TRUE;
  VDMInt13.Int13State = VDMINT13_START;
					      // Ready VDM Worker thread
  if (!(rc = DevHelp_ProcRun ((ULONG)VDMInt13BlkID,
			      (PUSHORT)&AwakeCount))) {
    DevHelp_ProcBlock ((ULONG)VDMDskBlkID, (ULONG)-1, (USHORT)0);
  }

  VDMInt13Active = FALSE;

  return (rc);
}


/*--------------------------------------------------------------------*
 *								      *
 *  Subroutine Name: VMDInt13CallBack() 			      *
 *								      *
 *  This routine gets called on the INT 13 VDM thread when the	      *
 *  VDM has completed a request. The thread which initiated	      *
 *  the request is unblocked and the INT 13 VDM thread is	      *
 *  captured by this routine.					      *
 *								      *
 *--------------------------------------------------------------------*/


VOID NEAR _loadds _cdecl VDMInt13CallBack (VOID)
{
  USHORT rc;
  USHORT AwakeCount;

  DISABLE;
  rc = 0;

  // Wake up DiskDD Thread
  if (!(rc = DevHelp_ProcRun ((ULONG)VDMDskBlkID,
			      (PUSHORT)&AwakeCount))) {
    // Block this (Int13 VDM) thread.  Wait is NOT interruptable.
    DevHelp_ProcBlock ((ULONG)VDMInt13BlkID, (ULONG)-1, 0);
  }
  ENABLE;

  return;
}

