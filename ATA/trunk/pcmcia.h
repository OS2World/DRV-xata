// ============================================================================
//
//     Include File for PCMCIA
//
//
//     OCO Source Materials
//
//     IBM CONFIDENTIAL (IBM CONFIDENTIAL-RESTRICTED when combined
//     with the aggregated OCO source modules for this program)
//
//     xxxx-xxx (C) Copyright IBM Corporation 1993
//
//
//     Module Name: PCMCIA.H
//     Originator : Michihiko Kondoh
//     Date	  : 8/31/93
//     Version	  : 1.00
//
// * Copyright : COPYRIGHT Daniela Engert 1999-2009
// * distributed under the terms of the GNU Lesser General Public License
//
// ============================================================================

// ----------------------------------------------------------------------------
//
//     Function Codes of Card Services
//
// ----------------------------------------------------------------------------

#ifndef __PCMCIA_H__
#define __PCMCIA_H__

// Client Services Functions
#define GetCardServicesInfo  0xAF0B
#define RegisterClient	     0xAF10
#define GetStatus	     0xAF0C
#define ResetCard	     0xAF11
#define SetEventMask	     0xAF31
#define GetEventMask	     0xAF2E

// Resource Management Functions
#define AdjustResourceInfo   0xAF32
#define RequestIO	     0xAF1F
#define ReleaseIO	     0xAF1B
#define RequestIRQ	     0xAF20
#define ReleaseIRQ	     0xAF1C
#define RequestWindow	     0xAF21
#define ReleaseWindow	     0xAF1D
#define ModifyWindow	     0xAF17
#define MapMemPage	     0xAF14
#define RequestSocketMask    0xAF22
#define ReleaseSocketMask    0xAF2F
#define RequestConfiguration 0xAF30
#define GetConfigurationInfo 0xAF04
#define ModifyConfiguration  0xAF27
#define ReleaseConfiguration 0xAF1E

// Bulk Memory Services Functions

// Client Utilities Functions
#define GetFirstTuple	     0xAF07
#define GetNextTuple	     0xAF0A
#define GetTupleData	     0xAF0D
#define GetFirstRegion	     0xAF06
#define GetNextRegion	     0xAF09
#define GetFirstPartition    0xAF05
#define GetNextPartition     0xAF08

// Advanced Client Services Functions

// ----------------------------------------------------------------------------
//
//     Event Codes
//
// ----------------------------------------------------------------------------

#define BATTERY_DEAD	      0x01
#define BATTERY_LOW	      0x02
#define CARD_INSERTION	      0x40
#define CARD_LOCK	      0x03
#define CARD_READY	      0x04
#define CARD_REMOVAL	      0x05
#define CARD_RESET	      0x11
#define CARD_UNLOCK	      0x06
#define EJECTION_COMPLETE     0x07
#define EJECTION_REQUEST      0x08
#define ERASE_COMPLETE	      0x81
#define EXCLUSIVE_COMPLETE    0x0D
#define EXCLUSIVE_REQUEST     0x0E
#define INSERTION_COMPLETE    0x09
#define INSERTION_REQUEST     0x0A
#define REGISTRATION_COMPLETE 0x82
#define RESET_COMPLETE	      0x80
#define RESET_PHYSICAL	      0x0F
#define RESET_REQUEST	      0x10
#define MTD_REQUEST	      0x12
#define CLIENTINFOSIZE	      0x13
#define CLIENTINFO	      0x14
#define TIMER_EXPIRED	      0x15
#define MTD_BUSY	      0x16
#define MTD_TIMER	      0x17
#define MTD_COMPLETE	      0x18
#define MTD_RDYBSY	      0x19

// ----------------------------------------------------------------------------
//
//     Return Codes
//
// ----------------------------------------------------------------------------

#ifndef SUCCESS
#define SUCCESS 	     0x00
#endif
#define BAD_ADAPTER	     0x01
#define BAD_ATTRIBUTE	     0x02
#define BAD_BASE	     0x03
#define BAD_EDC 	     0x04
#define BAD_IRQ 	     0x06
#define BAD_OFFSET	     0x07
#define BAD_PAGE	     0x08
#define READ_FAILURE	     0x09
#define BAD_SIZE	     0x0A
#define BAD_SOCKET	     0x0B
#define BAD_TYPE	     0x0D
#define BAD_VCC 	     0x0E
#define BAD_VPP 	     0x0F
#define BAD_WINDOW	     0x11
#define WRITE_FAILURE	     0x12
#define NO_CARD 	     0x14
#define UNSUPPORTED_FUNCTION 0x15
#define UNSUPPORTED_MODE     0x16
#define BAD_SPEED	     0x17
#define BUSY		     0x18
#define GENERAL_FAILURE      0x19
#define WRITE_PROTECTED      0x1A
#define BAD_ARG_LENGTH	     0x1B
#define BAD_ARGS	     0x1C
#define CONFIGURATION_LOCKED 0x1D
#define IN_USE		     0x1E
#define NO_MORE_ITEMS	     0x1F
#define OUT_OF_RESOURCE      0x20
#define BAD_HANDLE	     0x21

// ----------------------------------------------------------------------------
//
//     Argument Packet for GetCardServicesInfo
//
// ----------------------------------------------------------------------------

struct GCSI_P
{
    USHORT GCSI_InfoLen;		// O   length of data returned by CS
    UCHAR  GCSI_Signature[2];		// I/O ASCII 'CS' returned if CS installed
					//     (must be 0 before calling)
    USHORT GCSI_Count;			// O   number of sockets
    USHORT GCSI_Revision;		// O   BCD value of vendor's CS revision
    USHORT GCSI_CSLevel;		// O   BCD value of CS release
    USHORT GCSI_VStrOff;		// O   offset to vendor string
    USHORT GCSI_VStrLen;		// O   vendor string length
    UCHAR  GCSI_VendorString;		// O   ASCIIZ vendor string buffer area
};

// ----------------------------------------------------------------------------
//
//     Argument Packet for GetConfigurationInfo
//
// ----------------------------------------------------------------------------

struct GCI_P
{
    USHORT GCI_Socket;			// I   Logical socket
    USHORT GCI_Attributes;		// O   bit-mapped
    UCHAR  GCI_Vcc;			// O   Vcc setting
    UCHAR  GCI_Vpp1;			// O   Vpp1 setting
    UCHAR  GCI_Vpp2;			// O   Vpp2 setting
    UCHAR  GCI_IntType; 		// O   Memory or Memory+I/O interface
    ULONG  GCI_ConfigBase;		// O   Card base address of config reg.
    UCHAR  GCI_Status;			// O   Card Status register setting
    UCHAR  GCI_Pin;			// O   Card Pin register setting
    UCHAR  GCI_Copy;			// O   Card Socket/Copy register setting
    UCHAR  GCI_Option;			// O   Card Option register setting
    UCHAR  GCI_Present; 		// O   Card configration register present
    UCHAR  GCI_FirstDevType;		// O   from device ID tuple
    UCHAR  GCI_FuncCode;		// O   from function ID tuple
    UCHAR  GCI_SysInitMask;		// O   from function ID tuple
    USHORT GCI_ManufCode;		// O   from manufacture ID tuple
    USHORT GCI_ManufInfo;		// O   from manufacture ID tuple
    UCHAR  GCI_CardValues;		// O   Valid card register values
    UCHAR  GCI_AssignedIRQ;		// O   IRQ assinged to PC card
    USHORT GCI_IRQAttributes;		// O   Attribute for for assinged IRQ
    USHORT GCI_BasePort1;		// O   Base port address for range
    UCHAR  GCI_NumPorts1;		// O   Number of contigunous ports
    UCHAR  GCI_Attributes1;		// O   bit-mapped
    USHORT GCI_BasePort2;		// O   Base port address for range
    UCHAR  GCI_NumPorts2;		// O   Number of contigunous ports
    UCHAR  GCI_Attributes2;		// O   bit-mapped
    UCHAR  GCI_IOAddrLines;		// O   Number of address lines decoded
    UCHAR  GCI_ExStatus;		// O   Card Extended Status register setting
};

// ----------------------------------------------------------------------------
//
//     Argument Packet for GetFirstTuple/GetNextTuple
//
// ----------------------------------------------------------------------------

struct GFT_P
{
    USHORT GFT_Socket;			// I   Logical socket
    USHORT GFT_Attributes;		// I   bit-mapped
    UCHAR  GFT_DesiredTuple;		// I   Desired tuple code value
    UCHAR  GFT_Reserved;		// I   Reserved (reset to 0)
    USHORT GFT_Flags;			// I/O CS tuple flag data
    ULONG  GFT_LinkOffset;		// I/O CS link state information
    ULONG  GFT_CISOffset;		// I/O CS CIS state information
    UCHAR  GFT_TupleCode;		// O   Tuple found
    UCHAR  GFT_TupleLink;		// O   Link value for tuple found
};

// Bit Map of GFT_Attributes
#define ATB_Zero       0x0000		//
#define ATB_LinkTuples 0x0001		// Return link tuples

// ----------------------------------------------------------------------------
//
//     Argument Packet for GetStatus
//
// ----------------------------------------------------------------------------

struct GS_P
{
    USHORT GS_Socket;			// Logical socket
    USHORT GS_CardState;		// Card State output data
    USHORT GS_SocketState;		// Socket State output data
};

// ----------------------------------------------------------------------------
//
//     Argument Packet for GetTupleData
//
// ----------------------------------------------------------------------------

#define TUPLE_LEN 50

struct GTD_P
{
    USHORT GTD_Socket;			// I   Logical socket
    USHORT GTD_Attributes;		// I   bit-mapped
    UCHAR  GTD_DesiredTuple;		// I   Desired tuple code value
    UCHAR  GTD_TupleOffset;		// I   Offset into tuple
    USHORT GTD_Flags;			// I/O CS tuple flag data
    ULONG  GTD_LinkOffset;		// I/O CS link state information
    ULONG  GTD_CISOffset;		// I/O CS CIS state information
    USHORT GTD_TupleDataMax;		// I   Maximum size of tuple data area
    USHORT GTD_TupleDataLen;		// O   Number of bytes in tuple body
    UCHAR  GTD_TupleData[TUPLE_LEN];	// O   Tuple data
};

// ----------------------------------------------------------------------------
//
//     Argument Packet for MapMemPage
//
// ----------------------------------------------------------------------------

struct MMP_P
{
    ULONG MMP_CardOffset;		// I   Card Offset Address
    UCHAR MMP_Page;			// I   Page Number
};

// ----------------------------------------------------------------------------
//
//     Argument Packet for RegisterClient
//
// ----------------------------------------------------------------------------

struct RC_P
{
    USHORT RC_Attributes;		// I   Attributes
    USHORT RC_EventMask;		// I   Event mask
    USHORT RC_Data;			// I   16 bit data determined by client
    USHORT RC_Segment;			// I   16 bit segment for client data
    USHORT RC_Offset;			// I   16 bit offset determined
    USHORT RC_Reserved; 		// I   16 bit reserved (reset to 0)
    USHORT RC_Version;			// I   Client version number
};

// Bit Map of RC_Attributes
#define ATB_MemoryClient     0x0001	// Memory client driver
#define ATB_MemoryTechnology 0x0002	// Memory technology driver
#define ATB_IOClient	     0x0004	// I/O client driver
#define ATB_Insert4Sharable  0x0008	// INSERTION event for sharable PC cards
#define ATB_Insert4Exclusive 0x0010	// INSERTION event for cards being
					// exclusively used
// Bit Map of RC_EventMask
#define EM_WriteProtectChange 0x0001	// Write protect change
#define EM_CardLockChange     0x0002	// Card lock change
#define EM_EjectionRequest    0x0004	// Ejection request
#define EM_InsertionRequest   0x0008	// Insertion request
#define EM_BatteryDead	      0x0010	// Battery dead
#define EM_BatteryLow	      0x0020	// Battery low
#define EM_ReadyChange	      0x0040	// Ready change
#define EM_CardDetectChange   0x0080	// Card detect change
#define EM_PowerManageChange  0x0100	// Power management change
#define EM_Reset	      0x0200	// Reset
#define EM_SocketServices     0x0400	// Socket Services updated

// ----------------------------------------------------------------------------
//
//     Argument Packet for RequestIO/ReleaseIO
//
// ----------------------------------------------------------------------------

struct IO_P
{
    USHORT IO_Socket;			// I   Logical socket
    USHORT IO_BasePort1;		// I/O Base port address for range 1
    UCHAR  IO_NumPorts1;		// I   Number of contiguous ports
    UCHAR  IO_Attributes1;		// I   bit-mapped
    USHORT IO_BasePort2;		// I/O Base port address for range 2
    UCHAR  IO_NumPorts2;		// I   Number of contiguous ports
    UCHAR  IO_Attributes2;		// I   bit-mapped
    UCHAR  IO_IOAddrLines;		// I   Number of I/O address lines
};

// Bit Map of IO_Attributes1/IO_Attributes2
#define ATB_Shared	  0x01		// Shared
#define ATB_FirstShared   0x02		// First shared
#define ATB_ForceAlias	  0x04		// Force alias accessibility
#define ATB_DataPathWidth 0x08		// Data path width for I/O range

// ----------------------------------------------------------------------------
//
//     Argument Packet for RequestIRQ/ReleaseIRQ
//
// ----------------------------------------------------------------------------

struct IRQ_P				// RequestIRQ
{
    USHORT IRQ_Socket;			// I   Logical socket
    USHORT IRQ_Attributes;		// I/O bit-mapped
    UCHAR  IRQ_AssignedIRQ;		// O   IRQ number assigned by CS
    UCHAR  IRQ_IRQInfo1;		// I   First PCMCIA IRQ byte
    USHORT IRQ_IRQInfo2;		// I   Optional PCMCIA IRQ bytes
};					// The usage of IRQ_IRQInfo1/2 are the
					// same as Interrupt information in
					// Configuration-Table-Entry tuple.

struct RIRQ_P				// ReleaseIRQ
{
    USHORT RIRQ_Socket; 		// I   Logical socket
    USHORT RIRQ_Attributes;		// I   bit-mapped
    UCHAR  RIRQ_AssignedIRQ;		// I   IRQ number assigned by CS
};

// Bit map of IRQ_Attributes
					// bit 0-1: IRQ type
#define ATB_Exclusive	   0x0000	// Exclusive
#define ATB_TimeMultiShare 0x0001	// Time-multiplexed-sharing
#define ATB_DynamicShare   0x0002	// Dynamic sharing
#define ATB_Reserved	   0x0003	// Reserved
#define ATB_ForcePulse	   0x0004	// Force pulse
#define ATB_First_Shared   0x0008	// First shared
#define ATB_PulseIRQ	   0x0100	// Pulse IRQ allocated

// ----------------------------------------------------------------------------
//
//     Argument Packet for RequestWindow
//
// ----------------------------------------------------------------------------

struct RW_P
{
    USHORT RW_Socket;			// I   Logical socket
    USHORT RW_Attributes;		// I/O bit-mapped
    ULONG  RW_Base;			// I/O System base address
    ULONG  RW_Size;			// I   Memory window size
    UCHAR  RW_AccessSpeed;		// I   Window speed field
};

// Bit map of RW_Attributes
#define ATB_MemoryType	    0x0002	// Memory type
#define ATB_Enabled	    0x0004	// Enabled
#define ATB_Data_Path_Width 0x0008	// Data path width
#define ATB_Paged	    0x0010	// Paged
#define ATB__Shared	    0x0020	// Shared
#define ATB_First__Shared   0x0040	// First shared
#define ATB_WindowSized     0x0100	// Card offsets are window sized

// ----------------------------------------------------------------------------
//
//     Argument Packet for RequestConfiguration
//
// ----------------------------------------------------------------------------

struct RCF_P
{
    USHORT RCF_Socket;			// I   Logical Socket
    USHORT RCF_Attribute;		// I   bit-mapped
    UCHAR  RCF_Vcc;			// I   Vcc setting
    UCHAR  RCF_Vpp1;			// I   Vpp1 setting
    UCHAR  RCF_Vpp2;			// I   Vpp2 setting
    UCHAR  RCF_IntType; 		// I   Memory or Memory+I/O Interface
    ULONG  RCF_ConfigBase;		// I   Card base add of config register
    UCHAR  RCF_Status;			// I   Card Status register setting
    UCHAR  RCF_Pin;			// I   Card Pin register setting
    UCHAR  RCF_Copy;			// I   Card Socket/Copy register setting
    UCHAR  RCF_Option;			// I   Card Option register setting
    UCHAR  RCF_Present; 		// I   Card config register present
    UCHAR  RCF_ExStatus;		// I   Card Extended Status register setting
};

// Bit map of RCF_Attributes
#define ATB_ExclusiveUse  0x0001	      // Exclusive Use
#define ATB_EIRQ	  0x0002	      // Enable IRQ steering

// modify configuration
#define ATB_IRQChgValid   0x0004	      // Change Valid: IRQ
#define ATB_VccChgValid   0x0008	      // Change Valid: Vcc
#define ATB_Vpp1ChgValid  0x0010	      // Change Valid: Vpp1
#define ATB_Vpp2ChgValid  0x0020	      // Change Valid: Vpp2

// Bit map of RCF_IntType
#define IT_MEM	    0x0001		// Memory
#define IT_MEMandIO 0x0002		// Memory and I/O

// Bit map of RCF_Present
#define CB_Option   0x0001		// Option register
#define CB_Status   0x0002		// Status register
#define CB_Pin	    0x0004		// Pin Replacement register
#define CB_Copy     0x0008		// Socket/Copy register

// ----------------------------------------------------------------------------
//
//     Argument Packet for ReleaseConfiguration
//
// ----------------------------------------------------------------------------

struct RLC_P
{
    USHORT RLC_Socket;			// I   Logical Socket
};

// ----------------------------------------------------------------------------
//
//     Argument Packet for SetEventMask/GetEventMask
//
// ----------------------------------------------------------------------------

struct SEM_P
{
    USHORT SEM_Attributes;		// bit-mapped
    USHORT SEM_EventMask;		// event mask bit-mapped
    USHORT SEM_Socket;			// logical socket
};

// ----------------------------------------------------------------------------
//
//     Argument Packet for VendorSpecific (QueryLockStatus)
//
// ----------------------------------------------------------------------------

struct VS_QLS_P
{
    USHORT VS_QLS_InfoLen;		// length of returned information
    UCHAR  VS_QLS_FuncCode;		// function code
    UCHAR  VS_QLS_SubFunc;		// sub-function code
    USHORT VS_QLS_Socket;		// logical socket number
    USHORT VS_QLS_Signature;		// signature
    USHORT VS_QLS_Indicator;		// indicator
};

// Bit Map of VS_QLS_Indicator
#define IND_Lock 0x0001 		// locked
#define IND_Boot 0x0002 		// booted socket
#define IND_Auto 0x0004 		// auto lock mode enabled


struct CI_Info {
  USHORT CI_MaxLength;
  USHORT CI_InfoLen;
  USHORT CI_Attributes;
  USHORT CI_Revision;
  USHORT CI_CSLevel;
  USHORT CI_RevData;
  USHORT CI_NameOff;
  USHORT CI_NameLen;
  USHORT CI_VStringOff;
  USHORT CI_VStringLen;
  UCHAR  CI_Name[20];
  UCHAR  CI_Vendor[50];
};

// ----------------------------------------------------------------------------
//
//     Tuple Codes
//
// ----------------------------------------------------------------------------

// Layer 1 Basic Compatibility Tuples
#define CISTPL_NULL	     0x00	// Null Tuple
#define CISTPL_DEVICE	     0x01	// Device Information Tuple
#define CISTPL_CHECKSUM      0x10	// Checksum Control Tuple
#define CISTPL_LONGLINK_A    0x11	// Long-Link-Control Tuple (to Attribute)
#define CISTPL_LONGLINK_C    0x12	// Long-Link-Control Tuple (to Common)
#define CISTPL_LINKTARGET    0x13	// Link-Target-Control Tuple
#define CISTPL_NO_LINK	     0x14	// No-Link-Control Tuple
#define CISTPL_VERS_1	     0x15	// Level 1 Ver/Prod-Info Tuple
#define CISTPL_ALTSTR	     0x16	// Alternate-Language-String Tuple
#define CISTPL_DEVICE_A      0x17	// Attribute Memory Device Information
#define CISTPL_JEDEC_C	     0x18	// JEDEC Programming Info (for Common)
#define CISTPL_JEDEC_A	     0x19	// JEDEC Programming Info (for Attribute)
#define CISTPL_CONFIG	     0x1A	// Configuration Tuple
#define CISTPL_CFTABLE_ENTRY 0x1B	// Configuration-Table-Entry Tuple
#define CISTPL_DEVICE_OC     0x1C	// Other Operating Conditions Device Info for Common
#define CISTPL_DEVICE_OA     0x1D	// Other Operating Conditions Device Info for Attribute

// Layer 2 Data Recording Format Tuples
#define CISTPL_VERS_2	     0x40	// Level 2 Version Tuple
#define CISTPL_FORMAT	     0x41	// Format Tuple
#define CISTPL_GEOMETRY      0x42	// Geometry Tuple
#define CISTPL_BYTEORDER     0x43	// Byte Order Tuple
#define CISTPL_DATE	     0x44	// Card Initialization Date Tuple
#define CISTPL_BATTERY	     0x45	// Battery Replacement Date Tuple

// Layer 3 Data Organization Tuples
#define CISTPL_ORG	     0x46	// Organization Tuple

// Layer 4 System-Specific Standard Tuples
#define CISTPL_END	     0xFF	// End-of-List Tuple

#define CISTPL_FUNCID	     0x21	// Function Identification Tuple
#define CISTPL_FUNCE	     0x22	// Function Extension Tuple

// PC Card Functions in CISTPL_FUNCID
#define TPLFID_Multi	 0x00
#define TPLFID_Memory	 0x01
#define TPLFID_Serial	 0x02
#define TPLFID_Parallel  0x03
#define TPLFID_FixedDisk 0x04
#define TPLFID_Video	 0x05
#define TPLFID_LAN	 0x06

// Type of Extended Data in CISTPL_FUNCE
#define TPLFE_Serial	 0x00
#define TPLFE_ModemIF	 0x01
#define TPLFE_DataModem  0x02
#define TPLFE_FaxModem	 0x03
#define TPLFE_Voice	 0x04

#endif
