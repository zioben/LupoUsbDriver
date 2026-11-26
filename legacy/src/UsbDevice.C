#include <usb/HcDriverStructs.h>

#define  TOTAL_USB_FUNCTION 128

USB_DEVICE UsbDeviceList[TOTAL_USB_FUNCTION];


void SetupRootHub()
{
  UsbDeviceList[0].UsbDevice.bDeviceClass   = 0x09;
  UsbDeviceList[0].UsbDevice.bDeviceSubClass= 0x00;
  UsbDeviceList[0].Connected	    = 0x01;
  UsbDeviceList[0].Addressed	    = 0x01;
  UsbDeviceList[0].Configured	    = 0x01;
}



void InitUsbDeviceList()
{
  SetupRootHub();
}




PUSB_DEVICE GetUsbDevice(char Address)
{
  if (Address>TOTAL_USB_FUNCTION) return(NULL);
  return(&UsbDeviceList[Address]);
}




PUSB_DEVICE FindUsbDeviceClassSubClass(char Class,char SubClass)
{
  int i=0;
  while (i<TOTAL_USB_FUNCTION)
  {
    if ((UsbDeviceList[i].UsbDevice.bDeviceClass==Class) &&
        (UsbDeviceList[i].UsbDevice.bDeviceSubClass==SubClass))
	 return(&UsbDeviceList[i]);
    i++;
  }
  return(NULL);
}



PUSB_DEVICE FindUsbDeviceVendorProduct(short Vend,short Prod)
{
  int i=0;
  short V=((Vend & 0xFF)<<8)+((Vend & 0xFF00)>>8);
  short P=((Prod & 0xFF)<<8)+((Prod & 0xFF00)>>8);
  while (i<TOTAL_USB_FUNCTION)
  {
    if ((UsbDeviceList[i].UsbDevice.idVendor ==V) &&
        (UsbDeviceList[i].UsbDevice.idProduct==P))
        return(&UsbDeviceList[i]);
    else i++;
  }
  return(NULL);
}


