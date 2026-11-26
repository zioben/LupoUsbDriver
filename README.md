# LupoUsbDriver
Amiga USB Driver Library

This is one of my first Projects, which started in the early 2000s.
I resumed it days ago and uploaded it on GitHub, to share the code with everyone.

It attempts to create a USB driver stack for handling base devices like a USB mouse or joypad.
Due to my lack of experience then, the driver was incomplete, but the stack was tested and worked with a generic USB mouse and a Gravis gamepad.
The library was designed using Mediator PCI board with a USB 1.1 board installed on an A1200 tower system but theoretically can run on any USB board since implemented commands are standard.

# The new stack (2026)

The project is being rebuilt as a layered USB stack for AmigaOS 3.x
(see `Documents/ProjectReview.md` and `Documents/Layers.md`):

| Layer | What | Where |
|---|---|---|
| L0 | bus access (PCMCIA, clockport, PCI... and a virtual bus) | `src/bus/` |
| L1 | host controller chip driver + generic root hub emulation | `src/hcd/` |
| L2 | Exec device with the standard `usbhardware.device` interface | `src/amiga/lupovirt_dev.c` |
| L3 | USBD: enumeration, hub driver, class binding | `src/usbd/` |
| L4 | class drivers (HID: mouse, keyboard, gamepad) | `src/classes/` |
| L5 | tools | `tools/` |

While the real hardware is being chosen, L0 and L1 run on a **simulated**
host controller chip (LVHC) with simulated hub, mouse, keyboard and gamepad
(`src/sim/`). Everything above L1 is the real code.

## Build

    make test            # host tests (Linux, gcc, ASan/UBSan)
    make sim             # host simulator: ./build/host/usbsim
    make docker-amiga    # Amiga binaries via the toolchain container
    make dist            # LHA archive (inside the container)

Amiga binaries: `Devs/lupovirt.device` (virtual USB host controller,
usbhardware.device API) and `C/lupousbd` (the USB stack daemon). Copy the
device to `DEVS:` and run `lupousbd UNIT=1` for the moving-mouse demo.
Details (in Italian): `Documents/Development.md`.

# The original 2001-2003 sources

The original code is kept unchanged in `Sources/` for reference.

## How to compile the original sources (work in progress)
The project is written in 68K Assembly and plain C.
You need to build the assembly library core using Asm-one devpack.
Then You need to build the C part using StormC Compiler 2.0.
Open the StormC project file 'USB/Usb/LupoUsbLibrary.s' and browse the project using StormC GUI platform.

