/* $Id: 1$ */
/** @ipdc.h
  *
  *  Inter PSD Device communication module
  *
  * netlabs.org confidential
  *
  * Copyright (c) 2005 netlabs.org
  *
  * Author: Pavel Shtemenko <pasha@paco.odessa.ua>
  *
  * All Rights Reserved
  *
  */
#ifndef __IPDC_H__
#define __IPDC_H__

#include <bsekee.h>

#pragma pack(1)
//  Special function
#define EXEC_FUN_AT_ALL_CPU         0x7f          // Execute function @ all CPU !!!! exec in interrupt time !!!!
#define READ_WRITE_EMBEDDED         0x80          // Direct access to embedded controller
//  Public & export function
#define EVALUATE_FUNCTION           0x81
#define GETSYSTEMIFO_FUNCTION       0x82
#define GETTABLEHEADER_FUNCTION     0x83
#define GETTABLE_FUNCTION           0x84
#define WALKNAMESPACE_FUNCTION      0x85
#define GETNAME_FUNCTION            0x86
#define GETOBJECTINFO_FUNCTION      0x87
#define RSGETMETHODDATA_FUNCTION    0x88
#define GETTYPE_FUNCTION            0x89
#define GETPARENT_FUNCTION          0x8A
#define GETNEXTOBJECT_FUNCTION      0x8B
#define WAIT_BUTTON                 0x8C
#define GET_DRIVER_VERSION          0x8D
#define ACPI_ENTER_SLEEP            0x8E
#define GETTIMER_FUNCTION           0x8F
#define GETTIMERRESOLUTION_FUNCTION 0x90
#define PMTIMER_UPTIME_FUNCTION     0x91
#define SET_THROTTLING_C_STATE      0x92
#define GET_THROTTLING              0x93
#define GET_STATISTICS              0x94
#define SET_CPU_POWER_STATE         0x95
#define ENTER_EVENT_THREAD_FUNCTION 0x96
#define WAIT_NOTIFY_FUNCTION        0x97
#define WAIT_EMBEDDED_FUNCTION      0x98
#define FIND_PCI_DEVICE             0x99
#define GETHANDLE_FUNCTION          0x9A
#define ACPI_INTERNAL_TEST          0x9B
#define READ_WRITE_PCI              0x9C          // Direct PCI functions
// Set event flag
#define SET_POWERBUTTON_FLAG        0x100
#define SET_SLEEPBUTTON_FLAG        0x101
// Internal function
#define ACPI_INIT_EVENTS            0x200
#define ACPI_START_EVENTS           0x201
#define ACPI_SNOOPER_ENTER          0x202
#define SNOOPER_SET_PCI_INFO        0x203
// Function for drivers only
#define GET_PROCESSOR_DATA          0x210
#define GET_FUNCTION_ADDRESS        0x211
#define SET_IDLE_FUNCTION           0x212
#define REGISTERED_NOTIFY           0x213
#define DEREGISTER_NOTIFY           0x214
#define SET_POWER_FUNCTION          0x215
#define SET_THRTL_FUNCTION          0x216
#define REGISTERED_CPUEXEC          0x217
#define DEREGISTER_CPUEXEC          0x218
//
typedef struct _Get_Handle_
{
    ACPI_HANDLE             Parent;
    ACPI_STRING             Pathname;
    ACPI_HANDLE             *RetHandle;
    ACPI_STATUS             Status;               
} GETHANDLEPar, *PGETHANDLEPar;
// Parameters for Read/Write Embedded
typedef struct _RW_Embedded_
{
    UINT32                  Number;             // Controller numbers
    UINT32                  Function;           // 0 - read, 1 write
    ACPI_PHYSICAL_ADDRESS   Address;
    UINT32                  BitWidth;
    ACPI_INTEGER            *Value;
    ACPI_STATUS             Status;               
} RW_EMBEDDED, *PRW_EMBEDDED;
// Parameters for PCI functions
typedef struct _PCI_Function_
{
    ACPI_PCI_ID             PciId;
    UINT32                  Register;
    void                    *Value;
    UINT32                  Width;
    UINT32                  Function;           // 0 - read, 1 write
    ACPI_STATUS             Status;               
} PCI_FINCTION, *PPCI_FUNCTION;
// Parameters for AppComm export
typedef struct _AppCommPar_
{
    ULONG                   NumberFun;                            // What is the function
    void                    *par;                                 // List of parameters for this funtion
} APPCOMMPAR, *PAPPCOMMPAR;
// For PM timer function
typedef struct _PMwakeup_
{
    UINT64                  When;                                // Count when wakeup
    struct _PMwakeup_       *Next;                               // Next struct for check
} PMTWakeUp, *PPMTWakeUp;

typedef struct _PMTimer_
{
    UINT64                  PMCounter;                           // PMTimer counter
    UINT32                  PMValue;
    PPMTWakeUp              Chain;                               // Chain for wakeup
} PMTimer, *PPMTimer;
// Parameters for IRQ hook
typedef struct _PSD_IRQ_HOOK_
{
    ULONG                   *IRQFlag;                             // Address (in PSD) Flag: 1 - handled, 0 - not handled
    ULONG                   IRQNum;                               // Number of IRQ level, 0 based
    PPMTimer                Work;                                 // Address PMTimer struct
    ULONG                   (*Debug)(UINT8 *,UINT32);             // Function for copy debug buffer to Read operation
    ULONG                   (*Handler)(void);                     // Address for IRQ handler ACPI interrupt
} PSDIRQHOOK, *PPSDIRQHOOK;
// Parameters for AcpiEvaluateObject
typedef struct _EvaPar_
{
    ACPI_HANDLE             Object;
    ACPI_STRING             Pathname;
    ACPI_OBJECT_LIST        *ParameterObjects;
    ACPI_BUFFER             *ReturnObjectBuffer;
    ACPI_STATUS             Status;                           // Where returned status from driver call
} EvaPar, *PEvaPar;
// Parameters for AcpiGetSystemInfo
typedef struct _SGIPar_
{
    ACPI_BUFFER             *RetBuffer;
    ACPI_STATUS             Status;                           // Where returned status from driver call
} SGIPar, *PSGIPar;

typedef struct _GHTPar_                                       // AcpiGetTableHeader
{
    ACPI_STRING             Signature;
    UINT32        Instance;
    ACPI_TABLE_HEADER       *OutTableHeader;
    ACPI_STATUS             Status;                           // Where returned status from driver call
} GTHPar, *PGTHPar;

typedef struct _GTPar_                                        // AcpiGetTableHeader
{
    ACPI_STRING             Signature;
    UINT32        Instance;
    ACPI_TABLE_HEADER       **OutTable;
    ACPI_STATUS             Status;                           // Where returned status from driver call
} GTPar, *PGTPar;

typedef struct _WNSPar_
{
    ACPI_OBJECT_TYPE        Type;
    ACPI_HANDLE             StartObject;
    UINT32                  MaxDepth;
    ACPI_WALK_CALLBACK      UserFunction;
    void                    *Context;
    void                    **ReturnValue;
    ACPI_STATUS             Status;                // Where returned status from driver call
} WNSPar, *PWNSPar;
// AcpiGetName
typedef struct _GNPar_
{
    ACPI_HANDLE             Handle;
    UINT32                  NameType;
    ACPI_BUFFER             *RetPathPtr;
    ACPI_STATUS             Status;                // Where returned status from driver call
} GNPar, *PGNPar;
// AcpiGetObjectInfo 
typedef struct _GOIPar_
{
    ACPI_HANDLE             Handle;
    ACPI_BUFFER             *ReturnBuffer;
    ACPI_STATUS             Status;                // Where returned status from driver call
} GOIPar, *PGOIPar;

typedef struct _RSMDPar_
{
    ACPI_HANDLE             Handle;
    char                    *Path;
    ACPI_BUFFER             *RetBuffer;
    ACPI_STATUS             Status;                // Where returned status from driver call
} RSMDPar, *PRSMDPar;
typedef struct _GTyPar_
{
    ACPI_HANDLE             Object;
    ACPI_OBJECT_TYPE        *OutType;
    ACPI_STATUS             Status;                // Where returned status from driver call
} GTyPar, *PGTyPar;

typedef struct _GTpPar_
{
    ACPI_HANDLE             Object;
    ACPI_HANDLE             *OutType;
    ACPI_STATUS             Status;                // Where returned status from driver call
} GTpPar, *PGTpPar;

typedef struct _GNOPar_
{
    ACPI_OBJECT_TYPE        Type;
    ACPI_HANDLE             Parent;
    ACPI_HANDLE             Child;
    ACPI_HANDLE             *OutHandle;
    ACPI_STATUS             Status;                // Where returned status from driver call
} GNOPar, *PGNOPar;

typedef struct _SetCPUPower_
{
    UINT32                  CPUid;                 // CPU for set
    UINT32                  State;                 // State number for ser
    ACPI_STATUS             Status;                // Where returned status from driver call
} CPUPowerPar, *PCPUPowerPar;

typedef struct _Throttling_
{
    UINT32                  Value;                 // Throtlling value
    UINT8                   C_State;               // Cx state
    UINT8                   ProcId;                // Processor Id from evaluate
    UINT8                   Enable;                // Info, enable throttling or no
    UINT8                   DutyWidth;             // Readonly, value DUTY_WIDTH from FADT
    ACPI_STATUS             Status;                // Where returned status from driver call
} THROTTLING_C_STATE, *PTHROTTLING_C_STATE;

typedef struct _EmbbededPacket_
{
    UINT32                  Function;
    ACPI_PHYSICAL_ADDRESS   Address;
    UINT32                  Data;
    UINT32                  Stage;
    UINT32                  QData;
} EMBEDDED_PACKET, *PEMBEDDED_PACKET;

#define EMBEDDED_PACKET_STAGE_NONE      0x0
#define EMBEDDED_PACKET_STAGE_HDR       0x1
#define EMBEDDED_PACKET_STAGE_ADDR      0x2
#define EMBEDDED_PACKET_STAGE_DATA      0x3
#define EMBEDDED_PACKET_STAGE_END       0x4
#define EMBEDDED_PACKET_STAGE_QUEUE     0x5
#define EMBEDDED_PACKET_STAGE_BURNON    0x6
#define EMBEDDED_PACKET_STAGE_BURNOFF   0x7
#define EMBEDDED_PACKET_STAGE_NOTUSE    0x8

typedef struct _EmmbeddedController_
{
    ACPI_GENERIC_ADDRESS    EcControl;                        // Control register
    ACPI_GENERIC_ADDRESS    EcData;                           // Data register
    UINT32                  Uid;                              //
    UINT8                   GpeBit;                           // Number of GPE
    UINT8                   *EcId;                            // Name in acpi tree
    UINT8                   IBE;                              // Address for event Input Buffer Empty
    UINT8                   OBF;                              // Address for event Output Buffer Full
    UINT32                  Interrupt;                        // 0 w/o, 1 via GPE interrupt
    ACPI_HANDLE             Handle;                           // Handle to EC object
    UINT32                  ECEval;                           // Handle to KermArm
    UINT8                   *EvalMethod;                      // _Qxx to evaluate
    EMBEDDED_PACKET         Packet;                           // 
    UINT32                  NeedQueue;
    volatile int            Use;                              // Lock
    volatile int            QueueUse;                         // Lock
    ACPI_MUTEX              EC_MTX;
} EMMBEDDED_CONTROLLER, *PEMMBEDDED_CONTROLLER;

// Method for working with embedded controller
#define EMMBEDDED_PART                  0x0                   // Partially polling, default
#define EMMBEDDED_INT                   0x1                   // interrupt driven
#define EMMBEDDED_POLL                  0x2                   // full polling

typedef struct _EvaluateName_
{
    ACPI_HANDLE             Handle;                         // Handle to root name
    UINT8                   Number;
    char                    EvalString[5];                  // Name for evaluate
} EvaluateName, *PEvaluateName;

typedef struct _EventNotify_
{
    ACPI_HANDLE             Handle;                         // Device was notification
    UINT16                  Number;                         // Number of notification
    UINT16                  What;                           // Who System/Device
} EventNotify, *PEventNotify;

typedef struct _R3Notify_
{
    UINT32                  Counter;
    UINT32                  CurrentCounter;
    UINT32                  Timeout;
    UINT32                  What;
    EventNotify             Notify[1];
} R3Notify, *PR3Notify;

typedef struct _R3Embedded_
{
    UINT32                  Counter;
    UINT32                  CurrentCounter;
    UINT32                  Timeout;
    EvaluateName            Embedded[1];
} R3Embedded, *PR3Embedded;

typedef void  (ACPI_INTERNAL_VAR_XFACE *ACPINOTIFY)( PEventNotify Notify);

typedef struct _DrvNotify_
{
    ACPINOTIFY              Driver;
    UINT32                  Counter;
    struct _DrvNotify_      *Next;
} DriverNotify, *PDriverNotify;

typedef struct _OsExecute_
{
    ACPI_OSD_EXEC_CALLBACK  Function;
    void                    *Context;
} OSEXECUTE, *POSEXECUTE;

typedef void  (APIENTRY *ACPICPUEXEC)(UINT32 CPUNum, void *Context);

typedef struct _CpuExecute_
{
    ACPICPUEXEC             Function;
    void                    *Context;
} CPUEXECUTE, *PCPUEXECUTE;


#define EC_QUEUE_MASK 0xff
#define EC_QUEUE_SIZE 0x100

#define GPE_QUEUE_MASK 0xff
#define GPE_QUEUE_SIZE 0x100

#define NOTIFY_QUEUE_MASK 0xff
#define NOTIFY_QUEUE_SIZE 0x100

#define OS_EXECUTE_MASK 0xff
#define OS_EXECUTE_SIZE 0x100

#define CPU_EXECUTE_SIZE 0x100                               // Call in each busy/idle

typedef struct _WaitTick_
{
    UINT32                  WaitEnd;                         // Flag "wait overflow counter" 
    UINT32                  Start;                           // Start value of ticks
    UINT32                  Current;                         // Current value of ticks, need for clear WaitEnd
    UINT32                  Need;                            // Need value
} WAITTICK, *PWAITTICK;

ACPI_STATUS AcpiStartWaitPMTics(PWAITTICK w,UINT32 Tics);
UINT32 AcpiCheckWaitPMTics(PWAITTICK w);
UINT32 MicroSecondsToTics(UINT32 MicroSeconds);
void InternalThrtl(UINT32 ThrtlFlag);
#define ACPI_INTR_THRTL_ON   1
#define ACPI_INTR_THRTL_OFF  2

#pragma pack()

#endif
