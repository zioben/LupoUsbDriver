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

ULONG __saveds GetUsbError(PUSB_SUPPORT Req __asm("a0"))
{
  if(Req) return (Req->UsbRequest.IO_Error);
  return(NULL);
}

//------------------------------------------------------------------------
//				   Utilità gestione HostController
//------------------------------------------------------------------------

UBYTE __saveds SetupRootHubMessagePort(PUSB_SUPPORT Req __asm("a0"),
                                       struct MsgPort *Port __asm("a1"))
{
    if((!Req)||(!Port))return(0);
    Req->UsbRequest.IO_Command=CMD_SETUP_ROOT_HUB_PORT;
    Req->UsbRequest.IO_Data   =(ULONG)Port;
    return(SendUsbRequest(Req,KernelPort));
}

UBYTE __saveds FreeRootHubMessagePort(PUSB_SUPPORT Req __asm("a0"),
                                       struct MsgPort *Port __asm("a1"))
{
    if(Req && Port)
    {
      Req->UsbRequest.IO_Command=CMD_FREE_ROOT_HUB_PORT;
      Req->UsbRequest.IO_Data   =(ULONG)Port;
      return(SendUsbRequest(Req,KernelPort));
    }
    return(0);
}



//---------------------------------------------------------------------------------------------------------------------------

ULONG __saveds Get32BitFrameNumber(PUSB_SUPPORT Req __asm("a0"))
{
    if(!Req) return(0);
    Req->UsbRequest.IO_Command=CMD_GET_32BIT_FN;
    if(SendUsbRequest(Req,KernelPort))
        return(Req->UsbRequest.IO_Data);   // the frame number, not the outcome
    return(0);
}

//---------------------------------------------------------------------------------------------------------------------------

PHCD_DEVICE_DATA __saveds GetUsbBaseAddress(PUSB_SUPPORT Req __asm("a0"))
{
  if(!Req)return(NULL);
  Req->UsbRequest.IO_Command=CMD_GET_USB_BASE;
  if(SendUsbRequest(Req,KernelPort))     // SendUsbRequest returns 1 on success
      return((PHCD_DEVICE_DATA)Req->UsbRequest.IO_Data);
  return(NULL);
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   KillHost(PUSB_SUPPORT Req __asm("a0"))
{
  if(Req)
  {
    Req->UsbRequest.IO_Command=CMD_KILL_USB;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   StartHost(PUSB_SUPPORT Req __asm("a0"))
{
  if(Req)
  {
    Req->UsbRequest.IO_Command=CMD_START_HOST;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   StopHost(PUSB_SUPPORT Req __asm("a0"))
{
  if(Req)
  {
    Req->UsbRequest.IO_Command=CMD_STOP_HOST;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   PauseHost(PUSB_SUPPORT Req __asm("a0"))
{
  if(Req)
  {
    Req->UsbRequest.IO_Command=CMD_PAUSE_HOST;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------


UBYTE __saveds   SetUsbState(PUSB_SUPPORT Req __asm("a0"),
					  ULONG usbstate __asm("d0"))
{
  if(!Req)return(0);
  Req->UsbRequest.IO_Command=CMD_SET_USB_STATE;
  Req->UsbRequest.IO_Data   =usbstate;
  return(SendUsbRequest(Req,KernelPort));
}

//---------------------------------------------------------------------------------------------------------------------------

ULONG __saveds   GetUsbState(PUSB_SUPPORT Req __asm("a0"))
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


char * __saveds   AllocUsbMem(PUSB_SUPPORT Req __asm("a0"),
						ULONG BufferLeng __asm("d0"))
{
  if((!BufferLeng) || (!Req)) return(NULL);
  Req->UsbRequest.IO_Command=CMD_ALLOCMEM;
  Req->UsbRequest.IO_Data=BufferLeng;
  if(SendUsbRequest(Req,KernelPort))
  return((char*)Req->UsbRequest.IO_Data);
  return(NULL);
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   FreeUsbMem(PUSB_SUPPORT Req __asm("a0"),
					 char *Mem __asm("a1"))
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

UBYTE __saveds   InitUsbSupport(PUSB_SUPPORT Req __asm("a0"))
{
 if(!Req) return(0);
 if (!(Req->UserPort=CreatePort(NULL,0))) return(0);
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


void __saveds   FreeUsbSupport(PUSB_SUPPORT Req __asm("a0"))
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

PENDPOINT_DESCRIPTOR __saveds   AllocEndpoint(PUSB_SUPPORT Req __asm("a0"),
									 ULONG type __asm("d0"),
									 ULONG rate __asm("d1"),
									 PENDPOINT_CONTROL cont __asm("a1"))
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

void __saveds   FreeEndpoint(PUSB_SUPPORT Req __asm("a0"),
									  PENDPOINT_DESCRIPTOR Endpoint __asm("a1"))
{
  if (Req && Endpoint)
  {
    Req->UsbRequest.IO_Command=CMD_FREE_ENDPOINT;
    Req->UsbRequest.IO_Data=(ULONG)Endpoint;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   GetEndpointData(PUSB_SUPPORT Req __asm("a0"),
										  PENDPOINT_DESCRIPTOR Endpoint __asm("a1"))
{
 if (Req && Endpoint)
  {
    Req->UsbRequest.IO_Command=CMD_GET_ENDPOINT_DATA;
    Req->UsbRequest.IO_Data=(ULONG)Endpoint;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------

void __saveds   SetEndpointData(PUSB_SUPPORT Req __asm("a0"),
							PENDPOINT_DESCRIPTOR Endpoint __asm("a1"))
{
 if (Req && Endpoint)
  {
    Req->UsbRequest.IO_Command=CMD_SET_ENDPOINT_DATA;
    Req->UsbRequest.IO_Data=(ULONG)Endpoint;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------

UBYTE __saveds   OpenPipe(PUSB_SUPPORT Req __asm("a0"),
				  PENDPOINT_DESCRIPTOR ed __asm("a1"))
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

void __saveds   ClosePipe(PUSB_SUPPORT Req __asm("a0"),
					 PENDPOINT_DESCRIPTOR ed __asm("a1"))
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

PUSBD_REQUEST __saveds   AllocUsbdRequest(PUSB_SUPPORT Req __asm("a0"),
										 ULONG BufferLeng __asm("d0"))
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

void __saveds   FreeUsbdRequest(PUSB_SUPPORT Req __asm("a0"),
							PUSBD_REQUEST usb __asm("a1"))
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


void __saveds   AbortUsbdRequest(PUSB_SUPPORT Req __asm("a0"),
							 PUSBD_REQUEST usb __asm("a1"))
{
  if (Req && usb)
  {
    Req->UsbRequest.IO_Command=CMD_ABORT_USBD_REQUEST;
    Req->UsbRequest.IO_Data=(ULONG)usb;
    SendUsbRequest(Req,KernelPort);
  }
}

//---------------------------------------------------------------------------------------------------------------------------


void __saveds   FreeUsbdRequestBuffer(PUSB_SUPPORT Req __asm("a0"),
									PUSBD_REQUEST usb __asm("a1"))
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

ULONG __saveds   AllocUsbdRequestBuffer(PUSB_SUPPORT Req __asm("a0"),
										PUSBD_REQUEST usb __asm("a1"),
										ULONG BufferLeng __asm("d0"))
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

void __saveds   SetupUsbdRequest( PUSBD_REQUEST usb __asm("a0"),
											 UBYTE a __asm("d0"),
											 UBYTE b __asm("d1"),
											 UWORD c __asm("d2"),
											 UWORD d __asm("d3"),
											 UWORD e __asm("d4"))
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

UBYTE __saveds   SendUsbdRequest( PUSB_SUPPORT Req __asm("a0"),
						 					 PENDPOINT_DESCRIPTOR ed __asm("a1"),
											 PUSBD_REQUEST usb __asm("a2"))
{
  if((!Req) || (!ed) || (!usb)) return(0);
  Req->UsbRequest.IO_Command=CMD_SEND_USBD_REQUEST;
  Req->UsbRequest.IO_Data=(ULONG)usb;
  Req->UsbRequest.IO_Buffer[0]=(ULONG)ed;
  return(SendUsbRequest(Req,KernelPort));
}

//---------------------------------------------------------------------------------------------------------------------------

ULONG __saveds   WaitUsbdRequest( PUSB_SUPPORT support __asm("a0"),PUSBD_REQUEST usb __asm("a1"))
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

UBYTE __saveds  SendSyncUsbdRequest( PUSB_SUPPORT Req __asm("a0"),
						 					    PENDPOINT_DESCRIPTOR ed __asm("a1"),
											    PUSBD_REQUEST usb __asm("a2"))
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


UBYTE	__saveds SendUsbData( PUSB_SUPPORT Req __asm("a0"),
						 		    PENDPOINT_DESCRIPTOR ed __asm("a1"),
								    PUSBD_REQUEST usb __asm("a2"))
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


UBYTE __saveds ReceiveUsbData( PUSB_SUPPORT Req __asm("a0"),
						 				 PENDPOINT_DESCRIPTOR ed __asm("a1"),
									    PUSBD_REQUEST usb __asm("a2"))
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

UBYTE __saveds ClearFeature(PUSB_SUPPORT req __asm("a0"),
						 PENDPOINT_DESCRIPTOR ed __asm("a1"),
						 PUSBD_REQUEST usb __asm("a2"),
						 ULONG recipient __asm("d0"),
						 UWORD Feature __asm("d1"),
						 UBYTE num __asm("d2"))
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

UBYTE __saveds GetConfiguration(PUSB_SUPPORT req __asm("a0"),
							  PENDPOINT_DESCRIPTOR ed __asm("a1"),
							  PUSBD_REQUEST usb __asm("a2"))
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

UBYTE __saveds GetDescriptor(PUSB_SUPPORT req __asm("a0"),
						  PENDPOINT_DESCRIPTOR ed __asm("a1"),
						  PUSBD_REQUEST usb __asm("a2"),
						  UBYTE type __asm("d0"),
						  UBYTE index __asm("d1"),
						  UWORD Leng __asm("d2"),
						  UWORD LangID __asm("d3"))
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

UBYTE __saveds GetInterface(PUSB_SUPPORT req __asm("a0"),
						 PENDPOINT_DESCRIPTOR ed __asm("a1"),
						 PUSBD_REQUEST usb __asm("a2"),
						 UBYTE NInterface __asm("d0"))
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


UWORD __saveds GetStatus(PUSB_SUPPORT req __asm("a0"),
					 PENDPOINT_DESCRIPTOR ed __asm("a1"),
					 PUSBD_REQUEST usb __asm("a2"),
					 ULONG Recipient __asm("d0"),
					 UBYTE NumInterface __asm("d1"))
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

UBYTE __saveds SetAddress(PUSB_SUPPORT req __asm("a0"),
					  PENDPOINT_DESCRIPTOR ed __asm("a1"),
					  PUSBD_REQUEST usb __asm("a2"),
					  UBYTE Address __asm("d0"))
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

UBYTE __saveds SetConfiguration(PUSB_SUPPORT req __asm("a0"),
							  PENDPOINT_DESCRIPTOR ed __asm("a1"),
							  PUSBD_REQUEST usb __asm("a2"),
							  UBYTE Value __asm("d0"))
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

UBYTE __saveds SetDescriptor(PUSB_SUPPORT req __asm("a0"),
						  PENDPOINT_DESCRIPTOR ed __asm("a1"),
						  PUSBD_REQUEST usb __asm("a2"),
						  UBYTE Type __asm("d0"),
						  UBYTE Index __asm("d1"),
						  UWORD Leng __asm("d2"),
						  UWORD LangID __asm("d3"))
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

UBYTE __saveds SetFeature(PUSB_SUPPORT req __asm("a0"),
					  PENDPOINT_DESCRIPTOR ed __asm("a1"),
					  PUSBD_REQUEST usb __asm("a2"),
					  ULONG Recipient __asm("d0"),
					  UBYTE Interface __asm("d1"),
					  UWORD Feature __asm("d2"))
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


UBYTE __saveds SetInterface(PUSB_SUPPORT req __asm("a0"),
						 PENDPOINT_DESCRIPTOR ed __asm("a1"),
						 PUSBD_REQUEST usb __asm("a2"),
						 UBYTE Interface __asm("d0"),
						 UBYTE AltInterface __asm("d1"))
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

UBYTE __saveds SynchFrame(PUSB_SUPPORT req __asm("a0"),
					  PENDPOINT_DESCRIPTOR ed __asm("a1"),
					  PUSBD_REQUEST usb __asm("a2"),
					  UWORD Frame __asm("d0"))
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

UBYTE __saveds ClearDeviceFeature(PUSB_SUPPORT req __asm("a0"),
								 PENDPOINT_DESCRIPTOR ed __asm("a1"),
								 PUSBD_REQUEST usb __asm("a2"),
								 UWORD Feature __asm("d0"))
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

UBYTE __saveds ClearEndpointFeature(PUSB_SUPPORT req __asm("a0"),
									PENDPOINT_DESCRIPTOR ed __asm("a1"),
									PUSBD_REQUEST usb __asm("a2"),
									UWORD Feature __asm("d0"))
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

UBYTE __saveds ClearInterfaceFeature(PUSB_SUPPORT req __asm("a0"),
									 PENDPOINT_DESCRIPTOR ed __asm("a1"),
									 PUSBD_REQUEST usb __asm("a2"),
									 UWORD Feature __asm("d0"),
									 UBYTE NumInterface __asm("d1"))
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

UBYTE __saveds SetDeviceFeature(PUSB_SUPPORT req __asm("a0"),
							  PENDPOINT_DESCRIPTOR ed __asm("a1"),
							  PUSBD_REQUEST usb __asm("a2"),
							  UWORD Feature __asm("d0"))
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

UBYTE __saveds SetEndpointFeature(PUSB_SUPPORT req __asm("a0"),
                                  PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                  PUSBD_REQUEST usb __asm("a2"),
                                  UWORD Feature __asm("d0"))
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

UBYTE __saveds SetInterfaceFeature(PUSB_SUPPORT req __asm("a0"),
								  PENDPOINT_DESCRIPTOR ed __asm("a1"),
								  PUSBD_REQUEST usb __asm("a2"),
								  UWORD Feature __asm("d0"),
								  UBYTE NumInterface __asm("d1"))
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

PUSB_DEVICE_DESCRIPTOR __saveds GetDeviceDescriptor(PUSB_SUPPORT req __asm("a0"),
												       PENDPOINT_DESCRIPTOR ed __asm("a1"),
												       PUSBD_REQUEST usb __asm("a2"))
{
  if (GetDescriptor(req,ed,usb,DESCRIPTOR_DEVICE,0,sizeof(USB_DEVICE_DESCRIPTOR),0))
  {
    if(!(usb->USB_Reserved=(ULONG)AllocMem(sizeof(USB_DEVICE_DESCRIPTOR),MEMF_PUBLIC|MEMF_CLEAR))) return(NULL);
    CopyMem(usb->USB_Buffer,(APTR)usb->USB_Reserved,sizeof(USB_DEVICE_DESCRIPTOR));
    ((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->bcdUSB       =SwapWord(((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->bcdUSB);
    ((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->idVendor     =SwapWord(((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->idVendor);
    ((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->idProduct    =SwapWord(((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->idProduct);
    ((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->bcdDevice    =SwapWord(((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved)->bcdDevice);
    return((PUSB_DEVICE_DESCRIPTOR)usb->USB_Reserved);
  }
  return(NULL);
}

//------------------------------------------------------------------------------------------------------------------

void __saveds FreeDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR dev __asm("a0"))
{
 if(dev)FreeMem(dev,sizeof(USB_DEVICE_DESCRIPTOR));
 dev=NULL;
}

//------------------------------------------------------------------------------------------------------------------

PUSB_CONFIGURATION_DESCRIPTOR __saveds GetConfigurationDescriptor(PUSB_SUPPORT req __asm("a0"),
                                                         PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                                         PUSBD_REQUEST usb __asm("a2"),
                                                         UBYTE ConfValue __asm("d0"))
{
  if(GetDescriptor(req,ed,usb,DESCRIPTOR_CONFIGURATION,ConfValue,sizeof(USB_CONFIGURATION_DESCRIPTOR),0))
   {
    if(!(usb->USB_Reserved=(ULONG)AllocMem(sizeof(USB_CONFIGURATION_DESCRIPTOR),MEMF_PUBLIC|MEMF_CLEAR))) return(NULL);
    CopyMem(usb->USB_Buffer,(APTR)usb->USB_Reserved,sizeof(USB_CONFIGURATION_DESCRIPTOR));
    ((PUSB_CONFIGURATION_DESCRIPTOR)usb->USB_Reserved)->wTotalLength =SwapWord(((PUSB_CONFIGURATION_DESCRIPTOR)usb->USB_Reserved)->wTotalLength);
    return((PUSB_CONFIGURATION_DESCRIPTOR)usb->USB_Reserved);
   }
  return(NULL);
}

//------------------------------------------------------------------------------------------------------------------

void __saveds FreeConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR conf __asm("a0"))
{
 if(conf)FreeMem(conf,sizeof(USB_CONFIGURATION_DESCRIPTOR));
 conf=NULL;
}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds GetConfigurationChunk(PUSB_SUPPORT req __asm("a0"),
                                     PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                     PUSBD_REQUEST usb __asm("a2"),
                                     PUSB_CONFIGURATION_DESCRIPTOR conf __asm("a3"))
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

ULONG __saveds GetString(PUSB_SUPPORT req __asm("a0"),
                         PENDPOINT_DESCRIPTOR ed __asm("a1"),
                         PUSBD_REQUEST usb __asm("a2"),
                         UBYTE StringID __asm("d0"),
                         char* dest __asm("a3"))
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

UBYTE __saveds SendStackRequest(PUSB_SUPPORT Support __asm("a0"))
{
  if(!Support)return(0);
  PutMsg(StackPort,(struct Message*)&Support->UsbRequest);
  WaitPort(Support->UserPort);
  while(Support->DummyMess=GetMsg(Support->UserPort));
  if(Support->UsbRequest.IO_Error==NO_ERROR) return(1);
  return(0);
}

//------------------------------------------------------------------------------------------------------------------

UBYTE __saveds LockFunction(PUSB_SUPPORT Support __asm("a0"),ULONG FunctionAddress __asm("d0"))
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

void __saveds UnLockFunction(PUSB_SUPPORT Support __asm("a0"),ULONG FunctionAddress __asm("d0"))
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

UBYTE __saveds UsbInitFunction(PUSB_SUPPORT Support __asm("a0"),UBYTE DeviceSpeed __asm("d0"))
{
 if (!Support) return(0);
 if (DeviceSpeed) DeviceSpeed=1;
 Support->UsbRequest.IO_Command=CMD_ALLOC_FUNCTION;
 Support->UsbRequest.IO_Data=0|DeviceSpeed;
 SendStackRequest(Support);
 return(Support->UsbRequest.IO_Data);
}


//------------------------------------------------------------------------------------------------------------------

void __saveds UsbRemoveFunction(PUSB_SUPPORT Support __asm("a0"),UBYTE FunctionAddress __asm("d0"))
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

PUSB_DEVICE __saveds GetUsbDevice(PUSB_SUPPORT Support __asm("a0"),UBYTE FunctionAddress __asm("d0"))
{
  if(! Support) return(NULL);
  if(FunctionAddress>=128) return(NULL);
  Support->UsbRequest.IO_Command=CMD_GET_USB_DEVICE;
  Support->UsbRequest.IO_Data=0|FunctionAddress;
  SendStackRequest(Support);
  return((PUSB_DEVICE)Support->UsbRequest.IO_Data);
}