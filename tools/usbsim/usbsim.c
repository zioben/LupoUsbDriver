/*
 * LupoUsbDriver - usbsim: runs the whole stack on the host with simulated
 * hardware and prints what happens (enumeration log, device tree, decoded
 * input). Useful to watch the layers working together without an Amiga.
 *
 *   build/host/usbsim [-v]
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <string.h>
#include "lupousb/virtbus.h"
#include "lupousb/lvhc.h"
#include "lupousb/be_direct.h"
#include "lupousb/hid.h"
#include "lupousb/os.h"

static u32 now_ms = 1000;
static void advance(void) { host_set_time(++now_ms); }

static void p_mouse(void *c, s16 dx, s16 dy, s16 w, u16 b)
{
    (void)c;
    printf("  [input] mouse dx=%d dy=%d wheel=%d buttons=%x\n", dx, dy, w, b);
}
static void p_key(void *c, u8 k, int up)
{
    (void)c;
    printf("  [input] rawkey 0x%02x %s\n", k, up ? "up" : "down");
}
static void p_joy(void *c, int unit, u8 dirs, u16 b)
{
    (void)c;
    printf("  [input] joy %d dirs=%x buttons=%x\n", unit, dirs, b);
}
static const struct HidSink sink = { NULL, p_mouse, p_key, p_joy };

static struct UsbdBus bus;
static void run(u32 ms)
{
    u32 end = now_ms + ms;
    while (now_ms < end)
        usbd_run(&bus, 10);
}

int main(int argc, char **argv)
{
    static struct VirtBus vb;
    struct DirectBackend be;
    struct Hcd *hcd;
    struct VDev *hub, *mouse, *kbd, *pad;
    u8 k_a[6] = { 0x04 }, none[6] = { 0 };

    os_log_level = (argc > 1 && strcmp(argv[1], "-v") == 0) ? LU_LOG_DEBUG : LU_LOG_INFO;
    printf("== L0 virtual bus + LVHC chip, 2 root ports\n");
    virtbus_init(&vb, 2);
    hub = vdev_hub_create(4);
    mouse = vdev_mouse_create(1);
    kbd = vdev_keyboard_create();
    pad = vdev_gamepad_create();
    vsim_plug(&vb.chip, NULL, 0, hub);
    vsim_plug(&vb.chip, hub->hub, 0, mouse);
    vsim_plug(&vb.chip, hub->hub, 2, kbd);

    printf("== L1 lvhc driver\n");
    hcd = lvhc_create(&vb.bus);
    hcd->ops->start(hcd);
    be.hcd = hcd;
    be.advance = advance;

    printf("== L3 usbd + L4 hid\n");
    usbd_init(&bus, &usbd_direct_backend, &be);
    hid_class_register(&bus, &sink);
    usbd_start(&bus);
    run(2000);
    printf("== device tree\n");
    usbd_dump(&bus);

    printf("== mouse moves, 'a' is typed\n");
    vmouse_report(mouse, 1, 10, 0, 0);
    vmouse_report(mouse, 0, 0, -5, -1);
    vkbd_report(kbd, 0, k_a);
    vkbd_report(kbd, 0, none);
    run(200);

    printf("== gamepad plugged into root port 2\n");
    vsim_plug(&vb.chip, NULL, 1, pad);
    run(1000);
    vpad_report(pad, 255, 0, 0x03);
    run(100);

    printf("== hub unplugged\n");
    vsim_unplug(&vb.chip, NULL, 0);
    run(500);
    usbd_dump(&bus);

    usbd_stop(&bus);
    lvhc_destroy(hcd);
    vdev_destroy(hub); vdev_destroy(mouse); vdev_destroy(kbd); vdev_destroy(pad);
    return 0;
}
