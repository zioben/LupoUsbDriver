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

UWORD ConvertWord(register __d0 UWORD);
ULONG ConvertLong(register __d0 ULONG);
UWORD SwapWord(register __d0 UWORD);
ULONG SwapLong(register __d0 ULONG);
UBYTE SendUsbRequest(register __a0 PUSB_SUPPORT,register __a1 struct MsgPort*);

ULONG __saveds GetUsbError       (register __a0 PUSB_SUPPORT Req);
UBYTE __saveds SetupRootHubMessagePort(register __a0 PUSB_SUPPORT Req,
				 	register __a1 struct MsgPort *Port);
UBYTE __saveds FreeRootHubMessagePort(register __a0 PUSB_SUPPORT Req,
				 	register __a1 struct MsgPort *Port);

ULONG __saveds Get32BitFrameNumber	   (register __a0 PUSB_SUPPORT req);
PHCD_DEVICE_DATA __saveds GetUsbBaseAddress(register __a0 PUSB_SUPPORT Req);
void  __saveds KillHost		(register __a0 PUSB_SUPPORT Req);
void  __saveds StartHost	(register __a0 PUSB_SUPPORT Req);
void  __saveds StopHost		(register __a0 PUSB_SUPPORT Req);
void  __saveds PauseHost	(register __a0 PUSB_SUPPORT Req);
UBYTE __saveds SetUsbState	(register __a0 PUSB_SUPPORT Req,register __d0 ULONG usbstate);
ULONG __saveds GetUsbState	(register __a0 PUSB_SUPPORT Req);

char*  __saveds AllocUsbMem(register __a0 PUSB_SUPPORT Req,register __d0 ULONG BufferLeng);
void  __saveds FreeUsbMem  (register __a0 PUSB_SUPPORT Req,register __a1 char *Mem);

PUSB_SUPPORT __saveds AllocUsbSupport(void);
UBYTE __saveds InitUsbSupport(register __a0 PUSB_SUPPORT Req);
void  __saveds FreeUsbSupport(register __a0 PUSB_SUPPORT Req);

PENDPOINT_DESCRIPTOR __saveds AllocEndpoint(register __a0 PUSB_SUPPORT req,register __d0 ULONG type,
 				                                register __d1 ULONG rate,register __a1 PENDPOINT_CONTROL cont);
void  __saveds FreeEndpoint         (register __a0 PUSB_SUPPORT req,register __a1 PENDPOINT_DESCRIPTOR Endpoint);
void  __saveds GetEndpointData      (register __a0 PUSB_SUPPORT req,register __a1 PENDPOINT_DESCRIPTOR Endpoint);
void  __saveds SetEndpointData      (register __a0 PUSB_SUPPORT req,register __a1 PENDPOINT_DESCRIPTOR Endpoint);
UBYTE __saveds  OpenPipe            (register __a0 PUSB_SUPPORT req,register __a1 PENDPOINT_DESCRIPTOR ed);
void  __saveds ClosePipe            (register __a0 PUSB_SUPPORT req,register __a1 PENDPOINT_DESCRIPTOR ed);

PUSBD_REQUEST __saveds AllocUsbdRequest(register __a0 PUSB_SUPPORT req,register __d0 ULONG BufferLeng);
void  __saveds FreeUsbdRequest         (register __a0 PUSB_SUPPORT req,register __a1 PUSBD_REQUEST usb);
void  __saveds AbortUsbdRequest        (register __a0 PUSB_SUPPORT req,register __a1 PUSBD_REQUEST usb);
void  __saveds FreeUsbdRequestBuffer   (register __a0 PUSB_SUPPORT req,register __a1 PUSBD_REQUEST usb);
ULONG __saveds AllocUsbdRequestBuffer  (register __a0 PUSB_SUPPORT req,register __a1 PUSBD_REQUEST usb,
										 register __d0 ULONG BufferLength);
void  __saveds SetupUsbdRequest	  (register __a0 PUSBD_REQUEST usb, register __d0 UBYTE bRequestType,
                                   register __d1 UBYTE bRequest,register __d2 UWORD wValue,						 
                                   register __d3 UWORD wIndex,register __d4 UWORD wLength);
UBYTE __saveds SendUsbdRequest 	  (register __a0 PUSB_SUPPORT req,register __a1 PENDPOINT_DESCRIPTOR ed,
                                    register __a2 PUSBD_REQUEST usb);  
ULONG __saveds WaitUsbdRequest( register __a0 PUSB_SUPPORT support,register __a1 PUSBD_REQUEST usb);                                   

UBYTE __saveds SendSyncUsbdRequest(register __a0 PUSB_SUPPORT Req,
				   register __a1 PENDPOINT_DESCRIPTOR ed,
				   register __a2 PUSBD_REQUEST usb);
UBYTE __saveds SendUsbData        (register __a0 PUSB_SUPPORT Req,
                                   register __a1 PENDPOINT_DESCRIPTOR ed,
                                   register __a2 PUSBD_REQUEST usb);
UBYTE __saveds ReceiveUsbData     (register __a0 PUSB_SUPPORT Req,
                                   register __a1 PENDPOINT_DESCRIPTOR ed,
                                   register __a2 PUSBD_REQUEST usb);
UBYTE __saveds ClearFeature(register __a0 PUSB_SUPPORT req,
                            register __a1 PENDPOINT_DESCRIPTOR ed,
                            register __a2 PUSBD_REQUEST usb,
                            register __d0 ULONG recipient,
                            register __d1 UWORD Feature,
                            register __d2 UBYTE num);

UBYTE __saveds GetConfiguration(register __a0 PUSB_SUPPORT req,
                                register __a1 PENDPOINT_DESCRIPTOR ed,
                                register __a2 PUSBD_REQUEST usb);
UBYTE __saveds GetDescriptor(register __a0 PUSB_SUPPORT req,
                             register __a1 PENDPOINT_DESCRIPTOR ed,
                             register __a2 PUSBD_REQUEST usb,
                             register __d0 UBYTE type,
                             register __d1 UBYTE index,
                             register __d2 UWORD Leng,
                             register __d3 UWORD LangID);
UBYTE __saveds GetInterface(register __a0 PUSB_SUPPORT req,
                            register __a1 PENDPOINT_DESCRIPTOR ed,
                            register __a2 PUSBD_REQUEST usb,
                            register __d0 UBYTE NInterface);
UWORD __saveds GetStatus(register __a0 PUSB_SUPPORT req,
                         register __a1 PENDPOINT_DESCRIPTOR ed,
                         register __a2 PUSBD_REQUEST usb,
                         register __d0 ULONG Recipient,
                         register __d1 UBYTE NumInterface);
UBYTE __saveds SetAddress(register __a0 PUSB_SUPPORT req,
                          register __a1 PENDPOINT_DESCRIPTOR ed,
                          register __a2 PUSBD_REQUEST usb,
                          register __d0 UBYTE Address);

UBYTE __saveds SetConfiguration(register __a0 PUSB_SUPPORT req,
                                register __a1 PENDPOINT_DESCRIPTOR ed,
                                register __a2 PUSBD_REQUEST usb,
                                register __d0 UBYTE Value);

UBYTE __saveds SetDescriptor(register __a0 PUSB_SUPPORT req,
                             register __a1 PENDPOINT_DESCRIPTOR ed,
                             register __a2 PUSBD_REQUEST usb,
                             register __d0 UBYTE Type,
                             register __d1 UBYTE Index,
                             register __d2 UWORD Leng,
                             register __d3 UWORD LangID);

UBYTE __saveds SetFeature(register __a0 PUSB_SUPPORT req,
                          register __a1 PENDPOINT_DESCRIPTOR ed,
                          register __a2 PUSBD_REQUEST usb,
                          register __d0 ULONG Recipient,
                          register __d1 UBYTE Interface,
                          register __d2 UWORD Feature);


UBYTE __saveds SetInterface(register __a0 PUSB_SUPPORT req,
                            register __a1 PENDPOINT_DESCRIPTOR ed,
                            register __a2 PUSBD_REQUEST usb,
                            register __d0 UBYTE Interface,
                            register __d1 UBYTE AltInterface);

UBYTE __saveds SynchFrame(register __a0 PUSB_SUPPORT req,
                          register __a1 PENDPOINT_DESCRIPTOR ed,
                          register __a2 PUSBD_REQUEST usb,
                          register __d0 UWORD Frame);

UBYTE __saveds ClearDeviceFeature(register __a0 PUSB_SUPPORT req,
                                  register __a1 PENDPOINT_DESCRIPTOR ed,
                                  register __a2 PUSBD_REQUEST usb,
                                  register __d0 UWORD Feature);

UBYTE __saveds ClearEndpointFeature(register __a0 PUSB_SUPPORT req,
                                    register __a1 PENDPOINT_DESCRIPTOR ed,
                                    register __a2 PUSBD_REQUEST usb,
                                    register __d0 UWORD Feature);

UBYTE __saveds ClearInterfaceFeature(register __a0 PUSB_SUPPORT req,
                                     register __a1 PENDPOINT_DESCRIPTOR ed,
                                     register __a2 PUSBD_REQUEST usb,
                                     register __d0 UWORD Feature,
                                     register __d1 UBYTE NumInterface);

UBYTE __saveds SetDeviceFeature(register __a0 PUSB_SUPPORT req,
                                register __a1 PENDPOINT_DESCRIPTOR ed,
                                register __a2 PUSBD_REQUEST usb,
                                register __d0 UWORD Feature);
 
UBYTE __saveds SetEndpointFeature(register __a0 PUSB_SUPPORT req,
                                  register __a1 PENDPOINT_DESCRIPTOR ed,
                                  register __a2 PUSBD_REQUEST usb,
                                  register __d0 UWORD Feature);

UBYTE __saveds SetInterfaceFeature(register __a0 PUSB_SUPPORT req,
                                   register __a1 PENDPOINT_DESCRIPTOR ed,
                                   register __a2 PUSBD_REQUEST usb,
                                   register __d0 UWORD Feature,
                                   register __d1 UBYTE NumInterface);

PUSB_DEVICE_DESCRIPTOR __saveds GetDeviceDescriptor(register __a0 PUSB_SUPPORT req,
												       register __a1 PENDPOINT_DESCRIPTOR ed,
												       register __a2 PUSBD_REQUEST usb);
void __saveds FreeDeviceDescriptor(register __a0 PUSB_DEVICE_DESCRIPTOR dev);


PUSB_CONFIGURATION_DESCRIPTOR __saveds GetConfigurationDescriptor(register __a0 PUSB_SUPPORT req,
                                                         register __a1 PENDPOINT_DESCRIPTOR ed,
                                                         register __a2 PUSBD_REQUEST usb,
                                                         register __d0 UBYTE ConfValue);

void __saveds FreeConfigurationDescriptor(register __a0 PUSB_CONFIGURATION_DESCRIPTOR conf);

PUSB_INTERFACE_DESCRIPTOR __saveds GetInterfaceDescriptor(register __a0 PUSB_SUPPORT req,
                                                          register __a1 PENDPOINT_DESCRIPTOR ed,
                                                          register __a2 PUSBD_REQUEST usb,
                                                          register __d0 UBYTE IntValue);
void __saveds FreeInterfaceDescriptor(register __a0 PUSB_INTERFACE_DESCRIPTOR inte);
PUSB_ENDPOINT_DESCRIPTOR __saveds GetEndpointDescriptor(register __a0 PUSB_SUPPORT req,
                                                        register __a1 PENDPOINT_DESCRIPTOR ed,
                                                        register __a2 PUSBD_REQUEST usb,
                                                        register __d0 UBYTE EndpointNumber);
void __saveds FreeEndpointDescriptor(register __a0 PUSB_ENDPOINT_DESCRIPTOR endp);



UBYTE __saveds GetConfigurationChunk(register __a0 PUSB_SUPPORT req,
                                     register __a1 PENDPOINT_DESCRIPTOR ed,
                                     register __a2 PUSBD_REQUEST usb,
                                     register __a3 PUSB_CONFIGURATION_DESCRIPTOR conf);


ULONG __saveds GetString(register __a0 PUSB_SUPPORT req,
                         register __a1 PENDPOINT_DESCRIPTOR ed,
                         register __a2 PUSBD_REQUEST usb,
                         register __d0 UBYTE StringID,
                         register __a3 char* dest);





UBYTE __saveds SendStackRequest (register __a0 PUSB_SUPPORT Support);
UBYTE __saveds LockFunction     (register __a0 PUSB_SUPPORT Support,register __d0 ULONG FunctionNumber);
void  __saveds UnLockFunction   (register __a0 PUSB_SUPPORT Support,register __d0 ULONG FunctionAddress);
UBYTE __saveds UsbInitFunction  (register __a0 PUSB_SUPPORT Support,register __d0 UBYTE DeviceSpeed);
void  __saveds UsbRemoveFunction(register __a0 PUSB_SUPPORT Support,register __d0 UBYTE FunctionAddress);

PUSB_DEVICE __saveds GetUsbDevice(register __a0 PUSB_SUPPORT Support,register __d0 UBYTE FunctionAddress);

#endif