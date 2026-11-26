/*
 * LupoUsbDriver - Layer L0 backend: virtual bus to the simulated LVHC chip.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_VIRTBUS_H
#define LUPOUSB_VIRTBUS_H

#include "lupousb/hwbus.h"
#include "lupousb/vsim.h"

struct VirtBus {
    struct HwBus bus;           /* what L1 sees */
    struct VChip chip;          /* the simulated chip behind it */
    HwIrqHandler irq_handler;
    void *irq_data;
    u8 removed;                 /* simulate card removal */
};

void virtbus_init(struct VirtBus *vb, u8 nports);

#endif /* LUPOUSB_VIRTBUS_H */
