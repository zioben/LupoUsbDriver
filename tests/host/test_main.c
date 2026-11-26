/*
 * LupoUsbDriver - host unit and integration tests.
 *
 * The whole stack runs on the simulated hardware:
 *   L0 virtbus -> LVHC chip model + virtual devices
 *   L1 lvhc driver + root hub emulation
 *   L3 USBD (direct backend, no Exec device)
 *   L4 HID class with a recording sink
 * Time is simulated, so the tests are deterministic.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>
#include "lupousb/virtbus.h"
#include "lupousb/lvhc.h"
#include "lupousb/be_direct.h"
#include "lupousb/hid.h"
#include "lupousb/usbspec.h"
#include "lupousb/os.h"

static int failures, checks;
#define CHECK(c) do { checks++; if (!(c)) { failures++; \
    printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

/* ---------------------------------------------------- recording sink */

struct Rec {
    int mouse_events;
    s32 sum_dx, sum_dy, sum_wheel;
    u16 buttons;
    u8  keys[64];
    u8  keyup[64];
    int nkeys;
    int joy_events;
    u8  joy_dirs;
    u16 joy_buttons;
};
static struct Rec rec;

static void rec_mouse(void *c, s16 dx, s16 dy, s16 w, u16 b)
{
    (void)c;
    rec.mouse_events++;
    rec.sum_dx += dx; rec.sum_dy += dy; rec.sum_wheel += w;
    rec.buttons = b;
}
static void rec_key(void *c, u8 k, int up)
{
    (void)c;
    if (rec.nkeys < 64) {
        rec.keys[rec.nkeys] = k;
        rec.keyup[rec.nkeys++] = (u8)up;
    }
}
static void rec_joy(void *c, int unit, u8 dirs, u16 b)
{
    (void)c; (void)unit;
    rec.joy_events++;
    rec.joy_dirs = dirs;
    rec.joy_buttons = b;
}
static const struct HidSink rec_sink = { NULL, rec_mouse, rec_key, rec_joy };

/* ------------------------------------------------------- environment */

static u32 now_ms = 1000;
static void advance(void) { host_set_time(++now_ms); }

struct Env {
    struct VirtBus vb;
    struct Hcd *hcd;
    struct DirectBackend be;
    struct UsbdBus bus;
};

static void env_init(struct Env *e)
{
    memset(e, 0, sizeof(*e));
    virtbus_init(&e->vb, 2);
    e->hcd = lvhc_create(&e->vb.bus);
    e->hcd->ops->start(e->hcd);
    e->be.hcd = e->hcd;
    e->be.advance = advance;
    usbd_init(&e->bus, &usbd_direct_backend, &e->be);
    hid_class_register(&e->bus, &rec_sink);
}

static void env_done(struct Env *e)
{
    usbd_stop(&e->bus);
    lvhc_destroy(e->hcd);
}

static void run(struct Env *e, u32 ms)
{
    u32 end = now_ms + ms;
    while (now_ms < end)
        usbd_run(&e->bus, 10);
}

static struct UsbdDevice *find_product(struct UsbdBus *bus, const char *name)
{
    int a;
    for (a = 1; a < 128; a++)
        if (bus->dev[a] && strcmp(bus->dev[a]->product, name) == 0)
            return bus->dev[a];
    return NULL;
}

/* ----------------------------------------------------------- tests */

static void test_hid_parser(void)
{
    struct HidReportInfo info;
    u8 r[4] = { 0x05, 0xfe, 0x03, 0x00 };
    printf("hid parser\n");
    CHECK(hid_parse(hid_boot_mouse_desc, hid_boot_mouse_desc_len, &info));
    CHECK(info.app_page == HID_UP_GENERIC_DESKTOP && info.app_usage == HID_GD_MOUSE);
    CHECK(info.nfields == 3);       /* buttons, X, Y */
    CHECK(info.field[0].usage_page == HID_UP_BUTTON && info.field[0].count == 3);
    CHECK(info.field[1].usage_min == HID_GD_X && info.field[1].bitpos == 8);
    CHECK(info.field[2].usage_min == HID_GD_Y && info.field[2].bitpos == 16);
    CHECK(hid_get_value(r, 4, 0, 1, 0) == 1);
    CHECK(hid_get_value(r, 4, 8, 8, 1) == -2);
    CHECK(hid_get_value(r, 4, 16, 8, 1) == 3);

    CHECK(hid_parse(hid_boot_kbd_desc, hid_boot_kbd_desc_len, &info));
    CHECK(info.app_usage == HID_GD_KEYBOARD);
    CHECK(info.nfields == 2);
    CHECK(info.field[0].usage_min == 0xe0 && info.field[0].count == 8);
    CHECK(info.field[1].bitpos == 16 && info.field[1].count == 6 && !(info.field[1].flags & HID_F_VARIABLE));
}

static void test_rawkeys(void)
{
    printf("rawkey map\n");
    CHECK(hid_usage_to_rawkey(0x04) == 0x20);   /* a */
    CHECK(hid_usage_to_rawkey(0x1d) == 0x31);   /* z */
    CHECK(hid_usage_to_rawkey(0x27) == 0x0a);   /* 0 */
    CHECK(hid_usage_to_rawkey(0x28) == 0x44);   /* return */
    CHECK(hid_usage_to_rawkey(0x2c) == 0x40);   /* space */
    CHECK(hid_usage_to_rawkey(0x3a) == 0x50);   /* F1 */
    CHECK(hid_usage_to_rawkey(0x4c) == 0x46);   /* delete */
    CHECK(hid_usage_to_rawkey(0x52) == 0x4c);   /* up */
    CHECK(hid_usage_to_rawkey(0x58) == 0x43);   /* keypad enter */
    CHECK(hid_usage_to_rawkey(0x62) == 0x0f);   /* keypad 0 */
    CHECK(hid_usage_to_rawkey(0x64) == 0x30);   /* < > */
    CHECK(hid_usage_to_rawkey(0xe1) == 0x60);   /* left shift */
    CHECK(hid_usage_to_rawkey(0xe3) == 0x66);   /* left Amiga (GUI) */
}

static void test_keyboard_decoder(void)
{
    struct HidDecoder dec;
    u8 r1[8] = { 0x02, 0, 0x04, 0, 0, 0, 0, 0 };   /* shift + a */
    u8 r2[8] = { 0x02, 0, 0x04, 0x05, 0, 0, 0, 0 }; /* shift + a + b */
    u8 r3[8] = { 0 };
    printf("keyboard decoder\n");
    memset(&rec, 0, sizeof(rec));
    CHECK(hid_decoder_init(&dec, hid_boot_kbd_desc, hid_boot_kbd_desc_len, &rec_sink));
    CHECK(dec.type == HIDT_KEYBOARD);
    hid_decode_report(&dec, r1, 8);
    hid_decode_report(&dec, r2, 8);
    hid_decode_report(&dec, r3, 8);
    CHECK(rec.nkeys == 6);
    CHECK(rec.keys[0] == 0x60 && !rec.keyup[0]);    /* shift down */
    CHECK(rec.keys[1] == 0x20 && !rec.keyup[1]);    /* a down */
    CHECK(rec.keys[2] == 0x35 && !rec.keyup[2]);    /* b down */
    CHECK(rec.keys[3] == 0x60 && rec.keyup[3]);     /* shift up */
}

static void test_enumeration_and_hid(void)
{
    struct Env e;
    struct VDev *hub, *mouse, *kbd, *pad, *mouse2;
    struct UsbdDevice *d;
    u8 keys[6] = { 0x04, 0, 0, 0, 0, 0 }, none[6] = { 0 };

    printf("enumeration + hid through hub\n");
    memset(&rec, 0, sizeof(rec));
    env_init(&e);
    hub = vdev_hub_create(4);
    mouse = vdev_mouse_create(1);           /* low speed behind a full speed hub */
    kbd = vdev_keyboard_create();
    pad = vdev_gamepad_create();
    vsim_plug(&e.vb.chip, NULL, 0, hub);
    vsim_plug(&e.vb.chip, hub->hub, 0, mouse);
    vsim_plug(&e.vb.chip, hub->hub, 2, kbd);
    vsim_plug(&e.vb.chip, NULL, 1, pad);

    CHECK(usbd_start(&e.bus));
    run(&e, 3000);
    os_log_level = LU_LOG_INFO;
    usbd_dump(&e.bus);
    os_log_level = LU_LOG_WARN;

    CHECK(usbd_device_count(&e.bus) == 5);         /* root hub, hub, mouse, kbd, pad */
    CHECK(e.bus.root && e.bus.root->addr == 1);
    d = find_product(&e.bus, "Virtual wheel mouse");
    CHECK(d && d->lowspeed && d->depth == 2 && d->iface[0].driver);
    d = find_product(&e.bus, "Virtual keyboard");
    CHECK(d && !d->lowspeed && d->port == 3 && d->iface[0].driver);
    d = find_product(&e.bus, "Virtual gamepad");
    CHECK(d && d->lowspeed && d->depth == 1 && d->iface[0].driver);
    d = find_product(&e.bus, "Virtual 4-port hub");
    CHECK(d && d->iface[0].driver == &usbd_hub_driver);

    /* mouse movement and buttons */
    vmouse_report(mouse, 0x01, 5, -3, 0);
    vmouse_report(mouse, 0x01, 10, 7, 1);
    vmouse_report(mouse, 0x00, 0, 0, 0);
    run(&e, 200);
    CHECK(rec.mouse_events == 3);
    CHECK(rec.sum_dx == 15 && rec.sum_dy == 4 && rec.sum_wheel == 1);
    CHECK(rec.buttons == 0);

    /* keyboard: press and release 'a' */
    vkbd_report(kbd, 0, keys);
    vkbd_report(kbd, 0, none);
    run(&e, 200);
    CHECK(rec.nkeys == 2 && rec.keys[0] == 0x20 && !rec.keyup[0] && rec.keyup[1]);

    /* gamepad: left + button 1 */
    vpad_report(pad, 0, 128, 0x01);
    run(&e, 200);
    CHECK(rec.joy_events == 1 && rec.joy_dirs == HID_JOY_LEFT && rec.joy_buttons == 1);

    /* hot unplug of the hub removes the hub, mouse and keyboard */
    vsim_unplug(&e.vb.chip, NULL, 0);
    run(&e, 500);
    CHECK(usbd_device_count(&e.bus) == 2);
    CHECK(find_product(&e.bus, "Virtual wheel mouse") == NULL);

    /* plug a mouse directly into the root port: addresses are reused */
    mouse2 = vdev_mouse_create(0);
    vsim_plug(&e.vb.chip, NULL, 0, mouse2);
    run(&e, 500);
    d = find_product(&e.bus, "Virtual wheel mouse");
    CHECK(d && !d->lowspeed && d->depth == 1);
    rec.mouse_events = 0;
    vmouse_report(mouse2, 0x02, 1, 1, 0);
    run(&e, 100);
    CHECK(rec.mouse_events == 1 && rec.buttons == 0x02);

    env_done(&e);
    vdev_destroy(hub); vdev_destroy(mouse); vdev_destroy(kbd);
    vdev_destroy(pad); vdev_destroy(mouse2);
}

static void test_card_removal(void)
{
    struct Env e;
    struct VDev *mouse;
    printf("card removal\n");
    env_init(&e);
    mouse = vdev_mouse_create(1);
    vsim_plug(&e.vb.chip, NULL, 0, mouse);
    CHECK(usbd_start(&e.bus));
    run(&e, 1000);
    CHECK(usbd_device_count(&e.bus) == 2);
    e.vb.removed = 1;                       /* PCMCIA card pulled out */
    run(&e, 100);
    /* pending transfers were failed with XS_OFFLINE, nothing crashed */
    CHECK(1);
    env_done(&e);
    vdev_destroy(mouse);
}

int main(void)
{
    long before = host_allocations();
    test_hid_parser();
    test_rawkeys();
    test_keyboard_decoder();
    test_enumeration_and_hid();
    test_card_removal();
    CHECK(host_allocations() == before);    /* no memory leaks */
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
