/*
 * LupoUsbDriver - Layer L1: driver for the LVHC host controller.
 *
 * LVHC (see lvhc_regs.h) has one serial interface engine: the driver turns
 * every UsbXfer into a sequence of single transactions (SETUP / IN / OUT),
 * keeps the data toggles, retries NAKed and failed packets and schedules
 * interrupt endpoints by their polling interval. The same structure applies
 * to the real chips of the same family (SL811HS, MAX3421E).
 *
 * The chip is reached only through L0 (struct HwBus); the root hub is
 * provided by the generic emulation in rhemul.c.
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/lvhc.h"
#include "lupousb/lvhc_regs.h"
#include "lupousb/rhemul.h"
#include "lupousb/usbspec.h"
#include "lupousb/os.h"

enum { PH_SETUP = 0, PH_DATA = 1, PH_STATUS = 2 };

#define MAX_ERRORS      3
#define MAX_TX_PER_POLL 64

struct Lvhc {
    struct Hcd hcd;
    struct HwBus *bus;
    struct RootHub rh;
    u8   nports;
    u8   running;
    struct UsbXfer *q[4];               /* per transfer type, FIFO */
    struct UsbXfer *done_head, *done_tail;
    struct UsbXfer *cur;                /* transaction in flight */
    u8   cur_len;
    volatile u8 irq_pending;
    u16  tog_in[128];                   /* bit n = next DATAx expected on IN ep n */
    u16  tog_out[128];                  /* bit n = next DATAx to send on OUT ep n */
};

/* ------------------------------------------------------------ helpers */

static u8 rd(struct Lvhc *h, u8 r) { return hw_read8(h->bus, r); }
static void wr(struct Lvhc *h, u8 r, u8 v) { hw_write8(h->bus, r, v); }

static void q_append(struct UsbXfer **head, struct UsbXfer *x)
{
    x->next = NULL;
    while (*head)
        head = &(*head)->next;
    *head = x;
}

static int q_remove(struct UsbXfer **head, struct UsbXfer *x)
{
    for (; *head; head = &(*head)->next)
        if (*head == x) {
            *head = x->next;
            x->next = NULL;
            return 1;
        }
    return 0;
}

static void done_push(struct Lvhc *h, struct UsbXfer *x, int status)
{
    x->hcd_status = status;
    x->next = NULL;
    if (h->done_tail)
        h->done_tail->next = x;
    else
        h->done_head = x;
    h->done_tail = x;
}

static void flush_done(struct Lvhc *h)
{
    while (h->done_head) {
        struct UsbXfer *x = h->done_head;
        h->done_head = x->next;
        if (!h->done_head)
            h->done_tail = NULL;
        x->next = NULL;
        x->status = x->hcd_status;
        if (x->done)
            x->done(x);
    }
}

static void rh_complete_cb(void *ctx, struct UsbXfer *x, int status) { done_push(ctx, x, status); }

static void complete(struct Lvhc *h, struct UsbXfer *x, int status)
{
    q_remove(&h->q[x->type & 3], x);
    if (h->cur == x)
        h->cur = NULL;
    x->actual = x->offset;
    done_push(h, x, status);
}

static int get_tog(struct Lvhc *h, struct UsbXfer *x, int in)
{
    u16 *t = in ? h->tog_in : h->tog_out;
    return (t[x->devaddr] >> x->endpoint) & 1;
}

static void set_tog(struct Lvhc *h, u8 addr, u8 ep, int in, int v)
{
    u16 *t = in ? h->tog_in : h->tog_out;
    if (v) t[addr] |= (u16)(1u << ep);
    else   t[addr] &= (u16)~(1u << ep);
}

/* Toggle bookkeeping for standard requests that reset device toggles. */
static void snoop_control(struct Lvhc *h, struct UsbXfer *x)
{
    const u8 *s = x->setup;
    u16 value = lu_get_le16(s + 2), index = lu_get_le16(s + 4);
    if (s[0] == USB_RECIP_DEVICE && s[1] == USB_REQ_SET_ADDRESS && value < 128) {
        h->tog_in[value] = h->tog_out[value] = 0;
    } else if ((s[0] == USB_RECIP_DEVICE && s[1] == USB_REQ_SET_CONFIGURATION) ||
               (s[0] == USB_RECIP_INTERFACE && s[1] == USB_REQ_SET_INTERFACE)) {
        h->tog_in[x->devaddr] = h->tog_out[x->devaddr] = 0;
    } else if (s[0] == USB_RECIP_ENDPOINT && s[1] == USB_REQ_CLEAR_FEATURE &&
               value == USB_FEAT_ENDPOINT_HALT) {
        set_tog(h, x->devaddr, index & 0x0f, index & USB_EP_DIR_IN, 0);
    }
}

/* --------------------------------------------------- root port access */

static void port_select(struct Lvhc *h, int port) { wr(h, LVHC_PORTSEL, (u8)(port - 1)); }

static u16 rp_status(void *ctx, int port, u16 *change)
{
    struct Lvhc *h = ctx;
    u8 st, ch;
    u16 s = 0, c = 0;
    port_select(h, port);
    st = rd(h, LVHC_PORTSTAT);
    ch = rd(h, LVHC_PORTCHG);
    if (st & LVHC_PS_CONN)     s |= USB_PS_CONNECTION;
    if (st & LVHC_PS_ENABLE)   s |= USB_PS_ENABLE;
    if (st & LVHC_PS_SUSPEND)  s |= USB_PS_SUSPEND;
    if (st & LVHC_PS_OVERCUR)  s |= USB_PS_OVER_CURRENT;
    if (st & LVHC_PS_RESET)    s |= USB_PS_RESET;
    if (st & LVHC_PS_POWER)    s |= USB_PS_POWER;
    if (st & LVHC_PS_LOWSPEED) s |= USB_PS_LOW_SPEED;
    if (ch & LVHC_PC_CONN)     c |= USB_PC_CONNECTION;
    if (ch & LVHC_PC_ENABLE)   c |= USB_PC_ENABLE;
    if (ch & LVHC_PC_SUSPEND)  c |= USB_PC_SUSPEND;
    if (ch & LVHC_PC_OVERCUR)  c |= USB_PC_OVER_CURRENT;
    if (ch & LVHC_PC_RESET)    c |= USB_PC_RESET;
    *change = c;
    return s;
}

static void rp_set_feature(void *ctx, int port, u16 feature)
{
    struct Lvhc *h = ctx;
    port_select(h, port);
    switch (feature) {
    case PORT_RESET:   wr(h, LVHC_PORTCMD, LVHC_PCMD_RESET); break;
    case PORT_POWER:   wr(h, LVHC_PORTCMD, LVHC_PCMD_POWERON); break;
    case PORT_SUSPEND: wr(h, LVHC_PORTCMD, LVHC_PCMD_SUSPEND); break;
    default: break;
    }
}

static void rp_clear_feature(void *ctx, int port, u16 feature)
{
    struct Lvhc *h = ctx;
    port_select(h, port);
    switch (feature) {
    case PORT_ENABLE:         wr(h, LVHC_PORTCMD, LVHC_PCMD_DISABLE); break;
    case PORT_SUSPEND:        wr(h, LVHC_PORTCMD, LVHC_PCMD_RESUME); break;
    case PORT_POWER:          wr(h, LVHC_PORTCMD, LVHC_PCMD_POWEROFF); break;
    case C_PORT_CONNECTION:   wr(h, LVHC_PORTCHG, LVHC_PC_CONN); break;
    case C_PORT_ENABLE:       wr(h, LVHC_PORTCHG, LVHC_PC_ENABLE); break;
    case C_PORT_SUSPEND:      wr(h, LVHC_PORTCHG, LVHC_PC_SUSPEND); break;
    case C_PORT_OVER_CURRENT: wr(h, LVHC_PORTCHG, LVHC_PC_OVERCUR); break;
    case C_PORT_RESET:        wr(h, LVHC_PORTCHG, LVHC_PC_RESET); break;
    default: break;
    }
}

static const struct RhPortOps lvhc_port_ops = { rp_status, rp_set_feature, rp_clear_feature };

/* ------------------------------------------------- transaction engine */

static struct UsbXfer *pick_next(struct Lvhc *h, u32 now)
{
    static const u8 order[3] = { XT_INT, XT_CONTROL, XT_BULK };
    int i;
    for (i = 0; i < 3; i++) {
        struct UsbXfer *x;
        for (x = h->q[order[i]]; x; x = x->next)
            if ((s32)(now - x->due_ms) >= 0)
                return x;
    }
    return NULL;
}

static void start_transaction(struct Lvhc *h, struct UsbXfer *x)
{
    u8 pid, tog, len = 0;
    const u8 *out = NULL;

    if (x->type == XT_CONTROL && x->phase == PH_SETUP) {
        pid = LVHC_PID_SETUP; tog = 0; len = 8; out = x->setup;
    } else if (x->type == XT_CONTROL && x->phase == PH_STATUS) {
        pid = (x->dir == XD_IN && x->length) ? LVHC_PID_OUT : LVHC_PID_IN;
        tog = 1;
    } else {
        int in = (x->dir == XD_IN);
        pid = in ? LVHC_PID_IN : LVHC_PID_OUT;
        tog = (u8)get_tog(h, x, in);
        len = (u8)LU_MIN((u32)x->maxpkt, x->length - x->offset);
        if (!in)
            out = x->data + x->offset;
    }
    wr(h, LVHC_XADDR, x->devaddr);
    wr(h, LVHC_XEP, x->endpoint);
    wr(h, LVHC_XPID, pid);
    wr(h, LVHC_XTOGGLE, tog);
    wr(h, LVHC_XLEN, (pid == LVHC_PID_IN && !(x->type == XT_CONTROL && x->phase == PH_STATUS))
                         ? (u8)x->maxpkt : len);
    wr(h, LVHC_XFLAGS, (x->flags & XF_LOWSPEED) ? LVHC_XF_LOWSPEED : 0);
    wr(h, LVHC_FIFOPTR, 0);
    if (out && len)
        h->bus->ops->write_fifo(h->bus, LVHC_FIFODATA, out, len);
    h->cur = x;
    h->cur_len = len;
    wr(h, LVHC_XCMD, LVHC_XCMD_GO);
}

/* Data stage (or whole interrupt/bulk transfer) finished. */
static void stage_done(struct Lvhc *h, struct UsbXfer *x)
{
    if (x->type == XT_CONTROL) {
        x->phase = PH_STATUS;
        return;
    }
    if (x->dir == XD_IN && x->offset < x->length && !(x->flags & XF_ALLOWRUNT))
        complete(h, x, XS_RUNTPACKET);
    else
        complete(h, x, XS_OK);
}

static void handle_result(struct Lvhc *h, struct UsbXfer *x, u8 res, u32 now)
{
    h->cur = NULL;
    switch (res) {
    case LVHC_RES_ACK:
        x->errors = 0;
        if (x->type == XT_CONTROL && x->phase == PH_SETUP) {
            set_tog(h, x->devaddr, 0, 1, 1);
            set_tog(h, x->devaddr, 0, 0, 1);
            x->phase = x->length ? PH_DATA : PH_STATUS;
            x->offset = 0;
        } else if (x->type == XT_CONTROL && x->phase == PH_STATUS) {
            int st = XS_OK;
            if (x->dir == XD_IN && x->offset < x->length && !(x->flags & XF_ALLOWRUNT))
                st = XS_RUNTPACKET;
            if (st == XS_OK)
                snoop_control(h, x);
            complete(h, x, st);
        } else if (x->dir == XD_IN) {
            u8 n = rd(h, LVHC_XACTUAL);
            u8 rt = rd(h, LVHC_XRTOGGLE);
            if (rt != get_tog(h, x, 1)) {
                LU_DBG("lvhc: toggle mismatch addr %u ep %u, packet ignored",
                       x->devaddr, x->endpoint);
                break;      /* duplicate packet: data already received */
            }
            if (x->offset + n > x->length) {
                complete(h, x, XS_OVERFLOW);
                break;
            }
            h->bus->ops->read_fifo(h->bus, LVHC_FIFODATA, x->data + x->offset, n);
            x->offset += n;
            set_tog(h, x->devaddr, x->endpoint, 1, !get_tog(h, x, 1));
            if (n < x->maxpkt || x->offset >= x->length)
                stage_done(h, x);
        } else {
            x->offset += h->cur_len;
            set_tog(h, x->devaddr, x->endpoint, 0, !get_tog(h, x, 0));
            if (x->offset >= x->length)
                stage_done(h, x);
        }
        break;
    case LVHC_RES_NAK:
        if ((x->flags & XF_NAKTIMEOUT) && (now - x->start_ms) >= x->nak_timeout) {
            complete(h, x, XS_NAKTIMEOUT);
            break;
        }
        x->due_ms = now + ((x->type == XT_INT) ? LU_MAX(x->interval, 1) : 1);
        break;
    case LVHC_RES_STALL:
        complete(h, x, XS_STALL);
        break;
    case LVHC_RES_BABBLE:
        complete(h, x, XS_OVERFLOW);
        break;
    default:            /* timeout, CRC: retry a few times */
        if (++x->errors >= MAX_ERRORS)
            complete(h, x, res == LVHC_RES_CRC ? XS_CRCERROR : XS_TIMEOUT);
        else
            x->due_ms = now + 1;
        break;
    }
}

/* -------------------------------------------------------- HcdOps */

static int lvhc_irq(void *data)
{
    struct Lvhc *h = data;
    h->irq_pending = 1;
    if (h->hcd.wake)
        h->hcd.wake(h->hcd.wake_ctx);
    return 1;
}

static void fail_all(struct Lvhc *h, int status)
{
    int t;
    h->cur = NULL;
    for (t = 0; t < 4; t++)
        while (h->q[t])
            complete(h, h->q[t], status);
    if (h->rh.status_xfer)
        rh_abort(&h->rh, h->rh.status_xfer);
}

static int lvhc_start(struct Hcd *hcd)
{
    struct Lvhc *h = hcd->priv;
    if (rd(h, LVHC_ID) != LVHC_ID_VALUE) {
        LU_ERR("lvhc: chip not found");
        return 0;
    }
    wr(h, LVHC_CTRL, LVHC_CTRL_RESET);
    h->nports = rd(h, LVHC_NPORTS);
    rh_init(&h->rh, h->nports, &lvhc_port_ops, h, rh_complete_cb, h, "LVHC virtual root hub");
    h->bus->ops->add_irq(h->bus, lvhc_irq, h);
    wr(h, LVHC_INTSTAT, 0xff);
    wr(h, LVHC_INTMASK, LVHC_INT_XDONE | LVHC_INT_PORT);
    wr(h, LVHC_CTRL, LVHC_CTRL_ENABLE | LVHC_CTRL_INTEN);
    h->running = 1;
    LU_INFO("lvhc: started, %u root ports", h->nports);
    return 1;
}

static void lvhc_stop(struct Hcd *hcd)
{
    struct Lvhc *h = hcd->priv;
    if (!h->running)
        return;
    h->running = 0;
    if (h->bus->ops->present(h->bus)) {
        wr(h, LVHC_CTRL, 0);
        h->bus->ops->rem_irq(h->bus);
    }
    fail_all(h, XS_OFFLINE);
    flush_done(h);
}

static int lvhc_submit(struct Hcd *hcd, struct UsbXfer *x)
{
    struct Lvhc *h = hcd->priv;
    u32 now = os_time_ms();
    x->actual = 0;
    x->offset = 0;
    x->errors = 0;
    x->phase = (x->type == XT_CONTROL) ? PH_SETUP : PH_DATA;
    x->start_ms = x->due_ms = now;
    x->next = NULL;
    if (x->type == XT_CONTROL && !x->maxpkt)
        x->maxpkt = 8;
    x->status = XS_PENDING;
    if (!h->running) {
        done_push(h, x, XS_OFFLINE);
    } else if (x->devaddr > 127 || x->endpoint > 15 || x->type == XT_ISO ||
               x->maxpkt == 0 || x->maxpkt > LVHC_FIFO_SIZE ||
               (x->length && !x->data)) {
        done_push(h, x, x->type == XT_ISO ? XS_BADPARAMS :
                        x->maxpkt > LVHC_FIFO_SIZE ? XS_PKTTOOLARGE : XS_BADPARAMS);
    } else {
        if (rh_is_target(&h->rh, x))
            rh_submit(&h->rh, x);
        else
            q_append(&h->q[x->type & 3], x);
    }
    if (hcd->wake)
        hcd->wake(hcd->wake_ctx);
    return 1;
}

static int lvhc_abort(struct Hcd *hcd, struct UsbXfer *x)
{
    struct Lvhc *h = hcd->priv;
    if (rh_abort(&h->rh, x))
        return 1;
    if (q_remove(&h->q[x->type & 3], x)) {
        if (h->cur == x)
            h->cur = NULL;
        x->actual = x->offset;
        done_push(h, x, XS_ABORTED);
        return 1;
    }
    return 0;
}

static void lvhc_poll(struct Hcd *hcd)
{
    struct Lvhc *h = hcd->priv;
    u32 now = os_time_ms();
    u8 istat;
    int n;

    if (!h->running) {
        flush_done(h);
        return;
    }
    if (!h->bus->ops->present(h->bus)) {
        LU_WARN("lvhc: hardware removed");
        h->running = 0;
        fail_all(h, XS_OFFLINE);
        flush_done(h);
        return;
    }
    h->irq_pending = 0;
    istat = rd(h, LVHC_INTSTAT);
    if (istat)
        wr(h, LVHC_INTSTAT, istat);
    if (istat & LVHC_INT_PORT)
        rh_changed(&h->rh);

    for (n = 0; n < MAX_TX_PER_POLL; n++) {
        struct UsbXfer *x = h->cur;
        if (x) {
            u8 res = rd(h, LVHC_XRESULT);
            if (res == LVHC_RES_BUSY)
                break;              /* real chips: wait for the XDONE interrupt */
            handle_result(h, x, res, now);
            continue;
        }
        x = pick_next(h, now);
        if (!x)
            break;
        start_transaction(h, x);
    }
    /* acknowledge XDONE raised by the transactions just executed */
    istat = rd(h, LVHC_INTSTAT) & LVHC_INT_XDONE;
    if (istat)
        wr(h, LVHC_INTSTAT, istat);
    flush_done(h);
}

static u16 lvhc_frame(struct Hcd *hcd)
{
    struct Lvhc *h = hcd->priv;
    u8 lo = rd(h, LVHC_FRAMELO);
    return (u16)(lo | ((rd(h, LVHC_FRAMEHI) & 7) << 8));
}

static void lvhc_bus_reset(struct Hcd *hcd)
{
    struct Lvhc *h = hcd->priv;
    int i;
    fail_all(h, XS_ABORTED);
    for (i = 1; i <= h->nports; i++)
        rp_clear_feature(h, i, PORT_ENABLE);
    lu_memset(h->tog_in, 0, sizeof(h->tog_in));
    lu_memset(h->tog_out, 0, sizeof(h->tog_out));
    rh_reset(&h->rh);
    flush_done(h);
}

static const struct HcdOps lvhc_ops = {
    "lvhc", lvhc_start, lvhc_stop, lvhc_submit, lvhc_abort, lvhc_poll, lvhc_frame, lvhc_bus_reset
};

struct Hcd *lvhc_create(struct HwBus *bus)
{
    struct Lvhc *h = os_alloc(sizeof(*h));
    if (!h)
        return NULL;
    h->bus = bus;
    h->hcd.ops = &lvhc_ops;
    h->hcd.bus = bus;
    h->hcd.priv = h;
    return &h->hcd;
}

void lvhc_destroy(struct Hcd *hcd)
{
    if (!hcd)
        return;
    lvhc_stop(hcd);
    os_free(hcd->priv, sizeof(struct Lvhc));
}
