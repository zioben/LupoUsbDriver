     //--------------^------------------=====--------------------
    //
   //  Hub Class Headers from USB 1.1 Specification
  //   Root Hub compatibile (by zioben@inwind.it january-2003) 
 //
//----=========----^----------------^---------------------====---

#ifndef HUB_CLASS
#define HUB_CLASS

#ifdef __cplusplus
extern "C" {
#endif

#include <exec/exec.h>

typedef struct _HUB_DESCRIPTOR
{
  UBYTE bDescriptorLength;
  UBYTE bDescriptorType;
  UBYTE bNbrPorts;
  UWORD wHubCharacteristics;
  UBYTE bPwrOn2PwrGood;
  UBYTE bHubContrCurrent;
  UWORD DeviceRemovable;
  UWORD PortPwrCtrlMask;
}
HUB_DESCRIPTOR,*PHUB_DESCRIPTOR;

#define HUB_DESCRIPTOR_TYPE 	0x29
#define HUB_DESCRIPTOR_INDEX	0x00

#define C_HUB_LOCAL_POWER	0
#define C_HUB_OVER_CURRENT	1
#define PORT_CONNECTION		0
#define PORT_ENABLE		1
#define PORT_SUSPEND		2
#define PORT_OVER_CURRENT	3
#define PORT_RESET		4
#define PORT_POWER		8
#define PORT_LOW_SPEED		9
#define C_PORT_CONNECTION	16
#define C_PORT_ENABLE		17
#define C_PORT_SUSPEND		18
#define C_PORT_OVER_CURRENT	19
#define C_PORT_RESET		20

#ifdef __cplusplus
};
#endif
#endif

