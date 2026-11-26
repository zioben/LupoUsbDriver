/* LupoUsbDriver legacy port: replaces the StormC "#pragma amicall" file of
 * lupousb.library with gcc inline calls generated from LupoUsbLibrary.fd. */
#ifndef LEGACY_LUPOUSBLIBRARY_LIB_H
#define LEGACY_LUPOUSBLIBRARY_LIB_H
#include <exec/libraries.h>
extern struct Library *UsbLibrary;
#include <inline/lupousb.h>
#endif
