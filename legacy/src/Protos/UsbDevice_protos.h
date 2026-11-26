#ifndef USBDEVICE_PROTOS
#define USBDEVICE_PROTOS

#include <usb/HcDriverStructs.h>

void InitUsbDeviceList();
PUSB_DEVICE GetUsbDevice(char Address);
PUSB_DEVICE FindUsbDeviceClassSubClass(char Class,char SubClass);
PUSB_DEVICE FindUsbDeviceVendorProduct(short Vend,short Prod);

#endif