//------------------------------------------------------------------------
//
// Lupo Usb Library 2003 by Zioben (zioben@inwind.it painaga@dei.unipd.it)
//
// Configurazione dei comandi
//
//
//------------------------------------------------------------------------

#include <usb/HcDriverStructs.h>
#include <usb/protos/LupoUsbLibrary_protos_registers.h>

#include <exec/exec.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <pragma/exec_lib.h>
#include <pragma/dos_lib.h>

#pragma libbase Library;

UWORD SwapWord(UWORD a __asm("d0"))
{
  return(((a>>8)+(a<<8)));
}

//---------------------------------------------------------------------------------------------------------------------------

UBYTE WaitUsbResult(PUSB_REQUESTER Req __asm("a0"))
{
  WaitPort(Req->UserPort);
  while((Req->DummyMess=GetMsg(Req->UserPort)));
  if(Req->UsbRequest.IO_Error==0) return(1);
  return(0);
}

//---------------------------------------------------------------------------------------------------------------------------

UBYTE WaitResult(PUSB_REQUESTER req __asm("a0"), PUSBD_REQUEST usb __asm("a1"))
{
  if(WaitUsbResult(req))
    if(WaitUsbdRequest(usb)) return(1);
    else(req->UsbRequest.IO_Error=ERR_USBD_REQUEST);
  return(1);
}

//---------------------------------------------------------------------------------------------------------------------------

void SetupUsbdRequest(PUSBD_REQUEST usb __asm("a0"),
							 UBYTE a __asm("d0"),
							 UBYTE b __asm("d1"),
							 UWORD c __asm("d2"),
							 UWORD d __asm("d3"),
							 UWORD e __asm("d4"))
{
  usb->Setup[0]=a;
  usb->Setup[1]=b;
  usb->Setup[2]=c&0xFF;
  usb->Setup[3]=c>>8;
  usb->Setup[4]=d&0xFF;
  usb->Setup[5]=d>>8;
  usb->Setup[6]=e&0xFF;
  usb->Setup[7]=e>>8;
  usb->BufferLength=e;
}

//---------------------------------------------------------------------------------------------------------------------------

UBYTE AsyncSendRequest(PUSB_REQUESTER req __asm("a0"),
							  PHCD_ENDPOINT ed __asm("a1"),
							  PUSBD_REQUEST usb __asm("a2"))
{
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  PutMsg(KernelPort,(struct Message*)&req->UsbRequest);
  return(WaitUsbResult(req));
}

//---------------------------------------------------------------------------------------------------------------------------

UBYTE SyncSendRequest(PUSB_REQUESTER req __asm("a0"),
			  				 PHCD_ENDPOINT ed __asm("a1"),
							 PUSBD_REQUEST usb __asm("a2"))
{
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  PutMsg(KernelPort,(struct Message*)&req->UsbRequest);
  return(WaitResult(req,usb));
}



//---------------------------------------------------------------------------------------------------------------------------

UBYTE ClearFeature(PUSB_REQUESTER req __asm("a0"),
						 PHCD_ENDPOINT ed __asm("a1"),
						 PUSBD_REQUEST usb __asm("a2"),
						 ULONG recipient __asm("d0"),
						 UWORD Feature __asm("d1"),
						 UBYTE num __asm("d2"))
{
  // Init comune a tutti i campi:
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  switch (recipient)
  {
  	case RECIPIENT_DEVICE:  { SetupUsbdRequest(usb,0x00,CLEAR_FEATURE,Feature,0,0);break; }
  	case RECIPIENT_ENDPOINT:{ SetupUsbdRequest(usb,0x01,CLEAR_FEATURE,Feature,num | 0x80,0);break; }
 	default:  					{ SetupUsbdRequest(usb,0x02,CLEAR_FEATURE,Feature,num,0);break; }
  }
  return(SyncSendRequest(req,ed,usb));
}

//---------------------------------------------------------------------------------------------------------------------------

UBYTE GetConfiguration(PUSB_REQUESTER req __asm("a0"),
							  PHCD_ENDPOINT ed __asm("a1"),
							  PUSBD_REQUEST usb __asm("a2"))
{

  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  SetupUsbdRequest(usb,0x80,GET_CONFIGURATION,0,0,1);
  return(SyncSendRequest(req,ed,usb));
}

//------------------------------------------------------------------------------------------------------------------

UBYTE GetDescriptor(PUSB_REQUESTER req __asm("a0"),
						  PHCD_ENDPOINT ed __asm("a1"),
						  PUSBD_REQUEST usb __asm("a2"),
						  UBYTE type __asm("d0"),
						  UBYTE index __asm("d1"),
						  UWORD Leng __asm("d2"),
						  UWORD LangID __asm("d3"))
{

  if (ed->HcdED == NULL)           {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  if (usb->BufferTotalLength<Leng) {req->UsbRequest.IO_Error=ERR_INSUFFICIENT_USB_BUFFER;return(0);}
  SetupUsbdRequest(usb,0x80,GET_DESCRIPTOR,type<<8+index,LangID,Leng);
  return(SyncSendRequest(req,ed,usb));
}

//------------------------------------------------------------------------------------------------------------------

UBYTE GetInterface(PUSB_REQUESTER req __asm("a0"),
						 PHCD_ENDPOINT ed __asm("a1"),
						 PUSBD_REQUEST usb __asm("a2"),
						 UBYTE NInterface __asm("d0"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  SetupUsbdRequest(usb,0x81,GET_INTERFACE,0,NInterface,1);
  return(SyncSendRequest(req,ed,usb));
}




//------------------------------------------------------------------------------------------------------------------


UWORD GetStatus(PUSB_REQUESTER req __asm("a0"),
					 PHCD_ENDPOINT ed __asm("a1"),
					 PUSBD_REQUEST usb __asm("a2"),
					 ULONG Recipient __asm("d0"),
					 UBYTE NumInterface __asm("d1"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  switch(Recipient)
  {
	case RECIPIENT_DEVICE:     { SetupUsbdRequest(usb,0x80,GET_STATUS,0,0,2); break;}
	case	RECIPIENT_ENDPOINT:  { SetupUsbdRequest(usb,0x82,GET_STATUS,0,NumInterface,2); break;}
	default:							{ SetupUsbdRequest(usb,0x81,GET_STATUS,0,NumInterface,2); break;}
  }
  return(SyncSendRequest(req,ed,usb));
}


//------------------------------------------------------------------------------------------------------------------

UBYTE SetAddress(PUSB_REQUESTER req __asm("a0"),
					  PHCD_ENDPOINT ed __asm("a1"),
					  PUSBD_REQUEST usb __asm("a2"),
					  UBYTE Address __asm("d0"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  SetupUsbdRequest(usb,0x00,SET_ADDRESS,Address,0,0);
  return(SyncSendRequest(req,ed,usb));
}

//------------------------------------------------------------------------------------------------------------------

UBYTE SetConfiguration(PUSB_REQUESTER req __asm("a0"),
							  PHCD_ENDPOINT ed __asm("a1"),
							  PUSBD_REQUEST usb __asm("a2"),
							  UBYTE Value __asm("d0"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  SetupUsbdRequest(usb,0x00,SET_CONFIGURATION,Value,0,0);
  return(SyncSendRequest(req,ed,usb));
}

//------------------------------------------------------------------------------------------------------------------

UBYTE SetDescriptor(PUSB_REQUESTER req __asm("a0"),
						  PHCD_ENDPOINT ed __asm("a1"),
						  PUSBD_REQUEST usb __asm("a2"),
						  UBYTE Type __asm("d0"),
						  UBYTE Index __asm("d1"),
						  UWORD Leng __asm("d2"),
						  UWORD LangID __asm("d3"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  if (Leng>usb->BufferTotalLength) return(0);
  SetupUsbdRequest(usb,0x00,SET_DESCRIPTOR,Type<<8+Index,LangID,Leng);
  return(SyncSendRequest(req,ed,usb));
}

//------------------------------------------------------------------------------------------------------------------

UBYTE SetFeature(PUSB_REQUESTER req __asm("a0"),
					  PHCD_ENDPOINT ed __asm("a1"),
					  PUSBD_REQUEST usb __asm("a2"),
					  ULONG Recipient __asm("d0"),
					  UBYTE Interface __asm("d1"),
					  UWORD Feature __asm("d2"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  switch(Recipient)
  {
   case RECIPIENT_DEVICE:  { SetupUsbdRequest(usb,0x00,SET_FEATURE,Feature,0,0); break;}
  	case RECIPIENT_ENDPOINT:{ SetupUsbdRequest(usb,0x01,SET_FEATURE,Feature,Interface | 0x80,0);break; }
 	default:  					{ SetupUsbdRequest(usb,0x02,SET_FEATURE,Feature,Interface,0); break;}
  }
  return(SyncSendRequest(req,ed,usb));
}


//------------------------------------------------------------------------------------------------------------------


UBYTE SetInterface(PUSB_REQUESTER req __asm("a0"),
						 PHCD_ENDPOINT ed __asm("a1"),
						 PUSBD_REQUEST usb __asm("a2"),
						 UBYTE Interface __asm("d0"),
						 UBYTE AltInterface __asm("d1"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  SetupUsbdRequest(usb,0x01,SET_INTERFACE,AltInterface,Interface,0);
  return(SyncSendRequest(req,ed,usb));
}

//------------------------------------------------------------------------------------------------------------------

UBYTE SynchFrame(PUSB_REQUESTER req __asm("a0"),
					  PHCD_ENDPOINT ed __asm("a1"),
					  PUSBD_REQUEST usb __asm("a2"),
					  UWORD Frame __asm("d0"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  if (ed->Type != Endpoint_Isochronous) return(0);
  if (usb->BufferTotalLength<2)         return(0);
  SetupUsbdRequest(usb,0x82,SYNCH_FRAME,0,ed->Control.EndpointNumber | 0x80,2);
  usb->Buffer[0]=Frame&0xFF;									// Setup Data
  usb->Buffer[1]=(Frame>>8)&0xFF;							// Setup Data									// Data=2;
  return(AsyncSendRequest(req,ed,usb));
}

//------------------------------------------------------------------------------------------------------------------
//
//		Funzioni speciali scritte ad hoc
//
//------------------------------------------------------------------------------------------------------------------

UBYTE ClearDeviceFeature(PUSB_REQUESTER req __asm("a0"),
								 PHCD_ENDPOINT ed __asm("a1"),
								 PUSBD_REQUEST usb __asm("a2"),
								 UWORD Feature __asm("d0"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  SetupUsbdRequest(usb,0x00,CLEAR_FEATURE,Feature,0,0);
  return(SyncSendRequest(req,ed,usb));
}

//------------------------------------------------------------------------------------------------------------------

UBYTE ClearEndpointFeature(PUSB_REQUESTER req __asm("a0"),
									PHCD_ENDPOINT ed __asm("a1"),
									PUSBD_REQUEST usb __asm("a2"),
									UWORD Feature __asm("d0"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  SetupUsbdRequest(usb,0x02,CLEAR_FEATURE,Feature,ed->Control.EndpointNumber | 0x80,0);
  return(SyncSendRequest(req,ed,usb));
}

//------------------------------------------------------------------------------------------------------------------

UBYTE ClearInterfaceFeature(PUSB_REQUESTER req __asm("a0"),
									 PHCD_ENDPOINT ed __asm("a1"),
									 PUSBD_REQUEST usb __asm("a2"),
									 UWORD Feature __asm("d0"),
									 UBYTE NumInterface __asm("d1"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  SetupUsbdRequest(usb,0x01,CLEAR_FEATURE,Feature,NumInterface,0);
  return(SyncSendRequest(req,ed,usb));
}

//------------------------------------------------------------------------------------------------------------------

UBYTE SetDeviceFeature(PUSB_REQUESTER req __asm("a0"),
							  PHCD_ENDPOINT ed __asm("a1"),
							  PUSBD_REQUEST usb __asm("a2"),
							  UWORD Feature __asm("d0"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  SetupUsbdRequest(usb,0x00,SET_FEATURE,Feature,0,0);
  return(SyncSendRequest(req,ed,usb));
}

//------------------------------------------------------------------------------------------------------------------

UBYTE SetEndpointFeature(PUSB_REQUESTER req __asm("a0"),
								 PHCD_ENDPOINT ed __asm("a1"),
								 PUSBD_REQUEST usb __asm("a2"),
								 UWORD Feature __asm("d0"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  SetupUsbdRequest(usb,0x02,SET_FEATURE,Feature,ed->Control.EndpointNumber | 0x80,0);
  return(SyncSendRequest(req,ed,usb));
}

//------------------------------------------------------------------------------------------------------------------

UBYTE SetInterfaceFeature(PUSB_REQUESTER req __asm("a0"),
								  PHCD_ENDPOINT ed __asm("a1"),
								  PUSBD_REQUEST usb __asm("a2"),
								  UWORD Feature __asm("d0"),
								  UBYTE NumInterface __asm("d1"))
{
  if (ed->HcdED == NULL) {req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;return(0);}
  SetupUsbdRequest(usb,0x01,SET_FEATURE,Feature,NumInterface,0);
  return(SyncSendRequest(req,ed,usb));
}

//------------------------------------------------------------------------
//					Descriptor Macros
//------------------------------------------------------------------------

PDEVICE_DESCRIPTOR GetDeviceDescriptor(PUSB_REQUESTER req __asm("a0"),
												   PHCD_ENDPOINT ed __asm("a1"),
												   PUSBD_REQUEST usb __asm("a2"))
{
  PDEVICE_DESCRIPTOR a,b;
  if((GetDescriptor(req,ed,usb,DESCRIPTOR_DEVICE,0,0,sizeof(DEVICE_DESCRIPTOR))))
  {
    while(!(a=AllocMem(sizeof(DEVICE_DESCRIPTOR),MEMF_PUBLIC|MEMF_CLEAR)));
    b=(PDEVICE_DESCRIPTOR)usb->Buffer;
    a->bLength           = b->bLength;
    a->bDescriptorType   = b->bDescriptorType;
    a->bcdUSB			    = SwapWord(b->bcdUSB);
    a->bDeviceClass	    = b->bDeviceClass;
    a->bDeviceSubClass   = b->bDeviceSubClass;
    a->bDeviceProtocol   = b->bDeviceProtocol;
    a->bMaxPacketSize0   = b->bMaxPacketSize0;
    a->idVendor		    = SwapWord(b->idVendor);
    a->idProduct		    = SwapWord(b->idProduct);
    a->bcdDevice		    = SwapWord(b->bcdDevice);
    a->iManufacturer     = b->iManufacturer;
    a->iProduct		    = b->iProduct;
    a->iSerialNumber     = b->iSerialNumber;
    a->bNumConfigurations= b->bNumConfigurations;
    return(a);
  }
  return(NULL);
}

//------------------------------------------------------------------------------------------------------------------

void FreeDeviceDescriptor(PDEVICE_DESCRIPTOR dev __asm("a0"))
{
 if(dev)FreeMem(dev,sizeof(DEVICE_DESCRIPTOR));
}

//------------------------------------------------------------------------------------------------------------------

PCONFIGURATION_DESCRIPTOR GetConfigurationDescriptor(PUSB_REQUESTER req __asm("a0"),
																	   PHCD_ENDPOINT ed __asm("a1"),
																	   PUSBD_REQUEST usb __asm("a2"),
																	   UBYTE ConfValue __asm("d0"))
{
  PCONFIGURATION_DESCRIPTOR a,b;
  if((GetDescriptor(req,ed,usb,DESCRIPTOR_CONFIGURATION,ConfValue,0,sizeof(CONFIGURATION_DESCRIPTOR))))
  {
    while(!(a=AllocMem(sizeof(CONFIGURATION_DESCRIPTOR),MEMF_PUBLIC|MEMF_CLEAR)));
    b=(PCONFIGURATION_DESCRIPTOR)usb->Buffer;
    a->bLength             = b->bLength;
    a->bDescriptorType     = b->bDescriptorType;
    a->wTotalLength		   = SwapWord(b->wTotalLength);
    a->bNumInterfaces      = b->bNumInterfaces;
    a->bConfigurationValue = b->bConfigurationValue;
    a->iConfiguration      = b->iConfiguration;
    a->bmAttributes        = b->bmAttributes;
    a->MaxPower			   = b->MaxPower;
    return(a);
  }
  return(NULL);
}

//------------------------------------------------------------------------------------------------------------------

void FreeConfigurationDescriptor(PDEVICE_DESCRIPTOR conf __asm("a0"))
{
 if(conf)FreeMem(conf,sizeof(CONFIGURATION_DESCRIPTOR));
}
