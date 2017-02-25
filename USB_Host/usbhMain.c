/* INSERT LICENSE HERE */

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include <string.h>

#include "./utilities/sysif.h"
#include "usbhDeviceApi.h"
#include "usbhDriverInternal.h"

//TODO: Remove all instructions under _DEBUG_
#undef _DEBUG_


#if defined(_DEBUG_)
#include "iodefine.h"
#endif
/******************************************************************************
Macro definitions
******************************************************************************/

#define usbhFreeHub(pHub)           memset(pHub, 0, sizeof(USBHI))
#define usbhFreePort(pPort)         memset(pPort, 0, sizeof(USBPI))
#define usbhFreeDevice(pDevice)     memset(pDevice, 0, sizeof(USBDI))
#define usbhFreeEndpoint(pEndpoint) memset(pEndpoint, 0, sizeof(USBEI))

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#define ASSERT(x)	((void) 0)
#endif

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/

extern void enumInit(void);
extern void enumUninit(void);
extern void enumSuspend(void);
extern void enumResume(void);

/******************************************************************************
Private global variables and functions
******************************************************************************/

static void usbHostSchedule(PUSBHC pUsbHc);
static void usbhDestroyHubInformation(PUSBHI pHub);
static int usbhCancelRequests(PUSBEI pEndpoint, PUSBTR pRequestList);
static _Bool usbhValidDevice(PUSBDI pDevice);
static void usbhAddTransferRequest(PUSBTR pRequest);
static _Bool usbhRemoveTransferRequest(PUSBTR pRequest);
static void usbhSafeRemove(PUSBHC pUsbHc, PUSBTR pRequest);
static PUSBEI usbhCreateEndpointInformation(PUSBEP pUsbEpDesc);
static PUSBEI usbhCreatControlEndpointInformation(PUSBDI   pDevice,
                                                  uint16_t     wPacketSize,
                                                  USBDIR   transferDirection);
static uint16_t usbhGetWord(uint16_t * pWord);
static PUSBHI usbAllocHub(void);
static PUSBPI usbAllocPort(void);
static PUSBDI usbhAllocDevice(void);
static PUSBEI usbhAllocEndpoint(void);
static void usbhDestroyEndpointInformation(PUSBDI pDevice);

/******************************************************************************
Constant Data
******************************************************************************/
#if defined(TESTING)
const uint8_t gpbyDefaultDescriptor[] =
#else
static const uint8_t gpbyDefaultDescriptor[] =
#endif
{
    USBF_DB(0x12),                  /* bLength */
    USBF_DB(0x01),                  /* bDescriptorType */
    USBF_DW(0x0100),                /* bcdUSB */
    USBF_DB(0xFF),                  /* bDeviceClass */
    USBF_DB(0xFF),                  /* bDeviceSubClass */
    USBF_DB(0xFF),                  /* bDeviceProtocol */
    USBF_DB(0x08),                  /* bMaxPacketSize0 */
    USBF_DW(0x0000),                /* idVendor */
    USBF_DW(0x0000),                /* idProduct */
    USBF_DW(0x0000),                /* bcdDevice */
    USBF_DB(0x00),                  /* iManufacturer */
    USBF_DB(0x00),                  /* iProduct */
    USBF_DB(0x00),                  /* iSerialNumber */
    USBF_DB(0x01)                   /* bNumConfigurations */
};

/******************************************************************************
Global Variables
******************************************************************************/

/* An array to hold pointers to all the host controllers attached the system */
static struct _HCDLST
{
    PUSBHC  pUsbHc;
    _Bool   bfAllocated;
} gpHcdLst[USBH_MAX_CONTROLLERS];
/* The hub, port, device and endpoint resouces */
#if defined(TESTING)
USBHI gpUsbHub[USBH_MAX_HUBS];
USBPI gpUsbPort[USBH_MAX_PORTS];
USBDI gpUsbDevice[USBH_MAX_DEVICES];
USBEI gpUsbEndpoint[USBH_MAX_ENDPOINTS];
#else
static USBHI gpUsbHub[USBH_MAX_HUBS];
static USBPI gpUsbPort[USBH_MAX_PORTS];
static USBDI gpUsbDevice[USBH_MAX_DEVICES];
static USBEI gpUsbEndpoint[USBH_MAX_ENDPOINTS];
#endif
/* Flag to show that the enumerator has been started */
static _Bool gbfEnumeratorStarted = false;

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/******************************************************************************
Function Name: usbhOpen
Description:   Function to open the USB host driver
Arguments:     IN  pUsbHc - Pointer to the host controller data
Return value:  true for success or false if not enough room in the host array
******************************************************************************/
_Bool usbhOpen(PUSBHC pUsbHc)
{
    int iIndex = USBH_MAX_CONTROLLERS;
    /* Reset the host controller data */
    memset(pUsbHc, 0, sizeof(USBHC));
    /* The upper level API abstracts from the phusical number of host
       controllers attached to the system. Therefore there is only one
       enumerator */
    if (gbfEnumeratorStarted == false)
    {
        gbfEnumeratorStarted = true;
#if !defined(TESTING)
        enumInit();
#endif
    }
    /* Add the pointer to the first available entry in the array */
    while (iIndex--)
    {
        if (gpHcdLst[iIndex].bfAllocated == false)
        {
            gpHcdLst[iIndex].pUsbHc = pUsbHc;
            gpHcdLst[iIndex].bfAllocated = true;
            return true;
        }
    }
    return false;
}
/******************************************************************************
End of function  usbhOpen
******************************************************************************/

/*****************************************************************************
Function Name: usbhClose
Description:   Function to close the USB host driver
Arguments:     IN  pUsbHc - Pointer to the host controller data
Return value:  none
*****************************************************************************/
void usbhClose(PUSBHC pUsbHc)
{
    int     iIndex = USBH_MAX_CONTROLLERS;
    _Bool   bfClose = true;
    /* Scan all the Host controllers in the list to find the specified one */
    while (iIndex--)
    {
        if (gpHcdLst[iIndex].pUsbHc == pUsbHc)
        {
            /* Remove the pointer to the Host Controller Data Structure */
            gpHcdLst[iIndex].pUsbHc = NULL;
            gpHcdLst[iIndex].bfAllocated = false;
            break;
        }
    }
    iIndex = USBH_MAX_CONTROLLERS;
    /* Scan all the Host controllers in the list to find the specified one */
    while (iIndex--)
    {
        /* If all the Host controllers have been closed then we need to 
           turn off the enumerator. */
        if (gpHcdLst[iIndex].bfAllocated)
        {
            bfClose = false;
        }
    }    
    if (bfClose)
    {
#if !defined(TESTING)
        enumUninit();
#endif
        gbfEnumeratorStarted = false;
    }
}
/*****************************************************************************
End of function  usbhClose
******************************************************************************/

/******************************************************************************
Function Name: usbhAddRootPort
Description:   Function to add a root port to the driver
Arguments:     IN  pUsbHc - Pointer to the host controller data
               IN  pRoot - Pointer to the port control functions for this port
Return value:  true if the port was added
******************************************************************************/
_Bool usbhAddRootPort(PUSBHC pUsbHc, const PUSBPC pRoot)
{
    if (pRoot)
    {
        PUSBPI pPort = usbAllocPort();
    
        if (pPort)
        {
            PUSBPI  *ppEndOfList = &pUsbHc->pPort;
            /* Add the port functions and index to the port */
            pPort->pRoot = pRoot;
            pPort->uiPortIndex = pRoot->uiPortIndex;
            /* Add the pointer to the hardware */
            pPort->pUSB = pRoot->pUSB;
            /* And the pointer to the host controller data */
            pPort->pUsbHc = pUsbHc;
            /* Get list lock */
        
            /* Find the end of the list */
            while (*ppEndOfList)
            {
                ppEndOfList = &(*ppEndOfList)->pNext;
            }
            /* Add the port to the list */
            if (ppEndOfList)
            {
                /* Add to the End of the list */
                *ppEndOfList = pPort;
            }
            else
            {
                pPort->pNext = pUsbHc->pPort;
                pUsbHc->pPort = pPort;
            }
            /* Release list lock */
        
            return true;
        }
    }
    return false;
}
/******************************************************************************
End of function  usbhAddRootPort
******************************************************************************/

/******************************************************************************
Function Name: usbhComplete
Description:   Function to complete a transfer
Arguments:     pRequest - Pointer to the request to complete
Return value:  true if the request was completed
******************************************************************************/
_Bool usbhComplete(PUSBTR pRequest)
{
    /* Set the length transferred */
    pRequest->uiTransferLength = pRequest->stIdx;
    pRequest->bfInProgress = false;
    return usbhRemoveTransferRequest(pRequest);
}
/******************************************************************************
End of function  usbhComplete
******************************************************************************/

/******************************************************************************
Function Name: usbhIdleTimerTick
Description:   Function to perform an Idle time-out function and should be
               called at the start of each 1mS frame. If the request's idle
               time is exceeded then the request is completed with the error
               code. REQ_IDLE_TIME_OUT
Arguments:     IN  pRequest - Pointer to the request
               IN  bfIdle - true if the request has been idle in the last frame
Return value:  none
******************************************************************************/
void usbhIdleTimerTick(PUSBTR pRequest, _Bool bfIdle)
{
    if (pRequest->dwIdleTimeOut != REQ_IDLE_TIME_OUT_INFINITE)
    {
        if (bfIdle)
        {
            /* Check the idle time */
            if (pRequest->dwIdleTime)
            {
                /* Decrement the idle count */
                pRequest->dwIdleTime--;
            }
            else
            {
                PUSBDI  pDevice = pRequest->pEndpoint->pDevice;
                TRACE(("TimeOut: T%d D%d\r\n",
                       pRequest->pEndpoint->transferType,
                       pRequest->pEndpoint->transferDirection));
                /* The request idle time has been exceeded */
                pRequest->errorCode = REQ_IDLE_TIME_OUT;
                /* If this is a control transfer */
                if ((pRequest->pEndpoint == pDevice->pControlSetup)
                ||  (pRequest->pEndpoint == pDevice->pControlOut)
                ||  (pRequest->pEndpoint == pDevice->pControlIn))
                {
#if defined(_DEBUG_)
                    PORTC.PODR.BYTE = 0;
#endif
                    usbhCancelAllControlRequests(pDevice);
                }
                else
                {
                    /* Cancel the request */
                    usbhCancelTransfer(pRequest);
                }
            }
        }
    }
    else
    {
        pRequest->dwIdleTime = pRequest->dwIdleTimeOut;
    }
}
/******************************************************************************
End of function  usbhIdleTimerTick
******************************************************************************/

/******************************************************************************
The Upper level driver application interface
******************************************************************************/

/******************************************************************************
Function Name: usbhGetDevice
Description:   Function to get a device pointer
Arguments:     IN  pszLinkName - Pointer to the device link name
Return value:  Pointer to the device or NULL for invalid parameter
******************************************************************************/
PUSBDI usbhGetDevice(int8_t * pszLinkName)
{
    int iIndex = USBH_MAX_CONTROLLERS;
    while (iIndex--)
    {
        if (gpHcdLst[iIndex].bfAllocated) 
        {
            /* Point at the root port */
            PUSBPI  pPort = gpHcdLst[iIndex].pUsbHc->pPort;
            /* Go through each port */
            while (pPort)
            {
                /* If the port has a device on it */
                if ((pPort->pDevice)
                &&  (pPort->dwPortStatus & USBH_HUB_PORT_CONNECT_STATUS))
                {
                    /* Check the link name */
                    if (strcmp((const char*)pPort->pDevice->pszSymbolicLinkName,
                            (const char*)pszLinkName) == 0)
                    {
                        return pPort->pDevice;
                    }
                }
                /* Go to next on list */
                pPort = pPort->pNext;
            }
        }
    } 
    return NULL;
}
/******************************************************************************
End of function  usbhGetDevice
******************************************************************************/

/******************************************************************************
Function Name: usbhGetEndpoint
Description:   Function to open a device endpoint
Arguments:     IN  pDevice - The device to get
               IN  byEndpoint - The endpoint index to open
Return value:  Pointer to the endpoint or NULL if not found
******************************************************************************/
PUSBEI usbhGetEndpoint(PUSBDI pDevice, uint8_t byEndpoint)
{
    PUSBEI pEndpoint = pDevice->pEndpoint;
    if (!byEndpoint)
    {
        return NULL;
    }
    while (byEndpoint--)
    {
        pEndpoint = pEndpoint->pNext;
        if (!pEndpoint)
        {
            return pEndpoint;
        }
    }
    return pEndpoint;
}
/******************************************************************************
End of function  usbhGetEndpoint
******************************************************************************/

/******************************************************************************
Function Name: usbhStartTransfer
Description:   Function to start data transfer on an endpoint
Arguments:     IN  pDevice - Pointer to the device
               IN  pRequest -Pointer to the transfer request struct
               IN  pEndpoint -Pointer to the endpoint struct
               IN  pMemory - Pointer to the memory to transfer
               IN  stLength - The length of memory to transfer
               IN  dwIdleTimeOut - The request idle time out in mS
Return value:  true if the request was added
NOTE: Requires mutually exclusive access to protect list
******************************************************************************/
_Bool usbhStartTransfer(PUSBDI      pDevice,
                        PUSBTR      pRequest,
                        PUSBEI      pEndpoint,
                        uint8_t     *pMemory,
                        size_t      stLength,
                        uint32_t    dwIdleTimeOut)
{
    if ((usbhValidDevice(pDevice))
    &&  (sysGetSignalState(pRequest) != SYSIF_SIGNAL_ERROR))
    {
        int     iUnlock;
        PUSBHC  pUsbHc = pDevice->pPort->pUsbHc;
        /* ACQUIRE MUTEX LIST LOCK */
        iUnlock = sysLock(NULL);
        usbhSafeRemove(pUsbHc, pRequest);
        /* RELEASE MUTEX LIST LOCK */
        sysUnlock(NULL, iUnlock);
        memset(pRequest, 0, (sizeof(USBTR) - sizeof(USPTC)));
        /* Put the data into the request structure */
        pRequest->pUSB = pDevice->pPort->pUSB;
        pRequest->pUsbHc = pUsbHc;
        pRequest->pEndpoint = pEndpoint;
        pRequest->pMemory = pMemory;
        pRequest->stLength = stLength;
        pRequest->dwIdleTimeOut = dwIdleTimeOut;
        pRequest->dwIdleTime = dwIdleTimeOut;
        pRequest->errorCode = USBH_NO_ERROR;
        /* ACQUIRE MUTEX LIST LOCK */
        iUnlock = sysLock(NULL);
        /* Add the request to the list */
        usbhAddTransferRequest(pRequest);
        /* RELEASE MUTEX LIST LOCK */
        sysUnlock(NULL, iUnlock);
        /* Schedule BULK transfers immediately */
        if (pRequest->pEndpoint->transferType == USBH_BULK)
        {
            usbHostSchedule(pUsbHc);
        }
        return true;
    }
    return false;

}
/******************************************************************************
End of function  usbhStartTransfer
******************************************************************************/

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
NOTE: Requires mutually exclusive access to protect list
******************************************************************************/
_Bool usbhIsocTransfer(PUSBDI   pDevice,
                       PUSBTR   pRequest,
                       PUSBIV   pIsocPacketSize,
                       PUSBEI   pEndpoint,
                       uint8_t *pMemory,
                       size_t   stLength,
                       uint32_t dwIdleTimeOut)
{
    if ((usbhValidDevice(pDevice))
    &&  (sysGetSignalState(pRequest) != SYSIF_SIGNAL_ERROR))
    {
        int iUnlock;
        PUSBHC  pUsbHc = pDevice->pPort->pUsbHc;
        /* ACQUIRE MUTEX LIST LOCK */
        iUnlock = sysLock(NULL);
        usbhSafeRemove(pUsbHc, pRequest);
        /* RELEASE MUTEX LIST LOCK */
        sysUnlock(NULL, iUnlock);
        memset(pRequest, 0, (sizeof(USBTR) - sizeof(USPTC)));
        /* Put the data into the request structure */
        pRequest->pUSB = pDevice->pPort->pUSB;
        pRequest->pUsbHc = pUsbHc;
        pRequest->pEndpoint = pEndpoint;
        pRequest->pMemory = pMemory;
        pRequest->stLength = stLength;
        pRequest->dwIdleTimeOut = dwIdleTimeOut;
        pRequest->pIsocPacketSize = pIsocPacketSize;
        pRequest->dwIdleTime = dwIdleTimeOut;
        pRequest->errorCode = USBH_NO_ERROR;
        /* ACQUIRE MUTEX LIST LOCK */
        iUnlock = sysLock(NULL);
        /* Add the request to the list */
        usbhAddTransferRequest(pRequest);
        /* RELEASE MUTEX LIST LOCK */
        sysUnlock(NULL, iUnlock);
        return true;
    }
    return false;
}
/******************************************************************************
End of function  usbhIsocTransfer
******************************************************************************/

/******************************************************************************
Function Name: usbhCancelTransfer
Description:   Function to cancel a transfer
Arguments:     IN  pRequest - Pointer to the request to cancel
Return value:  true if the request was cancelled
******************************************************************************/
_Bool usbhCancelTransfer(PUSBTR pRequest)
{
    _Bool    bfResult;
    /* ACQUIRE MUTEX LIST LOCK */
    int iUnlock;
    iUnlock = sysLock(NULL);
    /* If there is a driver specific function that needs to be called */
    if (pRequest->pCancel)
    {
        /* Call it */
        pRequest->pCancel(pRequest);
    }
    bfResult = usbhRemoveTransferRequest(pRequest);
    /* RELEASE MUTEX LIST LOCK */
    sysUnlock(NULL, iUnlock);
    return bfResult;
}
/******************************************************************************
End of function  usbhCancelTransfer
******************************************************************************/

/******************************************************************************
Function Name: usbhCancelAllTransferReqests
Description:   Function to cancel all transfer requests for a device
Arguments:     IN  pDevice - Pointer to the device to cancel all transfer
                             requests
Return value:  The number of requests cancelled
******************************************************************************/
int usbhCancelAllTransferReqests(PUSBDI pDevice)
{
    int     iCount = 0;
    int     iIndex = USBH_MAX_CONTROLLERS;
    PUSBEI  pEndpoint = pDevice->pEndpoint;
    while (iIndex--)
    {
        if (gpHcdLst[iIndex].bfAllocated) 
        {
            /* Check the control list pointer */
            if (gpHcdLst[iIndex].pUsbHc->pControl)
            {
                /* Cancel any control requests */
                iCount +=
                    usbhCancelRequests(pEndpoint,
                                       gpHcdLst[iIndex].pUsbHc->pControl);
            }
            /* Check the Interrupt list pointer */
            if (gpHcdLst[iIndex].pUsbHc->pInterrupt)
            {
                /* Cancel any interrupt requests */
                iCount +=
                    usbhCancelRequests(pEndpoint,
                                       gpHcdLst[iIndex].pUsbHc->pInterrupt);
            }
            /* Check the bulk list pointer */
            if (gpHcdLst[iIndex].pUsbHc->pBulk)
            {
                /* Cancel any bulk requests */
                iCount +=
                    usbhCancelRequests(pEndpoint,
                                       gpHcdLst[iIndex].pUsbHc->pBulk);
            }
            /* Check the Isochronus list pointer */
            if (gpHcdLst[iIndex].pUsbHc->pIsochronus)
            {
                /* Cancel any Isochronus requests */
                iCount +=
                    usbhCancelRequests(pEndpoint,
                                       gpHcdLst[iIndex].pUsbHc->pIsochronus);
            }
        }
    }
    return iCount;
}
/******************************************************************************
End of function  usbhCancelAllTransferReqests
******************************************************************************/

/******************************************************************************
Function Name: usbhCancelAllControlRequests
Description:   Function to cancel all control requests for a device
Arguments:     IN  pDevice - Pointer to the device to cancel all transfer
                             requests
Return value:  The number of requests cancelled
******************************************************************************/
int usbhCancelAllControlRequests(PUSBDI pDevice)
{
    PUSBHC  pUsbHc = pDevice->pPort->pUsbHc;
    /* Get a pointer to the control list */
    PUSBTR  pRequest = pUsbHc->pControl;
    int     iCount = 0;
    /* Go through the list */
    while (pRequest)
    {
        if (pRequest->pEndpoint->pDevice == pDevice)
        {
            usbhRemoveTransferRequest(pRequest);
            iCount++;
            /* Get a pointer to the control list */
            pRequest = pUsbHc->pControl;
        }
        else
        {
            pRequest = pRequest->pNext;
        }
    }
    pUsbHc->pCurrentControl = NULL;
    return iCount;
}
/******************************************************************************
End of function  usbhCancelAllControlRequests
******************************************************************************/

/******************************************************************************
Function Name: usbhTransferInProgress
Description:   Function to find out if the request is in progress
Arguments:     IN  pRequest - Pointer to the request
Return value:  true if the transfer is in progress
******************************************************************************/
_Bool usbhTransferInProgress(PUSBTR pRequest)
{
    if (sysGetSignalState(pRequest) == SYSIF_SIGNAL_SET)
    {
        return false;
    }
    return true;
}
/******************************************************************************
End of function  usbhTransferInProgress
******************************************************************************/

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
#ifdef _NO_WORKAROUND_
REQERR usbhDeviceRequest(PUSBTR    pRequest,
                         PUSBDI    pDevice,
                         uint8_t   bmRequestType,
                         uint8_t   bRequest,
                         uint16_t  wValue,
                         uint16_t  wIndex,
                         uint16_t  wLength,
                         uint8_t  *pbyData)
#else
REQERR _usbhDeviceRequest(PUSBTR    pRequest,
                          PUSBDI    pDevice,
                          uint8_t   bmRequestType,
                          uint8_t   bRequest,
                          uint16_t  wValue,
                          uint16_t  wIndex,
                          uint16_t  wLength,
                          uint8_t  *pbyData)
#endif
{
    USBDR   deviceRequest;
    USBTR   setupRequest;
    USBTR   statusRequest;
    REQERR  reqResult = REQ_NO_ERROR;
    /* Pointer to the endpoints to make the device request */
    PUSBEI  pStatus = NULL, pSetup = pDevice->pControlSetup;
    /* Create the events for the setup and status phases */
    if (sysCreateSignal(&setupRequest))
    {
        return REQ_SIGNAL_CREATE_ERROR;
    }
    if (sysCreateSignal(&statusRequest))
    {
        sysDestroySignal(&setupRequest);
        return REQ_SIGNAL_CREATE_ERROR;
    }
#if !defined(TESTING)
    /* Suspend the enumerator */
    enumSuspend();
#endif
    /* Put the data in to the SETUP packet */
    deviceRequest.bmRequestType = bmRequestType;
    deviceRequest.bRequest = bRequest;
    deviceRequest.wValue = wValue;
    deviceRequest.wIndex = wIndex;
    deviceRequest.wLength = wLength;
    /* Setup packets are always have a PID of data 0 */
    pSetup->dataPID = USBH_DATA0;
    /* Start the setup packet transfer */
    if (usbhStartTransfer(pDevice,
                          &setupRequest,
                          pSetup,
                          (uint8_t *)&deviceRequest,
                          8,
                          100UL))
    {
        _Bool   bfStatus = true;
        /* If there is a data phase */
        if (wLength)
        {
            PUSBEI  pData;
            /* Select the endpoint to use for this request */
            if (bmRequestType & USB_DEVICE_TO_HOST_MASK)
            {
                pData = pDevice->pControlIn;
                /* The status phase is always in the opposite direction to the
                   data */
                pStatus = pDevice->pControlOut;
            }
            else
            {
                pData = pDevice->pControlOut;
                /* The status phase is always in the opposite direction to the
                   data */
                pStatus  = pDevice->pControlIn;
            }
            /* Data phase always has a PID of data 1 */
            pData->dataPID = USBH_DATA1;
            /* Start the data phase */
            bfStatus = usbhStartTransfer(pDevice,
                                         pRequest,
                                         pData,
                                         pbyData,
                                         (size_t)wLength,
                                         100UL);
        }
        else
        {
            /* Without a data phase the status phase is IN (Device to host) */
            pStatus = pDevice->pControlIn;
        }
        
        if (bfStatus)
        {
            /* Status phase always has a PID of data 1 */
            pStatus->dataPID = USBH_DATA1;
            /* Start the status phase */
            if (usbhStartTransfer(pDevice,
                                  &statusRequest,
                                  pStatus,
                                  NULL,
                                  0,
                                  100UL))
            {
                /* Wait for the setup phase to complete */
                sysWaitSignal(&setupRequest);
                if (setupRequest.errorCode)
                {
                    usbhCancelTransfer(pRequest);
                    usbhCancelTransfer(&statusRequest);
                    reqResult = setupRequest.errorCode;
                }
                else
                {
                    /* If there is a data phase */
                    if (wLength)
                    {
                        /* Wait for the data phase to complete */
                        sysWaitSignal(pRequest);
                        if (pRequest->errorCode)
                        {
                            usbhCancelTransfer(&statusRequest);
                            reqResult = pRequest->errorCode;
                        }
                    }
                    /* If there has been no error on the data phase */
                    if (reqResult == REQ_NO_ERROR)
                    {
                        /* Wait for the status phase to complete */
                        sysWaitSignal(&statusRequest);
                        if (statusRequest.errorCode)
                        {
                            reqResult = statusRequest.errorCode;
                        }
                    }
                }
            }
            else
            {
                usbhCancelTransfer(&setupRequest);
                usbhCancelTransfer(pRequest);
                reqResult = REQ_DEVICE_NOT_FOUND;
            }
        }
        else
        {
            usbhCancelTransfer(&setupRequest);
            reqResult = REQ_DEVICE_NOT_FOUND;
        }
    }
    else
    {
        reqResult = REQ_DEVICE_NOT_FOUND;
    }
#if !defined(TESTING)
    /* Resume the enumerator */
    enumResume();
#endif
    sysDestroySignal(&setupRequest);
    sysDestroySignal(&statusRequest);
    return reqResult;
}
#ifndef _NO_WORKAROUND_
REQERR usbhDeviceRequest(PUSBTR    pRequest,
                         PUSBDI    pDevice,
                         uint8_t   bmRequestType,
                         uint8_t   bRequest,
                         uint16_t  wValue,
                         uint16_t  wIndex,
                         uint16_t  wLength,
                         uint8_t  *pbyData)
{
    REQERR reqErr;
    /* Submit the request */
    reqErr = _usbhDeviceRequest(pRequest,
                                pDevice,
                                bmRequestType,
                                bRequest,
                                wValue,
                                wIndex,
                                wLength,
                                pbyData);
    /* Check the error code */
    switch (reqErr)
    {
        case REQ_NO_ERROR:
        break;
        /* Missing BEMP */
        case REQ_STATUS_PHASE_TIME_OUT:
        pRequest->errorCode = REQ_NO_ERROR;
        return REQ_NO_ERROR;
        /* FIFO Stuck */
        default:
        {
            /* Re-submit the request & hope that it works this time */
            reqErr = _usbhDeviceRequest(pRequest,
                                        pDevice,
                                        bmRequestType,
                                        bRequest,
                                        wValue,
                                        wIndex,
                                        wLength,
                                        pbyData);
            if (reqErr == REQ_FIFO_READ_ERROR)
            {
                /* Very unlucky to happen 3 times in a row */
                reqErr = _usbhDeviceRequest(pRequest,
                                            pDevice,
                                            bmRequestType,
                                            bRequest,
                                            wValue,
                                            wIndex,
                                            wLength,
                                            pbyData);
            }
        }
    }
    return reqErr;
}
#endif
/******************************************************************************
End of function  usbhDeviceRequest
******************************************************************************/

/******************************************************************************
Device Information Functions
******************************************************************************/

/******************************************************************************
Function Name: usbhCreateDeviceInformation
Description:   Function to create device information struct
Arguments:     IN  pUsbDevDesc - Pointer to the Usb Device descriptor 
               IN  transferSpeed - The device transfer speed
Return value:  Pointer to the device or NULL on error
******************************************************************************/
PUSBDI usbhCreateDeviceInformation(PUSBDD  pUsbDevDesc,
                                   USBTS   transferSpeed)
{
    /* Allocate a new device */
    PUSBDI pDevice = usbhAllocDevice();
    /* If one was allocated */
    if (pDevice)
    {
        uint16_t    wPacketSize = (uint16_t)pUsbDevDesc->bMaxPacketSize0;
        /* Set the device information */
        pDevice->transferSpeed = transferSpeed;
        pDevice->wVID = usbhGetWord(&pUsbDevDesc->wVendor);
        pDevice->wPID = usbhGetWord(&pUsbDevDesc->wProduct);
        pDevice->wDeviceVersion = usbhGetWord(&pUsbDevDesc->wDevice);
        /* Set the interface class initially from the device class then later
           overwrite with the interface class if 0 */
        pDevice->byInterfaceClass = pUsbDevDesc->bDeviceClass;
        pDevice->byInterfaceSubClass = pUsbDevDesc->bDeviceSubClass;
        pDevice->byInterfaceProtocol = pUsbDevDesc->bDeviceProtocol;

        TRACE(("PID0x%.4X VID0x%.4X V%.2d.%.2d\r\n",
                pDevice->wPID,
                pDevice->wVID,
                HIBYTE(pDevice->wDeviceVersion),
                LOBYTE(pDevice->wDeviceVersion)));

        /* Create the Setup control endpoint */
        pDevice->pControlSetup = 
            usbhCreatControlEndpointInformation(pDevice,
                                                wPacketSize,
                                                USBH_SETUP);
        if (pDevice->pControlSetup)
        {
            /* Create the OUT endpoint */
            pDevice->pControlOut = 
                usbhCreatControlEndpointInformation(pDevice,
                                                    wPacketSize,
                                                    USBH_OUT);
            if (pDevice->pControlOut)
            {
                /* Create the IN endpoint */
                pDevice->pControlIn = 
                    usbhCreatControlEndpointInformation(pDevice,
                                                        wPacketSize,
                                                        USBH_IN);
                if (pDevice->pControlIn)
                {
                    return pDevice;
                }
                usbhFreeEndpoint(pDevice->pControlIn);
            }
            usbhFreeEndpoint(pDevice->pControlOut);
        }
        usbhFreeEndpoint(pDevice->pControlSetup);
        usbhFreeDevice(pDevice);
    }
    return NULL;
}
/******************************************************************************
End of function  usbhCreateDeviceInformation
******************************************************************************/

/******************************************************************************
Function Name: usbhCreateEnumerationDeviceInformation
Description:   Function to create a device for enumeration
Arguments:     IN  transferSpeed - The required device transfer speed
Return value:  Pointer to the device or NULL on error
******************************************************************************/
PUSBDI usbhCreateEnumerationDeviceInformation(USBTS transferSpeed)
{
    return usbhCreateDeviceInformation((PUSBDD)gpbyDefaultDescriptor,
                                       transferSpeed);
}
/******************************************************************************
End of function  usbhCreateEnumerationDeviceInformation
******************************************************************************/

/******************************************************************************
Function Name: usbhSetEp0PacketSize
Description:   Function to set the control pipe packet size
Arguments:     IN  pDevice - Pointer to the device to set
               IN  wPacketSize - The packet size
Return value:  none
******************************************************************************/
void usbhSetEp0PacketSize(PUSBDI pDevice, uint16_t wPacketSize)
{
    pDevice->pControlSetup->wPacketSize = wPacketSize;
    pDevice->pControlOut->wPacketSize = wPacketSize;
    pDevice->pControlIn->wPacketSize = wPacketSize;
}
/******************************************************************************
End of function  usbhSetEp0PacketSize
******************************************************************************/

/******************************************************************************
Function Name: usbhCreateInterfaceInformation
Description:   Function to Create the Interface information
Arguments:     IN  pDevice - Pointer to the device
               IN  pUsbIfDesc - Pointer to the interface descriptor
Return value:  true if information created
******************************************************************************/
_Bool usbhCreateInterfaceInformation(PUSBDI pDevice, PUSBIF pUsbIfDesc)
{
    PUSBEP  pUsbEpDesc;
    PUSBEI  pEndpoint, *ppEndpoint;
    uint8_t    byNumEndpoints = pUsbIfDesc->bNumEndpoints;
    /* Set the class information - from the first interface */
    if ((pDevice->byInterfaceClass == (uint8_t)0xFF)
    ||  (!pDevice->byInterfaceClass))
    {
        pDevice->byInterfaceClass = pUsbIfDesc->bInterfaceClass;
    }
    if ((pDevice->byInterfaceSubClass == (uint8_t)0xFF)
    ||  (!pDevice->byInterfaceProtocol))
    {
        pDevice->byInterfaceSubClass = pUsbIfDesc->bInterfaceSubClass;
    }
    if ((pDevice->byInterfaceProtocol == (uint8_t)0xFF)
    ||  (!pDevice->byInterfaceProtocol))
    {
        pDevice->byInterfaceProtocol = pUsbIfDesc->bInterfaceProtocol;
    }
    if (byNumEndpoints)
    {
        pUsbEpDesc = (PUSBEP)((uint8_t *)pUsbIfDesc + pUsbIfDesc->bLength);
        ppEndpoint = &pDevice->pEndpoint;
        /* Go to the end of the endpoint list in the case of more than one
           interface adding endpoints in more than one call to this function */
        while (*ppEndpoint)
        {
            ppEndpoint = &(*ppEndpoint)->pNext;
        }
        /* Look for the first endpoint descriptor */
        while (true)
        {
            /* If it is an endpoint descriptor */
            if (pUsbEpDesc->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE)
            {
                break;
            }
            else
            {
                /* Advance to next descriptor */
                pUsbEpDesc = (PUSBEP)((uint8_t *)pUsbEpDesc +
                                                          pUsbEpDesc->bLength);
            }
        }
        while (byNumEndpoints--)
        {
            /* Create the endpoint */
            pEndpoint = usbhCreateEndpointInformation(pUsbEpDesc);
            if (pEndpoint)
            {
                /* Add to list */
                pEndpoint->pDevice = pDevice;
                *ppEndpoint = pEndpoint;
                ppEndpoint = &pEndpoint->pNext;
            }
            else
            {
                /* Out of memory so exit */
                usbhDestroyEndpointInformation(pDevice);
                return false;
            }
            /* Advance to next descriptor */
            pUsbEpDesc = (PUSBEP)((uint8_t *)pUsbEpDesc + pUsbEpDesc->bLength);
        }
    }
    return true;
}
/******************************************************************************
End of function  usbhCreateInterfaceInformation
******************************************************************************/

/******************************************************************************
Function Name: usbhConfigureDeviceInformation
Description:   Function to set the device interface information
Arguments:     IN  pDevice - Pointer to the device information to configure
               IN  pUsbCfgDesc - Pointer to the configuration descriptor
               IN  byInterfaceNumber - The interface number to use
               IN  byAlternateSetting - The alternate interface setting 
Return value:  true if the device information was cofigured successfuly
******************************************************************************/
_Bool usbhConfigureDeviceInformation(PUSBDI pDevice,
                                     PUSBCG pUsbCfgDesc,
                                     uint8_t   byInterfaceNumber,
                                     uint8_t   byAlternateSetting)
{
    /* Set the max power */
    pDevice->wMaxPower_mA = (uint16_t)((uint16_t)pUsbCfgDesc->bMaxPower << 1);
    /* Check to see if the interface exists */
    if (byInterfaceNumber <= pUsbCfgDesc->bNumInterfaces)
    {
        /* Get a pointer to the IF desc */
        PUSBIF pUsbIfDesc = (PUSBIF)((uint8_t *)pUsbCfgDesc +
                                                        pUsbCfgDesc->bLength);
        /* Get a pointer to the end of the descriptors */
        uint8_t *pbyEnd = ((uint8_t*)pUsbCfgDesc)
                       + usbhGetWord(&pUsbCfgDesc->wTotalLength);
        /* Take only the first interface */
        uint8_t   byNumInterfaces = pUsbCfgDesc->bNumInterfaces;
        /* Skip other descriptors looking for the first interface descriptor */
        while (((uint8_t*)pUsbIfDesc) < pbyEnd)
        {
        /* Check that we have the right one */
        if (pUsbIfDesc->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
        {
            /* Look for the desired interface */
            while ((pUsbIfDesc->bInterfaceNumber != byInterfaceNumber)
            &&     (pUsbIfDesc->bAlternateSetting != byAlternateSetting))
            {
                pUsbIfDesc = (PUSBIF)((uint8_t *)pUsbIfDesc +
                                                         pUsbIfDesc->bLength);
                byNumInterfaces--;
                if (byNumInterfaces == 0)
                {
                    TRACE(("DD_USBH_ConfigureDeviceInformation: "
                            "interface %d Alt %d does not exist\r\n", 
                             byInterfaceNumber,
                             byAlternateSetting));
                    return false;
                }
            }
            TRACE(("usbhConfigureDeviceInformation: Creating interface %d Alt %d\r\n",
                    byInterfaceNumber,
                    byAlternateSetting));
            /* Free previous device endpoints */
            usbhDestroyEndpointInformation(pDevice);
            /* Set the configuration value */
            pDevice->byConfigurationValue = pUsbCfgDesc->bConfigurationValue;
            pDevice->byInterfaceNumber = byInterfaceNumber;
            pDevice->byAlternateSetting = byAlternateSetting;
            /* Create the new endpoint information */
            return usbhCreateInterfaceInformation(pDevice, pUsbIfDesc);
        }
            /* Advance to the next descriptor */
            pUsbIfDesc = (PUSBIF)((uint8_t*)pUsbIfDesc + pUsbIfDesc->bLength);
        }
    }
    TRACE(("usbhConfigureDeviceInformation: interface %d does not exist\r\n",
	        byInterfaceNumber));
    return false;
}
/******************************************************************************
End of function  usbhConfigureDeviceInformation
******************************************************************************/

/******************************************************************************
Function Name: usbhDestroyDeviceInformation
Description:   Function to destroy the device
Arguments:     IN  pDevice - Pointer to the device information to destroy
Return value:  none
******************************************************************************/
void usbhDestroyDeviceInformation(PUSBDI pDevice)
{
    if (pDevice)
    {
        PUSBHI pHub = pDevice->pHub;
        /* If this device is a hub */
        if (pDevice->pHub)
        {
            /* Prevent re-entry */
            pDevice->pHub = NULL;
            /* Destroy all the ports attached to the hub and all of the devices
               attached to the ports */
            usbhDestroyHubInformation(pHub);
        }
        if (pDevice->pControlSetup)
        {
            usbhFreeEndpoint(pDevice->pControlSetup);
        }
        if (pDevice->pControlOut)
        {
            usbhFreeEndpoint(pDevice->pControlOut);
        }
        if (pDevice->pControlIn)
        {
            usbhFreeEndpoint(pDevice->pControlIn);
        }
        usbhDestroyEndpointInformation(pDevice);
        usbhFreeDevice(pDevice);
    }
}
/******************************************************************************
End of function  usbhDestroyDeviceInformation
******************************************************************************/

/******************************************************************************
Function Name: usbhCreateHubInformation
Description:   Function to create the hub information structure
Arguments:     IN  pDevice - Pointer to the device information
               IN  pHubDesc - Pointer to the hub descriptor
Return value:  Pointer to the hub information or NULL on failure
******************************************************************************/
PUSBHI usbhCreateHubInformation(PUSBDI pDevice, PUSBRH pHubDesc)
{
    PUSBHI  pHub = usbAllocHub();
    if (pHub)
    {
        uint32_t    uiPortIndex = 1;
        uint32_t    uiNumberOfPorts = (uint32_t)pHubDesc->bNumberOfPorts;
        /* Set the hub pointer */
        pDevice->pHub = pHub;
        /* The port on which the hub has been enumerated */
        pHub->pPort = pDevice->pPort;
        /* The number of ports on the hub is defined by the descriptor */
        pHub->byNumberOfPorts = pHubDesc->bNumberOfPorts;
        /* The hub address is that of the device */
        pHub->byHubAddress = pDevice->byAddress;
        /* The hub characteristics as defined by the descriptor */
        pHub->wCharacteristics = usbhGetWord(&pHubDesc->wHubCharacteristics);
        /* The power on to power good time in mS */
        pHub->wPowerOn2PowerGood_mS =
                      (uint16_t)((uint16_t)pHubDesc->bPowerOnToPowerGood << 1);
        /* Set the current used by the hub */
        pHub->byHubCurrent_mA = pHubDesc->bHubControlCurrent;
        /* Add each port on to the list */
        while (uiPortIndex <= uiNumberOfPorts)
        {
            /* Allocate a port */
            PUSBPI  pPort = usbAllocPort();
            if (pPort)
            {
                PUSBHC  pUsbHc = pDevice->pPort->pUsbHc;
                PUSBPI  *ppEndOfList = &pUsbHc->pPort;
                /* Set the hub pointer */
                pPort->pHub = pHub;
                /* Set the port index */
                pPort->uiPortIndex = uiPortIndex++;
                /* Add the pointer to the hardware */
                pPort->pUSB = pDevice->pPort->pUSB;
                /* And the pointer to the host controller data */
                pPort->pUsbHc = pUsbHc;
                /* ACQUIRE MUTEX LIST LOCK */

                /* Find the end of the list */
                while (*ppEndOfList)
                {
                    ppEndOfList = &(*ppEndOfList)->pNext;
                }                
                /* Add the port to the list */
                if (ppEndOfList)
                {
                    /* Add to the End of the list */
                    *ppEndOfList = pPort;
                }
                else
                {
                    pPort->pNext = pUsbHc->pPort;
                    pUsbHc->pPort = pPort;
                }
                /* RELEASE MUTEX LIST LOCK */
                
            }
            else
            {
                /* Insufficient ports to support hub */
                usbhFreeHub(pHub);
                return NULL;
            }
        }
        
    }
    return pHub;
}
/******************************************************************************
End of function  usbhCreateHubInformation
******************************************************************************/

/******************************************************************************
Function Name: usbhCalculateCurrent
Description:   Function to calculate the current draw of the devices attached
               to a hub
Arguments:     IN  pHub - Pointer to the hub
Return value:  The current draw in mA
******************************************************************************/
int usbhCalculateCurrent(PUSBHI pHub)
{
    int    iCurrent = (int)pHub->byHubCurrent_mA;
    PUSBHC pUsbHc = pHub->pPort->pUsbHc;
    PUSBPI pPortList = pUsbHc->pPort;
    while (pPortList)
    {
        /* If this port is attached to the hub */
        if ((pPortList->pHub == pHub)
        /* And there is a device attached to it */
        &&  (pPortList->pDevice))
        {
            /* Add on the max current this device uses */
            iCurrent += pPortList->pDevice->wMaxPower_mA;
        }
        /* Go to the next port on the list */
        pPortList = pPortList->pNext;
    }
    return iCurrent;
}
/******************************************************************************
End of function  usbhCalculateCurrent
******************************************************************************/

/******************************************************************************
Function Name: usbhGetDeviceAddress
Description:   Function to get the first available device address
Arguments:     IN  pUsbHc - Pointer to the host controller
Return value:  The first available device address (-1 if none available) 
******************************************************************************/
int8_t usbhGetDeviceAddress(PUSBHC pUsbHc)
{
    int8_t    byAddress = 1;
    PUSBPI  pPort = pUsbHc->pPort;
    while (pPort)
    {
        /* Check all devices for this address */
        if ((pPort->pDevice)
        &&  (pPort->pDevice->byAddress == byAddress))
        {
            /* Bump the address */
            byAddress++;
            /* Start from the top of the list */
            pPort = pUsbHc->pPort;
            /* no addresses available */
            if (byAddress > USBH_MAX_ADDRESS)
            {
                return -1;
            }
        }
        else
        {
            pPort = pPort->pNext;
        }
    }
    return byAddress;
}
/******************************************************************************
End of function  usbhGetDeviceAddress
******************************************************************************/

/******************************************************************************
Function Name: usbhGetRootPort
Description:   Function to get a pointer to the root port
Arguments:     IN  iIndex - The index of the Host Controller
Return value:  Pointer to the root pointer or NULL if no more root ports
               available
******************************************************************************/
PUSBPI usbhGetRootPort(int iIndex)
{
    if (iIndex < USBH_MAX_CONTROLLERS)
    {
        if (gpHcdLst[iIndex].bfAllocated)
        {
            return gpHcdLst[iIndex].pUsbHc->pPort;
        }
    }
    return NULL;
}
/******************************************************************************
End of function  usbhGetRootPort
******************************************************************************/

/******************************************************************************
Private Functions
******************************************************************************/

/******************************************************************************
Function Name: usbHostSchedule
Description:   Function to call the driver scheduler
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
Return value:  none
******************************************************************************/
static void usbHostSchedule(PUSBHC pUsbHc)
{
    int iLock = sysLock(NULL);
    usbhSchedule(pUsbHc);
    sysUnlock(NULL, iLock);
}
/******************************************************************************
End of function  usbHostSchedule
******************************************************************************/

/******************************************************************************
Function Name: usbhDestroyHubInformation
Description:   Function to destroy the hub, ports and connected devices
Arguments:     IN  pHub - Pointer to the HUB
Return value:  none
******************************************************************************/
static void usbhDestroyHubInformation(PUSBHI pHub)
{
    PUSBHC  pUsbHc = pHub->pPort->pUsbHc;
    PUSBPI *ppPortList = &pUsbHc->pPort;
    /* Look through the port list for ports attached to this hub */
    while (*ppPortList)
    {
        PUSBPI  pPort = *ppPortList;
        /* If this port is attached to the hub */
        if (pPort->pHub == pHub)
        {
            /* If there is a device attached to this port */
            if (pPort->pDevice)
            {
                /* Cancel all outstanding transfer requests on this device */
                usbhCancelAllTransferReqests(pPort->pDevice);
#if !defined(TESTING)
                /* Remove the device from the list */
                devRemove(pPort->pDevice->pszSymbolicLinkName);
#endif
                /* Destroy the device information */
                usbhDestroyDeviceInformation(pPort->pDevice);
            }
            /* Remove this port from the list */
            *ppPortList = (*ppPortList)->pNext;
            /* Free the port */
            usbhFreePort(pPort);
        }
        else
        {
            /* Check the next port on the list */
            ppPortList = &(*ppPortList)->pNext;
        }
    }
    /* Finally free the hub information */
    usbhFreeHub(pHub);
}
/******************************************************************************
End of function  usbhDestroyHubInformation
******************************************************************************/

/******************************************************************************
Function Name: usbhCancelRequests
Description:   Function to search a request list and cancel requests
Arguments:     IN  pEndpoint - Pointer to the list of endpoints
               IN  pRequestList - Pointer to the request list
Return value:  The number of requests calcelled
******************************************************************************/
static int usbhCancelRequests(PUSBEI pEndpoint, PUSBTR pRequestList)
{
    int iCount = 0;
    /* For each endpoint that the device has */
    while (pEndpoint)
    {
        PUSBTR  pRequest = pRequestList;
        /* Look for it on the list */
        while (pRequest)
        {
            if (pRequest->pEndpoint == pEndpoint)
            {
                pRequest->errorCode = REQ_DEVICE_NOT_FOUND;
                usbhCancelTransfer(pRequest);
                iCount++;
            }
            pRequest = pRequest->pNext;
        }
        pEndpoint = pEndpoint->pNext;
    }
    return iCount;
}
/******************************************************************************
End of function  usbhCancelRequests
******************************************************************************/

/******************************************************************************
Function Name: usbhValidDevice
Description:   Function to check to see if a device is attached 
Arguments:     IN  pDevice - Pointer to the device
Return value:  true if the device is attached
******************************************************************************/
static _Bool usbhValidDevice(PUSBDI pDevice)
{
    PUSBHC  pUsbHc = pDevice->pPort->pUsbHc;
    /* Point at the root port */
    PUSBPI  pPort = pUsbHc->pPort;
    /* Go through each port */
    while (pPort)
    {
        /* If the port has the device on it */
        if (pPort->pDevice == pDevice)
        {
            /* The device is attached */
            return true;
        }
        /* Go to next on list */
        pPort = pPort->pNext;
    }
    return false;
}
/******************************************************************************
End of function  usbhValidDevice
******************************************************************************/

/******************************************************************************
Function Name: usbhAddTransferRequest
Description:   Function to add a transfer request to the top of the approprate
               list
Arguments:     pRequest - Pointer to the request
Return value:  none
******************************************************************************/
static void usbhAddTransferRequest(PUSBTR pRequest)
{
    PUSBTR *ppRequestList;
    /* Mark the request as incomplete */
    sysResetSignal(pRequest);
    switch (pRequest->pEndpoint->transferType)
    {
    case USBH_CONTROL:
        ppRequestList = &pRequest->pUsbHc->pControl;
        break;

    case USBH_ISOCHRONOUS:

        ppRequestList = &pRequest->pUsbHc->pIsochronus;
        break;

    case USBH_BULK:
        ppRequestList = &pRequest->pUsbHc->pBulk;
        break;

    case USBH_INTERRUPT:
        ppRequestList = &pRequest->pUsbHc->pInterrupt;
        break;

    default:
        TRACE(("usbhAddTransferRequest: Unknown transfer type\r\n"));
        return;
    }
    /* Add to the end of the list */
    while (*ppRequestList)
    {
        ppRequestList = &(*ppRequestList)->pNext;
    }
    if (*ppRequestList)
    {
        *ppRequestList = pRequest;
    }
    else
    {
        pRequest->pNext = *ppRequestList;
        *ppRequestList = pRequest;
    }
}
/******************************************************************************
End of function  usbhAddTransferRequest
******************************************************************************/

/******************************************************************************
Function Name: usbhRemoveRequest
Description:   Function to remove the request from a list
Parameters:    IN  ppRequestList - Pointer to the request list pointer
               IN  pRequest - Pointer to the request to remove
Return value:  true if the request was removed
******************************************************************************/
static _Bool usbhRemoveRequest(PUSBTR *ppRequestList, PUSBTR pRequest)
{
    _Bool   bfResult = false;
    /* Search the list for the request */
    while ((*ppRequestList != NULL)
    &&     (*ppRequestList != pRequest))
    {
        ppRequestList = &(*ppRequestList)->pNext;
    }
    /* If it has been found */
    if (*ppRequestList)
    {
        /* Delete from the list */
        *ppRequestList = (*ppRequestList)->pNext;
        pRequest->pNext = NULL;
        bfResult = true;
    }
    return bfResult;
}
/******************************************************************************
End of function  usbhRemoveRequest
******************************************************************************/

/******************************************************************************
Function Name: usbhRemoveTransferRequest
Description:   Function to remove a transfer request
Arguments:     IN  pRequest - Pointer to the request to remove
Return value:  true if the request was removed
******************************************************************************/
static _Bool usbhRemoveTransferRequest(PUSBTR pRequest)
{
    _Bool    bfReturn = false;
    PUSBTR *ppRequestList;
    if (pRequest->pEndpoint)
    {
        switch (pRequest->pEndpoint->transferType)
        {
        case USBH_CONTROL:
            ppRequestList = &pRequest->pUsbHc->pControl;
            break;

        case USBH_ISOCHRONOUS:
            ppRequestList = &pRequest->pUsbHc->pIsochronus;
            break;

        case USBH_BULK:
            ppRequestList = &pRequest->pUsbHc->pBulk;
            break;

        case USBH_INTERRUPT:
            ppRequestList = &pRequest->pUsbHc->pInterrupt;
            break;

        default:
            TRACE(("usbhRemoveTransferRequest: Unknown transfer type\r\n"));
            return bfReturn;
        }
    }
    else
    {
        return bfReturn;
    }
    /* Remove the request */
    bfReturn = usbhRemoveRequest(ppRequestList, pRequest);
    /* Set the event to show that the request has been removed */
    sysSetSignal(pRequest);
    return bfReturn;
}
/******************************************************************************
End of function  usbhRemoveTransferRequest
******************************************************************************/

/******************************************************************************
Function Name: usbhSafeRemove
Description:   Function to remove a transfer request that is not initialised
Parameters:    IN  pUsbUc - Pointer to the USB host controller data
               IN  pRequest - Pointer to the request to remove
Return value:  none
******************************************************************************/
static void usbhSafeRemove(PUSBHC pUsbHc, PUSBTR pRequest)
{
    if (!usbhRemoveRequest(&pUsbHc->pControl, pRequest))
    {
        if (!usbhRemoveRequest(&pUsbHc->pBulk, pRequest))
        {
            if (!usbhRemoveRequest(&pUsbHc->pInterrupt, pRequest))
            {
                usbhRemoveRequest(&pUsbHc->pIsochronus, pRequest);
            }
        }
    }
}
/******************************************************************************
End of function  usbhSafeRemove
******************************************************************************/

/******************************************************************************
Function Name: usbhCreateEndpointInformation
Description:   Function to create the endpoint information
Arguments:     IN  pUsbEpDesc - Pointer to the endpoint descriptor
Return value:  Pointer to the endpoint or NULL on error
******************************************************************************/
static PUSBEI usbhCreateEndpointInformation(PUSBEP pUsbEpDesc)
{
    PUSBEI  pEndpoint = usbhAllocEndpoint();
    if (pEndpoint)
    {
        pEndpoint->wPacketSize = usbhGetWord(&pUsbEpDesc->wMaxPacketSize);
        pEndpoint->byEndpointNumber = (uint8_t)(pUsbEpDesc->bEndpointAddress & 
                                    ~ USB_ENDPOINT_TYPE_TX);
        TRACE(("Endpoint address %.2X, number %.2X, size %d\r\n",
                pUsbEpDesc->bEndpointAddress,
                pEndpoint->byEndpointNumber,
                pEndpoint->wPacketSize));
        pEndpoint->byInterval = pUsbEpDesc->bInterval;
        pEndpoint->transferType = (USBTT)
            (pUsbEpDesc->bAttributes & USB_ENDPOINT_TYPE_MASK);
        if (pUsbEpDesc->bEndpointAddress & USB_ENDPOINT_TYPE_TX)
        {
            pEndpoint->transferDirection = USBH_IN;
        }
        else
        {
            pEndpoint->transferDirection = USBH_OUT;
        }
    }
    
    return pEndpoint;
}
/******************************************************************************
End of function  usbhCreateEndpointInformation
******************************************************************************/

/******************************************************************************
Function Name: usbhCreatControlEndpointInformation
Description:   Function to create the default control pipe endpoint information
Arguments:     IN  pDevice - Pointer to the device information struct
               IN  wPacketSize - The endpoint packet size
               IN  transferDirection - The transfer direction
Return value:  Pointer to the control endpoint
******************************************************************************/
static PUSBEI usbhCreatControlEndpointInformation(PUSBDI   pDevice,
                                                  uint16_t     wPacketSize,
                                                  USBDIR   transferDirection)
{
    PUSBEI  pEndpoint = usbhAllocEndpoint();
    if (pEndpoint)
    {
        pEndpoint->pDevice = pDevice;
        pEndpoint->wPacketSize =  wPacketSize;
        pEndpoint->transferDirection = transferDirection;
    }
    return pEndpoint;
}
/******************************************************************************
End of function  usbhCreatControlEndpointInformation
******************************************************************************/

/******************************************************************************
Function Name: usbhGetWord
Description:   Function to get a word irrespective of alignment - also does
               endian
Arguments:     IN  pWord - Pointer to the word to get
Return value:  The word
******************************************************************************/
static uint16_t usbhGetWord(uint16_t * pWord)
{
    uint16_t    wReturn;
    uint8_t *   pbyByte = (uint8_t *)pWord;
    wReturn = (uint16_t)*pbyByte++;
    wReturn |= (uint16_t)((*pbyByte) << 8);
    return wReturn;
}
/******************************************************************************
End of function  usbhGetWord
******************************************************************************/

/******************************************************************************
Function Name: usbAllocHub
Description:   Function to allocate a hub
Arguments:     none
Return value:  Pointer to the hub or NULL on error
******************************************************************************/
static PUSBHI usbAllocHub(void)
{
    int iIndex;
    for (iIndex = 0; iIndex < USBH_MAX_HUBS; iIndex++)
    {
        if (!gpUsbHub[iIndex].bfAllocated)
        {
            gpUsbHub[iIndex].bfAllocated = true;
            return &gpUsbHub[iIndex];
        }
    }
    return NULL;
}
/******************************************************************************
End of function  usbAllocHub
******************************************************************************/

/******************************************************************************
Function Name: usbAllocPort
Description:   Function to allocate a port
Arguments:     none
Return value:  pointer to a port or NULL on error
******************************************************************************/
static PUSBPI usbAllocPort(void)
{
    int iIndex;
    for (iIndex = 0; iIndex < USBH_MAX_PORTS; iIndex++)
    {
        if (!gpUsbPort[iIndex].bfAllocated)
        {
            gpUsbPort[iIndex].bfAllocated = true;
            return &gpUsbPort[iIndex];
        }
    }
    return NULL;
}
/******************************************************************************
End of function  usbAllocPort
******************************************************************************/

/******************************************************************************
Function Name: usbhAllocDevice
Description:   Function to alocate a device
Arguments:     none
Return value:  pointer to a port or NULL on error
******************************************************************************/
static PUSBDI usbhAllocDevice(void)
{
    int iIndex;
    for (iIndex = 0; iIndex < USBH_MAX_DEVICES; iIndex++)
    {
        if (!gpUsbDevice[iIndex].bfAllocated)
        {
            gpUsbDevice[iIndex].bfAllocated = true;
            return &gpUsbDevice[iIndex];
        }
    }
    return NULL;
}
/******************************************************************************
End of function  usbhAllocDevice
******************************************************************************/

/******************************************************************************
Function Name: usbhAllocEndpoint
Description:   Function to allocate an endpoint
Arguments:     none
Return value:  Pointer to the endpoint or NULL on error
******************************************************************************/
static PUSBEI usbhAllocEndpoint(void)
{
    int iIndex;
    for (iIndex = 0; iIndex < USBH_MAX_ENDPOINTS; iIndex++)
    {
        if (!gpUsbEndpoint[iIndex].bfAllocated)
        {
            gpUsbEndpoint[iIndex].bfAllocated = true;
            return &gpUsbEndpoint[iIndex];
        }
    }
    return NULL;
}
/******************************************************************************
End of function  usbhAllocEndpoint
******************************************************************************/

/******************************************************************************
Function Name: usbhDestroyEndpointInformation
Description:   Function to free an endpoint information list
Arguments:     IN  pDevice - Pointer to the device
Return value:  none
******************************************************************************/
static void usbhDestroyEndpointInformation(PUSBDI pDevice)
{
    PUSBEI pNext, pEndpoint = pDevice->pEndpoint;
    while (pEndpoint)
    {
        pNext = pEndpoint->pNext;
        usbhFreeEndpoint(pEndpoint);
        pEndpoint = pNext;
    }
}
/******************************************************************************
End of function  usbhDestroyEndpointInformation
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
