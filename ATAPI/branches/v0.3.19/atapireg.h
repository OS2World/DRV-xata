/**************************************************************************
 *
 * SOURCE FILE NAME =	 ATAPIreg.h
 *
 * DESCRIPTIVE NAME = DaniATAPI.ADD - Filter Driver for ATAPI
 *
 * Copyright : COPYRIGHT IBM CORPORATION, 1993, 1994
 *	       COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : Register equates for ATAPI devices.
 *
 ****************************************************************************/

/* drives */

#define UNIT0  0x00
#define UNIT1  0x10

/* Status bits */

#define FX_BUSY      0x80	 /* Status port busy bit		    */
#define FX_READY     0x40	 /* Status port ready bit		    */
#define FX_WRTFLT    0x20	 /* Write Fault 			    */
#define FX_DSC	     0x10	 /* Seek complete			    */
#define FX_DRQ	     0x08	 /* Data Request bit			    */
#define FX_ERROR     0x01	 /* Error status			    */


/* I/O ports for AT hard drives */

#define FX_PRIMARY   0x01F0	   /* Default primary	ports		      */
#define FX_SECONDARY 0x0170	   /* Default secondary ports		      */

/* Default address for the io ports
ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
³ SIZE	 NAME	    ADDRESS   Description				       ³
³ ÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÅÄÄÄÄÄÄÄÄÄÄÄÄ				       ³
³ WORD	 DATA	    0x01F0    data register				       ³
³ BYTE	 FEATURE    0x01F1    feature register				       ³
³ BYTE	 ERROR	    0x01F1    error register				       ³
³ BYTE	 INTREASON  0x01F2    interrupt reason register 		       ³
³ BYTE	 SAMTAG     0x01F3    reserved for SAM TAG Byte 		       ³
³ BYTE	 BYTECTL    0x01F4    byte Count (Low order) register		       ³
³ BYTE	 BYTECTH    0x01F5    byte Count (high order) register		       ³
³ BYTE	 DRVSLCT    0x01F6    drive select register			       ³
³ BYTE	 CMD	    0x01F7    command register				       ³
³ BYTE	 STATUS     0x01F7    status register				       ³
³ BYTE	 DEVCON     0x03F6    device control register			       ³
ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
*/

/* Array indices (IOPorts[] and IORegs[]) for I/O port address and contents   */

/* Output Regs */

#define FI_PDAT       0 		  /* data register		      */
#define FI_PFEAT      1 		  /* feature register		      */
#define FI_PINTRSN    2 		  /* interrupt reason register	      */
#define FI_PSECCNT    2 		  /* ATA sector count register	      */
#define FI_PSAMTAG    3 		  /* reserved for SAM TAG Byte	      */
#define FI_PBCNTL     4 		  /* byte Count (Low order) register  */
#define FI_PBCNTH     5 		  /* byte Count (high order) register */
#define FI_PDRHD      6 		  /* drive select register	      */
#define FI_PCMD       7 		  /* command register		      */
#define FI_RFDR       8 		  /* device control register	      */

/* Input Regs */

#define FI_PERR      9			  /* error register		      */
#define FI_PSTAT    10			  /* status register		      */

/* Bit Masks to determine if a register is desired to be written/read	      */

#define FM_PDAT       (1<< FI_PDAT	) /* data register		      */
#define FM_PFEAT      (1<< FI_PFEAT	) /* feature register		      */
#define FM_PERROR     (1<< FI_PERROR	) /* error register		      */
#define FM_PINTRSN    (1<< FI_PINTRSN	)  /* interrupt reason register        */
#define FM_PSAMTAG    (1<< FI_PSAMTAG	) /* reserved for SAM TAG Byte	      */
#define FM_PBCNTL     (1<< FI_PBCNTL	) /* byte Count (Low order) register  */
#define FM_PBCNTH     (1<< FI_PBCNTH	) /* byte Count (high order) register */
#define FM_PDRHD      (1<< FI_PDRHD	) /* drive select register	      */
#define FM_PCMD       (1<< FI_PCMD	) /* command register		      */
#define FM_RFDR       (1<< FI_RFDR	) /* device control register	      */

#define FM_PATAPI_CMD (FM_PFEAT | FM_PBCNTL | FM_PBCNTH | FM_PDRHD | FM_PCMD)
#define FM_PATA_CMD   (FM_PFEAT | FM_PDRHD | FM_PCMD)

#define DEVCTL	    (npA->IORegs[FI_RFDR])
#define DEVCTLREG   (npA->StatusPort)
#define FEAT	    (npA->IORegs[FI_PFEAT])
#define FEATREG     (npA->IOPorts[FI_PFEAT])
#define SECCNT	    (npA->IORegs[FI_PSECCNT])
#define SECCNTREG   (npA->IOPorts[FI_PSECCNT])
#define INTRSN	    (npA->IORegs[FI_PINTRSN])
#define INTRSNREG   (npA->IOPorts[FI_PINTRSN])
#define SECNUM	    (npA->IORegs[FI_PSAMTAG])
#define SAMTAG	    (npA->IORegs[FI_PSAMTAG])
#define SAMTAGREG   (npA->IOPorts[FI_PSAMTAG])
#define CYLL	    (npA->IORegs[FI_PBCNTL])
#define BCNTL	    (npA->IORegs[FI_PBCNTL])
#define BCNTLREG    (npA->IOPorts[FI_PBCNTL])
#define CYLH	    (npA->IORegs[FI_PBCNTH])
#define BCNTH	    (npA->IORegs[FI_PBCNTH])
#define BCNTHREG    (npA->IOPorts[FI_PBCNTH])
#define DRVHD	    (npA->IORegs[FI_PDRHD])
#define DRVHDREG    (npA->IOPorts[FI_PDRHD])
#define COMMAND     (npA->IORegs[FI_PCMD])
#define COMMANDREG  (npA->IOPorts[FI_PCMD])
#define STATUS	    (npA->IORegs[FI_PSTAT])
#define STATUSREG   (npA->IOPorts[FI_PCMD])
#define ERROR	    (npA->IORegs[FI_PERR])
#define ERRORREG    (npA->IOPorts[FI_PFEAT])
#define BMSTATUS    (npA->BMSta)
#define BMSTATUSREG (npA->BMISTA)

/* Default Register Values */

#define DEFAULT_ATAPI_FEATURE_REG     0x00
#define DMA_ON			      0x01
#define DMA_DIR_TO_HOST 	      0x04

#define DEFAULT_ATAPI_DRV_SLCT_REG    0xA0
#define REV_17B_SET_LBA 	      0x40

#define DEFAULT_ATAPI_DEVCON_REG      0x08
#define INTERRUPT_DISABLE	      0x02

/*
ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
³Interrupt Reason		 defined value				       ³
³									       ³
³ IO	DRQ   COD							       ³
³  0	 1     1	   IR_PKTREADY - Device is ready to accept	       ³
³					 Packet Commands		       ³
³  1	 1     1	    IR_MESSAGE - Ready to send message data to host    ³
³  1	 1     0   IR_XFER_FROM_DEVICE - Data is ready to be transfered to Host³
³  0	 1     0     IR_XFER_TO_DEVICE - Device is ready to receive data       ³
³  1	 0     1	   IR_COMPLETE - Status Register contains	       ³
³					 completion status		       ³
³									       ³
ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
*/

/* Interrupt Reason Register */

#define IRR_COD 	      0x01
#define IRR_IO		      0x02


/* Interrupt Reson Register to Reason code masks */

#define IRM_COD 	      IRR_COD
#define IRM_IO		      IRR_IO
#define IRM_DRQ 	      FX_DRQ

/* Interrupt Reason Codes */

#define IR_PKTREADY	      (IRM_DRQ |	  IRM_COD)
#define IR_MESSAGE	      (IRM_DRQ | IRM_IO | IRM_COD)
#define IR_XFER_FROM_DEVICE   (IRM_DRQ | IRM_IO 	 )
#define IR_XFER_TO_DEVICE     (IRM_DRQ			 )
#define IR_COMPLETE	      ( 	 IRM_IO | IRM_COD)

#define SENSEKEY_MASK	      0XFF00

/* Interrupt control */

#define IRQ_FIXED_PRIMARY  0x000E  /* Fixed int errupt IRQ #		     */

/* ATA commands */

#define FX_CKPWRMODE   0x00E5	     /* Check Power Mode		      */
#define FX_EXDRVDIAG   0x0090	     /* Execute Drive Diagnostics	      */
#define FX_IDLEIMM     0x00E1	     /* Idle Immediate			      */
#define FX_NOP	       0x0000	     /* NOP				      */
#define FX_SEEK        0x0070	     /* Seek				      */
#define FX_SETFTURE    0x00EF	     /* Set Features			      */
#define FX_SLEEP       0x00E6	     /* Sleep				      */
#define FX_STDBYIMM    0x00E0	     /* Stand By Immediate		      */

/* ATA Atapi Commands	  OP Code  */
#define FX_SOFTRESET	  0x0008   /* ATAPI Soft Reset			    */
#define FX_PKTCMD	  0x00A0   /* ATAPI Packet Command		    */
#define FX_IDENTIFYDRIVE  0x00A1   /* ATAPI Identify Drive		    */
#define FX_GETMEDIASTAT   0x00DA   /* RPL */
#define FX_STANDBY	  0x00E2   /* RPL */

/* Atapi Packet Commands  OP Code  */
#define FXP_TESTUNIT_READY 0x0000   /* ATAPI Test Unit Ready		     */
#define FXP_REZERO_UNIT    0x0001   /* ATAPI ReZero Unit		     */
#define FXP_REQUEST_SENSE  0x0003   /* ATAPI ReQuest Sense		     */
#define FXP_NEW_FORMATUNIT 0x0004   /* ATAPI New FOrmat Unit		     */
#define FXP_REASSIGNBLOCKS 0x0007   /* ATAPI ReAssign Blocks		     */
#define FXP_INQUIRY	   0x0012   /* ATAPI Inquiry			     */
#define FXP_START_STOPUNIT 0x001B   /* ATAPI Start-Stop Unit		     */
#define FXP_PRV_ALWMEDRMVL 0x001E   /* ATAPI Prevent ALlow Medium Removal    */
#define FXP_READFORMAT_CAP 0x0023   /* ATAPI Read Format Capacity	     */
#define FXP_FORMAT_UNIT    0x0024   /* ATAPI Format Unit (old)		     */
#define FXP_READ_CAPACITY  0x0025   /* ATAPI Read Capacity		     */
#define FXP_READ_10	   0x0028   /* ATAPI Read 10			     */
#define FXP_WRITE_10	   0x002A   /* ATAPI Write 10			     */
#define FXP_SEEK	   0x002B   /* ATAPI Seek			     */
#define FXP_SEND_DIAG	   0x002D   /* ATAPI Send Diagnostics		     */
#define FXP_WRITE_VERIFY   0x002E   /* ATAPI Write and Verify		     */
#define FXP_VERIFY	   0x002F   /* ATAPI Verify			     */
#define FXP_READ_DEFECT    0x0037   /* ATAPI Read defect Data		     */
#define FXP_WRITE_BUFFER   0x003B   /* ATAPI Write Buffer		     */
#define FXP_READ_BUFFER    0x003C   /* ATAPI Read Buffer		     */
#define FXP_READ_LONG	   0x003E   /* ATAPI Read Long			     */
#define FXP_WRITE_LONG	   0x003F   /* ATAPI Write Long 		     */
#define FXP_MODE_SELECT    0x0055   /* ATAPI Mode Select		     */
#define FXP_MODE_SENSE	   0x005A   /* ATAPI Mode Sense 		     */
#define FXP_READ_12	   0x00A8   /* ATAPI Read 12			     */
#define FXP_WRITE_12	   0x00AA   /* ATAPI Write 12			     */

#define FXP_READ_REVERSE   0x000F
#define FXP_SYNC_CACHE	   0x0035
#define FXP_WRITE_SAME	   0x0041
#define FXP_CLOSE	   0x005B
#define FXP_SEND_CUE_SHEET 0x005D
#define FXP_BLANK	   0x00A1
#define FXP_VERIFY_12	   0x00AF
#define FXP_WRITE_VERIFY_12 0x00AE
#define FXP_RESERVE_RZN    0x0053

#define FXP_READ_16	   0x0088   /* ATAPI Read 16			     */
#define FXP_WRITE_16	   0x008A   /* ATAPI Write 16			     */
#define FXP_WRITE_SAME_16  0x0093   /* ATAPI Write Same 16		     */
#define FXP_VERIFY_16	   0x008F
#define FXP_WRITE_VERIFY_16 0x008E

/* Feature Commands */
#define FRC_ENABLE_MEDIA_STATUS  0x95	/* Enable Media Status Notification   */
#define FRC_DISABLE_MEDIA_STATUS 0x31	/* Disable			      */
#define FRC_SET_TRANSFER_MODE	 0x03	/* Set Transfer Mode		      */
#define FRC_DISABLE_PWRON_DFLT	 0x66	/* Disable reverting 2 PWRON Defaults */
#define FRC_ENABLE_PWRON_DFLT	 0xCC	/* Enable			      */

/* Error Register */
#define ER_RESERVED	   0x80     /* Reserved */
#define ER_UNC		   0x40     /* Unreceiverable error */
#define ER_MC		   0x20     /* Media Change	    */
#define ER_IDNF 	   0x10     /* ID not found	    */
#define ER_MCR		   0x08     /* Media Change Request */
#define ER_ABRT 	   0x04     /* Error Cmd Aborted    */
#define ER_TK0NF	   0x02     /* Track 0 Not Found    */
#define ER_AMNF 	   0x01     /* Address Mark Not Found */


/* Spec Revision 1.7 B */
#define REV_17B_ATAPI_AUDIO_SCAN       0xD8
#define REV_17B_ATAPI_SET_CDROM_SPEED  0xDA
#define REV_17B_ATAPI_READ_CD	       0xD4
#define REV_17B_ATAPI_READ_CD_MSF      0xD5

/* Mode Sense Page Codes  Capabilities Page */
#define REV_17B_PAGE_CAPABILITIES      0x0F

/* Page code Mask for mode sense */
#define REV_17B_PAGE_CODE_MASK	       0x38

/* REV 1.7B Capabilities bit field definitions */

#define REV_17B_CP_MODE2_FORM1		       0x00000002
#define REV_17B_CP_MODE2_FORM2		       0x00000004
#define REV_17B_CP_READ_CDDA		       0x00000008
#define REV_17B_CP_PHOTO_CD		       0x00000020
#define REV_17B_CP_EJECT		       0x00000040
#define REV_17B_CP_LOCK 		       0x00000080
#define REV_17B_CP_UPC			       0x00000200
#define REV_17B_CP_ISRC 		       0x00000400
#define REV_17B_CP_INDEPENDENT_VOL_LEV	       0x00000800
#define REV_17B_CP_SEPARATE_CH_MUTE	       0x00001000
#define REV_17B_CP_TRAY_LOADER		       0x00002000
#define REV_17B_CP_POPUP_LOADER 	       0x00004000
#define REV_17B_CP_RESERVED_LOADER	       0x00008000
#define REV_17B_CP_PREVENT_JUMPER	       0x00010000
#define REV_17B_CP_LOCK_STATE		       0x00020000
#define REV_17B_CP_CDDA_ACCURATE	       0x00040000
#define REV_17B_CP_C2_POINTERS		       0x00080000
