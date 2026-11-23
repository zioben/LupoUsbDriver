#ifndef ROOT_HUB_CONSTRUCTORS
#define ROOT_HUB_CONSTRUCTORS

#ifdef __cplusplus
extern "C" {
#endif

#include <usb/HcDriverStructs.h>
#include <usb/hubClass.h>

DEVICE_DESCRIPTOR	RHDevice=
{
 sizeof(DEVICE_DESCRIPTOR),
 0,
 0,
 0,
 0x09,
 0x00,
 0,
 0,
 0,
 0,
 0,
 0,
 0,
 1
};


CONFIGURATION_DESCRIPTOR RHConfiguration=
{
 sizeof(CONFIGURATION_DESCRIPTOR),
 0,
 0,
 1,
 0,
 0,
 0,
 250
};


INTERFACE_DESCRIPTOR     RHInterface=
{
 sizeof(INTERFACE_DESCRIPTOR),
 0,
 0,
 0,
 1,
 0x09,
 0,
 0,
 0
};


ENDPOINT_DESCRIPTOR      RHEndpoint=
{
 sizeof(ENDPOINT_DESCRIPTOR),
 0,
 1,
 0x07,
 0x08,
 0xFF
};


HUB_DESCRIPTOR           RHub=
{
 sizeof(HUB_DESCRIPTOR),
 0x29,
 0,
 0,
 0,
 250,
 0,
 0
};



#ifdef __cplusplus
};
#endif
#endif
