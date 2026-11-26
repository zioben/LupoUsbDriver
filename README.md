# LupoUsbDriver
Amiga USB Driver Library

This is one of my first Projects, which started in the early 2000s.
I resumed it days ago and uploaded it on GitHub, to share the code with everyone.

It attempts to create a USB driver stack for handling base devices like a USB mouse or joypad.
Due to my lack of experience then, the driver was incomplete, but the stack was tested and worked with a generic USB mouse and a Gravis gamepad.
The library was designed using Mediator PCI board with a USB 1.1 board installed on an A1200 tower system but theoretically can run on any USB board since implemented commands are standard.

# The new stack (2026)

[![build](https://github.com/zioben/LupoUsbDriver/actions/workflows/build.yml/badge.svg?branch=claude/project-review)](https://github.com/zioben/LupoUsbDriver/actions/workflows/build.yml)

The project is being rebuilt as a layered USB 1.1 stack for AmigaOS 3.x.
The Mediator PCI board is no longer available, so the target hardware will
be a small USB host controller chip (MAX3421E or CH376 on the clockport, or
a PCMCIA card) instead of OHCI on PCI.

| Layer | What | Where |
|---|---|---|
| L0 | bus access (PCMCIA, clockport, PCI... and a virtual bus) | `src/bus/` |
| L1 | host controller chip driver + generic root hub emulation | `src/hcd/` |
| L2 | Exec device with the standard `usbhardware.device` interface | `src/amiga/lupovirt_dev.c` |
| L3 | USBD: enumeration, hub driver, class binding | `src/usbd/` |
| L4 | class drivers (HID: mouse, keyboard, gamepad) | `src/classes/` |
| L5 | tools | `tools/` |

While the real hardware is being chosen, L0 and L1 run on a **simulated**
host controller chip (LVHC) with a simulated hub, mouse, keyboard and
gamepad (`src/sim/`). Everything above L1 is the real code.

## Status

- Host tests: 54 checks with ASan/UBSan, no leaks.
- The same test suite, built for 68k, passes under vamos (big endian).
- `lupovirt.device` + `lupousbd` run in an emulated A1200 (FS-UAE with the
  built-in AROS ROM, cycle exact, real 68020 speed): the 5 virtual devices
  are enumerated and bound, the stack shuts down cleanly, and the virtual
  USB mouse moves the Intuition pointer.
- All of this runs in GitHub CI on every push.

## Build and test

    make test              # host tests (Linux, gcc, ASan/UBSan)
    make sim               # host simulator: ./build/host/usbsim [-v]
    make docker-amiga      # Amiga binaries via the toolchain container
    make dist              # LHA archive (inside the container)
    tests/emu/run.sh [s]   # emulated A1200 test (needs fs-uae, xvfb, socat)
    vamos -C 68020 build/amiga/C/lupotest   # 68k test suite (amitools)

The toolchain is bebbo's m68k-amigaos-gcc with NDK 3.2, in the
`amigadev/crosstools` Docker image (pinned by digest).

## Using it on an Amiga

- `Devs/lupovirt.device`: virtual USB host controller with the
  `usbhardware.device` API. Copy it to `DEVS:`. Unit 0 has idle devices;
  unit 1 is a demo where the virtual mouse moves in a square.
- `C/lupousbd`: the USB stack daemon. It works with any
  `usbhardware.device`, not only with `lupovirt.device`.

      lupousbd [DEVICE=name.device] [UNIT=n] [VERBOSE] [NOINPUT] [TIMEOUT=s] [LOG=file]

  Ctrl-F prints the device tree, Ctrl-C quits. `lupousbd UNIT=1` runs the
  moving-mouse demo.

## Headers and licences

The stack does not include Chris Hodges' `devices/usbhardware.h` (All
Rights Reserved). It uses its own binary-compatible MIT header,
`include/lupousb/hwreq.h`, whose layout is checked against the original
header in CI.

## Documentation (in Italian)

| File | Content |
|---|---|
| `Documents/ProjectReview.md` | review of the original project, architecture, roadmap, bug list |
| `Documents/Layers.md` | the development layers |
| `Documents/Development.md` | guide to the virtual stack, CI, emulator test |
| `legacy/README.md` | gcc build of the original sources and bug fixes |

# The original 2001-2003 sources

The original code is kept unchanged in `Sources/` for reference.

`legacy/` holds a port of those sources to the modern toolchain (gcc +
vasm), with the bugs of the original fixed one per commit:

    make legacy            # needs the Mediator pci.library headers in third_party/

The Elbox headers cannot be redistributed, so the legacy build does not run
in CI. See `legacy/README.md`.

## How to compile the original sources (work in progress)
The project is written in 68K Assembly and plain C.
You need to build the assembly library core using Asm-one devpack.
Then You need to build the C part using StormC Compiler 2.0.
Open the StormC project file 'USB/Usb/LupoUsbLibrary.s' and browse the project using StormC GUI platform.

