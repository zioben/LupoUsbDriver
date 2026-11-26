/*
 * LupoUsbDriver - simulation: a full-speed USB 1.1 hub with up to 7 ports.
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/vsim.h"
#include "lupousb/usbspec.h"
#include "lupousb/os.h"

static const u8 hub_dev_desc[USB_DT_DEVICE_SIZE] = {
    18, USB_DT_DEVICE, 0x10, 0x01, USB_CLASS_HUB, 0, 0, 8,
    0x09, 0x12, 0x02, 0x00,         /* VID 0x1209 (pid.codes), PID 0x0002 */
    0x00, 0x01, 1, 2, 0, 1
};

static const u8 hub_cfg_desc[] = {
    9, USB_DT_CONFIG, 25, 0, 1, 1, 0, 0xe0, 50,
    9, USB_DT_INTERFACE, 0, 0, 1, USB_CLASS_HUB, 0, 0, 0,
    7, USB_DT_ENDPOINT, 0x81, USB_EP_XFER_INT, 1, 0, 255
};

static const char *const hub_strings[] = { "Lupo Virtual", "Virtual 4-port hub" };

static u32 sim_now(void) { return os_time_ms(); }

static int hub_control(struct VDev *d, const u8 *s, u8 *buf, u16 *len)
{
    struct VHub *h = d->hub;
    u8 rt = s[0], req = s[1];
    u16 value = lu_get_le16(s + 2), index = lu_get_le16(s + 4);

    if (rt == (USB_DIR_IN | USB_RT_HUB) && req == HUB_REQ_GET_DESCRIPTOR) {
        buf[0] = 9; buf[1] = USB_DT_HUB; buf[2] = h->nports;
        buf[3] = 0x01; buf[4] = 0x00;   /* individual port power switching */
        buf[5] = 10;                    /* PwrOn2PwrGood: 20 ms */
        buf[6] = 100;
        buf[7] = 0x00;                  /* all devices removable */
        buf[8] = 0xff;                  /* PortPwrCtrlMask (USB 1.0 compat) */
        *len = 9;
        return VR_ACK;
    }
    if (rt == (USB_DIR_IN | USB_RT_HUB) && req == HUB_REQ_GET_STATUS) {
        lu_memset(buf, 0, 4);
        buf[2] = h->hub_change;
        *len = 4;
        return VR_ACK;
    }
    if (rt == USB_RT_HUB && req == HUB_REQ_CLEAR_FEATURE) {
        h->hub_change = 0;
        return VR_ACK;
    }
    if (index < 1 || index > h->nports)
        return VR_STALL;
    {
        struct VPort *p = &h->port[index - 1];
        if (rt == (USB_DIR_IN | USB_RT_PORT) && req == HUB_REQ_GET_STATUS) {
            lu_put_le16(buf, p->status);
            lu_put_le16(buf + 2, p->change);
            *len = 4;
            return VR_ACK;
        }
        if (rt == USB_RT_PORT && req == HUB_REQ_SET_FEATURE) {
            vport_set_feature(p, value, sim_now());
            return VR_ACK;
        }
        if (rt == USB_RT_PORT && req == HUB_REQ_CLEAR_FEATURE) {
            vport_clear_feature(p, value);
            return VR_ACK;
        }
    }
    return VR_STALL;
}

static int hub_ep_in(struct VDev *d, u8 ep, u8 *buf, u16 max, u16 *len)
{
    struct VHub *h = d->hub;
    u8 bitmap = h->hub_change ? 1 : 0;
    int i;
    (void)max;
    if (ep != 1)
        return VR_STALL;
    for (i = 0; i < h->nports; i++)
        if (h->port[i].change)
            bitmap |= (u8)(1u << (i + 1));
    if (!bitmap)
        return VR_NAK;
    buf[0] = bitmap;
    *len = 1;
    return VR_ACK;
}

static void hub_tick(struct VDev *d, u32 now)
{
    int i;
    for (i = 0; i < d->hub->nports; i++)
        vport_tick(&d->hub->port[i], now);
}

static void hub_bus_reset(struct VDev *d)
{
    int i;
    /* A reset hub powers its ports off; devices stay physically attached. */
    for (i = 0; i < d->hub->nports; i++) {
        d->hub->port[i].status = 0;
        d->hub->port[i].change = 0;
    }
}

static const struct VDevOps hub_ops = {
    hub_control, hub_ep_in, NULL, hub_tick, hub_bus_reset
};

struct VDev *vdev_hub_create(u8 nports)
{
    struct VDev *d = os_alloc(sizeof(*d));
    if (!d)
        return NULL;
    d->hub = os_alloc(sizeof(*d->hub));
    if (!d->hub) {
        os_free(d, sizeof(*d));
        return NULL;
    }
    d->hub->nports = (u8)LU_MIN(nports, 7);
    d->ops = &hub_ops;
    d->name = "vhub";
    d->dev_desc = hub_dev_desc;
    d->cfg_desc = hub_cfg_desc;
    d->cfg_len = sizeof(hub_cfg_desc);
    d->strings = hub_strings;
    d->nstrings = 2;
    vdev_init(d);
    return d;
}
