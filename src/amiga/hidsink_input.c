/*
 * LupoUsbDriver - Layer L4 (Amiga side): HID sink that injects events into
 * input.device, so USB mice and keyboards work in Intuition like the
 * built-in ones.
 *
 * Mouse buttons: one IECLASS_RAWMOUSE event per button transition
 * (IECODE_LBUTTON / RBUTTON / MBUTTON, with IECODE_UP_PREFIX on release),
 * movement with IECODE_NOBUTTON and IEQUALIFIER_RELATIVEMOUSE.
 * Wheel: raw keys RAWKEY_WHEEL_UP/DOWN (0x7a/0x7b, NewMouse convention,
 * documented in libraries/keymap.h of NDK 3.2).
 *
 * SPDX-License-Identifier: MIT
 */
#include <exec/ports.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <proto/exec.h>
#include "lupousb/hid.h"
#include "lupousb/os.h"
#include "hidsink_input.h"

#define RAWKEY_WHEEL_UP_CODE   0x7a
#define RAWKEY_WHEEL_DOWN_CODE 0x7b

static void send(struct InputSink *s, struct InputEvent *ev)
{
    ev->ie_NextEvent = NULL;
    s->req->io_Command = IND_WRITEEVENT;
    s->req->io_Data = ev;
    s->req->io_Length = sizeof(*ev);
    s->req->io_Flags = 0;
    DoIO((struct IORequest *)s->req);
}

static UWORD button_quals(u16 b)
{
    UWORD q = 0;
    if (b & 1) q |= IEQUALIFIER_LEFTBUTTON;
    if (b & 2) q |= IEQUALIFIER_RBUTTON;
    if (b & 4) q |= IEQUALIFIER_MIDBUTTON;
    return q;
}

static void sink_mouse(void *ctx, s16 dx, s16 dy, s16 wheel, u16 buttons)
{
    static const UWORD codes[3] = { IECODE_LBUTTON, IECODE_RBUTTON, IECODE_MBUTTON };
    struct InputSink *s = ctx;
    struct InputEvent ev;
    int i;

    lu_memset(&ev, 0, sizeof(ev));
    if (dx || dy) {
        ev.ie_Class = IECLASS_RAWMOUSE;
        ev.ie_Code = IECODE_NOBUTTON;
        ev.ie_Qualifier = IEQUALIFIER_RELATIVEMOUSE | s->kbd_qual | button_quals(s->buttons);
        ev.ie_position.ie_xy.ie_x = dx;
        ev.ie_position.ie_xy.ie_y = dy;
        send(s, &ev);
    }
    for (i = 0; i < 3; i++) {
        u16 bit = (u16)(1u << i);
        if ((buttons ^ s->buttons) & bit) {
            s->buttons ^= bit;
            lu_memset(&ev, 0, sizeof(ev));
            ev.ie_Class = IECLASS_RAWMOUSE;
            ev.ie_Code = codes[i] | ((buttons & bit) ? 0 : IECODE_UP_PREFIX);
            ev.ie_Qualifier = IEQUALIFIER_RELATIVEMOUSE | s->kbd_qual | button_quals(s->buttons);
            send(s, &ev);
        }
    }
    while (wheel) {
        UWORD code = wheel > 0 ? RAWKEY_WHEEL_UP_CODE : RAWKEY_WHEEL_DOWN_CODE;
        lu_memset(&ev, 0, sizeof(ev));
        ev.ie_Class = IECLASS_RAWKEY;
        ev.ie_Code = code;
        ev.ie_Qualifier = s->kbd_qual;
        send(s, &ev);
        ev.ie_Code = code | IECODE_UP_PREFIX;
        send(s, &ev);
        wheel = (s16)(wheel > 0 ? wheel - 1 : wheel + 1);
    }
}

static UWORD qual_for_key(u8 rawkey)
{
    switch (rawkey) {
    case 0x60: return IEQUALIFIER_LSHIFT;
    case 0x61: return IEQUALIFIER_RSHIFT;
    case 0x63: return IEQUALIFIER_CONTROL;
    case 0x64: return IEQUALIFIER_LALT;
    case 0x65: return IEQUALIFIER_RALT;
    case 0x66: return IEQUALIFIER_LCOMMAND;
    case 0x67: return IEQUALIFIER_RCOMMAND;
    default:   return 0;
    }
}

static void sink_key(void *ctx, u8 rawkey, int up)
{
    struct InputSink *s = ctx;
    struct InputEvent ev;
    UWORD q = qual_for_key(rawkey);
    if (rawkey == 0x62) {               /* Caps Lock toggles on press */
        if (!up)
            s->kbd_qual ^= IEQUALIFIER_CAPSLOCK;
    } else if (q) {
        if (up) s->kbd_qual &= (UWORD)~q;
        else    s->kbd_qual |= q;
    }
    lu_memset(&ev, 0, sizeof(ev));
    ev.ie_Class = IECLASS_RAWKEY;
    ev.ie_Code = (UWORD)(rawkey | (up ? IECODE_UP_PREFIX : 0));
    ev.ie_Qualifier = s->kbd_qual | button_quals(s->buttons);
    send(s, &ev);
}

static void sink_joy(void *ctx, int unit, u8 dirs, u16 buttons)
{
    (void)ctx;
    /* TODO: joystick/gamepad support (lowlevel.library / game port emulation). */
    LU_INFO("joy %d: dirs %02x buttons %04x", unit, dirs, buttons);
}

int input_sink_open(struct InputSink *s)
{
    lu_memset(s, 0, sizeof(*s));
    s->port = CreateMsgPort();
    if (!s->port)
        return 0;
    s->req = (struct IOStdReq *)CreateIORequest(s->port, sizeof(struct IOStdReq));
    if (!s->req || OpenDevice((CONST_STRPTR)"input.device", 0, (struct IORequest *)s->req, 0)) {
        input_sink_close(s);
        return 0;
    }
    s->opened = 1;
    s->sink.ctx = s;
    s->sink.mouse = sink_mouse;
    s->sink.key = sink_key;
    s->sink.joy = sink_joy;
    return 1;
}

void input_sink_close(struct InputSink *s)
{
    if (s->opened)
        CloseDevice((struct IORequest *)s->req);
    if (s->req)
        DeleteIORequest((struct IORequest *)s->req);
    if (s->port)
        DeleteMsgPort(s->port);
    lu_memset(s, 0, sizeof(*s));
}
