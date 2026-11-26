#include <stdio.h>


#include <exec/exec.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h>


#include <devices/input.h>
#include <devices/inputevent.h>

#include <usb/protos/LupoUsbLibrary_protos.h>
#include <usb/LupoUsbLibrary_lib.h>

#include <hcDriverStructs.h>
#include <DefaultControlEndpoint.h>

#define  IEQUALIFIERPOS IEQUALIFIER_RELATIVEMOUSE | IEQUALIFIER_MULTIBROADCAST | IEQUALIFIERB_MULTIBROADCAST
#define  FUNCTIONADDRESS 2

//---------------------------------------------------------------------
// USB Objects
//---------------------------------------------------------------------

struct Library *UsbLibrary=NULL; /* port: was initialized with OpenLibrary before main() */
PUSB_SUPPORT   Support=NULL;
PUSBD_REQUEST  USB_Req=NULL;
PUSB_DEVICE    UsbMouse=NULL;
PENDPOINT_DESCRIPTOR ed=NULL;

ENDPOINT_CONTROL cont=
{
 FUNCTIONADDRESS,   /* Function Address */
 1,   /* Endpoint Number  */
 IN,  /* Direction        */
 1,   /* Speed            0=HighSpeed */
 0,   /* sKip             */
 0,   /* Format*/
 8,   /* MaxPacketSize    */
 0,   /* Halted           */
 0,   /* ToggleCarry      */
 0    /* Reserved         */
};

ENDPOINT_CONTROL contout=
{
 FUNCTIONADDRESS,   /* Function Address */
 1,   /* Endpoint Number  */
 OUT, /* Direction        */
 1,   /* Speed            0=HighSpeed */
 0,   /* sKip             */
 0,   /* Format*/
 8,   /* MaxPacketSize    */
 0,   /* Halted           */
 0,   /* ToggleCarry      */
 0    /* Reserved         */
};


//---------------------------------------------------------------------
//  Input Objects
//---------------------------------------------------------------------

struct InputEvent InputEvent1={0};
struct InputEvent InputEvent2={0};
ULONG  InputLocked=0;
ULONG  TaskEsci=0;

struct IOStdReq   InputRequest={0};
struct MsgPort    *Input_MP=NULL; /* port: was initialized with CreatePort before main() */

//---------------------------------------------------------------------

void MouseTask()
{
  while(!TaskEsci)
  {
    Delay(1);
    InputRequest.io_Command=IND_WRITEEVENT;
    InputRequest.io_Flags  =IOF_QUICK;
    InputRequest.io_Length =sizeof(struct InputEvent);
    if(!InputLocked) InputRequest.io_Data = &InputEvent2;
    else				 InputRequest.io_Data = &InputEvent1;
    DoIO((struct IORequest*)&InputRequest);
  }
}


//---------------------------------------------------------------------


void CloseAll()
{
 if (USB_Req)    FreeUsbdRequest(Support,USB_Req);
 if (Support)    FreeUsbSupport(Support);
 if (UsbLibrary) CloseLibrary(UsbLibrary);
 if (Input_MP)  DeletePort(Input_MP);
 USB_Req=NULL;
 Support=NULL;
 UsbLibrary=NULL;
 Input_MP=NULL;
}


//---------------------------------------------------------------------


int OpenAll()
{
  if(Input_MP)
   {
     Input_MP->mp_SigTask = FindTask(NULL);
     InputRequest.io_Message.mn_ReplyPort=Input_MP;
     InputEvent1.ie_Class=IECLASS_RAWMOUSE;
     InputEvent2.ie_Class=IECLASS_RAWMOUSE;
   }
  if(UsbLibrary)
  if(Support=AllocUsbSupport())
  if(InitUsbSupport(Support))
  if(USB_Req=AllocUsbdRequest(Support,256))
    return(1);
  return(0);
}

//---------------------------------------------------------------------

UBYTE SetupMouse(UBYTE FunctionNumber)
{
  if(LockFunction(Support,FunctionNumber))
  {
    if(UsbMouse=GetUsbDevice(Support,FunctionNumber))
    {
      ed=UsbMouse->USB_DEVICE_ControlPipe;
      if(UsbMouse->USB_DEVICE_Configuration=GetConfigurationDescriptor(Support,ed,USB_Req,0))
      {
        if(SetConfiguration(Support,ed,USB_Req,1))
        {
            UsbMouse->USB_DEVICE_Status|=USB_DEVICE_CONFIGURED;
            return(1);
        }
        FreeConfigurationDescriptor(UsbMouse->USB_DEVICE_Configuration);
      }
    }
    UnLockFunction(Support,FunctionNumber);
  }
  return(0);
}


//---------------------------------------------------------------------

void ClearMouse(UBYTE FunctionNumber)
{
  SetConfiguration(Support,ed,USB_Req,0);
  UsbMouse->USB_DEVICE_Status&=~USB_DEVICE_CONFIGURED;
  UsbMouse->USB_DEVICE_Status|=USB_DEVICE_ADDRESS;
  FreeConfigurationDescriptor(UsbMouse->USB_DEVICE_Configuration);
  UnLockFunction(Support,FunctionNumber);
}

void TestMouse(UBYTE FunctionNumber)
{
 struct InputEvent *MouseEvent;
 int i;
 int j=0;
 struct Task *MyTask=NULL;

 char str[4]={0x0d,0x0a,0x0c,0};
 PENDPOINT_DESCRIPTOR ed=AllocEndpoint(Support,Endpoint_Interrupt,ED_INTERRUPT_16ms,&cont);
 if(ed)
 {
   if(OpenPipe(Support,ed))
   {

    // Setup Idle interrupt for a mouse

    USB_Req->USB_Setup[0]=0x21;
    USB_Req->USB_Setup[1]=0x0A;
    USB_Req->USB_Setup[2]=0x00;
    USB_Req->USB_Setup[3]=0x04;
    USB_Req->USB_Setup[4]=0x00;
    USB_Req->USB_Setup[5]=0x00;
    USB_Req->USB_Setup[6]=0x00;
    USB_Req->USB_Setup[7]=0x00;
    USB_Req->USB_BufferLength=0;
    SendUsbData(Support,UsbMouse->USB_DEVICE_ControlPipe,USB_Req);
    printf("\nResult: %#X\n",USB_Req->USB_Status);
    MyTask=CreateTask("USB MOUSE TASK",0,MouseTask,4096);
    for(i=0;i<200;i++)
    {
      printf("%d\n",i);
      USB_Req->USB_BufferLength=8;
      USB_Req->USB_ShortXferOk=1;
      ReceiveUsbData(Support,ed,USB_Req);

      // convert usb mouse data in a choerent InputEvnt

      if(InputLocked==0) MouseEvent=&InputEvent1;
      else               MouseEvent=&InputEvent2;
	   InputLocked=!InputLocked;

      MouseEvent->ie_Qualifier=IEQUALIFIERPOS;

		MouseEvent->ie_Code=0;
      if(  USB_Req->USB_Buffer[0] & 0x01)  MouseEvent->ie_Code |= IECODE_LBUTTON;
      if(  USB_Req->USB_Buffer[0] & 0x02)  MouseEvent->ie_Code |= IECODE_RBUTTON;
      if(  USB_Req->USB_Buffer[0] & 0x04)  MouseEvent->ie_Code |= IECODE_MBUTTON;
      if(!(USB_Req->USB_Buffer[0] & 0x07)) MouseEvent->ie_Code  = IECODE_NOBUTTON;

      MouseEvent->ie_Qualifier=IEQUALIFIERPOS | IEQUALIFIER_RELATIVEMOUSE;
      MouseEvent->ie_position.ie_xy.ie_x=((WORD)USB_Req->USB_Buffer[1]);
      MouseEvent->ie_position.ie_xy.ie_y=((WORD)USB_Req->USB_Buffer[2]);
    }
   FreeEndpoint(Support,ed);
   }
 }
}



void TestForceFeedback(UBYTE FunctionNumber)
{
 int i;
 int j=0;
 PENDPOINT_DESCRIPTOR ed=AllocEndpoint(Support,Endpoint_Interrupt,ED_INTERRUPT_16ms,&contout);
 if(ed)
 {
   if(OpenPipe(Support,ed))
   {

    // Setup Idle interrupt for a mouse

    USB_Req->USB_Setup[0]=0x21;
    USB_Req->USB_Setup[1]=0x0A;
    USB_Req->USB_Setup[2]=0x00;
    USB_Req->USB_Setup[3]=0x0A;
    USB_Req->USB_Setup[4]=0x00;
    USB_Req->USB_Setup[5]=0x00;
    USB_Req->USB_Setup[6]=0x00;
    USB_Req->USB_Setup[7]=0x00;
    USB_Req->USB_BufferLength=0;
    SendUsbData(Support,UsbMouse->USB_DEVICE_ControlPipe,USB_Req);
    printf("\nResult: %#X\n",USB_Req->USB_Status);
    for(i=0;i<200;i++)
    {
      printf("%d\n",i);
      USB_Req->USB_BufferLength=8;
      USB_Req->USB_ShortXferOk=1;
      USB_Req->USB_XferInfo=OUT;
      USB_Req->USB_Buffer[0]=i;
      USB_Req->USB_Buffer[1]=i;
      USB_Req->USB_Buffer[2]=i;
      USB_Req->USB_Buffer[3]=i;
      USB_Req->USB_Buffer[4]=i;
      USB_Req->USB_Buffer[5]=i;
      USB_Req->USB_Buffer[6]=i;
      USB_Req->USB_Buffer[7]=i;

      SendUsbData(Support,ed,USB_Req);
      printf("Result: %#X\n",USB_Req->USB_Status);
//	   WaitTOF();
    }
    TaskEsci=1;
   }
   FreeEndpoint(Support,ed);
 }
}


//---------------------------------------------------------------------

void main(void)
{
 /* port: global initializers of the StormC (C++) build */
 UsbLibrary=OpenLibrary("lupousb.library",0L);
 Input_MP=CreatePort(NULL,0);

 if(OpenAll())
 {
   if(!OpenDevice("input.device",0L,(struct IORequest *) &InputRequest,0L))
   {
     if(SetupMouse(FUNCTIONADDRESS))
     {
      TestMouse(FUNCTIONADDRESS);
//      TestForceFeedback(FUNCTIONADDRESS);
      ClearMouse(FUNCTIONADDRESS);
     }
     CloseDevice((struct IORequest*)&InputRequest);
   }
 }
 CloseAll();
}