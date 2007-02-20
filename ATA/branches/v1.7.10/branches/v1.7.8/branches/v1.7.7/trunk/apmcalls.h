/**************************************************************************
 *
 * SOURCE FILE NAME = APMCALLS.H
 *
 * DESCRIPTIVE NAME = DANIS506.ADD - Adapter Driver for PATA/SATA DASD
 *
 * Copyright : COPYRIGHT Daniela Engert 1999-2006
 *
 * DESCRIPTION : APM definitions
 ****************************************************************************/

/*
** APM IDC Routines
 */
typedef struct _APMEVENT {
  USHORT  Function;
  ULONG   ulParm1;
  ULONG   ulParm2;
} APMEVENT, FAR *PAPMEVENT;

USHORT FAR _cdecl APMAttach (void);

typedef USHORT _cdecl APMHANDLER (PAPMEVENT);
typedef APMHANDLER FAR *PAPMHANDLER;

USHORT FAR _cdecl APMRegister (PAPMHANDLER Handler,
			       ULONG NotifyMask,
			       USHORT DeviceID);

USHORT FAR _cdecl APMDeregister (void);

#define APM_SETPWRSTATE 	0x6
#define APM_NORMRESUMEEVENT	0x8
#define APM_CRITRESUMEEVENT	0x9
#define APM_STBYRESUMEEVENT	0x9

#define APM_NOTIFYSETPWR	(1<<APM_SETPWRSTATE)
#define APM_NOTIFYNORMRESUME	(1<<APM_NORMRESUMEEVENT)
#define APM_NOTIFYCRITRESUME	(1<<APM_CRITRESUMEEVENT)
#define APM_NOTIFYSTBYRESUME	(1<<APM_STBYRESUMEEVENT)

#define APM_PWRSTATEREADY	0x0
#define APM_PWRSTATESTANDBY	0x1
#define APM_PWRSTATESUSPEND	0x2
#define APM_PWRSTATEOFF 	0x3

