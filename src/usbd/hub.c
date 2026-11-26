/*
 * LupoUsbDriver - Layer L3: hub class driver.
 *
 * One driver for every hub: the root hub (emulated by the HCD, see
 * rhemul.c) and external hubs are handled by exactly the same code.
 *
 * The status change endpoint is read with an asynchronous interrupt
 * transfer; its completion only records the change bitmap. The actual work
 * (debounce, port reset, enumeration, removal) happens in usbd_process(),
 * where synchronous control transfers and delays are allowed.
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/usbd.h"
#include "lupousb/usbspec.h"
#include "lupousb/os.h"

struct UsbdHub {
    struct UsbdHub *next;
    struct UsbdInterface *ifc;
    struct UsbdDevice *dev;
    const struct UsbdEndpoint *ep;
    u8  nports;
    u16 pwr_ms;
    u8  pending;                        /* bitmap: bit0 hub, bit n port n */
    u8  sbuf[2];
    struct UsbXfer sx;
    struct UsbdDevice *child[USBD_MAX_PORTS + 1];
};

static int hub_port_status(struct UsbdHub *h, int port, u16 *status, u16 *change)
{
    u8 b[4];
    u32 act = 0;
    if (usbd_control(h->dev, USB_DIR_IN | USB_RT_PORT, HUB_REQ_GET_STATUS, 0, (u16)port,
                     b, 4, &act) != XS_OK || act < 4)
        return 0;
    *status = lu_get_le16(b);
    *change = lu_get_le16(b + 2);
    return 1;
}

static int port_feature(struct UsbdHub *h, int port, u8 req, u16 feature)
{
    return usbd_control(h->dev, USB_RT_PORT, req, feature, (u16)port, NULL, 0, NULL);
}

static void status_done(struct UsbXfer *x)
{
    struct UsbdHub *h = x->user;
    if (x->status == XS_OK && x->actual) {
        h->pending |= h->sbuf[0];
        h->dev->bus->events = 1;
    } else if (x->status != XS_ABORTED && x->status != XS_OFFLINE) {
        if (x->status == XS_TIMEOUT)
            LU_INFO("hub %u: not responding", h->dev->addr);
        else
            LU_WARN("hub %u: status endpoint error %d", h->dev->addr, x->status);
        h->pending |= 0xfe;                 /* rescan all ports */
        h->dev->bus->events = 1;
    }
}

static void submit_status(struct UsbdHub *h)
{
    usbd_fill_xfer(&h->sx, h->ifc, h->ep, h->sbuf, 1, status_done, h);
    usbd_submit(h->dev, &h->sx);
}

static void remove_child(struct UsbdHub *h, int port)
{
    if (h->child[port]) {
        struct UsbdDevice *c = h->child[port];
        h->child[port] = NULL;
        usbd_remove_device(c);
    }
}

static void connect_port(struct UsbdHub *h, int port)
{
    struct UsbdBus *bus = h->dev->bus;
    u16 st, ch;
    int i;

    usbd_delay(bus, USB_T_ATTDB);                       /* debounce */
    if (!hub_port_status(h, port, &st, &ch) || !(st & USB_PS_CONNECTION))
        return;
    if (port_feature(h, port, HUB_REQ_SET_FEATURE, PORT_RESET) != XS_OK)
        return;
    for (i = 0; i < 50; i++) {                          /* up to 500 ms */
        usbd_delay(bus, USB_T_DRSTR);
        if (!hub_port_status(h, port, &st, &ch))
            return;
        if ((ch & USB_PC_RESET) || !(st & USB_PS_RESET))
            break;
    }
    port_feature(h, port, HUB_REQ_CLEAR_FEATURE, C_PORT_RESET);
    if (!(st & USB_PS_ENABLE)) {
        LU_WARN("hub %u: port %d not enabled after reset", h->dev->addr, port);
        return;
    }
    usbd_delay(bus, USB_T_RSTRCY);
    h->child[port] = usbd_enumerate(bus, h->dev, (u8)port, (st & USB_PS_LOW_SPEED) ? 1 : 0);
    if (!h->child[port])
        port_feature(h, port, HUB_REQ_CLEAR_FEATURE, PORT_ENABLE);
}

static void handle_port(struct UsbdHub *h, int port)
{
    u16 st, ch;
    if (!hub_port_status(h, port, &st, &ch))
        return;
    if (ch & USB_PC_CONNECTION)
        port_feature(h, port, HUB_REQ_CLEAR_FEATURE, C_PORT_CONNECTION);
    if (ch & USB_PC_ENABLE)
        port_feature(h, port, HUB_REQ_CLEAR_FEATURE, C_PORT_ENABLE);
    if (ch & USB_PC_SUSPEND)
        port_feature(h, port, HUB_REQ_CLEAR_FEATURE, C_PORT_SUSPEND);
    if (ch & USB_PC_RESET)
        port_feature(h, port, HUB_REQ_CLEAR_FEATURE, C_PORT_RESET);
    if (ch & USB_PC_OVER_CURRENT) {
        LU_WARN("hub %u: over-current on port %d", h->dev->addr, port);
        port_feature(h, port, HUB_REQ_CLEAR_FEATURE, C_PORT_OVER_CURRENT);
    }
    if ((ch & USB_PC_CONNECTION) || !(st & USB_PS_CONNECTION))
        remove_child(h, port);              /* a new connection replaces the old device */
    if ((st & USB_PS_CONNECTION) && !h->child[port])
        connect_port(h, port);
}

/* Called by usbd_process(). Returns 1 if the hub list may have changed. */
static int hub_service(struct UsbdHub *h)
{
    u8 bm = h->pending;
    int p;
    h->pending = 0;
    if (bm & 1) {
        usbd_control(h->dev, USB_RT_HUB, HUB_REQ_CLEAR_FEATURE, C_HUB_LOCAL_POWER, 0, NULL, 0, NULL);
        usbd_control(h->dev, USB_RT_HUB, HUB_REQ_CLEAR_FEATURE, C_HUB_OVER_CURRENT, 0, NULL, 0, NULL);
    }
    for (p = 1; p <= h->nports; p++)
        if (bm & (1u << p))
            handle_port(h, p);
    return 1;
}

void usbd_process(struct UsbdBus *bus)
{
    int guard = 32;
    while (bus->events && guard--) {
        struct UsbdHub *h;
        bus->events = 0;
        for (h = bus->hubs; h; h = h->next) {
            if (h->pending) {
                hub_service(h);
                bus->events = 1;        /* the list may have changed: rescan */
                break;
            }
        }
    }
    /* re-arm idle status endpoints */
    {
        struct UsbdHub *h;
        for (h = bus->hubs; h; h = h->next)
            if (h->sx.status != XS_PENDING && !h->pending)
                submit_status(h);
    }
}

/* ------------------------------------------------- class driver glue */

static int hub_probe(struct UsbdInterface *ifc)
{
    return ifc->cls == USB_CLASS_HUB;
}

static int hub_attach(struct UsbdInterface *ifc)
{
    struct UsbdDevice *d = ifc->dev;
    struct UsbdBus *bus = d->bus;
    struct UsbdHub *h;
    u8 desc[16];
    u32 act = 0;
    int p;

    if (d->depth >= 6) {
        LU_WARN("hub %u: too many hub tiers", d->addr);
        return 0;
    }
    h = os_alloc(sizeof(*h));
    if (!h)
        return 0;
    h->ifc = ifc;
    h->dev = d;
    h->ep = usbd_find_ep(ifc, USB_EP_XFER_INT, 1);
    if (!h->ep ||
        usbd_control(d, USB_DIR_IN | USB_RT_HUB, HUB_REQ_GET_DESCRIPTOR, USB_DT_HUB << 8, 0,
                     desc, sizeof(desc), &act) != XS_OK || act < 7) {
        os_free(h, sizeof(*h));
        return 0;
    }
    h->nports = (u8)LU_MIN(desc[USB_HD_bNbrPorts], USBD_MAX_PORTS);
    h->pwr_ms = (u16)(desc[USB_HD_bPwrOn2PwrGood] * 2);
    LU_INFO("hub %u: %u ports", d->addr, h->nports);

    for (p = 1; p <= h->nports; p++)
        port_feature(h, p, HUB_REQ_SET_FEATURE, PORT_POWER);
    usbd_delay(bus, LU_MAX(h->pwr_ms, 20));

    h->sx.status = XS_OK;               /* not in flight */
    h->pending = (u8)(((1u << (h->nports + 1)) - 1) & ~1u);   /* initial scan */
    h->next = bus->hubs;
    bus->hubs = h;
    bus->events = 1;
    ifc->driver_data = h;
    return 1;
}

static void hub_detach(struct UsbdInterface *ifc)
{
    struct UsbdHub *h = ifc->driver_data, **pp;
    struct UsbdBus *bus = ifc->dev->bus;
    int p;
    if (!h)
        return;
    usbd_abort_sync(bus, &h->sx);
    for (p = 1; p <= h->nports; p++)
        remove_child(h, p);
    for (pp = &bus->hubs; *pp; pp = &(*pp)->next)
        if (*pp == h) {
            *pp = h->next;
            break;
        }
    os_free(h, sizeof(*h));
    ifc->driver_data = NULL;
}

struct UsbdClassDriver usbd_hub_driver = { "hub", hub_probe, hub_attach, hub_detach, NULL };
