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

UWORD ConvertWord(UWORD _d0 __asm("d0"));
ULONG ConvertLong(ULONG _d0 __asm("d0"));
UWORD SwapWord(UWORD _d0 __asm("d0"));
ULONG SwapLong(ULONG _d0 __asm("d0"));
UBYTE SendUsbRequest(PUSB_SUPPORT _a0 __asm("a0"),struct MsgPort* _a1 __asm("a1"));

ULONG __saveds GetUsbError       (PUSB_SUPPORT Req __asm("a0"));
UBYTE __saveds SetupRootHubMessagePort(PUSB_SUPPORT Req __asm("a0"),
				 	struct MsgPort *Port __asm("a1"));
UBYTE __saveds FreeRootHubMessagePort(PUSB_SUPPORT Req __asm("a0"),
				 	struct MsgPort *Port __asm("a1"));

ULONG __saveds Get32BitFrameNumber	   (PUSB_SUPPORT req __asm("a0"));
PHCD_DEVICE_DATA __saveds GetUsbBaseAddress(PUSB_SUPPORT Req __asm("a0"));
void  __saveds KillHost		(PUSB_SUPPORT Req __asm("a0"));
void  __saveds StartHost	(PUSB_SUPPORT Req __asm("a0"));
void  __saveds StopHost		(PUSB_SUPPORT Req __asm("a0"));
void  __saveds PauseHost	(PUSB_SUPPORT Req __asm("a0"));
UBYTE __saveds SetUsbState	(PUSB_SUPPORT Req __asm("a0"),ULONG usbstate __asm("d0"));
ULONG __saveds GetUsbState	(PUSB_SUPPORT Req __asm("a0"));

char*  __saveds AllocUsbMem(PUSB_SUPPORT Req __asm("a0"),ULONG BufferLeng __asm("d0"));
void  __saveds FreeUsbMem  (PUSB_SUPPORT Req __asm("a0"),char *Mem __asm("a1"));

PUSB_SUPPORT __saveds AllocUsbSupport(void);
UBYTE __saveds InitUsbSupport(PUSB_SUPPORT Req __asm("a0"));
void  __saveds FreeUsbSupport(PUSB_SUPPORT Req __asm("a0"));

PENDPOINT_DESCRIPTOR __saveds AllocEndpoint(PUSB_SUPPORT req __asm("a0"),ULONG type __asm("d0"),
 				                                ULONG rate __asm("d1"),PENDPOINT_CONTROL cont __asm("a1"));
void  __saveds FreeEndpoint         (PUSB_SUPPORT req __asm("a0"),PENDPOINT_DESCRIPTOR Endpoint __asm("a1"));
void  __saveds GetEndpointData      (PUSB_SUPPORT req __asm("a0"),PENDPOINT_DESCRIPTOR Endpoint __asm("a1"));
void  __saveds SetEndpointData      (PUSB_SUPPORT req __asm("a0"),PENDPOINT_DESCRIPTOR Endpoint __asm("a1"));
UBYTE __saveds  OpenPipe            (PUSB_SUPPORT req __asm("a0"),PENDPOINT_DESCRIPTOR ed __asm("a1"));
void  __saveds ClosePipe            (PUSB_SUPPORT req __asm("a0"),PENDPOINT_DESCRIPTOR ed __asm("a1"));

PUSBD_REQUEST __saveds AllocUsbdRequest(PUSB_SUPPORT req __asm("a0"),ULONG BufferLeng __asm("d0"));
void  __saveds FreeUsbdRequest         (PUSB_SUPPORT req __asm("a0"),PUSBD_REQUEST usb __asm("a1"));
void  __saveds AbortUsbdRequest        (PUSB_SUPPORT req __asm("a0"),PUSBD_REQUEST usb __asm("a1"));
void  __saveds FreeUsbdRequestBuffer   (PUSB_SUPPORT req __asm("a0"),PUSBD_REQUEST usb __asm("a1"));
ULONG __saveds AllocUsbdRequestBuffer  (PUSB_SUPPORT req __asm("a0"),PUSBD_REQUEST usb __asm("a1"),
										 ULONG BufferLength __asm("d0"));
void  __saveds SetupUsbdRequest	  (PUSBD_REQUEST usb __asm("a0"), UBYTE bRequestType __asm("d0"),
                                   UBYTE bRequest __asm("d1"),UWORD wValue __asm("d2"),						 
                                   UWORD wIndex __asm("d3"),UWORD wLength __asm("d4"));
UBYTE __saveds SendUsbdRequest 	  (PUSB_SUPPORT req __asm("a0"),PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                    PUSBD_REQUEST usb __asm("a2"));  
ULONG __saveds WaitUsbdRequest( PUSB_SUPPORT support __asm("a0"),PUSBD_REQUEST usb __asm("a1"));                                   

UBYTE __saveds SendSyncUsbdRequest(PUSB_SUPPORT Req __asm("a0"),
				   PENDPOINT_DESCRIPTOR ed __asm("a1"),
				   PUSBD_REQUEST usb __asm("a2"));
UBYTE __saveds SendUsbData        (PUSB_SUPPORT Req __asm("a0"),
                                   PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                   PUSBD_REQUEST usb __asm("a2"));
UBYTE __saveds ReceiveUsbData     (PUSB_SUPPORT Req __asm("a0"),
                                   PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                   PUSBD_REQUEST usb __asm("a2"));
UBYTE __saveds ClearFeature(PUSB_SUPPORT req __asm("a0"),
                            PENDPOINT_DESCRIPTOR ed __asm("a1"),
                            PUSBD_REQUEST usb __asm("a2"),
                            ULONG recipient __asm("d0"),
                            UWORD Feature __asm("d1"),
                            UBYTE num __asm("d2"));

UBYTE __saveds GetConfiguration(PUSB_SUPPORT req __asm("a0"),
                                PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                PUSBD_REQUEST usb __asm("a2"));
UBYTE __saveds GetDescriptor(PUSB_SUPPORT req __asm("a0"),
                             PENDPOINT_DESCRIPTOR ed __asm("a1"),
                             PUSBD_REQUEST usb __asm("a2"),
                             UBYTE type __asm("d0"),
                             UBYTE index __asm("d1"),
                             UWORD Leng __asm("d2"),
                             UWORD LangID __asm("d3"));
UBYTE __saveds GetInterface(PUSB_SUPPORT req __asm("a0"),
                            PENDPOINT_DESCRIPTOR ed __asm("a1"),
                            PUSBD_REQUEST usb __asm("a2"),
                            UBYTE NInterface __asm("d0"));
UWORD __saveds GetStatus(PUSB_SUPPORT req __asm("a0"),
                         PENDPOINT_DESCRIPTOR ed __asm("a1"),
                         PUSBD_REQUEST usb __asm("a2"),
                         ULONG Recipient __asm("d0"),
                         UBYTE NumInterface __asm("d1"));
UBYTE __saveds SetAddress(PUSB_SUPPORT req __asm("a0"),
                          PENDPOINT_DESCRIPTOR ed __asm("a1"),
                          PUSBD_REQUEST usb __asm("a2"),
                          UBYTE Address __asm("d0"));

UBYTE __saveds SetConfiguration(PUSB_SUPPORT req __asm("a0"),
                                PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                PUSBD_REQUEST usb __asm("a2"),
                                UBYTE Value __asm("d0"));

UBYTE __saveds SetDescriptor(PUSB_SUPPORT req __asm("a0"),
                             PENDPOINT_DESCRIPTOR ed __asm("a1"),
                             PUSBD_REQUEST usb __asm("a2"),
                             UBYTE Type __asm("d0"),
                             UBYTE Index __asm("d1"),
                             UWORD Leng __asm("d2"),
                             UWORD LangID __asm("d3"));

UBYTE __saveds SetFeature(PUSB_SUPPORT req __asm("a0"),
                          PENDPOINT_DESCRIPTOR ed __asm("a1"),
                          PUSBD_REQUEST usb __asm("a2"),
                          ULONG Recipient __asm("d0"),
                          UBYTE Interface __asm("d1"),
                          UWORD Feature __asm("d2"));


UBYTE __saveds SetInterface(PUSB_SUPPORT req __asm("a0"),
                            PENDPOINT_DESCRIPTOR ed __asm("a1"),
                            PUSBD_REQUEST usb __asm("a2"),
                            UBYTE Interface __asm("d0"),
                            UBYTE AltInterface __asm("d1"));

UBYTE __saveds SynchFrame(PUSB_SUPPORT req __asm("a0"),
                          PENDPOINT_DESCRIPTOR ed __asm("a1"),
                          PUSBD_REQUEST usb __asm("a2"),
                          UWORD Frame __asm("d0"));

UBYTE __saveds ClearDeviceFeature(PUSB_SUPPORT req __asm("a0"),
                                  PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                  PUSBD_REQUEST usb __asm("a2"),
                                  UWORD Feature __asm("d0"));

UBYTE __saveds ClearEndpointFeature(PUSB_SUPPORT req __asm("a0"),
                                    PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                    PUSBD_REQUEST usb __asm("a2"),
                                    UWORD Feature __asm("d0"));

UBYTE __saveds ClearInterfaceFeature(PUSB_SUPPORT req __asm("a0"),
                                     PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                     PUSBD_REQUEST usb __asm("a2"),
                                     UWORD Feature __asm("d0"),
                                     UBYTE NumInterface __asm("d1"));

UBYTE __saveds SetDeviceFeature(PUSB_SUPPORT req __asm("a0"),
                                PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                PUSBD_REQUEST usb __asm("a2"),
                                UWORD Feature __asm("d0"));
 
UBYTE __saveds SetEndpointFeature(PUSB_SUPPORT req __asm("a0"),
                                  PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                  PUSBD_REQUEST usb __asm("a2"),
                                  UWORD Feature __asm("d0"));

UBYTE __saveds SetInterfaceFeature(PUSB_SUPPORT req __asm("a0"),
                                   PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                   PUSBD_REQUEST usb __asm("a2"),
                                   UWORD Feature __asm("d0"),
                                   UBYTE NumInterface __asm("d1"));

PUSB_DEVICE_DESCRIPTOR __saveds GetDeviceDescriptor(PUSB_SUPPORT req __asm("a0"),
												       PENDPOINT_DESCRIPTOR ed __asm("a1"),
												       PUSBD_REQUEST usb __asm("a2"));
void __saveds FreeDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR dev __asm("a0"));


PUSB_CONFIGURATION_DESCRIPTOR __saveds GetConfigurationDescriptor(PUSB_SUPPORT req __asm("a0"),
                                                         PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                                         PUSBD_REQUEST usb __asm("a2"),
                                                         UBYTE ConfValue __asm("d0"));

void __saveds FreeConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR conf __asm("a0"));

PUSB_INTERFACE_DESCRIPTOR __saveds GetInterfaceDescriptor(PUSB_SUPPORT req __asm("a0"),
                                                          PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                                          PUSBD_REQUEST usb __asm("a2"),
                                                          UBYTE IntValue __asm("d0"));
void __saveds FreeInterfaceDescriptor(PUSB_INTERFACE_DESCRIPTOR inte __asm("a0"));
PUSB_ENDPOINT_DESCRIPTOR __saveds GetEndpointDescriptor(PUSB_SUPPORT req __asm("a0"),
                                                        PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                                        PUSBD_REQUEST usb __asm("a2"),
                                                        UBYTE EndpointNumber __asm("d0"));
void __saveds FreeEndpointDescriptor(PUSB_ENDPOINT_DESCRIPTOR endp __asm("a0"));



UBYTE __saveds GetConfigurationChunk(PUSB_SUPPORT req __asm("a0"),
                                     PENDPOINT_DESCRIPTOR ed __asm("a1"),
                                     PUSBD_REQUEST usb __asm("a2"),
                                     PUSB_CONFIGURATION_DESCRIPTOR conf __asm("a3"));


ULONG __saveds GetString(PUSB_SUPPORT req __asm("a0"),
                         PENDPOINT_DESCRIPTOR ed __asm("a1"),
                         PUSBD_REQUEST usb __asm("a2"),
                         UBYTE StringID __asm("d0"),
                         char* dest __asm("a3"));





UBYTE __saveds SendStackRequest (PUSB_SUPPORT Support __asm("a0"));
UBYTE __saveds LockFunction     (PUSB_SUPPORT Support __asm("a0"),ULONG FunctionNumber __asm("d0"));
void  __saveds UnLockFunction   (PUSB_SUPPORT Support __asm("a0"),ULONG FunctionAddress __asm("d0"));
UBYTE __saveds UsbInitFunction  (PUSB_SUPPORT Support __asm("a0"),UBYTE DeviceSpeed __asm("d0"));
void  __saveds UsbRemoveFunction(PUSB_SUPPORT Support __asm("a0"),UBYTE FunctionAddress __asm("d0"));

PUSB_DEVICE __saveds GetUsbDevice(PUSB_SUPPORT Support __asm("a0"),UBYTE FunctionAddress __asm("d0"));

#endif