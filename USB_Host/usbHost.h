/* INSERT LICENSE HERE */

#if !defined(_usb_h_) || defined(USBCORE_H)
#error "Never include UsbCore.h directly; include Usb.h instead"
#else
#define USBCORE_H

#include <stdint.h>
#include <stdbool.h>
#include "iodefine.h"

#include <Arduino.h>
#include <assert.h>
#include "usbhost_typedefine.h"

#include "usb110.h"
#include "./utilities/ddusbh.h"
#include "usbhDriverInternal.h"

#define USBH_HUB_HIGH_SPEED_DEVICE   BIT_10
#define USBH_HUB_LOW_SPEED_DEVICE    BIT_9
#define USBH_HUB_PORT_POWER          BIT_8
#define USBH_HUB_PORT_RESET          BIT_4
#define USBH_HUB_PORT_OVER_CURRENT   BIT_3
#define USBH_HUB_PORT_SUSPEND        BIT_2
#define USBH_HUB_PORT_ENABLED        BIT_1
#define USBH_HUB_PORT_CONNECT_STATUS BIT_0


// DEFINITIONS ****************************************************************/
#define bmHUBPRE        0x04

#define USB_XFER_TIMEOUT        10000 //30000    // (5000) USB transfer timeout in milliseconds, per section 9.2.6.1 of USB 2.0 spec
//#define USB_NAK_LIMIT     32000   //NAK limit for a transfer. 0 means NAKs are not counted
#define USB_RETRY_LIMIT     3       // 3 retry limit for a transfer
#define USB_SETTLE_DELAY    200     //settle delay in milliseconds

#define USB_NUMDEVICES      10  //number of USB devices
//#define HUB_MAX_HUBS      7   // maximum number of hubs that can be attached to the host controller
#define HUB_PORT_RESET_DELAY    20  // hub port reset delay 10 ms recomended, can be up to 20 ms


/* Common setup data constant combinations  */
#define bmREQ_GET_DESCR     USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_DEVICE     //get descriptor request type
#define bmREQ_SET           USB_SETUP_HOST_TO_DEVICE|USB_SETUP_TYPE_STANDARD|USB_SETUP_RECIPIENT_DEVICE     //set request type for all but 'set feature' and 'set interface'
#define bmREQ_CL_GET_INTF   USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_INTERFACE     //get interface request type
#define bmREQ_CL_GET_ENDP   USB_SETUP_DEVICE_TO_HOST|USB_SETUP_TYPE_CLASS|USB_SETUP_RECIPIENT_ENDPOINT      //get endpoint descriptor

// USB Device Classes
#define USB_CLASS_USE_CLASS_INFO    0x00    // Use Class Info in the Interface Descriptors
#define USB_CLASS_AUDIO         0x01    // Audio
#define USB_CLASS_COM_AND_CDC_CTRL  0x02    // Communications and CDC Control
#define USB_CLASS_HID           0x03    // HID
#define USB_CLASS_PHYSICAL      0x05    // Physical
#define USB_CLASS_IMAGE         0x06    // Image
#define USB_CLASS_PRINTER       0x07    // Printer
#define USB_CLASS_MASS_STORAGE      0x08    // Mass Storage
#define USB_CLASS_HUB           0x09    // Hub
#define USB_CLASS_CDC_DATA      0x0a    // CDC-Data
#define USB_CLASS_SMART_CARD        0x0b    // Smart-Card
#define USB_CLASS_CONTENT_SECURITY  0x0d    // Content Security
#define USB_CLASS_VIDEO         0x0e    // Video
#define USB_CLASS_PERSONAL_HEALTH   0x0f    // Personal Healthcare
#define USB_CLASS_DIAGNOSTIC_DEVICE 0xdc    // Diagnostic Device
#define USB_CLASS_WIRELESS_CTRL     0xe0    // Wireless Controller
#define USB_CLASS_MISC          0xef    // Miscellaneous
#define USB_CLASS_APP_SPECIFIC      0xfe    // Application Specific
#define USB_CLASS_VENDOR_SPECIFIC   0xff    // Vendor Specific

// Additional Error Codes
#define USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED   0xD1
#define USB_DEV_CONFIG_ERROR_DEVICE_INIT_INCOMPLETE 0xD2
#define USB_ERROR_UNABLE_TO_REGISTER_DEVICE_CLASS   0xD3
#define USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL      0xD4
#define USB_ERROR_HUB_ADDRESS_OVERFLOW          0xD5
#define USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL     0xD6
#define USB_ERROR_EPINFO_IS_NULL            0xD7
#define USB_ERROR_INVALID_ARGUMENT          0xD8
#define USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE     0xD9
#define USB_ERROR_INVALID_MAX_PKT_SIZE          0xDA
#define USB_ERROR_EP_NOT_FOUND_IN_TBL           0xDB
#define USB_ERROR_CONFIG_REQUIRES_ADDITIONAL_RESET      0xE0
#define USB_ERROR_FailGetDevDescr                       0xE1
#define USB_ERROR_FailSetDevTblEntry                    0xE2
#define USB_ERROR_FailGetConfDescr                      0xE3
#define USB_ERROR_SET_DEVICE_ADDRESS_CFG_FAIL           0xE4
#define USB_ERROR_EP0_NOT_USED_FOR_CTRLREQ              0xE5
#define USB_ERROR_TRANSFER_TIMEOUT          0xFF


#define SE0     0
#define JSTATE  1
#define KSTATE  2
#define SE1     3

#define USB_POWERED_STATE   0
#define USB_BUS_RESET_STATE 4
#define USB_BUS_LOW_SPEED   1
#define USB_BUS_HIGH_SPEED  2

#define USB_STATE_DETACHED                                  0x10
#define USB_DETACHED_SUBSTATE_INITIALIZE                    0x11
#define USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE               0x12
#define USB_DETACHED_SUBSTATE_ILLEGAL                       0x13
#define USB_ATTACHED_SUBSTATE_SETTLE                        0x20
#define USB_ATTACHED_SUBSTATE_RESET_DEVICE                  0x30
#define USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE           0x40
#define USB_ATTACHED_SUBSTATE_WAIT_SOF                      0x50
#define USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE    0x60
#define USB_STATE_ADDRESSING                                0x70
#define USB_STATE_CONFIGURING                               0x80
#define USB_STATE_RUNNING                                   0x90
#define USB_STATE_ERROR                                     0xa0
#define USB_STATE_MASK                                      0xf0

/* Host transfer token values for writing the HXFR register (R30)   */
/* OR this bit field with the endpoint number in bits 3:0               */
#define tokSETUP  0x10  // HS=0, ISO=0, OUTNIN=0, SETUP=1
#define tokIN     0x00  // HS=0, ISO=0, OUTNIN=0, SETUP=0
#define tokOUT    0x20  // HS=0, ISO=0, OUTNIN=1, SETUP=0
#define tokINHS   0x80  // HS=1, ISO=0, OUTNIN=0, SETUP=0
#define tokOUTHS  0xA0  // HS=1, ISO=0, OUTNIN=1, SETUP=0
#define tokISOIN  0x40  // HS=0, ISO=1, OUTNIN=0, SETUP=0
#define tokISOOUT 0x60  // HS=0, ISO=1, OUTNIN=1, SETUP=0

/* Host error result codes, the 4 LSB's in the HRSL register */
#define hrSUCCESS   0x00
#define hrBUSY      0x01
#define hrBADREQ    0x02
#define hrUNDEF     0x03
#define hrNAK       0x04
#define hrSTALL     0x05
#define hrTOGERR    0x06
#define hrWRONGPID  0x07
#define hrBADBC     0x08
#define hrPIDERR    0x09
#define hrPKTERR    0x0A
#define hrCRCERR    0x0B
#define hrKERR      0x0C
#define hrJERR      0x0D
#define hrTIMEOUT   0x0E
#define hrBABBLE    0x0F

#define NUM_PIPES   (10)
// DECLARATIONS ***************************************************************/

/* USB Setup Packet Structure   */
typedef struct {

        union { // offset   description
                uint8_t bmRequestType; //   0      Bit-map of request type

                struct {
                        uint8_t recipient : 5; //          Recipient of the request
                        uint8_t type : 2; //          Type of request
                        uint8_t direction : 1; //          Direction of data X-fer
                } __attribute__((packed));
        } ReqType_u;
        uint8_t bRequest; //   1      Request

        union {
                uint16_t wValue; //   2      Depends on bRequest

                struct {
                        uint8_t wValueLo;
                        uint8_t wValueHi;
                } __attribute__((packed));
        } wVal_u;
} __attribute__((packed)) SETUP_PKT, *PSETUP_PKT;

class USBDeviceConfig {
public:
        virtual ~USBDeviceConfig(){}; //added to remove warning in GRSAKURA

        virtual uint8_t Init(uint8_t parent, uint8_t port, bool lowspeed) {
                return 0;
        }

        virtual uint8_t ConfigureDevice(uint8_t parent, uint8_t port, bool lowspeed) {
                return 0;
        }

        virtual uint8_t Release() {
                return 0;
        }

        virtual uint8_t Poll() {
                return 0;
        }

        virtual uint8_t GetAddress() {
                return 0;
        }

        virtual void ResetHubPort(uint8_t port) {
                return;
        } // Note used for hubs only!

        virtual boolean VIDPIDOK(uint16_t vid, uint16_t pid) {
                return false;
        }

        virtual boolean DEVCLASSOK(uint8_t klass) {
                return false;
        }

        virtual boolean DEVSUBCLASSOK(uint8_t subklass) {
                return true;
        }

};

// Base class for incoming data parser

class USBReadParser {
public:
        virtual ~USBReadParser(){};//added to remove warning in GRSAKURA
        virtual void Parse(const uint16_t len, const uint8_t *pbuf, const uint16_t &offset) = 0;
};

typedef enum {
        vbus_on = 1,
        vbus_off = 0
} VBUS_t;

/**
 * Provides USB Host functionality.
 * @author  E King
 * @version 1.00
 * @since   1.05
 */
class USB
{

    private:
        /** This module's name. */
        static const char *MODULE;
        static bool initialized;

        AddressPoolImpl<USB_NUMDEVICES> addrPool;
        USBDeviceConfig* devConfig[USB_NUMDEVICES];
        uint8_t bmHubPre;

        /* In:OK;Tested:OK*/ uint8_t SetAddress(uint8_t addr, uint8_t ep, EpInfo **ppep, uint16_t &nak_limit);
        /*FIXME*/ uint8_t OutTransfer(EpInfo *pep, uint16_t nak_limit, uint16_t nbytes, uint8_t *data);
        /*FIXME*/ uint8_t InTransfer(EpInfo *pep, uint16_t nak_limit, uint16_t *nbytesptr, uint8_t *data);
        /*FIXME*/ uint8_t AttemptConfig(uint8_t driver, uint8_t parent, uint8_t port, bool lowspeed);

        /*In:NN;NotNeeded*/   uint8_t dispatchPkt(uint8_t token, uint8_t ep, uint16_t nak_limit);
    public:
        /**
         * Default constructor.
         */
        USB();
        /**
         * Default destructor.
         */
        ~USB();
        /**
         * Polls connected USB devices for updates to their status.
         */
        void Task();

        int8_t Init(void)
        {
                return 0;
        }

        void SetHubPreMask() {
            bmHubPre |= bmHUBPRE;
        };

        void ResetHubPreMask() {
            bmHubPre &= (~bmHUBPRE);
        };
        void vbusPower(VBUS_t state);

        uint8_t RegisterDeviceClass(USBDeviceConfig *pdev) {
            for(uint8_t i = 0; i < USB_NUMDEVICES; i++) {
                if(!devConfig[i]) {
                    devConfig[i] = pdev;
                    return 0;
                }
            }
            return USB_ERROR_UNABLE_TO_REGISTER_DEVICE_CLASS;
        };

        AddressPool& GetAddressPool() {
            return (AddressPool&)addrPool;
        };

        void ForEachUsbDevice(UsbDeviceHandleFunc pfunc) {
            addrPool.ForEachUsbDevice(pfunc);
        };

        uint8_t getUsbTaskState(void);
        void setUsbTaskState(uint8_t state);

        /*In:OK*/   EpInfo* getEpInfoEntry(uint8_t addr, uint8_t ep);
        /*In:OK*/   uint8_t setEpInfoEntry(uint8_t addr, uint8_t epcount, EpInfo* eprecord_ptr);

        /* Control requests */
        /*In:OK:Tested:OK*/   uint8_t getDevDescr(uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t* dataptr);
        /*In:OK:Tested:OK*/   uint8_t getConfDescr(uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t conf, uint8_t* dataptr);

        /*In:OK;Tested:OK*/   uint8_t getConfDescr(uint8_t addr, uint8_t ep, uint8_t conf, USBReadParser *p);
        /*In:NT*/   uint8_t getIntfDescr(uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t conf, uint8_t* dataptr);
        /*In:NT*/   uint8_t getEndpDescr(uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t conf, uint8_t* dataptr);
        /*In:OK;NeverUsed*/   uint8_t getStrDescr(uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t index, uint16_t langid, uint8_t* dataptr);
        /*In:OK;Tested:OK*/   uint8_t setAddr(uint8_t oldaddr, uint8_t ep, uint8_t newaddr);
        /*In:OK;Tested:OK*/   uint8_t setConf(uint8_t addr, uint8_t ep, uint8_t conf_value);

        /*In:OK*/   uint8_t inTransfer(uint8_t addr, uint8_t ep, uint16_t *nbytesptr, uint8_t* data);
        /*In:OK*/   uint8_t outTransfer(uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t* data);


        /*In:OK;Tested:OK*/   uint8_t DefaultAddressing(uint8_t parent, uint8_t port, bool lowspeed);
        /*In:OK;Tested:OK*/   uint8_t Configuring(uint8_t parent, uint8_t port, bool lowspeed);
        /*In:OK*/   uint8_t ReleaseDevice(uint8_t addr);


        /*In:OK;Tested:OK*/      uint8_t ctrlReq(uint8_t addr, uint8_t ep, uint8_t bmReqType, uint8_t bRequest, uint8_t wValLo, uint8_t wValHi,
                        uint16_t wInd, uint16_t total, uint16_t nbytes, uint8_t* dataptr, USBReadParser *p);
        /**
         * Initialise the USB Host class.
         */
         void initialise();
};

#endif // USB_HOST_HPP_
