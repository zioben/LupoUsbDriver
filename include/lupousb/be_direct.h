/*
 * LupoUsbDriver - USBD backend that calls an L1 HCD directly (no Exec
 * device in between). Used by host tests and by the host simulator.
 * 'advance' moves the clock forward by 1 ms (simulated time on the host).
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_BE_DIRECT_H
#define LUPOUSB_BE_DIRECT_H

#include "lupousb/usbd.h"

struct DirectBackend {
    struct Hcd *hcd;
    void (*advance)(void);
};

extern const struct UsbdBackend usbd_direct_backend;

/* Host only: simulated clock */
void host_set_time(u32 ms);
long host_allocations(void);

#endif /* LUPOUSB_BE_DIRECT_H */
