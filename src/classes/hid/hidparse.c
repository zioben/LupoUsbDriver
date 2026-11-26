/*
 * LupoUsbDriver - Layer L4: HID report descriptor parser (HID 1.11, 6.2.2).
 *
 * Only input items are recorded (output/feature items just advance their
 * own bit positions). Long items are skipped.
 *
 * SPDX-License-Identifier: MIT
 */
#include "lupousb/hid.h"
#include "lupousb/os.h"

#define MAX_USAGES   16
#define MAX_STACK    4
#define MAX_IDS      16

struct Globals {
    u16 usage_page;
    s32 lmin, lmax;
    u8  rsize, rcount, rid;
};

static s32 item_sval(const u8 *p, int n)
{
    switch (n) {
    case 1: return (s8)p[0];
    case 2: return (s16)lu_get_le16(p);
    case 4: return (s32)lu_get_le32(p);
    default: return 0;
    }
}

static u32 item_uval(const u8 *p, int n)
{
    switch (n) {
    case 1: return p[0];
    case 2: return lu_get_le16(p);
    case 4: return lu_get_le32(p);
    default: return 0;
    }
}

int hid_parse(const u8 *desc, u16 len, struct HidReportInfo *info)
{
    struct Globals g, stack[MAX_STACK];
    int sp = 0;
    u32 usages[MAX_USAGES];
    int nusages = 0;
    u32 umin = 0, umax = 0;
    int have_range = 0;
    u16 inpos[MAX_IDS];            /* input bit position per report ID */
    int depth = 0;
    u16 pos = 0;

    lu_memset(info, 0, sizeof(*info));
    lu_memset(&g, 0, sizeof(g));
    lu_memset(inpos, 0, sizeof(inpos));

    while (pos < len) {
        u8 b = desc[pos];
        int size, type, tag;
        u32 uval;
        s32 sval;
        if (b == 0xfe) {                    /* long item */
            if (pos + 1 >= len)
                return 0;
            pos = (u16)(pos + 3 + desc[pos + 1]);
            continue;
        }
        size = b & 3;
        if (size == 3)
            size = 4;
        type = (b >> 2) & 3;
        tag = (b >> 4) & 0xf;
        if (pos + 1 + size > len)
            return 0;
        uval = item_uval(desc + pos + 1, size);
        sval = item_sval(desc + pos + 1, size);
        pos = (u16)(pos + 1 + size);

        if (type == 1) {                    /* global */
            switch (tag) {
            case 0x0: g.usage_page = (u16)uval; break;
            case 0x1: g.lmin = sval; break;
            case 0x2: g.lmax = (g.lmin >= 0 && size < 4) ? (s32)uval : sval; break;
            case 0x7: g.rsize = (u8)uval; break;
            case 0x8: g.rid = (u8)uval; info->uses_ids = 1; break;
            case 0x9: g.rcount = (u8)uval; break;
            case 0xa: if (sp < MAX_STACK) stack[sp++] = g; break;
            case 0xb: if (sp > 0) g = stack[--sp]; break;
            default: break;
            }
        } else if (type == 2) {             /* local */
            switch (tag) {
            case 0x0:
                if (nusages < MAX_USAGES)
                    usages[nusages++] = (size == 4) ? uval : ((u32)g.usage_page << 16) | uval;
                break;
            case 0x1: umin = (size == 4) ? uval : ((u32)g.usage_page << 16) | uval; have_range = 1; break;
            case 0x2: umax = (size == 4) ? uval : ((u32)g.usage_page << 16) | uval; have_range = 1; break;
            default: break;
            }
        } else if (type == 0) {             /* main */
            u16 *bp = &inpos[g.rid % MAX_IDS];
            switch (tag) {
            case 0xa:                       /* collection */
                if (uval == 1 && !info->app_usage && nusages) {
                    info->app_page = (u16)(usages[0] >> 16);
                    info->app_usage = (u16)usages[0];
                }
                depth++;
                break;
            case 0xc:                       /* end collection */
                depth--;
                break;
            case 0x8: {                     /* input */
                u8 flags = (u8)uval;
                if (!(flags & HID_F_CONSTANT)) {
                    if ((flags & HID_F_VARIABLE) && nusages && !have_range) {
                        /* one field per element, each with its own usage */
                        int i;
                        for (i = 0; i < g.rcount && info->nfields < HID_MAX_FIELDS; i++) {
                            struct HidField *f = &info->field[info->nfields++];
                            u32 u = usages[i < nusages ? i : nusages - 1];
                            f->usage_page = (u16)(u >> 16);
                            f->usage_min = f->usage_max = (u16)u;
                            f->bitpos = (u16)(*bp + i * g.rsize);
                            f->bitsize = g.rsize;
                            f->count = 1;
                            f->flags = flags;
                            f->report_id = g.rid;
                            f->logical_min = g.lmin;
                            f->logical_max = g.lmax;
                        }
                    } else if (info->nfields < HID_MAX_FIELDS) {
                        struct HidField *f = &info->field[info->nfields++];
                        u32 lo = have_range ? umin : (nusages ? usages[0] : 0);
                        u32 hi = have_range ? umax : (nusages ? usages[nusages - 1] : 0);
                        f->usage_page = (u16)((lo >> 16) ? (lo >> 16) : g.usage_page);
                        f->usage_min = (u16)lo;
                        f->usage_max = (u16)hi;
                        f->bitpos = *bp;
                        f->bitsize = g.rsize;
                        f->count = g.rcount;
                        f->flags = flags;
                        f->report_id = g.rid;
                        f->logical_min = g.lmin;
                        f->logical_max = g.lmax;
                    }
                }
                *bp = (u16)(*bp + g.rsize * g.rcount);
                break;
            }
            default:                        /* output, feature: other report types */
                break;
            }
            nusages = 0;
            have_range = 0;
        }
    }
    (void)depth;
    return info->nfields > 0;
}

s32 hid_get_value(const u8 *report, u32 len, u16 bitpos, u8 bitsize, int is_signed)
{
    u32 v = 0;
    int i;
    if (bitsize == 0 || bitsize > 32 || (u32)(bitpos + bitsize) > len * 8)
        return 0;
    for (i = 0; i < bitsize; i++) {
        u32 bit = (u32)bitpos + (u32)i;
        if (report[bit >> 3] & (1u << (bit & 7)))
            v |= 1u << i;
    }
    if (is_signed && bitsize < 32 && (v & (1u << (bitsize - 1))))
        v |= ~0u << bitsize;
    return (s32)v;
}

/* Boot protocol descriptors, HID 1.11 appendix B */
const u8 hid_boot_mouse_desc[] = {
    0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x09, 0x01, 0xa1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01,
    0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05,
    0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x15, 0x81,
    0x25, 0x7f, 0x75, 0x08, 0x95, 0x02, 0x81, 0x06, 0xc0, 0xc0
};
const u16 hid_boot_mouse_desc_len = sizeof(hid_boot_mouse_desc);

const u8 hid_boot_kbd_desc[] = {
    0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x05, 0x07, 0x19, 0xe0,
    0x29, 0xe7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08,
    0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x81, 0x01, 0x95, 0x06,
    0x75, 0x08, 0x15, 0x00, 0x25, 0x65, 0x05, 0x07, 0x19, 0x00,
    0x29, 0x65, 0x81, 0x00, 0xc0
};
const u16 hid_boot_kbd_desc_len = sizeof(hid_boot_kbd_desc);
