       //----------^---------------------^-----------===========----------------
      //
     // Lupo Usb Library 2003 by Zioben (zioben@inwind.it painaga@dei.unipd.it)
    //
   // Configurazione dei comandi
  //
 //      ___________________
//------<___________________>-------------===========---------------------------


#include <exec/exec.h>
#include <pragma/exec_lib.h>
#include <pragma/dos_lib.h>
#include <usb/HcDriverStructs.h>
#include <usb/protos/LupoUsbLibrary_protos_reg.h>
#include <usb/DefaultControlEndpoint.h>
#include <clib/alib_protos.h>
// --  Base Pragmas ----------

#pragma libbase Library;

struct MsgPort *KernelPort;
struct MsgPort *StackPort;


//---------------------------------------------------------------------------------------------------------------------------

ULONG __saveds GetUsbError(register __a0 PUSB_SUPPORT Req)
{
  if(Req) return (Req->UsbRequest.IO_Error);
  return(NULL);
}

//------------------------------------------------------------------------
//				   Utilità gestione HostController
//------------------------------------------------------------------------

UBYTE __saveds SetupRootHubMessagePort(register __a0 PUSB_SUPPORT Req,
                                       register __a1 struct MsgPort *Port)
{
    if((!Req)||(!Port))return(0);
    Req->UsbRequest.IO_Command=CMD_SETUP_ROOT_HUB_PORT;
    Req->UsbRequest.IO_Data   =(ULONG)Port;
    return(SendUsbRequest(Req,KernelPort));
}

UBYTE __saveds FreeRootHubMessagePort(register __a0 PUSB_SUPPORT Req,
                                       register __a1 struct MsgPort *Port)
{
    if(Req && Port)
    {
      Req->UsbRequest.IO_Command=CMD_FREE_ROOT_HUB_PORT;
      Req->UsbRequest.IO_Data   =(ULONG)Port;
      return(SendUsbRequest(Req,KernelPort));
    }
}



//---------------------------------------------------------------------------------------------------------------------------

ULONG __saveds Get32BitFrameNumber(register __a0 PUSB_SUPPORT Req)
{
    if(!Req) return(0);
    Req->UsbRequest.IO_Command=CMD_GET_32BIT_FN;
    return(SendUsbRequest(Req,KernelPort));
}

//---------------------------------------------------------------------------------------------------------------------------

PHCD_DEVICE_DATA __saveds GetUsbBaseAddress(register __a0 PUSB_SUPPORT Req)
{
  if(!Req)return(NULL);
  Req->UsbRequest.IO_Command=CMD_GET_USB_BASE;
  if(SendUsbRequest(Req,KernelPort)==0)
      return((PHCD_DEVICE_DATA)Req->UsbRequest.IO_Data);
  return(NULL);
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   KillHost(register __a0 PUSB_SUPPORT Req)
{
  if(Req)
  {
    Req->UsbRequest.IO_Command=CMD_KILL_USB;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   StartHost(register __a0 PUSB_SUPPORT Req)
{
  if(Req)
  {
    Req->UsbRequest.IO_Command=CMD_START_HOST;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   StopHost(register __a0 PUSB_SUPPORT Req)
{
  if(Req)
  {
    Req->UsbRequest.IO_Command=CMD_STOP_HOST;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   PauseHost(register __a0 PUSB_SUPPORT Req)
{
  if(Req)
  {
    Req->UsbRequest.IO_Command=CMD_PAUSE_HOST;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------


UBYTE __saveds   SetUsbState(register __a0 PUSB_SUPPORT Req,
					  register __d0 ULONG usbstate)
{
  if(!Req)return(0);
  Req->UsbRequest.IO_Command=CMD_SET_USB_STATE;
  return(SendUsbRequest(Req,KernelPort));
}

//---------------------------------------------------------------------------------------------------------------------------

ULONG __saveds   GetUsbState(register __a0 PUSB_SUPPORT Req)
{
  if(!Req)return(-1);
  Req->UsbRequest.IO_Command=CMD_GET_USB_STATE;
  if(SendUsbRequest(Req,KernelPort))
     return(Req->UsbRequest.IO_Data);
  return(-1);
}



//------------------------------------------------------------------------
//				   ALLOCATE DEALLOCATE USB MEMORY BUFFER
//------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------


char * __saveds   AllocUsbMem(register __a0 PUSB_SUPPORT Req,
						register __d0 ULONG BufferLeng)
{
  if((!BufferLeng) || (!Req)) return(NULL);
  Req->UsbRequest.IO_Command=CMD_ALLOCMEM;
  Req->UsbRequest.IO_Data=BufferLeng;
  if(SendUsbRequest(Req,KernelPort))
  return((char*)Req->UsbRequest.IO_Data);
  return(NULL);
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   FreeUsbMem(register __a0 PUSB_SUPPORT Req,
					 register __a1 char *Mem)
{
  if(Req && Mem)
  {
    Req->UsbRequest.IO_Command=CMD_FREEMEM;
    Req->UsbRequest.IO_Data=(ULONG)Mem;
    SendUsbRequest(Req,KernelPort);
    Mem=NULL;
  }
}




//------------------------------------------------------------------------
//					ALLOCATE DEALLOCATE USB REQUESTER
//------------------------------------------------------------------------
PUSB_SUPPORT AllocUsbSupport(void)
{
 KernelPort=FindPort(UsbKernelHcPortName);
 StackPort =FindPort(UsbStackPortName);
 if(KernelPort && StackPort)
   return(AllocMem(sizeof(USB_SUPPORT),MEMF_PUBLIC|MEMF_CLEAR));
 return(NULL);
}


//---------------------------------------------------------------------------------------------------------------------------

UBYTE __saveds   InitUsbSupport(register __a0 PUSB_SUPPORT Req)
{
 if(!Req) return(0);
 while (!(Req->UserPort=CreatePort(NULL,0)));
 Req->UsbRequest.IO_Command=CMD_GET_USB_BASE;
 Req->UsbRequest.message.mn_ReplyPort=Req->UserPort;
 if(SendUsbRequest(Req,KernelPort))
 {
     Req->devicedata=(PHCD_DEVICE_DATA) Req->UsbRequest.IO_Data;
     return(1);
 }
 return(0);
}

//---------------------------------------------------------------------------------------------------------------------------


void __saveds   FreeUsbSupport(register __a0 PUSB_SUPPORT Req)
{
 if (Req)
 {
   DeletePort(Req->UserPort);
   FreeMem(Req,sizeof(USB_SUPPORT));
   Req=NULL;
 }
}


//------------------------------------------------------------------------
//					ENDPOINT REQUESTER
//------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------

PENDPOINT_DESCRIPTOR __saveds   AllocEndpoint(register __a0 PUSB_SUPPORT Req,
									 register __d0 ULONG type,
									 register __d1 ULONG rate,
									 register __a1 PENDPOINT_CONTROL cont)
{
  if(!Req) return(NULL);
  if(!cont->EC_MaximumPacketSize) return(NULL);
  Req->UsbRequest.IO_Command=CMD_ALLOC_ENDPOINT;
  Req->UsbRequest.IO_Buffer[0]=type;
  Req->UsbRequest.IO_Buffer[1]=rate;
  Req->UsbRequest.IO_Buffer[2]=(ULONG)cont;
  if(type==Endpoint_Control)       Req->UsbRequest.IO_Buffer[1]=ED_CONTROL;
  if(type==Endpoint_Bulk)          Req->UsbRequest.IO_Buffer[1]=ED_BULK;
  if(type==Endpoint_Isochronous)   Req->UsbRequest.IO_Buffer[1]=ED_ISOCHRONOUS;
  if(SendUsbRequest(Req,KernelPort))
       return ((PENDPOINT_DESCRIPTOR)  Req->UsbRequest.IO_Data);
  else return(NULL);
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   FreeEndpoint(register __a0 PUSB_SUPPORT Req,
									  register __a1 PENDPOINT_DESCRIPTOR Endpoint)
{
  if (Req && Endpoint)
  {
    Req->UsbRequest.IO_Command=CMD_FREE_ENDPOINT;
    Req->UsbRequest.IO_Data=(ULONG)Endpoint;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   GetEndpointData(register __a0 PUSB_SUPPORT Req,
										  register __a1 PENDPOINT_DESCRIPTOR Endpoint)
{
 if (Req && Endpoint)
  {
    Req->UsbRequest.IO_Command=CMD_GET_ENDPOINT_DATA;
    Req->UsbRequest.IO_Data=(ULONG)Endpoint;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   SetEndpointData(register __a0 PUSB_SUPPORT Req,
							register __a1 PENDPOINT_DESCRIPTOR Endpoint)
{
 if (Req && Endpoint)
  {
    Req->UsbRequest.IO_Command=CMD_SET_ENDPOINT_DATA;
    Req->UsbRequest.IO_Data=(ULONG)Endpoint;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------

UBYTE __saveds   OpenPipe(register __a0 PUSB_SUPPORT Req,
				  register __a1 PENDPOINT_DESCRIPTOR ed)
{
 if(Req && ed)
 {
    if(!ed->ED_Reserved[0])
    {
      Req->UsbRequest.IO_Command=CMD_OPEN_PIPE;
      Req->UsbRequest.IO_Data=(ULONG)ed;
      return(SendUsbRequest(Req,KernelPort));
    }
    return(0);
 }
 return(0);
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   ClosePipe(register __a0 PUSB_SUPPORT Req,
					 register __a1 PENDPOINT_DESCRIPTOR ed)
{
 if(Req && ed)
 {
    Req->UsbRequest.IO_Command=CMD_CLOSE_PIPE;
    Req->UsbRequest.IO_Data=(ULONG)ed;
    SendUsbRequest(Req,KernelPort);
 }
}

//------------------------------------------------------------------------
//					USBD REQUESTER
//------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------

PUSBD_REQUEST __saveds   AllocUsbdRequest(register __a0 PUSB_SUPPORT Req,
										 register __d0 ULONG BufferLeng)
{
  if(!Req)return(NULL);
  Req->UsbRequest.IO_Command=CMD_ALLOC_USBD_REQUEST;
  Req->UsbRequest.IO_Data=BufferLeng;
  if(SendUsbRequest(Req,KernelPort))
  {
       while(! (((PUSBD_REQUEST)Req->UsbRequest.IO_Data)->USB_MsgPort=CreatePort(NULL,0)));
       ((PUSBD_REQUEST)Req->UsbRequest.IO_Data)->USB_DummyMessage.mn_ReplyPort=((PUSBD_REQUEST)Req->UsbRequest.IO_Data)->USB_MsgPort;
       return((PUSBD_REQUEST)Req->UsbRequest.IO_Data);
  }
  else return(NULL);
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   FreeUsbdRequest(register __a0 PUSB_SUPPORT Req,
							register __a1 PUSBD_REQUEST usb)
{
  if (Req && usb)
  {
    while(GetMsg(usb->USB_MsgPort));
    DeletePort(usb->USB_MsgPort);
    usb->USB_MsgPort=NULL;
    Req->UsbRequest.IO_Command=CMD_FREE_USBD_REQUEST;
    Req->UsbRequest.IO_Data=(ULONG)usb;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------


void __saveds   AbortUsbdRequest(register __a0 PUSB_SUPPORT Req,
							 register __a1 PUSBD_REQUEST usb)
{
  if (Req && usb)
  {
    Req->UsbRequest.IO_Command=CMD_ABORT_USBD_REQUEST;
    Req->UsbRequest.IO_Data=(ULONG)usb;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------


void __saveds   FreeUsbdRequestBuffer(register __a0 PUSB_SUPPORT Req,
									register __a1 PUSBD_REQUEST usb)
{
  if (Req && usb)
  {
  	  FreeUsbMem(Req,usb->USB_Buffer);
  	  usb->USB_BufferLength=0;
  	  usb->USB_BufferTotalLength=0;
  	  usb->USB_BytesCopied=0;
  }
}

//---------------------------------------------------------------------------------------------------------------------------

ULONG __saveds   AllocUsbdRequestBuffer(register __a0 PUSB_SUPPORT Req,
										register __a1 PUSBD_REQUEST usb,
										register __d0 ULONG BufferLeng)
{
  if ((!Req)||(!usb)||(!BufferLeng)) return(0);
  FreeUsbMem(Req,usb->USB_Buffer);
  usb->USB_BufferTotalLength=0;
  usb->USB_BufferLength=0;
  usb->USB_BytesCopied=0;
  if((usb->USB_Buffer=AllocUsbMem(Req,BufferLeng))) usb->USB_BufferTotalLength=BufferLeng;
    return(usb->USB_BufferTotalLength);
  return(0);
}


//----------------------------------------------------------------------------------------------------------------------
//				Usb Default Command Request
//----------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   SetupUsbdRequest( register __a0 PUSBD_REQUEST usb,
											 register __d0 UBYTE a,
											 register __d1 UBYTE b,
											 register __d2 UWORD c,
											 register __d3 UWORD d,
											 register __d4 UWORD e)
{
  if(usb)
  {
    usb->USB_Setup[0]=a;
    usb->USB_Setup[1]=b;
    usb->USB_Setup[2]=c&0xFF;
    usb->USB_Setup[3]=c>>8;
    usb->USB_Setup[4]=d&0xFF;
    usb->USB_Setup[5]=d>>8;
    usb->USB_Setup[6]=e&0xFF;
    usb->USB_Setup[7]=e>>8;
    usb->USB_BufferLength=e;
  }
}

//---------------------------------------------------------------------------------------------------------------------------

UBYTE __saveds   SendUsbdRequest( register __a0 PUSB_SUPPORT Req,
						 					 register __a1 PENDPOINT_DESCRIPTOR ed,
											 register __a2 PUSBD_REQUEST usb)
{
  if((!Req) || (!ed) || (!usb)) return(0);
  Req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  Req->UsbRequest.IO_Data=(ULONG)usb;
  Req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  return(SendUsbRequest(Req,KernelPort));
}

//---------------------------------------------------------------------------------------------------------------------------

ULONG __saveds   WaitUsbdRequest( register __a0 PUSB_SUPPORT support,register __a1 PUSBD_REQUEST usb)
{
  if(!usb)return(0);
  //  usb->USB_Reserved=0;
  //  while ((usb->USB_Status==USBD_NOT_DONE) && (usb->USB_Reserved<50))
  WaitPort(usb->USB_MsgPort);
  {
     while(usb->USB_Message=GetMsg(usb->USB_MsgPort)) ;
  }
  if(usb->USB_Status==USBD_NOT_DONE)
  {
    support->UsbRequest.IO_Command=CMD_ABORT_USBD_REQUEST;
    support->UsbRequest.IO_Data=(ULONG)(usb);
    SendUsbRequest(support,KernelPort);
    usb->USB_Status=USBD_ERR_NOT_ACCESSED;
    return(0);
  }
  if(usb->USB_Status==USBD_COMPLETE) return(1);
  return(0);
}

//---------------------------------------------------------------------------------------------------------------------------

UBYTE __saveds  SendSyncUsbdRequest( register __a0 PUSB_SUPPORT Req,
						 					    register __a1 PENDPOINT_DESCRIPTOR ed,
											    register __a2 PUSBD_REQUEST usb)
{
  if((!Req) || (!ed) || (!usb)) return(0);
  Req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  Req->UsbRequest.IO_Data=(ULONG)usb;
  Req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(Req,KernelPort))
  {
    return(WaitUsbdRequest(Req,usb));
  }
  return(0);
}


//---------------------------------------------------------------------------------------------------------------------------


UBYTE	__saveds SendUsbData( register __a0 PUSB_SUPPORT Req,
						 		    register __a1 PENDPOINT_DESCRIPTOR ed,
								    register __a2 PUSBD_REQUEST usb)
{
  if((!Req) || (!ed) || (!usb)) return(0);
  if(usb->USB_Buffer==NULL && usb->USB_BufferLength!=0)
  {
    Req->UsbRequest.IO_Error=ERR_BAD_ARGUMENTS;
    usb->USB_Status=USBD_ERR_NOT_ACCESSED;
    return(0);
  }
  usb->USB_XferInfo=OUT;
  return(SendSyncUsbdRequest(Req,ed,usb));
}



//---------------------------------------------------------------------------------------------------------------------------


UBYTE __saveds ReceiveUsbData( register __a0 PUSB_SUPPORT Req,
						 				 register __a1 PENDPOINT_DESCRIPTOR ed,
									    register __a2 PUSBD_REQUEST usb)
{
 if((!Req) || (!ed) || (!usb)) return(0);
 if(usb->USB_Buffer==NULL && usb->USB_BufferLength!=0)
  {
    Req->UsbRequest.IO_Error=ERR_BAD_ARGUMENTS;
    usb->USB_Status=USBD_ERR_NOT_ACCESSED;
    return(0);
  }
  usb->USB_XferInfo=IN;
  return(SendSyncUsbdRequest(Req,ed,usb));
}


//---------------------------------------------------------------------------------------------------------------------------
//           USB DEFAULT CONTROL ENDPOINT SUPPORT
//---------------------------------------------------------------------------------------------------------------------------



//---------------------------------------------------------------------------------------------------------------------------

UBYTE __saveds ClearFeature(register __a0 PUSB_SUPPORT req,
						 register __a1 PENDPOINT_DESCRIPTOR ed,
						 register __a2 PUSBD_REQUEST usb,
						 register __d0 ULONG recipient,
						 register __d1 UWORD Feature,
						 register __d2 UBYTE num)
{
  // Test Correttezza dei dati:
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  switch (recipient)
  {
  	case RECIPIENT_DEVICE:  { SetupUsbdRequest(usb,0x00,CLEAR_FEATURE,Feature,0,0);break; }
  	case RECIPIENT_ENDPOINT:{ SetupUsbdRequest(usb,0x01,CLEAR_FEATURE,Feature,num | 0x80,0);break; }
 	default:  					{ SetupUsbdRequest(usb,0x02,CLEAR_FEATURE,Feature,num,0);break; }
  }
  usb->USB_XferInfo=OUT;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);
}

//---------------------------------------------------------------------------------------------------------------------------

UBYTE __saveds GetConfiguration(register __a0 PUSB_SUPPORT req,
							  register __a1 PENDPOINT_DESCRIPTOR ed,
							  register __a2 PUSBD_REQUEST usb)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  SetupUsbdRequest(usb,0x80,GET_CONFIGURATION,0,0,1);
  usb->USB_XferInfo=IN;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);
}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds GetDescriptor(register __a0 PUSB_SUPPORT req,
						  register __a1 PENDPOINT_DESCRIPTOR ed,
						  register __a2 PUSBD_REQUEST usb,
						  register __d0 UBYTE type,
						  register __d1 UBYTE index,
						  register __d2 UWORD Leng,
						  register __d3 UWORD LangID)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  if(Leng==0)
   {
     req->UsbRequest.IO_Error=ERR_BAD_ARGUMENTS;
     return(0);

   }
  if (usb->USB_BufferTotalLength<Leng)
   {
     req->UsbRequest.IO_Error=ERR_INSUFFICIENT_USB_BUFFER;
     return(0);
   }
  usb->USB_XferInfo=IN;
  SetupUsbdRequest(usb,0x80,GET_DESCRIPTOR,(index&0xFF)+(type<<8),LangID,Leng);
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);
}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds GetInterface(register __a0 PUSB_SUPPORT req,
						 register __a1 PENDPOINT_DESCRIPTOR ed,
						 register __a2 PUSBD_REQUEST usb,
						 register __d0 UBYTE NInterface)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  SetupUsbdRequest(usb,0x81,GET_INTERFACE,0,NInterface,1);
  usb->USB_XferInfo=IN;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);
}




//------------------------------------------------------------------------------------------------------------------


UWORD __saveds GetStatus(register __a0 PUSB_SUPPORT req,
					 register __a1 PENDPOINT_DESCRIPTOR ed,
					 register __a2 PUSBD_REQUEST usb,
					 register __d0 ULONG Recipient,
					 register __d1 UBYTE NumInterface)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  switch(Recipient)
  {
	case RECIPIENT_DEVICE:     { SetupUsbdRequest(usb,0x80,GET_STATUS,0,0,2); break;}
	case	RECIPIENT_ENDPOINT:  { SetupUsbdRequest(usb,0x82,GET_STATUS,0,NumInterface,2); break;}
	default:							{ SetupUsbdRequest(usb,0x81,GET_STATUS,0,NumInterface,2); break;}
  }
  usb->USB_XferInfo=IN;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);
}


//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds SetAddress(register __a0 PUSB_SUPPORT req,
					  register __a1 PENDPOINT_DESCRIPTOR ed,
					  register __a2 PUSBD_REQUEST usb,
					  register __d0 UBYTE Address)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  SetupUsbdRequest(usb,0x00,SET_ADDRESS,Address,0,0);
  usb->USB_XferInfo=OUT;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);
}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds SetConfiguration(register __a0 PUSB_SUPPORT req,
							  register __a1 PENDPOINT_DESCRIPTOR ed,
							  register __a2 PUSBD_REQUEST usb,
							  register __d0 UBYTE Value)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  SetupUsbdRequest(usb,0x00,SET_CONFIGURATION,Value,0,0);
  usb->USB_XferInfo=OUT;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);

}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds SetDescriptor(register __a0 PUSB_SUPPORT req,
						  register __a1 PENDPOINT_DESCRIPTOR ed,
						  register __a2 PUSBD_REQUEST usb,
						  register __d0 UBYTE Type,
						  register __d1 UBYTE Index,
						  register __d2 UWORD Leng,
						  register __d3 UWORD LangID)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  if (Leng>usb->USB_BufferTotalLength) return(0);
  SetupUsbdRequest(usb,0x00,SET_DESCRIPTOR,(Type<<8)+(Index&0xFF),LangID,Leng);
  usb->USB_XferInfo=OUT;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);

}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds SetFeature(register __a0 PUSB_SUPPORT req,
					  register __a1 PENDPOINT_DESCRIPTOR ed,
					  register __a2 PUSBD_REQUEST usb,
					  register __d0 ULONG Recipient,
					  register __d1 UBYTE Interface,
					  register __d2 UWORD Feature)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  switch(Recipient)
  {
   case RECIPIENT_DEVICE:  { SetupUsbdRequest(usb,0x00,SET_FEATURE,Feature,0,0); break;}
  	case RECIPIENT_ENDPOINT:{ SetupUsbdRequest(usb,0x01,SET_FEATURE,Feature,Interface | 0x80,0);break; }
 	default:  					{ SetupUsbdRequest(usb,0x02,SET_FEATURE,Feature,Interface,0); break;}
  }
  usb->USB_XferInfo=OUT;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);

}


//------------------------------------------------------------------------------------------------------------------


UBYTE __saveds SetInterface(register __a0 PUSB_SUPPORT req,
						 register __a1 PENDPOINT_DESCRIPTOR ed,
						 register __a2 PUSBD_REQUEST usb,
						 register __d0 UBYTE Interface,
						 register __d1 UBYTE AltInterface)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  SetupUsbdRequest(usb,0x01,SET_INTERFACE,AltInterface,Interface,0);
  usb->USB_XferInfo=OUT;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);

}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds SynchFrame(register __a0 PUSB_SUPPORT req,
					  register __a1 PENDPOINT_DESCRIPTOR ed,
					  register __a2 PUSBD_REQUEST usb,
					  register __d0 UWORD Frame)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  if (ed->ED_Type != Endpoint_Isochronous) return(0);
  if (usb->USB_BufferTotalLength<2)         return(0);
  SetupUsbdRequest(usb,0x82,SYNCH_FRAME,0,ed->ED_Control.EC_EndpointNumber | 0x80,2);
  usb->USB_XferInfo=OUT;
  usb->USB_Buffer[0]=Frame&0xFF;									// Setup Data
  usb->USB_Buffer[1]=(Frame>>8)&0xFF;							// Setup Data									// Data=2;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);
}

//------------------------------------------------------------------------------------------------------------------
//
//		Funzioni speciali scritte ad hoc
//
//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds ClearDeviceFeature(register __a0 PUSB_SUPPORT req,
								 register __a1 PENDPOINT_DESCRIPTOR ed,
								 register __a2 PUSBD_REQUEST usb,
								 register __d0 UWORD Feature)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  SetupUsbdRequest(usb,0x00,CLEAR_FEATURE,Feature,0,0);
  usb->USB_XferInfo=OUT;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);

}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds ClearEndpointFeature(register __a0 PUSB_SUPPORT req,
									register __a1 PENDPOINT_DESCRIPTOR ed,
									register __a2 PUSBD_REQUEST usb,
									register __d0 UWORD Feature)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  SetupUsbdRequest(usb,0x02,CLEAR_FEATURE,Feature,ed->ED_Control.EC_EndpointNumber | 0x80,0);
  usb->USB_XferInfo=OUT;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);
}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds ClearInterfaceFeature(register __a0 PUSB_SUPPORT req,
									 register __a1 PENDPOINT_DESCRIPTOR ed,
									 register __a2 PUSBD_REQUEST usb,
									 register __d0 UWORD Feature,
									 register __d1 UBYTE NumInterface)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  SetupUsbdRequest(usb,0x01,CLEAR_FEATURE,Feature,NumInterface,0);
  usb->USB_XferInfo=OUT;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);
}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds SetDeviceFeature(register __a0 PUSB_SUPPORT req,
							  register __a1 PENDPOINT_DESCRIPTOR ed,
							  register __a2 PUSBD_REQUEST usb,
							  register __d0 UWORD Feature)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  SetupUsbdRequest(usb,0x00,SET_FEATURE,Feature,0,0);
  usb->USB_XferInfo=OUT;
  req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data=(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);

}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds SetEndpointFeature(register __a0 PUSB_SUPPORT req,
                                  register __a1 PENDPOINT_DESCRIPTOR ed,
                                  register __a2 PUSBD_REQUEST usb,
                                  register __d0 UWORD Feature)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  SetupUsbdRequest(usb,0x02,SET_FEATURE,Feature,ed->ED_Control.EC_EndpointNumber | 0x80,0);
  usb->USB_XferInfo           =OUT;
  req->UsbRequest.IO_Command  =CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data     =(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))
  {
    return(WaitUsbdRequest(req,usb));
  }
  return(0);

//  return(SendSyncUsbdRequest(req,ed,usb));
}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds SetInterfaceFeature(register __a0 PUSB_SUPPORT req,
								  register __a1 PENDPOINT_DESCRIPTOR ed,
								  register __a2 PUSBD_REQUEST usb,
								  register __d0 UWORD Feature,
								  register __d1 UBYTE NumInterface)
{
  if((!req) || (!ed) || (!usb))return(0);
  if(!ed->ED_Reserved[0])
   {
     usb->USB_Status=USBD_PIPE_NOT_OPENED;
     req->UsbRequest.IO_Error=ERR_PIPE_NOT_OPENED;
     return(0);
   }
  SetupUsbdRequest(usb,0x01,SET_FEATURE,Feature,NumInterface,0);
  usb->USB_XferInfo           =OUT;
  req->UsbRequest.IO_Command  =CMD_SEND_USBD_REQUEST;
  req->UsbRequest.IO_Data     =(ULONG)usb;
  req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  if(SendUsbRequest(req,KernelPort))  return(WaitUsbdRequest(req,usb));
  return(0);
}

//------------------------------------------------------------------------------------------------------------------
//  Macros GET_DESCRIPTOR:  DEVICE - CONFIGURATION - INTERFACE - ENDPOINT
//------------------------------------------------------------------------------------------------------------------

PUSB_DEVICE_DESCRIPTOR __saveds GetDeviceDescriptor(register __a0 PUSB_SUPPORT req,
												       register __a1 PENDPOINT_DESCRIPTOR ed,
												       register __a2 PUSBD_REQUEST usb)
{
  if (GetDescriptor(req,ed,usb,DESCRIPTOR_DEVICE,0,sizeof(USB_DEVICE_DESCRIPTOR),0))
  {
    while(!(usb->USB_Reserved=(ULONG)AllocMem(sizeof(USB_DEVICE_DESCRIPTOR),MEMF_PUBLIC|MEMF_CLEAR)));
    CopyMem(usb->USB_Buffer,(APTR)usb->USB_Reserved,sizeof(USB_DEVICE_DESCRIPTOR));
    ((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->bcdUSB       =SwapWord(((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->bcdUSB);
    ((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->idVendor     =SwapWord(((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->idVendor);
    ((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->idProduct    =SwapWord(((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->idProduct);
    ((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->iSerialNumber=SwapWord(((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->iSerialNumber);
    return((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved);
  }
  return(NULL);
}

//------------------------------------------------------------------------------------------------------------------

void __saveds FreeDeviceDescriptor(register __a0 PUSB_DEVICE_DESCRIPTOR dev)
{
 if(dev)FreeMem(dev,sizeof(USB_DEVICE_DESCRIPTOR));
 dev=NULL;
}

//------------------------------------------------------------------------------------------------------------------

PUSB_CONFIGURATION_DESCRIPTOR __saveds GetConfigurationDescriptor(register __a0 PUSB_SUPPORT req,
                                                         register __a1 PENDPOINT_DESCRIPTOR ed,
                                                         register __a2 PUSBD_REQUEST usb,
                                                         register __d0 UBYTE ConfValue)
{
  if(GetDescriptor(req,ed,usb,DESCRIPTOR_CONFIGURATION,ConfValue,sizeof(USB_CONFIGURATION_DESCRIPTOR),0))
   {
    while(!(usb->USB_Reserved=(ULONG)AllocMem(sizeof(USB_CONFIGURATION_DESCRIPTOR),MEMF_PUBLIC|MEMF_CLEAR)));
    CopyMem(usb->USB_Buffer,(APTR)usb->USB_Reserved,sizeof(USB_CONFIGURATION_DESCRIPTOR));
    ((PUSB_CONFIGURATION_DESCRIPTOR)usb->USB_Reserved)->wTotalLength =SwapWord(((PUSB_CONFIGURATION_DESCRIPTOR)usb->USB_Reserved)->wTotalLength);
    return((PUSB_CONFIGURATION_DESCRIPTOR)usb->USB_Reserved);
   }
  return(NULL);
}

//------------------------------------------------------------------------------------------------------------------

void __saveds FreeConfigurationDescriptor(register __a0 PUSB_CONFIGURATION_DESCRIPTOR conf)
{
 if(conf)FreeMem(conf,sizeof(USB_CONFIGURATION_DESCRIPTOR));
 conf=NULL;
}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds GetConfigurationChunk(register __a0 PUSB_SUPPORT req,
                                     register __a1 PENDPOINT_DESCRIPTOR ed,
                                     register __a2 PUSBD_REQUEST usb,
                                     register __a3 PUSB_CONFIGURATION_DESCRIPTOR conf)
{
  if( !conf ) return(0);
  if( conf->wTotalLength > usb->USB_BufferTotalLength)
  {
    return(0);
  }
  conf->bConfigurationValue--;
  GetDescriptor(req,ed,usb,DESCRIPTOR_CONFIGURATION,conf->bConfigurationValue,conf->wTotalLength,0);
  conf->bConfigurationValue++;
  if(usb->USB_Status==USBD_COMPLETE)return(1);
  return(0);
}

//------------------------------------------------------------------------------------------------------------------

ULONG __saveds GetString(register __a0 PUSB_SUPPORT req,
                         register __a1 PENDPOINT_DESCRIPTOR ed,
                         register __a2 PUSBD_REQUEST usb,
                         register __d0 UBYTE StringID,
                         register __a3 char* dest)
{
  if( !req || !ed || !usb || !dest) return(0);
  usb->USB_ShortXferOk=1;
  // Clear the buffer
  for(usb->USB_Reserved=0;usb->USB_Reserved<usb->USB_BufferTotalLength;usb->USB_Reserved++)
      usb->USB_Buffer[usb->USB_Reserved]=0;
  // Get String Descriptor
  usb->USB_Reserved=usb->USB_BufferTotalLength;
  if(usb->USB_Reserved>2+2*MAX_STRING_CHARS)usb->USB_Reserved=2+2*MAX_STRING_CHARS;
  if(GetDescriptor(req,ed,usb,DESCRIPTOR_STRING,StringID,usb->USB_Reserved,0))
  {
    usb->USB_ShortXferOk=0;
    usb->USB_Reserved=2;
    while((usb->USB_Buffer[usb->USB_Reserved]) || (usb->USB_Reserved<2*(MAX_STRING_CHARS-1)) || usb->USB_Reserved<usb->USB_BytesCopied)
    {
       dest[(usb->USB_Reserved-2)>>1]=usb->USB_Buffer[usb->USB_Reserved];
       usb->USB_Reserved+=2;
    }
    dest[(usb->USB_Reserved-2)>>1]=0;
    return(usb->USB_Reserved);
  }
  usb->USB_ShortXferOk=0;
  return(0);
}


//-------------------------------------------------------------------------------------------------------
//
//     USB STACK FUNCTIONS
//
//-------------------------------------------------------------------------------------------------------

UBYTE __saveds SendStackRequest(register __a0 PUSB_SUPPORT Support)
{
  if(!Support)return(0);
  PutMsg(StackPort,(struct Message*)&Support->UsbRequest);
  WaitPort(Support->UserPort);
  while(Support->DummyMess=GetMsg(Support->UserPort));
  if(Support->UsbRequest.IO_Error==NO_ERROR) return(1);
  return(0);
}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds LockFunction(register __a0 PUSB_SUPPORT Support,register __d0 ULONG FunctionAddress)
{
 if(!Support) return(0);
 if(FunctionAddress>=128)
 {
  Support->UsbRequest.IO_Error=ERR_BAD_ARGUMENTS;
  Support->UsbRequest.IO_Data=0;
 }
 else
 {
  Support->UsbRequest.IO_Command=CMD_LOCK_FUNCTION;
  Support->UsbRequest.IO_Data=FunctionAddress;
  SendStackRequest(Support);
 }
 return(Support->UsbRequest.IO_Data);
}

//------------------------------------------------------------------------------------------------------------------

void __saveds UnLockFunction(register __a0 PUSB_SUPPORT Support,register __d0 ULONG FunctionAddress)
{
 if(Support)
 {
   if(FunctionAddress>=128)
   {
    Support->UsbRequest.IO_Error=ERR_BAD_ARGUMENTS;
   }
   else
   {
    Support->UsbRequest.IO_Command=CMD_UNLOCK_FUNCTION;
    Support->UsbRequest.IO_Data=FunctionAddress;
    SendStackRequest(Support);
   }
  }
}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds UsbInitFunction(register __a0 PUSB_SUPPORT Support,register __d0 UBYTE DeviceSpeed)
{
 if (!Support) return(0);
 if (DeviceSpeed) DeviceSpeed=1;
 Support->UsbRequest.IO_Command=CMD_ALLOC_FUNCTION;
 Support->UsbRequest.IO_Data=0|DeviceSpeed;
 SendStackRequest(Support);
 return(Support->UsbRequest.IO_Data);
}


//------------------------------------------------------------------------------------------------------------------

void __saveds UsbRemoveFunction(register __a0 PUSB_SUPPORT Support,register __d0 UBYTE FunctionAddress)
{
 if ((!Support) || FunctionAddress>=128)
 {
    if(Support) Support->UsbRequest.IO_Error=ERR_BAD_ARGUMENTS;
 }
 else
 {
   Support->UsbRequest.IO_Command=CMD_FREE_FUNCTION;
   Support->UsbRequest.IO_Data=0|FunctionAddress;
   SendStackRequest(Support);
 }
}

//------------------------------------------------------------------------------------------------------------------

PUSB_DEVICE __saveds GetUsbDevice(register __a0 PUSB_SUPPORT Support,register __d0 UBYTE FunctionAddress)
{
  if(! Support) return(NULL);
  if(FunctionAddress>=128) return(NULL);
  Support->UsbRequest.IO_Command=CMD_GET_USB_DEVICE;
  Support->UsbRequest.IO_Data=0|FunctionAddress;
  SendStackRequest(Support);
  return((PUSB_DEVICE)Support->UsbRequest.IO_Data);
}