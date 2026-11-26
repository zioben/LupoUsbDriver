/*
 * LupoUsbDriver - ABI compatibility check (compile time only).
 *
 * Verifies that include/lupousb/hwreq.h (our MIT licensed definition) is
 * binary compatible with the usbhardware.device interface as defined by
 * devices/usbhardware.h of the toolchain. The toolchain header is only read
 * here, at build time; it is not part of this repository.
 *
 * Built with m68k-amigaos-gcc by 'make abi-check' (part of 'make amiga',
 * hence of the CI). Any mismatch is a compile error.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <devices/usbhardware.h>
#include "lupousb/hwreq.h"
#include "lupousb/hcd.h"

#define SAME_OFFSET(ours, theirs) \
    _Static_assert(offsetof(struct LuHwReq, ours) == offsetof(struct IOUsbHWReq, theirs), \
                   "offset of " #ours " differs from " #theirs)
#define SAME_VALUE(ours, theirs) \
    _Static_assert((ours) == (theirs), #ours " differs from " #theirs)

/* --- layout --- */
_Static_assert(sizeof(struct LuHwReq) == sizeof(struct IOUsbHWReq), "size differs (V2)");
_Static_assert(sizeof(struct LuHwReq) == LUHWREQ_SIZE_V2, "LUHWREQ_SIZE_V2 wrong");
_Static_assert(offsetof(struct LuHwReq, lhr_Frame) == LUHWREQ_SIZE_V1, "LUHWREQ_SIZE_V1 wrong");
_Static_assert(sizeof(struct IOUsbHWReqObsolete) == LUHWREQ_SIZE_V1, "V1 size differs");
_Static_assert(sizeof(struct LuSetupData) == sizeof(struct UsbSetupData), "setup size differs");
_Static_assert(offsetof(struct LuSetupData, wValue) == offsetof(struct UsbSetupData, wValue), "wValue");
_Static_assert(offsetof(struct LuSetupData, wIndex) == offsetof(struct UsbSetupData, wIndex), "wIndex");
_Static_assert(offsetof(struct LuSetupData, wLength) == offsetof(struct UsbSetupData, wLength), "wLength");

SAME_OFFSET(lhr_Req, iouh_Req);
SAME_OFFSET(lhr_Flags, iouh_Flags);
SAME_OFFSET(lhr_State, iouh_State);
SAME_OFFSET(lhr_Dir, iouh_Dir);
SAME_OFFSET(lhr_DevAddr, iouh_DevAddr);
SAME_OFFSET(lhr_Endpoint, iouh_Endpoint);
SAME_OFFSET(lhr_MaxPktSize, iouh_MaxPktSize);
SAME_OFFSET(lhr_Actual, iouh_Actual);
SAME_OFFSET(lhr_Length, iouh_Length);
SAME_OFFSET(lhr_Data, iouh_Data);
SAME_OFFSET(lhr_Interval, iouh_Interval);
SAME_OFFSET(lhr_NakTimeout, iouh_NakTimeout);
SAME_OFFSET(lhr_SetupData, iouh_SetupData);
SAME_OFFSET(lhr_UserData, iouh_UserData);
SAME_OFFSET(lhr_ExtError, iouh_ExtError);
SAME_OFFSET(lhr_Frame, iouh_Frame);
SAME_OFFSET(lhr_SplitHubAddr, iouh_SplitHubAddr);
SAME_OFFSET(lhr_SplitHubPort, iouh_SplitHubPort);
SAME_OFFSET(lhr_DriverPrivate1, iouh_DriverPrivate1);
SAME_OFFSET(lhr_DriverPrivate2, iouh_DriverPrivate2);

/* --- constants --- */
SAME_VALUE(LUHCMD_QUERYDEVICE, UHCMD_QUERYDEVICE);
SAME_VALUE(LUHCMD_USBRESET, UHCMD_USBRESET);
SAME_VALUE(LUHCMD_USBRESUME, UHCMD_USBRESUME);
SAME_VALUE(LUHCMD_USBSUSPEND, UHCMD_USBSUSPEND);
SAME_VALUE(LUHCMD_USBOPER, UHCMD_USBOPER);
SAME_VALUE(LUHCMD_CONTROLXFER, UHCMD_CONTROLXFER);
SAME_VALUE(LUHCMD_ISOXFER, UHCMD_ISOXFER);
SAME_VALUE(LUHCMD_INTXFER, UHCMD_INTXFER);
SAME_VALUE(LUHCMD_BULKXFER, UHCMD_BULKXFER);

SAME_VALUE(LUHIOERR_NO_ERROR, UHIOERR_NO_ERROR);
SAME_VALUE(LUHIOERR_USBOFFLINE, UHIOERR_USBOFFLINE);
SAME_VALUE(LUHIOERR_NAK, UHIOERR_NAK);
SAME_VALUE(LUHIOERR_HOSTERROR, UHIOERR_HOSTERROR);
SAME_VALUE(LUHIOERR_STALL, UHIOERR_STALL);
SAME_VALUE(LUHIOERR_PKTTOOLARGE, UHIOERR_PKTTOOLARGE);
SAME_VALUE(LUHIOERR_TIMEOUT, UHIOERR_TIMEOUT);
SAME_VALUE(LUHIOERR_OVERFLOW, UHIOERR_OVERFLOW);
SAME_VALUE(LUHIOERR_CRCERROR, UHIOERR_CRCERROR);
SAME_VALUE(LUHIOERR_RUNTPACKET, UHIOERR_RUNTPACKET);
SAME_VALUE(LUHIOERR_NAKTIMEOUT, UHIOERR_NAKTIMEOUT);
SAME_VALUE(LUHIOERR_BADPARAMS, UHIOERR_BADPARAMS);
SAME_VALUE(LUHIOERR_OUTOFMEMORY, UHIOERR_OUTOFMEMORY);

SAME_VALUE(LUHDIR_SETUP, UHDIR_SETUP);
SAME_VALUE(LUHDIR_OUT, UHDIR_OUT);
SAME_VALUE(LUHDIR_IN, UHDIR_IN);

SAME_VALUE(LUHF_LOWSPEED, UHFF_LOWSPEED);
SAME_VALUE(LUHF_HIGHSPEED, UHFF_HIGHSPEED);
SAME_VALUE(LUHF_NOSHORTPKT, UHFF_NOSHORTPKT);
SAME_VALUE(LUHF_NAKTIMEOUT, UHFF_NAKTIMEOUT);
SAME_VALUE(LUHF_ALLOWRUNTPKTS, UHFF_ALLOWRUNTPKTS);
SAME_VALUE(LUHF_SPLITTRANS, UHFF_SPLITTRANS);
SAME_VALUE(LUHF_MULTI_1, UHFF_MULTI_1);
SAME_VALUE(LUHF_MULTI_2, UHFF_MULTI_2);

SAME_VALUE(LUHS_OPERATIONAL, UHSF_OPERATIONAL);
SAME_VALUE(LUHS_RESUMING, UHSF_RESUMING);
SAME_VALUE(LUHS_SUSPENDED, UHSF_SUSPENDED);
SAME_VALUE(LUHS_RESET, UHSF_RESET);

SAME_VALUE(LUHA_State, UHA_State);
SAME_VALUE(LUHA_Manufacturer, UHA_Manufacturer);
SAME_VALUE(LUHA_ProductName, UHA_ProductName);
SAME_VALUE(LUHA_Version, UHA_Version);
SAME_VALUE(LUHA_Revision, UHA_Revision);
SAME_VALUE(LUHA_Description, UHA_Description);
SAME_VALUE(LUHA_Copyright, UHA_Copyright);
SAME_VALUE(LUHA_DriverVersion, UHA_DriverVersion);

/* --- the portable core's own codes must keep matching too --- */
SAME_VALUE(XS_OK, LUHIOERR_NO_ERROR);
SAME_VALUE(XS_OFFLINE, LUHIOERR_USBOFFLINE);
SAME_VALUE(XS_STALL, LUHIOERR_STALL);
SAME_VALUE(XS_TIMEOUT, LUHIOERR_TIMEOUT);
SAME_VALUE(XS_NAKTIMEOUT, LUHIOERR_NAKTIMEOUT);
SAME_VALUE(XS_NOMEM, LUHIOERR_OUTOFMEMORY);
SAME_VALUE(XF_LOWSPEED, LUHF_LOWSPEED);
SAME_VALUE(XF_NOSHORTPKT, LUHF_NOSHORTPKT);
SAME_VALUE(XF_NAKTIMEOUT, LUHF_NAKTIMEOUT);
SAME_VALUE(XF_ALLOWRUNT, LUHF_ALLOWRUNTPKTS);
SAME_VALUE(XD_SETUP, LUHDIR_SETUP);
SAME_VALUE(XD_OUT, LUHDIR_OUT);
SAME_VALUE(XD_IN, LUHDIR_IN);

int lupousb_abi_check_passed;
