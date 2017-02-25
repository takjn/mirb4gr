/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized. This
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
* © 2012-2013 Renesas Electronics Corporation All rights reserved.
*******************************************************************************/

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include "usbhDriverInternal.h"

//We are done with debugging. Disable it for this file.
//TODO: Remove all instructions under _DEBUG_
#undef _DEBUG_

#if defined(_DEBUG_)
#include "iodefine.h"
#endif
/******************************************************************************
Macro definitions
******************************************************************************/

#ifdef _TRACE_SETUP_
#define STRACE(_x_)          Trace _x_
#else
#define STRACE(_x_)			((void)0)
#endif

/******************************************************************************
Function Prototypes
******************************************************************************/

static _Bool usbhControlSetupPacket(PUSBTR pRequest);
static void usbhControlData(PUSBTR pRequest);
static _Bool usbhSetDevAddrCfg(PUSBDI pDevice);
static void usbhControlCancel(PUSBTR pRequest);

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/******************************************************************************
Function Name: usbhControlRequest
Description:   Function to process a control request
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  none
******************************************************************************/
void usbhControlRequest(PUSBTR pRequest)
{
    if ((pRequest)
    &&  (pRequest->bfInProgress == false))
    {
        /* Check that the request has valid pointers */
        if (pRequest->pEndpoint)
        {
            PUSBEI  pEndpoint = pRequest->pEndpoint;
            /* Set the cancel function */
            pRequest->pCancel = usbhControlCancel;
            /* Check the transfer direction */
            if (pEndpoint->transferDirection == USBH_SETUP)
            {
                /* Make sure that errors from a prevous setup packet are
                   cleared */
                R_USBH_ClearSetupRequest(pRequest->pUSB);
                /* Send the setup packet */
                if (usbhControlSetupPacket(pRequest))
                {
                    /* Enable the setup transaction interrupts */
                    R_USBH_EnableSetupInterrupts(pRequest->pUSB, true);
                }
                else
                {
                    PUSBHC  pUsbHc = pRequest->pUsbHc;
                    /* Set the error code and complete the request */
                    pRequest->errorCode = REQ_INVALID_PARAMETER;
                    /* Complete the request */
                    usbhComplete(pRequest);
                    pUsbHc->pCurrentControl = NULL;
                }
            }
            else
            {
                /* Handle the data phase */
                usbhControlData(pRequest);
            }
            /* Show that the request is in progress */
            pRequest->bfInProgress = true;
        }
        else
        {
            pRequest->errorCode = (USBEC)REQ_ENDPOINT_NOT_FOUND;
        }
    }
}
/******************************************************************************
End of function  usbhControlRequest
******************************************************************************/

/******************************************************************************
Function Name: usbhControlCompleteSetup
Description:   Function to complete a setup transaction from the SACK and SIGN
               interrupts
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  pRequest - Pointer to the request
               IN  errorCode - The error code
Return value:  none
******************************************************************************/
void usbhControlCompleteSetup(PUSB pUSB, PUSBTR pRequest, USBEC errorCode)
{
    if ((pRequest)
    &&  (pRequest->bfInProgress == true))
    {
#if defined(_DEBUG_)
        /* Using PC4 for checking the time difference for control setup transaction */
        PORTC.PODR.BIT.B4 = 0;
#endif
        PUSBHC  pUsbHc = pRequest->pUsbHc;
        /* Set the error code and complete the request */
        pRequest->errorCode = errorCode;
        /* Disable the interrupt requests */
        R_USBH_EnableSetupInterrupts(pUSB, false);
        /* Complete the request */
        usbhComplete(pRequest);
        pUsbHc->pCurrentControl = NULL;
    }
    else
    {
        R_USBH_EnableSetupInterrupts(pUSB, false);
    }
}
/******************************************************************************
End of function  usbhControlCompleteSetup
******************************************************************************/

/******************************************************************************
Function Name: usbhControlIn
Description:   Function to process data from the control IN phase from the
               PIPE0BRDYE interrupt
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  pRequest - Pointer to the transfer request list
Return value:  none
******************************************************************************/
void usbhControlIn(PUSB pUSB, PUSBTR pRequest)
{
    if ((pRequest)
    &&  (pRequest->bfInProgress == true))
    {
        PUSBHC  pUsbHc = pRequest->pUsbHc;
        uint8_t *pbyDest = pRequest->pMemory + pRequest->stIdx;
        size_t  stLength = pRequest->stLength - pRequest->stIdx;
        #if USBH_HIGH_SPEED_SUPPORT == 1
        /* Check for a slow device */
        if ((pRequest->pEndpoint->pDevice->transferSpeed != USBH_HIGH)
        &&  (stLength))
        {
            int iDelay = USBH_PIPE_0_ACCESS_DELAY;
            while (iDelay--)
            /* Wait */;
        }
        #endif
        /* Disable the control pipe */
        R_USBH_EnablePipe(pUSB, 0, false);
        /* Clear the ready status flag */
        R_USBH_ClearPipeInterrupt(pUSB,
                                  0,
                                  USBH_PIPE_BUFFER_READY);
        /* Read the FIFO */
        pRequest->pUsbHc->pPipeTrack[0].iFifoUsedCount++;
        pRequest->stTransferSize = R_USBH_ReadPipe(pUSB, 0, pbyDest, stLength);
        if (pRequest->stTransferSize != -1UL)
        {
            /* Update the index */
            pRequest->stIdx += pRequest->stTransferSize;
            /* Transfer may have been completed by length */
            if ((pRequest->stIdx == pRequest->stLength)
            /* or a short packet */
            ||  (pRequest->stTransferSize < pRequest->pEndpoint->wPacketSize))
            {
                /* Disable the buffer ready & not ready interrupts */
                R_USBH_SetPipeInterrupt(pUSB,
                                        0,
                                        USBH_PIPE_BUFFER_READY |
                                        USBH_PIPE_BUFFER_NOT_READY |
                                        USBH_PIPE_INT_DISABLE);
                /* Complete the request */
                pRequest->errorCode = USBH_NO_ERROR;
                usbhComplete(pRequest);
                pUsbHc->pCurrentControl = NULL;
#if defined(_DEBUG_)
                /* Using PC6 for checking the time difference for control-in data transaction */
                PORTC.PODR.BIT.B6 = 0;
#endif
            }
            else
            {
                /* Enable the control pipe for next packet */
                R_USBH_EnablePipe(pUSB, 0, true);
#if defined(_DEBUG_)
                /* Using PC6 for checking the time difference for control-in data transaction */
                PORTC.PODR.BIT.B6 ^= 1;
#endif
            }
        }
        else
        /* The FIFO was not available to read */
        {
            /* Disable the buffer ready interrupt */
            R_USBH_SetPipeInterrupt(pUSB,
                                    0,
                                    USBH_PIPE_BUFFER_READY |
                                    USBH_PIPE_BUFFER_NOT_READY |
                                    USBH_PIPE_INT_DISABLE);
            /* Set the error code */
            pRequest->errorCode = USBH_FIFO_READ_ERROR;
            /* Complete the request */
            usbhComplete(pRequest);
            pUsbHc->pCurrentControl = NULL;
#if defined(_DEBUG_)
                /* Using PC6 for checking the time difference for control data transaction */
                PORTC.PODR.BIT.B6 = 0;
#endif
        }
    }
    else
    {
        /* Disable the control pipe */
        R_USBH_EnablePipe(pUSB, 0, false);
        /* Clear the buffer interrupts */
        R_USBH_ClearPipeInterrupt(pUSB,
                                  0,
                                  USBH_PIPE_BUFFER_READY |
                                  USBH_PIPE_BUFFER_NOT_READY);
        /* Disable the buffer ready interrupt */
        R_USBH_SetPipeInterrupt(pUSB,
                                0,
                                USBH_PIPE_BUFFER_READY |
                                USBH_PIPE_BUFFER_NOT_READY |
                                USBH_PIPE_INT_DISABLE);
    }
}
/******************************************************************************
End of function  usbhControlIn
******************************************************************************/

/******************************************************************************
Function Name: usbhControlOut
Description:   Function to process the data from a control OUT phase
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  pRequest - Pointer to the transfer request list
Return value:  none
******************************************************************************/
void usbhControlOut(PUSB pUSB, PUSBTR pRequest)
{
    if ((pRequest)
    &&  (pRequest->bfInProgress == true))
    {
        PUSBHC  pUsbHc = pRequest->pUsbHc;
        /* Disable the control pipe */
        R_USBH_EnablePipe(pRequest->pUSB, 0, false);
        /* Clear the interrupt flag */
        R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                                  0,
                                  USBH_PIPE_BUFFER_EMPTY);
        /* Check for any data to be written */
        if (pRequest->stIdx < pRequest->stLength)
        {
            uint8_t *   pbySrc = pRequest->pMemory + pRequest->stIdx;
            size_t  stLength = pRequest->stLength - pRequest->stIdx;
            /* Write the next packet into the FIFO */
            pRequest->pUsbHc->pPipeTrack[0].iFifoUsedCount++;
            pRequest->stTransferSize = R_USBH_WritePipe(pUSB,
                                                        0,
                                                        pbySrc,
                                                        stLength);
            if (pRequest->stTransferSize != -1UL)
            {
                /* Update the index */
                pRequest->stIdx += pRequest->stTransferSize;
                /* Set the pipe endpoint to buffer to start transfer */
                R_USBH_EnablePipe(pUSB, 0, true);
#if defined(_DEBUG_)
                /* Using PC5 for checking the time difference for control-out data transaction */
                PORTC.PODR.BIT.B5 ^= 1;
#endif
            }
            else
            {
                pRequest->errorCode = USBH_FIFO_WRITE_ERROR;
                /* Complete the request */
                usbhComplete(pRequest);
                pUsbHc->pCurrentControl = NULL;
#if defined(_DEBUG_)
                /* Using PC5 for checking the time difference for control-out data transaction */
                PORTC.PODR.BIT.B5 = 0;
#endif
            }
        }
        /* Check for short packet termination */
        else if (pRequest->stTransferSize == pRequest->pEndpoint->wPacketSize)
        {
            /* Send a zero length packet */
            pRequest->pUsbHc->pPipeTrack[0].iFifoUsedCount++;
            if (R_USBH_WritePipe(pUSB, 0, NULL, 0UL) != -1UL)
            {
                pRequest->errorCode = USBH_NO_ERROR;
            }
            else
            {
                pRequest->errorCode = USBH_FIFO_WRITE_ERROR;
            }
            /* Disable the buffer empty interrupt */
            R_USBH_SetPipeInterrupt(pUSB,
                                    0,
                                    USBH_PIPE_BUFFER_EMPTY |
                                    USBH_PIPE_INT_DISABLE);
            /* Complete the request */
            usbhComplete(pRequest);
            pUsbHc->pCurrentControl = NULL;
#if defined(_DEBUG_)
                /* Using PC5 for checking the time difference for control data transaction */
                PORTC.PODR.BIT.B5 = 0;
#endif
        }
        else
        /* The last packet sent was short which terminates the transfer */
        {
            /* Complete the request */
            pRequest->errorCode = USBH_NO_ERROR;
            /* Disable the buffer empty interrupt */
            R_USBH_SetPipeInterrupt(pUSB,
                                    0,
                                    USBH_PIPE_BUFFER_EMPTY |
                                    USBH_PIPE_INT_DISABLE);
            usbhComplete(pRequest);
            pUsbHc->pCurrentControl = NULL;
#if defined(_DEBUG_)
                /* Using PC5 for checking the time difference for control data transaction */
                PORTC.PODR.BIT.B5 = 0;
#endif
        }
    }
    else
    {
        /* Disable the control pipe */
        R_USBH_EnablePipe(pUSB, 0, false);
        /* Clear the interrupt flag */
        R_USBH_ClearPipeInterrupt(pUSB,
                                  0,
                                  USBH_PIPE_BUFFER_EMPTY);
        /* Disable the buffer empty interrupt */
        R_USBH_SetPipeInterrupt(pUSB,
                                0,
                                USBH_PIPE_BUFFER_EMPTY |
                                USBH_PIPE_INT_DISABLE);        
    }
}
/******************************************************************************
End of function  usbhControlOut
******************************************************************************/

/******************************************************************************
Function Name: usbhControlDataError
Description:   Function to handle an error in the data phase of a control
               transfer
Arguments:     IN  pUSB - Pointer to the Host Controller
               IN  pRequest - Pointer to the request
Return value:  none
******************************************************************************/
void usbhControlDataError(PUSB pUSB, PUSBTR pRequest)
{
    if ((pRequest)
    &&  (pRequest->bfInProgress == true))
    {
        /* Set a default error code (PID set to NAK (0)) */
        USBEC errorCode = USBH_NOT_RESPONDING_ERROR;
        /* Disable the pipe */
        R_USBH_EnablePipe(pUSB, 0, false);
        /* Check for endpoint stall */
        if (R_USBH_GetPipeStall(pRequest->pUSB, 0))
        {
            errorCode = USBH_HALTED_ERROR;
        }
        /* Check the direction of the transfer */
       if (pRequest->pEndpoint->transferDirection == USBH_OUT)
        {
            /* Disable the interrupt request */
            R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                                      0,
                                      USBH_PIPE_BUFFER_EMPTY);
            R_USBH_SetPipeInterrupt(pRequest->pUSB,
                                      0,
                                      USBH_PIPE_BUFFER_EMPTY |
                                      USBH_PIPE_INT_DISABLE);
        }
        else
        {
            /* Disable the interrupt requests */
            R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                                      0,
                                      USBH_PIPE_BUFFER_READY |
                                      USBH_PIPE_BUFFER_NOT_READY);
            R_USBH_SetPipeInterrupt(pRequest->pUSB,
                                    0,
                                    USBH_PIPE_BUFFER_READY |
                                    USBH_PIPE_BUFFER_NOT_READY |
                                    USBH_PIPE_INT_DISABLE);
        }
        /* Set the error code and complete the request */
        pRequest->errorCode = errorCode;
        /* Cancel all other requests for this device now */
        usbhCancelAllControlRequests(pRequest->pEndpoint->pDevice);
#if defined(_DEBUG_)
                /* Using PC5 for checking the time difference for control-out data transaction */
                PORTC.PODR.BIT.B5 = 0;
                /* Using PC6 for checking the time difference for control-in setup transaction */
                PORTC.PODR.BIT.B6 = 0;
#endif
    }
    else
    {
        /* Disable the interrupt requests */
        R_USBH_ClearPipeInterrupt(pUSB,
                                  0,
                                  USBH_PIPE_BUFFER_EMPTY |
                                  USBH_PIPE_BUFFER_READY |
                                  USBH_PIPE_BUFFER_NOT_READY);
        R_USBH_SetPipeInterrupt(pUSB,
                                0,
                                USBH_PIPE_BUFFER_EMPTY |
                                USBH_PIPE_BUFFER_READY |
                                USBH_PIPE_BUFFER_NOT_READY |
                                USBH_PIPE_INT_DISABLE);
    }
}
/******************************************************************************
End of function  usbhControlDataError
******************************************************************************/

/******************************************************************************
Function Name: usbhControlPipeIdle
Description:   Function to return true if pipe has been idle since the last
               call
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  true if the pipe has been idle
******************************************************************************/
_Bool usbhControlPipeIdle(PUSBTR pRequest)
{
    if (pRequest->pUsbHc->pPipeTrack[0].iFifoUsedCount)
    {
        pRequest->pUsbHc->pPipeTrack[0].iFifoUsedCount = 0;
        return false;
    }
    return true;
}
/******************************************************************************
End of function  usbhControlPipeIdle
******************************************************************************/

/******************************************************************************
Private global variables and functions
******************************************************************************/

/******************************************************************************
Function Name: usbhControlSetupPacket
Description:   Function to handle a setup packet
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  true if the request was processed succesfully
******************************************************************************/
static _Bool usbhControlSetupPacket(PUSBTR pRequest)
{
    PUSBEI  pEndpoint = pRequest->pEndpoint;
    PUSBDI  pDevice = pEndpoint->pDevice;
    /* Clear the buffer */
    R_USBH_ClearPipeFifo(pRequest->pUSB, 0);
    /* Check to see if the device is attached to a root port */
    if (pDevice->pPort->pRoot)
    {
        /* Set the SOF output for a low speed device */
        R_USBH_SetSOF_Speed(pRequest->pUSB, pDevice->transferSpeed);
    }
    /* Set the device Address Configuration Register */
    if (usbhSetDevAddrCfg(pDevice))
    {
        PUSBDR  pDevReq = (PUSBDR)pRequest->pMemory;
#if defined(_DEBUG_)
        /* Using PC4 for checking the time difference for control setup transaction */
        PORTC.PODR.BIT.B4 = 1;
#endif
        /* Submit the request */
        R_USBH_SendSetupPacket(pRequest->pUSB,
                               pDevice->byAddress,
                               pEndpoint->wPacketSize,
                               pDevReq->bmRequestType,
                               pDevReq->bRequest,
                               pDevReq->wValue,
                               pDevReq->wIndex,
                               pDevReq->wLength);
        return true;
    }
    /* The address was not valid! */
    pRequest->errorCode = (USBEC)REQ_INVALID_ADDRESS;
    return false;
}
/******************************************************************************
End of function  usbhControlSetupPacket
******************************************************************************/

/******************************************************************************
Function Name: usbhControlData
Description:   Function to handle control data phase start
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  none
******************************************************************************/
static void usbhControlData(PUSBTR pRequest)
{
    PUSBEI  pEndpoint = pRequest->pEndpoint;
    /* Set the PID */
    R_USBH_SetPipePID(pRequest->pUSB, 0, pEndpoint->dataPID);
    /* Check the transfer direction */
    if (pEndpoint->transferDirection == USBH_OUT)
    {
#if defined(_DEBUG_)
                /* Using PC5 for checking the time difference for control-out data transaction */
                PORTC.PODR.BIT.B5 = 1;
#endif
        /* Set the transfer direction */
        R_USBH_SetControlPipeDirection(pRequest->pUSB, USBH_OUT);
        /* Write the first packet into the FIFO - even if it is zero length */
        pRequest->pUsbHc->pPipeTrack[0].iFifoUsedCount++;
        pRequest->stTransferSize = R_USBH_WritePipe(pRequest->pUSB,
                                                    0,
                                                    pRequest->pMemory,
                                                    pRequest->stLength);
        if (pRequest->stTransferSize != -1UL)
        {
            /* Update the index */
            pRequest->stIdx += pRequest->stTransferSize;
            /* Clear the buffer empty status */
            R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                                      0,
                                      USBH_PIPE_BUFFER_EMPTY);
            /* Enable the buffer empty interrupt */
            R_USBH_SetPipeInterrupt(pRequest->pUSB,
                                    0,
                                    USBH_PIPE_BUFFER_EMPTY);
        }
        else
        {
            PUSBHC  pUsbHc = pRequest->pUsbHc;
            /* Error accessing FIFO for write */
            pRequest->errorCode = USBH_FIFO_WRITE_ERROR;
#if defined(_DEBUG_)
                /* Using PC5 for checking the time difference for control-out data transaction */
                PORTC.PODR.BIT.B5 = 0;
#endif
            /* Complete the request */
            usbhComplete(pRequest);
            pUsbHc->pCurrentControl = NULL;
            return;
        }
    }
    else
    {
#if defined(_DEBUG_)
                /* Using PC6 for checking the time difference for control-in data transaction */
                PORTC.PODR.BIT.B6 = 1;
#endif
        /* Set the transfer direction */
        R_USBH_SetControlPipeDirection(pRequest->pUSB, USBH_IN);
        /* Enable the buffer ready & not ready interrupts */
        R_USBH_SetPipeInterrupt(pRequest->pUSB,
                                0,
                                USBH_PIPE_BUFFER_READY |
                                USBH_PIPE_BUFFER_NOT_READY);
    }
    /* Set the pipe endpoint to buffer to start transfer */
    R_USBH_EnablePipe(pRequest->pUSB, 0, true);
}
/******************************************************************************
End of function  usbhControlData
******************************************************************************/

/******************************************************************************
Function Name: usbhSetDevAddrCfg
Description:   Function to set the appropriate Device Address Configuration
               Register for a given device
Arguments:     IN  pDevice - Pointer to the device information
Return value:  true if succesful
******************************************************************************/
static _Bool usbhSetDevAddrCfg(PUSBDI pDevice)
{
    uint8_t *pbyHubAddress = NULL;
    uint8_t byHubPort = 0;
    uint8_t byRootPort;
    /* Check to see if this device is on a hub */
    if (pDevice->pPort->pHub)
    {
        /* Set the pointer to the hub address */
        pbyHubAddress = &pDevice->pPort->pHub->byHubAddress;
        /* Set the index of the port on the hub */
        byHubPort = (uint8_t)pDevice->pPort->uiPortIndex;
        /* Set the index of the root port to which the hub is attached */
        byRootPort = (uint8_t)pDevice->pPort->pHub->pPort->uiPortIndex;
    }
    /* If the port is not on a hub then it must be on the root */
    else
    {
        /* Set index of the port which the device is attached to */
        byRootPort = (uint8_t)pDevice->pPort->pRoot->uiPortIndex;
    }
    /* Set the address configuration registers for this device */
    return R_USBH_SetDevAddrCfg(pDevice->pPort->pUSB,
                                pDevice->byAddress,
                                pbyHubAddress,
                                byHubPort,
                                byRootPort,
                                pDevice->transferSpeed);
}
/******************************************************************************
End of function  usbhSetDevAddrCfg
******************************************************************************/

/******************************************************************************
Function Name: usbhControlCancel
Description:   
Arguments:     IN  pRequest - Pointer to the request
Return value:  
******************************************************************************/
static void usbhControlCancel(PUSBTR pRequest)
{
    /* Disable the control pipe */
    R_USBH_EnablePipe(pRequest->pUSB, 0, false);
    /* Disable all interrupts */
    R_USBH_SetPipeInterrupt(pRequest->pUSB,
                            0,
                            USBH_PIPE_BUFFER_EMPTY |
                            USBH_PIPE_BUFFER_READY |
                            USBH_PIPE_BUFFER_NOT_READY |
                            USBH_PIPE_INT_DISABLE);
    /* Clear the interrupt status flags */
    R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                              0,
                              USBH_PIPE_BUFFER_EMPTY |
                              USBH_PIPE_BUFFER_READY |
                              USBH_PIPE_BUFFER_NOT_READY);
    /* Clear the setup interrupts */
    R_USBH_EnableSetupInterrupts(pRequest->pUSB, false);
    /* Show that the request has been cancelled */
    pRequest->pUsbHc->pCurrentControl = NULL;
}
/******************************************************************************
End of function  usbhControlCancel
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
