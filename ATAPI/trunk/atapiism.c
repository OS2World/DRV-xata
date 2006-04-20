/******************************************************************************
 *
 * SOURCE FILE NAME =	  ATAPIISM.C
 *
 * DESCRIPTIVE NAME =	  ATAPI Inner State Machine
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
#include "ata.h"

#include "atapicon.h"
#include "atapireg.h"
#include "atapityp.h"
#include "atapiext.h"
#include "atapipro.h"

#pragma optimize(OPTIMIZE, on)

#define PCITRACER 0
#define TRPORT 0xE00C

#if PCITRACER
#define TR(x) outp (TRPORT,(x));
#else
#define TR(x)
#endif

/* ��������������������������������������������������������������������������ͻ
�									      �
�  StartOSMRequest							      �
�									      �
�  Outer State Machine Interface:  This routine initializes the inner state   �
�  machine and sets up the outer state machine to go into a WAITSTATE until   �
�  the inner state machine finishes the requested operation.		      �
�									      �
�����������������������������������������������������������������������������ͼ */
VOID NEAR StartOSMRequest (NPA npA)
{
  npA->ISMState      = ACBIS_START_STATE;
  npA->ISMDoneReturn = StartOSM;

  StartISM (npA);
}

/* ���������������������������������ͻ
�				     �
�  StartISM			     �
�				     �
�  Inner State Machine Router	     �
�				     �
������������������������������������*/
VOID NEAR StartISM (NPA npA)
{
  NPOSMEntry ISMDoneReturn;

  TR(1)

  if (0 == saveINC (&(npA->ISMUseCount))) {
    do {

      npA->ISMFlags &= ~ACBIF_WAITSTATE;
      do {
#if PCITRACER
  outp (TRPORT, 0x60);
  outpw(TRPORT, (npA->ISMState << 12) | npA->ISMFlags);
#endif
	switch (npA->ISMState) {
	  case ACBIS_START_STATE:
	    ISMStartState (npA) ;
	    break;

	  case ACBIS_INTERRUPT_STATE:
	    InterruptState (npA) ;
	    break;

	  case ACBIS_WRITE_ATAPI_PACKET_STATE:
	    WriteATAPIPkt (npA) ;
	    break;

	  case ACBIS_COMPLETE_STATE:
	    npA->OSMState     = ACBOS_ISM_COMPLETE;
	    npA->ISMUseCount  = 1;
	    npA->ISMFlags    |= ACBIF_WAITSTATE;
	    break;

	  default :					/* unknown state */
	    npA->ISMUseCount  = 1;
	    npA->ISMFlags    |= ACBIF_WAITSTATE;
	    break;
	}
      } while (!(npA->ISMFlags & ACBIF_WAITSTATE));
    } while (saveDEC (&(npA->ISMUseCount)));
  }

  TR(0x61)

  if (npA->ISMState == ACBIS_COMPLETE_STATE) {

  TR(0x62)

    ISMDoneReturn = (NPOSMEntry)saveXCHG ((NPUSHORT)&(npA->ISMDoneReturn), 0);
    if (ISMDoneReturn) {
      (*ISMDoneReturn)(npA);
    }
  }
}

/*
������������������������������������ͻ
�				     �
�  ISMStartState		     �
�				     �
�  Initializes the Inner State	     �
�  Machine			     �
�				     �
������������������������������������*/
VOID NEAR ISMStartState (NPA npA)
{
  NPU npU = npA->npU;

  TR(2)

  if ((npU->Flags & (UCBF_NOTPRESENT | UCBF_FORCE)) == (UCBF_NOTPRESENT | UCBF_FORCE)) {
    // This is a not-present forced drive, fake the request
    if (npA->ISMFlags & ACBIF_ATA_OPERATION) {
      FakeATA (npA);
    } else {
      FakeATAPI (npA);
    }
    npA->ISMFlags &= ~ACBIF_WAITSTATE;
    npA->ISMState = ACBIS_COMPLETE_STATE;   // force completion
    return;   //done
  }

  outp (DRVHDREG, DRVHD = npU->DriveHead);

  if (!(npA->OSMReqFlags & ACBR_RESET) &&
     (!DRQ_AND_BSY_CLEAR_WAIT (npA))) {
					      /* Max Poll wait time exceeded */
  TR(3)

    if (npU->Flags & UCBF_FORCE) {
      npU->Flags     |= UCBF_NOTPRESENT; //set fake flag
      npA->IORBError  = IOERR_UNIT_NOT_READY;
      npA->ISMState   = ACBIS_COMPLETE_STATE;
    } else {
      npA->ISMState	= ACBIS_COMPLETE_STATE;
      npA->OSMReqFlags |= ACBR_RESET;  /* Set OSM for reset */
      return;
    }
  } else {

    InitializeISM (npA) ;
    StartATA_ATAPICmd (npA);
  }
}

/*
���������������������������������������ͻ
�					�
�  InitializeISM			�
�					�
�  Initializes state and flag variables �
�					�
���������������������������������������ͼ */
VOID NEAR InitializeISM (NPA npA)
{
  NPU	 npU = npA->npU;
  NPCMDIO npCmdIO = npA->npCmdIO;
  UCHAR  Cmd = npCmdIO->ATAPIPkt[0];
  struct CDB_ModeSense_10  NEAR *pModeSenseCmd; /* in order to fix 1.7B drives */
  struct CDB_PlayAudio_MSF NEAR *pPlayAudioMSFCmd;

  TR(4)

  if (npU->Flags & UCBF_PIO32) npA->ISMFlags |= ACBIF_PIO32;

  /* insert LUN */

  npCmdIO->ATAPIPkt[1] = (npCmdIO->ATAPIPkt[1] & ~0xE0) | npU->CDBLUN;

  /*������������������������������Ŀ
  � Check ATAPI Command Operations �
  ��������������������������������*/
  if (!(npA->ISMFlags & ACBIF_ATA_OPERATION)) {
    npA->BM_CommandCode = 0;
    /*��������������������������Ŀ
    � set opcode dependent flags �
    ����������������������������*/

    switch (Cmd) {
      /*---------------------*/
      /*      Block IO	     */
      /*---------------------*/
      case  SCSI_READ_6     :
      case  SCSI_WRITE_6    :
      case  FXP_READ_10     :
      case  FXP_WRITE_10    :
      case  FXP_READ_12     :
      case  FXP_WRITE_12    :
      case  FXP_WRITE_VERIFY:
      case ATAPI_READ_CD     :
      case ATAPI_READ_CD_MSF :

      case  FXP_READ_REVERSE:
      case  SCSI_RECOVER_BUFFER:
      case  FXP_WRITE_SAME:
      case  FXP_SEND_CUE_SHEET:
      case  FXP_WRITE_VERIFY_12:

	if (InitIOComplete &&
	    !(npA->ISMFlags & ACBIF_BUSMASTERDMA_FORCEPIO) &&
	    (npU->Flags & UCBF_BM_DMA) &&
	    !(npCmdIO->cXferBytesRemain & 1)) {
	  if(!CreateBMSGList (npA)) {	/* try and create scatter/gather list */
	    /* change transfer setup to Bus Master DMA */

	    npA->ISMFlags |= ACBIF_BUSMASTERDMAIO;

	    npA->BM_CommandCode = (DirTable[Cmd >> 3] & (0x80 >> (Cmd & 7))) ?
				   BMICOM_START : BMICOM_START | BMICOM_TO_HOST ;

	    /* Shut down Bus Master DMA controller */

	    outpd (npA->BMIDTP, npA->ppSGL);
	    outp  (npA->BMICOM, npA->BM_CommandCode & ~BMICOM_START);
	    outp  (npA->BMISTA, npA->BMStatus | (BMISTA_INTERRUPT | BMISTA_ERROR));

	    #ifdef ENABLE_COUNTERS
    //		      ++npU->DeviceCounters.TotalBMReadOperations;
	    #endif

	    /*
	     * Bus Master DMA is now ready for the transfer.  It must be started after the
	     * command is issued to the drive, to avoid data corruption in case a spurious
	     * interrupt occurred at the beginning of the xfer and the drive is in an unknown
	     * state.
	     */
	  }
	  npA->ISMFlags &= ~ACBIF_BUSMASTERDMA_FORCEPIO;
	}

	break;

      case FXP_SYNC_CACHE :
      case FXP_CLOSE : if (npA->IRQTimeOut < 600000UL)	npA->IRQTimeOut = 600000UL;  break;

      case FXP_NEW_FORMATUNIT:
      case FXP_BLANK : if (npA->IRQTimeOut < 3600000UL) npA->IRQTimeOut = 3600000UL; break;
    }
    npA->ISMFlags |= ACBIF_WAITSTATE;
  }

  /*���������������������������������������������Ŀ
  � If drive is only revision 1.7B compatible,	  �
  � change the opcodes to the 1.7B spec's opcodes �
  � and fix the inconsistancies in the data	  �
  �����������������������������������������������*/
  if (npU->Capabilities & UCBC_SPEC_REV_17B) {
    switch (Cmd) {
      case ATAPI_AUDIO_SCAN :
	npCmdIO->ATAPIPkt[0] = REV_17B_ATAPI_AUDIO_SCAN;
	break;

      case ATAPI_SET_CDROM_SPEED :
	npCmdIO->ATAPIPkt[0] = REV_17B_ATAPI_SET_CDROM_SPEED;
	break;

      case ATAPI_READ_CD :
	npCmdIO->ATAPIPkt[0] = REV_17B_ATAPI_READ_CD;
	break;

      case ATAPI_READ_CD_MSF :
	npCmdIO->ATAPIPkt[0] = REV_17B_ATAPI_READ_CD_MSF;
	break;

      case SCSI_PLAY_MSF :
	pPlayAudioMSFCmd = (struct CDB_PlayAudio_MSF NEAR *)npCmdIO->ATAPIPkt;
	/* silly 1.7B drives want this data in BCD */
	pPlayAudioMSFCmd->starting_M = x2BCD(pPlayAudioMSFCmd->starting_M);
	pPlayAudioMSFCmd->starting_S = x2BCD(pPlayAudioMSFCmd->starting_S);
	pPlayAudioMSFCmd->starting_F = x2BCD(pPlayAudioMSFCmd->starting_F);
	pPlayAudioMSFCmd->ending_M   = x2BCD(pPlayAudioMSFCmd->ending_M);
	pPlayAudioMSFCmd->ending_S   = x2BCD(pPlayAudioMSFCmd->ending_S);
	pPlayAudioMSFCmd->ending_F   = x2BCD(pPlayAudioMSFCmd->ending_F);
	break;

      case SCSI_MODE_SENSE_10 :
	pModeSenseCmd = (struct CDB_ModeSense_10 NEAR *)npCmdIO->ATAPIPkt;
	if (pModeSenseCmd->page_code == PAGE_CAPABILITIES)
	   pModeSenseCmd->page_code = REV_17B_PAGE_CAPABILITIES;
	break;
    }
  }
}

/* ���������������������������������ͻ
�				     �
�  StartATA_ATAPICmd		     �
�				     �
�  Write an ATA or ATAPI Cmd	     �
�				     �
������������������������������������*/
VOID NEAR StartATA_ATAPICmd (NPA npA)
{
  USHORT IOMask;

  TR(5)

  /*��������������������������������������Ŀ
  � Do not write if unknown command	   �
  ����������������������������������������*/
  if (npA->IORBError & IOERR_CMD_NOT_SUPPORTED) {
    npA->ISMState      = ACBIS_COMPLETE_STATE;
    npA->ISMDoneReturn = 0;
    return;
  }

  /*��������������������������������������Ŀ
  � Processing of the ATA or ATAPI command �
  ����������������������������������������*/
  if (!(npA->ISMFlags & ACBIF_ATA_OPERATION)) {
    IOMask = FM_PATAPI_CMD;	      /* ATAPI Packet Command Register Mask */
    SetATAPICmd (npA) ;
  } else {
    IOMask = FM_PATA_CMD;	      /* ATA Command Register Mask */
    SetATACmd (npA) ;
  }

  /*��������������������������������Ŀ
  � Actual write of registers	     �
  ����������������������������������*/
  if (IOMask & FM_PINTRSN)  { outp (SECCNTREG  , SECCNT ) ; IODelay();}
  if (IOMask & FM_PSAMTAG)  { outp (SAMTAGREG  , SAMTAG ) ; IODelay();}
  if (IOMask & FM_PBCNTL )  { outp (BCNTLREG   , BCNTL	) ; IODelay();}
  if (IOMask & FM_PBCNTH )  { outp (BCNTHREG   , BCNTH	) ; IODelay();}
  if (IOMask & FM_PFEAT  )  { outp (FEATREG    , FEAT	) ; IODelay();}
  if (IOMask & FM_PCMD	 )  { outp (COMMANDREG , COMMAND) ; IODelay();}

  TR(6)
}


/*����������������������������������ͻ
�				     �
�  SetATAPICmd			     �
�				     �
�  Write an ATAPI CMD and Paramters  �
�				     �
������������������������������������*/
VOID NEAR SetATAPICmd (NPA npA)
{
  NPCMDIO npCmdIO = npA->npCmdIO;
  USHORT  cntBytes;

  TR(7)

  /*---------------------------------*/
  /* Enable if DMA set for this Xfer */
  /*---------------------------------*/
  if (npA->ISMFlags & ACBIF_BUSMASTERDMAIO) {
    FEAT = DEFAULT_ATAPI_FEATURE_REG | DMA_ON;
    if (npA->FlagsT & ATBF_SATA)
      FEAT |= (npA->BM_CommandCode >> 1) & DMA_DIR_TO_HOST;

    BCNTH = BCNTL = 0xFF;
  } else {
    FEAT = DEFAULT_ATAPI_FEATURE_REG & ~DMA_ON;
    /*-----------------------------------------------*/
    /* Set Byte Count registers (High and Low order) */
    /*-----------------------------------------------*/
    if (npCmdIO->cXferBytesRemain > MAX_XFER_BYTES_PER_INTERRUPT)
      cntBytes = MAX_XFER_BYTES_PER_INTERRUPT;
    else
      cntBytes = ~1 & (1 + (USHORT)npCmdIO->cXferBytesRemain);

    *(NPUSHORT)&BCNTL = cntBytes;
  }

  COMMAND = FX_PKTCMD;

  /*---------------------------------------------------------------------------*/
  /* The LBA bit of the ATAPI Drive Select Register is a reserved bit starting */
  /* in revision 1.2, and therefore must be set to 0.  However, in the earlier */
  /* specs, the LBA bit must be 1.					       */
  /*---------------------------------------------------------------------------*/

  DRVHD = npA->npU->DriveHead;

  if (npA->npU->Capabilities & UCBC_INTERRUPT_DRQ) {

  TR(8)

    /*-------------------------*/
    /* Set ISM Flags and State */
    /*-------------------------*/
    npA->ISMFlags |= ACBIF_WAITSTATE;
    npA->ISMState  = ACBIS_INTERRUPT_STATE;

    /*�������������������������Ŀ
    � Start the interrupt timer �
    ���������������������������*/
    ADD_StartTimerMS (&npA->IRQTimeOutHandle, npA->IRQTimeOut,
		      (PFN)IRQTimeOutHandler, npA);
  } else {

  TR(9)

    npA->ISMFlags &= ~ACBIF_WAITSTATE;
    npA->ISMState  = ACBIS_WRITE_ATAPI_PACKET_STATE;
  }
}

/*����������������������������������ͻ
�				     �
�  SetATACmd			     �
�				     �
�  Write an ATA CMD and Paramters    �
�				     �
������������������������������������*/
VOID NEAR SetATACmd (NPA npA)
{
  NPU npU = npA->npU;

  TR(10)

  FEAT	= 0;
  DRVHD = npA->npU->DriveHead & (DEFAULT_ATAPI_DRV_SLCT_REG | UNIT1);

  /*�������������������������Ŀ
  �   Set Command Register    �
  ���������������������������*/
  if (npU->ReqFlags & UCBR_RESET) {
    COMMAND = FX_SOFTRESET;

  } else if (npU->ReqFlags & UCBR_IDENTIFY) {
    COMMAND	   = FX_IDENTIFYDRIVE;
    npU->ReqFlags &= ~UCBR_IDENTIFY;

  } else if (npU->ReqFlags & UCBR_PASSTHRU) {
    npU->ReqFlags &= ~UCBR_PASSTHRU;

  } else {
     /*�������������������������Ŀ
     �	  Unknown ATA command	 �
     ���������������������������*/
     _asm { push ax
	    mov  ax, 0x4444
	    int  3
	    pop  ax
	  }
  }

  /*�������������������������Ŀ
  �  Check for Reset Request  �
  ���������������������������*/
  if (npU->ReqFlags & UCBR_RESET) {
    npU->ReqFlags &= ~UCBR_RESET;
    npA->ISMState  =  ACBIS_COMPLETE_STATE;
    npA->ISMFlags &= ~ACBIF_WAITSTATE;
  } else {

    /*������������������������������������Ŀ
    � Continue with NON-Reset ATA Request  �
    ��������������������������������������*/

    /*�����������������������Ŀ
    � Set ISM Flags and State �
    �������������������������*/
    npA->ISMFlags |= ACBIF_WAITSTATE;
    npA->ISMState  = ACBIS_INTERRUPT_STATE;

    /*�������������������������Ŀ
    � Start the interrupt timer �
    ���������������������������*/
    ADD_StartTimerMS (&npA->IRQTimeOutHandle, npA->IRQTimeOut,
		      (PFN)IRQTimeOutHandler, npA);
  }
}

/*������������������������������������ͻ
�				       �
�  InterruptState		       �
�				       �
�  Routes interrupt generated threads  �
�  to the selected interrupt function  �
�				       �
��������������������������������������*/
VOID NEAR InterruptState (NPA npA)
{
  NPCMDIO npCmdIO = npA->npCmdIO;
  ULONG   cXferBytes;
  USHORT  cExtraBytes = 0;
  UCHAR   Reason;

  TR(11)

  if (!(npA->TimerFlags & ACBT_INTERRUPT)) {
    do
      STATUS = inp (STATUSREG);
    while (STATUS & FX_BUSY);

  TR(12)

    INTRSN = inp (INTRSNREG);
    Reason = ((INTRSN & IR_COMPLETE) | (STATUS & FX_DRQ));

    /*�������������������������Ŀ
    �	  Set ATA bits		�
    ���������������������������*/
    if (npA->ISMFlags & ACBIF_ATA_OPERATION) {
      Reason = IR_XFER_FROM_DEVICE;

      if (COMMAND == FX_GETMEDIASTAT)
	Reason = IR_COMPLETE;
    } else if (!(STATUS & FX_DRQ)) {
      Reason = IR_COMPLETE;
    }

    /*�����������������������������������������������Ŀ
    � If there was an error, go to the complete phase �
    �������������������������������������������������*/
    if (STATUS & FX_ERROR)
      if (!(STATUS & FX_DRQ) || (INTRSN & IRR_COD))
	Reason = IR_COMPLETE;

    switch (Reason) {
      case IR_PKTREADY:
	npA->ISMState  = ACBIS_WRITE_ATAPI_PACKET_STATE;
	npA->ISMFlags &= ~ACBIF_WAITSTATE;
	break;

      case IR_XFER_FROM_DEVICE:

	npCmdIO->IOSGPtrs.Mode = PORT_TO_SGLIST;

	/*�������������������������Ŀ
	� ATA Xfer from Device	    �
	���������������������������*/

	if (npA->ISMFlags & ACBIF_ATA_OPERATION) {
	  cXferBytes	 = npCmdIO->cXferBytesRemain;
	  npA->ISMFlags &= ~ACBIF_WAITSTATE;
	  npA->ISMState  = ACBIS_COMPLETE_STATE;

	} else {
	  /*�������������������������Ŀ
	  � ATAPI Xfer from Device    �
	  ���������������������������*/
	  BCNTH = inp (BCNTHREG);
	  BCNTL = inp (BCNTLREG);
	  cXferBytes = *(NPUSHORT)&BCNTL;
	}
	cXferBytes = (cXferBytes + 1) & ~1;

	/*����������������������������������������������������������������Ŀ
	� If overrun, determine amount of extra bytes to transfer, and set �
	� IOSGPtrs count to get only the number of bytes we are expecting  �
	�������������������������������������������������������������������� */
	if (cXferBytes > npCmdIO->cXferBytesRemain) {
	  cExtraBytes = cXferBytes - npCmdIO->cXferBytesRemain;
	  cXferBytes  = npCmdIO->cXferBytesRemain;
	}

	npCmdIO->IOSGPtrs.numTotalBytes = cXferBytes;

	// Read more bytes into the client's buffer only if
	// there are more bytes available.  If the device has
	// some more bytes then read them into the bit bucket
	// in the next paragraph.
	if (cXferBytes) {
	  /*������������������������������������������������������Ŀ
	  � Start the interrupt timer if this is an ATAPI transfer �
	  ��������������������������������������������������������*/
	  if (!(npA->ISMFlags & ACBIF_ATA_OPERATION)) {

	    npA->ISMFlags |= ACBIF_WAITSTATE;

	    ADD_StartTimerMS (&npA->IRQTimeOutHandle, npA->IRQTimeOut,
			      (PFN)IRQTimeOutHandler, npA);
	  }

	  /*���������������������������������������Ŀ
	  � Device will prepare for next interrupt  �
	  � after last byte is transfered to host   �
	  �����������������������������������������*/

  TR(0x50)

	  if ((npA->ISMFlags & ACBIF_PIO32) && !(cXferBytes & 3))
	    npCmdIO->IOSGPtrs.Mode |= 0x80;
	  ADD_XferIO (&npCmdIO->IOSGPtrs);

  TR(0x51)

	  if (npA->ISMFlags & ACBIF_ATA_OPERATION) {
	    BSYWAIT (npA);
	    INTRSN = inp (INTRSNREG);
	  }
	}
	/* ������������������������������������������������Ŀ
	� if extra bytes remain, put them in the byte bucket�
	���������������������������������������������������*/
	while (cExtraBytes >= 2) {

  TR(0x52)

	  inpw (npCmdIO->IOSGPtrs.iPortAddress);
	  cExtraBytes-=2;
	}

	/* ����������������������������������Ŀ
	� Adjust counts to show last transfer �
	�������������������������������������*/
	npCmdIO->cXferBytesComplete += cXferBytes;
	npCmdIO->cXferBytesRemain   -= cXferBytes;

	if (STATUS & FX_ERROR) goto Complete;
	break;

      case IR_XFER_TO_DEVICE:

	npCmdIO->IOSGPtrs.Mode = SGLIST_TO_PORT;

	BCNTH = inp (BCNTHREG);
	BCNTL = inp (BCNTLREG);
	cXferBytes = *(NPUSHORT)&BCNTL;

	/*����������������������������������������������������������������Ŀ
	� If UNDERRUN, set IOSGPtrs count to send only the number of bytes �
	� the device is expecting.  Set up cExtraBytes to fill remaining   �
	� bytes with 0's                                                   �
	������������������������������������������������������������������*/
	if (cXferBytes > npCmdIO->cXferBytesRemain) {
	  cExtraBytes = cXferBytes - npCmdIO->cXferBytesRemain;
	  cXferBytes = npCmdIO->cXferBytesRemain;
	}

	npCmdIO->IOSGPtrs.numTotalBytes = cXferBytes;

	// Write more bytes from the client's buffer only if
	// there are more bytes to write.  If the device wants
	// some more bytes then write some zeros in he next
	// paragraph.
	if (cXferBytes) {
	  /* ������������������������Ŀ
	  � Start the interrupt timer �
	  ���������������������������*/

	  npA->ISMFlags |= ACBIF_WAITSTATE;

	  ADD_StartTimerMS (&npA->IRQTimeOutHandle, npA->IRQTimeOut,
			    (PFN)IRQTimeOutHandler, npA);

	  /*����������������������������������������Ŀ
	  � Device will prepare for next interrupt   �
	  � after last byte is transfered from host  �
	  ������������������������������������������*/

	  if ((npA->ISMFlags & ACBIF_PIO32) && !(cXferBytes & 3))
	    npCmdIO->IOSGPtrs.Mode |= 0x80;
	  ADD_XferIO (&npCmdIO->IOSGPtrs);
	}

	/* ������������������������������������������������Ŀ
	� if device expected more, give it zeros	    �
	���������������������������������������������������*/
	while (cExtraBytes) {
	  outpw (npCmdIO->IOSGPtrs.iPortAddress, 0);
	  cExtraBytes-=2;
	}

	npCmdIO->cXferBytesComplete += cXferBytes;
	npCmdIO->cXferBytesRemain   -= cXferBytes;

	if (STATUS & FX_ERROR) goto Complete;
	break;

      case IR_MESSAGE: // not used
      case IR_COMPLETE:
      Complete:
	if (npA->ISMFlags & ACBIF_BUSMASTERDMAIO)
	  PerfBMComplete (npA, (UCHAR)(STATUS & FX_ERROR));

	/*�����������������������������������Ŀ
	� if an error, get the error register �
	�������������������������������������*/
	if (STATUS & FX_ERROR) {
	  ERROR = inp (ERRORREG);

	  if (npA->OSMFlags & ACBOF_SENSE_DATA_ACTIVE) {
	    npA->IORBError = SenseKeyMap[ERROR >> 4];
	  } else if (!(npA->ISMFlags & ACBIF_ATA_OPERATION)) {
	  /*����������������������������������Ŀ
	  �	      Request Sense	       �
	  ������������������������������������*/
	    npA->OSMReqFlags |= ACBR_SENSE_DATA;
	  }
	}
	npA->ISMState  = ACBIS_COMPLETE_STATE;
	break;

      default :

	//BMK if all is 0 then there was an unknown problem
	STATUS = FX_ERROR;
	//Do a reset
	npA->ISMFlags	 &= ~ACBIF_WAITSTATE;
	npA->OSMReqFlags |= ACBR_RESET;
	npA->ISMState	  = ACBIS_COMPLETE_STATE;
	break;

    }
  } else {

  TR(13)

    /*����������������������Ŀ
    � An Interrupt Timed Out �
    ������������������������*/

    if (npA->ISMFlags & ACBIF_BUSMASTERDMAIO)
      PerfBMComplete (npA, 1);

    npA->ISMState     = ACBIS_COMPLETE_STATE;
    npA->ISMFlags    &= ~ACBIF_WAITSTATE;
    npA->OSMReqFlags |= ACBR_RESET;

    // Fake interrupt timeout to get an error
    npA->TimerFlags |= ACBT_INTERRUPT;
  }

  TR(0x54)
}


/*�������������������������������������ͻ
�					�
�  WriteATAPIPkt			�
�					�
�  Write ATAPI command packet to data	�
�  register				�
�					�
���������������������������������������*/
VOID NEAR WriteATAPIPkt (NPA npA)
{
  USHORT Port;
  NPCMDIO npCmdIO = npA->npCmdIO;

  TR(14)

  if (!BSY_CLR_DRQ_SET_WAIT (npA)) { /* Max Poll wait time exceeded */

  TR(15)

    npA->ISMState     = ACBIS_COMPLETE_STATE;
    npA->OSMReqFlags |= ACBR_RESET;  /* Set OSM for reset */
  } else {

  TR(16)

    /* ������������������������Ŀ
    � Start the interrupt timer �
    ���������������������������*/

    npA->ISMFlags |= ACBIF_WAITSTATE;
    npA->ISMState  = ACBIS_INTERRUPT_STATE;

    ADD_StartTimerMS (&npA->IRQTimeOutHandle, npA->IRQTimeOut,
		      (PFN)IRQTimeOutHandler, npA);

    /*------------------------------------------------------------*/
    /* Start Bus Master DMA transfer after issuing Packet command */
    /*------------------------------------------------------------*/

    if (npA->BM_CommandCode & BMICOM_START) {
      outp (npA->BMISTA, npA->BMStatus | BMISTA_INTERRUPT | BMISTA_ERROR);
////	  if (npA->HardwareType <= Via)
//	if (npA->HardwareType < Via)
//	  outp (npA->BMICOM, npA->BM_CommandCode); /* Start BM controller */
    }

    /* �����������������������������������Ŀ
    �	   Write out the ATAPI Packet	   �
    ��������������������������������������*/
    Port = npA->BasePort;
    {
      USHORT CmdLen = npA->npU->CmdPacketLength;
      NPCH Cmd = npCmdIO->ATAPIPkt;

      _asm {
	cld
	mov cx, CmdLen
	mov dx, Port
	mov si, Cmd
      }
      if (!(npA->ISMFlags & ACBIF_PIO32)) {
	_asm {
	  shr cx, 1
	  rep outsw
	}
      } else {
	_asm {
	  shr cx, 2
	  _emit 0x66
	  rep outsw
	}
      }
    }

////	if ((npA->BM_CommandCode & BMICOM_START) && (npA->HardwareType > Via)) {
//    if ((npA->BM_CommandCode & BMICOM_START) && (npA->HardwareType >= Via)) {
    if (npA->BM_CommandCode & BMICOM_START) {

      DRQ_CLEAR_WAIT (npA);
      outp (npA->BMICOM, npA->BM_CommandCode);	   /* Start BM controller */
    }
  }
}

/*
������������������������������������ͻ
�				     �
�  IRQTimeOutHandler		     �
�				     �
�  Times out the interrupt	     �
�				     �
������������������������������������ͼ */
VOID FAR _cdecl IRQTimeOutHandler (USHORT TimerHandle, PA pA)
{
  NPA npA = (NPA)pA;

#if 0
  TimerHandle = saveXCHG (&(npA->IRQTimeOutHandle), 0);
  ADD_CancelTimer (TimerHandle) ;
#else
  ADD_CancelTimer (TimerHandle) ;
  saveXCHG (&(npA->IRQTimeOutHandle), 0);
#endif

  npA->TimerFlags |= ACBT_INTERRUPT;   /* Tell intrt state we timedout */

  if (npA->ISMState == ACBIS_INTERRUPT_STATE)
    StartISM (npA) ;
}

USHORT FAR _loadds AdapterIRQ0() { return (AdptInterrupt (ACBPtrs[0].npA)); }

USHORT FAR _loadds AdapterIRQ1() { return (AdptInterrupt (ACBPtrs[1].npA)); }

USHORT FAR _loadds AdapterIRQ2() { return (AdptInterrupt (ACBPtrs[2].npA)); }

USHORT FAR _loadds AdapterIRQ3() { return (AdptInterrupt (ACBPtrs[3].npA)); }

USHORT FAR _loadds AdapterIRQ4() { return (AdptInterrupt (ACBPtrs[4].npA)); }

USHORT FAR _loadds AdapterIRQ5() { return (AdptInterrupt (ACBPtrs[5].npA)); }

USHORT FAR _loadds AdapterIRQ6() { return (AdptInterrupt (ACBPtrs[6].npA)); }

USHORT FAR _loadds AdapterIRQ7() { return (AdptInterrupt (ACBPtrs[7].npA)); }

/*
��������������������������������������ͻ
�				       �
�  AdptInterrupt		       �
�				       �
�  Determines if IRQ should be claimed �
�  and sets state flags accordingly    �
�				       �
��������������������������������������ͼ */
USHORT NEAR AdptInterrupt (NPA npA)
{
  USHORT TimerHandle;

  TimerHandle = saveXCHG (&(npA->IRQTimeOutHandle), 0);
  DevHelp_EOI (npA->IRQLevel);

  if (TimerHandle) {
    ADD_CancelTimer (TimerHandle) ;
    StartISM (npA);
  }
  return (0);
}

/*--------------------------------------------*/
/* CreateBMSGList			      */
/* --------------			      */
/*					      */
/* Arguments:				      */
/*					      */
/*					      */
/* Actions:				      */
/*	Takes OS/2 scatter/gather list and    */
/*	builds SFF-8038i compatible list for  */
/*	DMA controller. 		      */
/*					      */
/*					      */
/* Returns:				      */
/*	0 if successful 		      */
/*					      */
/*					      */
/*--------------------------------------------*/
BOOL NEAR CreateBMSGList (NPA npA)
{
  NPCMDIO npCmdIO = npA->npCmdIO;

  if (BuildSGList (npA->pSGL, &(npCmdIO->IOSGPtrs), npCmdIO->cXferBytesRemain, npA->SGAlign)) {
    return (0); 			 /* finished building sg list */
  } else {
//    ++npA->npU->DeviceCounters.ByteMisalignedBuffers;
    return (1); 		    /* fail conversion */
  }
}



/*-------------------------*/
/*			   */
/*			   */
/*			   */
/*-------------------------*/
VOID NEAR PerfBMComplete (NPA npA, UCHAR immediate)
{
  USHORT  Port;
  UCHAR   loop, Data;
  NPCMDIO npCmdIO = npA->npCmdIO;

  npA->ISMFlags &= ~ACBIF_BUSMASTERDMAIO;
  npCmdIO->cXferBytesComplete = npCmdIO->cXferBytesRemain;
  npCmdIO->cXferBytesRemain = 0;

  outp (npA->BMICOM, npA->BM_CommandCode & npA->BM_StopMask); /* shut down BM DMA controller */

  /* Wait for BM DMA active to go away for a while. */

  Port = npA->BMISTA;
  Data = inp (Port);

  if (!immediate) {
    for (loop = 255 ; loop ; --loop) {
      if (!(Data & BMISTA_ACTIVE))
	break;
      Data = inp (Port);
      IODelay();
    }

    if (Data & BMISTA_ACTIVE) {
       npA->ISMState	 = ACBIS_COMPLETE_STATE;
       npA->ISMFlags	&= ~ACBIF_WAITSTATE;
       npA->OSMReqFlags |= ACBR_RESET;
    }
  }

  /* Fix so that Bus Master errors will definitely result in retries */

  if (Data & BMISTA_ERROR) {
    npA->ISMState     = ACBIS_COMPLETE_STATE;
    npA->ISMFlags    &= ~ACBIF_WAITSTATE;
    npA->OSMReqFlags |= ACBR_RESET;
  }
}

