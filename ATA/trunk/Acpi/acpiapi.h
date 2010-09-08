/** @acpi.c
  *
  *  API interface for ACPI
  *  Modified to support 16-bit eComStation/OS2 drivers (see _MSC_VER)
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
#ifndef __ACPIAPI_H__
#define __ACPIAPI_H__

#pragma pack(1)

#define DRV_NAME "ACPICA$"
typedef struct _VersionAcpi_
{
    ULONG                   Major;
    ULONG                   Minor;

} ACPI_VERSION;

typedef struct _AcpiApiHandle_
{
    HFILE                   AcpiDrv;                       // Handle to ACPICA driver
    ACPI_VERSION            PSD;                           // Version PSD
    ACPI_VERSION            Driver;                        // Version ACPICA driver
    ACPI_VERSION            DLL;                           // Version acpi32.dll
    ULONG                   StartAddrPSD;                  // Start address PSD (for testcase)
    ULONG                   AddrCommApp;                   // Address DosCommApp from PSD (which not write IBM)
    ULONG                   StartAddrDriver;               // Start address ACPICA (for testcase)
    ULONG                   AddrFindPSD;                   // Address function for find PSD (find CommApp)
    ULONG                   IRQNumber;                     // Number use IRQ
    void                    *Internal;                     // For internal DLL use
} ACPI_API_HANDLE, *PACPI_API_HANDLE;

typedef struct _LAPICError_
{
    UINT32                  SendChecksum;
    UINT32                  ReceiveChecksum;
    UINT32                  SendAccept;
    UINT32                  ReceiveAccept;
    UINT32                  SendIllegal;
    UINT32                  ReceiveIllegal;
    UINT32                  IllegalRegAddress;
} LAPICError, *PLAPICError;

#define MAX_CSTATE  3

#ifndef _MSC_VER

typedef struct _Processor_
{
    ACPI_IO_ADDRESS         PblkAddress;                   // Address P_BLK
    UINT32                  PblkLength;                    // Length  P_BLK
    UINT8                   Width;                         // Width of trottling
    UINT8                   Offset;                        // ofsset from begin
    UINT8                   Lint;                          // Field from MADT
    UINT8                   LocalApicId;                   // Field from MADT
    UINT32                  ProcessorId;                   // Field from MADT
    LAPICError              APIC;                          // Local APIC error counter
    ACPI_HANDLE             Handle;                        // Handle to ACPI tree
    ACPI_GENERIC_ADDRESS    PerfControl;                   // Perfomance control
    ACPI_GENERIC_ADDRESS    PerfStatus;                    // Perfomance control
    UINT32                  CxLatency[MAX_CSTATE];         // C state latency
    ACPI_GENERIC_ADDRESS    CxState[MAX_CSTATE];           // C state control register
    UINT32                  State;                         // CPU State Idle/Busy
    UINT32                  Divider;                       // Divider for APIC timer (HLT)
    UINT64                  IPICount;                      // Statistics for IPI
    UINT32                  TimerValue;                    // How many time in HLT
    UINT32                  InIdle;                        // For skip 1st going to Idle
    UINT32                  CR3Addr;                       // Phys addr of PageDir
} PROCESSOR, *PPROCESSOR;

#endif // _MSC_VER

typedef void   (*EOIFunction)(void);
typedef void   (*KernelFunction)(void);
typedef void   (*MaskFunction)(UINT32 IRQ,UINT32 Flag);
typedef UINT32 (*IxRFunction )(UINT32 IRQ,UINT32 Flag);
typedef struct _IRQ_
{
    EOIFunction             EOI;                           // EOI for current chip
    UINT64                  Count;                         // Called count
    UINT32                  Triggering;                    // Edge/Level
    UINT32                  Polarity;                      // High/Low
    UINT32                  Sharable;                      // Yes/No
    UINT32                  Vector;                        // Vector for IDT
    volatile UINT32         *IOApicAddress;                // Address IO APIC for this IRQ
    UINT32                  Line;                          // Line number for IO APIC (not EQ to IRQ on bus)
    UINT32                  SetIRQLow;                     // Low dword for IO APIC
    UINT32                  SetIRQHigh;                    // High dword for IO APIC
    KernelFunction          OS2handler;                    // OS/2 IRQ handler
    MaskFunction            Mask;                          // Function for mask/unmask IRQ
    IxRFunction             IxR;                           // Function for ISR/IRR 
} AcpiIRQ,   *PAcpiIRQ;

#define IRQ_EOI_APIC          1                            // EOI as APIC
#define IRQ_EOI_PIC           2                            // EOI as PIC
#define IRQ_EOI_VIRTUALWIRED  4                            // EOI first PIC, then APIC

typedef struct _IRQBus_
{
    char                    *Name;                         // Name in ACPI tree
    UINT32                  BusNumber;                     // Bus number for all devices in this name
    ACPI_OBJECT             *Pointer;                      // Pointer to routing table for this bus
} IRQBus, *PIRQBus;

typedef struct _IOAPIC_
{
    UINT32                  PhysAddr;                      // Physical address from MADT
    UINT32                  IOApicId;                      // ID from MADT
    UINT32                  BaseIRQ;                       // Start IRQ from. getting from MADT
    volatile UINT32         *LinAddr;                      // Linear address
    UINT8                   Version;                       // Chip version
    UINT8                   ID;                            // Chip ID (must be equ with MADT)
    UINT8                   Arbitration;                   // Chip arbitration
    UINT8                   Lines;                         // How many lines in this chip
} IOAPIC, *PIOAPIC;

typedef struct _ACPI_STATISTICS_
{
    UINT32                  Command;
    UINT32                  Number;
    UINT32                  Status;
    PAcpiIRQ                Data;
}  OS2_ACPI_STATISTICS, *POS2_ACPI_STATISTICS;

typedef struct _ACPI_Notify_
{
    UINT32                  Number;                        // Message Number
    ACPI_HANDLE             Handle;                        // Handle to ACPI device
} ACPINotify, *PACPINotify;

#define ACPI_STATISTICS_GETSIZEIRQ    1
#define ACPI_STATISTICS_GETIRQ        2
#define ACPI_STATISTICS_CLEARIRQ      3
#define ACPI_STATISTICS_GETSIZECPU    4
#define ACPI_STATISTICS_GETCPU        5

typedef struct _KnownDevice_
{
    ACPI_PCI_ID             PciId;                         // PCI id
    UINT32                  PicIrq;                        // IRQ in PIC mode
                                                           // ====== WARNING!!!!
                                                           // If set LINK name x - this field will has value x
                                                           // ====== WARNING!!!!
    UINT32                   ApicIrq;                      // IRQ in APIC mode.
                                                           // ====== WARNING!!!!
                                                           // If set REMAP x TO y - this field will has value y
                                                           // ====== WARNING!!!!
    ACPI_HANDLE               Handle;                      // Handle to ACPI name, if present
} KNOWNDEVICE, *PKNOWNDEVICE;

typedef struct _ButtonEvent_
{
    UINT32                  ButtonStatus;
    UINT32                  WhichButton;
} BUTTONEVENT, *PBUTTONEVENT;
//---------------------------------------------------------------------
// Rudi:  additional IOCTL data structures

typedef struct
{
    ULONG                   ulADDMajor;
    ULONG                   ulADDMinor;
    ULONG                   ulADDRsvd1;                    // Address 1st 32bit function
    ULONG                   ulADDRsvd2;                    // Address last 32 bit function

    ULONG                   ulPSDMajor;
    ULONG                   ulPSDMinor;
    ULONG                   ulPSDRsvd1;                    // Address 1st 32bit function
    ULONG                   ulPSD_APP_COMM;                // Address PSD_APP_COMM. PS change. Need for detect function if trap is going
    ULONG                   ulIntNum;                      // IRQ use for acpi
} ACPI_VERSION_REQ;


typedef struct
{
    ULONGLONG               ullPMCounter;
    ULONG                   ulAcpiTimer;
} ACPI_TIMER_REQ;


typedef struct
{
    ULONG                   ulTimeout;                     // on return:  button state
    ULONG                   ulButtonID;                    // on return:  error code
} ACPI_BUTTON_REQ;


typedef struct
{
    ULONG                   ulMajor;                       // version number
    ULONG                   ulMinor;                       // version number
    ULONG                   ulAPI16;                       // CallACPI16 entry
    ULONG                   ulTBL16;                       // 16bit function table
    ULONG                   ulAPI32;                       // CallACPI32 entry
    ULONG                   ulTBL32;                       // 32bit function table
} ACPI_VERSION2_REQ;


// Rudi:  additional IOCTL data structures end
//---------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

APIRET APIENTRY AcpiStartApi(ACPI_API_HANDLE *);
APIRET APIENTRY AcpiEndApi(ACPI_API_HANDLE *);
APIRET APIENTRY AcpiWaitPressButton(ACPI_API_HANDLE *,ULONG,ULONG,ULONG *);
APIRET APIENTRY AcpiGoToSleep(ACPI_API_HANDLE *,UINT8);
APIRET APIENTRY AcpiGetUpTime(ACPI_API_HANDLE *,UINT64 *,UINT32 *);
APIRET APIENTRY AcpiGetTrottling(ACPI_API_HANDLE *Hdl,UINT32 ProcId,UINT32 *Perfomance,UINT8 *Width);
APIRET APIENTRY AcpiSetTrottling(ACPI_API_HANDLE *,UINT32 ,UINT32 );
APIRET APIENTRY AcpiSetCState(ACPI_API_HANDLE *Hdl,UINT32 ProcId,UINT32 Perfomance,UINT32 State);
APIRET APIENTRY OS2AcpiGetStatistics(ACPI_API_HANDLE *Hdl,void *Buffer);
APIRET APIENTRY AcpiSetPowerState(ACPI_API_HANDLE *Hdl,UINT32 ProcId,UINT32 State);
APIRET APIENTRY AcpiEC(ACPI_API_HANDLE *Hdl);
APIRET APIENTRY WaitNotify(ACPI_API_HANDLE *Hdl,ACPI_HANDLE *NotifyHandle,UINT32 *NotifyNumber,UINT32 What,UINT32 TimeOut);
APIRET APIENTRY WaitEmbeddedEvent(ACPI_API_HANDLE *Hdl,ACPI_HANDLE *EmbeddedHandle,UINT32 *EmbeddedNumber,UINT32 TimeOut);
APIRET APIENTRY AcpiGetPCIDev(ACPI_API_HANDLE *Hdl,PKNOWNDEVICE dev);
APIRET APIENTRY AcpiRWEmbedded(ACPI_API_HANDLE *Hdl,UINT32 Number,UINT32 Function, ACPI_PHYSICAL_ADDRESS Address, ACPI_INTEGER *Value);
unsigned char * APIENTRY AcpiStatusToStr(ACPI_STATUS Status);
void SetCPU ( UINT32 Number);

#ifdef __cplusplus
}
#endif

#define WAIT_POWER_FOREVER             -1
#define SET_POWERBUTTON                0x100               // Wait Power Button
#define SET_SLEEPBUTTON                0x101               // Wait Sleep Button
#define ERROR_WRONG_VERSION            0xC0000001          // Wrong version
// For PMtimer use
#define PM_TIMER_FREQUENCY_SEC         3579545
#define PM_TIMER_FREQUENCY_MSEC        3580
#define PM_TIMER_FREQUENCY_10MSEC      35795
#define PM_TIMER_FREQUENCY_20MSEC      35795 * 2
#define PM_TIMER_FREQUENCY_MkSEC       4
#define PM_TIMER_FREQUENCY_2MkSEC      7
#define PM_TIMER_FREQUENCY_10MkSEC     36
#define PM_TIMER_FREQUENCY_20MkSEC     72
#define PM_TIMER_FREQUENCY_100MkSEC    358
// For execute at all CPU
#define SMPEXEC_IPITIME                0x0100              // run from IPI, stack is NOT OS/2 stack
#define SMPEXEC_TASKTIME               0x0200              // run from PSD_SET_PROC_STATE, PSD stack
#define SMPEXEC_WBINDVD                0x0400              // Flush cache at all CPU
#define SMPEXEC_SYNC                   0x0800              // Synchronize CPU
#define SMPEXEC_FLAGMASK               0xff00              // Max flags
#define SMPEXEC_CPUMASK                0xff                // CPU mask, each bit mean - fuction must be execute in this CPU(BitNumber)

#pragma pack()

#endif

