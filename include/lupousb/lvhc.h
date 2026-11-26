/*
 * LupoUsbDriver - Layer L1: driver for the LVHC host controller.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_LVHC_H
#define LUPOUSB_LVHC_H

#include "lupousb/hcd.h"
#include "lupousb/hwbus.h"

struct Hcd *lvhc_create(struct HwBus *bus);
void lvhc_destroy(struct Hcd *hcd);

#endif /* LUPOUSB_LVHC_H */
