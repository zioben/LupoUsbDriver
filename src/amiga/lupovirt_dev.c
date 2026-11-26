/*
 * LupoUsbDriver - Layer L2: lupovirt.device
 *
 * An Exec device implementing the usbhardware.device interface
 * (devices/usbhardware.h) on top of the L1 LVHC driver and the L0 virtual
 * bus. The "hardware" behind it is the software model of the LVHC chip with
 * simulated USB devices attached:
 *
 *   root port 1: 4-port hub -> port 1: low speed wheel mouse
 *                              port 3: keyboard
 *   root port 2: low speed gamepad
 *
 *   unit 0: devices are idle
 *   unit 1: demo mode, the virtual mouse moves along a square
 *
 * Any USB stack speaking usbhardware.device can be used on top of it; the
 * LupoUsbDriver stack does so with lupousbd.
 *
 * Structure (generic part of L2): BeginIO queues requests to the unit task,
 * which owns the HCD; the HCD interrupt only signals that task.
 *
 * SPDX-License-Identifier: MIT
 */
#include <exec/types.h>
#include <exec/resident.h>
#include <exec/devices.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <devices/timer.h>
#include <devices/usbhardware.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <clib/alib_protos.h>

#include "lupousb/virtbus.h"
#include "lupousb/lvhc.h"
#include "lupousb/os.h"

#define REG(r, d) d __asm(#r)

#define DEVNAME     "lupovirt.device"
#define VERSION     0
#define REVISION    1
#define VSTRING     "lupovirt.device 0.1 (26.11.2026) LupoUsbDriver virtual USB host"
#define NUNITS      2
#define TICK_US     2000
#define ABORT_MARK  ((APTR)1)
#define TAKEN_MARK  ((APTR)2)

/* Must be the first code in the file: running the device as a program
 * returns immediately instead of crashing. */
int _start(void) { return -1; }

struct ExecBase *SysBase;
extern struct Device *TimerBase;

struct VirtUnit;

struct DevBase {
    struct Device dev;
    BPTR seglist;
    struct VirtUnit *unit[NUNITS];
};

struct VirtUnit {
    struct Unit u;                      /* unit_MsgPort receives requests */
    struct DevBase *base;
    int number;
    struct Task *task;
    struct Task *creator;
    ULONG irq_mask, abort_mask, port_mask;
    struct List active;                 /* requests owned by the HCD */
    struct MsgPort *tport;
    struct timerequest *treq;
    UBYTE tpending;
    UBYTE ok;
    UBYTE demo;
    UWORD state;
    struct VirtBus vb;
    struct Hcd *hcd;
    struct VDev *hub, *mouse, *kbd, *pad;
    u32 demo_next;
    int demo_step;
};

static const char devname[] = DEVNAME;
static const char idstring[] = VSTRING;

/* ------------------------------------------------------- unit task */

static void hcd_wake(void *ctx)
{
    struct VirtUnit *vu = ctx;
    Signal(vu->task, vu->irq_mask);     /* may be called from interrupts */
}

static BYTE map_error(int st)
{
    if (st >= XS_OK && st <= XS_NOMEM)
        return (BYTE)st;                /* same values as UHIOERR_* */
    if (st == XS_ABORTED)
        return IOERR_ABORTED;
    return UHIOERR_HOSTERROR;
}

static void reply(struct IOUsbHWReq *io)
{
    io->iouh_DriverPrivate1 = NULL;
    io->iouh_DriverPrivate2 = NULL;
    ReplyMsg(&io->iouh_Req.io_Message);
}

static void xfer_done(struct UsbXfer *x)
{
    struct IOUsbHWReq *io = x->user;
    io->iouh_Actual = x->actual;
    io->iouh_Req.io_Error = map_error(x->status);
    Remove(&io->iouh_Req.io_Message.mn_Node);
    os_free(x, sizeof(*x));
    reply(io);
}

static void submit_xfer(struct VirtUnit *vu, struct IOUsbHWReq *io)
{
    UWORD cmd = io->iouh_Req.io_Command;
    struct UsbXfer *x = os_alloc(sizeof(*x));
    if (!x) {
        io->iouh_Req.io_Error = UHIOERR_OUTOFMEMORY;
        reply(io);
        return;
    }
    x->type = cmd == UHCMD_CONTROLXFER ? XT_CONTROL : cmd == UHCMD_INTXFER ? XT_INT : XT_BULK;
    if (x->type == XT_CONTROL)
        x->dir = (io->iouh_SetupData.bmRequestType & 0x80) ? XD_IN : XD_OUT;
    else
        x->dir = io->iouh_Dir == UHDIR_IN ? XD_IN : XD_OUT;
    x->devaddr = (u8)io->iouh_DevAddr;
    x->endpoint = (u8)io->iouh_Endpoint;
    x->maxpkt = io->iouh_MaxPktSize;
    x->interval = io->iouh_Interval;
    x->nak_timeout = io->iouh_NakTimeout;
    x->flags = io->iouh_Flags & (UHFF_LOWSPEED | UHFF_NOSHORTPKT | UHFF_NAKTIMEOUT | UHFF_ALLOWRUNTPKTS);
    lu_memcpy(x->setup, &io->iouh_SetupData, 8);   /* wValue etc. are already little endian */
    x->data = io->iouh_Data;
    x->length = io->iouh_Length;
    x->done = xfer_done;
    x->user = io;
    x->user2 = vu;
    io->iouh_Actual = 0;
    io->iouh_DriverPrivate1 = x;
    AddTail(&vu->active, &io->iouh_Req.io_Message.mn_Node);
    hcd_submit(vu->hcd, x);
}

static void handle_request(struct VirtUnit *vu, struct IOUsbHWReq *io)
{
    io->iouh_Req.io_Error = 0;
    switch (io->iouh_Req.io_Command) {
    case CMD_RESET:
    case UHCMD_USBRESET:
        vu->hcd->ops->bus_reset(vu->hcd);
        vu->state = UHSF_OPERATIONAL;
        break;
    case UHCMD_USBRESUME:
    case UHCMD_USBOPER:
        vu->state = UHSF_OPERATIONAL;
        break;
    case UHCMD_USBSUSPEND:
        vu->state = UHSF_SUSPENDED;
        break;
    case CMD_FLUSH: {
        struct Node *n;
        for (n = vu->active.lh_Head; n->ln_Succ; n = n->ln_Succ)
            vu->hcd->ops->abort(vu->hcd, ((struct IOUsbHWReq *)n)->iouh_DriverPrivate1);
        break;
    }
    case UHCMD_CONTROLXFER:
    case UHCMD_INTXFER:
    case UHCMD_BULKXFER:
        if (!(vu->state & UHSF_OPERATIONAL)) {
            io->iouh_Req.io_Error = UHIOERR_USBOFFLINE;
            break;
        }
        submit_xfer(vu, io);
        return;                         /* replied on completion */
    case UHCMD_ISOXFER:
        io->iouh_Req.io_Error = UHIOERR_BADPARAMS;  /* not supported yet */
        break;
    default:
        io->iouh_Req.io_Error = IOERR_NOCMD;
        break;
    }
    reply(io);
}

static void process_aborts(struct VirtUnit *vu)
{
    struct Node *n, *next;
    for (n = vu->active.lh_Head; (next = n->ln_Succ) != NULL; n = next) {
        struct IOUsbHWReq *io = (struct IOUsbHWReq *)n;
        if (io->iouh_DriverPrivate2 == ABORT_MARK)
            vu->hcd->ops->abort(vu->hcd, io->iouh_DriverPrivate1);
    }
}

static void demo_tick(struct VirtUnit *vu)
{
    static const s8 dx[4] = { 3, 0, -3, 0 }, dy[4] = { 0, 3, 0, -3 };
    u32 now = os_time_ms();
    int side;
    if (!vu->demo || (s32)(now - vu->demo_next) < 0)
        return;
    vu->demo_next = now + 20;
    side = (vu->demo_step / 40) & 3;
    vmouse_report(vu->mouse, 0, dx[side], dy[side], 0);
    vu->demo_step++;
}

static void build_topology(struct VirtUnit *vu)
{
    vu->hub = vdev_hub_create(4);
    vu->mouse = vdev_mouse_create(1);
    vu->kbd = vdev_keyboard_create();
    vu->pad = vdev_gamepad_create();
    if (vu->hub) {
        vsim_plug(&vu->vb.chip, NULL, 0, vu->hub);
        if (vu->mouse) vsim_plug(&vu->vb.chip, vu->hub->hub, 0, vu->mouse);
        if (vu->kbd)   vsim_plug(&vu->vb.chip, vu->hub->hub, 2, vu->kbd);
    }
    if (vu->pad)
        vsim_plug(&vu->vb.chip, NULL, 1, vu->pad);
}

static void unit_task(void)
{
    struct Task *me = FindTask(NULL);
    struct VirtUnit *vu = me->tc_UserData;
    struct MsgPort *port = &vu->u.unit_MsgPort;
    BYTE psig = AllocSignal(-1), isig = AllocSignal(-1), asig = AllocSignal(-1);
    ULONG tmask = 0;

    port->mp_Node.ln_Type = NT_MSGPORT;
    port->mp_Flags = PA_SIGNAL;
    port->mp_SigBit = (UBYTE)psig;
    port->mp_SigTask = me;
    NewList(&port->mp_MsgList);
    NewList(&vu->active);
    vu->port_mask = 1UL << psig;
    vu->irq_mask = 1UL << isig;
    vu->abort_mask = 1UL << asig;

    vu->tport = CreateMsgPort();
    vu->treq = vu->tport ? CreateIORequest(vu->tport, sizeof(struct timerequest)) : NULL;
    if (psig < 0 || isig < 0 || asig < 0 || !vu->treq ||
        OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_MICROHZ, &vu->treq->tr_node, 0) != 0)
        goto out;
    TimerBase = vu->treq->tr_node.io_Device;
    tmask = 1UL << vu->tport->mp_SigBit;

    virtbus_init(&vu->vb, 2);
    build_topology(vu);
    vu->hcd = lvhc_create(&vu->vb.bus);
    if (!vu->hcd)
        goto out;
    vu->hcd->wake = hcd_wake;
    vu->hcd->wake_ctx = vu;
    if (!vu->hcd->ops->start(vu->hcd))
        goto out;
    vu->state = UHSF_OPERATIONAL;
    vu->ok = 1;
    Signal(vu->creator, SIGF_SINGLE);

    for (;;) {
        ULONG sigs;
        struct Message *m;
        if (!vu->tpending) {
            vu->treq->tr_node.io_Command = TR_ADDREQUEST;
            vu->treq->tr_time.tv_secs = 0;
            vu->treq->tr_time.tv_micro = TICK_US;
            SendIO(&vu->treq->tr_node);
            vu->tpending = 1;
        }
        sigs = Wait(vu->port_mask | vu->irq_mask | vu->abort_mask | tmask | SIGBREAKF_CTRL_C);
        if (sigs & SIGBREAKF_CTRL_C)
            break;
        if (GetMsg(vu->tport))
            vu->tpending = 0;
        for (;;) {
            Forbid();
            m = GetMsg(port);
            if (m)
                ((struct IOUsbHWReq *)m)->iouh_DriverPrivate2 = TAKEN_MARK;
            Permit();
            if (!m)
                break;
            /* DriverPrivate2 stays TAKEN_MARK: AbortIO will not touch the
             * node until DriverPrivate1 points to the in-flight transfer. */
            handle_request(vu, (struct IOUsbHWReq *)m);
        }
        if (sigs & vu->abort_mask)
            process_aborts(vu);
        demo_tick(vu);
        hcd_poll(vu->hcd);
    }

out:
    if (vu->hcd)
        lvhc_destroy(vu->hcd);          /* fails and replies all pending requests */
    vu->hcd = NULL;
    {
        struct Message *m;
        while ((m = GetMsg(port)) != NULL) {
            ((struct IORequest *)m)->io_Error = UHIOERR_USBOFFLINE;
            ReplyMsg(m);
        }
    }
    vdev_destroy(vu->mouse);
    vdev_destroy(vu->kbd);
    vdev_destroy(vu->pad);
    vdev_destroy(vu->hub);
    if (vu->treq) {
        if (vu->tpending) {
            AbortIO(&vu->treq->tr_node);
            WaitIO(&vu->treq->tr_node);
        }
        if (vu->treq->tr_node.io_Device)
            CloseDevice(&vu->treq->tr_node);
        DeleteIORequest(vu->treq);
    }
    if (vu->tport)
        DeleteMsgPort(vu->tport);
    Forbid();                           /* the task is removed before anyone runs */
    Signal(vu->creator, SIGF_SINGLE);
}

/* ---------------------------------------------------- device vectors */

static struct VirtUnit *unit_start(struct DevBase *base, int n)
{
    struct VirtUnit *vu = os_alloc(sizeof(*vu));
    if (!vu)
        return NULL;
    vu->base = base;
    vu->number = n;
    vu->demo = (UBYTE)(n == 1);
    vu->creator = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    vu->task = CreateTask((CONST_STRPTR)"lupovirt unit", 5, (APTR)unit_task, 8192);
    if (!vu->task) {
        os_free(vu, sizeof(*vu));
        return NULL;
    }
    vu->task->tc_UserData = vu;         /* we are still under Forbid (OpenDevice) */
    Wait(SIGF_SINGLE);
    if (!vu->ok) {
        os_free(vu, sizeof(*vu));
        return NULL;
    }
    return vu;
}

static void unit_stop(struct VirtUnit *vu)
{
    vu->creator = FindTask(NULL);
    SetSignal(0, SIGF_SINGLE);
    Signal(vu->task, SIGBREAKF_CTRL_C);
    Wait(SIGF_SINGLE);
    os_free(vu, sizeof(*vu));
}

static BPTR dev_expunge(REG(a6, struct DevBase *base));

static struct DevBase *dev_init(REG(d0, struct DevBase *base), REG(a0, BPTR seglist),
                                REG(a6, struct ExecBase *sysbase))
{
    SysBase = sysbase;
    base->seglist = seglist;
    base->dev.dd_Library.lib_Node.ln_Type = NT_DEVICE;
    base->dev.dd_Library.lib_Node.ln_Name = (char *)devname;
    base->dev.dd_Library.lib_Flags = LIBF_SUMUSED | LIBF_CHANGED;
    base->dev.dd_Library.lib_Version = VERSION;
    base->dev.dd_Library.lib_Revision = REVISION;
    base->dev.dd_Library.lib_IdString = (APTR)idstring;
    return base;
}

static void dev_open(REG(a1, struct IOUsbHWReq *io), REG(d0, ULONG unit), REG(d1, ULONG flags),
                     REG(a6, struct DevBase *base))
{
    struct VirtUnit *vu;
    (void)flags;
    base->dev.dd_Library.lib_OpenCnt++;     /* prevent expunge while we wait */
    io->iouh_Req.io_Error = IOERR_OPENFAIL;
    if (unit >= NUNITS || io->iouh_Req.io_Message.mn_Length < sizeof(struct IOUsbHWReq)) {
        base->dev.dd_Library.lib_OpenCnt--;
        return;
    }
    vu = base->unit[unit];
    if (!vu)
        vu = base->unit[unit] = unit_start(base, (int)unit);
    if (!vu) {
        base->dev.dd_Library.lib_OpenCnt--;
        return;
    }
    vu->u.unit_OpenCnt++;
    io->iouh_Req.io_Unit = &vu->u;
    io->iouh_Req.io_Device = &base->dev;
    io->iouh_Req.io_Error = 0;
    io->iouh_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
    base->dev.dd_Library.lib_Flags &= ~LIBF_DELEXP;
}

static BPTR dev_close(REG(a1, struct IOUsbHWReq *io), REG(a6, struct DevBase *base))
{
    struct VirtUnit *vu = (struct VirtUnit *)io->iouh_Req.io_Unit;
    io->iouh_Req.io_Unit = (APTR)-1;
    io->iouh_Req.io_Device = (APTR)-1;
    if (vu && --vu->u.unit_OpenCnt == 0) {
        base->unit[vu->number] = NULL;
        unit_stop(vu);
    }
    if (--base->dev.dd_Library.lib_OpenCnt == 0 &&
        (base->dev.dd_Library.lib_Flags & LIBF_DELEXP))
        return dev_expunge(base);
    return 0;
}

static BPTR dev_expunge(REG(a6, struct DevBase *base))
{
    BPTR seg;
    if (base->dev.dd_Library.lib_OpenCnt) {
        base->dev.dd_Library.lib_Flags |= LIBF_DELEXP;
        return 0;
    }
    seg = base->seglist;
    Remove(&base->dev.dd_Library.lib_Node);
    FreeMem((UBYTE *)base - base->dev.dd_Library.lib_NegSize,
            base->dev.dd_Library.lib_NegSize + base->dev.dd_Library.lib_PosSize);
    return seg;
}

static ULONG dev_null(void) { return 0; }

static void query_device(struct VirtUnit *vu, struct IOUsbHWReq *io)
{
    struct TagItem *ti = (struct TagItem *)io->iouh_Data;
    ULONG count = 0;
    while (ti && ti->ti_Tag != TAG_DONE) {
        switch (ti->ti_Tag) {
        case TAG_IGNORE: break;
        case TAG_MORE:   ti = (struct TagItem *)ti->ti_Data; continue;
        case TAG_SKIP:   ti += ti->ti_Data; break;
        case UHA_State:       *(ULONG *)ti->ti_Data = vu->state; count++; break;
        case UHA_Manufacturer:*(STRPTR *)ti->ti_Data = (STRPTR)"LupoUsbDriver"; count++; break;
        case UHA_ProductName: *(STRPTR *)ti->ti_Data = (STRPTR)"Virtual USB host (LVHC)"; count++; break;
        case UHA_Version:     *(ULONG *)ti->ti_Data = VERSION; count++; break;
        case UHA_Revision:    *(ULONG *)ti->ti_Data = REVISION; count++; break;
        case UHA_Description: *(STRPTR *)ti->ti_Data = (STRPTR)"Simulated LVHC chip with hub, mouse, keyboard, gamepad"; count++; break;
        case UHA_Copyright:   *(STRPTR *)ti->ti_Data = (STRPTR)"MIT License"; count++; break;
        case UHA_DriverVersion: *(ULONG *)ti->ti_Data = 0x0200; count++; break;
        default: break;
        }
        ti++;
    }
    io->iouh_Actual = count;
    io->iouh_State = vu->state;
}

static void dev_beginio(REG(a1, struct IOUsbHWReq *io), REG(a6, struct DevBase *base))
{
    struct VirtUnit *vu = (struct VirtUnit *)io->iouh_Req.io_Unit;
    (void)base;
    io->iouh_Req.io_Message.mn_Node.ln_Type = NT_MESSAGE;
    io->iouh_Req.io_Error = 0;
    io->iouh_DriverPrivate1 = NULL;
    io->iouh_DriverPrivate2 = NULL;
    if (io->iouh_Req.io_Command == UHCMD_QUERYDEVICE) {
        query_device(vu, io);
        if (!(io->iouh_Req.io_Flags & IOF_QUICK))
            ReplyMsg(&io->iouh_Req.io_Message);
        return;
    }
    io->iouh_Req.io_Flags &= ~IOF_QUICK;
    PutMsg(&vu->u.unit_MsgPort, &io->iouh_Req.io_Message);
}

static LONG dev_abortio(REG(a1, struct IOUsbHWReq *io), REG(a6, struct DevBase *base))
{
    struct VirtUnit *vu = (struct VirtUnit *)io->iouh_Req.io_Unit;
    (void)base;
    Forbid();
    if (io->iouh_Req.io_Message.mn_Node.ln_Type == NT_MESSAGE) {
        if (io->iouh_DriverPrivate1 == NULL && io->iouh_DriverPrivate2 == NULL) {
            /* still waiting in the unit port */
            Remove(&io->iouh_Req.io_Message.mn_Node);
            io->iouh_Req.io_Error = IOERR_ABORTED;
            ReplyMsg(&io->iouh_Req.io_Message);
        } else if (io->iouh_DriverPrivate1) {
            io->iouh_DriverPrivate2 = ABORT_MARK;
            Signal(vu->task, vu->abort_mask);
        }
    }
    Permit();
    return 0;
}

static const APTR functable[] = {
    (APTR)dev_open, (APTR)dev_close, (APTR)dev_expunge, (APTR)dev_null,
    (APTR)dev_beginio, (APTR)dev_abortio, (APTR)-1
};

static const ULONG inittable[4] = {
    sizeof(struct DevBase), (ULONG)functable, 0, (ULONG)dev_init
};

const struct Resident __attribute__((used)) romtag = {
    RTC_MATCHWORD, (struct Resident *)&romtag, (APTR)(&romtag + 1), RTF_AUTOINIT,
    VERSION, NT_DEVICE, 0, (char *)devname, (char *)idstring, (APTR)inittable
};
