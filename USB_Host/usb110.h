/* INSERT LICENSE HERE */

#ifndef USB110_H_INCLUDED
#define USB110_H_INCLUDED

/******************************************************************************
Macro definitions
******************************************************************************/
#include <stdint.h>
/******************************************************************************
* >>>>>>>>>>>>> All of this information came from USB 1.1 spec. <<<<<<<<<<<<  *
******************************************************************************/

/* Values for the bits returned by the USB GET_STATUS command */
#define USB_GETSTATUS_SELF_POWERED              0x01
#define USB_GETSTATUS_REMOTE_WAKEUP_ENABLED     0x02

/* Descriptor types */
#define USB_DEVICE_DESCRIPTOR_TYPE              (uint8_t)0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE       (uint8_t)0x02
#define USB_STRING_DESCRIPTOR_TYPE              (uint8_t)0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE           (uint8_t)0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE            (uint8_t)0x05
#define USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE    (uint8_t)0x06
#define USB_OTHER_SPEED_DESCRIPTOR_TYPE         (uint8_t)0x07
#define USB_POWER_DESCRIPTOR_TYPE               (uint8_t)0x08
#define USB_HID_DESCRIPTOR_TYPE                 (uint8_t)0x21
#define USB_REPORT_DESCRIPTOR_TYPE              (uint8_t)0x22
#define USB_PHYS_DESCRIPTOR_TYPE                (uint8_t)0x23
#define USB_AUDIO_IF_DESCRIPTOR_TYPE            (uint8_t)0x24
#define USB_AUDIO_EP_DESCRIPTOR_TYPE            (uint8_t)0x25
#define USB_HUB_DESCRIPTOR_TYPE                 (uint8_t)0x29

/* USB defined request codes
   See chapter 9 of the USB 1.1 specifcation for more information.
   Pg.187 Table 9.4 */
#if !defined(USB_REQUEST_GET_STATUS)
#define USB_REQUEST_GET_STATUS                  (uint8_t)0x00
#endif
#if !defined(USB_REQUEST_CLEAR_FEATURE)
#define USB_REQUEST_CLEAR_FEATURE               (uint8_t)0x01
#endif
#if !defined(USB_RESERVED_ONE)
#define USB_RESERVED_ONE                        (uint8_t)0x02
#endif
#if !defined(USB_REQUEST_SET_FEATURE)
#define USB_REQUEST_SET_FEATURE                 (uint8_t)0x03
#endif
#if !defined(USB_RESERVED_TWO)
#define USB_RESERVED_TWO                        (uint8_t)0x04
#endif
#if !defined(USB_REQUEST_SET_ADDRESS)
#define USB_REQUEST_SET_ADDRESS                 (uint8_t)0x05
#endif
#if !defined(USB_REQUEST_GET_DESCRIPTOR)
#define USB_REQUEST_GET_DESCRIPTOR              (uint8_t)0x06
#endif
#if !defined(USB_REQUEST_SET_DESCRIPTOR)
#define USB_REQUEST_SET_DESCRIPTOR              (uint8_t)0x07
#endif
#if !defined(USB_REQUEST_GET_CONFIGURATION)
#define USB_REQUEST_GET_CONFIGURATION           (uint8_t)0x08
#endif
#if !defined(USB_REQUEST_SET_CONFIGURATION)
#define USB_REQUEST_SET_CONFIGURATION           (uint8_t)0x09
#endif
#if !defined(USB_REQUEST_GET_INTERFACE)
#define USB_REQUEST_GET_INTERFACE               (uint8_t)0x0A
#endif
#if !defined(USB_REQUEST_SET_INTERFACE)
#define USB_REQUEST_SET_INTERFACE               (uint8_t)0x0B
#endif
#if !defined(USB_REQUEST_SYNC_FRAME)
#define USB_REQUEST_SYNC_FRAME                  (uint8_t)0x0C
#endif
/* defined USB device classes */
#define USB_DEVICE_CLASS_RESERVED               0x00
#define USB_DEVICE_CLASS_AUDIO                  0x01
#define USB_DEVICE_CLASS_COMMUNICATIONS         0x02
#define USB_DEVICE_CLASS_HUMAN_INTERFACE        0x03
#define USB_DEVICE_CLASS_MONITOR                0x04
#define USB_DEVICE_CLASS_PHYSICAL_INTERFACE     0x05
#define USB_DEVICE_CLASS_POWER                  0x06
#define USB_DEVICE_CLASS_PRINTER                0x07
#define USB_DEVICE_CLASS_STORAGE                0x08
#define USB_DEVICE_CLASS_HUB                    0x09
#define USB_DEVICE_CLASS_VENDOR_SPECIFIC        0xFF

/* Hub class request types */
#define USB_CLEAR_HUB_FEATURE                   0x20
#define USB_CLEAR_PORT_FEATURE                  0x23
#define USB_CLEAR_TT_BUFFER                     0x23
#define USB_GET_HUB_DESCRIPTOR                  0xA0
#define USB_GET_HUB_STATUS                      0xA0
#define USB_GET_PORT_STATUS                     0xA3
#define USB_RESET_TT                            0x23
#define USB_SET_HUB_DESCRIPTOR                  0x20
#define USB_SET_HUB_FEATURE                     0x20
#define USB_SET_PORT_FEATURE                    0x23
#define USB_GET_TT_STATE                        0xA3
#define USB_STOP_TT                             0x23

/* Hub request codes */
#define USB_HUB_REQUEST_CLEAR_TT_BUFFER         (uint8_t)0x08
#define USB_HUB_REQUEST_RESET_TT                (uint8_t)0x09
#define USB_HUB_REQUEST_GET_TT_STATE            (uint8_t)0x0A
#define USB_HUB_REQUEST_STOP_TT                 (uint8_t)0x0B

/* Hub feature selectors */
#define USB_C_HUB_LOCAL_POWER                   0x0001
#define USB_C_HUB_OVER_CURRENT                  0x0002
#define USB_PORT_CONNECTION                     0
#define USB_PORT_ENABLE                         1
#define USB_PORT_SUSPEND                        2
#define USB_PORT_OVER_CURRENT                   3
#define USB_PORT_RESET                          4
#define USB_HUB_PORT_POWER                      8
#define USB_HUB_PORT_LOW_SPEED                  9
#define USB_HUB_PORT_CONNECTION                 16
#define USB_HUB_PORT_ENABLE                     17
#define USB_HUB_PORT_SUSPEND                    18
#define USB_HUB_PORT_OVER_CURRENT               19
#define USB_HUB_PORT_RESET                      20
#define USB_HUB_PORT_TEST                       21
#define USB_HUB_PORT_INDICATOR                  22

/* USB defined Feature selectors */
#if !defined(USB_FEATURE_ENDPOINT_STALL)
#define USB_FEATURE_ENDPOINT_STALL              0x0000
#endif
#define USB_FEATURE_REMOTE_WAKEUP               0x0001
#define USB_FEATURE_POWER_D0                    0x0002
#define USB_FEATURE_POWER_D1                    0x0003
#define USB_FEATURE_POWER_D2                    0x0004
#define USB_FEATURE_POWER_D3                    0x0005

/* HID Class request values */
#define USBHID_REQUEST_GET_REPORT               (uint8_t)0x01
#define USBHID_REQUEST_GET_IDLE                 (uint8_t)0x02
#define USBHID_REQUEST_GET_PROTOCOL             (uint8_t)0x03
#define USBHID_REQUEST_SET_REPORT               (uint8_t)0x09
#define USBHID_REQUEST_SET_IDLE                 (uint8_t)0x0A
#define USBHID_REQUEST_SET_PROTOCOL             (uint8_t)0x0B

/* Set feature requests specific to HIGH speed devices */
#define USB_REQUEST_TEST_J                      0x0100
#define USB_REQUEST_TEST_K                      0x0200
#define USB_REQUEST_TEST_SE0_NAK                0x0300
#define USB_REQUEST_TEST_PACKET                 0x0400
#define USB_REQUEST_TEST_FORCE_ENABLE           0x0500

#define USB_HOST_TO_DEVICE      (uint8_t)0
#define USB_DEVICE_TO_HOST      (uint8_t)0x80
#define USB_DEVICE_TO_HOST_MASK (uint8_t)0x80
#define USB_RECIPIENT_DEVICE    (uint8_t)0x00
#define USB_RECIPIENT_INTERFACE (uint8_t)0x01
#define USB_RECIPIENT_ENDPOINT  (uint8_t)0x02
#define USB_RECIPIENT_OTHER     (uint8_t)0x03

#define USB_STANDARD_REQUEST    (uint8_t)0x00
#define USB_CLASS_REQUEST       (uint8_t)0x01
#define USB_VENDOR_REQUEST      (uint8_t)0x02
#define USB_RESERVED_REQUEST    (uint8_t)0x03

/******************************************************************************
Typedef definitions
******************************************************************************/
#if defined(__GNUC__) || defined(GRSAKURA)
#pragma pack(push)
#pragma pack(1)
#else
#pragma pack
#endif
/* USB Device descriptor */
typedef struct
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t wVendor;
    uint16_t wProduct;
    uint16_t wDevice;
    uint8_t bManufacturer;
    uint8_t bProduct;
    uint8_t bSerialNumber;
    uint8_t bNumConfigurations;

} USBDD,
*PUSBDD;

/* Values for Attributes field of an endpoint descriptor */
#define USB_ENDPOINT_TYPE_MASK                  0x03
#define USB_ENDPOINT_TYPE_CONTROL               0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS           0x01
#define USB_ENDPOINT_TYPE_BULK                  0x02
#define USB_ENDPOINT_TYPE_INTERRUPT             0x03
#define USB_ENDPOINT_TYPE_TX                    0x80
#define USB_ENDPOINT_TYPE_RX                    0x00
#define USB_ENDPOINT_TYPE_NOT_USED              0x00

/* USB end point descriptor */
typedef struct
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;

} USBEP,
*PUSBEP;

/* Definitions for bits in the Attributes field of a config. descriptor. */
#define USB_CONFIG_POWERED_MASK                 0xC0
#define USB_CONFIG_BUS_POWERED                  0x80
#define USB_CONFIG_SELF_POWERED                 0x40
#define USB_CONFIG_REMOTE_WAKEUP                0x20

/* Values for the Attributes Field in USB configuration descriptor */
#define BUS_POWERED                             0x80
#define SELF_POWERED                            0x40
#define REMOTE_WAKEUP                           0x20

/* USB configuration descriptor */
typedef struct
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t bConfiguration;
    uint8_t bAttributes;
    uint8_t bMaxPower;

} USBCG,
*PUSBCG;

/* USB interface descriptor */
typedef struct
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t bInterface;

} USBIF,
*PUSBIF;

/* USB String descriptor */
typedef struct
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bString[1];

} USBST,
*PUSBST;

/* USB power descriptor values */
#define USB_SUPPORT_D0_COMMAND      0x01
#define USB_SUPPORT_D1_COMMAND      0x02
#define USB_SUPPORT_D2_COMMAND      0x04
#define USB_SUPPORT_D3_COMMAND      0x08
#define USB_SUPPORT_D1_WAKEUP       0x10
#define USB_SUPPORT_D2_WAKEUP       0x20

/* USB power descriptor */
typedef struct
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bCapabilitiesFlags;
    uint16_t wEventNotification;
    uint16_t wD1LatencyTime;
    uint16_t wD2LatencyTime;
    uint16_t wD3LatencyTime;
    uint8_t bPowerUnit;
    uint16_t wD0PowerConsumption;
    uint16_t wD1PowerConsumption;
    uint16_t wD2PowerConsumption;

} USBPW,
*PUSBPW;

/* USB HID descriptor */
typedef struct
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bVersion[2];
    uint8_t bCountry;
    uint8_t bNumHID;
    uint8_t bDescType;
    uint8_t bDescLengthLo;
    uint8_t bDescLengthHi;

} USBHD,
*PUSBHD;

/* USB Common descriptor */
typedef struct
{
    uint8_t bLength;
    uint8_t bDescriptorType;

} USBCO,
*PUSBCO;

/* USB descriptor union */
typedef union
{
    USBCO Common;
    USBDD Device;
    USBCG Configuration;
    USBIF Interface;
    USBEP Endpoint;
    USBST String;
    USBPW Power;
    USBHD HID;
} USBDESC,
*PUSBDESC;

/* Standard USB HUB definitions. See USB Specification Chapter 11 */
typedef struct _USB_HUB_DESCRIPTOR
{
    uint8_t bDescriptorLength;       /* Length of this descriptor */
    uint8_t bDescriptorType;         /* Hub configuration type */
    uint8_t bNumberOfPorts;          /* number of ports on this hub */
    uint16_t wHubCharacteristics;     /* Hub Charateristics */
    uint8_t bPowerOnToPowerGood;     /* port power on till power good in 2ms */
    uint8_t bHubControlCurrent;      /* max current in mA */
    /* room for 255 ports power control and removable bitmask */
    uint8_t bRemoveAndPowerMask[64];
} USBRH,
*PUSBRH;

/******************************************************************************
* >>>>>>>>>>>>> Structures defined by the control pipe protocol <<<<<<<<<<<<  *
******************************************************************************/

/* Structure of Request type */
typedef union
{
    uint8_t        bmRequestType;
    struct {
        #ifdef _BITFIELDS_MSB_FIRST_
        uint8_t    Direction   :1;     /* MSB */
        uint8_t    Type        :2;
        uint8_t    Recipient   :5;     /* Low Nibble */
        #else
        uint8_t    Recipient   :5;     /* Low Nibble */
        uint8_t    Type        :2;
        uint8_t    Direction   :1;     /* MSB */
        #endif
    } Field;
} USBRQ,
*PUSBRQ;

/* USB device requests structures */
typedef struct
{
    uint8_t    bmRequestType;
    uint8_t    bRequest;
    uint16_t    wValue;
    uint16_t    wIndex;
    uint16_t    wLength;
} USBDR,
*PUSBDR;

/* Structure of the Index */
typedef union
{
    uint16_t        wIndex;
    struct {
        #ifdef _BITFIELDS_MSB_FIRST_
        uint16_t    Reserved1   :8;
        uint16_t    Direction   :1;
        uint16_t    Reserved2   :3;
        uint16_t    Number      :4;
        #else
        uint16_t    Number      :4;
        uint16_t    Reserved2   :3;
        uint16_t    Direction   :1;
        uint16_t    Reserved1   :8;
        #endif
    } Endpoint;
    struct {
        #ifdef _BITFIELDS_MSB_FIRST_
        uint16_t    Reserved    :8;
        uint16_t    Number      :8;
        #else
        uint16_t    Number      :8;
        uint16_t    Reserved    :8;
        #endif
    } Interface;
} USBINDEX,
*PUSBINDEX;

/* Return structure for Get status */
typedef union
{
    uint8_t        bReturn[sizeof(uint16_t)];
    uint16_t        Word;
    struct {                    /* Bits for device Status */
        #ifdef _BITFIELDS_MSB_FIRST_
        uint16_t    Reserved:14;
        uint16_t    Remote_Wakeup:1;
        uint16_t    Self_Powered:1;
        #else
        uint16_t    Self_Powered:1;
        uint16_t    Remote_Wakeup:1;
        uint16_t    Reserved:14;
        #endif
    } Device;
    struct {                    /* Bits for end point status */
        #ifdef _BITFIELDS_MSB_FIRST_
        uint16_t    Reserved:15;
        uint16_t    Halt:1;
        #else
        uint16_t    Halt:1;
        uint16_t    Reserved:15;
        #endif
    } Endpoint;
} USBGETSTATUS,
*PUSBGETSTATUS;

#if defined(__GNUC__) || defined(GRSAKURA)
#pragma pack(pop)
#else
#pragma unpack
#endif

#endif /* USB110_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
