#include <stdio.h>
#include <stdlib.h>
#include <exec/exec.h>
#include <clib/exec_protos.h>
#include <intuition/intuition.h>
#include <clib/intuition_protos.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h>

#include <usb/protos/LupoUsbLibrary_protos.h>
#include <usb/LupoUsbLibrary_lib.h>

#include <hcDriverStructs.h>
#include <DefaultControlEndpoint.h>

struct Library *UsbLibrary=NULL; /* port: was initialized with OpenLibrary before main() */
PUSB_SUPPORT   Support=NULL;


void CloseAll()
{
 if (Support) FreeUsbSupport(Support);
 if (UsbLibrary) CloseLibrary(UsbLibrary);
 Support=NULL;
 UsbLibrary=NULL;
}


int OpenAll()
{
  if(UsbLibrary)
  if(Support=AllocUsbSupport())
  if(InitUsbSupport(Support))
    return(1);
  return(0);
}


void main(int argc,char **argv)
{
 /* port: global initializers of the StormC (C++) build */
 UsbLibrary=OpenLibrary("lupousb.library",0L);

 int FunctionNumber=atoi(argv[1]);
 if (FunctionNumber >1) (printf("Add Function %d\n",FunctionNumber));
 if (FunctionNumber==1) (printf("Add RootHub  \n",FunctionNumber));
 if(OpenAll() & FunctionNumber)
 {
   if(FunctionNumber==1) Execute("run c:UsbRootHub",NULL,NULL);
   if(FunctionNumber==0) printf("BUG Detecting Free Address!!!!\n");
   if(FunctionNumber>1)
   {
    // GetUsbFunction(FunctionNumber);
    // Test Class
    // Test SubClass
    // Lauch Driver
   }
 }
 CloseAll();
}

