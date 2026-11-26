#include <stdio.h>
#include <exec/exec.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h>


#include <usb/protos/LupoUsbLibrary_protos.h>
#include <usb/LupoUsbLibrary_lib.h>

#include <usb/HcDriverStructs.h>
#include <usb/HubClass.h>
#include <usb/Protos/HubClass_protos.h>


struct Library *UsbLibrary=NULL; /* port: was initialized with OpenLibrary before main() */

PUSB_SUPPORT Support=NULL;
PUSB_SUPPORT RH_Supp=NULL;
char  FunctionAddress[16]={0};
ULONG PendingStatus[16]={0};
ULONG ILockSomething=0;

void CloseAll()
{
 if (RH_Supp) FreeUsbSupport(RH_Supp);
 if (Support) FreeUsbSupport(Support);
 if (UsbLibrary) CloseLibrary(UsbLibrary);
 RH_Supp=Support=NULL;
 UsbLibrary=NULL;
}


int OpenAll()
{
  if(UsbLibrary)
  if(RH_Supp=AllocUsbSupport())
  if(Support=AllocUsbSupport())
  if(InitUsbSupport(RH_Supp))
  if(InitUsbSupport(Support))
    return(1);
  CloseAll();
  return(0);
}




void ProcessHubStatusRegister(ULONG HubStatus)
{
}


void ProcessPort(ULONG PortStatus,UBYTE i)
{

 //----------------------------------------

 if(PortStatus & (1<<C_PORT_CONNECTION))
 { // Connection Detected
   ClearPortFeature(RH_Supp,NULL,NULL,C_PORT_CONNECTION,i);
   if(PortStatus & (1<<PORT_CONNECTION))
   {
     // Connection detected:
     // if (Lock Function) Reset the device
     // else Remember the status
     if(!ILockSomething)
     {
       while(LockFunction(Support,0))
       {
         printf("I'm waiting the end of activity of another hub\n");
         Delay(50);
       }
//     else
//     if((LockFunction(Support,0)))
       {
//         Delay(10);
         printf("Root Hub: Processing device\n");
         SetPortFeature(RH_Supp,NULL,NULL,PORT_RESET,i);
         ILockSomething=1;
         PendingStatus[i]=0;
       }
     }
     else
       {
         printf("Root Hub: Pending Device\n");
         PendingStatus[i]=PortStatus;
       }

   }
   else
   {
     // Unconnection Detected: remove function
     if(FunctionAddress[i])
         UsbRemoveFunction(Support,FunctionAddress[i]);
         PendingStatus[i]=0;
   }
 }
 //----------------------------------------

 if(PortStatus & (1<<C_PORT_RESET))
   {
     // Device Resetted...
     // now respond at device pipe 0
     ClearPortFeature(RH_Supp,NULL,NULL,C_PORT_RESET,i);
     if(PortStatus&(1<<PORT_LOW_SPEED))
        FunctionAddress[i]=UsbInitFunction(Support,1);
     else
        FunctionAddress[i]=UsbInitFunction(Support,0);
     UnLockFunction(Support,0);
     ILockSomething=0;
     // solve pending device
     for (i=0;i<16;i++)
     {
      if(PendingStatus[i])
      {
        ULONG tmp=PendingStatus[i];
        printf("Root Hub: Solving Device %d\n",i);
        ProcessPort(tmp,i);
      }
     }

   }

 //----------------------------------------

 if(PortStatus & (1<<C_PORT_ENABLE))
   {
     ClearPortFeature(RH_Supp,NULL,NULL,C_PORT_ENABLE,i);
   }

 //----------------------------------------

 if(PortStatus & (1<<C_PORT_SUSPEND))
   {
     ClearPortFeature(RH_Supp,NULL,NULL,C_PORT_SUSPEND,i);
   }

 //----------------------------------------

 if(PortStatus & (1<<C_PORT_OVER_CURRENT))
   {
     ClearPortFeature(RH_Supp,NULL,NULL,C_PORT_OVER_CURRENT,i);
   }
}


void ProcessHub()
{
  UBYTE i,Esci=0;
  PROOT_HUB_MESSAGE HubMessShared;
  ROOT_HUB_MESSAGE  HubCopy;
  PROOT_HUB_MESSAGE HubMess=&HubCopy;
  while (!Esci)
  {
    HubMessShared=(PROOT_HUB_MESSAGE)GetMsg(RH_Supp->UserPort);

    if  (HubMessShared==NULL)Esci=1;
    else
    {
       // The kernel's interrupt handler keeps updating the message: take a
       // consistent copy and reset the change bits atomically, then work
       // on the copy.
       Disable();
       HubCopy=*HubMessShared;
       if (HubMessShared->Status_Change!=0xFFFF) HubMessShared->Status_Change=0;
       Enable();
       if  ((HubMess->Status_Change==0xFFFF))
       {
         Esci=1;
         Delay(100);
       }
       else
       {
          if (HubMess->Status_Change & 0x01)
          {
           HubMess->Status_Change &=0xFFFE;
      	  ProcessHubStatusRegister(GetHubStatus(RH_Supp,NULL,NULL));
          }
          for (i=1;i<HubMess->NumPorts+1;i++)
	       if((HubMess->Status_Change)&(1<<i))
	       {
	        HubMess->Status_Change&=~(1<<i);
   	     ProcessPort(GetPortStatus(RH_Supp,NULL,NULL,i),i);
   	    }
   	 }
    }
  }
}




void main(void)
{
 /* port: global initializers of the StormC (C++) build */
 UsbLibrary=OpenLibrary("lupousb.library",0L);

  ULONG esci=0;
  if (OpenAll())
  {
    printf("Root hub: Hello World\n");
//   printf("Lock 2: %d\n",LockFunction(Req,2));
//   printf("Lock 2: %d\n",LockFunction(Req,2));
//   UnLockFunction(Req,2);


   if(SetupRootHubMessagePort(RH_Supp,RH_Supp->UserPort))
   {
     //printf("Powering ports!\n");
     //ClearPortFeature(RH_Supp,NULL,NULL,PORT_POWER,1);
     //ClearPortFeature(RH_Supp,NULL,NULL,PORT_POWER,2);
     //Delay(20);
     SetPortFeature(RH_Supp,NULL,NULL,PORT_POWER,1);
     SetPortFeature(RH_Supp,NULL,NULL,PORT_POWER,2);
     //printf("now Looping\n");
     while(FindPort(UsbKernelHcPortName))
     {
        //printf("Waiting hub\n");
     	  WaitCSC(RH_Supp,NULL);
     	  //printf("Processing hub\n");
     	  ProcessHub();
     }
   }
  }
  CloseAll();
  printf("Root Hub: Bye Bye\n");
}
