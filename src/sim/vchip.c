/*
 * LupoUsbDriver - simulation of the LVHC host controller chip.
 *
 * Register semantics are documented in include/lupousb/lvhc_regs.h.
 * A transaction started with XCMD=GO completes immediately (the simulated
 * bus is infinitely fast); the result is visible in XRESULT and signalled
 * with INT_XDONE, exactly as a real chip would do a few microseconds later.
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/vsim.h"
#include "lupousb/usbspec.h"
#include "lupousb/os.h"

static void check_irq(struct VChip *c, u8 newbits)
{
    if ((newbits & c->intmask) && (c->ctrl & LVHC_CTRL_INTEN) && c->raise_irq)
        c->raise_irq(c->irq_ctx);
}

/* Like a real interrupt line: the owner is notified only when a status bit
 * goes from 0 to 1, not again while it is still pending. (Raising it at
 * every register access kept the lupovirt.device unit task, priority 5,
 * waking itself up forever and starved the USB stack task.) */
static void set_int(struct VChip *c, u8 bits)
{
    u8 newbits = (u8)(bits & ~c->intstat);
    c->intstat |= bits;
    if (newbits)
        check_irq(c, newbits);
}

void vchip_init(struct VChip *c, u8 nports)
{
    lu_memset(c, 0, sizeof(*c));
    c->nports = (u8)LU_MIN(nports, LVHC_MAX_PORTS);
    c->xresult = LVHC_RES_ACK;
}

/* Calls the tick handler of every device reachable from a port array
 * (hubs advance the reset timers of their own ports in their tick). */
static void tick_tree(struct VPort *ports, int n, u32 now)
{
    int i;
    for (i = 0; i < n; i++) {
        struct VDev *d = ports[i].dev;
        if (!d)
            continue;
        if (d->ops && d->ops->tick)
            d->ops->tick(d, now);
        if (d->hub)
            tick_tree(d->hub->port, d->hub->nports, now);
    }
}

void vchip_advance(struct VChip *c, u32 now)
{
    int i, changed = 0;
    if (c->last_ms == 0)
        c->last_ms = now;
    if ((s32)(now - c->last_ms) <= 0)
        return;
    if (c->ctrl & LVHC_CTRL_ENABLE)
        c->frame = (u16)((c->frame + (now - c->last_ms)) & 0x7ff);
    c->last_ms = now;
    c->now = now;
    for (i = 0; i < c->nports; i++)
        changed |= vport_tick(&c->port[i], now);
    tick_tree(c->port, c->nports, now);
    /* only NEW change bits raise the port interrupt */
    for (i = 0; i < c->nports; i++) {
        if (c->port[i].change & ~c->port_seen[i])
            changed = 1;
        c->port_seen[i] = c->port[i].change;
    }
    if (changed)
        set_int(c, LVHC_INT_PORT);
    if (c->ctrl & LVHC_CTRL_ENABLE)
        set_int(c, LVHC_INT_SOF);
}

static u8 port_stat_reg(const struct VPort *p)
{
    u8 r = 0;
    if (p->status & USB_PS_CONNECTION)   r |= LVHC_PS_CONN;
    if (p->status & USB_PS_ENABLE)       r |= LVHC_PS_ENABLE;
    if (p->status & USB_PS_LOW_SPEED)    r |= LVHC_PS_LOWSPEED;
    if (p->status & USB_PS_RESET)        r |= LVHC_PS_RESET;
    if (p->status & USB_PS_SUSPEND)      r |= LVHC_PS_SUSPEND;
    if (p->status & USB_PS_POWER)        r |= LVHC_PS_POWER;
    if (p->status & USB_PS_OVER_CURRENT) r |= LVHC_PS_OVERCUR;
    return r;
}

static u8 port_chg_reg(const struct VPort *p)
{
    u8 r = 0;
    if (p->change & USB_PC_CONNECTION)   r |= LVHC_PC_CONN;
    if (p->change & USB_PC_ENABLE)       r |= LVHC_PC_ENABLE;
    if (p->change & USB_PC_RESET)        r |= LVHC_PC_RESET;
    if (p->change & USB_PC_SUSPEND)      r |= LVHC_PC_SUSPEND;
    if (p->change & USB_PC_OVER_CURRENT) r |= LVHC_PC_OVERCUR;
    return r;
}

static void do_transaction(struct VChip *c)
{
    struct VDev *d;
    u8 buf[LVHC_FIFO_SIZE];
    u16 actual = 0;
    u8 rtog = 0;
    int r;

    c->transactions++;
    c->xactual = 0;
    if (!(c->ctrl & LVHC_CTRL_ENABLE) || c->xlen > LVHC_FIFO_SIZE) {
        c->xresult = LVHC_RES_TIMEOUT;
        goto done;
    }
    d = vsim_find(c, c->xaddr, c->now);
    /* No device, or wrong signalling speed: no handshake at all. */
    if (!d || ((c->xflags & LVHC_XF_LOWSPEED) ? 1 : 0) != (d->lowspeed ? 1 : 0)) {
        c->xresult = LVHC_RES_TIMEOUT;
        goto done;
    }
    if (c->xpid == LVHC_PID_IN) {
        r = vdev_transaction(d, LVHC_PID_IN, c->xep, 0, buf, LVHC_FIFO_SIZE, &actual, &rtog);
        if (r == VR_ACK && actual > c->xlen) {
            c->xresult = LVHC_RES_BABBLE;
            goto done;
        }
        if (r == VR_ACK) {
            lu_memcpy(c->fifo, buf, actual);
            c->xactual = (u8)actual;
            c->xrtoggle = rtog;
        }
    } else {
        lu_memcpy(buf, c->fifo, c->xlen);
        r = vdev_transaction(d, c->xpid, c->xep, c->xtoggle, buf, c->xlen, &actual, &rtog);
    }
    c->xresult = (r == VR_ACK) ? LVHC_RES_ACK : (r == VR_NAK) ? LVHC_RES_NAK : LVHC_RES_STALL;
done:
    c->fifoptr = 0;
    set_int(c, LVHC_INT_XDONE);
}

u8 vchip_read(struct VChip *c, u8 reg)
{
    struct VPort *p = &c->port[c->portsel < c->nports ? c->portsel : 0];
    switch (reg) {
    case LVHC_ID:       return LVHC_ID_VALUE;
    case LVHC_REV:      return LVHC_REV_VALUE;
    case LVHC_CTRL:     return c->ctrl;
    case LVHC_INTSTAT:  return c->intstat;
    case LVHC_INTMASK:  return c->intmask;
    case LVHC_NPORTS:   return c->nports;
    case LVHC_PORTSEL:  return c->portsel;
    case LVHC_PORTSTAT: return port_stat_reg(p);
    case LVHC_PORTCHG:  return port_chg_reg(p);
    case LVHC_FRAMELO:  return (u8)c->frame;
    case LVHC_FRAMEHI:  return (u8)(c->frame >> 8);
    case LVHC_XADDR:    return c->xaddr;
    case LVHC_XEP:      return c->xep;
    case LVHC_XPID:     return c->xpid;
    case LVHC_XTOGGLE:  return c->xtoggle;
    case LVHC_XLEN:     return c->xlen;
    case LVHC_XFLAGS:   return c->xflags;
    case LVHC_XRESULT:  return c->xresult;
    case LVHC_XACTUAL:  return c->xactual;
    case LVHC_XRTOGGLE: return c->xrtoggle;
    case LVHC_FIFODATA: {
        u8 v = c->fifo[c->fifoptr % LVHC_FIFO_SIZE];
        c->fifoptr = (u8)((c->fifoptr + 1) % LVHC_FIFO_SIZE);
        return v;
    }
    default:            return 0xff;
    }
}

void vchip_write(struct VChip *c, u8 reg, u8 val)
{
    struct VPort *p = &c->port[c->portsel < c->nports ? c->portsel : 0];
    switch (reg) {
    case LVHC_CTRL:
        if (val & LVHC_CTRL_RESET) {
            int i;
            c->ctrl = 0; c->intstat = 0; c->intmask = 0; c->frame = 0;
            for (i = 0; i < c->nports; i++) {
                struct VDev *d = c->port[i].dev;
                c->port[i].status = 0;
                c->port[i].change = 0;
                c->port[i].dev = d;
            }
            return;
        }
        c->ctrl = val & (LVHC_CTRL_ENABLE | LVHC_CTRL_INTEN);
        check_irq(c, c->intstat);
        break;
    case LVHC_INTSTAT:  c->intstat &= (u8)~val; break;
    case LVHC_INTMASK:  c->intmask = val; check_irq(c, c->intstat); break;
    case LVHC_PORTSEL:  c->portsel = val; break;
    case LVHC_PORTCHG:
        if (val & LVHC_PC_CONN)    p->change &= (u16)~USB_PC_CONNECTION;
        if (val & LVHC_PC_ENABLE)  p->change &= (u16)~USB_PC_ENABLE;
        if (val & LVHC_PC_RESET)   p->change &= (u16)~USB_PC_RESET;
        if (val & LVHC_PC_SUSPEND) p->change &= (u16)~USB_PC_SUSPEND;
        if (val & LVHC_PC_OVERCUR) p->change &= (u16)~USB_PC_OVER_CURRENT;
        break;
    case LVHC_PORTCMD:
        switch (val) {
        case LVHC_PCMD_RESET:    vport_set_feature(p, PORT_RESET, c->now); break;
        case LVHC_PCMD_DISABLE:  vport_clear_feature(p, PORT_ENABLE); break;
        case LVHC_PCMD_SUSPEND:  vport_set_feature(p, PORT_SUSPEND, c->now); break;
        case LVHC_PCMD_RESUME:   vport_clear_feature(p, PORT_SUSPEND); break;
        case LVHC_PCMD_POWERON:  vport_set_feature(p, PORT_POWER, c->now); break;
        case LVHC_PCMD_POWEROFF: vport_clear_feature(p, PORT_POWER); break;
        default: break;
        }
        if (p->change)
            set_int(c, LVHC_INT_PORT);
        break;
    case LVHC_XADDR:    c->xaddr = val & 0x7f; break;
    case LVHC_XEP:      c->xep = val & 0x0f; break;
    case LVHC_XPID:     c->xpid = val; break;
    case LVHC_XTOGGLE:  c->xtoggle = val & 1; break;
    case LVHC_XLEN:     c->xlen = val; break;
    case LVHC_XFLAGS:   c->xflags = val; break;
    case LVHC_XCMD:     if (val & LVHC_XCMD_GO) do_transaction(c); break;
    case LVHC_FIFOPTR:  c->fifoptr = 0; break;
    case LVHC_FIFODATA:
        c->fifo[c->fifoptr % LVHC_FIFO_SIZE] = val;
        c->fifoptr = (u8)((c->fifoptr + 1) % LVHC_FIFO_SIZE);
        break;
    default:
        break;
    }
}
