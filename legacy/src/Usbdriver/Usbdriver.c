
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
volatile ULONG 			 AvvisaCambioStatoRootHub=0;   // written by the interrupt handler
ULONG              IrqSigMask=0;   // signal sent by the interrupt handler to this task


#define MAXDELAY 10

//-------------------------------------
//   Is the message still waiting in the port? (RootHubMess is a single
//   static message: putting it twice in the same list corrupts the list)
//-------------------------------------

static int MessageQueued(struct MsgPort *port, struct Message *msg)
{
  struct Node *n;
  int found=0;
  Forbid();
  for(n=port->mp_MsgList.lh_Head; n->ln_Succ; n=n->ln_Succ)
    if(n==&msg->mn_Node) { found=1; break; }
  Permit();
  return(found);
}

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
   if(!CreateTDCache())
   {
     kill();                // DMA memory window too small
     return(0);
   }
   if(HcSetup(HostController))
   {
     // card not found or wrong revision: do not go on with freed structures
     FreeTDCache();
     kill();
     return(0);
   }
   {
     // the interrupt handler wakes this task instead of being polled
     BYTE sig=AllocSignal(-1);
     if(sig!=-1) IrqSigMask=1L<<sig;
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
				if (HostController->RootHubMessagePort &&
				    !MessageQueued(HostController->RootHubMessagePort,(struct Message*)&RootHubMess))
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
         // Sleep until a request arrives or the interrupt handler has work
         // for us (done queue, root hub change). Before, the loop polled at
         // every video frame with WaitTOF(). Signals that arrived while we
         // were busy are latched, so nothing is lost.
         if(IrqSigMask)
            Wait((1L<<HostController->HcMessagePort->mp_SigBit) | IrqSigMask);
         else
            WaitTOF();     // no free signal: fall back to polling
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
									if((mymess->IO_Data=(ULONG)AllocMem(sizeof(USB_SUPPORT),MEMF_PUBLIC|MEMF_CLEAR))==NULL)
										mymess->IO_Error=ERR_USB_MEMORY_FULL;
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
										if(!MyRequest)
										{
											mymess->IO_Data=0;
											mymess->IO_Error=ERR_USB_MEMORY_FULL;
											break;
										}
										// Alloca Buffer I/O;
										if(mymess->IO_Data)
										{
												MyRequest->USB_Buffer=AllocUsbBuffer(mymess->IO_Data);
												if(!MyRequest->USB_Buffer)
												{
													FreeUsbdRequest(MyRequest);
													mymess->IO_Data=0;
													mymess->IO_Error=ERR_USB_MEMORY_FULL;
													break;
												}
										}
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
							  	      if(!QueueRequest(Endpoint,request))
							  	      {
							  	        // out of DMA memory for the TDs: report it on the
							  	        // command, the client then does not wait for the request
							  	        request->USB_Status=USBD_FAILED;
							  	        mymess->IO_Error=ERR_USB_MEMORY_FULL;
							  	      }
 								    }
									else
									 {
										request->USB_Status=USBD_PIPE_NOT_OPENED;
										mymess->IO_Error=ERR_PIPE_NOT_OPENED;   // else the client waits forever
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
        }
       FreeTDCache();
	    RemoveMediatorPciInterrupt(HostController);
       if(IrqSigMask)
       {
          ULONG m=IrqSigMask;
          BYTE s=0;
          IrqSigMask=0;
          while(!(m&1)) { m>>=1; s++; }
          FreeSignal(s);
       }
   }
   else
   {
      kill();
     return(0);
   }
 kill();
 return(1);
}
