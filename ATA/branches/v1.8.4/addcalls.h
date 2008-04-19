/**************************************************************************
 *
 * SOURCE FILE NAME =  ADDCALLS.H
 *
 * DESCRIPTIVE NAME =  C Function Prototypes/Structures for services
 *		       contained in ADDCALLS.LIB
 *		       This file also contains IN/OUT Port C-Macros.
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2006
 * Copyright : COPYRIGHT IBM CORPORATION, 1991, 1992
 *	       LICENSED MATERIAL - PROGRAM PROPERTY OF IBM
 *	       REFER TO COPYRIGHT INSTRUCTION FORM#G120-2083
 *	       RESTRICTED MATERIALS OF IBM
 *	       IBM CONFIDENTIAL
 *
 * VERSION = V2.0
 *
 * DATE
 *
 * DESCRIPTION :
 *
 * Purpose:   ADDCALLS.LIB provides the following services:
 *
 *		   - C Callable Timer Manager
 *		   - Scatter/Gather List Emulation
 *		   - RBA to CHS conversion
 *		   - Diskette DMA channel setup
 *
 *
 * FUNCTIONS  :
 *
 *
 *
 * NOTES
 *
 *
 * STRUCTURES
 *
 * EXTERNAL REFERENCES
 *
 *
 *
 * EXTERNAL FUNCTIONS
 *
 * CHANGE ACTIVITY =
 *   DATE      FLAG	   APAR   CHANGE DESCRIPTION
 *   --------  ----------  -----  --------------------------------------
 *   mm/dd/yy  @VR.MPPPXX  XXXXX  XXXXXXX
 *
 ****************************************************************************/

/*
** Xfer Buffer Data Structure
*/

typedef struct _ADD_XFER_DATA	{	      /* ADDX */

  USHORT	Mode;			      /* Direction of xferdata	    */
  USHORT	cSGList;		      /* Count of S/G list elements */
  PSCATGATENTRY pSGList;		      /* Far pointer to S/G List    */
  PBYTE 	pBuffer;		      /* Far pointer to buffer	    */
  ULONG 	numTotalBytes;		      /* Total bytes to copy	    */
  USHORT	iSGList;		      /* Current index of S/G List  */
  ULONG 	SGOffset;		      /* Current offset 	    */
} ADD_XFER_DATA, FAR *PADD_XFER_DATA;

#define SGLIST_TO_BUFFER     1		      /* From S/G list to buffer    */
#define BUFFER_TO_SGLIST     2		      /* From buffer to S/G list    */

/*
** Xfer I/O Data Structure
*/

typedef struct _ADD_XFER_IO   { 	      /* ADDIO */

  USHORT	Mode;			      /* Direction of xferdata	    */
  USHORT	cSGList;		      /* Count of S/G list elements */
  PSCATGATENTRY pSGList;		      /* Far pointer to S/G List    */
  ULONG 	iPortAddress;		      /* I/O Port/Mem Address	    */
  ULONG 	numTotalBytes;		      /* Total bytes to copy	    */
  USHORT	iSGList;		      /* Current index of S/G List  */
  ULONG 	SGOffset;		      /* Current offset 	    */
  USHORT	iSGListStart;		      /* Save area - S/G List Index */
  ULONG 	SGOffsetStart;		      /* Save area - S/G List Offset*/
} ADD_XFER_IO, FAR *PADD_XFER_IO, NEAR *NPADD_XFER_IO;

typedef struct _ADD_XFER_IO_X { 	      /* ADDIO */

  USHORT	Mode;			      /* Direction of xferdata	    */
  USHORT	cSGList;		      /* Count of S/G list elements */
  PSCATGATENTRY pSGList;		      /* Far pointer to S/G List    */
  ULONG 	iPortAddress;		      /* I/O Port/Mem Address	    */
  ULONG 	numTotalBytes;		      /* Total bytes to copy	    */
  USHORT	iSGList;		      /* Current index of S/G List  */
  ULONG 	SGOffset;		      /* Current offset 	    */
  USHORT	iSGListStart;		      /* Save area - S/G List Index */
  ULONG 	SGOffsetStart;		      /* Save area - S/G List Offset*/
  SEL		Selector;		      /* temp selector for PIO	    */
} ADD_XFER_IO_X, FAR *PADD_XFER_IO_X, NEAR *NPADD_XFER_IO_X;


#define SGLIST_TO_PORT	     1		      /* From S/G list to I/O Port  */
#define PORT_TO_SGLIST	     2		      /* From I/O Port to S/G list  */
#define PORT_32BIT	  0x80
#define PORT_slow	  0x40
#define PORT_MMIO	  0x04

/*
** Timer Data Structure
*/

typedef struct _ADD_TIMER_DATA	 {	      /* ADDT */

  ULONG        Interval;		      /* Interval value in millisecond */
  ULONG        BackupInterval;		      /* Interval value for backup     */
  PFN	       NotifyEntry;		      /* Notify address 	       */
  PVOID        Parm_1;			      /* parameter which ADD wants     */
} ADD_TIMER_DATA;


/*
** Timer Pool Structure
*/

typedef struct _ADD_TIMER_POOL	 {	      /* ADDT */

  USHORT	 MTick; 		      /* Milliseconds per timer tick   */
  ADD_TIMER_DATA TimerData[1];		      /* Interval value for backup     */
} ADD_TIMER_POOL;

/*
** If the caller wants "n" timer elements, the size of data pool is
**
**    sizeof(ADD_TIMER_POOL) + (n-1)*(ADD_TIMER_DATA).
**
*/

#ifdef IOMACROS

/*
** I/O Instruction macro
*/
					      /* OUT			    */
#define outp(port, data) _asm{ \
      _asm    push ax	       \
      _asm    push dx	       \
      _asm    mov  ax,data     \
      _asm    mov  dx,port     \
      _asm    out  dx,al       \
      _asm    pop  dx	       \
      _asm    pop  ax	       \
}
					     /* IN			   */
#define inp(port, data) _asm{  \
      _asm    push ax	       \
      _asm    push dx	       \
      _asm    xor  ax,ax       \
      _asm    mov  dx,port     \
      _asm    in   al,dx       \
      _asm    mov  data,ax     \
      _asm    pop  dx	       \
      _asm    pop  ax	       \
}
					     /* OUTW			   */
#define outwp(port, data) _asm{ \
      _asm    push ax	       \
      _asm    push dx	       \
      _asm    mov  ax,data     \
      _asm    mov  dx,port     \
      _asm    out  dx,ax       \
      _asm    pop  dx	       \
      _asm    pop  ax	       \
}
					     /* INW			   */
#define inwp(port, data) _asm{ \
      _asm    push ax	       \
      _asm    push dx	       \
      _asm    xor  ax,ax       \
      _asm    mov  dx,port     \
      _asm    in   ax,dx       \
      _asm    mov  data,ax     \
      _asm    pop  dx	       \
      _asm    pop  ax	       \
}
					     /* OUTSW			    */
#define outswp(port, pdata, len) _asm{	\
	  _asm	 push ds	 \
	  _asm	 push si	 \
	  _asm	 push dx	 \
	  _asm	 push cx	 \
	  _asm	 cld		 \
	  _asm	 mov  cx,len	 \
	  _asm	 mov  dx,port	 \
	  _asm	 lds  si,pdata	 \
	  _asm	 rep  outsw	 \
	  _asm	 pop  cx	 \
	  _asm	 pop  dx	 \
	  _asm	 pop  si	 \
	  _asm	 pop  ds	 \
}
					     /* INSW			    */
#define inswp(port, pdata, len) _asm{ \
	  _asm	 push es	 \
	  _asm	 push di	 \
	  _asm	 push dx	 \
	  _asm	 push cx	 \
	  _asm	 cld		 \
	  _asm	 mov  cx,len	 \
	  _asm	 mov  dx,port	 \
	  _asm	 les  di,pdata	 \
	  _asm	 rep  insw	 \
	  _asm	 pop  cx	 \
	  _asm	 pop  dx	 \
	  _asm	 pop  di	 \
	  _asm	 pop  es	 \
}
#endif

/*
** ADD Common Services
*/


BOOL APIENTRY f_ADD_XferBuffData(PADD_XFER_DATA);
BOOL APIENTRY f_ADD_DMASetup(USHORT, USHORT, USHORT, ULONG);
BOOL APIENTRY f_ADD_ConvRBAtoCHS(ULONG, VOID NEAR *, PCHS_ADDR);

BOOL APIENTRY f_ADD_InitTimer(PBYTE, USHORT);
BOOL APIENTRY f_ADD_DeInstallTimer(VOID);
BOOL APIENTRY f_ADD_StartTimerMS(PUSHORT, ULONG, PFN, PVOID);
BOOL APIENTRY f_ADD_CancelTimer(USHORT);


BOOL PASCAL NEAR ADD_XferBuffData(PADD_XFER_DATA);
BOOL PASCAL NEAR ADD_DMASetup(USHORT, USHORT, USHORT, ULONG);
BOOL PASCAL NEAR ADD_ConvRBAtoCHS(ULONG, VOID NEAR *, PCHS_ADDR);

BOOL PASCAL NEAR ADD_InitTimer(PBYTE, USHORT);
BOOL PASCAL NEAR ADD_DeInstallTimer(VOID);
BOOL PASCAL NEAR ADD_StartTimerMS(PUSHORT, ULONG, PFN, PVOID);
BOOL PASCAL NEAR ADD_CancelTimer(USHORT);

BOOL PASCAL NEAR ADD_XferIOW( NPADD_XFER_IO npADDX );
BOOL PASCAL NEAR ADD_XferIOD( NPADD_XFER_IO npADDX );
BOOL PASCAL NEAR ADD_XferIO ( NPADD_XFER_IO_X npADDX );


/*
** ADD Common Services R / C
*/

#define ADD_SUCCESS   0
#define ADD_ERROR     1


