#ifndef ROOT_HUB_CLASS_PROTS
#define ROOT_HUB_CLASS_PROTS

#ifdef __cplusplus
extern "C" {
#endif

#include <exec/exec.h>
#include <usb/HcDriverStructs.h>
#include <usb/HubClass.h>

UBYTE ClearHubFeature(PUSB_SUPPORT Req,
                      PENDPOINT_DESCRIPTOR   ed,
		      PUSBD_REQUEST   usb,
                      UBYTE FEATURE_SELECTOR);

void ClearPortFeature(PUSB_SUPPORT Req,
                      PENDPOINT_DESCRIPTOR   ed,
		      PUSBD_REQUEST   usb,
		      ULONG FEATURE_SELECTOR,
		      UBYTE PORT);

UBYTE GetBusState(PUSB_SUPPORT Req,
                  PENDPOINT_DESCRIPTOR   ed,
		  PUSBD_REQUEST   usb,
		  UBYTE PORT);

PHUB_DESCRIPTOR GetHubDescriptor(PUSB_SUPPORT Req,
		                 PENDPOINT_DESCRIPTOR   ed,
				 PUSBD_REQUEST   usb);

void FreeHubDescriptor(PHUB_DESCRIPTOR hub);

ULONG GetHubStatus(PUSB_SUPPORT  Req,
		   PENDPOINT_DESCRIPTOR   ed,
 		   PUSBD_REQUEST   usb);

ULONG GetPortStatus(PUSB_SUPPORT Req,
		    PENDPOINT_DESCRIPTOR   ed,
 		    PUSBD_REQUEST   usb,
		    UBYTE PortNumber);

UBYTE SetHubDescriptor(PUSB_SUPPORT Req,
                       PENDPOINT_DESCRIPTOR   ed,
		       PUSBD_REQUEST   usb);

UBYTE SetHubFeature(PUSB_SUPPORT Req,
                    PENDPOINT_DESCRIPTOR   ed,
		    PUSBD_REQUEST   usb,
		    ULONG FEATURE_SELECTOR);

void SetPortFeature(PUSB_SUPPORT Req,
		    PENDPOINT_DESCRIPTOR   ed,
		    PUSBD_REQUEST   usb,
		    ULONG FEATURE_SELECTOR,
		    UBYTE PORT);
void WaitCSC(PUSB_SUPPORT Req,
	     PUSBD_REQUEST usb);

#ifdef __cplusplus
};
#endif
#endif
