/*
 * LupoUsbDriver - Layer L1 helper: generic root hub emulation.
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/rhemul.h"
#include "lupousb/usbspec.h"
#include "lupousb/os.h"

static const u8 rh_dev_desc[USB_DT_DEVICE_SIZE] = {
    18, USB_DT_DEVICE, 0x10, 0x01, USB_CLASS_HUB, 0, 0, 8,
    0x09, 0x12, 0x01, 0x00,             /* VID 0x1209, PID 0x0001 */
    0x00, 0x01, 1, 2, 0, 1
};

static const u8 rh_cfg_desc[] = {
    9, USB_DT_CONFIG, 25, 0, 1, 1, 0, 0xc0, 0,
    9, USB_DT_INTERFACE, 0, 0, 1, USB_CLASS_HUB, 0, 0, 0,
    7, USB_DT_ENDPOINT, 0x81, USB_EP_XFER_INT, 1, 0, 255
};

void rh_init(struct RootHub *rh, u8 nports, const struct RhPortOps *ops, void *ctx,
             void (*complete)(void *, struct UsbXfer *, int), void *cctx, const char *product)
{
    lu_memset(rh, 0, sizeof(*rh));
    rh->nports = (u8)LU_MIN(nports, 7);
    rh->ops = ops;
    rh->ctx = ctx;
    rh->complete = complete;
    rh->cctx = cctx;
    rh->product = product;
}

static void finish(struct RootHub *rh, struct UsbXfer *x, int status)
{
    rh->complete(rh->cctx, x, status);
}

void rh_reset(struct RootHub *rh)
{
    struct UsbXfer *x = rh->status_xfer;
    rh->addr = 0;
    rh->config = 0;
    rh->status_xfer = NULL;
    if (x)
        finish(rh, x, XS_ABORTED);
}

static u8 change_bitmap(struct RootHub *rh)
{
    u8 bm = 0;
    int i;
    for (i = 1; i <= rh->nports; i++) {
        u16 chg = 0;
        rh->ops->status(rh->ctx, i, &chg);
        if (chg)
            bm |= (u8)(1u << i);
    }
    return bm;
}

void rh_changed(struct RootHub *rh)
{
    struct UsbXfer *x = rh->status_xfer;
    u8 bm;
    if (!x)
        return;
    bm = change_bitmap(rh);
    if (!bm)
        return;
    rh->status_xfer = NULL;
    if (x->length) {
        x->data[0] = bm;
        x->actual = 1;
    }
    finish(rh, x, XS_OK);
}

int rh_abort(struct RootHub *rh, struct UsbXfer *x)
{
    if (rh->status_xfer != x)
        return 0;
    rh->status_xfer = NULL;
    finish(rh, x, XS_ABORTED);
    return 1;
}

static u16 make_string(struct RootHub *rh, u8 idx, u8 *buf)
{
    const char *s;
    u16 n = 0;
    if (idx == 0) {
        buf[0] = 4; buf[1] = USB_DT_STRING; buf[2] = 0x09; buf[3] = 0x04;
        return 4;
    }
    s = (idx == 1) ? "LupoUsbDriver" : (idx == 2) ? rh->product : NULL;
    if (!s)
        return 0;
    while (s[n] && n < 60) {
        buf[2 + 2 * n] = (u8)s[n];
        buf[3 + 2 * n] = 0;
        n++;
    }
    buf[0] = (u8)(2 + 2 * n);
    buf[1] = USB_DT_STRING;
    return buf[0];
}

/* Executes a control request; returns XS_OK or XS_STALL, response in buf. */
static int control(struct RootHub *rh, const u8 *s, u8 *buf, u16 *len)
{
    u8 rt = s[0], req = s[1];
    u16 value = lu_get_le16(s + 2), index = lu_get_le16(s + 4);

    *len = 0;
    switch ((rt << 8) | req) {
    case ((USB_DIR_IN | USB_RECIP_DEVICE) << 8) | USB_REQ_GET_DESCRIPTOR:
        switch (value >> 8) {
        case USB_DT_DEVICE: lu_memcpy(buf, rh_dev_desc, 18); *len = 18; return XS_OK;
        case USB_DT_CONFIG: lu_memcpy(buf, rh_cfg_desc, sizeof(rh_cfg_desc));
                            *len = sizeof(rh_cfg_desc); return XS_OK;
        case USB_DT_STRING: *len = make_string(rh, (u8)value, buf);
                            return *len ? XS_OK : XS_STALL;
        default:            return XS_STALL;
        }
    case (USB_RECIP_DEVICE << 8) | USB_REQ_SET_ADDRESS:
        rh->addr = (u8)(value & 0x7f);
        return XS_OK;
    case (USB_RECIP_DEVICE << 8) | USB_REQ_SET_CONFIGURATION:
        rh->config = (u8)value;
        return XS_OK;
    case ((USB_DIR_IN | USB_RECIP_DEVICE) << 8) | USB_REQ_GET_CONFIGURATION:
        buf[0] = rh->config; *len = 1;
        return XS_OK;
    case ((USB_DIR_IN | USB_RECIP_DEVICE) << 8) | USB_REQ_GET_STATUS:
        buf[0] = 1; buf[1] = 0; *len = 2;          /* self powered */
        return XS_OK;
    case ((USB_DIR_IN | USB_RECIP_INTERFACE) << 8) | USB_REQ_GET_STATUS:
    case ((USB_DIR_IN | USB_RECIP_ENDPOINT) << 8) | USB_REQ_GET_STATUS:
        buf[0] = buf[1] = 0; *len = 2;
        return XS_OK;
    case (USB_RECIP_ENDPOINT << 8) | USB_REQ_CLEAR_FEATURE:
    case (USB_RECIP_INTERFACE << 8) | USB_REQ_SET_INTERFACE:
        return XS_OK;

    /* hub class */
    case ((USB_DIR_IN | USB_RT_HUB) << 8) | HUB_REQ_GET_DESCRIPTOR:
        buf[0] = 9; buf[1] = USB_DT_HUB; buf[2] = rh->nports;
        buf[3] = 0x09; buf[4] = 0;      /* individual power + over-current */
        buf[5] = 10;                    /* 20 ms */
        buf[6] = 0;
        buf[7] = 0; buf[8] = 0xff;
        *len = 9;
        return XS_OK;
    case ((USB_DIR_IN | USB_RT_HUB) << 8) | HUB_REQ_GET_STATUS:
        lu_memset(buf, 0, 4); *len = 4;
        return XS_OK;
    case (USB_RT_HUB << 8) | HUB_REQ_CLEAR_FEATURE:
    case (USB_RT_HUB << 8) | HUB_REQ_SET_FEATURE:
        return XS_OK;
    case ((USB_DIR_IN | USB_RT_PORT) << 8) | HUB_REQ_GET_STATUS: {
        u16 chg = 0, st;
        if (index < 1 || index > rh->nports)
            return XS_STALL;
        st = rh->ops->status(rh->ctx, index, &chg);
        lu_put_le16(buf, st);
        lu_put_le16(buf + 2, chg);
        *len = 4;
        return XS_OK;
    }
    case (USB_RT_PORT << 8) | HUB_REQ_SET_FEATURE:
        if (index < 1 || index > rh->nports)
            return XS_STALL;
        rh->ops->set_feature(rh->ctx, index, value);
        return XS_OK;
    case (USB_RT_PORT << 8) | HUB_REQ_CLEAR_FEATURE:
        if (index < 1 || index > rh->nports)
            return XS_STALL;
        rh->ops->clear_feature(rh->ctx, index, value);
        return XS_OK;
    default:
        return XS_STALL;
    }
}

void rh_submit(struct RootHub *rh, struct UsbXfer *x)
{
    x->actual = 0;
    if (x->type == XT_CONTROL) {
        u8 buf[128];
        u16 len = 0;
        int st = control(rh, x->setup, buf, &len);
        if (st == XS_OK && (x->setup[0] & USB_DIR_IN)) {
            u32 n = LU_MIN((u32)len, x->length);
            lu_memcpy(x->data, buf, n);
            x->actual = n;
            if (n < x->length && !(x->flags & XF_ALLOWRUNT))
                st = XS_RUNTPACKET;
        }
        finish(rh, x, st);
        return;
    }
    if (x->type == XT_INT && x->endpoint == 1 && x->dir == XD_IN && rh->config) {
        if (rh->status_xfer) {
            finish(rh, x, XS_BADPARAMS);
            return;
        }
        rh->status_xfer = x;
        rh_changed(rh);
        return;
    }
    finish(rh, x, XS_STALL);
}
