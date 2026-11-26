/*
 * LupoUsbDriver - Layer L1: host controller driver core interface.
 *
 * A UsbXfer is the unit of work exchanged between L3 (USBD), L2 (the Exec
 * device) and L1 (the chip driver). Its semantics follow the transaction
 * level abstraction of the usbhardware.device API: the caller describes a
 * whole transfer (address, endpoint, direction, length, max packet size)
 * and the HCD splits it into packets, handles NAK/retries and data toggles
 * and reports the outcome. Error codes use the same numeric values as the
 * usbhardware.device UHIOERR_* codes so L2 can pass them through.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_HCD_H
#define LUPOUSB_HCD_H

#include "lupousb/types.h"

/* Transfer types */
enum { XT_CONTROL = 0, XT_ISO = 1, XT_BULK = 2, XT_INT = 3 };

/* Direction (same values as UHDIR_*) */
enum { XD_SETUP = 0, XD_OUT = 1, XD_IN = 2 };

/* Flags (same bit values as UHFF_*) */
#define XF_LOWSPEED      (1u << 0)
#define XF_HIGHSPEED     (1u << 1)
#define XF_NOSHORTPKT    (1u << 2)
#define XF_NAKTIMEOUT    (1u << 3)
#define XF_ALLOWRUNT     (1u << 4)

/* Status codes (same values as UHIOERR_*) */
enum {
    XS_OK = 0, XS_OFFLINE = 1, XS_NAK = 2, XS_HOSTERROR = 3, XS_STALL = 4,
    XS_PKTTOOLARGE = 5, XS_TIMEOUT = 6, XS_OVERFLOW = 7, XS_CRCERROR = 8,
    XS_RUNTPACKET = 9, XS_NAKTIMEOUT = 10, XS_BADPARAMS = 11, XS_NOMEM = 12,
    XS_ABORTED = 100, XS_PENDING = 101
};

struct UsbXfer;
typedef void (*UsbXferDone)(struct UsbXfer *x);

struct UsbXfer {
    /* --- filled by the caller --- */
    u8   type;          /* XT_* */
    u8   dir;           /* XD_IN / XD_OUT (data stage direction for control) */
    u8   devaddr;       /* 0..127 */
    u8   endpoint;      /* 0..15 */
    u16  flags;         /* XF_* */
    u16  maxpkt;        /* max packet size of the endpoint */
    u16  interval;      /* polling interval in ms (interrupt transfers) */
    u32  nak_timeout;   /* ms, used with XF_NAKTIMEOUT */
    u8   setup[8];      /* SETUP packet for control transfers */
    u8  *data;
    u32  length;
    UsbXferDone done;   /* completion callback (may be NULL) */
    void *user;         /* caller private */
    void *user2;        /* caller private */
    void *be_priv;      /* private to the USBD backend (e.g. its IORequest) */

    /* --- results --- */
    u32  actual;
    int  status;        /* XS_*; stays XS_PENDING until 'done' is called */

    /* --- private to the HCD --- */
    struct UsbXfer *next;
    u8   phase;
    u8   errors;
    u32  offset;
    u32  start_ms;
    u32  due_ms;
    int  hcd_status;    /* final status, copied to 'status' just before 'done' */
};

struct Hcd;

struct HcdOps {
    const char *name;
    int  (*start)(struct Hcd *hcd);              /* reset chip, power ports */
    void (*stop)(struct Hcd *hcd);
    int  (*submit)(struct Hcd *hcd, struct UsbXfer *x);
    int  (*abort)(struct Hcd *hcd, struct UsbXfer *x);
    /* Make progress: process chip events, run transactions, complete
     * transfers. Called by the owner (L2 unit task or the test harness)
     * on every interrupt signal and on a periodic tick. */
    void (*poll)(struct Hcd *hcd);
    u16  (*frame)(struct Hcd *hcd);
    /* Bus reset (UHCMD_USBRESET): the root hub reappears at address 0. */
    void (*bus_reset)(struct Hcd *hcd);
};

struct Hcd {
    const struct HcdOps *ops;
    struct HwBus *bus;
    /* Called (possibly from interrupt context) when poll() should run soon. */
    void (*wake)(void *ctx);
    void *wake_ctx;
    void *priv;
};

static inline int hcd_submit(struct Hcd *h, struct UsbXfer *x) { return h->ops->submit(h, x); }
static inline void hcd_poll(struct Hcd *h) { h->ops->poll(h); }

#endif /* LUPOUSB_HCD_H */
