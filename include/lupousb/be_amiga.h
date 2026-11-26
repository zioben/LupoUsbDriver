/*
 * LupoUsbDriver - USBD backend for AmigaOS: talks to any usbhardware.device
 * (lupovirt.device, or a third party host controller driver) with
 * IOUsbHWReq requests.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_BE_AMIGA_H
#define LUPOUSB_BE_AMIGA_H

#include <exec/ports.h>
#include <devices/timer.h>
#include <devices/usbhardware.h>
#include "lupousb/usbd.h"

struct AmigaBackend {
    struct MsgPort *port;           /* replies of transfer requests */
    struct IOUsbHWReq *ctl;         /* opened request, used as template */
    struct MsgPort *tport;
    struct timerequest *treq;
    ULONG last_sigs;                /* signals seen by the last wait() */
    UBYTE opened;
    UBYTE timer_open;
    UWORD inflight;
};

extern const struct UsbdBackend usbd_amiga_backend;

int  amiga_backend_open(struct AmigaBackend *b, const char *device, ULONG unit);
void amiga_backend_close(struct AmigaBackend *b);

#endif /* LUPOUSB_BE_AMIGA_H */
