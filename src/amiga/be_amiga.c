/*
 * LupoUsbDriver - USBD backend for AmigaOS (usbhardware.device client).
 *
 * SPDX-License-Identifier: MIT
 */
#include <exec/errors.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include "lupousb/be_amiga.h"
#include "lupousb/os.h"

extern struct Device *TimerBase;

static int map_status(BYTE err)
{
    if (err == 0)
        return XS_OK;
    if (err == IOERR_ABORTED)
        return XS_ABORTED;
    if (err > 0 && err <= UHIOERR_OUTOFMEMORY)
        return err;                     /* UHIOERR_* == XS_* */
    return XS_HOSTERROR;
}

static int a_submit(void *ctx, struct UsbXfer *x)
{
    struct AmigaBackend *b = ctx;
    struct IOUsbHWReq *io = (struct IOUsbHWReq *)CreateIORequest(b->port, sizeof(struct IOUsbHWReq));
    if (!io) {
        x->status = XS_NOMEM;           /* reported synchronously */
        if (x->done)
            x->done(x);
        return 0;
    }
    io->iouh_Req.io_Device = b->ctl->iouh_Req.io_Device;
    io->iouh_Req.io_Unit = b->ctl->iouh_Req.io_Unit;
    io->iouh_Req.io_Command = x->type == XT_CONTROL ? UHCMD_CONTROLXFER :
                              x->type == XT_INT ? UHCMD_INTXFER :
                              x->type == XT_BULK ? UHCMD_BULKXFER : UHCMD_ISOXFER;
    io->iouh_Flags = x->flags & (UHFF_LOWSPEED | UHFF_NOSHORTPKT | UHFF_NAKTIMEOUT | UHFF_ALLOWRUNTPKTS);
    io->iouh_Dir = x->dir == XD_IN ? UHDIR_IN : UHDIR_OUT;
    io->iouh_DevAddr = x->devaddr;
    io->iouh_Endpoint = x->endpoint;
    io->iouh_MaxPktSize = x->maxpkt;
    io->iouh_Length = x->length;
    io->iouh_Data = x->data;
    io->iouh_Interval = x->interval;
    io->iouh_NakTimeout = x->nak_timeout;
    lu_memcpy(&io->iouh_SetupData, x->setup, 8);    /* already little endian */
    io->iouh_UserData = x;
    x->be_priv = io;
    x->status = XS_PENDING;
    b->inflight++;
    SendIO(&io->iouh_Req);
    return 1;
}

static int a_abort(void *ctx, struct UsbXfer *x)
{
    (void)ctx;
    if (x->status == XS_PENDING && x->be_priv)
        AbortIO(&((struct IOUsbHWReq *)x->be_priv)->iouh_Req);
    return 1;
}

static void drain(struct AmigaBackend *b)
{
    struct IOUsbHWReq *io;
    while ((io = (struct IOUsbHWReq *)GetMsg(b->port)) != NULL) {
        struct UsbXfer *x = io->iouh_UserData;
        x->actual = io->iouh_Actual;
        x->status = map_status(io->iouh_Req.io_Error);
        x->be_priv = NULL;
        DeleteIORequest(&io->iouh_Req);
        b->inflight--;
        if (x->done)
            x->done(x);
    }
}

static void a_wait(void *ctx, u32 ms)
{
    struct AmigaBackend *b = ctx;
    ULONG pmask = 1UL << b->port->mp_SigBit;
    ULONG tmask = 1UL << b->tport->mp_SigBit;
    ULONG sigs;

    drain(b);
    if (!ms)
        return;
    b->treq->tr_node.io_Command = TR_ADDREQUEST;
    b->treq->tr_time.tv_secs = ms / 1000;
    b->treq->tr_time.tv_micro = (ms % 1000) * 1000;
    SetSignal(0, tmask);
    SendIO(&b->treq->tr_node);
    sigs = Wait(pmask | tmask | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);
    if (!CheckIO(&b->treq->tr_node))
        AbortIO(&b->treq->tr_node);
    WaitIO(&b->treq->tr_node);
    b->last_sigs |= sigs & (SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F);
    drain(b);
}

static void a_bus_reset(void *ctx)
{
    struct AmigaBackend *b = ctx;
    b->ctl->iouh_Req.io_Command = UHCMD_USBRESET;
    DoIO(&b->ctl->iouh_Req);
    b->ctl->iouh_Req.io_Command = UHCMD_USBOPER;
    DoIO(&b->ctl->iouh_Req);
}

const struct UsbdBackend usbd_amiga_backend = { a_submit, a_abort, a_wait, a_bus_reset };

int amiga_backend_open(struct AmigaBackend *b, const char *device, ULONG unit)
{
    lu_memset(b, 0, sizeof(*b));
    b->port = CreateMsgPort();
    b->tport = CreateMsgPort();
    if (!b->port || !b->tport)
        return 0;
    b->treq = (struct timerequest *)CreateIORequest(b->tport, sizeof(struct timerequest));
    if (!b->treq || OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_MICROHZ, &b->treq->tr_node, 0))
        return 0;
    b->timer_open = 1;
    TimerBase = b->treq->tr_node.io_Device;
    b->ctl = (struct IOUsbHWReq *)CreateIORequest(b->port, sizeof(struct IOUsbHWReq));
    if (!b->ctl || OpenDevice((CONST_STRPTR)device, unit, &b->ctl->iouh_Req, 0)) {
        LU_ERR("cannot open %s unit %u", device, (u32)unit);
        return 0;
    }
    b->opened = 1;
    return 1;
}

void amiga_backend_close(struct AmigaBackend *b)
{
    while (b->inflight)                 /* every request must come back */
        a_wait(b, 10);
    if (b->opened)
        CloseDevice(&b->ctl->iouh_Req);
    if (b->ctl)
        DeleteIORequest(&b->ctl->iouh_Req);
    if (b->timer_open)
        CloseDevice(&b->treq->tr_node);
    if (b->treq)
        DeleteIORequest(&b->treq->tr_node);
    if (b->tport)
        DeleteMsgPort(b->tport);
    if (b->port)
        DeleteMsgPort(b->port);
    TimerBase = NULL;
}
