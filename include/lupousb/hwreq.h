/*
 * LupoUsbDriver - host controller device request (layer L2 interface).
 *
 * Independent, MIT licensed definition of the request block exchanged
 * between a USB stack and a USB host controller Exec device.
 *
 * Provenance: the binary layout (field order, sizes, offsets) and the
 * numeric values below were derived from the published usbhardware.device
 * interface, version 2, for the purpose of interoperability with existing
 * drivers and stacks for AmigaOS 3.x. No text of the original include
 * files was copied; names are our own. tests/abi/hwreq_abi.c verifies at
 * build time that this definition is binary compatible with the
 * toolchain's devices/usbhardware.h.
 *
 * Layout (m68k, 2-byte alignment, total 90 bytes):
 *   0 lhr_Req  32 Flags  34 State  36 Dir  38 DevAddr  40 Endpoint
 *  42 MaxPktSize  44 Actual  48 Length  52 Data  56 Interval
 *  58 NakTimeout  62 SetupData(8)  70 UserData  74 ExtError
 *  76 Frame  78 SplitHubAddr  80 SplitHubPort  82 DriverPrivate1
 *  86 DriverPrivate2
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_HWREQ_H
#define LUPOUSB_HWREQ_H

#include <exec/types.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <utility/tagitem.h>

#if defined(__GNUC__)
#pragma pack(2)
#endif

/* USB SETUP packet. wValue/wIndex/wLength are stored LITTLE ENDIAN
 * (exactly as sent on the bus), also on the big endian 68k. */
struct LuSetupData {
    UBYTE bmRequestType;
    UBYTE bRequest;
    UWORD wValue;
    UWORD wIndex;
    UWORD wLength;
};

struct LuHwReq {
    struct IORequest lhr_Req;
    UWORD lhr_Flags;            /* LUHF_* */
    UWORD lhr_State;            /* LUHS_* (filled by the device) */
    UWORD lhr_Dir;              /* LUHDIR_* */
    UWORD lhr_DevAddr;          /* 0..127 */
    UWORD lhr_Endpoint;         /* 0..15 */
    UWORD lhr_MaxPktSize;
    ULONG lhr_Actual;           /* bytes transferred (filled by the device) */
    ULONG lhr_Length;
    APTR  lhr_Data;
    UWORD lhr_Interval;         /* ms (interrupt transfers) */
    ULONG lhr_NakTimeout;       /* ms, with LUHF_NAKTIMEOUT */
    struct LuSetupData lhr_SetupData;
    APTR  lhr_UserData;         /* owned by the caller, never touched by the device */
    UWORD lhr_ExtError;
    /* version 2 extension */
    UWORD lhr_Frame;
    UWORD lhr_SplitHubAddr;
    UWORD lhr_SplitHubPort;
    APTR  lhr_DriverPrivate1;   /* owned by the device */
    APTR  lhr_DriverPrivate2;   /* owned by the device */
};

#if defined(__GNUC__)
#pragma pack()
#endif

#define LUHWREQ_SIZE_V1     76  /* requests up to lhr_ExtError */
#define LUHWREQ_SIZE_V2     90

/* Commands */
#define LUHCMD_QUERYDEVICE  (CMD_NONSTD + 0)
#define LUHCMD_USBRESET     (CMD_NONSTD + 1)
#define LUHCMD_USBRESUME    (CMD_NONSTD + 2)
#define LUHCMD_USBSUSPEND   CMD_STOP
#define LUHCMD_USBOPER      CMD_START
#define LUHCMD_CONTROLXFER  (CMD_NONSTD + 3)
#define LUHCMD_ISOXFER      (CMD_NONSTD + 4)
#define LUHCMD_INTXFER      (CMD_NONSTD + 5)
#define LUHCMD_BULKXFER     (CMD_NONSTD + 6)

/* io_Error values (in addition to the standard IOERR_*) */
#define LUHIOERR_NO_ERROR     0
#define LUHIOERR_USBOFFLINE   1
#define LUHIOERR_NAK          2
#define LUHIOERR_HOSTERROR    3
#define LUHIOERR_STALL        4
#define LUHIOERR_PKTTOOLARGE  5
#define LUHIOERR_TIMEOUT      6
#define LUHIOERR_OVERFLOW     7
#define LUHIOERR_CRCERROR     8
#define LUHIOERR_RUNTPACKET   9
#define LUHIOERR_NAKTIMEOUT   10
#define LUHIOERR_BADPARAMS    11
#define LUHIOERR_OUTOFMEMORY  12

/* lhr_Dir */
#define LUHDIR_SETUP    0
#define LUHDIR_OUT      1
#define LUHDIR_IN       2

/* lhr_Flags */
#define LUHF_LOWSPEED       (1u << 0)
#define LUHF_HIGHSPEED      (1u << 1)
#define LUHF_NOSHORTPKT     (1u << 2)
#define LUHF_NAKTIMEOUT     (1u << 3)
#define LUHF_ALLOWRUNTPKTS  (1u << 4)
#define LUHF_SPLITTRANS     (1u << 5)
#define LUHF_MULTI_1        (1u << 6)
#define LUHF_MULTI_2        (1u << 7)

/* lhr_State and LUHA_State */
#define LUHS_OPERATIONAL    (1u << 0)
#define LUHS_RESUMING       (1u << 1)
#define LUHS_SUSPENDED      (1u << 2)
#define LUHS_RESET          (1u << 3)

/* Tags for LUHCMD_QUERYDEVICE (lhr_Data points to a tag list; each
 * ti_Data is a pointer to where the value is stored) */
#define LUHA_Base           (TAG_USER + 0x4711)
#define LUHA_State          (LUHA_Base + 0x01)   /* ULONG */
#define LUHA_Manufacturer   (LUHA_Base + 0x10)   /* STRPTR */
#define LUHA_ProductName    (LUHA_Base + 0x11)   /* STRPTR */
#define LUHA_Version        (LUHA_Base + 0x12)   /* ULONG */
#define LUHA_Revision       (LUHA_Base + 0x13)   /* ULONG */
#define LUHA_Description    (LUHA_Base + 0x14)   /* STRPTR */
#define LUHA_Copyright      (LUHA_Base + 0x15)   /* STRPTR */
#define LUHA_DriverVersion  (LUHA_Base + 0x20)   /* ULONG, e.g. 0x0200 */

#endif /* LUPOUSB_HWREQ_H */
