
#include <exec/exec.h>
#include <pragma/exec_lib.h>
#include <pragma/dos_lib.h>
#include <pragma/intuition_lib.h>
#include <clib/alib_protos.h>

#include <libraries/pci.h>
#include <clib/pci_lib_protos.h>
#include <pragma/pci_lib.h>

#include "protos/Endpoint_protos.h"
#include "protos/interrupt_protos.h"
#include "protos/gfxmemory_protos.h"
#include "protos/HcDriver_protos.h"
//#include "protos"
//#include "compiler:usb/protosasm/asmhc_protos.h"

//#include "protos/debugger_protos.h"

//-------------------------------------
//   Strutture di controllo globali
//-------------------------------------

struct  PciBase*   PciBase;
struct  PciDevice* HcDevice;
struct  Task*      ThisTask=NULL; /* port: was initialized with FindTask before main() */
//ULONG 				 DONEQUEUEBIT;
//ULONG 				 ROOTHUBBIT;

PHCD_DEVICE_DATA   HostController;
ROOT_HUB_MESSAGE   RootHubMess;
ULONG 				 AvvisaCambioStatoRootHub=0;


#define MAXDELAY 10

//-------------------------------------
//   Dealloca la memoria
//-------------------------------------

void kill(void)
{
  if (HostController)
    {
      if (HostController->BaseAddress!=0)
      {
        HcStop(HostController);
        HcDeleteDeviceData(HostController);
		  GfxKillList();
      }
      else
      {
        // the card was never found: free the structures without touching
        // the hardware, otherwise the public port "Lupo Usb Port" stays
        // and the next start believes a kernel is already running
        HcKillHccaArea(HostController);
        HcKillMessagePort(HostController);
        HcKillHc(HostController);
      }
      HostController=NULL;
    }
 if (PciBase) CloseLibrary((struct Library*)PciBase);
 PciBase=NULL;
}

//---------------------------------------
//    Il MAIN
//---------------------------------------



ULONG main (void)
{
 /* port: global initializers of the StormC (C++) build */
 ThisTask=FindTask(NULL);

   if(FindPort(UsbKernelHcPortName)) return(0); // another kernel is running
   if (!(PciBase=(struct PciBase*)OpenLibrary("pci.library",0L)))
   {
     kill();
     return(0);
   }
   if (!(HostController=HcCreateDeviceData()))
   {
     kill();
     return(0);
   }
   {
     PENV_DATA d=&HostController->InitData;
     SetupGfxMemory(d->BufferMemAddress,d->BufferMemOffset,d->BufferMemSize);
   }
   CreateTDCache();
   if(HcSetup(HostController))
   {
     // card not found or wrong revision: do not go on with freed structures
     FreeTDCache();
     kill();
     return(0);
   }
   if(AddMediatorPciInterrupt(HostController))
   {
	  PHC_REQUEST   mymess=NULL;
     ULONG 		    DELAY=0;
     ULONG         esci=0;
     ULONG         err=0;

     HcStart(HostController);
     while (!esci)
     {
//         Wait(1<<HostController->HcMessagePort->mp_SigBit);
			while(AvvisaCambioStatoRootHub)
			{
				AvvisaCambioStatoRootHub=0;
				if (HostController->RootHubMessagePort)
					PutMsg(HostController->RootHubMessagePort,(struct Message*)&RootHubMess);
			}
         if(HostController->DoneQueue1Head)
			{
   		  HostController->DoneQueue1Locked=1;
			  ProcessDoneQueue(HostController->DoneQueue1Head);
			  HostController->DoneQueue1Head=NULL;
			  HostController->DoneQueue1Tail=NULL;
			  HostController->DoneQueue1Locked=0;
			  DELAY=0;
  			}
         if(HostController->DoneQueue2Head)
			{
   		  HostController->DoneQueue2Locked=1;
			  ProcessDoneQueue(HostController->DoneQueue2Head);
			  HostController->DoneQueue2Head=NULL;
			  HostController->DoneQueue2Tail=NULL;
			  HostController->DoneQueue2Locked=0;
			  DELAY=0;
  			}
         WaitTOF();
		 	while((mymess=(PHC_REQUEST)GetMsg(HostController->HcMessagePort)))
			{
					mymess->IO_Error=NO_ERROR;
					switch (mymess->IO_Command)
					{

						//======================================================================
						//----------------------------------------------------------------------
						//
						//		Comandi relativi all'Hardware
						//
						//----------------------------------------------------------------------
						//======================================================================

						//----------------------------------------------------------------------
						case	CMD_KILL_USB:
							{
                        ROOT_HUB_MESSAGE Mess={0,0,0,0,0,0,0,0,0};
                        Mess.Status_Change=-1;
								if(HostController->RootHubMessagePort)
                        {
                          PutMsg(HostController->RootHubMessagePort,(struct Message *)&Mess);
  								  Delay(50);
                        }
								esci=1;
 							}
						break;
						//----------------------------------------------------------------------
						case	CMD_GET_USB_BASE:
								mymess->IO_Data=(ULONG)HostController;
						break;
						//----------------------------------------------------------------------
						case  CMD_SET_USB_STATE:
								HcSetState(HostController,mymess->IO_Data);
						break;
						//----------------------------------------------------------------------
						case  CMD_GET_USB_STATE:
								mymess->IO_Data=HcGetState(HostController);
						break;

						//----------------------------------------------------------------------
						case  CMD_START_HOST:
								HcStart(HostController);
						break;
						//----------------------------------------------------------------------
						case  CMD_STOP_HOST:
								HcStop(HostController);
						break;
						//----------------------------------------------------------------------
						case  CMD_SETUP_ROOT_HUB_PORT:
                        {
                         struct MsgPort *MyPort=(struct MsgPort*)mymess->IO_Data;
                         if(MyPort)
                         {
 									if(SetupRootHubMessagePort(HostController,MyPort)==0)
 										mymess->IO_Error = ERR_ROOT_HUB_LOCKED;
                         }
                         else mymess->IO_Error = ERR_BAD_ARGUMENTS;
                        }
 						break;
						//----------------------------------------------------------------------
						case  CMD_FREE_ROOT_HUB_PORT:
                        {
                         struct MsgPort *MyPort=(struct MsgPort*)mymess->IO_Data;
                         if(MyPort)
                         {
 									if(FreeRootHubMessagePort(HostController,MyPort)==0)
 										mymess->IO_Error = ERR_ROOT_HUB_LOCKED;
                         }
                         else mymess->IO_Error = ERR_BAD_ARGUMENTS;

                        }
						break;
						//======================================================================
						//----------------------------------------------------------------------
						//
						//		Comandi relativi alla gestione della memoria
						//
						//----------------------------------------------------------------------
						//======================================================================

						case CMD_ALLOC_USB_SUPPORT:
									while((mymess->IO_Data=(ULONG)AllocMem(sizeof(USB_SUPPORT),MEMF_PUBLIC|MEMF_CLEAR))==NULL);
						break;
						//----------------------------------------------------------------------
						case CMD_FREE_USB_SUPPORT:
									if(mymess->IO_Data) FreeMem((char*)mymess->IO_Data,sizeof(USB_SUPPORT));
						break;
						//----------------------------------------------------------------------
						case	CMD_ALLOCMEM:
								if(mymess->IO_Data) mymess->IO_Data=(ULONG)AllocUsbBuffer(mymess->IO_Data);
								else
								{
									mymess->IO_Data=0;
									mymess->IO_Error=ERR_BAD_ARGUMENTS;
								}
						break;
						//----------------------------------------------------------------------
						case  CMD_FREEMEM:
								if(mymess->IO_Data) FreeUsbBuffer((char*)mymess->IO_Data);
								else	mymess->IO_Error=ERR_BAD_ARGUMENTS;
						break;

						//======================================================================
						//----------------------------------------------------------------------
						//
						//		Comandi relativi alla gestione degli endpoint
						//
						//----------------------------------------------------------------------
						//======================================================================

						//----------------------------------------------------------------------
						case  CMD_ALLOC_ENDPOINT:
								if(mymess->IO_Buffer[0] && mymess->IO_Buffer[2])
								mymess->IO_Data=(ULONG)CreateEndpoint(	mymess->IO_Buffer[0],
																					mymess->IO_Buffer[1],
																					(PENDPOINT_CONTROL)mymess->IO_Buffer[2],
																					HostController);
								else mymess->IO_Error=ERR_BAD_ARGUMENTS;
						break;
						//----------------------------------------------------------------------
						case  CMD_FREE_ENDPOINT:
								if (mymess->IO_Data)DeleteEndpoint((PENDPOINT_DESCRIPTOR)mymess->IO_Data);
								else mymess->IO_Error=ERR_BAD_ARGUMENTS;
						break;
						//----------------------------------------------------------------------
						case  CMD_SET_ENDPOINT_DATA:
								if (mymess->IO_Data)SetEndpointData((PENDPOINT_DESCRIPTOR)mymess->IO_Data);
								else mymess->IO_Error=ERR_BAD_ARGUMENTS;
						break;
						//----------------------------------------------------------------------
						case  CMD_GET_ENDPOINT_DATA:
								if (mymess->IO_Data)GetEndpointData((PENDPOINT_DESCRIPTOR)mymess->IO_Data);
								else mymess->IO_Error=ERR_BAD_ARGUMENTS;
						break;
						//----------------------------------------------------------------------

						//======================================================================
						//----------------------------------------------------------------------
						//
						//		Comandi relativi alla gestione delle richieste USB
						//
						//----------------------------------------------------------------------
						//======================================================================

						//----------------------------------------------------------------------
						case	CMD_ALLOC_USBD_REQUEST:
									{
										// Alloca USBD Request;
										PUSBD_REQUEST MyRequest=AllocUsbdRequest();
										// Alloca Buffer I/O;
										if(mymess->IO_Data)
												MyRequest->USB_Buffer=AllocUsbBuffer(mymess->IO_Data);
										MyRequest->USB_BufferTotalLength=mymess->IO_Data;
										// Init Porta Messaggi;
										//MyRequest->usb_Message.mn_ReplyPort=(struct MsgPort *)mymess->IO_Buffer[0];
										mymess->IO_Data=(ULONG)MyRequest;
									}

						break;
						//----------------------------------------------------------------------
						case  CMD_FREE_USBD_REQUEST:
									{
										PUSBD_REQUEST pippo=(PUSBD_REQUEST)mymess->IO_Data;
										if(pippo)
										{
											if(pippo->USB_Buffer) FreeUsbBuffer(pippo->USB_Buffer);
											FreeUsbdRequest(pippo);
										}
										else mymess->IO_Error=ERR_BAD_ARGUMENTS;
									}

						break;
						//----------------------------------------------------------------------
						case	CMD_ABORT_USBD_REQUEST:
								if(mymess->IO_Data) CancelRequest((PUSBD_REQUEST)mymess->IO_Data);
								else mymess->IO_Error=ERR_BAD_ARGUMENTS;
						break;

						//----------------------------------------------------------------------
						case  CMD_OPEN_PIPE:
								if(mymess->IO_Data)
									{
 									 OpenPipe((PENDPOINT_DESCRIPTOR)mymess->IO_Data);
									}
								else mymess->IO_Error=ERR_BAD_ARGUMENTS;
						break;
						//----------------------------------------------------------------------
						case  CMD_CLOSE_PIPE:
								if(mymess->IO_Data)
								{
								  ClosePipe((PENDPOINT_DESCRIPTOR)mymess->IO_Data);
								}
								else mymess->IO_Error=ERR_BAD_ARGUMENTS;
						break;

						//----------------------------------------------------------------------
						case  CMD_SEND_USBD_REQUEST:
								{
								 PENDPOINT_DESCRIPTOR Endpoint=(PENDPOINT_DESCRIPTOR) mymess->IO_Buffer[0];
								 PUSBD_REQUEST request =(PUSBD_REQUEST) mymess->IO_Data;
								if(request && Endpoint)
								{
								  if (HcGetState(HostController)!=HC_USB_OPERATIONAL)
								  {
  									 request->USB_Status=USBD_USB_NOT_OPEARATIONAL;
									 mymess->IO_Error=ERR_USB_NOT_OPERATIONAL;
								  }
 								  else
                          	if (Endpoint->ED_Reserved[0])
 								    {
							  	      QueueRequest(Endpoint,request);
 								    }
									else
									 {
										request->USB_Status=USBD_PIPE_NOT_OPENED;
									 }
								}
								else mymess->IO_Error=ERR_BAD_ARGUMENTS;
                        }
						break;

						//======================================================================
						//----------------------------------------------------------------------
						//
						//		DEBUGGER CRITICISSIMO!!!!
						//
						//----------------------------------------------------------------------
						//======================================================================

//						case	CMD_PROCESS_DONEQUEUE:
//									ProcessDoneQueue(HostController->HCCA->HccaDoneHead&0xF0FFFFFF);
//									HostController->HCCA->HccaDoneHead=0;
//						break;

						case	CMD_GET_32BIT_FN:
								mymess->IO_Data=Get32BitFrameNumber(HostController);
						break;

						//======================================================================
						//----------------------------------------------------------------------
						//
						//		Se siamo qui il comando non è ancora implementato!
						//
						//----------------------------------------------------------------------
						//======================================================================

						//----------------------------------------------------------------------
						default:
								mymess->IO_Error=ERR_UNKNOW_COMMAND;
						break;
					}
					if (mymess->message.mn_ReplyPort)
 					{
 						ReplyMsg((struct Message*)mymess);
 					}
				}
				while (mymess=(PHC_REQUEST)GetMsg(HostController->HcMessagePort));
        }
       FreeTDCache();
	    RemoveMediatorPciInterrupt();
   }
   else
   {
      kill();
     return(0);
   }
 kill();
 return(1);
}
