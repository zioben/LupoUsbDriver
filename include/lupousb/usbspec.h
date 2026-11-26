/*
 * LupoUsbDriver - constants from the USB 1.1/2.0 specification
 * (chapter 9 "USB Device Framework", chapter 11 "Hub Specification")
 * and from the HID 1.11 specification.
 *
 * Written from the public specifications. Descriptors are handled as byte
 * arrays (little endian, unaligned) and parsed with lu_get_le16(), so no
 * packed structures are needed.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef LUPOUSB_USBSPEC_H
#define LUPOUSB_USBSPEC_H

/* bmRequestType */
#define USB_DIR_OUT             0x00
#define USB_DIR_IN              0x80
#define USB_TYPE_STANDARD       0x00
#define USB_TYPE_CLASS          0x20
#define USB_TYPE_VENDOR         0x40
#define USB_RECIP_DEVICE        0x00
#define USB_RECIP_INTERFACE     0x01
#define USB_RECIP_ENDPOINT      0x02
#define USB_RECIP_OTHER         0x03

/* Standard requests (bRequest) */
#define USB_REQ_GET_STATUS          0
#define USB_REQ_CLEAR_FEATURE       1
#define USB_REQ_SET_FEATURE         3
#define USB_REQ_SET_ADDRESS         5
#define USB_REQ_GET_DESCRIPTOR      6
#define USB_REQ_SET_DESCRIPTOR      7
#define USB_REQ_GET_CONFIGURATION   8
#define USB_REQ_SET_CONFIGURATION   9
#define USB_REQ_GET_INTERFACE       10
#define USB_REQ_SET_INTERFACE       11
#define USB_REQ_SYNCH_FRAME         12

/* Standard feature selectors */
#define USB_FEAT_ENDPOINT_HALT        0
#define USB_FEAT_DEVICE_REMOTE_WAKEUP 1

/* Descriptor types */
#define USB_DT_DEVICE           1
#define USB_DT_CONFIG           2
#define USB_DT_STRING           3
#define USB_DT_INTERFACE        4
#define USB_DT_ENDPOINT         5
#define USB_DT_HID              0x21
#define USB_DT_REPORT           0x22
#define USB_DT_HUB              0x29

#define USB_DT_DEVICE_SIZE      18
#define USB_DT_CONFIG_SIZE      9
#define USB_DT_INTERFACE_SIZE   9
#define USB_DT_ENDPOINT_SIZE    7

/* Device descriptor field offsets */
#define USB_DD_bcdUSB           2
#define USB_DD_bDeviceClass     4
#define USB_DD_bDeviceSubClass  5
#define USB_DD_bDeviceProtocol  6
#define USB_DD_bMaxPacketSize0  7
#define USB_DD_idVendor         8
#define USB_DD_idProduct        10
#define USB_DD_bcdDevice        12
#define USB_DD_iManufacturer    14
#define USB_DD_iProduct         15
#define USB_DD_iSerialNumber    16
#define USB_DD_bNumConfigurations 17

/* Configuration descriptor field offsets */
#define USB_CD_wTotalLength     2
#define USB_CD_bNumInterfaces   4
#define USB_CD_bConfigurationValue 5
#define USB_CD_bmAttributes     7
#define USB_CD_bMaxPower        8

/* Interface descriptor field offsets */
#define USB_ID_bInterfaceNumber   2
#define USB_ID_bAlternateSetting  3
#define USB_ID_bNumEndpoints      4
#define USB_ID_bInterfaceClass    5
#define USB_ID_bInterfaceSubClass 6
#define USB_ID_bInterfaceProtocol 7

/* Endpoint descriptor field offsets and attributes */
#define USB_ED_bEndpointAddress 2
#define USB_ED_bmAttributes     3
#define USB_ED_wMaxPacketSize   4
#define USB_ED_bInterval        6
#define USB_EP_DIR_IN           0x80
#define USB_EP_NUM_MASK         0x0f
#define USB_EP_XFER_MASK        0x03
#define USB_EP_XFER_CONTROL     0
#define USB_EP_XFER_ISO         1
#define USB_EP_XFER_BULK        2
#define USB_EP_XFER_INT         3

/* Class codes */
#define USB_CLASS_PER_INTERFACE 0x00
#define USB_CLASS_HID           0x03
#define USB_CLASS_MASS_STORAGE  0x08
#define USB_CLASS_HUB           0x09

/* ---- Hub class (chapter 11) ---- */
#define USB_RT_HUB   (USB_TYPE_CLASS | USB_RECIP_DEVICE)
#define USB_RT_PORT  (USB_TYPE_CLASS | USB_RECIP_OTHER)

#define HUB_REQ_GET_STATUS      0
#define HUB_REQ_CLEAR_FEATURE   1
#define HUB_REQ_SET_FEATURE     3
#define HUB_REQ_GET_DESCRIPTOR  6

/* Hub feature selectors */
#define C_HUB_LOCAL_POWER       0
#define C_HUB_OVER_CURRENT      1

/* Port feature selectors */
#define PORT_CONNECTION         0
#define PORT_ENABLE             1
#define PORT_SUSPEND            2
#define PORT_OVER_CURRENT       3
#define PORT_RESET              4
#define PORT_POWER              8
#define PORT_LOW_SPEED          9
#define C_PORT_CONNECTION       16
#define C_PORT_ENABLE           17
#define C_PORT_SUSPEND          18
#define C_PORT_OVER_CURRENT     19
#define C_PORT_RESET            20

/* wPortStatus bits (low word of GET_PORT_STATUS) */
#define USB_PS_CONNECTION       (1u << 0)
#define USB_PS_ENABLE           (1u << 1)
#define USB_PS_SUSPEND          (1u << 2)
#define USB_PS_OVER_CURRENT     (1u << 3)
#define USB_PS_RESET            (1u << 4)
#define USB_PS_POWER            (1u << 8)
#define USB_PS_LOW_SPEED        (1u << 9)
#define USB_PS_HIGH_SPEED       (1u << 10)
/* wPortChange bits (high word) */
#define USB_PC_CONNECTION       (1u << 0)
#define USB_PC_ENABLE           (1u << 1)
#define USB_PC_SUSPEND          (1u << 2)
#define USB_PC_OVER_CURRENT     (1u << 3)
#define USB_PC_RESET            (1u << 4)

/* Hub descriptor field offsets */
#define USB_HD_bNbrPorts            2
#define USB_HD_wHubCharacteristics  3
#define USB_HD_bPwrOn2PwrGood       5
#define USB_HD_bHubContrCurrent     6
#define USB_HD_DeviceRemovable      7

/* ---- HID class ---- */
#define HID_SUBCLASS_BOOT       1
#define HID_PROTO_KEYBOARD      1
#define HID_PROTO_MOUSE         2

#define HID_REQ_GET_REPORT      0x01
#define HID_REQ_GET_IDLE        0x02
#define HID_REQ_GET_PROTOCOL    0x03
#define HID_REQ_SET_REPORT      0x09
#define HID_REQ_SET_IDLE        0x0a
#define HID_REQ_SET_PROTOCOL    0x0b

/* Timings (ms), USB 2.0 spec 7.1.7.3 and 9.2.6.3 */
#define USB_T_ATTDB             100  /* connect debounce */
#define USB_T_DRSTR             10   /* root port reset (min 10, hub uses 10-20) */
#define USB_T_RSTRCY            10   /* reset recovery */
#define USB_T_SETADDR_RECOVERY  2

#endif /* LUPOUSB_USBSPEC_H */
