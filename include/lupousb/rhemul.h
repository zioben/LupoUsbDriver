/*
 * LupoUsbDriver - Layer L1 helper: generic root hub emulation.
 *
 * Chips like SL811HS, MAX3421E, CH376 or the virtual LVHC expose raw ports
 * instead of a USB hub function. The usbhardware.device model requires the
 * root hub to look like a normal USB hub (chapter 11) that appears at
 * address 0 after a bus reset. This module answers the control and status
 * change transfers addressed to the root hub and translates hub class port
 * requests into calls to the chip specific port operations.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_RHEMUL_H
#define LUPOUSB_RHEMUL_H

#include "lupousb/hcd.h"

struct RhPortOps {
    /* Port numbers are 1-based as in the hub class. Status and change use
     * the hub class bit layout (USB_PS_* / USB_PC_*). */
    u16  (*status)(void *ctx, int port, u16 *change);
    void (*set_feature)(void *ctx, int port, u16 feature);
    void (*clear_feature)(void *ctx, int port, u16 feature);
};

struct RootHub {
    u8   addr;
    u8   config;
    u8   nports;
    const struct RhPortOps *ops;
    void *ctx;
    struct UsbXfer *status_xfer;            /* pending status change request */
    void (*complete)(void *cctx, struct UsbXfer *x, int status);
    void *cctx;
    const char *product;
};

void rh_init(struct RootHub *rh, u8 nports, const struct RhPortOps *ops, void *ctx,
             void (*complete)(void *, struct UsbXfer *, int), void *cctx, const char *product);
void rh_reset(struct RootHub *rh);              /* bus reset: back to address 0 */
static inline int rh_is_target(const struct RootHub *rh, const struct UsbXfer *x)
{
    return x->devaddr == rh->addr;
}
/* Handles a transfer addressed to the root hub. Always completes it
 * (immediately or, for the status endpoint, when a change happens). */
void rh_submit(struct RootHub *rh, struct UsbXfer *x);
int  rh_abort(struct RootHub *rh, struct UsbXfer *x);
/* To be called when port change bits may have been set. */
void rh_changed(struct RootHub *rh);

#endif /* LUPOUSB_RHEMUL_H */
