/*
 * LupoUsbDriver - USBD backend: direct calls into an L1 HCD.
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/be_direct.h"
#include "lupousb/os.h"

static int d_submit(void *ctx, struct UsbXfer *x)
{
    struct DirectBackend *b = ctx;
    return b->hcd->ops->submit(b->hcd, x);
}

static int d_abort(void *ctx, struct UsbXfer *x)
{
    struct DirectBackend *b = ctx;
    int r = b->hcd->ops->abort(b->hcd, x);
    b->hcd->ops->poll(b->hcd);              /* deliver the completion */
    return r;
}

static void d_wait(void *ctx, u32 ms)
{
    struct DirectBackend *b = ctx;
    u32 i;
    b->hcd->ops->poll(b->hcd);
    for (i = 0; i < ms; i++) {
        if (b->advance)
            b->advance();
        b->hcd->ops->poll(b->hcd);
    }
}

static void d_bus_reset(void *ctx)
{
    struct DirectBackend *b = ctx;
    b->hcd->ops->bus_reset(b->hcd);
}

const struct UsbdBackend usbd_direct_backend = { d_submit, d_abort, d_wait, d_bus_reset };
