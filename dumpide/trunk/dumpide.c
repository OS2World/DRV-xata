#define INCL_DOS
#include <OS2.H>
#include <acpi.h>
#include <acpiapi.h>
#include <amlresrc.h>
#include <ipdc.h>
#include <STDIO.H>
#include <STDLIB.H>
#include <CTYPE.H>
#include <MEMORY.H>
#include <STRING.H>

/* --------------------------------------------------------------------------*/

#define VENDORIDINTEL	    0x8086
#define VENDORIDALI	    0x10B9
#define VENDORIDVIA	    0x1106
#define VENDORIDSIS	    0x1039
#define VENDORIDCMD	    0x1095
#define VENDORIDPROMISE     0x105A
#define VENDORIDCYRIX	    0x1078
#define VENDORIDNS	    0x100B
#define VENDORIDAMD	    0x1022
#define VENDORIDARTOP	    0x1191
#define VENDORIDEFAR	    0x1055
#define VENDORIDNVIDIA	    0x10DE
#define VENDORIDSERVERWORKS 0x1166
#define VENDORIDOPTI	    0x10B5
#define VENDORIDITE	    0x1283
#define VENDORIDINITIO	    0x1101

/* --------------------------------------------------------------------------*/

typedef USHORT WORD;
#undef APIENTRY16
#define APIENTRY16 _Optlink

ULONG APIENTRY16 ReadPort (USHORT Port);
ULONG APIENTRY16 ReadPortPDC (USHORT Port, UCHAR Index);
ULONG APIENTRY16 ReadPortOpti (USHORT Port);
VOID APIENTRY16 WritePort (USHORT Port, ULONG Value);
VOID APIENTRY16 WritePortOpti (USHORT Port, ULONG Value);

ULONG MapPhysicalToLinear (ULONG PhysicalAddress, ULONG Length);
static ULONG in32 (int Reg);
static void out32 (int Reg, ULONG Val);

static UCHAR clearSATAStatus = 0;
static UCHAR VIAspecial = 0;
static UCHAR SISspecial = 0;
static UCHAR scanACPI	 = TRUE;
static UCHAR verboseScan = FALSE;

static UCHAR	BusCount;
static unsigned numAdapters = 0;

/* --------------------------------------------------------------------------*/

#pragma pack(1)

typedef struct {
  WORD	VendorID, DeviceID, Command, Status;
  UCHAR Revision;
  UCHAR ProgIF, SubClassCode, ClassCode;
  UCHAR CacheLineSize, LatencyTimer, HeaderType, BIST;
  ULONG BaseAddr[6], CIS;
  WORD	SubVendorID, SubDeviceID;
  ULONG EPROMBase;
  ULONG reserved[2];
  UCHAR IRQ, Int, MinGnt, MaxLat;
  UCHAR x[0xC0];
} tPCIConfigArea, *pPCIConfigArea;

typedef struct {
  UCHAR Command, res1;
  UCHAR Status, res2;
  ULONG pTable;
} tBMChannel, *pBMChannel;

typedef struct {
  ULONG Status, Error, Control, Active, Notification;
} tSCR, *pSCR;

static int ReadPCIConfigArea (UCHAR Bus, UCHAR Dev, UCHAR Fnc, pPCIConfigArea pC);
static int ReadBMIFArea (WORD Adr, WORD Idx, WORD Len, void *pC);
static void DumpBMIFArea (WORD Idx, WORD Len, void *pC);
static void DumpPCIConfigArea (pPCIConfigArea pC);
static void DumpSCR (int Channel, int numRegs);

static int rc;
static HFILE hPCI, hIO, hACPI;
static UCHAR Bus, Dev, Fnc;
static tPCIConfigArea PCIConfigArea;
static ULONG BMIFAddr, BMIFLen;
static UCHAR BMData[0x10000];
static tSCR SCR;

/* --------------------------------------------------------------------------*/

typedef struct _Adapter tAdapter, *pAdapter;
typedef void (*tPostFun)(pAdapter);

typedef struct {
  USHORT DeviceId, VendorId;
  char	*Name;
  UCHAR  Config;
  tPostFun PostFun;
  tPostFun PostSATA;
} tAdapterTable, *pAdapterTable;

struct _Adapter {
  UCHAR  Bus, Dev, Fnc;
  UCHAR  Rev;
  USHORT Vendor, Device;
  pAdapterTable Adapter;

  ACPI_HANDLE Handle;
  ACPI_NAME   Name;
  UCHAR       PCIIRQ, PicIRQ, APicIRQ;

  UCHAR ProgIF;
};

#define MAX_NUM_ADAPTERS 256
static tAdapter Adapter[MAX_NUM_ADAPTERS];

/* --------------------------------------------------------------------------*/

static void FindSB (pAdapter p);
static void FindHB (pAdapter p);
static void FindViaSB (pAdapter p);
static void FindSwSB (pAdapter p);
static void FindOptiSB (pAdapter p);
static void FindALiSB (pAdapter p);
static void FindPiixSB (pAdapter p);
static void CheckGeneric (pAdapter p);
static void GetPDCData (pAdapter p);
static void GetPDCDataSATA (pAdapter p);
static void GetATP867Data (pAdapter p);
static void GetViaSCR (pAdapter p);
static void GetNvidiaSCR (pAdapter p);
static void GetSiSSCR (pAdapter p);
static void GetALiSCR5281 (pAdapter p);
static void GetALiSCR5287 (pAdapter p);
static void GetALiSCR5289 (pAdapter p);
static void GetSiISCR (pAdapter p);
static void GetIntelSCR (pAdapter p);
static void GetPdcSCR_SATA (pAdapter p);
static void GetPdcSCR_SATA2 (pAdapter p);
static void GetPdcSCR_Combo (pAdapter p);
static void GetPdcSCR_Combo2 (pAdapter p);
static void GetJMicronSCR (pAdapter p);
static void GetInitioSCR (pAdapter p);
static void GetViaAHCISCR (pAdapter p);
static void GetIxpSCR (pAdapter p);

static tAdapterTable AdapterTable[] = {
  { 0x0640, 0x1095, "CMD 640",                         3, NULL },
  { 0x1230, 0x8086, "Intel PIIX",                      1, FindSB },
  { 0x7010, 0x8086, "Intel PIIX3",                     1, FindSB },
  { 0x7111, 0x8086, "Intel PIIX4",                     1, FindPiixSB },
  { 0x7199, 0x8086, "Intel PIIX4",                     1, FindPiixSB },
  { 0x1571, 0x1106, "VIA 571",                         1, FindSB },
  { 0x0571, 0x1106, "VIA 571",                         3, FindViaSB },
  { 0x3149, 0x1106, "VIA VT8237/6420 SATA",            3, FindViaSB, GetViaSCR },
  { 0x3249, 0x1106, "VIA VT6421 SATA",                 3, NULL, GetViaSCR },
  { 0x3164, 0x1106, "VIA VT6410 PATA",                 3, NULL },
  { 0x3349, 0x1106, "VIA VT8251 SATA",                 3, NULL, GetViaAHCISCR },
  { 0x5229, 0x10B9, "ALi 5229",                        1, FindALiSB },
  { 0x5228, 0x10B9, "ALi 5228",                        2, NULL },
  { 0x5281, 0x10B9, "ALi 5281 SATA",                   2, NULL, GetALiSCR5281 },
  { 0x5287, 0x10B9, "ALi 5287 SATA",                   2, NULL, GetALiSCR5287 },
  { 0x5289, 0x10B9, "ALi 5289 SATA",                   2, NULL, GetALiSCR5289 },
  { 0x5513, 0x1039, "SiS 5513",                        1, FindHB },
  { 0x5517, 0x1039, "SiS 5517",                        1, FindHB },
  { 0x5518, 0x1039, "SiS 5518",                        1, FindHB },
  { 0x0180, 0x1039, "SiS 180 SATA",                    2, NULL, GetSiSSCR },
  { 0x0181, 0x1039, "SiS 181 SATA",                    2, NULL, GetSiSSCR },
  { 0x0182, 0x1039, "SiS 182 SATA",                    2, NULL, GetSiSSCR },
  { 0x0643, 0x1095, "CMD 643",                         3, NULL },
  { 0x0646, 0x1095, "CMD 646",                         3, NULL },
  { 0x0648, 0x1095, "CMD 648",                         3, NULL },
  { 0x0649, 0x1095, "CMD 649",                         3, NULL },
  { 0x0680, 0x1095, "SiliconImage 680",                2, NULL },
  { 0x3112, 0x1095, "SiliconImage 3112 SATA",          2, NULL, GetSiISCR },
  { 0x3114, 0x1095, "SiliconImage 3114 SATA",          2, NULL, GetSiISCR },
  { 0x3512, 0x1095, "SiliconImage 3512 SATA",          2, NULL, GetSiISCR },
  { 0x0240, 0x1095, "Adaptec AAR-1210SA",              2, NULL, GetSiISCR },
  { 0x4D33, 0x105A, "Promise Ultra33",                 2, NULL },
  { 0x4D38, 0x105A, "Promise Ultra66",                 2, NULL },
  { 0x0D38, 0x105A, "Promise Ultra66",                 2, NULL },
  { 0x4D30, 0x105A, "Promise Ultra100",                2, NULL },
  { 0x0D30, 0x105A, "Promise Ultra100",                2, NULL },
  { 0x4D68, 0x105A, "Promise Ultra100 TX2",            2, GetPDCData },
  { 0x6268, 0x105A, "Promise Ultra100 TX2",            2, GetPDCData },
  { 0x4D69, 0x105A, "Promise Ultra133 TX2",            2, GetPDCData },
  { 0x6269, 0x105A, "Promise Ultra133 TX2",            2, GetPDCData },
  { 0x1275, 0x105A, "Promise Ultra133 TX2",            2, GetPDCData },
  { 0x5275, 0x105A, "Promise Ultra133 TX2",            2, GetPDCData },
  { 0x7275, 0x105A, "Promise Ultra133 TX2",            2, GetPDCData },
  { 0x3318, 0x105A, "Promise SATA150 TX4",             2, GetPDCDataSATA, GetPdcSCR_SATA },
  { 0x3319, 0x105A, "Promise FT S150 TX4",             2, GetPDCDataSATA, GetPdcSCR_SATA },
  { 0x3371, 0x105A, "Promise FT S150 TX2plus",         2, GetPDCDataSATA, GetPdcSCR_Combo },
  { 0x3375, 0x105A, "Promise SATA150 TX2plus",         2, GetPDCDataSATA, GetPdcSCR_Combo },
  { 0x3376, 0x105A, "Promise FT S150 376",             2, GetPDCDataSATA, GetPdcSCR_Combo },
  { 0x3377, 0x105A, "Promise FT S150 377",             2, GetPDCDataSATA, GetPdcSCR_Combo },
  { 0x3373, 0x105A, "Promise FT S150 378",             2, GetPDCDataSATA, GetPdcSCR_Combo },
  { 0x3372, 0x105A, "Promise FT S150 379",             2, GetPDCDataSATA, GetPdcSCR_Combo },
  { 0x3571, 0x105A, "Promise FT S150 TX2200",          2, GetPDCDataSATA, GetPdcSCR_SATA2 },
  { 0x3D75, 0x105A, "Promise SATAII150 TX2plus",       2, GetPDCDataSATA, GetPdcSCR_Combo2 },
  { 0x3574, 0x105A, "Promise SATAII150 TX2plus",       2, GetPDCDataSATA, GetPdcSCR_Combo2 },
  { 0x3570, 0x105A, "Promise FT S300 TX2300",          2, GetPDCDataSATA, GetPdcSCR_SATA2 },
  { 0x3D18, 0x105A, "Promise SATAII150 TX4",           2, GetPDCDataSATA, GetPdcSCR_SATA2 },
  { 0x3519, 0x105A, "Promise FT S300 TX4200",          2, GetPDCDataSATA, GetPdcSCR_SATA2 },
  { 0x3D17, 0x105A, "Promise SATA300 TX4",             2, GetPDCDataSATA, GetPdcSCR_SATA2 },
  { 0x3515, 0x105A, "Promise FT S300 TX4300",          2, GetPDCDataSATA, GetPdcSCR_SATA2 },
  { 0x3D73, 0x105A, "Promise SATA300 TX2plus",         2, GetPDCDataSATA, GetPdcSCR_Combo2 },
  { 0x6617, 0x105A, "Promise Ultra 617",               2, GetPDCDataSATA },
  { 0x6626, 0x105A, "Promise Ultra 618",               2, GetPDCDataSATA },
  { 0x6629, 0x105A, "Promise Ultra 619",               2, GetPDCDataSATA },
  { 0x6620, 0x105A, "Promise Ultra 620",               2, GetPDCDataSATA },
  { 0x2411, 0x8086, "Intel ICH",                       1, FindSB },
  { 0x7406, 0x8086, "Intel ICH",                       1, FindSB },
  { 0x2421, 0x8086, "Intel ICH0",                      1, FindSB },
  { 0x244B, 0x8086, "Intel ICH2",                      1, FindSB },
  { 0x244A, 0x8086, "Intel ICH2",                      1, FindSB },
  { 0x245B, 0x8086, "Intel C-ICH",                     1, FindSB },
  { 0x248B, 0x8086, "Intel ICH3",                      1, FindSB },
  { 0x248A, 0x8086, "Intel ICH3",                      1, FindSB },
  { 0x24C1, 0x8086, "Intel ICH4",                      1, FindSB },
  { 0x24CB, 0x8086, "Intel ICH4",                      1, FindSB },
  { 0x24CA, 0x8086, "Intel ICH4",                      1, FindSB },
  { 0x24DB, 0x8086, "Intel ICH5",                      3, FindSB },
  { 0x25A2, 0x8086, "Intel ICH6300",                   3, FindSB },
  { 0x269E, 0x8086, "Intel ESB",                       3, FindSB },
  { 0x24D1, 0x8086, "Intel ICH5 SATA",                 3, FindSB },
  { 0x25A3, 0x8086, "Intel ICH5 SATA",                 3, FindSB },
  { 0x24DF, 0x8086, "Intel ICH5R SATA",                3, FindSB },
  { 0x25B0, 0x8086, "Intel ICH5R SATA",                3, FindSB },
  { 0x266F, 0x8086, "Intel ICH6",                      3, FindSB },
  { 0x2680, 0x8086, "Intel ESB2 SATA",                 3, FindSB },
  { 0x27DF, 0x8086, "Intel ICH7",                      3, FindSB },
  { 0x2651, 0x8086, "Intel ICH6 SATA",                 3, FindSB, GetIntelSCR },
  { 0x2652, 0x8086, "Intel ICH6R SATA",                3, FindSB, GetIntelSCR },
  { 0x2653, 0x8086, "Intel ICH6M SATA",                3, FindSB, GetIntelSCR },
  { 0x27C0, 0x8086, "Intel ICH7 SATA",                 3, FindSB, GetIntelSCR },
  { 0x27C2, 0x8086, "Intel ICH7R SATA",                3, FindSB, GetIntelSCR },
  { 0x27C3, 0x8086, "Intel ICH7R SATA",                3, FindSB, GetIntelSCR },
  { 0x27C4, 0x8086, "Intel ICH7M SATA",                3, FindSB, GetIntelSCR },
  { 0x0102, 0x1078, "Cyrix 5530",                      1, NULL },
  { 0x0004, 0x1103, "HPT 36x/37x",                     2, NULL },
  { 0x0005, 0x1103, "HPT 372A",                        2, NULL },
  { 0x0006, 0x1103, "HPT 302",                         2, NULL },
  { 0x0007, 0x1103, "HPT 371",                         2, NULL },
  { 0x0008, 0x1103, "HPT 374",                         2, NULL },
  { 0x0009, 0x1103, "HPT 372N",                        2, NULL },
  { 0x7401, 0x1022, "AMD 751",                         1, FindSB },
  { 0x7409, 0x1022, "AMD 756",                         1, FindSB },
  { 0x7411, 0x1022, "AMD 766",                         1, FindSB },
  { 0x7441, 0x1022, "AMD 768",                         1, FindSB },
  { 0x209A, 0x1022, "AMD 5536 (Geode LX)",             1, FindSB },
  { 0x7469, 0x1022, "AMD 8111",                        1, FindSB },
  { 0x0005, 0x1191, "AEC 6210",                        3, NULL },
  { 0x0006, 0x1191, "AEC 6260",                        3, NULL },
  { 0x0007, 0x1191, "AEC 6260",                        3, NULL },
  { 0x0009, 0x1191, "AEC 6280/6880",                   3, NULL },
  { 0x000A, 0x1191, "AEC 6885/6896",                   2, GetATP867Data },
  { 0x9130, 0x1055, "SMSC SLC90E66",                   1, FindSB },
  { 0x0211, 0x1166, "ServerWorks OSB4",                1, FindSwSB },
  { 0x0212, 0x1166, "ServerWorks CSB5",                1, FindSwSB },
  { 0x0213, 0x1166, "ServerWorks CSB6",                1, FindSwSB },
  { 0x0214, 0x1166, "ServerWorks HT1000 / BCM5785",    1, FindSwSB },
  { 0x4349, 0x1002, "ATI IXP200",                      1, FindSB },
  { 0x4369, 0x1002, "ATI IXP300",                      1, FindSB },
  { 0x4376, 0x1002, "ATI IXP400",                      1, FindSB },
  { 0x438C, 0x1002, "ATI IXP600",                      1, FindSB },
  { 0x436E, 0x1002, "ATI IXP300 SATA",                 2, NULL, GetSiISCR },
  { 0x4379, 0x1002, "ATI IXP400 SATA",                 2, NULL, GetSiISCR },
  { 0x437A, 0x1002, "ATI IXP400 SATA",                 2, NULL, GetSiISCR },
  { 0x4380, 0x1002, "ATI IXP600 SATA",                 2, NULL, GetIxpSCR },
  { 0x4381, 0x1002, "ATI IXP600 SATA",                 2, NULL, GetIxpSCR },
  { 0xC558, 0x1045, "Opti Viper",                      1, FindOptiSB },
  { 0xC621, 0x1045, "Opti Viper",                      1, FindOptiSB },
  { 0xD568, 0x1045, "Opti Viper",                      1, FindOptiSB },
  { 0xD721, 0x1045, "Opti Viper",                      1, FindOptiSB },
  { 0xD768, 0x1045, "Opti Viper",                      1, FindOptiSB },
  { 0x01BC, 0x10DE, "Nvidia nForce",                   1, NULL },
  { 0x0065, 0x10DE, "Nvidia nForce2",                  1, NULL },
  { 0x0085, 0x10DE, "Nvidia nForce2 ultra",            1, NULL },
  { 0x00D5, 0x10DE, "Nvidia nForce3",                  3, NULL },
  { 0x00E5, 0x10DE, "Nvidia nForce3 ultra",            3, NULL },
  { 0x0035, 0x10DE, "Nvidia nForce4 M04",              3, NULL },
  { 0x0053, 0x10DE, "Nvidia nForce4 C04",              3, NULL },
  { 0x0265, 0x10DE, "Nvidia nForce5 M51",              3, NULL },
  { 0x036E, 0x10DE, "Nvidia nForce5 M55",              3, NULL },
  { 0x008E, 0x10DE, "Nvidia nForce2 ultra SATA",       3, NULL, GetNvidiaSCR },
  { 0x00E3, 0x10DE, "Nvidia nForce3 CK8 SATA",         3, NULL, GetNvidiaSCR },
  { 0x00EE, 0x10DE, "Nvidia nForce3 CK8 SATA",         3, NULL, GetNvidiaSCR },
  { 0x0036, 0x10DE, "Nvidia nForce4 M04 SATA",         3, NULL, GetNvidiaSCR },
  { 0x003E, 0x10DE, "Nvidia nForce4 M04 SATA",         3, NULL, GetNvidiaSCR },
  { 0x0054, 0x10DE, "Nvidia nForce4 C04 SATA",         3, NULL, GetNvidiaSCR },
  { 0x0055, 0x10DE, "Nvidia nForce4 C04 SATA",         3, NULL, GetNvidiaSCR },
  { 0x0266, 0x10DE, "Nvidia nForce5 M51 SATA",         3, NULL, GetNvidiaSCR },
  { 0x0267, 0x10DE, "Nvidia nForce5 M51 SATA",         3, NULL, GetNvidiaSCR },
  { 0x036F, 0x10DE, "Nvidia nForce5 M55 SATA",         3, NULL, GetNvidiaSCR },
  { 0x0502, 0x100B, "NS Geode SCx200",                 1, NULL },
  { 0x8211, 0x1283, "ITE 8211",                        2, NULL },
  { 0x8212, 0x1283, "ITE 8212",                        2, NULL },
  { 0x0044, 0x169C, "NetCell SyncRAID",                2, NULL },
  { 0x2360, 0x197B, "JMicron JMB360 SATA",             2, NULL, GetJMicronSCR },
  { 0x2363, 0x197B, "JMicron JMB363 xATA",             2, NULL, GetJMicronSCR },
  { 0x1622, 0x1101, "Initio SATA",                     2, NULL, GetInitioSCR },
  { 0, 0, "Generic SFF-8038i compliant PCI busmaster", 3, CheckGeneric },
  { 0, 0, "Generic PCI IDE (no busmaster DMA)",        3, NULL }
};

/* --------------------------------------------------------------------------*/

static int PCIBIOSPresent (char *VersionMajor, char *VersionMinor, char *BusCount) {
  struct {
    BYTE SubFunction;
  } Parm = {0} ;
  ULONG ParmLen = sizeof (Parm);
  struct {
    BYTE rc;
    BYTE Mechanism;
    BYTE VMajor, VMinor;
    BYTE LastBus;
  } Data;
  ULONG DataLen = sizeof (Data);

  DosDevIOCtl (hPCI, 0x80, 0x0B, &Parm, ParmLen, &ParmLen, &Data, DataLen, &DataLen);
  if (0 == Data.rc) {
    *VersionMajor = Data.VMajor;
    *VersionMinor = Data.VMinor;
    *BusCount	  = Data.LastBus;
  }
  return (Data.rc);
}

static int PCIFindDevice (int DeviceId, int VendorId, int Occurence,
			  char *Bus, char *Device, char *Function) {
  struct {
    BYTE SubFunction;
    WORD DeviceID;
    WORD VendorID;
    BYTE Index;
  } Parm = {1};
  ULONG ParmLen = sizeof (Parm);
  struct {
    BYTE rc;
    BYTE Bus;
    BYTE DevFunc;
  } Data;
  ULONG DataLen = sizeof (Data);

  Parm.DeviceID = DeviceId;
  Parm.VendorID = VendorId;

  DosDevIOCtl (hPCI, 0x80, 0x0B, &Parm, ParmLen, &ParmLen, &Data, DataLen, &DataLen);
  if (0 == Data.rc) {
    *Bus      = Data.Bus;
    *Device   = (Data.DevFunc >> 3) & 0x1F;
    *Function = Data.DevFunc & 0x07;
  }
  return (Data.rc);
}

static int PCIReadConfigB (char Bus, char Device, char Function, int Reg,
			   char *Value) {
  struct {
    BYTE SubFunction;
    BYTE Bus, DevFunc;
    BYTE Register;
    BYTE Size;
  } Parm = {3, 0, 0, 0, 1};
  ULONG ParmLen = sizeof (Parm);
  struct {
    BYTE  rc;
    ULONG Data;
  } Data;
  ULONG DataLen = sizeof (Data);

  Parm.Bus	= Bus;
  Parm.DevFunc	= (Device << 3) | (Function & 0x07);
  Parm.Register = Reg;

  DosDevIOCtl (hPCI, 0x80, 0x0B, &Parm, ParmLen, &ParmLen, &Data, DataLen, &DataLen);
  if (0 == Data.rc) {
    *Value = Data.Data;
  }
  return (Data.rc);
}

static int PCIReadConfigW (char Bus, char Device, char Function, int Reg,
			   USHORT *Value) {
  struct {
    BYTE SubFunction;
    BYTE Bus, DevFunc;
    BYTE Register;
    BYTE Size;
  } Parm = {3, 0, 0, 0, 2};
  ULONG ParmLen = sizeof (Parm);
  struct {
    BYTE  rc;
    ULONG Data;
  } Data;
  ULONG DataLen = sizeof (Data);

  Parm.Bus	= Bus;
  Parm.DevFunc	= (Device << 3) | (Function & 0x07);
  Parm.Register = Reg;

  DosDevIOCtl (hPCI, 0x80, 0x0B, &Parm, ParmLen, &ParmLen, &Data, DataLen, &DataLen);
  if (0 == Data.rc) {
    *Value = Data.Data;
  }
  return (Data.rc);
}

static int PCIReadConfigL (char Bus, char Device, char Function, int Reg,
			   ULONG *Value) {
  struct {
    BYTE SubFunction;
    BYTE Bus, DevFunc;
    BYTE Register;
    BYTE Size;
  } Parm = {3, 0, 0, 0, 4};
  ULONG ParmLen = sizeof (Parm);
  struct {
    BYTE  rc;
    ULONG Data;
  } Data;
  ULONG DataLen = sizeof (Data);

  Parm.Bus	= Bus;
  Parm.DevFunc	= (Device << 3) | (Function & 0x07);
  Parm.Register = Reg;

  DosDevIOCtl (hPCI, 0x80, 0x0B, &Parm, ParmLen, &ParmLen, &Data, DataLen, &DataLen);
  if (0 == Data.rc) {
    *Value = Data.Data;
  }
  return (Data.rc);
}

static int PCIWriteConfigL (char Bus, char Device, char Function, int Reg,
			   ULONG Value) {
  struct {
    BYTE SubFunction;
    BYTE Bus, DevFunc;
    BYTE Register;
    BYTE Size;
    ULONG Data;
  } Parm = {4, 0, 0, 0, 4, 0};
  ULONG ParmLen = sizeof (Parm);
  struct {
    BYTE  rc;
  } Data;
  ULONG DataLen = sizeof (Data);

  Parm.Bus	= Bus;
  Parm.DevFunc	= (Device << 3) | (Function & 0x07);
  Parm.Register = Reg;
  Parm.Data	= Value;

  DosDevIOCtl (hPCI, 0x80, 0x0B, &Parm, ParmLen, &ParmLen, &Data, DataLen, &DataLen);
  return (Data.rc);
}

static int PCIWriteConfigW (char Bus, char Device, char Function, int Reg,
			   USHORT Value) {
  struct {
    BYTE SubFunction;
    BYTE Bus, DevFunc;
    BYTE Register;
    BYTE Size;
    ULONG Data;
  } Parm = {4, 0, 0, 0, 2, 0};
  ULONG ParmLen = sizeof (Parm);
  struct {
    BYTE  rc;
  } Data;
  ULONG DataLen = sizeof (Data);

  Parm.Bus	= Bus;
  Parm.DevFunc	= (Device << 3) | (Function & 0x07);
  Parm.Register = Reg;
  Parm.Data	= Value;

  DosDevIOCtl (hPCI, 0x80, 0x0B, &Parm, ParmLen, &ParmLen, &Data, DataLen, &DataLen);
  return (Data.rc);
}

static int PCIWriteConfigB (char Bus, char Device, char Function, int Reg,
			   UCHAR Value) {
  struct {
    BYTE SubFunction;
    BYTE Bus, DevFunc;
    BYTE Register;
    BYTE Size;
    ULONG Data;
  } Parm = {4, 0, 0, 0, 1, 0};
  ULONG ParmLen = sizeof (Parm);
  struct {
    BYTE  rc;
  } Data;
  ULONG DataLen = sizeof (Data);

  Parm.Bus	= Bus;
  Parm.DevFunc	= (Device << 3) | (Function & 0x07);
  Parm.Register = Reg;
  Parm.Data	= Value;

  DosDevIOCtl (hPCI, 0x80, 0x0B, &Parm, ParmLen, &ParmLen, &Data, DataLen, &DataLen);
  return (Data.rc);
}

static ULONG GetRangeSize (char Bus, char Device, char Function, int Reg) {
  ULONG Save, Test;

  PCIReadConfigL (Bus, Device, Function, Reg, &Save);
  if (Save & 1)
    PCIWriteConfigW (Bus, Device, Function, Reg, (USHORT)-1);
  else
    PCIWriteConfigL (Bus, Device, Function, Reg, (ULONG)-1);
  PCIReadConfigL (Bus, Device, Function, Reg, &Test);
  if (Save & 1) {
    Test |= 0xFFFF0000UL;
    Test &= 0xFFFFFFFCUL;
  } else {
    Test &= 0xFFFFFFF0UL;
  }
  Test = ~Test + 1;
  if (Save & 1)
    PCIWriteConfigW (Bus, Device, Function, Reg, Save);
  else
    PCIWriteConfigL (Bus, Device, Function, Reg, Save);

  return (Test);
}

/* --------------------------------------------------------------------------*/

#define MAX_PCI_BUSES		 256
#define MAX_PCI_DEVICES 	  32
#define MAX_PCI_FUNCTIONS	   8

#define PCI_MASS_STORAGE	0x01	/* PCI Base Class code */
#define PCI_IDE_CONTROLLER	0x01	/* PCI Sub Class code  */
#define PCI_RAID_CONTROLLER	0x04	/* PCI Sub Class code  */
#define PCI_SATA_CONTROLLER	0x06	/* PCI Sub Class code  */

#define PCIREG_VENDOR_ID	0x00
#define PCIREG_DEVICE_ID	0x02
#define PCIREG_COMMAND		0x04
#define PCIREG_STATUS		0x06
#define PCIREG_REVISION 	0x08
#define PCIREG_PROGIF		0x09
#define PCIREG_CLASS_CODE	0x0A
#define PCIREG_SUBCLASS 	0x0A
#define PCIREG_CLASS		0x0B
#define PCIREG_CACHE_LINE	0x0C
#define PCIREG_LATENCY		0x0D
#define PCIREG_HEADER_TYPE	0x0E
#define PCIREG_PRIMARY_BUS	0x18
#define PCIREG_SECONDARY_BUS	0x19
#define PCIREG_SUBORDINATE_BUS	0x1A
#define PCIREG_IRQ		0x3C
#define PCIREG_INT_PIN		0x3D

#define PCI_HEADER_TYPE_NORMAL	   0
#define PCI_HEADER_TYPE_BRIDGE	   1
#define PCI_HEADER_TYPE_CARDBUS    2

#define PCI_CLASS_BRIDGE_HOST	 0x0600
#define PCI_CLASS_BRIDGE_ISA	 0x0601
#define PCI_CLASS_BRIDGE_PCI	 0x0604
#define PCI_CLASS_BRIDGE_CARDBUS 0x0607

typedef enum { PICmode = 0, APICmode = 1 } tRouterModes;

typedef struct _IRQRoute {
  UCHAR Dev;
  UCHAR Pin;
  UCHAR IRQ;
} tIRQRoute, *pIRQRoute;

typedef struct _Bridge {
  UCHAR  Bus, Dev, Fnc;
  UCHAR  HeaderType;
  USHORT BridgeType;
  SHORT  PriBus;
  UCHAR  SecBus, SubBus;

  ACPI_HANDLE Handle;
  ACPI_NAME   Name;
  unsigned    numRouterEntries[2];
  pIRQRoute   RouterTable[2];
} tBridge, *pBridge;

static tBridge Bridges[MAX_PCI_BUSES];

static pAdapter matchAdapter (UCHAR Bus, UCHAR Dev, UCHAR Fnc) {
  pAdapter p;

  for (p = Adapter; p < (Adapter + numAdapters); p++)
    if ((p->Bus == Bus) && (p->Dev == Dev) && (p->Fnc == Fnc))
      return (p);
  return (NULL);
}

static void HandlePCIFunction (UCHAR Bus, UCHAR Dev, UCHAR Fnc, USHORT Class) {
  union {
    struct {
      UCHAR Sub, Base;
    } Cls;
    USHORT Class;
  } ClassCode;

  ClassCode.Class = Class;

  if (ClassCode.Cls.Base == PCI_MASS_STORAGE) {
    int k;
    pAdapterTable p = AdapterTable;
    struct {
      USHORT Vendor, Device;
    } ChipID;
    UCHAR ProgIF, Revision, IRQ;

    PCIReadConfigB (Bus, Dev, Fnc, PCIREG_REVISION, &Revision);
    PCIReadConfigB (Bus, Dev, Fnc, PCIREG_PROGIF,   &ProgIF);
    PCIReadConfigB (Bus, Dev, Fnc, PCIREG_IRQ,	    &IRQ);

    if (ClassCode.Cls.Sub != PCI_IDE_CONTROLLER) ProgIF = 0x85;

    PCIReadConfigL (Bus, Dev, Fnc, PCIREG_VENDOR_ID, (PULONG)&ChipID);
    for (k = 0; k < (sizeof (AdapterTable) / sizeof (AdapterTable[0])) - 2; k++, p++) {
      if ((p->VendorId == ChipID.Vendor) && (p->DeviceId == ChipID.Device))
	break;
    }
    if ((p->VendorId == 0) && !(ProgIF & 0x80)) p++;

    if ((p->VendorId != 0) ||
	((p->VendorId == 0) &&
	 (ClassCode.Cls.Sub == PCI_IDE_CONTROLLER) &&
	!(ProgIF & 0x05))) {

      if (numAdapters < MAX_NUM_ADAPTERS) {
	pAdapter q = Adapter + numAdapters++;

	q->Bus = Bus;
	q->Dev = Dev;
	q->Fnc = Fnc & 7;
	q->Rev = Revision;
	q->ProgIF = ProgIF;
	q->Vendor = ChipID.Vendor;
	q->Device = ChipID.Device;
	q->PCIIRQ  = (IRQ == 0xFF) ? 0 : IRQ;
	q->Adapter = p;
      }
    }
  }
}

static void ScanPCIDevices (void) {
  UCHAR  Bus, Dev, Fnc, maxFnc;
  USHORT Class;

  /* scan all PCI buses beginning with bus 0
   * don't assume that the BIOS has found the correct number of buses
   *
   * keep all bridges to create a spanning tree of the PCI buses
   */

  for (Bus = 0; Bus <= BusCount; Bus++) {

    // on each bus: enumerate all possible PCI devices

    for (Dev = 0; Dev < MAX_PCI_DEVICES; Dev++) {

      /* enumerate all functions of a given PCI device
       * attention: some old hardware is lying at us here!
       */

      maxFnc = MAX_PCI_FUNCTIONS;  // this is the default

      for (Fnc = 0; Fnc < maxFnc; Fnc++) {
	UCHAR HeaderType;

	PCIReadConfigW (Bus, Dev, Fnc, PCIREG_CLASS_CODE, &Class);
	PCIReadConfigB (Bus, Dev, Fnc, PCIREG_HEADER_TYPE, &HeaderType);

	/* check for invalid values */

	if (Class == 0) continue;
	if (Class == 0xFFFF) continue;
	if (HeaderType == 0xFF) continue;

if (verboseScan)
  printf ("Bus:%d Dev:%2d Fnc:%d   Class:%04X   Hdr:%02X\n", Bus, Dev, Fnc, Class, HeaderType);

	if ((0 == Fnc) && !(HeaderType & 0x80)) {

	  /* this is a single function device
	   * enumerate function 0 only
	   * but some old PCI-ISA-bridges have broken PCI config spaces !
	   */

	  maxFnc = 1;
	  if (Class == PCI_CLASS_BRIDGE_ISA) maxFnc++;
	}

	HeaderType &= ~0x80;

	if (HeaderType) {  // this is a bridge!
	  if (HeaderType <= PCI_HEADER_TYPE_CARDBUS) {
	    UCHAR PriBus, SecBus, SubBus;
	    pBridge pB;

	    PCIReadConfigB (Bus, Dev, Fnc, PCIREG_PRIMARY_BUS,	   &PriBus);
	    PCIReadConfigB (Bus, Dev, Fnc, PCIREG_SECONDARY_BUS,   &SecBus);
	    PCIReadConfigB (Bus, Dev, Fnc, PCIREG_SUBORDINATE_BUS, &SubBus);

if (verboseScan)
  printf ("   Bridge %d->%d..%d\n", PriBus, SecBus, SubBus);

	    /* check for invalid values */

	    if ((PriBus == 0xFF) || (PriBus != Bus)) continue;
	    if (SecBus == 0) continue;
	    if (SubBus == 0) continue;

	    pB = Bridges + SecBus;
	    pB->Bus = Bus;
	    pB->Dev = Dev;
	    pB->Fnc = Fnc;
	    pB->HeaderType = HeaderType;
	    pB->BridgeType = Class;
	    pB->PriBus = Bus;
	    pB->SecBus = SecBus;
	    pB->SubBus = SubBus;

	    /* adjust number of PCI buses according to found bridge setup */

	    if (BusCount < SubBus) BusCount = SubBus;
	  }
	} else {

	  /* HOST->PCI bridges have header type 0! */

	  if (Class == PCI_CLASS_BRIDGE_HOST) {
	    USHORT  Command;
	    pBridge pB;

	    PCIReadConfigW (Bus, Dev, Fnc, PCIREG_COMMAND, &Command);
	    if ((Command == 0) || (Command == 0xFFFF)) continue;

	    /* this is an active HOST->PCI bridge */

	    pB = Bridges + Bus;

	    if (!pB->BridgeType) {
	      pB->Bus = Bus;
	      pB->Dev = Dev;
	      pB->Fnc = Fnc;
	      pB->HeaderType = HeaderType;
	      pB->BridgeType = Class;
	      pB->PriBus = -1;
	      pB->SecBus = Bus;
	      pB->SubBus = 0;
	    }
	  } else {
	    HandlePCIFunction (Bus, Dev, Fnc, Class);
	  }
	}
      }
    }
  }
}

/* --------------------------------------------------------------------------*/

APIRET CallAcpiCA (void *Packet, ULONG PacketSize, ULONG Function) {
  APIRET rc;
  rc = DosDevIOCtl (hACPI, 0x88, Function, NULL, 0, NULL, Packet, PacketSize, &PacketSize);
  return rc;
}

ACPI_STATUS AcpiGetType (
  ACPI_HANDLE	    Object,
  ACPI_OBJECT_TYPE *OutType)
{
  GTyPar Packet = {Object, OutType, AE_ERROR};

  CallAcpiCA (&Packet, sizeof (Packet), GETTYPE_FUNCTION);
  return (Packet.Status);
}

ACPI_STATUS AcpiGetObjectInfo (
  ACPI_HANDLE  Handle,
  ACPI_BUFFER *ReturnBuffer)
{
  GOIPar Packet = {Handle, ReturnBuffer, AE_ERROR};

  CallAcpiCA (&Packet, sizeof (Packet), GETOBJECTINFO_FUNCTION);
  return (Packet.Status);
}

ACPI_STATUS AcpiGetNextObject (
  ACPI_OBJECT_TYPE Type,
  ACPI_HANDLE	   Parent,
  ACPI_HANDLE	   Child,
  ACPI_HANDLE	  *OutHandle)
{
  GNOPar Packet = {Type, Parent, Child, OutHandle, AE_ERROR};

  CallAcpiCA (&Packet, sizeof (Packet), GETNEXTOBJECT_FUNCTION);
  return (Packet.Status);
}

ACPI_STATUS AcpiGetParent (
  ACPI_HANDLE  Object,
  ACPI_HANDLE *OutHandle)
{
  GTpPar Packet = {Object, OutHandle, AE_ERROR};

  CallAcpiCA (&Packet, sizeof (Packet), GETPARENT_FUNCTION);
  return (Packet.Status);
}

ACPI_STATUS AcpiGetName (
  ACPI_HANDLE  Handle,
  UINT32       NameType,
  ACPI_BUFFER *RetPathPtr)
{
  GNPar Packet = {Handle, NameType, RetPathPtr, AE_ERROR};
  CallAcpiCA (&Packet, sizeof (Packet), GETNAME_FUNCTION);
  return (Packet.Status);
}

ACPI_STATUS AcpiEvaluateObject (
  ACPI_HANDLE	    Object,
  ACPI_STRING	    Pathname,
  ACPI_OBJECT_LIST *ParameterObjects,
  ACPI_BUFFER	   *ReturnObjectBuffer)
{
  EvaPar Packet = {Object, Pathname, ParameterObjects, ReturnObjectBuffer, AE_ERROR};
  CallAcpiCA (&Packet, sizeof (Packet), EVALUATE_FUNCTION);
  return (Packet.Status);
}

/* --------------------------------------------------------------------------*/

#if defined (IBMVAC3)
#define Val32(x)  ((x).ulLo)
#else
#define Val32(x)  ((ULONG)(x))
#endif

static UCHAR queryPCIIntPin (UCHAR Bus, UCHAR Dev, UCHAR Fnc) {
  UCHAR IntPin;

  PCIReadConfigB (Bus, Dev, Fnc, PCIREG_INT_PIN, &IntPin);
  switch (IntPin) {
    case 0x01: IntPin = 0; break;
    case 0x02: IntPin = 1; break;
    case 0x04: IntPin = 2; break;
    case 0x08: IntPin = 3; break;
    default  : IntPin = 0; break;  /* error in PCI INTx info, assume INTA */
  }
  return (IntPin);
}

static int queryIRQforPCIFunction (UCHAR Bus, UCHAR Dev, UCHAR Fnc, tRouterModes Mode) {
  UCHAR IntPin;
  UCHAR IRQ = 0;
  int	PCIBus = Bus;
  pBridge pB;

  IntPin = queryPCIIntPin (Bus, Dev, Fnc);

  while (PCIBus >= 0) {
    pB = Bridges + PCIBus;

    /* if any of the PCI buses on the way up to the host bridge is
     * missing, what to do?
     */

    if (!pB->BridgeType)
      exit;

    if (pB->RouterTable[Mode]) {
      /* this bridge has IRQ router info, look it up */

      pIRQRoute pI;
      for (pI = pB->RouterTable[Mode];
	   pI < (pB->RouterTable[Mode] + pB->numRouterEntries[Mode]);
	   pI++) {
	if ((pI->Dev == Dev) && (pI->Pin == IntPin)) {
	  IRQ = pI->IRQ;
	  PCIBus = -1;
	  break;
	}
      }

    } else {
      /* INT wires are routed to the parent bus
       * update info according to the common rules and retry
       */

      switch (pB->BridgeType) {
	case PCI_CLASS_BRIDGE_HOST :
	  /* what's going on here - we've already reached the root ?
	   * bail out confused ...
	   */
	  break;

	case PCI_CLASS_BRIDGE_CARDBUS :
	  /* let's assume PCI INT routing mode ... */
	  IntPin = queryPCIIntPin (pB->PriBus, pB->Dev, pB->Fnc);
	  break;

	case PCI_CLASS_BRIDGE_PCI :
	  /* assume PCI INT swizzle as per PCI-to-PCI bridge spec. ECN */
	  IntPin = (IntPin + Dev) & 3;
	  break;
      }

      PCIBus = pB->PriBus;
      Dev    = pB->Dev;
      Fnc    = pB->Fnc;
    }
  }

  return (IRQ);
}

static int BridgeFromPCIAddress (UCHAR Bus, UCHAR Dev, UCHAR Fnc) {
  int i;

  for (i = 0; i <= BusCount; i++) {
    pBridge pB = Bridges + i;
    if ((Bus == pB->Bus) && (Dev == pB->Dev) && (Fnc == pB->Fnc) && (pB->BridgeType != 0))
      return (i);
  }
  return (-1);
}

static UCHAR EvaluateMethod (ACPI_HANDLE Handle, char *MethodName, ACPI_BUFFER *Buffer) {
  Buffer->Length  = 0;
  Buffer->Pointer = NULL;

  if (AE_BUFFER_OVERFLOW == AcpiEvaluateObject (Handle, MethodName, NULL, Buffer)) {
    Buffer->Pointer = malloc (Buffer->Length);
    if (Buffer->Pointer != NULL)
      return (ACPI_SUCCESS (AcpiEvaluateObject (Handle, MethodName, NULL, Buffer)));
  }
  return (FALSE);
}

static UCHAR EvaluateCRSforBus (ACPI_HANDLE Handle, UCHAR *minBus, UCHAR *maxBus) {
  ACPI_BUFFER ResultBuffer;
  UCHAR rc = FALSE;

  if (EvaluateMethod (Handle, METHOD_NAME__CRS, &ResultBuffer)) {
    /* _CRS evaluates to a buffer pointing to the resource byte stream */

    ACPI_OBJECT *pO = (ACPI_OBJECT *)ResultBuffer.Pointer;
    if (pO->Type == ACPI_TYPE_BUFFER) {
      char *Resource = (char *)pO->Buffer.Pointer;
      char *RsrcEnd  = (char *)pO->Buffer.Pointer + pO->Buffer.Length;
      char DescriptorType;

      for (; Resource && (Resource < RsrcEnd);) {
	DescriptorType = *Resource;

	if (DescriptorType == 0x79) { /* end tag */
	  break;

	} else if (DescriptorType == 0x88) { /* WORD address tag */
	  AML_RESOURCE_ADDRESS16 *r = (AML_RESOURCE_ADDRESS16 *)Resource;
	  if (r->ResourceType == 2) { /* BusNumber */
	    rc = (r->Minimum <= 0xFF);
	    if (rc) {
	      *minBus = r->Minimum;
	      rc = ((r->Maximum <= 0xFF) && (r->Minimum <= r->Maximum));
	      if (rc) {
		*maxBus = r->Maximum;
		break;
	      }
	    }
	  }

	} /* we don't care about other address space types because the
	   * 'WordBusNumber' AML macro expands to the 16bit type
	   */

	if (DescriptorType & 0x80)
	  Resource += ((AML_RESOURCE_LARGE_HEADER *)Resource)->ResourceLength
		    + sizeof (AML_RESOURCE_LARGE_HEADER);
	else
	  Resource += (DescriptorType & 0x7)
		    + sizeof (AML_RESOURCE_SMALL_HEADER);
      }
    }
  }
  free (ResultBuffer.Pointer);
  return (rc);
}

static UCHAR EvaluateRSforSingleIRQ (ACPI_HANDLE Handle, char getPRS) {
  ACPI_BUFFER ResultBuffer;
  char *MethodName = getPRS ? METHOD_NAME__PRS : METHOD_NAME__CRS;
  int IRQ = 0;

  if (EvaluateMethod (Handle, MethodName, &ResultBuffer)) {
    /* _CRS evaluates to a buffer pointing to the resource byte stream */

    ACPI_OBJECT *pO = (ACPI_OBJECT *)ResultBuffer.Pointer;
    if (pO->Type == ACPI_TYPE_BUFFER) {
      char *Resource = (char *)pO->Buffer.Pointer;
      char *RsrcEnd  = (char *)pO->Buffer.Pointer + pO->Buffer.Length;
      char DescriptorType;

      for (; Resource && (Resource < RsrcEnd);) {
	DescriptorType = *Resource;

	if (DescriptorType == 0x79) { /* end tag */
	  break;

	} else if ((DescriptorType == 0x22) || (DescriptorType == 0x23)) { /* IRQ tag */
	  AML_RESOURCE_IRQ *r = (AML_RESOURCE_IRQ *)Resource;
	  USHORT Bits = r->IrqMask;

	  if (Bits != 0) {
	    for (IRQ = 0; IRQ <= 15; IRQ++, Bits >>= 1)
	      if (Bits & 1) break;
	  }
	  if (getPRS && (Bits > 1))
	    /* _PRS returned more than one IRQ value */
	    IRQ = 0;

	} else if (DescriptorType == 0x89) { /* extended IRQ tag */
	  AML_RESOURCE_EXTENDED_IRQ *r = (AML_RESOURCE_EXTENDED_IRQ *)Resource;
	  if (r->InterruptCount >= 1)
	    IRQ = r->Interrupts[0];

	  if (getPRS && (r->InterruptCount > 1))
	    /* _PRS returned more than one IRQ value */
	    IRQ = 0;

	  /* we don't handle resource source & index here - should we? */
	}

	if (DescriptorType & 0x80)
	  Resource += ((AML_RESOURCE_LARGE_HEADER *)Resource)->ResourceLength
		    + sizeof (AML_RESOURCE_LARGE_HEADER);
	else
	  Resource += (DescriptorType & 0x7)
		    + sizeof (AML_RESOURCE_SMALL_HEADER);
      }
    }
  }
  free (ResultBuffer.Pointer);
  return (IRQ);
}

static void EvaluatePRTElement (pIRQRoute pI, ACPI_OBJECT *pO) {
  if ((pO->Type == ACPI_TYPE_PACKAGE) &&
      (pO->Package.Count >= 4)) {

    char  rc;
    UCHAR Dev, Fnc, Pin, IRQ;

    /* this may be a PRT IRQ package element
     * evaluate its fields
     */

    ACPI_OBJECT *F = pO->Package.Elements;

    /* field 0: address on the parent PCI bus */

    rc = (F->Type == ACPI_TYPE_INTEGER);
    if (rc) {
      Dev = Val32 (F->Integer.Value) >> 16;
      rc = (Dev <= MAX_PCI_DEVICES);
      if (rc)
	pI->Dev = Dev;
    }
    F++;

    /* field 1: INT pin */

    rc = (F->Type == ACPI_TYPE_INTEGER);
    if (rc) {
      ULONG value = Val32 (F->Integer.Value);
      rc = (value < 0xFF);
      if (rc)
	pI->Pin = Pin = value;
    }
    F++;

    /* field 2: IRQ source, either zero or a reference to an IRQ producer */

    switch (F->Type) {
      case ACPI_TYPE_INTEGER:
	rc = (Val32 (F->Integer.Value) == 0);
	if (rc) {
	  F++;

	  /* field 3: global system interrupt */

	  rc = (F->Type == ACPI_TYPE_INTEGER);
	  if (rc)
	    pI->IRQ = IRQ = Val32 (F->Integer.Value);
	}
	break;

      case ACPI_TYPE_ANY:
	rc = (F->Reference.Handle != NULL);
	if (rc)
	  IRQ = EvaluateRSforSingleIRQ (F->Reference.Handle, FALSE); // try _CRS
	  if (!IRQ)
	    IRQ = EvaluateRSforSingleIRQ (F->Reference.Handle, TRUE); // try _PRS
	  pI->IRQ = IRQ;
	break;

      default: rc = FALSE;
    }
  }
}

static UCHAR setRouterMode (tRouterModes Mode) {
  ACPI_STATUS	   Status;
  ACPI_OBJECT	   Obj;
  ACPI_OBJECT_LIST Params;

  Params.Count	 = 1;
  Params.Pointer = &Obj;
  Obj.Type	 = ACPI_TYPE_INTEGER;

  /* Caution: the _PIC method is optional! If it's not present we may
   * safely (as per spec!) assume PIC mode IRQ routing tables to be returned
   */

#if defined (IBMVAC3)
  Obj.Integer.Value.ulLo = Mode;
  Obj.Integer.Value.ulHi = 0;
#else
  Obj.Integer.Value = 0;
#endif

  Status = AcpiEvaluateObject (NULL, "\\_PIC", &Params, NULL);
  if (Mode == PICmode)
    return ((Status == AE_OK) || (Status == AE_NOT_FOUND));
  else
    return (Status == AE_OK);
}

static void EvaluatePRT (pBridge pB, tRouterModes Mode) {
  pIRQRoute RouterTable = NULL;

  pB->RouterTable[Mode]      = NULL;
  pB->numRouterEntries[Mode] = 0;

  /* advise ACPI to return the desired router tables
   * this may fail for modes other than PIC!
   */

  if (setRouterMode (Mode)) {
    ACPI_BUFFER  ResultBuffer = {0, NULL};
    ACPI_OBJECT *Obj;

    /* evaluate _PRT package into a prellocated buffer of sufficient size
     * check for correct type and non-zero result
     * enumerate table elements
     */

    if ((AE_BUFFER_OVERFLOW == AcpiEvaluateObject (pB->Handle, METHOD_NAME__PRT, NULL, &ResultBuffer)) &&
	((Obj = (ACPI_OBJECT *)(ResultBuffer.Pointer = malloc (ResultBuffer.Length))) != NULL) &&
	(ACPI_SUCCESS (AcpiEvaluateObject (pB->Handle, METHOD_NAME__PRT, NULL, &ResultBuffer))) &&
	(Obj->Type == ACPI_TYPE_PACKAGE) &&
	(Obj->Package.Count > 0) &&
	((RouterTable = (pIRQRoute)calloc (Obj->Package.Count, sizeof (RouterTable[0]))) != NULL)) {

      unsigned i;
      ACPI_OBJECT *PRTElement = Obj->Package.Elements;

      /* evaluate each individual PRT table element */

      for (i = 0; i < Obj->Package.Count; i++)
	EvaluatePRTElement (RouterTable + i, PRTElement + i);

      pB->RouterTable[Mode]	 = RouterTable;
      pB->numRouterEntries[Mode] = i;
    }
    free (Obj);
  }
}

static int hasPRT (ACPI_HANDLE Object) {
  /* check for presence of _PRT member */

  ACPI_STATUS Status;
  ACPI_BUFFER ResultBuffer = {0, NULL};

  Status = AcpiEvaluateObject (Object, METHOD_NAME__PRT, NULL, &ResultBuffer);
  return ((AE_BUFFER_OVERFLOW == Status) || (AE_OK == Status));
}

static int testRootBridge (ACPI_HANDLE Object, ACPI_DEVICE_INFO *Info) {

  /* a PCI root bridge is required to
   *  - have a HID or CID of "PNP0A03"
   *  - have a _PRT member
   *  - have a _ADR member (dev.fnc on root PCI bus!)
   *  - _BBN is optional and can generally safely be assumed as 0 because
   *	there is no support for multiple root PCI buses in OS/2
   *  for more on this see http://www.acpi.info/acpi_faq.htm
   */

  static char BridgePnPID[] = "PNP0A03";
  char	  rc = FALSE;
  UCHAR   Bus, Dev, Fnc, minBus, maxBus;
  pBridge pB;

  /* first check HID if present */

  if (Info->Valid & ACPI_VALID_HID)
    rc = !strncmp (Info->HardwareId.Value, BridgePnPID, sizeof (BridgePnPID)-1);

  /* if no success, try CID instead */

  if (!rc && (Info->Valid & ACPI_VALID_CID)) {
    int i;
    for (i = 0; i < Info->CompatibilityId.Count; i++) {
      rc = !strncmp (Info->CompatibilityId.Id[i].Value,  BridgePnPID, sizeof (BridgePnPID)-1);
      if (rc) break;
    }
  }

  /* if there's no match in both HID and CID bail out */

  if (!rc) return (-1);

  /* next, check for presence of _PRT member */

  if (!hasPRT (Object)) return (-1);

  /* preset some defaults because the following stuff is mostly optional
   * and we are forgiving
   */

  Bus = Dev = Fnc = 0;
  minBus = 0;
  maxBus = MAX_PCI_BUSES;

  if (Info->Valid & ACPI_VALID_ADR) {
    Dev = Val32 (Info->Address) >> 16;
    Fnc = Val32 (Info->Address) & 0xFFFF;
  }

  /* next, get range of PCI bus numbers accessible through this device */

  if (EvaluateCRSforBus (Object, &minBus, &maxBus))
    Bus = minBus;

  {
    /* next, check for presence and value of the optional _BBN member */

    ACPI_STATUS Status;
    ACPI_OBJECT Value;
    ACPI_BUFFER ResultBuffer = {sizeof (Value), &Value};

    Status = AcpiEvaluateObject (Object, METHOD_NAME__BBN, NULL, &ResultBuffer);
    if (ACPI_SUCCESS (Status) && (Value.Type == ACPI_TYPE_INTEGER))
      Bus = Val32 (Value.Integer.Value);
  }

  pB = Bridges + Bus;

  if (pB->BridgeType != PCI_CLASS_BRIDGE_HOST) return (-1);

  pB->SecBus = minBus;
  pB->SubBus = maxBus;
  pB->Handle = Object;
  pB->Name   = Info->Name;

  EvaluatePRT (pB, PICmode);
  EvaluatePRT (pB, APICmode);

  return (Bus);
}

static void TraverseSystemBus (void) {
  ACPI_HANDLE ParentHandle, ChildHandle;
  int Level = 1, MaxDepth = 100;
  int PCIBus	 = -1;
  int nextPCIBus = -1;

  ParentHandle = ACPI_ROOT_OBJECT;
  ChildHandle  = 0;

  /*
   * Traverse the tree of objects until we bubble back up to where we
   * started. When Level is zero, the loop is done because we have
   * bubbled up to (and passed) the original parent handle (StartHandle)
   */

  while (Level > 0) {

    /* Get the next typed object in this scope. Null returned
     * if not found
     */

    if (ACPI_SUCCESS (AcpiGetNextObject (ACPI_TYPE_ANY,
		      ParentHandle, ChildHandle, &ChildHandle))) {

      char examineChilds = TRUE;

      ACPI_OBJECT_TYPE ChildType;
      AcpiGetType (ChildHandle, &ChildType);

      if (ChildType == ACPI_TYPE_DEVICE) {
	/* Found a device object */

	char Data[1024];
	ACPI_BUFFER Buf = {sizeof (Data), Data};
	ACPI_DEVICE_INFO *Info = (ACPI_DEVICE_INFO *)Data;

	if (ACPI_SUCCESS (AcpiGetObjectInfo (ChildHandle, &Buf))) {
	  if (PCIBus < 0) {

	    /* find the PCI root bridges */

	    nextPCIBus = testRootBridge (ChildHandle, Info);

	  } else {
	    /* separate PCI functions and PCI bridges from the rest
	     * get PCI header type
	     */

	    UCHAR HeaderType;
	    UCHAR Dev, Fnc;

	    examineChilds = FALSE;

	    if (Info->Valid & ACPI_VALID_ADR) {
	      Dev = (Val32 (Info->Address) >> 16) & (MAX_PCI_DEVICES - 1);
	      Fnc = Val32 (Info->Address) & (MAX_PCI_FUNCTIONS - 1);

	      PCIReadConfigB (PCIBus, Dev, Fnc, PCIREG_HEADER_TYPE, &HeaderType);
	      HeaderType &= ~0x80;

	      if (HeaderType == 0) {

		/* this is a regular PCI function
		 * check if it's in the set of 'interesting' devices
		 * if so, update the device info accordingly
		 */

		pAdapter pA = matchAdapter (PCIBus, Dev, Fnc);
		if (pA) {
		  pA->Handle = ChildHandle;
		  pA->Name   = Info->Name;
		}

	      } else if (HeaderType <= PCI_HEADER_TYPE_CARDBUS) {

		int Bridge = BridgeFromPCIAddress (PCIBus, Dev, Fnc);
		if (Bridge >= 0) {
		  pBridge pB = Bridges + Bridge;

		  /* this is a viable PCI bridge from the PCI bus scan
		   * update ACPI info
		   * and enumerate childs
		   */

		  pB->Handle = ChildHandle;
		  pB->Name   = Info->Name;
		  if (hasPRT (ChildHandle)) {
		    EvaluatePRT (pB, PICmode);
		    EvaluatePRT (pB, APICmode);
		  }

		  nextPCIBus	= pB->SecBus;
		  examineChilds = TRUE;
		}
	      }
	    }
	  }
	}
      }

      /* enumerate ACPI child objects if appropriate */

      if ((Level < MaxDepth) && examineChilds) {
	if (ACPI_SUCCESS (AcpiGetNextObject (ACPI_TYPE_ANY, ChildHandle, 0, NULL))) {

	  /* There is at least one child of this object, visit the object */
	  Level++;
	  ParentHandle = ChildHandle;
	  ChildHandle  = 0;
	  PCIBus       = nextPCIBus;
	}
      }
    } else {

      /*
       * No more children in this object (AcpiGetNextObject failed),
       * go back upwards in the namespace tree to the object's parent.
       */

      Level--;
      ChildHandle = ParentHandle;
      AcpiGetParent (ParentHandle, &ParentHandle);
      if (PCIBus >= 0)
	PCIBus = Bridges[PCIBus].PriBus;
    }
  }
}

static void ScanACPITree (void) {
  /* we are interested in the PIC mode IRQ routing
   * tell this to the ACPI interpreter
   */

  setRouterMode (PICmode);
  TraverseSystemBus();
}

/* --------------------------------------------------------------------------*/

static void CheckGeneric (pAdapter p) {
  ULONG value;
  UCHAR a, o;
  int i;

  PCIReadConfigL (p->Bus, p->Dev, p->Fnc, 4, &value);
  if (!(value & 4)) return;

  PCIReadConfigL (p->Bus, p->Dev, p->Fnc, 0x20, &value);
  value &= 0xFFFC;
  if (value == 0) return;

  ReadBMIFArea (value, 0, 16, BMData);
  a = 0xFF;
  o = 0;
  for (i = 0; i < 16; i++) {
    a &= BMData[i];
    o |= BMData[i];
  }
  if ((a == 0xFF) || (o == 0)) return;

  printf ("It looks like the BIOS has initialized the busmaster properly,\n"
	  "you may try the driver with the /GBM option.\n");
}

static void GetATP867Data (pAdapter p) {
  int Len = 0x40;
  ULONG Port;

  PCIReadConfigL (p->Bus, p->Dev, p->Fnc, 0x10, &Port);
  Port &= 0xFFFC;

  ReadBMIFArea (Port, 0x40, Len, BMData);
  printf ("Host Registers:\n");
  DumpBMIFArea (0x40, Len, BMData);
}

static void GetPDCData (pAdapter p) {
  int rc;
  int i;
  UCHAR *q;
  int Len = 0x30;
  ULONG Port;

  PCIReadConfigL (p->Bus, p->Dev, p->Fnc, 0x20, &Port);
  Port &= 0xFFFC;

  q = BMData;
  for (i = 0; i < Len; i++)
    *(q++) = ReadPortPDC (Port | 1, i);

  printf ("Channel 0 Registers:\n");
  DumpBMIFArea (0, Len, BMData);

  q = BMData;
  for (i = 0; i < Len; i++)
    *(q++) = ReadPortPDC (Port | 9, i);

  printf ("Channel 1 Registers:\n");
  DumpBMIFArea (0, Len, BMData);
}

static void GetPDCDataSATA (pAdapter p) {
  int Len = 0x30;
  ULONG Port;

  PCIReadConfigL (p->Bus, p->Dev, p->Fnc, 0x18, &Port);
  Port &= 0xFFFC;

  ReadBMIFArea (Port, 0x40, Len, BMData);
  printf ("Host Registers:\n");
  DumpBMIFArea (0x40, Len, BMData);
}

static void FindSB (pAdapter p) {
  UCHAR BusB, DevB, FncB, RevB;
  int rc;
  struct {
    USHORT Vendor, Device;
  } ChipID;

  BusB = p->Bus;
  DevB = p->Dev;
  FncB = 0;

  PCIReadConfigL (BusB, DevB, FncB, 0, (PULONG)&ChipID);
  PCIReadConfigB (BusB, DevB, FncB, 0x08, &RevB);
  printf ("South bridge (%04X/%04X rev %02X) on %d:%d.%d\n",
	   ChipID.Vendor, ChipID.Device, RevB,
	   BusB, DevB, FncB);
}

static void FindViaSB (pAdapter p) {
  UCHAR BusB, DevB, FncB, RevB;
  int rc;
  struct {
    USHORT Vendor, Device;
  } ChipID;

  BusB = p->Bus;
  DevB = p->Dev;
  if (p->Fnc)
    FncB = p->Fnc - 1;
  else
    FncB = 0;

  for (; DevB < 32; DevB++) {
    UCHAR Class;

    PCIReadConfigB (BusB, DevB, FncB, 0x0B, &Class);
    if (6 == Class) break;
  }

  PCIReadConfigL (BusB, DevB, FncB, 0, (PULONG)&ChipID);
  PCIReadConfigB (BusB, DevB, FncB, 0x08, &RevB);
  printf ("South bridge (%04X/%04X rev %02X) on %d:%d.%d\n",
	   ChipID.Vendor, ChipID.Device, RevB,
	   BusB, DevB, FncB);
}

static void FindSwSB (pAdapter p) {
  UCHAR BusB, DevB, FncB, RevB;
  int rc;
  struct {
    USHORT Vendor, Device;
  } ChipID;

  BusB = p->Bus;
  DevB = p->Dev;
  FncB = 0;

  PCIReadConfigL (BusB, DevB, FncB, 0, (PULONG)&ChipID);
  PCIReadConfigB (BusB, DevB, FncB, 0x08, &RevB);
  printf ("South bridge (%04X/%04X rev %02X) on %d:%d.%d\n",
	   ChipID.Vendor, ChipID.Device, RevB,
	   BusB, DevB, FncB);

  ReadPCIConfigArea (BusB, DevB, FncB, &PCIConfigArea);
  DumpPCIConfigArea (&PCIConfigArea);
}

static void FindHB (pAdapter p) {
  UCHAR BusB, DevB, FncB, RevB;
  int rc;
  struct {
    USHORT Vendor, Device;
  } ChipID;

  BusB = p->Bus;
  DevB = 0;
  FncB = 0;

  PCIReadConfigL (BusB, DevB, FncB, 0, (PULONG)&ChipID);
  PCIReadConfigB (BusB, DevB, FncB, 0x08, &RevB);
  printf ("Host bridge (%04X/%04X rev %02X) on %d:%d.%d\n",
	   ChipID.Vendor, ChipID.Device, RevB,
	   BusB, DevB, FncB);

  BusB = p->Bus;
  DevB = p->Dev;
  FncB = 0;

  if (DevB > 0) {
    PCIReadConfigL (BusB, DevB, FncB, 0, (PULONG)&ChipID);
    PCIReadConfigB (BusB, DevB, FncB, 0x08, &RevB);
    printf ("South bridge (%04X/%04X rev %02X) on %d:%d.%d\n",
	     ChipID.Vendor, ChipID.Device, RevB,
	     BusB, DevB, FncB);
  }
}

static void FindOptiSB (pAdapter p) {
  UCHAR BusB, DevB, FncB, RevB;
  int rc;
  struct {
    USHORT Vendor, Device;
  } ChipID;
  USHORT Port;
  UCHAR save;

  Port = 0x1F0;

  printf ("Control 0:");
  save = ReadPortOpti (Port | 6);
  WritePortOpti (Port | 6, save & ~1);
  printf (" r0 %02X", ReadPortOpti (Port | 0));
  printf (", w0 %02X", ReadPortOpti (Port | 1));
  WritePortOpti (Port | 6, save | 1);
  printf (", r1 %02X", ReadPortOpti (Port | 0));
  printf (", w1 %02X", ReadPortOpti (Port | 1));
  WritePortOpti (Port | 6, save & ~1);
  printf (", c %02X", ReadPortOpti (Port | 3));
  printf (", s %02X", ReadPortOpti (Port | 5));
  printf (", m %02X\n", save);
  WritePortOpti (Port | 6, save);

  Port = 0x170;

  printf ("Control 1:");
  save = ReadPortOpti (Port | 6);
  WritePortOpti (Port | 6, save & ~1);
  printf (" r0 %02X", ReadPortOpti (Port | 0));
  printf (", w0 %02X", ReadPortOpti (Port | 1));
  WritePortOpti (Port | 6, save | 1);
  printf (", r1 %02X", ReadPortOpti (Port | 0));
  printf (", w1 %02X", ReadPortOpti (Port | 1));
  WritePortOpti (Port | 6, save & ~1);
  printf (", c %02X", ReadPortOpti (Port | 3));
  printf (", s %02X", ReadPortOpti (Port | 5));
  printf (", m %02X\n", save);
  WritePortOpti (Port | 6, save);

  BusB = p->Bus;
  DevB = 1;
  FncB = 0;

  PCIReadConfigL (BusB, DevB, FncB, 0, (PULONG)&ChipID);
  PCIReadConfigB (BusB, DevB, FncB, 0x08, &RevB);
  printf ("\nSouth bridge (%04X/%04X rev %02X) on %d:%d.%d\n",
	   ChipID.Vendor, ChipID.Device, RevB,
	   BusB, DevB, FncB);

  ReadPCIConfigArea (BusB, DevB, FncB, &PCIConfigArea);
  DumpPCIConfigArea (&PCIConfigArea);
}

#define DEVICEIDALI1533 0x1533

static void FindALiSB (pAdapter p) {
  UCHAR BusB, DevB, FncB, VerB, RevB;
  int rc;
  struct {
    USHORT Vendor, Device;
  } ChipID;

  rc = PCIFindDevice (DEVICEIDALI1533, VENDORIDALI, 0, &BusB, &DevB, &FncB);
  if (rc == 0) {
    PCIReadConfigL (BusB, DevB, FncB, 0, (PULONG)&ChipID);
    PCIReadConfigB (BusB, DevB, FncB, 0x08, &RevB);
    PCIReadConfigB (BusB, DevB, FncB, 0x5E, &VerB);
    printf ("ALI15XX south bridge (%04X/%04X rev %02X) on %d:%d.%d\n",
	     ChipID.Vendor, ChipID.Device, RevB,
	     BusB, DevB, FncB);

    printf ("54:  %02X\n", VerB);
    VerB &= 0x1E;
    if ((p->Rev == 0xC1) && (RevB == 0xC3) && (VerB == 0x12))
      printf ("Flaky 1543C-E south bridge, no UDMA on WDC drives.\n");
  }
  if (p->Rev <= 0xC1)
    printf ("No busmaster DMA writes with ATAPI devices.\n");
}

static void FindPiixSB (pAdapter p) {
  UCHAR BusB, DevB, FncB, RevB;
  int rc;
  struct {
    USHORT Vendor, Device;
  } ChipID;

  BusB = p->Bus;
  DevB = p->Dev;
  FncB = p->Fnc - 1;

  PCIReadConfigL (BusB, DevB, FncB, 0, (PULONG)&ChipID);
  PCIReadConfigB (BusB, DevB, FncB, 0x08, &RevB);
  printf ("South bridge (%04X/%04X rev %02X) on %d:%d.%d\n",
	   ChipID.Vendor, ChipID.Device, RevB,
	   BusB, DevB, FncB);
  PCIReadConfigB (BusB, DevB, FncB, 0xB1, &RevB);
  printf ("B1:  %02X\n", RevB);
}

static ULONG GetBAR5 (pAdapter p, ULONG *Len, BOOL doDump) {
  ULONG SCRPorts;

  if (PCIConfigArea.BaseAddr[5] & 1)
    SCRPorts = PCIConfigArea.BaseAddr[5] & 0x0000FFFC;
  else
    SCRPorts = PCIConfigArea.BaseAddr[5] & 0xFFFFFFF0;

  if (!SCRPorts) {
    printf ("SATA control registers inaccessible\n");
    *Len = 0;
    return 0;
  }

  *Len = GetRangeSize (p->Bus, p->Dev, p->Fnc, 0x24);

  printf ("BAR5:\n");
  if (PCIConfigArea.BaseAddr[5] & 1) {
    if (doDump) {
      ReadBMIFArea (SCRPorts, 0, *Len, BMData);
      DumpBMIFArea (0x00, *Len, BMData);
    }
  } else {
    SCRPorts = MapPhysicalToLinear (SCRPorts, *Len);
    if (doDump) DumpBMIFArea (0x00, *Len, (char *)SCRPorts);
  }

  return SCRPorts;
}

static ULONG GetBAR3 (pAdapter p, ULONG *Len) {
  ULONG SCRPorts;

  if (PCIConfigArea.BaseAddr[3] & 1)
    SCRPorts = PCIConfigArea.BaseAddr[3] & 0x0000FFFC;
  else
    SCRPorts = PCIConfigArea.BaseAddr[3] & 0xFFFFFFF0;

  if (!SCRPorts) {
    printf ("SATA control registers inaccessible\n");
    *Len = 0;
    return 0;
  }

  *Len = GetRangeSize (p->Bus, p->Dev, p->Fnc, 0x1C);

  printf ("BAR3:\n");
  if (PCIConfigArea.BaseAddr[3] & 1) {
    ReadBMIFArea (SCRPorts, 0, *Len, BMData);
    DumpBMIFArea (0x00, *Len, BMData);
  } else {
    SCRPorts = MapPhysicalToLinear (SCRPorts, *Len);
    DumpBMIFArea (0x00, 0x100, (char *)SCRPorts);
  }

  return SCRPorts;
}

static void GetViaSCR (pAdapter p) {
  int i, j;
  ULONG *s;
  ULONG SCRPorts, q;
  ULONG Len;
  UCHAR PortCtrl;
  ULONG SCRInc, Ports;

  SCRPorts = GetBAR5 (p, &Len, TRUE);
  if (!SCRPorts) {
    printf ("SATA PHY control registers inaccessible\n");
    return;
  }

  PCIReadConfigB (p->Bus, p->Dev, p->Fnc, 0x49, &PortCtrl);
  printf ("\nPHY configuration: ");
  switch ((PortCtrl & 0x60) >> 5) {
    case 0: Ports = 2; SCRInc = 0x80; printf ("2xSATA (master only)\n"); break;
    case 1: Ports = 2; SCRInc = 0x40; printf ("2xSATA (master/slave) & 1xPATA\n"); break;
    case 2: Ports = 4; SCRInc = 0x40; printf ("4xSATA (?)\n"); break;
    case 3: Ports = 4; SCRInc = 0x40; printf ("4xSATA (master/slave)\n"); break;
  }

  if (VIAspecial) {
    printf ("Port 0x49 before: %02X", PortCtrl);
    PortCtrl ^= ((VIAspecial & 3) << 5);
    printf (" -> after: %02X\n", PortCtrl);
    PCIWriteConfigB (p->Bus, p->Dev, p->Fnc, 0x49, PortCtrl);
  }

  for (i = 0; i < Ports; i++) {
    s = (PULONG)&SCR;
    q = SCRPorts;
    for (j = 0; j < (sizeof (tSCR) / sizeof (ULONG)); j++, q += sizeof (ULONG)) {
      *(s++) = (q < 0x10000) ? in32 (q) : *(PULONG)q;
      if (clearSATAStatus && (j == 1))
	if (q < 0x10000) out32 (q, -1); else *(PULONG)q = -1;
    }
    DumpSCR (i, 3);

    SCRPorts += SCRInc;
  }
}

static void GetNvidiaSCR (pAdapter p) {
  int i, j;
  ULONG *s;
  ULONG SCRPorts, q;
  ULONG Len;

  SCRPorts = GetBAR5 (p, &Len, TRUE);
  if (!SCRPorts) {
    printf ("SATA PHY control registers inaccessible\n");
    return;
  }

  for (i = 0; i < 2; i++) {
    s = (PULONG)&SCR;
    q = SCRPorts;
    for (j = 0; j < (sizeof (tSCR) / sizeof (ULONG)); j++, q += sizeof (ULONG)) {
      *(s++) = (q < 0x10000) ? in32 (q) : *(PULONG)q;
      if (clearSATAStatus && (j == 1))
	if (q < 0x10000) out32 (q, -1); else *(PULONG)q = -1;
    }
    DumpSCR (i, 3);

    SCRPorts += 0x40;
  }
}

static void GetSiSSCR (pAdapter p) {
  int i, j;
  ULONG *s;
  ULONG SCRPorts, q;
  ULONG Len;
  ULONG Cfg;
  USHORT DID;
  UCHAR PortCtrl;
  ULONG SCRInc, Ports;

  PCIReadConfigL (p->Bus, p->Dev, p->Fnc, 0x54, &Cfg);
  if (Cfg & (1 << 26))
    SCRPorts = GetBAR5 (p, &Len, TRUE);
  else
    SCRPorts = 0xC0;
  if (!SCRPorts) {
    printf ("SATA PHY control registers inaccessible\n");
    return;
  }

  PCIReadConfigB (p->Bus, p->Dev, p->Fnc, 0x90, &PortCtrl);
  if (PCIConfigArea.DeviceID == 0x182) PortCtrl = 0x20;
  printf ("\nPHY configuration: ");
  switch ((PortCtrl & 0x30) >> 4) {
    case 0: Ports = 2; SCRInc = 0x40; printf ("2xSATA (master only)\n"); break;
    case 3: Ports = 2; SCRInc = 0x20; printf ("2xSATA (master/slave) & 1xPATA primary\n"); break;
    case 1: Ports = 2; SCRInc = 0x20; printf ("2xSATA (master/slave) & 1xPATA secondary\n"); break;
    case 2: Ports = 4; SCRInc = 0x20; printf ("4xSATA (master/slave)\n"); break;
  }

  if (SISspecial) {
    printf ("Port 0x90 before: %02X", PortCtrl);
    PortCtrl ^= ((SISspecial & 3) << 4);
    printf (" -> after: %02X\n", PortCtrl);
    PCIWriteConfigB (p->Bus, p->Dev, p->Fnc, 0x90, PortCtrl);
  }

  for (i = 0; i < Ports; i++) {
    s = (PULONG)&SCR;
    q = SCRPorts;
    if (SCRPorts < 0x100) {
      for (j = 0; j < (sizeof (tSCR) / sizeof (ULONG)); j++, s++, q += sizeof (ULONG)) {
	PCIReadConfigL (p->Bus, p->Dev, p->Fnc, q, s);
	if (clearSATAStatus && (j == 1))
	  PCIWriteConfigL (p->Bus, p->Dev, p->Fnc, q, (ULONG)-1);
      }
      SCRPorts += 0x10;
    } else {
      for (j = 0; j < (sizeof (tSCR) / sizeof (ULONG)); j++, q += sizeof (ULONG)) {
	*(s++) = (q < 0x10000) ? in32 (q) : *(PULONG)q;
	if (clearSATAStatus && (j == 1))
	  if (q < 0x10000) out32 (q, -1); else *(PULONG)q = -1;
      }
      SCRPorts += SCRInc;
    }
    DumpSCR (i, 3);
  }
}

static void GetSiISCR (pAdapter p) {
  int i, j;
  ULONG *SCRPorts, *q;
  ULONG Len;

  SCRPorts = (PULONG) GetBAR5 (p, &Len, FALSE);
  if (!SCRPorts) {
    printf ("SATA PHY control registers inaccessible\n");
    return;
  }

  for (i = Len, j = 0; i > 0; i -= 0x200, j += 0x200) {
    DumpBMIFArea (j + 0x00, 0x020, j + 0x00 + (char *)SCRPorts);
    DumpBMIFArea (j + 0x40, 0x010, j + 0x40 + (char *)SCRPorts);
    DumpBMIFArea (j + 0x90, 0x030, j + 0x90 + (char *)SCRPorts);
    DumpBMIFArea (j + 0xD0, 0x130, j + 0xD0 + (char *)SCRPorts);
  }

  i = 0;
  for (; Len > 0; Len -= 0x200) {
    SCRPorts += 0x100 / sizeof (ULONG);
    for (j = 0; j < 2; j++) {
      q = SCRPorts;
      SCR.Control = *(q++);
      SCR.Status  = *(q++);
      SCR.Error   = *q;
      if (clearSATAStatus) *q = -1;
      DumpSCR (i++, 3);

      SCRPorts += 0x80 / sizeof (ULONG);
    }
  }
}

static void GetInitioSCR (pAdapter p) {
  int i, j;
  ULONG *SCRPorts, *q;
  ULONG Len;
  USHORT GCTRL;

  SCRPorts = (PULONG) GetBAR5 (p, &Len, FALSE);

  // map page 0
  q = (ULONG *)(SCRPorts + 0x7C);
  GCTRL = *(USHORT *)q;
  GCTRL &= ~0xE000;
  *(USHORT *)q = GCTRL;

  memset (BMData, 0, 0x100);
  memcpy (BMData + 0x01, 0x01 + (char *)SCRPorts, 0x3B);
  memcpy (BMData + 0x41, 0x41 + (char *)SCRPorts, 0x3B);
  memcpy (BMData + 0x7C, 0x7C + (char *)SCRPorts, 0x80);

  DumpBMIFArea (0, 0x100, BMData);

  SCRPorts += 0x20 / sizeof (ULONG);
  for (i = 0; i < 2; i++) {
    q = SCRPorts;
    SCR.Status	= q[0];
    if (clearSATAStatus) q[0] = -1;
    SCR.Error	= q[1];
    SCR.Control = q[2];
    SCR.Active	= q[3];
    DumpSCR (i, 4);

    SCRPorts += 0x40 / sizeof (ULONG);
  }
}

static void GetAHCISCR (pAdapter p, ULONG numPorts) {
  int i, j;
  ULONG *s;
  ULONG SCRPorts, q;
  ULONG Len;

  SCRPorts = GetBAR5 (p, &Len, TRUE);
  if (!SCRPorts) {
    printf ("AHCI registers inaccessible\n");
    return;
  }

  s = (PULONG)SCRPorts;
  printf ("AHCI CAP:%08lX GHC:%08lX PI:%08lX\n", s[0], s[1], s[3]);
  if ((s[0] != 0) && (numPorts > (s[0] & 0x1F)))
    numPorts = (s[0] & 0x1F) + 1;

  SCRPorts += 0x128;
  for (i = 0; i < numPorts; i++) {
    s = (PULONG)&SCR;
    q = SCRPorts;
    for (j = 0; j < (sizeof (tSCR) / sizeof (ULONG)); j++, q += sizeof (ULONG)) {
      *(s++) = *(PULONG)q;
      if (clearSATAStatus && (j == 1))
	*(PULONG)q = -1;
    }
    DumpSCR (i, 3);

    SCRPorts += 0x80;
  }
}

static void GetIntelSCR (pAdapter p) {
  GetAHCISCR (p, 4);
}

static void GetJMicronSCR (pAdapter p) {
  GetAHCISCR (p, 32);
}

static void GetViaAHCISCR (pAdapter p) {
  GetAHCISCR (p, 32);
}

static void GetIxpSCR (pAdapter p) {
  GetAHCISCR (p, 4);
}

static void GetPdcSCR (pAdapter p, ULONG SCRPorts, int numPorts) {
  int i, j;
  ULONG *s;
  ULONG q;
  ULONG Len;

  SCRPorts += 0x400;
  for (i = 0; i < numPorts; i++) {
    s = (PULONG)&SCR;
    q = SCRPorts;
    for (j = 0; j < (sizeof (tSCR) / sizeof (ULONG)); j++, q += sizeof (ULONG)) {
      *(s++) = *(PULONG)q;
      if (clearSATAStatus && (j == 1))
	*(PULONG)q = -1;
    }
    DumpSCR (i, 3);
    printf ("PHY ctl: %08lX  (Marvell specific)\n", SCR.Active);

    SCRPorts += 0x100;
  }
}

static void GetPdcSCR_SATA (pAdapter p) {
  ULONG SCRPorts, Len;

  SCRPorts = GetBAR3 (p, &Len);
  if (!SCRPorts) {
    printf ("SATA PHY control registers inaccessible\n");
    return;
  }

  printf ("Port configuration: 4 SATA ports\n");
  GetPdcSCR (p, SCRPorts, 4);
}

static void GetPdcSCR_SATA2 (pAdapter p) {
  ULONG SCRPorts, Len;

  SCRPorts = GetBAR3 (p, &Len);
  if (!SCRPorts) {
    printf ("SATA PHY control registers inaccessible\n");
    return;
  }

  printf ("Port configuration: 4 SATA ports\n");
  GetPdcSCR (p, SCRPorts, 4);
}

static void GetPdcSCR_Combo (pAdapter p) {
  int	numPorts;
  ULONG SCRPorts, Len;

  SCRPorts = GetBAR3 (p, &Len);
  if (!SCRPorts) {
    printf ("SATA PHY control registers inaccessible\n");
    return;
  }

  numPorts = *(PULONG)(SCRPorts + 0x48);
  printf ("Port configuration: 2 SATA ports + %d PATA ports\n", (numPorts & 0x02 ? 2 : 1));
  GetPdcSCR (p, SCRPorts, 2);
}

static void GetPdcSCR_Combo2 (pAdapter p) {
  ULONG SCRPorts, Len;

  SCRPorts = GetBAR3 (p, &Len);
  if (!SCRPorts) {
    printf ("SATA PHY control registers inaccessible\n");
    return;
  }

  printf ("Port configuration: 2 SATA ports + 1 PATA port\n");
  GetPdcSCR (p, SCRPorts, 2);
}

static void GetALiSCRcommon (pAdapter p, int Port, ULONG q) {
  int j;
  ULONG *s;

  s = (PULONG)&SCR;
  for (j = 0; j < (sizeof (tSCR) / sizeof (ULONG)); j++, s++, q += sizeof (ULONG)) {
    PCIReadConfigL (p->Bus, p->Dev, p->Fnc, q, s);
    if (clearSATAStatus && (j == 1))
      PCIWriteConfigL (p->Bus, p->Dev, p->Fnc, q, (ULONG)-1);
  }
  DumpSCR (Port, 3);
}

static void GetALiSCR5281 (pAdapter p) {
  GetALiSCRcommon (p, 0, 0x60);
  GetALiSCRcommon (p, 1, 0xC0);
}

static void GetALiSCR5287 (pAdapter p) {
  GetALiSCRcommon (p, 0, 0x90);
  GetALiSCRcommon (p, 1, 0xA0);
  GetALiSCRcommon (p, 2, 0xD0);
  GetALiSCRcommon (p, 3, 0xE0);
}

static void GetALiSCR5289 (pAdapter p) {
  GetALiSCRcommon (p, 0, 0x90);
  GetALiSCRcommon (p, 1, 0xA0);
}

static int ReadPCIConfigArea (UCHAR Bus, UCHAR Dev, UCHAR Fnc,
			      pPCIConfigArea pC) {
  int rc = 0;
  UCHAR *p;
  int i;

  p = (UCHAR *)pC;
  for (i = 0; (i < sizeof (tPCIConfigArea)) && (rc == 0); i++)
    rc = PCIReadConfigB (Bus, Dev, Fnc, i, p++);
  return (rc);
}

static void DumpPCIConfigArea (pPCIConfigArea pC) {
  int i, j, l, a;
  UCHAR *p;

  for (l = 255; l > 0; l--)
    if (((UCHAR *)pC)[l] != 0) break;
  l = (l + 15) & ~15;

  p = (UCHAR *)pC;
  for (a = 0; a < l; a += 16) {
    printf ("%02X:  ", a);
    for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++)
	printf ("%02X ", *(p++));
      printf (" ");
    }
    printf ("\n");
  }
}

static void PrintPCIConfigArea (pPCIConfigArea pC) {
  int i;
  ULONG Len;

  printf ("VendorID %04X, DeviceID %04X",
	  pC->VendorID, pC->DeviceID);
  if (*(PULONG)&(pC->SubVendorID) != 00)
    printf (", Subsystem VendorID %04X, DeviceID %04X\n",
	    pC->SubVendorID, pC->SubDeviceID);
  else
    printf ("\n");
  for (i = 0; i < 6; i++)
    if (pC->BaseAddr[i] != 0) {
      Len = GetRangeSize (Bus, Dev, Fnc, 0x10 + 4 * i);
      if (pC->BaseAddr[i] & 1) {
	printf ("IOBase      % 4lXh (Len %lXh)\n",  pC->BaseAddr[i] & 0x0000FFFC, Len);
      } else {
	printf ("MemBase %08lXh (Len %lXh)\n",  pC->BaseAddr[i] & 0xFFFFFFF0, Len);
      }
    }
  if (pC->EPROMBase != 0) {
    Len = GetRangeSize (Bus, Dev, Fnc, 0x30);
    printf ("ROMBase %08lXh (Len %lXh)\n",  pC->EPROMBase & 0xFFFFFC00, Len);
  }
  if (pC->Int)
    printf ("INT%c -> IRQ%0d\n", pC->Int+'@', pC->IRQ);
}

static ULONG in32 (int Reg) {
  APIRET rc;
  struct {
    WORD Register;
    WORD Size;
  } Parm = {0, 4};
  ULONG ParmLen = sizeof (Parm);
  struct {
    ULONG Value;
  } Data;
  ULONG DataLen = sizeof (Data);

  Parm.Register = Reg;

  rc = DosDevIOCtl (hIO, 0x80, 0x41, &Parm, ParmLen, &ParmLen, &Data, DataLen, &DataLen);
  if (rc != 0) {
    return (ReadPort (Reg));
  } else
    return (Data.Value);
}

static void out32 (int Reg, ULONG Val) {
  APIRET rc;
  struct {
    WORD Register;
    WORD Size;
  } Parm = {0, 4};
  ULONG ParmLen = sizeof (Parm);
  struct {
    ULONG Value;
  } Data;
  ULONG DataLen = sizeof (Data);

  Parm.Register = Reg;
  Data.Value	= Val;

  rc = DosDevIOCtl (hIO, 0x80, 0x42, &Parm, ParmLen, &ParmLen, &Data, DataLen, &DataLen);
  if (rc) WritePort (Reg, Val);
}

static int ReadBMIFArea (WORD Adr, WORD Idx, WORD Len, void *pC) {
  int rc = 0;
  ULONG *p;
  int i;

  p = (ULONG *)pC;
  for (i = 0; i < (Len / 4); i++) {
    *(p++) = in32 (Adr + Idx);
    Idx += 4;
  }

  return (rc);
}

static void DumpBMIFArea (WORD Idx, WORD Len, void *pC) {
  int i, j, a;
  UCHAR *p;

  if (!pC) {
    printf ("PROBLEM: registers are inaccessible\n");
    return;
  }

  p = (UCHAR *)pC;
  for (a = 0; a < Len; a += 16, Idx += 16) {
    printf ("%02X:  ", Idx);
    for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++)
	if ((a + i*4 +j) < Len)
	  printf ("%02X ", *(p++));
      printf (" ");
    }
    printf ("\n");
  }
}

static void DumpSCR (int Channel, int numRegs) {
  ULONG val;
  char *p;
  char sError[17];
  int i;

  static char *StatusDET[16] = {
    "no device",
    "device pres., no PHY comm", 0,
    "device pres., PHY comm",
    "PHY offline"
  };
  static char *StatusSPD[16] = {
    "no negotiated speed",
    "1.5Gb/s",
    "3.0Gb/s"
  };
  static char *StatusIPM[16] = {
    "-",
    "interface active",
    "i/f power PARTIAL", 0, 0, 0,
    "i/f power SLUMBER"
  };
  static char *res = "reserved";
  static char ErrorERR[17]  = "rrrrepctrrrrrrmi";
  static char ErrorDIAG[17] = "rrrrrxftshcdbwin";

  printf ("SATA link %d:\n", Channel);
  val = SCR.Status;
  printf ("Status : %08lX - ", val);
  p = StatusDET[val & 0x0F]; if (!p) p = res;
  printf ("%s; ", p);
  val >>= 4;
  p = StatusSPD[val & 0x0F]; if (!p) p = res;
  printf ("%s; ", p);
  val >>= 4;
  p = StatusIPM[val & 0x0F]; if (!p) p = res;
  printf ("%s\n", p);
  val >>= 4;

  val = SCR.Error;
  printf ("Error  : %08lX - ", val);
  memcpy (sError, ErrorERR, sizeof (sError));
  p = sError + 15;
  for (i = 0; i < 16; i++) {
    if (!(val & 1)) *p = '-';
    p--;
    val >>= 1;
  }
  printf ("ERR: %s   ", sError);
  memcpy (sError, ErrorDIAG, sizeof (sError));
  p = sError + 15;
  for (i = 0; i < 16; i++) {
    if (!(val & 1)) *p = '-';
    p--;
    val >>= 1;
  }
  printf ("DIAG: %s\n", sError);

  printf ("Control: %08lX\n", SCR.Control);

  if (numRegs <= 3) return;  // SATA generation 1

  printf ("Active : %08lX\n", SCR.Active);
  if (numRegs <= 4) return;
  printf ("Notify : %08lX\n", SCR.Notification);
}

/* --------------------------------------------------------------------------*/

/* This routine will open up a handle to a device driver with the service    */
/* to map physical ram to a linear address (specifically SSMDD.SYS).	     */

static HFILE hMapDev;  /* Handle to the device driver to do mapping. */

#define PAGE_SIZE 4096	// size of physical page on x86
#define MAX_NUM_MAPPINGS 16

ULONG MapPhysicalToLinear (ULONG PhysicalAddress, ULONG Length) {
  ULONG ActionTaken;
  #pragma pack (1)
  struct {
    ULONG  PhysAddr;
    ULONG  Length;
    PULONG pLinearAddress;
    ULONG  Flag;
    ULONG  unknown;
  } parameter;
  #pragma pack ()
  ULONG PLength = sizeof (parameter);
  ULONG LinearAdr = 0, Offset = 0;
  static APIRET rc = 0;
  int i;

  static struct {
    ULONG PhysicalS, PhysicalE, Linear;
  } MapTable[MAX_NUM_MAPPINGS];
  static ULONG numMappings = 0;

  // scan mapping cache first

  for (i = 0; i < numMappings; i++)
    if ((PhysicalAddress >= MapTable[i].PhysicalS) &&
	((PhysicalAddress + Length) <= MapTable[i].PhysicalE)) { // found match
      return (MapTable[i].Linear + PhysicalAddress - MapTable[i].PhysicalS);
    }

  if (rc == 0) {
    /* Attempt to open up the device driver.				      */
    if ((hMapDev == NULLHANDLE) && (rc == 0)) {
      rc = DosOpen ("\\DEV\\SSM$", &hMapDev, &ActionTaken, 0,
		    FILE_SYSTEM,
		    OPEN_ACTION_OPEN_IF_EXISTS, OPEN_SHARE_DENYNONE |
		    OPEN_FLAGS_NOINHERIT | OPEN_ACCESS_READONLY, NULL);
      if (rc != 0) {
	fprintf (stderr, "Oops, something is missing ...\nPlease add DEVICE=SSMDD.SYS to CONFIG.SYS\nCannot access memory mapped controller registers\n");
	return (0);
      }
    }
  } else {
    fprintf (stderr, "Cannot access memory mapped controller registers\n");
    return (0);
  }

  if ((hMapDev != NULLHANDLE) && (rc == 0)) {
    Offset = PhysicalAddress & (PAGE_SIZE - 1); // within Page
    Length = (Length + Offset + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
    PhysicalAddress &= ~(PAGE_SIZE - 1);

    /* Set up the parameters for the IOCtl to map the phys addr to linear addr.*/
    parameter.Flag     = 0x100;     /* Meaning MapRam. */
    parameter.PhysAddr = PhysicalAddress;
    parameter.Length   = Length;
    parameter.pLinearAddress = &LinearAdr;

    /* Call the IOCtl to do the map.					     */
    rc = DosDevIOCtl (hMapDev, 0x80, 0x06,
		      &parameter, PLength, &PLength,
		      NULL, 0, NULL);
    if ((rc != 0) || (LinearAdr == 0)) {
      rc = DosDevIOCtl (hMapDev, 0x80, 0x46,
			&parameter, PLength, &PLength,
			NULL, 0, NULL);
      if ((rc != 0) || (LinearAdr == 0)) {
	fprintf (stderr, "Oops, could not map physical memory into process space (rc = %02X, LA=%08lX)\n", rc, LinearAdr);
	return (0);
      }
    }

    if (numMappings < MAX_NUM_MAPPINGS) {
      MapTable[numMappings].PhysicalS = PhysicalAddress;
      MapTable[numMappings].PhysicalE = PhysicalAddress + Length;
      MapTable[numMappings].Linear    = LinearAdr;
      numMappings++;
    }

    LinearAdr += Offset;
  }
  return (LinearAdr);
}

/* --------------------------------------------------------------------------*/

int main (int argc, char *argv[]) {
  APIRET rc;
  ULONG Action;
  int i;
  UCHAR isACPIpresent = FALSE;
  UCHAR VersionMajor, VersionMinor;
  pAdapter p;

  for (i = 1; i < argc; i++) {
    clearSATAStatus = (argv[i][0] == 'c');
    VIAspecial = 0;
//    if (argv[i][0] == 'v') VIAspecial = argv[i][1] & 3;
    SISspecial = 0;
//    if (argv[i][0] == 's') SISspecial = argv[i][1] & 3;
    if (argv[i][0] == 'b') verboseScan = TRUE;
    if (argv[i][0] == 'a') scanACPI = FALSE;
  }

  rc = DosOpen ("OEMHLP$", &hPCI, &Action, 0, 0, FILE_OPEN,
       OPEN_SHARE_DENYNONE | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_ACCESS_READWRITE,
       NULL);
  if (rc != 0) exit (98);
  rc = DosOpen ("TESTCFG$", &hIO, &Action, 0, 0, FILE_OPEN,
       OPEN_SHARE_DENYNONE | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_ACCESS_READWRITE,
       NULL);
  rc = DosOpen ("ACPICA$", &hACPI, &Action, 0, 0, FILE_OPEN,
       OPEN_SHARE_DENYNONE | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_ACCESS_READWRITE,
       NULL);
  if (0 == rc) {
    isACPIpresent = TRUE;
    printf ("ACPI present\n");
  }

  rc = PCIBIOSPresent (&VersionMajor, &VersionMinor, &BusCount);
  if (rc != 0) {
    printf ("PCI BIOS not present (rc=%02Xh).\n", rc);
    exit (1);
  } else {
    printf ("PCI BIOS V%X.%02X detected, ", VersionMajor, VersionMinor);
    printf ("%d PCI bus%s reported.\n", BusCount+1, BusCount ? "es" : "");
  }

  ScanPCIDevices();
  if (isACPIpresent && scanACPI) {
    pAdapter pA;

    ScanACPITree();

    for (pA = Adapter; pA < (Adapter + numAdapters); pA++) {
      if (pA->ProgIF & 0x05) {	/* native PCI mode */
	pA->PicIRQ  = queryIRQforPCIFunction (pA->Bus, pA->Dev, pA->Fnc, PICmode);
	pA->APicIRQ = queryIRQforPCIFunction (pA->Bus, pA->Dev, pA->Fnc, APICmode);
      } else {
	pA->PicIRQ  = 14;
	pA->APicIRQ = 15;
      }
    }
  }

  if (verboseScan) {
    pBridge  pB;
    pAdapter pA;

    printf ("Bridges found:\n");
    for (pB = Bridges; pB < (Bridges + MAX_PCI_BUSES); pB++) {
      if (pB->BridgeType) {
	printf ("%2d:%02d.%02d  class:%04X  type: %d  Bus ",
		 pB->Bus, pB->Dev, pB->Fnc, pB->BridgeType, pB->HeaderType);
	if (pB->PriBus >= 0)
	    printf ("%d->%d", pB->PriBus, pB->SecBus);
	else
	    printf ("H->%d",  pB->SecBus);
	if (pB->Handle)
	  printf ("  ACPI: '%.4s' (%08lX) %s", &pB->Name, pB->Handle, pB->numRouterEntries[0] ? "_PRT" : "");
	printf ("\n");
      }
    }

    printf ("Adapters found:\n");
    for (pA = Adapter; pA < (Adapter + numAdapters); pA++) {
      printf ("%2d:%02d.%02d  %04X:%04X  IRQs: PCI:%2d PIC:%2d APIC:%2d",
	      pA->Bus, pA->Dev, pA->Fnc, pA->Vendor, pA->Device, pA->PCIIRQ, pA->PicIRQ, pA->APicIRQ);
      if (pA->Handle)
	printf ("  ACPI: '%.4s' (%08lX)", &pA->Name, pA->Handle);
      printf ("\n");
    }
  }

  printf ("PCI Bus scan: %d PCI bus%s found.\n", BusCount+1, BusCount ? "es" : "");
  if (numAdapters == 0)
    printf ("No busmaster capable EIDE controllers found.\n");
  else
    for (i = numAdapters, p = Adapter; i > 0; i--, p++) {
      printf ("Found %s %s (%04X/%04X rev %02X) on %d:%d.%d\n",
	       (p->ProgIF & 0x05) ? "native mode" : "legacy mode",
	       p->Adapter->Name, p->Vendor, p->Device, p->Rev,
	       p->Bus, p->Dev, p->Fnc);

      ReadPCIConfigArea (p->Bus, p->Dev, p->Fnc, &PCIConfigArea);
      if (PCIConfigArea.BaseAddr[4] & 1)
	BMIFAddr = PCIConfigArea.BaseAddr[4] & 0x0000FFFC;
      else
	BMIFAddr = 0;

      if (p->Vendor == VENDORIDINITIO) BMIFAddr = 0;
      if (BMIFAddr) {
	BMIFLen = GetRangeSize (p->Bus, p->Dev, p->Fnc, 0x20);
	if (p->Vendor == VENDORIDCYRIX) BMIFLen = 0x80;
	ReadBMIFArea (BMIFAddr, 0, BMIFLen, BMData);
      }
      DumpPCIConfigArea (&PCIConfigArea);
      if (BMIFAddr) {
	printf ("BusMaster Registers:\n");
	DumpBMIFArea (0, BMIFLen, BMData);
      }
      printf ("\n");
      Bus = p->Bus;
      Dev = p->Dev;
      Fnc = p->Fnc;
      PrintPCIConfigArea (&PCIConfigArea);
      printf ("\n");
      if (p->Adapter->PostFun) {
	p->Adapter->PostFun (p);
	printf ("\n");
      }
      if (p->Adapter->PostSATA) {
	p->Adapter->PostSATA (p);
	printf ("\n");
      }
    }

  DosClose (hACPI);
  DosClose (hPCI);
  DosClose (hIO);
  return (0);
}
