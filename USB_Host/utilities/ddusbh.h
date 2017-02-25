/* INSERT LICENSE HERE */

#ifndef DDUSBH_H_INCLUDED
#define DDUSBH_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include <string.h>

#include "../usbhost_typedefine.h"

#include "../usb110.h"

#include "../r_usbh_driver.h"


/******************************************************************************
Macro definitions
******************************************************************************/

#define USBH_MAX_STRING_LENGTH       256

#define USBH_HUB_HIGH_SPEED_DEVICE   BIT_10
#define USBH_HUB_LOW_SPEED_DEVICE    BIT_9
#define USBH_HUB_PORT_POWER          BIT_8
#define USBH_HUB_PORT_RESET          BIT_4
#define USBH_HUB_PORT_OVER_CURRENT   BIT_3
#define USBH_HUB_PORT_SUSPEND        BIT_2
#define USBH_HUB_PORT_ENABLED        BIT_1
#define USBH_HUB_PORT_CONNECT_STATUS BIT_0

#define USBF_DB(b)                   (uint8_t)b
#ifdef _USB_SWAP_WORDS_
#define USBF_DW(w)                   (uint8_t)(w >> 8),(uint8_t)w
#else
#define USBF_DW(w)                   (uint8_t)w,(uint8_t)(w >> 8)
#endif

#ifdef _USB_SWAP_WORDS_
#define USB_SET_VALUE(h,l)           (uint16_t)(((uint16_t)l << 8) | h)
#define USB_SET_INDEX(h,l)           (uint16_t)(((uint16_t)l << 8) | h)
#else
#define USB_SET_VALUE(h,l)           (uint16_t)(((uint16_t)h << 8) | l)
#define USB_SET_INDEX(h,l)           (uint16_t)(((uint16_t)h << 8) | l)
#endif

/******************************************************************************
Typedef definitions
******************************************************************************/

/* Error Codes */
typedef enum _USBEC
{
    USBH_NO_ERROR = 0,              /* 0 */
    USBH_CRC_ERROR,                 /* 1 */
    USBH_BIT_STUFFING_ERROR,        /* 2 */
    USBH_DATA_PID_MISS_MATCH_ERROR, /* 3 */
    USBH_HALTED_ERROR,              /* 4 */
    USBH_NOT_RESPONDING_ERROR,      /* 5 */
    USBH_PID_CHECK_ERROR,           /* 6 */
    USBH_UNEXPECTED_PID_ERROR,      /* 7 */
    USBH_DATA_OVERRUN_ERROR,        /* 8 */
    USBH_DATA_UNDERRUN_ERROR,       /* 9 */
    USBH_BABBLE_ERROR,              /* 10 */
    USBH_XACT_ERROR,                /* 11 */
    USBH_BUFFER_OVERRUN_ERROR,      /* 12 */
    USBH_BUFFER_UNDERRUN_ERROR,     /* 13 */
    USBH_FIFO_WRITE_ERROR,          /* 14 */
    USBH_FIFO_READ_ERROR            /* 15 */
} USBEC;

/* This is an extension of the above error codes which are generic
   to USB transactions with extensions for the standard device request */
typedef enum _REQERR
{
    REQ_NO_ERROR = 0,
    REQ_CRC_ERROR,
    REQ_BIT_STUFFING_ERROR,
    REQ_DATA_PID_MISS_MATCH_ERROR,
    REQ_STALL_ERROR,
    REQ_NOT_RESPONDING_ERROR,
    REQ_PID_CHECK_ERROR,
    REQ_UNEXPECTED_PID_ERROR,
    REQ_DATA_OVERRUN_ERROR,
    REQ_DATA_UNDERRUN_ERROR,
    REQ_USBH_BABBLE_ERROR,
    REQ_USBH_XACT_ERROR,
    REQ_BUFFER_OVERRUN_ERROR,
    REQ_BUFFER_UNDERRUN_ERROR,
    REQ_FIFO_WRITE_ERROR,
    REQ_FIFO_READ_ERROR,
    REQ_INVALID_PARAMETER,
    REQ_SETUP_PHASE_TIME_OUT,
    REQ_DATA_PHASE_TIME_OUT,
    REQ_STATUS_PHASE_TIME_OUT,
    REQ_ENDPOINT_NOT_FOUND,
    REQ_DEVICE_NOT_FOUND,
    REQ_INVALID_ADDRESS,
    REQ_IDLE_TIME_OUT,
    REQ_SIGNAL_CREATE_ERROR,
    REQ_PENDING
} REQERR;

/* Pipe assignment */
typedef enum _PASGN
{
    PIPE_NOT_ASSIGNED = 0,
    PIPE_IN,
    PIPE_OUT
    
} PASGN;

typedef struct _USBHI *PUSBHI;      /* Pointer to the hub information */
typedef struct _USBPI *PUSBPI;      /* Pointer to the port information */
typedef struct _USBDI *PUSBDI;      /* Pointer to the device information */
typedef struct _USBEI *PUSBEI;      /* Pointer to the endpoint information */
typedef struct _USBTR *PUSBTR;      /* Pointer to a transfer request */
typedef struct _USBHC *PUSBHC;      /* Pointer to the Host Controller data */

/* Hub information */
typedef struct _USBHI
{
    PUSBPI   pPort;                 /* Pointer to the port that the hub is
                                       attached to */
    uint16_t wCharacteristics;      /* The characteristics as described in
                                       the USB Spec. */
    uint16_t wPowerOn2PowerGood_mS; /* The power on time in mS */
    uint8_t  byNumberOfPorts;       /* The number of ports on this hub */
    uint8_t  byHubAddress;          /* The address of the hub */
    uint8_t  byHubCurrent_mA;       /* The hub current in mA */
    _Bool    bfBusPowered;          /* The power state of the hub */
    _Bool    bfOverCurrent;         /* The over current flag */
    _Bool    bfAllocated;           /* true if allocated */

} USBHI;

/* Root port control functions */
typedef struct _USBPC
{
    /* The functions to control a root port */
    void (*const Reset)(_Bool bfState);
    void (*const Enable)(_Bool bfState);
    void (*const Suspend)(_Bool bfState);
    uint32_t (*const GetStatus)(void);
    void (*const Power)(_Bool bfState);
    /* The index of this port as used by the Host Controller in
       the DEVADDn register RTPORT field. If this field is missing
       then it should be set to 0 */
    uint32_t    uiPortIndex;
    /* Pointer to the Host Controller to which the port is attached */
    PUSB        pUSB;
} USBPC,
*PUSBPC;

/* Port Information */
typedef struct _USBPI
{
    PUSBPI   pNext;                 /* Pointer to the next port */
    PUSBPC   pRoot;                 /* Pointer to root-port control
                                       functions */
    PUSBHI   pHub;                  /* Pointer to the hub this port is on */
    PUSBDI   pDevice;               /* Pointer to a device attached to the port
                                       NULL if no device attached */
    uint32_t uiPortIndex;           /* The index of the port */
    uint32_t dwPortStatus;          /* The current status of the port */
    PUSB     pUSB;                  /* Pointer to the Host Controller to which 
                                       the port is attached */
    PUSBHC   pUsbHc;                /* Pointer to the host controller data */
    _Bool    bfAllocated;           /* true if allocated */
} USBPI;

/* Device Information */
typedef struct _USBDI
{
    PUSBPI   pPort;                 /* Pointer to the port information */
    PUSBEI   pEndpoint;             /* Pointer to the next endpoint */
    PUSBEI   pControlSetup;         /* Pointer to the control SETUP endpoint */
    PUSBEI   pControlOut;           /* Pointer to the control OUT endpoint */
    PUSBEI   pControlIn;            /* Pointer to the control IN endpoint */
    PUSBHI   pHub;                  /* Pointer to the hub information if this
                                       device is a hub */
                                    /* The device strings */
    int8_t   pszSymbolicLinkName[USBH_MAX_STRING_LENGTH];
    int8_t   pszManufacturer[USBH_MAX_STRING_LENGTH];
    int8_t   pszProduct[USBH_MAX_STRING_LENGTH];
    int8_t   pszSerialNumber[USBH_MAX_STRING_LENGTH];
    uint16_t wVID;                  /* The Vendor ID */
    uint16_t wPID;                  /* The product ID */
    uint16_t wDeviceVersion;        /* The device version */
    uint8_t  byNumberOfEndpoints;   /* The number of endpoints (including 0) */
    uint8_t  byAddress;             /* The device address */
    _Bool    bfConfigured;          /* true to show that the device is
                                       configured */
    uint8_t  byConfigurationValue;  /* The configuration value */
    uint8_t  byInterfaceNumber;     /* The current Interface */
    uint8_t  byAlternateSetting;    /* The current alternate interface
                                       setting */
    uint8_t  byInterfaceClass;      /* The Interface class (0xFF for vendor
                                       specific) */
    uint8_t  byInterfaceSubClass;   /* The sub class */
    uint8_t  byInterfaceProtocol;   /* The protocol code */
    USBTS    transferSpeed;         /* The Transfer speed */
    uint16_t wMaxPower_mA;          /* The maximum power in mA */
    _Bool    bfAllocated;           /* true if allocated */
} USBDI;

/* Endpoint Information */
typedef struct _USBEI
{
    PUSBEI   pNext;                 /* Pointer to the next endpoint */
    PUSBDI   pDevice;               /* Pointer to the device */
    uint16_t wPacketSize;           /* The endpoint packet size */
    uint8_t  byEndpointNumber;      /* The device endpoint number */
    uint8_t  byInterval;            /* The polling interval for interrupt
                                       transfers */
    USBTT    transferType;          /* The type of transfer */
    USBDIR   transferDirection;     /* The direction of transfer */
                                    /* UPDATED at the end of each
                                       transaction */
    USBDP    dataPID;               /* The DATA0/1 PID */
    _Bool    bfAllocated;           /* true if allocated */
} USBEI;

/* Define the structure of the data used by the host controller */
typedef struct _USBHC
{
    /* Pointers to a lists of hubs, ports and devices attached */
    PUSBPI  pPort;
    /* The transfer lists */
    PUSBTR  pControl;
    PUSBTR  pCurrentControl;
    PUSBTR  pInterrupt;
    PUSBTR  pBulk;
    PUSBTR  pIsochronus;
    /* A structure of data used to keeps track of pipe activity so the 
       idle time-out can be implemeted by the host driver and USB stack */
    struct
    {
        /* When a bulk transfer is using the FIFO this is incremented on each
           successful FIFO access to show activity */
        int      iFifoUsedCount;
        /* When a bulk transfer is performed by DMA this is used to hold the
           last value of the transaction counter to show activity */
        uint32_t ulDmaTransCnt;
        /* When an OUT DMA is used to transfer data on a pipe this is set true
           and is cleared to zero when the transfer is complete */
        _Bool    bfTerminateOutDma;
        /* When an IN DMA is used to transfer data on a pipe this is set true
           and is cleared to zero when the transfer is complete */
        _Bool    bfTerminateInDma;
    } pPipeTrack[USBH_MAX_NUM_PIPES];
    /* The pipe assignment information - entry for pipe 0 is never used as
       the function is fixed to that of the control pipe */
    struct _EPASSIGN
    {
        /* A record of the transfer request to which this pipe was assigned */
        PUSBTR   pRequest;
        /* A cache of the last device to re-assign the same pipe if possible */
        PUSBEI   pEndpointCache;
        /* Flag to show that it is in use for a transfer */
        _Bool    bfAllocated;
        /* The previous assignement of this pipe */
        PASGN    pipeAssign;
    /* The Host Controller Peripheral has 9 pipes 1 to 9 which can be
       allocated */
    } pEndpointAssign[USBH_MAX_NUM_PIPES];
    /* Generic pointer for interface specific use */
    void    *pvInterface;
} USBHC;

/* Define the structure of an Isochronous endpoint packet size variation
   This is only used for isochronous transfers where the packet size
   must vary during the transfer to maintain an average sample rate. */
typedef struct _USBIV
{
    uint16_t *pwPacketSizeList;     /* Pointer to an array of packet sizes */
    int      iScheduleIndex;        /* The current index into the array */
    int      iListLength;           /* The number elements in the array */
    uint16_t wPacketSize;           /* The packet size used on the last
                                       transfer */
} USBIV,
*PUSBIV;

/* Define the data required to signal completion of IO */
typedef struct _USBTC
{
    /* This can be replaced by anything requred by the system to
       signal the completion of the IO */
    void    *pvComplete;
} USPTC,
*PUSBTC;

/* Define the structure of a USB Transfer Request */
typedef struct _USBTR
{
    PUSBTR   pNext;                 /* List pointer */
    PUSB     pUSB;                  /* Pointer to the Host Controller to which 
                                       the port is attached */
    PUSBHC   pUsbHc;                /* Pointer to the host controller data */
    PUSBEI   pEndpoint;             /* Pointer to endpoint information */
                                    /* INTERNAL */
    void     *pInternal;            /* Pointer to any host driver specific
                                       data required for processing the 
									   transfer */
    void     (*pCancel)(PUSBTR);    /* Pointer to a host driver specific
                                       function to cancel a transfer */
    _Bool    bfInProgress;          /* Flag to show that the request needs to 
                                       be processed */
    size_t   stTransferSize;        /* The size of the last transfer made by 
                                       the hardware driver*/
    uint32_t dwIdleTime;            /* Idle time in mS */
                                    /* IN */
    uint8_t  *pMemory;              /* A pointer to the memory to transfer */
    size_t   stIdx;                 /* The current index during data
                                       transfer */
    size_t   stLength;              /* The length of the memory */
    uint32_t dwIdleTimeOut;         /* An transfer idle time-out in mS */
    PUSBIV   pIsocPacketSize;       /* Pointer to the isochronous packet size
                                       schedule */
                                    /* OUT */
    uint32_t uiTransferLength;      /* The number of bytes transfered */
    USBEC    errorCode;             /* The error code */
    USPTC    ioSignal;              /* Signal object for the completion of the
                                       transfer */
} USBTR;

#endif /* DDUSBH_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
