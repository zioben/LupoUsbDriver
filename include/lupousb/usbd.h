/*
 * LupoUsbDriver - Layer L3: USBD core (bus management, enumeration,
 * hub driver, class driver binding).
 *
 * The core is portable C. It talks to the host controller through a small
 * backend interface:
 *   - host tests / simulator: direct calls to an L1 HCD (src/usbd/be_direct.c)
 *   - AmigaOS: IOUsbHWReq requests to any usbhardware.device (src/amiga/be_amiga.c)
 *
 * Threading model: everything runs in ONE context (the "bus task"). Transfer
 * completions are dispatched by backend->wait(), inside that same context.
 * Completion callbacks must not block (no synchronous transfers, no delays):
 * they record what happened and return; slow work is done in usbd_process().
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_USBD_H
#define LUPOUSB_USBD_H

#include "lupousb/hcd.h"

#define USBD_MAX_IFACES   8
#define USBD_MAX_EPS      8
#define USBD_MAX_PORTS    8
#define USBD_CTRL_TIMEOUT 5000

struct UsbdBus;
struct UsbdDevice;
struct UsbdHub;

struct UsbdBackend {
    int  (*submit)(void *ctx, struct UsbXfer *x);
    int  (*abort)(void *ctx, struct UsbXfer *x);
    /* Waits up to 'ms' milliseconds, dispatching completed transfers. */
    void (*wait)(void *ctx, u32 ms);
    void (*bus_reset)(void *ctx);
};

struct UsbdEndpoint {
    u8  address;            /* bEndpointAddress */
    u8  attributes;         /* bmAttributes */
    u16 maxpkt;
    u8  interval;
};

struct UsbdInterface {
    struct UsbdDevice *dev;
    u8  number, alt, cls, subcls, proto;
    u8  neps;
    struct UsbdEndpoint ep[USBD_MAX_EPS];
    const u8 *desc;         /* interface descriptor (inside dev->cfg) */
    u16 desc_len;           /* bytes up to the next interface descriptor */
    const struct UsbdClassDriver *driver;
    void *driver_data;
};

struct UsbdDevice {
    struct UsbdBus *bus;
    struct UsbdDevice *parent;  /* NULL for the root hub */
    u8  port;                   /* port on the parent hub (1-based) */
    u8  depth;
    u8  addr;
    u8  lowspeed;
    u8  mps0;
    u8  config_value;
    u8  ddesc[18];
    u8 *cfg;
    u16 cfg_len;
    u8  niface;
    struct UsbdInterface iface[USBD_MAX_IFACES];
    char manufacturer[32];
    char product[32];
};

struct UsbdClassDriver {
    const char *name;
    int  (*probe)(struct UsbdInterface *ifc);   /* 1 = I can drive it */
    int  (*attach)(struct UsbdInterface *ifc);  /* 1 = success */
    void (*detach)(struct UsbdInterface *ifc);  /* must abort its transfers */
    struct UsbdClassDriver *next;
};

enum { USBD_EV_ATTACH = 1, USBD_EV_DETACH = 2 };

struct UsbdBus {
    const struct UsbdBackend *be;
    void *be_ctx;
    struct UsbdDevice *dev[128];
    struct UsbdDevice *root;
    struct UsbdClassDriver *drivers;
    struct UsbdHub *hubs;
    u8  events;
    u8  running;
    void (*notify)(void *ctx, int event, struct UsbdDevice *d);
    void *notify_ctx;
};

void usbd_init(struct UsbdBus *bus, const struct UsbdBackend *be, void *be_ctx);
void usbd_register_driver(struct UsbdBus *bus, struct UsbdClassDriver *drv);
int  usbd_start(struct UsbdBus *bus);
void usbd_stop(struct UsbdBus *bus);
/* Handles pending hub events (connect/disconnect). Call after wait(). */
void usbd_process(struct UsbdBus *bus);
/* Convenience main loop step: wait up to ms, then process events. */
void usbd_run(struct UsbdBus *bus, u32 ms);
void usbd_delay(struct UsbdBus *bus, u32 ms);

/* Transfers */
int  usbd_control(struct UsbdDevice *d, u8 rt, u8 req, u16 value, u16 index,
                  void *data, u16 len, u32 *actual);
int  usbd_xfer_sync(struct UsbdBus *bus, struct UsbXfer *x, u32 timeout_ms);
int  usbd_submit(struct UsbdDevice *d, struct UsbXfer *x);
void usbd_abort_sync(struct UsbdBus *bus, struct UsbXfer *x);
/* Prepares an interrupt/bulk transfer for an endpoint of an interface. */
void usbd_fill_xfer(struct UsbXfer *x, struct UsbdInterface *ifc, const struct UsbdEndpoint *ep,
                    u8 *buf, u32 len, UsbXferDone done, void *user);
const struct UsbdEndpoint *usbd_find_ep(struct UsbdInterface *ifc, u8 type, int in);
/* Finds a class specific descriptor following the interface descriptor. */
const u8 *usbd_find_iface_desc(struct UsbdInterface *ifc, u8 type);

void usbd_dump(struct UsbdBus *bus);
int  usbd_device_count(struct UsbdBus *bus);

/* Internal (hub.c <-> usbd.c) */
struct UsbdDevice *usbd_enumerate(struct UsbdBus *bus, struct UsbdDevice *parent, u8 port, int lowspeed);
void usbd_remove_device(struct UsbdDevice *d);
extern struct UsbdClassDriver usbd_hub_driver;

#endif /* LUPOUSB_USBD_H */
