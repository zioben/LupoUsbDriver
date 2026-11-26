/*
 * LupoUsbDriver - Layer L0: bus access.
 *
 * L0 hides how the host controller chip is reached: the PCMCIA slot of the
 * A1200 (Gayle), a clockport, a PCI busboard or - for development - a
 * software model of a chip (src/bus/virtbus.c). L0 knows nothing about USB.
 *
 * L1 drivers access their chip only through these operations, so the same
 * L1 driver works on every bus that can reach that chip.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_HWBUS_H
#define LUPOUSB_HWBUS_H

#include "lupousb/types.h"

struct HwBus;

/* Interrupt handler installed by L1. Called in interrupt context on real
 * hardware: it may only read/acknowledge chip registers, set flags and
 * wake the owning task. It returns non-zero if the interrupt was ours. */
typedef int (*HwIrqHandler)(void *data);

struct HwBusOps {
    u8   (*read8)(struct HwBus *bus, u32 reg);
    void (*write8)(struct HwBus *bus, u32 reg, u8 val);
    u16  (*read16)(struct HwBus *bus, u32 reg);
    void (*write16)(struct HwBus *bus, u32 reg, u16 val);
    /* Block transfers to/from a data port (FIFO register): every byte goes
     * to/comes from the same register address. */
    void (*read_fifo)(struct HwBus *bus, u32 reg, u8 *dst, u32 len);
    void (*write_fifo)(struct HwBus *bus, u32 reg, const u8 *src, u32 len);
    int  (*add_irq)(struct HwBus *bus, HwIrqHandler handler, void *data);
    void (*rem_irq)(struct HwBus *bus);
    /* 0 if the card was removed: L1 must stop touching the chip. */
    int  (*present)(struct HwBus *bus);
};

struct HwBus {
    const struct HwBusOps *ops;
    const char *name;
    void *priv;              /* backend private data */
};

static inline u8 hw_read8(struct HwBus *b, u32 r) { return b->ops->read8(b, r); }
static inline void hw_write8(struct HwBus *b, u32 r, u8 v) { b->ops->write8(b, r, v); }

#endif /* LUPOUSB_HWBUS_H */
