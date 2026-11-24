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

// Nota: IEQUALIFIERB_xxx e' un numero di bit, non una maschera: la vecchia
// definizione includeva IEQUALIFIERB_MULTIBROADCAST (=11) che accendeva
// LSHIFT|RSHIFT|CONTROL su ogni evento.
#define  IEQUALIFIERPOS (IEQUALIFIER_RELATIVEMOUSE)
#define  FUNCTIONADDRESS 2

//---------------------------------------------------------------------
// USB Objects
//---------------------------------------------------------------------

struct Library *UsbLibrary=OpenLibrary("lupousb.library",0L);
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

struct InputEvent MouseEvent={0};
UBYTE  OldButtons=0;   // stato pulsanti dell'ultimo report

struct IOStdReq   InputRequest={0};
struct MsgPort    *Input_MP=CreatePort(NULL,0);

//---------------------------------------------------------------------

// Inietta un singolo evento RAWMOUSE in input.device.
// Viene chiamata dal task principale (proprietario di InputRequest e
// Input_MP), una volta per ogni evento: prima un task separato
// reiniettava l'ultimo evento ogni 1/50s, ripetendo i delta relativi.

void WriteMouseEvent(UWORD Code,UWORD Qualifier,WORD dx,WORD dy)
{
  MouseEvent.ie_NextEvent=NULL;
  MouseEvent.ie_Class    =IECLASS_RAWMOUSE;
  MouseEvent.ie_SubClass =0;
  MouseEvent.ie_Code     =Code;
  MouseEvent.ie_Qualifier=Qualifier;
  MouseEvent.ie_position.ie_xy.ie_x=dx;
  MouseEvent.ie_position.ie_xy.ie_y=dy;
  InputRequest.io_Command=IND_WRITEEVENT;
  InputRequest.io_Flags  =0;
  InputRequest.io_Length =sizeof(struct InputEvent);
  InputRequest.io_Data   =&MouseEvent;
  DoIO((struct IORequest*)&InputRequest);
}

// Converte un report HID boot mouse (byte0=pulsanti, byte1=dx, byte2=dy)
// in eventi Amiga: un evento per ogni pulsante che cambia stato
// (IECODE_xBUTTON o IECODE_xBUTTON|IECODE_UP_PREFIX), il movimento viaggia
// col primo evento; se nessun pulsante cambia si usa IECODE_NOBUTTON.

void ProcessMouseReport(UBYTE *Report)
{
  static const UBYTE Mask[3]={0x01,0x02,0x04};
  static const UWORD Code[3]={IECODE_LBUTTON,IECODE_RBUTTON,IECODE_MBUTTON};
  UBYTE Buttons=Report[0]&0x07;
  WORD  dx=(WORD)((BYTE)Report[1]);   // delta con segno
  WORD  dy=(WORD)((BYTE)Report[2]);
  UWORD Qual=IEQUALIFIERPOS;
  UBYTE Sent=0;
  int   b;

  if(Buttons&0x01) Qual|=IEQUALIFIER_LEFTBUTTON;
  if(Buttons&0x02) Qual|=IEQUALIFIER_RBUTTON;
  if(Buttons&0x04) Qual|=IEQUALIFIER_MIDBUTTON;

  for(b=0;b<3;b++)
  {
    if((Buttons^OldButtons)&Mask[b])
    {
      if(Buttons&Mask[b]) WriteMouseEvent(Code[b],Qual,dx,dy);
      else                WriteMouseEvent(Code[b]|IECODE_UP_PREFIX,Qual,dx,dy);
      dx=dy=0;
      Sent=1;
    }
  }
  if(!Sent && (dx || dy)) WriteMouseEvent(IECODE_NOBUTTON,Qual,dx,dy);
  OldButtons=Buttons;
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
 PENDPOINT_DESCRIPTOR ed=AllocEndpoint(Support,Endpoint_Interrupt,ED_INTERRUPT_16ms,&cont);
 if(ed)
 {
   if(OpenPipe(Support,ed))
   {
    // SET_PROTOCOL(Boot): garantisce il formato di report boot
    // (pulsanti, dx, dy) che ProcessMouseReport si aspetta.
    // Opzionale per i device che non lo supportano: l'esito si ignora.
    USB_Req->USB_Setup[0]=0x21;   // Class, Interface, Host->Device
    USB_Req->USB_Setup[1]=0x0B;   // SET_PROTOCOL
    USB_Req->USB_Setup[2]=0x00;   // wValue = 0 (Boot Protocol)
    USB_Req->USB_Setup[3]=0x00;
    USB_Req->USB_Setup[4]=0x00;   // wIndex = interfaccia 0
    USB_Req->USB_Setup[5]=0x00;
    USB_Req->USB_Setup[6]=0x00;   // wLength = 0
    USB_Req->USB_Setup[7]=0x00;
    USB_Req->USB_BufferLength=0;
    SendUsbData(Support,UsbMouse->USB_DEVICE_ControlPipe,USB_Req);
    printf("\nSET_PROTOCOL Result: %#X\n",USB_Req->USB_Status);

    // SET_IDLE: report ogni 4*4ms=16ms anche senza variazioni
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
    printf("SET_IDLE Result: %#X\n",USB_Req->USB_Status);

    OldButtons=0;
    for(i=0;i<200;i++)
    {
      USB_Req->USB_BufferLength=8;
      USB_Req->USB_ShortXferOk=1;
      // Usa il buffer solo se il trasferimento e' andato a buon fine
      // e contiene almeno pulsanti+dx+dy
      if(ReceiveUsbData(Support,ed,USB_Req) && USB_Req->USB_BytesCopied>=3)
         ProcessMouseReport((UBYTE*)USB_Req->USB_Buffer);
      else
         printf("%d: Receive error %#X\n",i,USB_Req->USB_Status);
    }
    // Rilascia eventuali pulsanti rimasti premuti
    if(OldButtons)
    {
      UBYTE Released[3]={0,0,0};
      ProcessMouseReport(Released);
    }
   }
   FreeEndpoint(Support,ed);   // libera l'endpoint anche se OpenPipe fallisce
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
   }
   FreeEndpoint(Support,ed);
 }
}


//---------------------------------------------------------------------

void main(void)
{
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