#include <exec/exec.h>
#include <dos/dos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h>
#include <stdio.h>
#include <string.h>

#include <Usb/HcDriverStructs.h>
#include <Usb/Protos/LupoUsbLibrary_protos_reg.h>
//#include <Usb/LupoUsbLibrary_lib.h>
#include <Usb/DefaultControlEndpoint.h>


struct Library *UsbLibrary=NULL;//OpenLibrary("lupousb.library",0L);
struct MsgPort *StackPort=NULL; /* port: was initialized with CreatePort before main() */

PUSB_SUPPORT   Support=NULL;
//PUSB_SUPPORT   RH_Supp=NULL;
PUSBD_REQUEST  UsbdRequest=NULL;
USB_DEVICE     UsbDeviceList[127]={0};
char			   StringBuffer[256]={0};

PUSB_DEVICE_DESCRIPTOR        dev0=NULL;
PUSB_CONFIGURATION_DESCRIPTOR con=NULL;

ENDPOINT_CONTROL EndpointControl0=
{
 0,                  /* Function Address */
 0,                  /* Endpoint Number  */
 GetDirectionFromTD, /* Direction        */
 0,   /* Speed            0=HighSpeed */
 0,   /* sKip             */
 0,   /* Format*/
 8,   /* MaxPacketSize    */
 0,   /* Halted           */
 0,   /* ToggleCarry      */
 0    /* Reserved         */
};

PENDPOINT_DESCRIPTOR ControlPipe[2]={NULL,NULL};

//-------------------------------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------------------------------


UBYTE SetupUsbDevice()
{
  // "Device 0" = Default control endpoint
  // "Device 1" = Root HUB
  ULONG i;
  ULONG flags= //USB_DEVICE_ATTACHED | USB_DEVICE_POWERED |
               USB_DEVICE_ADDRESS | USB_DEVICE_CONFIGURED;
  // Setup device number;
  for(i=0;i<127;i++) UsbDeviceList[i].USB_DEVICE_Number=i;
  //
  // Setup Device 0 (DEFAULT PIPE)
  //
  UsbDeviceList[0].USB_DEVICE_Status=flags;
  UsbDeviceList[0].USB_DEVICE_Status=flags;
  EndpointControl0.EC_Speed=0;
  ControlPipe[0]=AllocEndpoint(Support,Endpoint_Control,ED_CONTROL,&EndpointControl0);
  EndpointControl0.EC_Speed=1;
  ControlPipe[1]=AllocEndpoint(Support,Endpoint_Control,ED_CONTROL,&EndpointControl0);
  if(!(OpenPipe(Support,ControlPipe[0])))
   {
     FreeEndpoint(Support,ControlPipe[0]);
     FreeEndpoint(Support,ControlPipe[1]);
     return(0);
   }
  if(!(OpenPipe(Support,ControlPipe[1])))
   {
     FreeEndpoint(Support,ControlPipe[0]);
     FreeEndpoint(Support,ControlPipe[1]);
     return(0);
   }
  //
  // Setup Device 1 (ROOT HUB)
  //
  UsbDeviceList[1].USB_DEVICE_Status=flags|USB_DEVICE_LOCKED;
  return(1);
}


//-------------------------------------------------------------------------------------------------


void ClearUsbDevice()
{
     int i;
     FreeEndpoint(Support,ControlPipe[0]);
     FreeEndpoint(Support,ControlPipe[1]);
     for(i=0;i<127;i++)
     {
       FreeDeviceDescriptor(UsbDeviceList[i].USB_DEVICE_Descriptor);
     }

}


//-------------------------------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------------------------------


ULONG LockAddress(UBYTE FunctionNumber)
{
  if(! (UsbDeviceList[FunctionNumber].USB_DEVICE_Status & USB_DEVICE_LOCKED) )
  {
    UsbDeviceList[FunctionNumber].USB_DEVICE_Status |= USB_DEVICE_LOCKED;
    return(1);
  }
  return(0);
}


//-------------------------------------------------------------------------------------------------


void  UnLockAddress(UBYTE FunctionNumber)
{
    UsbDeviceList[FunctionNumber].USB_DEVICE_Status &= (~USB_DEVICE_LOCKED);
}


//-------------------------------------------------------------------------------------------------


UBYTE FindFreeAddress()
{
  UWORD i=1;
  while(i<128)
  {
    if(!UsbDeviceList[i].USB_DEVICE_Status)return(i);
    i++;
  }
  return(0);
}


//-------------------------------------------------------------------------------------------------

void SetUsbStatus(UBYTE FunctionNumber,ULONG Status)
{
  UsbDeviceList[FunctionNumber].USB_DEVICE_Status=Status;
}

//-------------------------------------------------------------------------------------------------


UBYTE InitFunction(UBYTE FunctionSpeed)
{
  //1: Find a Free Address
  //2: Get Device Descriptor (obtain default control endpoint for tath address)
  //3: Set Address
  //4: Set Function as Addressed
  //5: Create default control endpoint
  //6: Obtain Full Device Descriptor
  int i=0;
  UBYTE Address=FindFreeAddress();
  PENDPOINT_DESCRIPTOR ed=ControlPipe[0];
  if(FunctionSpeed)    ed=ControlPipe[1];
  if(GetDescriptor(Support,ed,UsbdRequest,DESCRIPTOR_DEVICE,0,0x08,0))
    if(SetAddress(Support,ed,UsbdRequest,Address))
    {
      EndpointControl0.EC_Speed            =FunctionSpeed;
      EndpointControl0.EC_FunctionAddress  =Address;
      EndpointControl0.EC_MaximumPacketSize=(UsbdRequest->USB_Buffer[6]<<8)+(UsbdRequest->USB_Buffer[7]&0xFF);
      UsbDeviceList[Address].USB_DEVICE_ControlPipe=AllocEndpoint(Support,Endpoint_Control,ED_CONTROL,&EndpointControl0);
      if(!(OpenPipe(Support,UsbDeviceList[Address].USB_DEVICE_ControlPipe)))
      {
        FreeEndpoint(Support,UsbDeviceList[Address].USB_DEVICE_ControlPipe);
        return(0);
      }
      UsbDeviceList[Address].USB_DEVICE_Descriptor=GetDeviceDescriptor(Support,UsbDeviceList[Address].USB_DEVICE_ControlPipe,UsbdRequest);
      if(UsbDeviceList[Address].USB_DEVICE_Descriptor->iManufacturer)
              GetString(Support,UsbDeviceList[Address].USB_DEVICE_ControlPipe,UsbdRequest,UsbDeviceList[Address].USB_DEVICE_Descriptor->iManufacturer,UsbDeviceList[Address].iManufacturer);
      if(UsbDeviceList[Address].USB_DEVICE_Descriptor->iProduct)
              GetString(Support,UsbDeviceList[Address].USB_DEVICE_ControlPipe,UsbdRequest,UsbDeviceList[Address].USB_DEVICE_Descriptor->iProduct,UsbDeviceList[Address].iProduct);
      if(UsbDeviceList[Address].USB_DEVICE_Descriptor->iSerialNumber)
              GetString(Support,UsbDeviceList[Address].USB_DEVICE_ControlPipe,UsbdRequest,UsbDeviceList[Address].USB_DEVICE_Descriptor->iSerialNumber,UsbDeviceList[Address].iSerialNumber);
      SetUsbStatus(Address,USB_DEVICE_ADDRESS);
      printf("\n Added New Device at address %d\n",Address);
      printf("%s\n",UsbDeviceList[Address].iManufacturer);
      printf("%s\n",UsbDeviceList[Address].iProduct);
      printf("%s\n",UsbDeviceList[Address].iSerialNumber);
      printf("bcdUSB.............:%#X\n",UsbDeviceList[Address].USB_DEVICE_Descriptor->bcdUSB);
      printf("bDeviceClass.......:%#X\n",UsbDeviceList[Address].USB_DEVICE_Descriptor->bDeviceClass);
      printf("bDeviceSubClass....:%#X\n",UsbDeviceList[Address].USB_DEVICE_Descriptor->bDeviceSubClass);
      printf("bDeviceProtocol....:%#X\n",UsbDeviceList[Address].USB_DEVICE_Descriptor->bDeviceProtocol);
      printf("bMaxPacketSize0....:%#X\n",UsbDeviceList[Address].USB_DEVICE_Descriptor->bMaxPacketSize0);
      printf("idVendor...........:%#X\n",UsbDeviceList[Address].USB_DEVICE_Descriptor->idVendor);
      printf("idProduct..........:%#X\n",UsbDeviceList[Address].USB_DEVICE_Descriptor->idProduct);
      printf("bcdDevice..........:%#X\n",UsbDeviceList[Address].USB_DEVICE_Descriptor->bcdDevice);
      printf("iManufacturer......:%#X\n",UsbDeviceList[Address].USB_DEVICE_Descriptor->iManufacturer);
      printf("iProduct...........:%#X\n",UsbDeviceList[Address].USB_DEVICE_Descriptor->iProduct);
      printf("iSerialNumber......:%#X\n",UsbDeviceList[Address].USB_DEVICE_Descriptor->iSerialNumber);
      printf("bNumConfigurations.:%#X\n",UsbDeviceList[Address].USB_DEVICE_Descriptor->bNumConfigurations);
      printf("\n");
      return(Address);
    }
  printf("Error UsbdStatus= %#X\n",UsbdRequest->USB_Status);

  return(0);
}


//-------------------------------------------------------------------------------------------------


void FreeFunction(UBYTE Address)
{
  int i;
  FreeEndpoint(Support,UsbDeviceList[Address].USB_DEVICE_ControlPipe);
  FreeDeviceDescriptor(UsbDeviceList[Address].USB_DEVICE_Descriptor);
  UsbDeviceList[Address].USB_DEVICE_Status=0;
  for(i=0;i<MAX_STRING_CHARS;i++)
  {
   UsbDeviceList[Address].iManufacturer[i]=0;
   UsbDeviceList[Address].iProduct[i]=0;
   UsbDeviceList[Address].iSerialNumber[i]=0;
   UsbDeviceList[Address].iConfiguration[i]=0;



  }
  printf("\n Device Removed at address %d\n",Address);
}


//-------------------------------------------------------------------------------------------------




//-------------------------------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------------------------------


void CloseAll()
{
 ClearUsbDevice();
 if (UsbdRequest) FreeUsbdRequest(Support,UsbdRequest);
 if (Support)
 {
     KillHost(Support);
     FreeUsbSupport(Support);
 }
 if (UsbLibrary)  CloseLibrary(UsbLibrary);
 if (StackPort)   DeletePort(StackPort);
 UsbLibrary=NULL;
 Support=NULL;
 StackPort=NULL;
 UsbdRequest=NULL;
}


//-------------------------------------------------------------------------------------------------


int OpenAll()
{
 if(!UsbLibrary)
   if(StackPort)
     if((Support=AllocUsbSupport()))
       if(InitUsbSupport(Support))
         if(SetupUsbDevice())
            if(UsbdRequest=AllocUsbdRequest(Support,4096))
	           return(1);
 return(0);
}


char Buffer[128]={0};
char Number[4]={0};

void ExecuteCommand(char* Command,UBYTE val)
{
 UBYTE tmp=val;
 Number[3]=0;
 Number[0]='0'+tmp/100;
 tmp=tmp%100;
 Number[1]='0'+tmp/10;
 tmp=tmp%10;
 Number[2]='0'+tmp;
 Number[3]=0;
 if(val<10) Number[1]=' ';
 if(val<100)Number[0]=' ';
 strcpy(Buffer,Command);
 strcat(Buffer," ");
 strcat(Buffer,Number);
 Execute(Buffer,NULL,NULL);
}




void main()
{
 /* port: global initializers of the StormC (C++) build */
 StackPort=CreatePort(UsbStackPortName,0L);

 int esci=0;
 ULONG segnale;
 PHC_REQUEST mymess;
 ExecuteCommand("run c:usbdriver",0);
 if(OpenAll())
 {
   ExecuteCommand("run c:AddUsbFunction",1);
   while(!esci)
   {
     segnale=Wait(1<<StackPort->mp_SigBit | SIGBREAKF_CTRL_C);
     if(segnale & SIGBREAKF_CTRL_C) esci=1;
     while(mymess=(PHC_REQUEST)GetMsg(StackPort))
     {
      mymess->IO_Error=NO_ERROR;
      switch(mymess->IO_Command)
      {
        case CMD_LOCK_FUNCTION:
             mymess->IO_Data=LockAddress(mymess->IO_Data);
        break;
        case CMD_UNLOCK_FUNCTION:
               UnLockAddress(mymess->IO_Data);
        break;
        case CMD_ALLOC_FUNCTION:
               mymess->IO_Data=InitFunction(mymess->IO_Data);
               ExecuteCommand("run c:AddUsbFunction",mymess->IO_Data);
        break;
        case CMD_FREE_FUNCTION:
               FreeFunction(mymess->IO_Data);
        break;
        case CMD_GET_USB_DEVICE:
               mymess->IO_Data=(ULONG)(&UsbDeviceList[mymess->IO_Data]);
        break;

//        case CMD_GET_CONTROL_ENDPOINT:
//        break;
//		  case CMD_SET_FUNCTION_STATE:
//		  break;
//        case CMD_KILL_USB_STACK:
//              esci=1;
//        break;
      }
      if (mymess->message.mn_ReplyPort)
 		{
 			ReplyMsg((struct Message*)mymess);
 		}
    }
   }
 }
// KillHost(Support);
 CloseAll();
}