# LupoUsbDriver
Amiga USB Driver Library

This is one of my first Projects, which started in the early 2000s.
I resumed it days ago and uploaded it on GitHub, to share the code with everyone.

It attempts to create a USB driver stack for handling base devices like a USB mouse or joypad.
Due to my lack of experience then, the driver was incomplete, but the stack was tested and worked with a generic USB mouse and a Gravis gamepad.
The library was designed using Mediator PCI board with a USB 1.1 board installed on an A1200 tower system but theoretically can run on any USB board since implemented commands are standard.

# How to compile (Work in progress, I need time to reconstruct the entire compiling process since my A1200 died!)
The project is written in 68K Assembly and plain C.
You need to build the assembly library core using Asm-one devpack.
Then You need to build the C part using StormC Compiler 2.0.
Open the StormC project file 'USB/Usb/LupoUsbLibrary.s' and browse the project using StormC GUI platform.

