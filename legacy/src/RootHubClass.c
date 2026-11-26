#include <exec/exec.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <usb/HcDriverStructs.h>
#include <usb/HubClass.h>
#include <usb/ProtosAsm/AsmHc_Protos.h>
//#include "RootHubConstructors.h"

UBYTE ClearHubFeature(PUSB_SUPPORT Req,
                      PENDPOINT_DESCRIPTOR   ed,
		      PUSBD_REQUEST   usb,
                      UBYTE FEATURE_SELECTOR)
{
  switch(FEATURE_SELECTOR)
  {
   case C_HUB_LOCAL_POWER:  HcRhClearGlobalPower(Req->devicedata);break;
   case C_HUB_OVER_CURRENT: HcRhClearOvercurrentIndicatorChange(Req->devicedata);break;
   default:break;
  }
  return(1);
}

void ClearPortFeature(PUSB_SUPPORT Req,
                      PENDPOINT_DESCRIPTOR   ed,
		      PUSBD_REQUEST   usb,
		      ULONG FEATURE_SELECTOR,
		      UBYTE PORT)
{
  switch(FEATURE_SELECTOR)
  {
   case PORT_ENABLE:        HcRhPort_ClearPortEnable   (Req->devicedata,PORT);break;
   case PORT_SUSPEND:       HcRhPort_ClearSuspendStatus(Req->devicedata,PORT);break;
   case PORT_POWER:         HcRhPort_ClearPortPower    (Req->devicedata,PORT);break;
   case C_PORT_CONNECTION:  HcRhPort_ClearConnectStatusChange     (Req->devicedata,PORT);break;
   case C_PORT_RESET:       HcRhPort_ClearPortResetstatusChange   (Req->devicedata,PORT);break;
   case C_PORT_ENABLE:      HcRhPort_ClearPortEnableStatusChange  (Req->devicedata,PORT);break;
   case C_PORT_SUSPEND:     HcRhPort_ClearPortSuspendSatatusChange(Req->devicedata,PORT);break;
   case C_PORT_OVER_CURRENT:HcRhPort_ClearPortOvercurrentIndicatorChange(Req->devicedata,PORT);break;
   default:break;
  }
}

UBYTE GetBusState(PUSB_SUPPORT Req,
                  PENDPOINT_DESCRIPTOR   ed,
		  PUSBD_REQUEST   usb,
		  UBYTE PORT)
{
 return(3);
}

extern HUB_DESCRIPTOR RHub;

PHUB_DESCRIPTOR GetHubDescriptor(PUSB_SUPPORT Req,
		                 PENDPOINT_DESCRIPTOR   ed,
				 PUSBD_REQUEST   usb)
{
 ULONG A=ConvertLong(Req->devicedata->HC->HcRhDescriptorA);
 ULONG B=ConvertLong(Req->devicedata->HC->HcRhDescriptorB);
 RHub.bNbrPorts           = A & 0xFF;
 RHub.wHubCharacteristics = (A >> 8) & 0xFFFF;
 RHub.bPwrOn2PwrGood      = (A >>24) & 0xFF;
 RHub.DeviceRemovable     = B & 0xFFFF;
 RHub.PortPwrCtrlMask     = (B >>16) & 0xFFFF;
 return(&RHub);
}

void FreeHubDescriptor(PHUB_DESCRIPTOR hub)
{
 hub=NULL;
}

ULONG GetHubStatus(PUSB_SUPPORT  Req,
		   PENDPOINT_DESCRIPTOR   ed,
 		   PUSBD_REQUEST   usb)
{
 return(ConvertLong(Req->devicedata->HC->HcRhStatus));
}

ULONG GetPortStatus(PUSB_SUPPORT Req,
		    PENDPOINT_DESCRIPTOR   ed,
 		    PUSBD_REQUEST   usb,
		    UBYTE PortNumber)
{
 return(ConvertLong(Req->devicedata->HC->HcRhPortStatus[(PortNumber-1)]));
}

UBYTE SetHubDescriptor(PUSB_SUPPORT Req,
                       PENDPOINT_DESCRIPTOR   ed,
		       PUSBD_REQUEST   usb)
{
 return(1);
}

UBYTE SetHubFeature(PUSB_SUPPORT Req,
                    PENDPOINT_DESCRIPTOR   ed,
		    PUSBD_REQUEST   usb,
		    ULONG FEATURE_SELECTOR)
{
 switch(FEATURE_SELECTOR)
 {
  default:break;
 } 
 return(1);
}

void SetPortFeature(PUSB_SUPPORT Req,
		    PENDPOINT_DESCRIPTOR   ed,
		    PUSBD_REQUEST   usb,
		    ULONG FEATURE_SELECTOR,
		    UBYTE PORT)
{
 switch (FEATURE_SELECTOR)
 {
  case PORT_RESET:  HcRhPort_SetPortReset  (Req->devicedata,PORT);break;
  case PORT_SUSPEND:HcRhPort_SetPortSuspend(Req->devicedata,PORT);break;
  case PORT_POWER:  HcRhPort_SetPortPower  (Req->devicedata,PORT);break;
  deafult:break;
 }
}

void WaitCSC(PUSB_SUPPORT Req,
	     PUSBD_REQUEST usb)
{
 WaitPort(Req->UserPort);
  // while(Req->DummyMess=GetMsg(Req->UserPort));
}
