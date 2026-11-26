/*
 * LupoUsbDriver - Layer L3: USBD core.
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/usbd.h"
#include "lupousb/usbspec.h"
#include "lupousb/os.h"

void usbd_init(struct UsbdBus *bus, const struct UsbdBackend *be, void *be_ctx)
{
    lu_memset(bus, 0, sizeof(*bus));
    bus->be = be;
    bus->be_ctx = be_ctx;
    usbd_register_driver(bus, &usbd_hub_driver);
}

void usbd_register_driver(struct UsbdBus *bus, struct UsbdClassDriver *drv)
{
    struct UsbdClassDriver **p = &bus->drivers;
    while (*p)
        p = &(*p)->next;
    drv->next = NULL;
    *p = drv;
}

/* ------------------------------------------------------- transfers */

static void sync_done(struct UsbXfer *x) { (void)x; }

int usbd_xfer_sync(struct UsbdBus *bus, struct UsbXfer *x, u32 timeout_ms)
{
    u32 start = os_time_ms();
    UsbXferDone saved = x->done;
    x->done = sync_done;
    x->status = XS_PENDING;
    bus->be->submit(bus->be_ctx, x);
    while (x->status == XS_PENDING) {
        if (os_time_ms() - start > timeout_ms) {
            usbd_abort_sync(bus, x);
            x->status = XS_TIMEOUT;
            break;
        }
        bus->be->wait(bus->be_ctx, 1);
    }
    x->done = saved;
    return x->status;
}

void usbd_abort_sync(struct UsbdBus *bus, struct UsbXfer *x)
{
    int tries = 1000;
    if (x->status != XS_PENDING)
        return;
    bus->be->abort(bus->be_ctx, x);
    while (x->status == XS_PENDING && tries--)
        bus->be->wait(bus->be_ctx, 1);
}

int usbd_submit(struct UsbdDevice *d, struct UsbXfer *x)
{
    (void)d;
    x->status = XS_PENDING;
    return d->bus->be->submit(d->bus->be_ctx, x);
}

int usbd_control(struct UsbdDevice *d, u8 rt, u8 req, u16 value, u16 index,
                 void *data, u16 len, u32 *actual)
{
    struct UsbXfer x;
    int st;
    lu_memset(&x, 0, sizeof(x));
    x.type = XT_CONTROL;
    x.dir = (rt & USB_DIR_IN) ? XD_IN : XD_OUT;
    x.devaddr = d->addr;
    x.endpoint = 0;
    x.maxpkt = d->mps0 ? d->mps0 : 8;
    x.flags = (u16)((d->lowspeed ? XF_LOWSPEED : 0) | XF_ALLOWRUNT | XF_NAKTIMEOUT);
    x.nak_timeout = USBD_CTRL_TIMEOUT;
    x.setup[0] = rt;
    x.setup[1] = req;
    lu_put_le16(x.setup + 2, value);
    lu_put_le16(x.setup + 4, index);
    lu_put_le16(x.setup + 6, len);
    x.data = data;
    x.length = len;
    st = usbd_xfer_sync(d->bus, &x, USBD_CTRL_TIMEOUT + 500);
    if (actual)
        *actual = x.actual;
    if (st != XS_OK)
        LU_DBG("usbd: control %02x/%02x to addr %u failed: %d", rt, req, d->addr, st);
    return st;
}

void usbd_fill_xfer(struct UsbXfer *x, struct UsbdInterface *ifc, const struct UsbdEndpoint *ep,
                    u8 *buf, u32 len, UsbXferDone done, void *user)
{
    struct UsbdDevice *d = ifc->dev;
    lu_memset(x, 0, sizeof(*x));
    x->type = (u8)(ep->attributes & USB_EP_XFER_MASK);
    x->dir = (ep->address & USB_EP_DIR_IN) ? XD_IN : XD_OUT;
    x->devaddr = d->addr;
    x->endpoint = ep->address & USB_EP_NUM_MASK;
    x->maxpkt = ep->maxpkt;
    x->interval = ep->interval ? ep->interval : 1;
    x->flags = (u16)((d->lowspeed ? XF_LOWSPEED : 0) | XF_ALLOWRUNT);
    x->data = buf;
    x->length = len;
    x->done = done;
    x->user = user;
}

const struct UsbdEndpoint *usbd_find_ep(struct UsbdInterface *ifc, u8 type, int in)
{
    int i;
    for (i = 0; i < ifc->neps; i++) {
        const struct UsbdEndpoint *e = &ifc->ep[i];
        if ((e->attributes & USB_EP_XFER_MASK) == type &&
            ((e->address & USB_EP_DIR_IN) ? 1 : 0) == (in ? 1 : 0))
            return e;
    }
    return NULL;
}

const u8 *usbd_find_iface_desc(struct UsbdInterface *ifc, u8 type)
{
    u16 pos = ifc->desc[0];
    while (pos + 2 <= ifc->desc_len) {
        const u8 *p = ifc->desc + pos;
        if (p[0] < 2)
            break;
        if (p[1] == type)
            return p;
        pos = (u16)(pos + p[0]);
    }
    return NULL;
}

void usbd_delay(struct UsbdBus *bus, u32 ms)
{
    u32 start = os_time_ms();
    while (os_time_ms() - start < ms)
        bus->be->wait(bus->be_ctx, ms - (os_time_ms() - start));
}

/* ----------------------------------------------------- enumeration */

static u8 alloc_address(struct UsbdBus *bus)
{
    u8 a;
    for (a = 1; a < 128; a++)
        if (!bus->dev[a])
            return a;
    return 0;
}

static void get_string(struct UsbdDevice *d, u8 index, u16 lang, char *out, int outlen)
{
    u8 buf[128];
    u32 act = 0;
    int i, n;
    out[0] = 0;
    if (!index)
        return;
    if (usbd_control(d, USB_DIR_IN, USB_REQ_GET_DESCRIPTOR, (u16)((USB_DT_STRING << 8) | index),
                     lang, buf, sizeof(buf), &act) != XS_OK || act < 2)
        return;
    n = (LU_MIN((int)act, (int)buf[0]) - 2) / 2;
    for (i = 0; i < n && i < outlen - 1; i++) {
        u16 c = lu_get_le16(buf + 2 + 2 * i);
        out[i] = (char)(c < 0x100 ? c : '?');      /* Latin-1 subset */
    }
    out[i] = 0;
}

static void parse_config(struct UsbdDevice *d)
{
    u16 pos = 0;
    struct UsbdInterface *cur = NULL;
    while (pos + 2 <= d->cfg_len) {
        const u8 *p = d->cfg + pos;
        if (p[0] < 2 || pos + p[0] > d->cfg_len)
            break;
        if (p[1] == USB_DT_INTERFACE && p[0] >= USB_DT_INTERFACE_SIZE) {
            if (cur)
                cur->desc_len = (u16)(p - cur->desc);
            cur = NULL;
            if (p[USB_ID_bAlternateSetting] == 0 && d->niface < USBD_MAX_IFACES) {
                cur = &d->iface[d->niface++];
                cur->dev = d;
                cur->desc = p;
                cur->number = p[USB_ID_bInterfaceNumber];
                cur->alt = 0;
                cur->cls = p[USB_ID_bInterfaceClass];
                cur->subcls = p[USB_ID_bInterfaceSubClass];
                cur->proto = p[USB_ID_bInterfaceProtocol];
            }
        } else if (p[1] == USB_DT_ENDPOINT && p[0] >= USB_DT_ENDPOINT_SIZE && cur &&
                   cur->neps < USBD_MAX_EPS) {
            struct UsbdEndpoint *e = &cur->ep[cur->neps++];
            e->address = p[USB_ED_bEndpointAddress];
            e->attributes = p[USB_ED_bmAttributes];
            e->maxpkt = (u16)(lu_get_le16(p + USB_ED_wMaxPacketSize) & 0x7ff);
            e->interval = p[USB_ED_bInterval];
        }
        pos = (u16)(pos + p[0]);
    }
    if (cur)
        cur->desc_len = (u16)(d->cfg + d->cfg_len - cur->desc);
}

static void free_device(struct UsbdDevice *d)
{
    if (d->cfg)
        os_free(d->cfg, d->cfg_len);
    os_free(d, sizeof(*d));
}

static void bind_drivers(struct UsbdDevice *d)
{
    int i;
    for (i = 0; i < d->niface; i++) {
        struct UsbdInterface *ifc = &d->iface[i];
        struct UsbdClassDriver *drv;
        for (drv = d->bus->drivers; drv; drv = drv->next) {
            if (drv->probe(ifc) && drv->attach(ifc)) {
                ifc->driver = drv;
                LU_INFO("usbd: addr %u interface %u bound to %s", d->addr, ifc->number, drv->name);
                break;
            }
        }
        if (!ifc->driver)
            LU_INFO("usbd: addr %u interface %u (class %02x) has no driver",
                    d->addr, ifc->number, ifc->cls);
    }
}

struct UsbdDevice *usbd_enumerate(struct UsbdBus *bus, struct UsbdDevice *parent, u8 port, int lowspeed)
{
    struct UsbdDevice *d;
    u8 buf[USB_DT_CONFIG_SIZE];
    u32 act = 0;
    u16 lang = 0x0409;
    u8 addr = alloc_address(bus);
    int tries;

    if (!addr) {
        LU_ERR("usbd: no free USB address");
        return NULL;
    }
    d = os_alloc(sizeof(*d));
    if (!d)
        return NULL;
    d->bus = bus;
    d->parent = parent;
    d->port = port;
    d->depth = parent ? (u8)(parent->depth + 1) : 0;
    d->lowspeed = (u8)(lowspeed ? 1 : 0);
    d->addr = 0;
    d->mps0 = 8;

    /* 1. first 8 bytes of the device descriptor at the default address */
    for (tries = 0; tries < 3; tries++) {
        if (usbd_control(d, USB_DIR_IN, USB_REQ_GET_DESCRIPTOR, USB_DT_DEVICE << 8, 0,
                         d->ddesc, 8, &act) == XS_OK && act >= 8)
            break;
        usbd_delay(bus, 10);
    }
    if (tries == 3)
        goto fail;
    d->mps0 = d->ddesc[USB_DD_bMaxPacketSize0];       /* byte 7, not 6! */
    if (d->mps0 != 8 && d->mps0 != 16 && d->mps0 != 32 && d->mps0 != 64)
        d->mps0 = 8;

    /* 2. assign the address */
    if (usbd_control(d, USB_RECIP_DEVICE, USB_REQ_SET_ADDRESS, addr, 0, NULL, 0, NULL) != XS_OK)
        goto fail;
    d->addr = addr;
    bus->dev[addr] = d;                 /* reserve the address */
    usbd_delay(bus, USB_T_SETADDR_RECOVERY);

    /* 3. full device descriptor and configuration */
    if (usbd_control(d, USB_DIR_IN, USB_REQ_GET_DESCRIPTOR, USB_DT_DEVICE << 8, 0,
                     d->ddesc, USB_DT_DEVICE_SIZE, &act) != XS_OK || act < USB_DT_DEVICE_SIZE)
        goto fail;
    if (usbd_control(d, USB_DIR_IN, USB_REQ_GET_DESCRIPTOR, USB_DT_CONFIG << 8, 0,
                     buf, USB_DT_CONFIG_SIZE, &act) != XS_OK || act < USB_DT_CONFIG_SIZE)
        goto fail;
    d->cfg_len = (u16)LU_MIN(lu_get_le16(buf + USB_CD_wTotalLength), 1024);
    d->cfg = os_alloc(d->cfg_len);
    if (!d->cfg ||
        usbd_control(d, USB_DIR_IN, USB_REQ_GET_DESCRIPTOR, USB_DT_CONFIG << 8, 0,
                     d->cfg, d->cfg_len, &act) != XS_OK || act < USB_DT_CONFIG_SIZE)
        goto fail;
    d->cfg_len = (u16)act;
    d->config_value = d->cfg[USB_CD_bConfigurationValue];
    parse_config(d);

    /* 4. strings (best effort) */
    if (d->ddesc[USB_DD_iManufacturer] || d->ddesc[USB_DD_iProduct]) {
        u8 l[4];
        if (usbd_control(d, USB_DIR_IN, USB_REQ_GET_DESCRIPTOR, USB_DT_STRING << 8, 0,
                         l, 4, &act) == XS_OK && act >= 4)
            lang = lu_get_le16(l + 2);
        get_string(d, d->ddesc[USB_DD_iManufacturer], lang, d->manufacturer, sizeof(d->manufacturer));
        get_string(d, d->ddesc[USB_DD_iProduct], lang, d->product, sizeof(d->product));
    }

    /* 5. configure */
    if (usbd_control(d, USB_RECIP_DEVICE, USB_REQ_SET_CONFIGURATION, d->config_value, 0,
                     NULL, 0, NULL) != XS_OK)
        goto fail;

    LU_INFO("usbd: new device addr %u %04x:%04x %s speed \"%s\"", addr,
            lu_get_le16(d->ddesc + USB_DD_idVendor), lu_get_le16(d->ddesc + USB_DD_idProduct),
            d->lowspeed ? "low" : "full", d->product);
    if (!parent)
        bus->root = d;
    bind_drivers(d);
    if (bus->notify)
        bus->notify(bus->notify_ctx, USBD_EV_ATTACH, d);
    return d;

fail:
    LU_WARN("usbd: enumeration failed (parent addr %u port %u)", parent ? parent->addr : 0, port);
    if (d->addr && bus->dev[d->addr] == d)
        bus->dev[d->addr] = NULL;
    free_device(d);
    return NULL;
}

void usbd_remove_device(struct UsbdDevice *d)
{
    struct UsbdBus *bus = d->bus;
    int i;
    for (i = 0; i < d->niface; i++) {
        struct UsbdInterface *ifc = &d->iface[i];
        if (ifc->driver) {
            ifc->driver->detach(ifc);       /* hubs remove their children here */
            ifc->driver = NULL;
        }
    }
    LU_INFO("usbd: device addr %u removed", d->addr);
    if (bus->notify)
        bus->notify(bus->notify_ctx, USBD_EV_DETACH, d);
    if (bus->dev[d->addr] == d)
        bus->dev[d->addr] = NULL;
    if (bus->root == d)
        bus->root = NULL;
    free_device(d);
}

/* ------------------------------------------------------- bus level */

int usbd_start(struct UsbdBus *bus)
{
    if (bus->be->bus_reset)
        bus->be->bus_reset(bus->be_ctx);
    bus->running = 1;
    usbd_delay(bus, 10);
    /* After a bus reset the root hub answers at address 0. */
    if (!usbd_enumerate(bus, NULL, 0, 0)) {
        LU_ERR("usbd: root hub not found");
        bus->running = 0;
        return 0;
    }
    usbd_process(bus);
    return 1;
}

void usbd_stop(struct UsbdBus *bus)
{
    if (bus->root)
        usbd_remove_device(bus->root);
    bus->running = 0;
}

void usbd_run(struct UsbdBus *bus, u32 ms)
{
    bus->be->wait(bus->be_ctx, ms);
    usbd_process(bus);
}

int usbd_device_count(struct UsbdBus *bus)
{
    int a, n = 0;
    for (a = 1; a < 128; a++)
        if (bus->dev[a])
            n++;
    return n;
}

static void dump_dev(struct UsbdBus *bus, struct UsbdDevice *d)
{
    char ind[20];
    int i, a;
    for (i = 0; i < d->depth * 2 && i < 18; i++)
        ind[i] = ' ';
    ind[i] = 0;
    LU_INFO("%s[%u] port %u %04x:%04x %s %s/%s", ind, d->addr, d->port,
            lu_get_le16(d->ddesc + USB_DD_idVendor), lu_get_le16(d->ddesc + USB_DD_idProduct),
            d->lowspeed ? "LS" : "FS", d->manufacturer, d->product);
    for (i = 0; i < d->niface; i++)
        LU_INFO("%s    if%u class %02x/%02x/%02x -> %s", ind, d->iface[i].number,
                d->iface[i].cls, d->iface[i].subcls, d->iface[i].proto,
                d->iface[i].driver ? d->iface[i].driver->name : "-");
    for (a = 1; a < 128; a++)
        if (bus->dev[a] && bus->dev[a]->parent == d)
            dump_dev(bus, bus->dev[a]);
}

void usbd_dump(struct UsbdBus *bus)
{
    if (bus->root)
        dump_dev(bus, bus->root);
    else
        LU_INFO("usbd: no devices");
}
