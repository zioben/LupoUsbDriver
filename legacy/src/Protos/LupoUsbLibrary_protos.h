//------------------------------------------------------------------------
//
// Lupo Usb Library 2003 by Zioben (zioben@inwind.it painaga@dei.unipd.it)
//
// Configurazione dei comandi
//
//
//------------------------------------------------------------------------
#ifndef  LUPOUSB_PROTOS
#define  LUPOUSB_PROTOS

#include <usb/HcDriverStructs.h>
#include <usb/DefaultControlEndpoint.h>
#include <exec/exec.h>
#include <exec/types.h>

UWORD ConvertWord(  UWORD);
ULONG ConvertLong(  ULONG);
UWORD SwapWord(  UWORD);
ULONG SwapLong(  ULONG);
UBYTE SendUsbRequest(  PUSB_SUPPORT,  struct MsgPort*);

ULONG  GetUsbError       (  PUSB_SUPPORT Req);
UBYTE  SetupRootHubMessagePort(  PUSB_SUPPORT Req,
				 	  struct MsgPort *Port);
UBYTE  FreeRootHubMessagePort(  PUSB_SUPPORT Req,
				 	  struct MsgPort *Port);

ULONG  Get32BitFrameNumber	   (  PUSB_SUPPORT req);
PHCD_DEVICE_DATA  GetUsbBaseAddress(  PUSB_SUPPORT Req);
void   KillHost		(  PUSB_SUPPORT Req);
void   StartHost	(  PUSB_SUPPORT Req);
void   StopHost		(  PUSB_SUPPORT Req);
void   PauseHost	(  PUSB_SUPPORT Req);
UBYTE  SetUsbState	(  PUSB_SUPPORT Req,  ULONG usbstate);
ULONG  GetUsbState	(  PUSB_SUPPORT Req);

char*   AllocUsbMem(  PUSB_SUPPORT Req,  ULONG BufferLeng);
void   FreeUsbMem  (  PUSB_SUPPORT Req,  char *Mem);

PUSB_SUPPORT  AllocUsbSupport(void);
UBYTE  InitUsbSupport(  PUSB_SUPPORT Req);
void   FreeUsbSupport(  PUSB_SUPPORT Req);

PENDPOINT_DESCRIPTOR  AllocEndpoint(  PUSB_SUPPORT req,  ULONG type,
 				       ULONG rate,  PENDPOINT_CONTROL cont);
void   FreeEndpoint         (  PUSB_SUPPORT req,  PENDPOINT_DESCRIPTOR Endpoint);
void   GetEndpointData      (  PUSB_SUPPORT req,  PENDPOINT_DESCRIPTOR Endpoint);
void   SetEndpointData      (  PUSB_SUPPORT req,  PENDPOINT_DESCRIPTOR Endpoint);
UBYTE   OpenPipe            (  PUSB_SUPPORT req,  PENDPOINT_DESCRIPTOR ed);
void   ClosePipe            (  PUSB_SUPPORT req,  PENDPOINT_DESCRIPTOR ed);

PUSBD_REQUEST  AllocUsbdRequest(  PUSB_SUPPORT req,  ULONG BufferLeng);
void   FreeUsbdRequest         (  PUSB_SUPPORT req,  PUSBD_REQUEST usb);
void   AbortUsbdRequest        (  PUSB_SUPPORT req,  PUSBD_REQUEST usb);
void   FreeUsbdRequestBuffer   (  PUSB_SUPPORT req,  PUSBD_REQUEST usb);
ULONG  AllocUsbdRequestBuffer  (  PUSB_SUPPORT req,  PUSBD_REQUEST usb,
										   ULONG BufferLength);
void   SetupUsbdRequest	  (  PUSBD_REQUEST usb,   UBYTE bRequestType,
                                     UBYTE bRequest,  UWORD wValue,						 
                                     UWORD wIndex,  UWORD wLength);
UBYTE  SendUsbdRequest 	  (  PUSB_SUPPORT req,  PENDPOINT_DESCRIPTOR ed,
                                      PUSBD_REQUEST usb);
                
ULONG    WaitUsbdRequest(   PUSB_SUPPORT support,  PUSBD_REQUEST usb);

                                   
UBYTE  SendSyncUsbdRequest(  PUSB_SUPPORT Req,
				     PENDPOINT_DESCRIPTOR ed,
				     PUSBD_REQUEST usb);
UBYTE  SendUsbData        (  PUSB_SUPPORT Req,
                                     PENDPOINT_DESCRIPTOR ed,
                                     PUSBD_REQUEST usb);
UBYTE  ReceiveUsbData     (  PUSB_SUPPORT Req,
                                     PENDPOINT_DESCRIPTOR ed,
                                     PUSBD_REQUEST usb);

UBYTE  ClearFeature(  PUSB_SUPPORT req,
                              PENDPOINT_DESCRIPTOR ed,
                              PUSBD_REQUEST usb,
                              ULONG recipient,
                              UWORD Feature,
                              UBYTE num);

UBYTE  GetConfiguration(  PUSB_SUPPORT req,
                                  PENDPOINT_DESCRIPTOR ed,
                                  PUSBD_REQUEST usb);

UBYTE  GetDescriptor(  PUSB_SUPPORT req,
                               PENDPOINT_DESCRIPTOR ed,
                               PUSBD_REQUEST usb,
                               UBYTE type,
                               UBYTE index,
                               UWORD Leng,
                               UWORD LangID);

UBYTE  GetInterface(  PUSB_SUPPORT req,
                              PENDPOINT_DESCRIPTOR ed,
                              PUSBD_REQUEST usb,
                              UBYTE NInterface);

UWORD  GetStatus(  PUSB_SUPPORT req,
                           PENDPOINT_DESCRIPTOR ed,
                           PUSBD_REQUEST usb,
                           ULONG Recipient,
                           UBYTE NumInterface);

UBYTE  SetAddress(  PUSB_SUPPORT req,
                            PENDPOINT_DESCRIPTOR ed,
                            PUSBD_REQUEST usb,
                            UBYTE Address);

UBYTE  SetConfiguration(  PUSB_SUPPORT req,
                                  PENDPOINT_DESCRIPTOR ed,
                                  PUSBD_REQUEST usb,
                                  UBYTE Value);

UBYTE  SetDescriptor(  PUSB_SUPPORT req,
                               PENDPOINT_DESCRIPTOR ed,
                               PUSBD_REQUEST usb,
                               UBYTE Type,
                               UBYTE Index,
                               UWORD Leng,
                               UWORD LangID);

UBYTE  SetFeature(  PUSB_SUPPORT req,
                            PENDPOINT_DESCRIPTOR ed,
                            PUSBD_REQUEST usb,
                            ULONG Recipient,
                            UBYTE Interface,
                            UWORD Feature);


UBYTE  SetInterface(  PUSB_SUPPORT req,
                              PENDPOINT_DESCRIPTOR ed,
                              PUSBD_REQUEST usb,
                              UBYTE Interface,
                              UBYTE AltInterface);

UBYTE  SynchFrame(  PUSB_SUPPORT req,
                            PENDPOINT_DESCRIPTOR ed,
                            PUSBD_REQUEST usb,
                            UWORD Frame);

UBYTE  ClearDeviceFeature(  PUSB_SUPPORT req,
                                    PENDPOINT_DESCRIPTOR ed,
                                    PUSBD_REQUEST usb,
                                    UWORD Feature);

UBYTE  ClearEndpointFeature(  PUSB_SUPPORT req,
                                      PENDPOINT_DESCRIPTOR ed,
                                      PUSBD_REQUEST usb,
                                      UWORD Feature);

UBYTE  ClearInterfaceFeature(  PUSB_SUPPORT req,
                                       PENDPOINT_DESCRIPTOR ed,
                                       PUSBD_REQUEST usb,
                                       UWORD Feature,
                                       UBYTE NumInterface);

UBYTE  SetDeviceFeature(  PUSB_SUPPORT req,
                                  PENDPOINT_DESCRIPTOR ed,
                                  PUSBD_REQUEST usb,
                                  UWORD Feature);
 
UBYTE  SetEndpointFeature(  PUSB_SUPPORT req,
                                    PENDPOINT_DESCRIPTOR ed,
                                    PUSBD_REQUEST usb,
                                    UWORD Feature);

UBYTE  SetInterfaceFeature(  PUSB_SUPPORT req,
                                     PENDPOINT_DESCRIPTOR ed,
                                     PUSBD_REQUEST usb,
                                     UWORD Feature,
                                     UBYTE NumInterface);

PUSB_DEVICE_DESCRIPTOR  GetDeviceDescriptor(  PUSB_SUPPORT req,
												         PENDPOINT_DESCRIPTOR ed,
												         PUSBD_REQUEST usb);
void  FreeDeviceDescriptor(  PUSB_DEVICE_DESCRIPTOR dev);


PUSB_CONFIGURATION_DESCRIPTOR  GetConfigurationDescriptor(  PUSB_SUPPORT req,
                                                           PENDPOINT_DESCRIPTOR ed,
                                                           PUSBD_REQUEST usb,
                                                           UBYTE ConfValue);

void  FreeConfigurationDescriptor(  PUSB_CONFIGURATION_DESCRIPTOR conf);

PUSB_INTERFACE_DESCRIPTOR  GetInterfaceDescriptor(  PUSB_SUPPORT req,
                                                            PENDPOINT_DESCRIPTOR ed,
                                                            PUSBD_REQUEST usb,
                                                            UBYTE IntValue);
void  FreeInterfaceDescriptor(  PUSB_INTERFACE_DESCRIPTOR inte);
PUSB_ENDPOINT_DESCRIPTOR  GetEndpointDescriptor(  PUSB_SUPPORT req,
                                                          PENDPOINT_DESCRIPTOR ed,
                                                          PUSBD_REQUEST usb,
                                                          UBYTE EndpointNumber);
void  FreeEndpointDescriptor(  PUSB_ENDPOINT_DESCRIPTOR endp);



UBYTE  GetConfigurationChunk(  PUSB_SUPPORT req,
                                       PENDPOINT_DESCRIPTOR ed,
                                       PUSBD_REQUEST usb,
                                       PUSB_CONFIGURATION_DESCRIPTOR conf);

ULONG GetString(PUSB_SUPPORT req,
                PENDPOINT_DESCRIPTOR ed,
                PUSBD_REQUEST usb,
                UBYTE StringID,
                char* dest);




UBYTE SendStackRequest ( PUSB_SUPPORT Support);
UBYTE LockFunction     ( PUSB_SUPPORT Support, ULONG FunctionNumber);
void  UnLockFunction   ( PUSB_SUPPORT Support, ULONG FunctionAddress);
UBYTE UsbInitFunction  ( PUSB_SUPPORT Support, UBYTE DeviceSpeed);
void  UsbRemoveFunction( PUSB_SUPPORT Support, UBYTE FunctionAddress);

PUSB_DEVICE GetUsbDevice(PUSB_SUPPORT Support, UBYTE FunctionAddress);


#endif