/*
 * LupoUsbDriver - Layer L4: HID class driver (mouse, keyboard, joystick).
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/hid.h"
#include "lupousb/usbspec.h"
#include "lupousb/os.h"

struct HidDev {
    struct UsbdInterface *ifc;
    struct HidDecoder dec;
    struct UsbXfer x;
    u8  buf[64];
    u8  running;
};

static const struct HidSink *hid_sink;
static u8 next_joy_unit;

static void report_done(struct UsbXfer *x)
{
    struct HidDev *h = x->user;
    if (x->status == XS_OK) {
        hid_decode_report(&h->dec, h->buf, x->actual);
    } else if (x->status == XS_ABORTED || x->status == XS_OFFLINE) {
        h->running = 0;
        return;
    } else {
        if (x->status == XS_TIMEOUT)    /* typically: device unplugged */
            LU_INFO("hid %u: device not responding", h->ifc->dev->addr);
        else
            LU_WARN("hid %u: interrupt transfer error %d", h->ifc->dev->addr, x->status);
        h->running = 0;
        return;
    }
    if (h->running)
        usbd_submit(h->ifc->dev, &h->x);
}

static int hid_probe(struct UsbdInterface *ifc)
{
    return ifc->cls == USB_CLASS_HID && usbd_find_ep(ifc, USB_EP_XFER_INT, 1) != NULL;
}

static int hid_attach(struct UsbdInterface *ifc)
{
    struct UsbdDevice *d = ifc->dev;
    const struct UsbdEndpoint *ep = usbd_find_ep(ifc, USB_EP_XFER_INT, 1);
    const u8 *hd = usbd_find_iface_desc(ifc, USB_DT_HID);
    struct HidDev *h;
    u8 *rdesc = NULL;
    u16 rlen = 0;
    u32 act = 0;
    int ok = 0;

    h = os_alloc(sizeof(*h));
    if (!h)
        return 0;
    h->ifc = ifc;

    /* Report only on change (idle rate 0); many devices STALL this: ignore. */
    usbd_control(d, USB_TYPE_CLASS | USB_RECIP_INTERFACE, HID_REQ_SET_IDLE, 0, ifc->number,
                 NULL, 0, NULL);

    if (hd && hd[0] >= 9)
        rlen = lu_get_le16(hd + 7);
    if (rlen && rlen <= 1024 && (rdesc = os_alloc(rlen)) != NULL &&
        usbd_control(d, USB_DIR_IN | USB_RECIP_INTERFACE, USB_REQ_GET_DESCRIPTOR,
                     USB_DT_REPORT << 8, ifc->number, rdesc, rlen, &act) == XS_OK)
        ok = hid_decoder_init(&h->dec, rdesc, (u16)act, hid_sink);
    if (rdesc)
        os_free(rdesc, rlen);

    if (!ok && ifc->subcls == HID_SUBCLASS_BOOT &&
        (ifc->proto == HID_PROTO_MOUSE || ifc->proto == HID_PROTO_KEYBOARD)) {
        /* Fall back to the boot protocol, whose report layout is fixed. */
        LU_INFO("hid %u: using boot protocol", d->addr);
        usbd_control(d, USB_TYPE_CLASS | USB_RECIP_INTERFACE, HID_REQ_SET_PROTOCOL, 0,
                     ifc->number, NULL, 0, NULL);
        if (ifc->proto == HID_PROTO_MOUSE)
            ok = hid_decoder_init(&h->dec, hid_boot_mouse_desc, hid_boot_mouse_desc_len, hid_sink);
        else
            ok = hid_decoder_init(&h->dec, hid_boot_kbd_desc, hid_boot_kbd_desc_len, hid_sink);
    }
    if (!ok) {
        LU_INFO("hid %u: unsupported HID device", d->addr);
        os_free(h, sizeof(*h));
        return 0;
    }
    if (h->dec.type == HIDT_JOY)
        h->dec.joy_unit = next_joy_unit++;
    LU_INFO("hid %u: %s", d->addr, h->dec.type == HIDT_MOUSE ? "mouse" :
            h->dec.type == HIDT_KEYBOARD ? "keyboard" : "joystick/gamepad");

    usbd_fill_xfer(&h->x, ifc, ep, h->buf, LU_MIN(ep->maxpkt, (u16)sizeof(h->buf)), report_done, h);
    h->running = 1;
    ifc->driver_data = h;
    usbd_submit(d, &h->x);
    return 1;
}

static void hid_detach(struct UsbdInterface *ifc)
{
    struct HidDev *h = ifc->driver_data;
    if (!h)
        return;
    h->running = 0;
    usbd_abort_sync(ifc->dev->bus, &h->x);
    os_free(h, sizeof(*h));
    ifc->driver_data = NULL;
}

static struct UsbdClassDriver hid_driver = { "hid", hid_probe, hid_attach, hid_detach, NULL };

void hid_class_register(struct UsbdBus *bus, const struct HidSink *sink)
{
    hid_sink = sink;
    usbd_register_driver(bus, &hid_driver);
}
