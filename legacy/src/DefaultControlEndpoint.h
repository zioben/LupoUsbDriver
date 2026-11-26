    //--------------------------------------------------------------
   // Standard Device Request  From Universal Serial Bus Spec. 1.1
  //  Imported By Zioben (painaga@dei.unipd.it zioben@inwind.it)
 //
//------------------------------------------------------------------

#ifndef CONTROL_ENDPOINT_DEFAULT
#define CONTROL_ENDPOINT_DEFAULT

#ifdef __cplusplus
extern "C" {
#endif

#include <exec/exec.h>
#include "HcDriverstructs.h"

#define MAX_STRING_CHARS 24
#define MAX_ENDPOINTS  	 15



// bmRequestType standard type class

#define BM_REQUEST_DIRECTION_IN	0x80
#define BM_REQUEST_DIRECTION_OUT 0x00
#define BM_REQUEST_ENDPOINT		0x02
#define BM_REQUEST_INTERFACE		0x01
#define BM_REQUEST_DEVICE			0x00

// bRequest standard request commands

#define GET_STATUS					0
#define CLEAR_FEATURE				1
#define SET_FEATURE					3
#define SET_ADDRESS					5
#define GET_DESCRIPTOR				6
#define SET_DESCRIPTOR				7
#define GET_CONFIGURATION			8
#define SET_CONFIGURATION			9
#define GET_INTERFACE				10
#define SET_INTERFACE				11
#define SYNCH_FRAME					12

// Descriptor types (for type in wValue)

#define DESCRIPTOR_DEVICE			1
#define DESCRIPTOR_CONFIGURATION	2
#define DESCRIPTOR_STRING			3
#define DESCRIPTOR_INTERFACE		4
#define DESCRIPTOR_ENDPOINT		5

//feature selectors (use with RECIPIENT_TYPE)

#define DEVICE_REMOTE_WAKEUP		1
#define ENDPOINT_HALT				0

//recipient types

#define RECIPIENT_INTERFACE		0
#define RECIPIENT_ENDPOINT			1
#define RECIPIENT_DEVICE			2


//-----------------------------------------------
//Strutture:
//-----------------------------------------------



typedef struct _USB_DEVICE_REQUEST
{
  UBYTE bmRequestType;// Direction ,Type , Recipient
  UBYTE bRequest;     // Specific Request
  UWORD wValue;       // Lil Endian  wValue = ConvertWord( 16_bit_number );
  UWORD wIndex;       // Lil Endian  IDEM
  UWORD wLength;	    // Lil Endian  IDEM
}
USB_DEVICE_REQUEST,*PUSB_DEVICE_REQUEST;


typedef struct _USB_DEVICE_DESCRIPTOR
{
  UBYTE bLength;
  UBYTE bDescriptorType;
  UWORD bcdUSB;
  UBYTE bDeviceClass;
  UBYTE bDeviceSubClass;
  UBYTE bDeviceProtocol;
  UBYTE bMaxPacketSize0;
  UWORD idVendor;
  UWORD idProduct;
  UWORD bcdDevice;
  UBYTE iManufacturer;
  UBYTE iProduct;
  UBYTE iSerialNumber;
  UBYTE bNumConfigurations;
}
USB_DEVICE_DESCRIPTOR,*PUSB_DEVICE_DESCRIPTOR;


typedef struct _USB_CONFIGURATION_DESCRIPTOR
{
  UBYTE bLength;
  UBYTE bDescriptorType;
  UWORD wTotalLength;
  UBYTE bNumInterfaces;
  UBYTE bConfigurationValue;
  UBYTE iConfiguration;
  UBYTE bmAttributes;
  UBYTE MaxPower;
}
USB_CONFIGURATION_DESCRIPTOR,*PUSB_CONFIGURATION_DESCRIPTOR;


typedef struct _USB_INTERFACE_DESCRIPTOR
{
  UBYTE bLength;
  UBYTE bDescriptorType;
  UBYTE bInterfaceNumber;
  UBYTE bAlternateSettings;
  UBYTE bNumEndpoints;
  UBYTE bInterfaceClass;
  UBYTE bInterfaceSubClass;
  UBYTE bInterfaceProtocol;
  UBYTE iInterface;
}
USB_INTERFACE_DESCRIPTOR,*PUSB_INTERFACE_DESCRIPTOR;


typedef struct _USB_ENDPOINT_DESCRIPTOR
{
  UBYTE bLength;
  UBYTE bDescriptorType;
  UBYTE bEndpointAddress;
  UBYTE bmAttributes;
  UWORD wMaxPacketSize;
  UBYTE bInterval;
}
USB_ENDPOINT_DESCRIPTOR,*PUSB_ENDPOINT_DESCRIPTOR;



typedef struct _USB_STRING
{
  UBYTE bLength;
  UBYTE bDescriptorType;
  UBYTE cString[MAX_STRING_CHARS];
}
USB_STRING,*PUSB_STRING;

typedef struct _USB_STRING_LANGUAGE
{
  UBYTE bLength;
  UBYTE bDescriptorType;
  UWORD wLANGID[15];
}
USB_STRING_LANGUAGE,*PUSB_STRING_LANGUAGE;



//-----------------------------------------------------------
// Definizione di un USB device
//-----------------------------------------------------------


//#define USB_DEVICE_ATTACHED	1<<0
//#define USB_DEVICE_POWERED    1<<1
//#define USB_DEVICE_DEFAULT    1<<2
#define USB_DEVICE_ADDRESS    1<<3
#define USB_DEVICE_CONFIGURED 1<<4
#define USB_DEVICE_LOCKED     1<<8


typedef struct _USB_DEVICE
{
 ULONG                          USB_DEVICE_Status;
 ULONG                          USB_DEVICE_Number;
 PUSB_DEVICE_DESCRIPTOR         USB_DEVICE_Descriptor;
 char									  iManufacturer[MAX_STRING_CHARS];
 char									  iProduct[MAX_STRING_CHARS];
 char									  iSerialNumber[MAX_STRING_CHARS];
 PUSB_CONFIGURATION_DESCRIPTOR  USB_DEVICE_Configuration;
 char									  iConfiguration[MAX_STRING_CHARS];
 PUSB_INTERFACE_DESCRIPTOR      USB_DEVICE_Interface;
 PUSB_ENDPOINT_DESCRIPTOR       USB_DEVICE_Endpoint[MAX_ENDPOINTS];
 PUSB_INTERFACE_DESCRIPTOR      USB_DEVICE_AltInterface;
 PUSB_ENDPOINT_DESCRIPTOR       USB_DEVICE_AltEndpoint[MAX_ENDPOINTS];
 PENDPOINT_DESCRIPTOR           USB_DEVICE_ControlPipe;
 PENDPOINT_DESCRIPTOR           USB_DEVICE_Pipe[MAX_ENDPOINTS];
}
USB_DEVICE,*PUSB_DEVICE;

#ifdef __cplusplus
};
#endif
#endif
