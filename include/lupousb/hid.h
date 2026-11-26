/*
 * LupoUsbDriver - Layer L4: HID class (report descriptor parser, input
 * mapping, class driver).
 *
 * The class driver is portable; what happens with the decoded input is
 * decided by a HidSink supplied by the platform:
 *   - AmigaOS: src/amiga/hidsink_input.c injects events into input.device
 *   - tests:   a recording sink
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_HID_H
#define LUPOUSB_HID_H

#include "lupousb/usbd.h"

/* Usage pages / usages used by the mapper */
#define HID_UP_GENERIC_DESKTOP  0x01
#define HID_UP_KEYBOARD         0x07
#define HID_UP_LED              0x08
#define HID_UP_BUTTON           0x09
#define HID_GD_POINTER          0x01
#define HID_GD_MOUSE            0x02
#define HID_GD_JOYSTICK         0x04
#define HID_GD_GAMEPAD          0x05
#define HID_GD_KEYBOARD         0x06
#define HID_GD_X                0x30
#define HID_GD_Y                0x31
#define HID_GD_WHEEL            0x38
#define HID_GD_HATSWITCH        0x39

#define HID_MAX_FIELDS  48

/* Main item flags */
#define HID_F_CONSTANT  0x01
#define HID_F_VARIABLE  0x02
#define HID_F_RELATIVE  0x04

struct HidField {
    u16 usage_page;
    u16 usage_min, usage_max;   /* variable: element i has usage_min + i (or single usage) */
    u16 bitpos;                 /* offset in the report, without the report ID byte */
    u8  bitsize;
    u8  count;
    u8  flags;
    u8  report_id;
    s32 logical_min, logical_max;
};

struct HidReportInfo {
    struct HidField field[HID_MAX_FIELDS];
    u8  nfields;
    u8  uses_ids;
    u16 app_page, app_usage;    /* first application collection */
};

int hid_parse(const u8 *desc, u16 len, struct HidReportInfo *info);
s32 hid_get_value(const u8 *report, u32 len, u16 bitpos, u8 bitsize, int is_signed);

/* Amiga raw key code for a HID keyboard usage, 0xff if none. */
u8  hid_usage_to_rawkey(u16 usage);

/* Where decoded input goes. Buttons are reported as absolute state. */
#define HID_JOY_UP      0x01
#define HID_JOY_DOWN    0x02
#define HID_JOY_LEFT    0x04
#define HID_JOY_RIGHT   0x08

struct HidSink {
    void *ctx;
    void (*mouse)(void *ctx, s16 dx, s16 dy, s16 wheel, u16 buttons);
    void (*key)(void *ctx, u8 rawkey, int up);
    void (*joy)(void *ctx, int unit, u8 dirs, u16 buttons);
};

enum { HIDT_UNKNOWN = 0, HIDT_MOUSE, HIDT_KEYBOARD, HIDT_JOY };

/* Decoder state for one HID interface (portable, testable without USB). */
struct HidDecoder {
    struct HidReportInfo info;
    u8  type;
    u8  joy_unit;
    u8  prev_keys[8];           /* modifiers + 6 keys of the previous report */
    u16 prev_buttons;
    u8  prev_dirs;
    const struct HidSink *sink;
};

int  hid_decoder_init(struct HidDecoder *dec, const u8 *rdesc, u16 rlen, const struct HidSink *sink);
void hid_decode_report(struct HidDecoder *dec, const u8 *report, u32 len);

/* Class driver: registers with usbd and sends input to 'sink'. */
void hid_class_register(struct UsbdBus *bus, const struct HidSink *sink);

/* Standard boot protocol report descriptors (fallbacks) */
extern const u8 hid_boot_mouse_desc[];
extern const u16 hid_boot_mouse_desc_len;
extern const u8 hid_boot_kbd_desc[];
extern const u16 hid_boot_kbd_desc_len;

#endif /* LUPOUSB_HID_H */
