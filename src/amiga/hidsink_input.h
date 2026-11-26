/*
 * LupoUsbDriver - HID sink injecting events into input.device (AmigaOS).
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_HIDSINK_INPUT_H
#define LUPOUSB_HIDSINK_INPUT_H

#include <exec/io.h>
#include "lupousb/hid.h"

struct InputSink {
    struct HidSink sink;
    struct MsgPort *port;
    struct IOStdReq *req;
    u16  buttons;
    UWORD kbd_qual;
    UBYTE opened;
};

int  input_sink_open(struct InputSink *s);
void input_sink_close(struct InputSink *s);

#endif
