/*
 * LupoUsbDriver - software simulation of the USB hardware (development only).
 *
 *   VChip  : the LVHC host controller chip (registers, root ports, SIE)
 *   VPort  : a downstream port (root port or port of a simulated hub)
 *   VDev   : a simulated USB device (generic chapter 9 engine + callbacks)
 *   vdev_* : concrete devices: hub, HID mouse, HID keyboard, HID gamepad
 *
 * The simulation is driven by a millisecond clock (os_time_ms()): time is
 * advanced lazily whenever the chip is accessed through L0.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_VSIM_H
#define LUPOUSB_VSIM_H

#include "lupousb/types.h"
#include "lupousb/lvhc_regs.h"

/* Handshake returned by device endpoint callbacks */
enum { VR_ACK = 0, VR_NAK = 1, VR_STALL = 2 };

struct VDev;
struct VPort;

struct VDevOps {
    /* Class/vendor control request. For IN requests fill buf and *len;
     * OUT data (if any) is in buf/len when called. Return VR_ACK or VR_STALL. */
    int  (*control)(struct VDev *d, const u8 *setup, u8 *buf, u16 *len);
    /* Non-control endpoints */
    int  (*ep_in)(struct VDev *d, u8 ep, u8 *buf, u16 max, u16 *len);
    int  (*ep_out)(struct VDev *d, u8 ep, const u8 *buf, u16 len);
    void (*tick)(struct VDev *d, u32 now);      /* optional */
    void (*bus_reset)(struct VDev *d);          /* optional */
};

#define VDEV_CTL_BUF 512

struct VDev {
    const struct VDevOps *ops;
    const char *name;
    const u8 *dev_desc;             /* 18 bytes */
    const u8 *cfg_desc;
    u16  cfg_len;
    const char *const *strings;     /* index 1..nstrings, ASCII */
    u8   nstrings;
    const u8 *report_desc;          /* HID report descriptor (if any) */
    u16  report_len;
    u8   lowspeed;
    struct VHub *hub;               /* non-NULL for hub devices */
    void *priv;

    /* --- device state (managed by the generic engine) --- */
    u8   addr;
    u8   pending_addr;
    u8   config;
    u8   attached;
    u8   tog_in;                    /* bit n: next DATAx for IN on ep n */
    u8   tog_out_lo;                /* bits 0..7: expected DATAx for OUT ep n */
    u16  halt_in, halt_out;
    /* control transfer state */
    u8   setup[8];
    u8   ctl_state;
    u8   ctl_buf[VDEV_CTL_BUF];
    u16  ctl_len, ctl_pos;
    u32  stats_tx, stats_rx;
};

/* A downstream port. Status/change use the hub class bit layout (USB_PS_*,
 * USB_PC_*); the chip converts them to its own register format. */
struct VPort {
    struct VDev *dev;
    u16  status;
    u16  change;
    u32  reset_until;
};

struct VHub {
    u8   nports;
    struct VPort port[7];
    u8   hub_change;
};

#define VCHIP_EVQ 0
struct VChip {
    u8   ctrl, intstat, intmask, portsel;
    u8   xaddr, xep, xpid, xtoggle, xlen, xflags, xresult, xactual, xrtoggle;
    u8   fifo[LVHC_FIFO_SIZE];
    u8   fifoptr;
    u8   nports;
    struct VPort port[LVHC_MAX_PORTS];
    u16  port_seen[LVHC_MAX_PORTS];  /* change bits already signalled */
    u32  now;
    u32  last_ms;
    u16  frame;
    /* interrupt line towards L0 */
    void (*raise_irq)(void *ctx);
    void *irq_ctx;
    u32  transactions;
};

void vchip_init(struct VChip *c, u8 nports);
void vchip_advance(struct VChip *c, u32 now_ms);
u8   vchip_read(struct VChip *c, u8 reg);
void vchip_write(struct VChip *c, u8 reg, u8 val);

/* Hot-plug helpers. 'hub' NULL means a root port of the chip. port is 0-based. */
void vsim_plug(struct VChip *c, struct VHub *hub, int port, struct VDev *d);
void vsim_unplug(struct VChip *c, struct VHub *hub, int port);

/* Port logic shared by root ports and simulated hubs */
void vport_attach(struct VPort *p, struct VDev *d);
void vport_detach(struct VPort *p);
void vport_set_feature(struct VPort *p, u16 feature, u32 now);
void vport_clear_feature(struct VPort *p, u16 feature);
int  vport_tick(struct VPort *p, u32 now);      /* returns 1 if change bits were set */

/* Generic device engine */
void vdev_init(struct VDev *d);
void vdev_bus_reset(struct VDev *d);
int  vdev_transaction(struct VDev *d, u8 pid, u8 ep, u8 toggle,
                      u8 *buf, u16 len, u16 *actual, u8 *rtoggle);
struct VDev *vsim_find(struct VChip *c, u8 addr, u32 now);

/* Concrete devices (src/sim/vdev_*.c). Each returns a ready VDev. */
struct VDev *vdev_hub_create(u8 nports);
struct VDev *vdev_mouse_create(int lowspeed);
struct VDev *vdev_keyboard_create(void);
struct VDev *vdev_gamepad_create(void);
void vdev_destroy(struct VDev *d);

/* Stimuli */
void vmouse_report(struct VDev *d, u8 buttons, s8 dx, s8 dy, s8 wheel);
void vkbd_report(struct VDev *d, u8 modifiers, const u8 keys[6]);
void vpad_report(struct VDev *d, u8 x, u8 y, u8 buttons);

#endif /* LUPOUSB_VSIM_H */
