/*
 * LupoUsbDriver - mousepos: prints the position of the Intuition pointer.
 * Used by the emulator test to check that USB mouse events injected into
 * input.device really move the pointer.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>

struct IntuitionBase *IntuitionBase;

int main(int argc, char **argv)
{
    IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR)"intuition.library", 36);
    if (!IntuitionBase) {
        printf("mousepos: no intuition.library\n");
        return 20;
    }
    printf("mousepos %s: x=%d y=%d\n", argc > 1 ? argv[1] : "", IntuitionBase->MouseX, IntuitionBase->MouseY);
    CloseLibrary((struct Library *)IntuitionBase);
    return 0;
}
