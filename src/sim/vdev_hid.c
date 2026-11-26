/*
 * LupoUsbDriver - simulation: HID devices (mouse with wheel, boot keyboard,
 * Gravis-like gamepad). Reports are queued by the test/demo code and sent
 * on the interrupt IN endpoint; with an empty queue the device NAKs, which
 * is what a real HID device does after SET_IDLE(0).
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/vsim.h"
#include "lupousb/usbspec.h"
#include "lupousb/os.h"

#define QLEN 32
#define RLEN 8

struct HidPriv {
    u8 q[QLEN][RLEN];
    u8 qlen[QLEN];
    u8 head, count;
    u8 protocol;            /* 0 boot, 1 report */
    u8 idle;
    u8 leds;
};

/* ---- mouse: 3 buttons, X, Y, wheel (relative) ---- */
static const u8 mouse_report_desc[] = {
    0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x09, 0x01, 0xa1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01,
    0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05,
    0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x38,
    0x15, 0x81, 0x25, 0x7f, 0x75, 0x08, 0x95, 0x03, 0x81, 0x06,
    0xc0, 0xc0
};

static const u8 mouse_dev_desc[USB_DT_DEVICE_SIZE] = {
    18, USB_DT_DEVICE, 0x10, 0x01, 0, 0, 0, 8,
    0x09, 0x12, 0x03, 0x00, 0x00, 0x01, 1, 2, 0, 1
};

static const u8 mouse_cfg_desc[] = {
    9, USB_DT_CONFIG, 34, 0, 1, 1, 0, 0xa0, 50,
    9, USB_DT_INTERFACE, 0, 0, 1, USB_CLASS_HID, HID_SUBCLASS_BOOT, HID_PROTO_MOUSE, 0,
    9, USB_DT_HID, 0x11, 0x01, 0, 1, USB_DT_REPORT,
       (u8)sizeof(mouse_report_desc), (u8)(sizeof(mouse_report_desc) >> 8),
    7, USB_DT_ENDPOINT, 0x81, USB_EP_XFER_INT, 4, 0, 10
};

static const char *const mouse_strings[] = { "Lupo Virtual", "Virtual wheel mouse" };

/* ---- boot keyboard (HID 1.11 appendix B.1 layout) ---- */
static const u8 kbd_report_desc[] = {
    0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x05, 0x07, 0x19, 0xe0,
    0x29, 0xe7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08,
    0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x81, 0x01, 0x95, 0x05,
    0x75, 0x01, 0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02,
    0x95, 0x01, 0x75, 0x03, 0x91, 0x01, 0x95, 0x06, 0x75, 0x08,
    0x15, 0x00, 0x25, 0x65, 0x05, 0x07, 0x19, 0x00, 0x29, 0x65,
    0x81, 0x00, 0xc0
};

static const u8 kbd_dev_desc[USB_DT_DEVICE_SIZE] = {
    18, USB_DT_DEVICE, 0x10, 0x01, 0, 0, 0, 8,
    0x09, 0x12, 0x04, 0x00, 0x00, 0x01, 1, 2, 0, 1
};

static const u8 kbd_cfg_desc[] = {
    9, USB_DT_CONFIG, 34, 0, 1, 1, 0, 0xa0, 50,
    9, USB_DT_INTERFACE, 0, 0, 1, USB_CLASS_HID, HID_SUBCLASS_BOOT, HID_PROTO_KEYBOARD, 0,
    9, USB_DT_HID, 0x11, 0x01, 0, 1, USB_DT_REPORT,
       (u8)sizeof(kbd_report_desc), (u8)(sizeof(kbd_report_desc) >> 8),
    7, USB_DT_ENDPOINT, 0x81, USB_EP_XFER_INT, 8, 0, 10
};

static const char *const kbd_strings[] = { "Lupo Virtual", "Virtual keyboard" };

/* ---- gamepad: X, Y absolute 0..255, 4 buttons (no boot protocol) ---- */
static const u8 pad_report_desc[] = {
    0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x09, 0x01, 0xa1, 0x00,
    0x09, 0x30, 0x09, 0x31, 0x15, 0x00, 0x26, 0xff, 0x00, 0x75,
    0x08, 0x95, 0x02, 0x81, 0x02, 0xc0, 0x05, 0x09, 0x19, 0x01,
    0x29, 0x04, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x04,
    0x81, 0x02, 0x95, 0x04, 0x81, 0x01, 0xc0
};

static const u8 pad_dev_desc[USB_DT_DEVICE_SIZE] = {
    18, USB_DT_DEVICE, 0x10, 0x01, 0, 0, 0, 8,
    0x09, 0x12, 0x05, 0x00, 0x00, 0x01, 1, 2, 0, 1
};

static const u8 pad_cfg_desc[] = {
    9, USB_DT_CONFIG, 34, 0, 1, 1, 0, 0xa0, 50,
    9, USB_DT_INTERFACE, 0, 0, 1, USB_CLASS_HID, 0, 0, 0,
    9, USB_DT_HID, 0x11, 0x01, 0, 1, USB_DT_REPORT,
       (u8)sizeof(pad_report_desc), (u8)(sizeof(pad_report_desc) >> 8),
    7, USB_DT_ENDPOINT, 0x81, USB_EP_XFER_INT, 3, 0, 10
};

static const char *const pad_strings[] = { "Lupo Virtual", "Virtual gamepad" };

/* ---- common HID behaviour ---- */

static int hid_control(struct VDev *d, const u8 *s, u8 *buf, u16 *len)
{
    struct HidPriv *p = d->priv;
    if ((s[0] & 0x60) != USB_TYPE_CLASS)
        return VR_STALL;
    switch (s[1]) {
    case HID_REQ_SET_IDLE:     p->idle = s[3]; return VR_ACK;
    case HID_REQ_GET_IDLE:     buf[0] = p->idle; *len = 1; return VR_ACK;
    case HID_REQ_SET_PROTOCOL: p->protocol = s[2]; return VR_ACK;
    case HID_REQ_GET_PROTOCOL: buf[0] = p->protocol; *len = 1; return VR_ACK;
    case HID_REQ_SET_REPORT:   p->leds = buf[0]; return VR_ACK;     /* keyboard LEDs */
    case HID_REQ_GET_REPORT:   lu_memset(buf, 0, 8); *len = 8; return VR_ACK;
    default:                   return VR_STALL;
    }
}

static int hid_ep_in(struct VDev *d, u8 ep, u8 *buf, u16 max, u16 *len)
{
    struct HidPriv *p = d->priv;
    u8 n;
    (void)max;
    if (ep != 1)
        return VR_STALL;
    if (!p->count)
        return VR_NAK;
    n = p->qlen[p->head];
    lu_memcpy(buf, p->q[p->head], n);
    *len = n;
    p->head = (u8)((p->head + 1) % QLEN);
    p->count--;
    return VR_ACK;
}

static void hid_bus_reset(struct VDev *d)
{
    struct HidPriv *p = d->priv;
    p->protocol = 1;
    p->count = 0;
}

static const struct VDevOps hid_ops = { hid_control, hid_ep_in, NULL, NULL, hid_bus_reset };

static struct VDev *hid_create(const char *name, const u8 *dd, const u8 *cd, u16 cl,
                               const char *const *str, const u8 *rd, u16 rl, int ls)
{
    struct VDev *d = os_alloc(sizeof(*d));
    if (!d)
        return NULL;
    d->priv = os_alloc(sizeof(struct HidPriv));
    if (!d->priv) {
        os_free(d, sizeof(*d));
        return NULL;
    }
    d->ops = &hid_ops;
    d->name = name;
    d->dev_desc = dd;
    d->cfg_desc = cd;
    d->cfg_len = cl;
    d->strings = str;
    d->nstrings = 2;
    d->report_desc = rd;
    d->report_len = rl;
    d->lowspeed = (u8)(ls ? 1 : 0);
    ((struct HidPriv *)d->priv)->protocol = 1;
    vdev_init(d);
    return d;
}

static void queue_report(struct VDev *d, const u8 *r, u8 n)
{
    struct HidPriv *p = d->priv;
    u8 slot;
    if (p->count == QLEN)
        return;                         /* overflow: drop, like a busy device */
    slot = (u8)((p->head + p->count) % QLEN);
    lu_memcpy(p->q[slot], r, n);
    p->qlen[slot] = n;
    p->count++;
}

struct VDev *vdev_mouse_create(int lowspeed)
{
    return hid_create("vmouse", mouse_dev_desc, mouse_cfg_desc, sizeof(mouse_cfg_desc),
                      mouse_strings, mouse_report_desc, sizeof(mouse_report_desc), lowspeed);
}

struct VDev *vdev_keyboard_create(void)
{
    return hid_create("vkeyboard", kbd_dev_desc, kbd_cfg_desc, sizeof(kbd_cfg_desc),
                      kbd_strings, kbd_report_desc, sizeof(kbd_report_desc), 0);
}

struct VDev *vdev_gamepad_create(void)
{
    return hid_create("vgamepad", pad_dev_desc, pad_cfg_desc, sizeof(pad_cfg_desc),
                      pad_strings, pad_report_desc, sizeof(pad_report_desc), 1);
}

void vmouse_report(struct VDev *d, u8 buttons, s8 dx, s8 dy, s8 wheel)
{
    u8 r[4];
    r[0] = buttons; r[1] = (u8)dx; r[2] = (u8)dy; r[3] = (u8)wheel;
    queue_report(d, r, 4);
}

void vkbd_report(struct VDev *d, u8 modifiers, const u8 keys[6])
{
    u8 r[8];
    r[0] = modifiers; r[1] = 0;
    lu_memcpy(r + 2, keys, 6);
    queue_report(d, r, 8);
}

void vpad_report(struct VDev *d, u8 x, u8 y, u8 buttons)
{
    u8 r[3];
    r[0] = x; r[1] = y; r[2] = buttons;
    queue_report(d, r, 3);
}
