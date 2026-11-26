#include <stdio.h>
#include <exec/exec.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h>

#include <usb/HcDriverStructs.h>
#include <usb/HubClass.h>
#include <usb/Protos/HubClass_protos.h>

#include <usb/Protos/LupoUsbLibrary_protos.h>
#include <usb/LupoUsbLibrary_Lib.h>


struct MsgPort *ReplyPort =CreatePort(UsbKernelRhPortName,0L);
struct Library *UsbLibrary=OpenLibrary("lupousb.library",0L);
PUSB_REQUESTER Req=NULL;


void ProcessHubStatusRegister(ULONG HubStatus)
{
}


void ProcessPort(ULONG PortStatus,UBYTE i)
{

 //----------------------------------------

 if(PortStatus & (1<<C_PORT_CONNECTION))
 { // (Un) Connection Detected
   ClearPortFeature(Req,NULL,NULL,C_PORT_CONNECTION,i);
   if(PortStatus & (1<<PORT_CONNECTION))
   {
     Delay(20);
     SetPortFeature(Req,NULL,NULL,PORT_RESET,i);
   }
   else
   {
     //RemoveDevice
   }
 }
 //----------------------------------------

 if(PortStatus & (1<<C_PORT_RESET))
   {
     Delay(10);
     ClearPortFeature(Req,NULL,NULL,C_PORT_RESET,i);
   }

 //----------------------------------------

 if(PortStatus & (1<<C_PORT_ENABLE))
   {
     ClearPortFeature(Req,NULL,NULL,C_PORT_ENABLE,i);
   }

 //----------------------------------------

 if(PortStatus & (1<<C_PORT_SUSPEND))
   {
     ClearPortFeature(Req,NULL,NULL,C_PORT_SUSPEND,i);
   }

 //----------------------------------------

 if(PortStatus & (1<<C_PORT_OVER_CURRENT))
   {
     ClearPortFeature(Req,NULL,NULL,C_PORT_OVER_CURRENT,i);
   }
}


void ProcessHub()
{
  UBYTE i;
  PROOT_HUB_MESSAGE HubMess;
  while ( (HubMess=(PROOT_HUB_MESSAGE)GetMsg(Req->UserPort)) )
  {
    if  (HubMess->Status_Change & 0x01)
    {
           HubMess->Status_Change &=0xFFFE;
      	  ProcessHubStatusRegister(GetHubStatus(Req,NULL,NULL));
    }
    for (i=1;i<HubMess->NumPorts+1;i++)
	    if  ((HubMess->Status_Change)&(1<<i))
	    {
	        HubMess->Status_Change&=~(1<<i);
   	     ProcessPort(GetPortStatus(Req,NULL,NULL,i),i);
   	 }
  }
}




void main(void)
{
  ULONG esci=0;
  if (UsbLibrary)
  {
    if ((Req=AllocUsbRequester()))
    {
      printf("Hub Port Detected\n");
      if(InitUsbRequester(Req,ReplyPort))
      {
        if(SetupRhMessagePort(Req,ReplyPort))
        {
          printf("Powering ports!\n");
          SetPortFeature(Req,NULL,NULL,PORT_POWER,1);
    	    SetPortFeature(Req,NULL,NULL,PORT_POWER,2);
     	    printf("now Looping\n");
    	    while(FindPort(UsbKernelHcPortName))
    	    {
    	       printf("Waiting hub\n");
        	    WaitCSC(Req,NULL);
      	    printf("Processing hub\n");
      	    if(FindPort(UsbKernelHcPortName)) ProcessHub();
    	    }
    	    printf("Usb Kernel Killed, now Quitting\n");
    	  }
    	  else printf("Cannot Setup Root Hub Port\n");
       }
       else printf("Cannot Init Usb Requester\n");
    }
    else printf("Cannot Alloate Usb Requester (Kernel not found)\n");
  }
  if (ReplyPort) DeletePort(ReplyPort);
  if (Req)FreeUsbRequester(Req);
  if (UsbLibrary)CloseLibrary(UsbLibrary);
}
