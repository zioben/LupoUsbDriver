/*
 * LupoUsbDriver - Layer L4: HID report decoding and mapping to Amiga input.
 *
 * Portable: it produces calls on a HidSink, it does not know input.device.
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/hid.h"
#include "lupousb/os.h"

/* HID keyboard usage (page 0x07) -> Amiga raw key code. 0xff = no key.
 * Amiga raw keys: see the RKM Libraries manual, "Keyboard" (raw key map). */
static const u8 rawkey_map[0x68] = {
    /* 0x00 */ 0xff, 0xff, 0xff, 0xff,
    /* 0x04 a..z */
    0x20, 0x35, 0x33, 0x22, 0x12, 0x23, 0x24, 0x25, 0x17, 0x26, 0x27, 0x28, 0x37,
    0x36, 0x18, 0x19, 0x10, 0x13, 0x21, 0x14, 0x16, 0x34, 0x11, 0x32, 0x15, 0x31,
    /* 0x1e 1..9 0 */
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
    /* 0x28 enter esc bksp tab space - = [ ] \ #(non-US) ; ' ` , . / */
    0x44, 0x45, 0x41, 0x42, 0x40, 0x0b, 0x0c, 0x1a, 0x1b, 0x0d, 0x2b, 0x29, 0x2a,
    0x00, 0x38, 0x39, 0x3a,
    /* 0x39 caps lock */
    0x62,
    /* 0x3a F1..F12 (F11/F12 have no Amiga key) */
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0xff, 0xff,
    /* 0x46 PrtSc ScrLk Pause Ins Home PgUp Del End PgDn Right Left Down Up */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x46, 0xff, 0xff, 0x4e, 0x4f, 0x4d, 0x4c,
    /* 0x53 NumLk KP/ KP* KP- KP+ KPEnter */
    0xff, 0x5c, 0x5d, 0x4a, 0x5e, 0x43,
    /* 0x59 KP1..KP9 KP0 KP. */
    0x1d, 0x1e, 0x1f, 0x2d, 0x2e, 0x2f, 0x3d, 0x3e, 0x3f, 0x0f, 0x3c,
    /* 0x64 non-US \| (the key next to left shift), Application -> Help */
    0x30, 0x5f, 0xff, 0xff
};

/* 0xe0.. LCtrl LShift LAlt LGUI RCtrl RShift RAlt RGUI */
static const u8 rawkey_mod[8] = { 0x63, 0x60, 0x64, 0x66, 0x63, 0x61, 0x65, 0x67 };

u8 hid_usage_to_rawkey(u16 usage)
{
    if (usage < sizeof(rawkey_map))
        return rawkey_map[usage];
    if (usage >= 0xe0 && usage <= 0xe7)
        return rawkey_mod[usage - 0xe0];
    return 0xff;
}

int hid_decoder_init(struct HidDecoder *dec, const u8 *rdesc, u16 rlen, const struct HidSink *sink)
{
    lu_memset(dec, 0, sizeof(*dec));
    dec->sink = sink;
    if (!hid_parse(rdesc, rlen, &dec->info))
        return 0;
    if (dec->info.app_page == HID_UP_GENERIC_DESKTOP) {
        switch (dec->info.app_usage) {
        case HID_GD_MOUSE:
        case HID_GD_POINTER:  dec->type = HIDT_MOUSE; break;
        case HID_GD_KEYBOARD: dec->type = HIDT_KEYBOARD; break;
        case HID_GD_JOYSTICK:
        case HID_GD_GAMEPAD:  dec->type = HIDT_JOY; break;
        default: break;
        }
    }
    return dec->type != HIDT_UNKNOWN;
}

static int is_signed_field(const struct HidField *f) { return f->logical_min < 0; }

static u8 axis_dir(s32 v, const struct HidField *f, u8 neg, u8 pos)
{
    s32 range = f->logical_max - f->logical_min;
    if (range <= 0)
        return 0;
    if (v < f->logical_min + range / 3)
        return neg;
    if (v > f->logical_max - range / 3)
        return pos;
    return 0;
}

static void decode_keyboard(struct HidDecoder *dec, const u8 *r, u32 len)
{
    u8 now[8];
    int nkeys = 0, i, j;
    lu_memset(now, 0, sizeof(now));
    for (i = 0; i < dec->info.nfields; i++) {
        const struct HidField *f = &dec->info.field[i];
        int e;
        if (f->usage_page != HID_UP_KEYBOARD)
            continue;
        for (e = 0; e < f->count; e++) {
            s32 v = hid_get_value(r, len, (u16)(f->bitpos + e * f->bitsize), f->bitsize, 0);
            if (f->flags & HID_F_VARIABLE) {
                u16 usage = (u16)(f->usage_min + e);
                if (usage >= 0xe0 && usage <= 0xe7 && v)
                    now[0] |= (u8)(1u << (usage - 0xe0));
            } else {
                u16 usage = (u16)(f->usage_min + (v - f->logical_min));
                if (usage == 0x01)
                    return;             /* ErrorRollOver: keep previous state */
                if (usage > 0x01 && nkeys < 6)
                    now[2 + nkeys++] = (u8)usage;
            }
        }
    }
    if (!dec->sink || !dec->sink->key)
        goto out;
    /* modifiers */
    for (i = 0; i < 8; i++) {
        u8 bit = (u8)(1u << i);
        if ((now[0] ^ dec->prev_keys[0]) & bit)
            dec->sink->key(dec->sink->ctx, rawkey_mod[i], !(now[0] & bit));
    }
    /* released keys */
    for (i = 2; i < 8; i++) {
        u8 k = dec->prev_keys[i];
        int still = 0;
        if (!k)
            continue;
        for (j = 2; j < 8; j++)
            if (now[j] == k)
                still = 1;
        if (!still && hid_usage_to_rawkey(k) != 0xff)
            dec->sink->key(dec->sink->ctx, hid_usage_to_rawkey(k), 1);
    }
    /* pressed keys */
    for (i = 2; i < 8; i++) {
        u8 k = now[i];
        int was = 0;
        if (!k)
            continue;
        for (j = 2; j < 8; j++)
            if (dec->prev_keys[j] == k)
                was = 1;
        if (!was && hid_usage_to_rawkey(k) != 0xff)
            dec->sink->key(dec->sink->ctx, hid_usage_to_rawkey(k), 0);
    }
out:
    lu_memcpy(dec->prev_keys, now, sizeof(now));
}

static void decode_pointer_or_joy(struct HidDecoder *dec, const u8 *r, u32 len)
{
    s32 dx = 0, dy = 0, wheel = 0;
    u16 buttons = 0;
    u8 dirs = 0;
    int i, e;
    for (i = 0; i < dec->info.nfields; i++) {
        const struct HidField *f = &dec->info.field[i];
        if (f->usage_page == HID_UP_BUTTON) {
            for (e = 0; e < f->count; e++) {
                u16 usage = (u16)(f->usage_min + e);
                s32 v = hid_get_value(r, len, (u16)(f->bitpos + e * f->bitsize), f->bitsize, 0);
                if (v && usage >= 1 && usage <= 16)
                    buttons |= (u16)(1u << (usage - 1));
            }
        } else if (f->usage_page == HID_UP_GENERIC_DESKTOP && f->count == 1) {
            s32 v = hid_get_value(r, len, f->bitpos, f->bitsize, is_signed_field(f));
            switch (f->usage_min) {
            case HID_GD_X:
                dx = v;
                dirs |= axis_dir(v, f, HID_JOY_LEFT, HID_JOY_RIGHT);
                break;
            case HID_GD_Y:
                dy = v;
                dirs |= axis_dir(v, f, HID_JOY_UP, HID_JOY_DOWN);
                break;
            case HID_GD_WHEEL:
                wheel = v;
                break;
            case HID_GD_HATSWITCH: {
                static const u8 hat[8] = {
                    HID_JOY_UP, HID_JOY_UP | HID_JOY_RIGHT, HID_JOY_RIGHT,
                    HID_JOY_DOWN | HID_JOY_RIGHT, HID_JOY_DOWN, HID_JOY_DOWN | HID_JOY_LEFT,
                    HID_JOY_LEFT, HID_JOY_UP | HID_JOY_LEFT };
                s32 h = v - f->logical_min;
                if (h >= 0 && h < 8)
                    dirs |= hat[h];
                break;
            }
            default:
                break;
            }
        }
    }
    if (!dec->sink)
        return;
    if (dec->type == HIDT_MOUSE) {
        if (dec->sink->mouse)
            dec->sink->mouse(dec->sink->ctx, (s16)dx, (s16)dy, (s16)wheel, buttons);
    } else if (dirs != dec->prev_dirs || buttons != dec->prev_buttons) {
        if (dec->sink->joy)
            dec->sink->joy(dec->sink->ctx, dec->joy_unit, dirs, buttons);
    }
    dec->prev_dirs = dirs;
    dec->prev_buttons = buttons;
}

void hid_decode_report(struct HidDecoder *dec, const u8 *report, u32 len)
{
    /* Fields of other report IDs are filtered by rebuilding a temporary view:
     * for simplicity only the first report ID's fields are decoded when IDs are used. */
    if (dec->info.uses_ids) {
        struct HidReportInfo saved = dec->info;
        int i, n = 0;
        if (len < 1)
            return;
        for (i = 0; i < saved.nfields; i++)
            if (saved.field[i].report_id == report[0])
                dec->info.field[n++] = saved.field[i];
        dec->info.nfields = (u8)n;
        if (dec->type == HIDT_KEYBOARD)
            decode_keyboard(dec, report + 1, len - 1);
        else
            decode_pointer_or_joy(dec, report + 1, len - 1);
        dec->info = saved;
        return;
    }
    if (dec->type == HIDT_KEYBOARD)
        decode_keyboard(dec, report, len);
    else
        decode_pointer_or_joy(dec, report, len);
}
