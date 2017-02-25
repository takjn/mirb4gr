/* INSERT LICENSE HERE */

#ifndef USBHDEVICEAPI_H_INCLUDED
#define USBHDEVICEAPI_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "./utilities/sysif.h"

/******************************************************************************
Macro definitions
******************************************************************************/

#define REQ_IDLE_TIME_OUT_INFINITE  -1UL

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
Function Name: usbhGetDevice
Description:   Function to get a device pointer
Arguments:     IN  pszLinkName - Pointer to the device link name
Return value:  Pointer to the device or NULL for invalid parameter
******************************************************************************/

extern PUSBDI usbhGetDevice(int8_t * pszLinkName);

/******************************************************************************
Function Name: usbhGetEndpoint
Description:   Function to open a device endpoint
Arguments:     IN  pDevice - The device to get
               IN  byEndpoint - The endpoint index to open
Return value:  Pointer to the endpoint or NULL if not found
******************************************************************************/

extern  PUSBEI usbhGetEndpoint(PUSBDI pDevice, uint8_t byEndpoint);

/******************************************************************************
Function Name: usbhStartTransfer
Description:   Function to start data transfer on an endpoint
Arguments:     IN  pDevice - Pointer to the device
               IN  pRequest -Pointer to the transfer request struct
               IN  pEndpoint - Pointer to the endpoint to use
               IN  pMemory - Pointer to the memory to transfer
               IN  stLength - The length of memory to transfer
               IN  dwIdleTimeOut - The request idle time out in mS
Return value:  true if the request was added
NOTE: Requires mutually exclusive access to protect list
******************************************************************************/

extern  _Bool usbhStartTransfer(PUSBDI   pDevice,
                                PUSBTR   pRequest,
                                PUSBEI   pEndpoint,
                                uint8_t *pMemory,
                                size_t   stLength,
                                uint32_t dwIdleTimeOut);

/******************************************************************************
Function Name: usbhIsocTransfer
Description:   Function to start an isochronous transfer with a packet size
               variation schedule
Arguments:     IN  pDevice - Pointer to the device
               IN  pRequest -Pointer to the transfer request struct
               IN  pIsocPacketSize - Pointer to the isoc packet size variation
                                     data
               IN  pMemory - Pointer to the memory to transfer
               IN  stLength - The length of memory to transfer
               IN  dwIdleTimeOut - The request idle time out in mS
Return value:  true if the request was added
NOTE: Requires mutually exclusive access to protect lists
******************************************************************************/

extern  _Bool usbhIsocTransfer(PUSBDI   pDevice,
                               PUSBTR   pRequest,
                               PUSBIV   pIsocPacketSize,
                               PUSBEI   pEndpoint,
                               uint8_t  *pMemory,
                               size_t   stLength,
                               uint32_t dwIdleTimeOut);

/******************************************************************************
Function Name: usbhCancelTransfer
Description:   Function to cancel a transfer
Arguments:     IN  pRequest - Pointer to the request to cancel
Return value:  true if the request was cancelled
NOTE: Requires mutually exclusive access to protect list
******************************************************************************/

extern  _Bool usbhCancelTransfer(PUSBTR pRequest);

/******************************************************************************
Function Name: usbhCancelAllControlRequests
Description:   Function to cancel all control requests for a device
Arguments:     IN  pDevice - Pointer to the device to cancel all control
                             requests
Return value:  The number of requests cancelled
******************************************************************************/

extern  int usbhCancelAllControlRequests(PUSBDI pDevice);

/******************************************************************************
Function Name: usbhDeviceRequest
Description:   Function to send a device request
Arguments:     IN  pRequest - Pointer to the request
               IN  pDevice - Pointer to the device
               IN  bmRequestType - The request type
               IN  bRequest - The request
               IN  wValue - The Value
               IN  wIndex - The Index
               IN  wLength - The length of the data
               IN/OUT pbyData - Pointer to the data
Return value:  0 for success or error code
******************************************************************************/

extern  REQERR usbhDeviceRequest(PUSBTR    pRequest,
                                 PUSBDI    pDevice,
                                 uint8_t   bmRequestType,
                                 uint8_t   bRequest,
                                 uint16_t  wValue,
                                 uint16_t  wIndex,
                                 uint16_t  wLength,
                                 uint8_t  *pbyData);

/******************************************************************************
Function Name: usbhCancelAllTransferReqests
Description:   Function to cancel all transfer requests for a device
Arguments:     IN  pDevice - Pointer to the device to cancel all transfer
                             requests
Return value:  The number of requests cancelled
******************************************************************************/

extern  int usbhCancelAllTransferReqests(PUSBDI pDevice);

/******************************************************************************
Function Name: usbhTransferInProgress
Description:   Function to find out if the request is in progress
Arguments:     IN  pRequest - Pointer to the request
Return value:  true if the transfer is in progress
******************************************************************************/

extern  _Bool usbhTransferInProgress(PUSBTR pRequest);

#ifdef __cplusplus
}
#endif

#endif /* USBHDEVICEAPI_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
