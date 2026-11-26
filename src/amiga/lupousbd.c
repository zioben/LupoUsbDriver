/*
 * LupoUsbDriver - lupousbd: the USB stack daemon (layers L3 + L4) for
 * AmigaOS 3.x. It drives any usbhardware.device:
 *
 *   lupousbd                                  ; lupovirt.device unit 0
 *   lupousbd DEVICE=lupovirt.device UNIT=1    ; virtual demo (moving mouse)
 *   lupousbd VERBOSE                          ; debug log
 *   lupousbd NOINPUT                          ; do not inject input events
 *   lupousbd UNIT=1 TIMEOUT=10                ; quit by itself after 10 s
 *   lupousbd LOG=RAM:usb.log                  ; log to a file (unbuffered)
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

/* LOG=file: every line is appended and the file closed at once, so the log
 * survives a hang or a crash (used by the emulator test). */
static const char *log_file;
static u32 log_t0;

static void log_to_file(const char *line)
{
    char stamp[16];
    BPTR fh = Open((CONST_STRPTR)log_file, MODE_READWRITE);
    if (!fh)
        return;
    Seek(fh, 0, OFFSET_END);
    {
        /* elapsed milliseconds since start, to measure the emulator speed */
        u32 t = os_time_ms() - log_t0;
        int n = 0, i;
        char d[12];
        do { d[n++] = (char)('0' + t % 10); t /= 10; } while (t && n < 11);
        for (i = 0; i < 8 - n; i++) stamp[i] = ' ';
        for (; n; ) stamp[i++] = d[--n];
        stamp[i++] = ' ';
        stamp[i] = 0;
    }
    Write(fh, (APTR)stamp, (LONG)lu_strlen(stamp));
    Write(fh, (APTR)line, (LONG)lu_strlen(line));
    Write(fh, (APTR)"\n", 1);
    Close(fh);
}

enum { ARG_DEVICE, ARG_UNIT, ARG_VERBOSE, ARG_NOINPUT, ARG_TIMEOUT, ARG_LOG, ARG_COUNT };

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
    u32 timeout_ms = 0, start;

    rda = ReadArgs((CONST_STRPTR)"DEVICE/K,UNIT/N,VERBOSE/S,NOINPUT/S,TIMEOUT/N,LOG/K", args, NULL);
    if (!rda) {
        PrintFault(IoErr(), (CONST_STRPTR)"lupousbd");
        return RETURN_FAIL;
    }
    if (args[ARG_DEVICE])
        device = (const char *)args[ARG_DEVICE];
    if (args[ARG_UNIT])
        unit = *(ULONG *)args[ARG_UNIT];
    if (args[ARG_TIMEOUT])
        timeout_ms = *(ULONG *)args[ARG_TIMEOUT] * 1000;
    os_log_hook = log_to_console;
    if (args[ARG_LOG]) {
        log_file = (const char *)args[ARG_LOG];
        log_t0 = 0;                     /* set once the timer is open */
        os_log_hook = log_to_file;
    }
    os_log_level = args[ARG_VERBOSE] ? LU_LOG_DEBUG : LU_LOG_INFO;

    if (!amiga_backend_open(&be, device, unit))
        goto out;
    log_t0 = os_time_ms();
    if (!args[ARG_NOINPUT] && !input_sink_open(&in))
        LU_WARN("input.device not available, events will not be injected");

    usbd_init(&bus, &usbd_amiga_backend, &be);
    hid_class_register(&bus, in.opened ? &in.sink : &log_sink);
    LU_INFO("lupousbd: using %s unit %u (Ctrl-F: device tree, Ctrl-C: quit)", device, (u32)unit);
    if (!usbd_start(&bus))
        goto out;

    start = os_time_ms();
    while (!(be.last_sigs & SIGBREAKF_CTRL_C)) {
        usbd_run(&bus, 20);
        if (timeout_ms && os_time_ms() - start >= timeout_ms)
            break;
        if (be.last_sigs & SIGBREAKF_CTRL_F) {
            be.last_sigs &= ~SIGBREAKF_CTRL_F;
            usbd_dump(&bus);
        }
    }
    LU_INFO("lupousbd: %u devices, shutting down", (u32)usbd_device_count(&bus));
    usbd_dump(&bus);
    usbd_stop(&bus);
    rc = RETURN_OK;
out:
    if (in.opened)
        input_sink_close(&in);
    amiga_backend_close(&be);
    FreeArgs(rda);
    return rc;
}
