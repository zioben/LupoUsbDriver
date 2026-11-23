#include <exec/exec.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <hcdriverstructs.h>
#include <protos/lupousblibrary_protos.h>
#include <lupousblibrary_lib.h>

struct Library *UsbLibrary=OpenLibrary("lupousb.library",0L);
struct MsgPort *UserPort =CreatePort(NULL,0L);
PUSB_SUPPORT UsbSupport;


void main()
{
 if(UsbLibrary)
 {
   if(UsbSupport=AllocUsbSupport())
   {
    InitUsbSupport(UsbSupport,UserPort);
    FreeUsbSupport(UsbSupport);
   }
   CloseLibrary(UsbLibrary);
 }
 DeletePort(UserPort);
}
