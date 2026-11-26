#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

#define  IEQUALIFIERPOS IEQUALIFIER_RELATIVEMOUSE
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

// Sends one input event to input.device (from the main process: the old
// MouseTask was an Exec task calling DOS Delay(), replied on a port whose
// mp_SigTask was the main task, and resent the same event at every tick).

void SendInputEvent(struct InputEvent *ev)
{
    ev->ie_NextEvent=NULL;
    InputRequest.io_Command=IND_WRITEEVENT;
    InputRequest.io_Flags  =0;
    InputRequest.io_Length =sizeof(struct InputEvent);
    InputRequest.io_Data   =ev;
    DoIO((struct IORequest*)&InputRequest);
}

UWORD ButtonQualifiers(UBYTE buttons)
{
    UWORD q=IEQUALIFIERPOS;
    if(buttons & 0x01) q|=IEQUALIFIER_LEFTBUTTON;
    if(buttons & 0x02) q|=IEQUALIFIER_RBUTTON;
    if(buttons & 0x04) q|=IEQUALIFIER_MIDBUTTON;
    return(q);
}

// One RAWMOUSE event for the movement, then one event per button that
// changed state (IECODE_xBUTTON, with IECODE_UP_PREFIX when released):
// input.device and Intuition expect exactly one transition per event.

void InjectMouse(UBYTE *oldbuttons, UBYTE buttons, BYTE dx, BYTE dy)
{
    static const UWORD code[3]={IECODE_LBUTTON,IECODE_RBUTTON,IECODE_MBUTTON};
    struct InputEvent ev;
    int b;
    if(dx || dy)
    {
      memset(&ev,0,sizeof(ev));
      ev.ie_Class=IECLASS_RAWMOUSE;
      ev.ie_Code=IECODE_NOBUTTON;
      ev.ie_Qualifier=ButtonQualifiers(*oldbuttons);
      ev.ie_position.ie_xy.ie_x=dx;
      ev.ie_position.ie_xy.ie_y=dy;
      SendInputEvent(&ev);
    }
    for(b=0;b<3;b++)
    {
      UBYTE bit=1<<b;
      if((buttons ^ *oldbuttons) & bit)
      {
        *oldbuttons^=bit;
        memset(&ev,0,sizeof(ev));
        ev.ie_Class=IECLASS_RAWMOUSE;
        ev.ie_Code=code[b] | ((buttons & bit) ? 0 : IECODE_UP_PREFIX);
        ev.ie_Qualifier=ButtonQualifiers(*oldbuttons);
        SendInputEvent(&ev);
      }
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
 int i;
 int j=0;
 UBYTE buttons=0;

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
    printf("USB mouse at address %d: Ctrl-C to quit\n",FunctionNumber);
    while(!(SetSignal(0,0) & SIGBREAKF_CTRL_C))
    {
      USB_Req->USB_BufferLength=8;
      USB_Req->USB_ShortXferOk=1;
      if(!ReceiveUsbData(Support,ed,USB_Req)) continue;   // no new report
      // boot protocol report: buttons, X, Y (signed bytes)
      InjectMouse(&buttons,USB_Req->USB_Buffer[0]&0x07,
                  (BYTE)USB_Req->USB_Buffer[1],(BYTE)USB_Req->USB_Buffer[2]);
    }
    SetSignal(0,SIGBREAKF_CTRL_C);
    InjectMouse(&buttons,0,0,0);      // release buttons still pressed
   }
   FreeEndpoint(Support,ed);
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

int main(int argc,char **argv)
{
 // the USB address of the mouse (the stack prints it when the device is
 // added); before it was hard coded to 2
 UBYTE address=FUNCTIONADDRESS;
 if(argc>1) address=(UBYTE)atoi(argv[1]);
 /* port: global initializers of the StormC (C++) build */
 UsbLibrary=OpenLibrary("lupousb.library",0L);
 Input_MP=CreatePort(NULL,0);

 if(OpenAll())
 {
   if(!OpenDevice("input.device",0L,(struct IORequest *) &InputRequest,0L))
   {
     if(SetupMouse(address))
     {
      // interrupt endpoint of this device: address and speed of the device
      cont.EC_FunctionAddress=address;
      cont.EC_Speed=UsbMouse->USB_DEVICE_ControlPipe->ED_Control.EC_Speed;
      TestMouse(address);
//      TestForceFeedback(address);
      ClearMouse(address);
     }
     CloseDevice((struct IORequest*)&InputRequest);
   }
 }
 CloseAll();
 return(0);
}