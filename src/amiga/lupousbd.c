/*
 * LupoUsbDriver - lupousbd: the USB stack daemon (layers L3 + L4) for
 * AmigaOS 3.x. It drives any usbhardware.device:
 *
 *   lupousbd                                  ; lupovirt.device unit 0
 *   lupousbd DEVICE=lupovirt.device UNIT=1    ; virtual demo (moving mouse)
 *   lupousbd VERBOSE                          ; debug log
 *   lupousbd NOINPUT                          ; do not inject input events
 *
 * Ctrl-F prints the device tree, Ctrl-C quits.
 *
 * In a later phase the same core will be packaged as lupousb.library with
 * dynamically loaded class drivers; the daemon is the first integration step.
 *
 * SPDX-License-Identifier: MIT
 */
#include <exec/types.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "lupousb/be_amiga.h"
#include "lupousb/hid.h"
#include "lupousb/os.h"
#include "hidsink_input.h"

static const char version[] __attribute__((used)) = "$VER: lupousbd 0.1 (26.11.2026)";

extern void (*os_log_hook)(const char *line);

static void log_to_console(const char *line)
{
    PutStr((CONST_STRPTR)line);
    PutStr((CONST_STRPTR)"\n");
}

enum { ARG_DEVICE, ARG_UNIT, ARG_VERBOSE, ARG_NOINPUT, ARG_COUNT };

int main(void)
{
    LONG args[ARG_COUNT] = { 0 };
    struct RDArgs *rda;
    const char *device = "lupovirt.device";
    ULONG unit = 0;
    static struct AmigaBackend be;
    static struct UsbdBus bus;
    static struct InputSink in;
    static const struct HidSink log_sink = { NULL, NULL, NULL, NULL };
    int rc = RETURN_FAIL;

    rda = ReadArgs((CONST_STRPTR)"DEVICE/K,UNIT/N,VERBOSE/S,NOINPUT/S", args, NULL);
    if (!rda) {
        PrintFault(IoErr(), (CONST_STRPTR)"lupousbd");
        return RETURN_FAIL;
    }
    if (args[ARG_DEVICE])
        device = (const char *)args[ARG_DEVICE];
    if (args[ARG_UNIT])
        unit = *(ULONG *)args[ARG_UNIT];
    os_log_hook = log_to_console;
    os_log_level = args[ARG_VERBOSE] ? LU_LOG_DEBUG : LU_LOG_INFO;

    if (!amiga_backend_open(&be, device, unit))
        goto out;
    if (!args[ARG_NOINPUT] && !input_sink_open(&in))
        LU_WARN("input.device not available, events will not be injected");

    usbd_init(&bus, &usbd_amiga_backend, &be);
    hid_class_register(&bus, in.opened ? &in.sink : &log_sink);
    LU_INFO("lupousbd: using %s unit %u (Ctrl-F: device tree, Ctrl-C: quit)", device, (u32)unit);
    if (!usbd_start(&bus))
        goto out;

    while (!(be.last_sigs & SIGBREAKF_CTRL_C)) {
        usbd_run(&bus, 20);
        if (be.last_sigs & SIGBREAKF_CTRL_F) {
            be.last_sigs &= ~SIGBREAKF_CTRL_F;
            usbd_dump(&bus);
        }
    }
    LU_INFO("lupousbd: shutting down");
    usbd_stop(&bus);
    rc = RETURN_OK;
out:
    if (in.opened)
        input_sink_close(&in);
    amiga_backend_close(&be);
    FreeArgs(rda);
    return rc;
}
