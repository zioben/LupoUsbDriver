/*
 * LupoUsbDriver - simulation: generic USB device engine and port logic.
 *
 * Implements what every USB device does by itself: the control transfer
 * state machine on endpoint 0 (SETUP / DATA / STATUS stages), the standard
 * chapter 9 requests, data toggles and endpoint halt. Concrete devices only
 * provide descriptors and class specific callbacks.
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/vsim.h"
#include "lupousb/usbspec.h"
#include "lupousb/os.h"

enum { CTL_IDLE = 0, CTL_DATA_IN, CTL_DATA_OUT, CTL_STATUS_IN, CTL_STATUS_OUT, CTL_STALL };

void vdev_init(struct VDev *d)
{
    d->addr = 0;
    d->pending_addr = 0xff;
    d->config = 0;
    d->tog_in = 0;
    d->tog_out_lo = 0;
    d->halt_in = d->halt_out = 0;
    d->ctl_state = CTL_IDLE;
}

void vdev_bus_reset(struct VDev *d)
{
    vdev_init(d);
    if (d->ops && d->ops->bus_reset)
        d->ops->bus_reset(d);
}

static u8 vdev_mps0(struct VDev *d) { return d->dev_desc[USB_DD_bMaxPacketSize0]; }

/* Finds a descriptor of the given type inside the configuration descriptor. */
static const u8 *find_cfg_desc(struct VDev *d, u8 type, u8 iface)
{
    u16 pos = 0;
    int in_iface = 0;
    while (pos + 2 <= d->cfg_len) {
        const u8 *p = d->cfg_desc + pos;
        if (p[0] < 2)
            break;
        if (p[1] == USB_DT_INTERFACE)
            in_iface = (p[USB_ID_bInterfaceNumber] == iface);
        else if (p[1] == type && in_iface)
            return p;
        pos += p[0];
    }
    return NULL;
}

static u16 make_string(struct VDev *d, u8 index, u8 *buf)
{
    if (index == 0) {
        buf[0] = 4; buf[1] = USB_DT_STRING; buf[2] = 0x09; buf[3] = 0x04;
        return 4;
    }
    if (index > d->nstrings || !d->strings)
        return 0;
    {
        const char *s = d->strings[index - 1];
        u16 n = 0;
        while (s[n] && n < 120) {
            buf[2 + n * 2] = (u8)s[n];
            buf[3 + n * 2] = 0;
            n++;
        }
        buf[0] = (u8)(2 + n * 2);
        buf[1] = USB_DT_STRING;
        return buf[0];
    }
}

/* Handles a request whose data stage (if OUT) is in d->ctl_buf.
 * For IN requests it fills d->ctl_buf/ctl_len. Returns VR_ACK or VR_STALL. */
static int handle_request(struct VDev *d)
{
    const u8 *s = d->setup;
    u8 rt = s[0], req = s[1];
    u16 value = lu_get_le16(s + 2), index = lu_get_le16(s + 4);
    u8 *buf = d->ctl_buf;

    if ((rt & 0x60) == USB_TYPE_STANDARD) {
        u8 recip = rt & 0x1f;
        switch (req) {
        case USB_REQ_GET_DESCRIPTOR: {
            u8 type = (u8)(value >> 8), idx = (u8)value;
            if (recip == USB_RECIP_DEVICE && type == USB_DT_DEVICE) {
                lu_memcpy(buf, d->dev_desc, USB_DT_DEVICE_SIZE);
                d->ctl_len = USB_DT_DEVICE_SIZE;
            } else if (recip == USB_RECIP_DEVICE && type == USB_DT_CONFIG && idx == 0) {
                lu_memcpy(buf, d->cfg_desc, d->cfg_len);
                d->ctl_len = d->cfg_len;
            } else if (recip == USB_RECIP_DEVICE && type == USB_DT_STRING) {
                d->ctl_len = make_string(d, idx, buf);
                if (!d->ctl_len)
                    return VR_STALL;
            } else if (recip == USB_RECIP_INTERFACE && type == USB_DT_REPORT && d->report_desc) {
                lu_memcpy(buf, d->report_desc, d->report_len);
                d->ctl_len = d->report_len;
            } else if (recip == USB_RECIP_INTERFACE && type == USB_DT_HID) {
                const u8 *h = find_cfg_desc(d, USB_DT_HID, (u8)index);
                if (!h)
                    return VR_STALL;
                lu_memcpy(buf, h, h[0]);
                d->ctl_len = h[0];
            } else if (d->ops->control) {
                return d->ops->control(d, s, buf, &d->ctl_len);   /* e.g. hub descriptor */
            } else {
                return VR_STALL;
            }
            return VR_ACK;
        }
        case USB_REQ_SET_ADDRESS:
            if (value > 127)
                return VR_STALL;
            d->pending_addr = (u8)value;      /* applied after the status stage */
            return VR_ACK;
        case USB_REQ_SET_CONFIGURATION:
            if (value != 0 && value != d->cfg_desc[USB_CD_bConfigurationValue])
                return VR_STALL;
            d->config = (u8)value;
            d->tog_in &= 1; d->tog_out_lo &= 1;
            d->halt_in = d->halt_out = 0;
            return VR_ACK;
        case USB_REQ_GET_CONFIGURATION:
            buf[0] = d->config;
            d->ctl_len = 1;
            return VR_ACK;
        case USB_REQ_SET_INTERFACE:
            return value == 0 ? VR_ACK : VR_STALL;
        case USB_REQ_GET_INTERFACE:
            buf[0] = 0;
            d->ctl_len = 1;
            return VR_ACK;
        case USB_REQ_GET_STATUS:
            buf[0] = buf[1] = 0;
            if (recip == USB_RECIP_ENDPOINT) {
                u8 ep = index & 0x0f;
                u16 h = (index & USB_EP_DIR_IN) ? d->halt_in : d->halt_out;
                buf[0] = (h >> ep) & 1;
            }
            d->ctl_len = 2;
            return VR_ACK;
        case USB_REQ_CLEAR_FEATURE:
        case USB_REQ_SET_FEATURE:
            if (recip == USB_RECIP_ENDPOINT && value == USB_FEAT_ENDPOINT_HALT) {
                u8 ep = index & 0x0f;
                if (req == USB_REQ_SET_FEATURE) {
                    if (index & USB_EP_DIR_IN) d->halt_in |= (u16)(1u << ep);
                    else d->halt_out |= (u16)(1u << ep);
                } else if (index & USB_EP_DIR_IN) {
                    d->halt_in &= (u16)~(1u << ep);
                    d->tog_in &= (u8)~(1u << ep);
                } else {
                    d->halt_out &= (u16)~(1u << ep);
                    d->tog_out_lo &= (u8)~(1u << ep);
                }
                return VR_ACK;
            }
            if (recip == USB_RECIP_DEVICE && value == USB_FEAT_DEVICE_REMOTE_WAKEUP)
                return VR_ACK;
            return VR_STALL;
        default:
            return VR_STALL;
        }
    }
    /* Class and vendor requests (e.g. hub port features, HID SET_IDLE) */
    if (d->ops->control)
        return d->ops->control(d, s, buf, &d->ctl_len);
    return VR_STALL;
}

int vdev_transaction(struct VDev *d, u8 pid, u8 ep, u8 toggle,
                     u8 *buf, u16 len, u16 *actual, u8 *rtoggle)
{
    *actual = 0;
    *rtoggle = 0;

    if (ep == 0) {
        u16 wlen;
        switch (pid) {
        case LVHC_PID_SETUP:
            if (len != 8)
                return VR_STALL;
            lu_memcpy(d->setup, buf, 8);
            d->tog_in |= 1;
            d->tog_out_lo |= 1;
            d->ctl_len = 0;
            d->ctl_pos = 0;
            wlen = lu_get_le16(d->setup + 6);
            if (d->setup[0] & USB_DIR_IN) {
                if (handle_request(d) != VR_ACK) {
                    d->ctl_state = CTL_STALL;
                } else {
                    if (d->ctl_len > wlen)
                        d->ctl_len = wlen;
                    d->ctl_state = wlen ? CTL_DATA_IN : CTL_STATUS_OUT;
                }
            } else if (wlen) {
                d->ctl_state = CTL_DATA_OUT;
            } else {
                d->ctl_state = (handle_request(d) == VR_ACK) ? CTL_STATUS_IN : CTL_STALL;
            }
            d->stats_rx++;
            return VR_ACK;      /* SETUP is always acknowledged */

        case LVHC_PID_IN:
            if (d->ctl_state == CTL_DATA_IN) {
                u16 n = (u16)LU_MIN((u16)(d->ctl_len - d->ctl_pos), (u16)vdev_mps0(d));
                lu_memcpy(buf, d->ctl_buf + d->ctl_pos, n);
                d->ctl_pos = (u16)(d->ctl_pos + n);
                *actual = n;
                *rtoggle = d->tog_in & 1;
                d->tog_in ^= 1;
                d->stats_tx++;
                return VR_ACK;
            }
            if (d->ctl_state == CTL_DATA_OUT) {
                /* status stage of a control write: execute the request now */
                if (handle_request(d) != VR_ACK) {
                    d->ctl_state = CTL_STALL;
                    return VR_STALL;
                }
                d->ctl_state = CTL_STATUS_IN;
            }
            if (d->ctl_state == CTL_STATUS_IN) {
                *rtoggle = 1;
                d->ctl_state = CTL_IDLE;
                if (d->pending_addr != 0xff) {
                    d->addr = d->pending_addr;
                    d->pending_addr = 0xff;
                }
                return VR_ACK;
            }
            return VR_STALL;

        case LVHC_PID_OUT:
            if (d->ctl_state == CTL_DATA_OUT) {
                wlen = lu_get_le16(d->setup + 6);
                if ((toggle & 1) == (d->tog_out_lo & 1)) {
                    u16 n = (u16)LU_MIN(len, (u16)(VDEV_CTL_BUF - d->ctl_len));
                    lu_memcpy(d->ctl_buf + d->ctl_len, buf, n);
                    d->ctl_len = (u16)(d->ctl_len + n);
                    d->tog_out_lo ^= 1;
                }
                (void)wlen;
                return VR_ACK;
            }
            if (d->ctl_state == CTL_DATA_IN || d->ctl_state == CTL_STATUS_OUT) {
                d->ctl_state = CTL_IDLE;    /* status stage of a control read */
                return VR_ACK;
            }
            return VR_STALL;
        default:
            return VR_STALL;
        }
    }

    /* Non-control endpoints exist only in the configured state */
    if (!d->config)
        return VR_STALL;
    if (pid == LVHC_PID_IN) {
        u16 n = 0;
        int r;
        if (d->halt_in & (1u << ep))
            return VR_STALL;
        if (!d->ops->ep_in)
            return VR_STALL;
        r = d->ops->ep_in(d, ep, buf, len, &n);
        if (r == VR_ACK) {
            *actual = n;
            *rtoggle = (d->tog_in >> ep) & 1;
            d->tog_in ^= (u8)(1u << ep);
            d->stats_tx++;
        }
        return r;
    }
    if (pid == LVHC_PID_OUT) {
        int r = VR_ACK;
        if (d->halt_out & (1u << ep))
            return VR_STALL;
        if ((toggle & 1) != ((d->tog_out_lo >> ep) & 1))
            return VR_ACK;      /* retransmission of a packet already received */
        if (d->ops->ep_out)
            r = d->ops->ep_out(d, ep, buf, len);
        if (r == VR_ACK)
            d->tog_out_lo ^= (u8)(1u << ep);
        return r;
    }
    return VR_STALL;
}

/* ---------------------------------------------------------------- ports */

void vport_attach(struct VPort *p, struct VDev *d)
{
    p->dev = d;
    d->attached = 1;
    vdev_init(d);
    if (p->status & USB_PS_POWER) {
        p->status |= USB_PS_CONNECTION;
        if (d->lowspeed) p->status |= USB_PS_LOW_SPEED;
        else p->status &= (u16)~USB_PS_LOW_SPEED;
        p->change |= USB_PC_CONNECTION;
    }
}

void vport_detach(struct VPort *p)
{
    if (!p->dev)
        return;
    p->dev->attached = 0;
    p->dev = NULL;
    if (p->status & USB_PS_CONNECTION)
        p->change |= USB_PC_CONNECTION;
    p->status &= (u16)~(USB_PS_CONNECTION | USB_PS_ENABLE | USB_PS_LOW_SPEED |
                        USB_PS_RESET | USB_PS_SUSPEND);
}

void vport_set_feature(struct VPort *p, u16 feature, u32 now)
{
    switch (feature) {
    case PORT_POWER:
        if (!(p->status & USB_PS_POWER)) {
            p->status |= USB_PS_POWER;
            if (p->dev) {
                p->status |= USB_PS_CONNECTION;
                if (p->dev->lowspeed) p->status |= USB_PS_LOW_SPEED;
                p->change |= USB_PC_CONNECTION;
            }
        }
        break;
    case PORT_RESET:
        if (p->status & USB_PS_CONNECTION) {
            p->status |= USB_PS_RESET;
            p->status &= (u16)~USB_PS_ENABLE;
            p->reset_until = now + USB_T_DRSTR;
        }
        break;
    case PORT_SUSPEND:
        if (p->status & USB_PS_ENABLE)
            p->status |= USB_PS_SUSPEND;
        break;
    default:
        break;
    }
}

void vport_clear_feature(struct VPort *p, u16 feature)
{
    switch (feature) {
    case PORT_ENABLE:     p->status &= (u16)~USB_PS_ENABLE; break;
    case PORT_SUSPEND:    if (p->status & USB_PS_SUSPEND) {
                              p->status &= (u16)~USB_PS_SUSPEND;
                              p->change |= USB_PC_SUSPEND;
                          }
                          break;
    case PORT_POWER:      p->status &= (u16)~(USB_PS_POWER | USB_PS_CONNECTION |
                                              USB_PS_ENABLE | USB_PS_LOW_SPEED);
                          break;
    case C_PORT_CONNECTION:   p->change &= (u16)~USB_PC_CONNECTION; break;
    case C_PORT_ENABLE:       p->change &= (u16)~USB_PC_ENABLE; break;
    case C_PORT_SUSPEND:      p->change &= (u16)~USB_PC_SUSPEND; break;
    case C_PORT_OVER_CURRENT: p->change &= (u16)~USB_PC_OVER_CURRENT; break;
    case C_PORT_RESET:        p->change &= (u16)~USB_PC_RESET; break;
    default: break;
    }
}

int vport_tick(struct VPort *p, u32 now)
{
    if ((p->status & USB_PS_RESET) && (s32)(now - p->reset_until) >= 0) {
        p->status &= (u16)~USB_PS_RESET;
        if (p->dev) {
            p->status |= USB_PS_ENABLE;
            vdev_bus_reset(p->dev);
        }
        p->change |= USB_PC_RESET;
        return 1;
    }
    return 0;
}

/* Address routing through root ports and enabled hub ports. */
static struct VDev *find_in_ports(struct VPort *ports, int n, u8 addr)
{
    int i;
    for (i = 0; i < n; i++) {
        struct VDev *d = ports[i].dev;
        if (!d || !(ports[i].status & USB_PS_ENABLE) || (ports[i].status & USB_PS_SUSPEND))
            continue;
        if (d->addr == addr)
            return d;
        if (d->hub && d->config) {
            struct VDev *r = find_in_ports(d->hub->port, d->hub->nports, addr);
            if (r)
                return r;
        }
    }
    return NULL;
}

struct VDev *vsim_find(struct VChip *c, u8 addr, u32 now)
{
    (void)now;
    return find_in_ports(c->port, c->nports, addr);
}

void vsim_plug(struct VChip *c, struct VHub *hub, int port, struct VDev *d)
{
    struct VPort *p = hub ? &hub->port[port] : &c->port[port];
    if (p->dev)
        vport_detach(p);
    vport_attach(p, d);
    if (!hub)
        c->intstat |= LVHC_INT_PORT;
}

void vsim_unplug(struct VChip *c, struct VHub *hub, int port)
{
    struct VPort *p = hub ? &hub->port[port] : &c->port[port];
    vport_detach(p);
    if (!hub)
        c->intstat |= LVHC_INT_PORT;
}

void vdev_destroy(struct VDev *d)
{
    if (!d)
        return;
    if (d->priv)
        os_free(d->priv, 0);
    if (d->hub)
        os_free(d->hub, sizeof(*d->hub));
    os_free(d, sizeof(*d));
}
