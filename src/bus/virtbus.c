/*
 * LupoUsbDriver - Layer L0 backend: virtual bus.
 *
 * Connects an L1 driver to the software model of the LVHC chip instead of
 * real hardware. Every register access first advances the simulated time
 * of the chip to os_time_ms(), so ports/devices evolve even though nobody
 * drives a separate simulation thread.
 *
 * The interrupt line of the chip calls the handler installed by L1 directly
 * (synchronously). This is allowed by the L0 contract: an interrupt handler
 * may only acknowledge, set flags and wake the owner task.
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/virtbus.h"
#include "lupousb/os.h"

static struct VChip *chip_of(struct HwBus *b)
{
    struct VirtBus *vb = b->priv;
    /* Reading the clock at every register access is cheap on the host but
     * expensive on a 68k (GetSysTime() each time): an Amiga owner advances
     * the chip once per loop with virtbus_tick() instead. */
    if (!vb->manual_clock)
        vchip_advance(&vb->chip, os_time_ms());
    return &vb->chip;
}

void virtbus_tick(struct VirtBus *vb, u32 now)
{
    vchip_advance(&vb->chip, now);
}

static u8 vb_read8(struct HwBus *b, u32 reg) { return vchip_read(chip_of(b), (u8)reg); }
static void vb_write8(struct HwBus *b, u32 reg, u8 v) { vchip_write(chip_of(b), (u8)reg, v); }

/* LVHC is byte wide: 16-bit accesses are two byte accesses, low byte first. */
static u16 vb_read16(struct HwBus *b, u32 reg)
{
    u8 lo = vb_read8(b, reg), hi = vb_read8(b, reg + 1);
    return (u16)(lo | (hi << 8));
}

static void vb_write16(struct HwBus *b, u32 reg, u16 v)
{
    vb_write8(b, reg, (u8)v);
    vb_write8(b, reg + 1, (u8)(v >> 8));
}

static void vb_read_fifo(struct HwBus *b, u32 reg, u8 *dst, u32 len)
{
    struct VChip *c = chip_of(b);
    while (len--)
        *dst++ = vchip_read(c, (u8)reg);
}

static void vb_write_fifo(struct HwBus *b, u32 reg, const u8 *src, u32 len)
{
    struct VChip *c = chip_of(b);
    while (len--)
        vchip_write(c, (u8)reg, *src++);
}

static void vb_raise(void *ctx)
{
    struct VirtBus *vb = ctx;
    if (vb->irq_handler)
        vb->irq_handler(vb->irq_data);
}

static int vb_add_irq(struct HwBus *b, HwIrqHandler h, void *data)
{
    struct VirtBus *vb = b->priv;
    vb->irq_handler = h;
    vb->irq_data = data;
    vb->chip.raise_irq = vb_raise;
    vb->chip.irq_ctx = vb;
    return 1;
}

static void vb_rem_irq(struct HwBus *b)
{
    struct VirtBus *vb = b->priv;
    vb->chip.raise_irq = NULL;
    vb->irq_handler = NULL;
}

static int vb_present(struct HwBus *b)
{
    struct VirtBus *vb = b->priv;
    return !vb->removed;
}

static const struct HwBusOps virtbus_ops = {
    vb_read8, vb_write8, vb_read16, vb_write16,
    vb_read_fifo, vb_write_fifo, vb_add_irq, vb_rem_irq, vb_present
};

void virtbus_init(struct VirtBus *vb, u8 nports)
{
    lu_memset(vb, 0, sizeof(*vb));
    vchip_init(&vb->chip, nports);
    vb->bus.ops = &virtbus_ops;
    vb->bus.name = "virtbus";
    vb->bus.priv = vb;
}
