     //----======------------===-----------^---------------------------
    //
   //   Funzioni esterne alla UsbLibrary (Test per vedere se sblocco)
  //    lo stallo (18 gen 2003)
 //__________________________________________________________/\________
//=======================----------------==============================

#ifndef USB_UTILITIES_PROTOS
#define USB_UTILITIES_PROTOS

#ifdef __cplusplus
extern "C" {
#endif

#include <exec/exec.h>
#include <usb/HcDriverStructs.h>

ULONG WaitUsbresult(PUSB_REQUESTER Req)

#ifdef __cplusplus
};
#endif
#endif
