/* INSERT LICENSE HERE */

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include <string.h>
#if !defined(__GNUC__) && !defined(GRSAKURA)
#include <machine.h>
#endif
#include "usbhDriverInternal.h"

/******************************************************************************
Macro definitions
******************************************************************************/

/* Comment this line out to turn ON module trace in this file */
//REA Crashes traces!!!
//#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
Private global variables and functions
******************************************************************************/

/******************************************************************************
Constant Data
******************************************************************************/

/* The pipe type ranges */
static const struct _PTRNG
{
    int iFirstPipe;
    int iLastPipe;

} gciPipeRange[] =
{
    /* USBH_CONTROL */
    {0, 0,},
    /* USBH_ISOCHRONOUS */
    {1, 2,},
    /* USBH_BULK */
    {1, 5,},
    /* USBH_INTERRUPT */
    {6, 9},
};

/******************************************************************************
Function Prototypes
******************************************************************************/

static int usbhAllocPipeFromRange(PUSBTR pRequest, int iPipeStart, int iPipeEnd);
static int usbhConfigurePipe(PUSBHC pUsbHc, PUSBEI pEndpoint, int iPipeNumber);

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/******************************************************************************
Function Name: usbhAllocPipeNumber
Description:   Function to get a pipe number (1 to 9) suitable for the endpoint
               and configures the pipe control register accordingly
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  The pipe number or -1 if no available pipe
               (or wrong transfer type)
******************************************************************************/
int usbhAllocPipeNumber(PUSBTR pRequest)
{
    USBTT   transferType = pRequest->pEndpoint->transferType;
    int     iPipeNumber;
    /* The host controller only has two pipes capable of isoc transfers */
    if (transferType == USBH_ISOCHRONOUS)
    {
        /* Always assign one pipe to OUT transfers and one pipe to IN
           transfers */
        if (pRequest->pEndpoint->transferDirection == USBH_OUT)
        {
            /* Always use pipe 2 for OUT transfers */
            iPipeNumber = usbhAllocPipeFromRange(pRequest,
                                                 2,
                                                 2);
        } 
        else
        {
            /* Always use pipe 1 for IN transfers */
            iPipeNumber = usbhAllocPipeFromRange(pRequest,
                                                 1,
                                                 1);
        }
    }
    else
    {
        iPipeNumber = usbhAllocPipeFromRange(pRequest,
                                             gciPipeRange[transferType].iFirstPipe,
                                             gciPipeRange[transferType].iLastPipe);
    }
    TRACE(("Pipe Allocated: %d\r\n", iPipeNumber));
    return iPipeNumber;
}
/******************************************************************************
End of function  usbhAllocPipeNumber
******************************************************************************/

/******************************************************************************
Function Name: usbhFreePipeNumber
Description:   Function to free a pipe so it can be used for another transfer
Arguments:     IN  pUsbHc - Pointer to Host Controller data
               IN  iPipeNumber - The number of the pipe
Return value:  none
******************************************************************************/
void usbhFreePipeNumber(PUSBHC pUsbHc, int iPipeNumber)
{
    /* Unconfigure the pipe */
    R_USBH_UnconfigurePipe(pUsbHc->pPort->pUSB, iPipeNumber);
    /* Cache the endpoint so the same pipe can be allocated on the next
       transfer */
    pUsbHc->pEndpointAssign[iPipeNumber].pEndpointCache =
        pUsbHc->pEndpointAssign[iPipeNumber].pRequest->pEndpoint;
    /* Clear the assignment */
    pUsbHc->pEndpointAssign[iPipeNumber].pRequest = NULL;
    /* Mark the pipe as unallocated */
    pUsbHc->pEndpointAssign[iPipeNumber].bfAllocated = false;
    TRACE(("usbhFreePipeNumber: P%d\r\n\r\n", iPipeNumber));
}
/******************************************************************************
End of function  usbhFreePipeNumber
******************************************************************************/

/******************************************************************************
Function Name: usbhTransferError
Description:   Function to handle a transfer error
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  none
******************************************************************************/
void usbhTransferError(PUSBTR pRequest)
{
    int iPipeNumber = (int)pRequest->pInternal;
    /* This is called from the NRDY interrupt, which could be because of any
       of the following conditions:
       1. When STALL is received from the peripheral side for the issued token
       2. When a response cannot be received correctly from the peripheral side
          for the issued token (No response is returned three consecutive times
          or a packet reception error occurred three consecutive times.)
       3. When an overrun/underrun occurred during isochronous transfer. */
    /* Check to see if this is a stall */
    if (R_USBH_GetPipeStall(pRequest->pUSB, iPipeNumber))
    {
        /* Set the stall error */
        pRequest->errorCode = USBH_HALTED_ERROR;
        /* Complete the request */
        usbhCancelTransfer(pRequest);
        TRACE(("usbhTransferError:\r\n"));
    }
    if (pRequest->pEndpoint->transferType != USBH_ISOCHRONOUS)
    {
        /* Set the a generic error */
        pRequest->errorCode = USBH_NOT_RESPONDING_ERROR;
        /* Complete the request */
        usbhCancelTransfer(pRequest);
        TRACE(("usbhTransferError: Who knows what really happened\r\n"));
    }
    /*
       We don't care about isochronous overruns. This could be because the
       device is "slow" and has not responded to the issued token because it is
       busy. Don't cancel the request - let it time-out */
}
/******************************************************************************
End of function  usbhTransferError
******************************************************************************/

/******************************************************************************
Function Name: usbhPipeIdle
Description:   Function to return true if pipe has been idle since the last
               call
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The number of the pipe to check
Return value:  true if the pipe has been idle
******************************************************************************/
_Bool usbhPipeIdle(PUSBTR pRequest, int iPipeNumber)
{
    if (pRequest->pUsbHc->pPipeTrack[iPipeNumber].iFifoUsedCount)
    {
        pRequest->pUsbHc->pPipeTrack[iPipeNumber].iFifoUsedCount = 0;
        return false;
    }
    return true;
}
/******************************************************************************
End of function  usbhPipeIdle
******************************************************************************/

/******************************************************************************
Function Name: usbhContinueInFifo
Description:   Function to continue an IN FIFO transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/
_Bool usbhContinueInFifo(PUSBTR pRequest, int iPipeNumber)
{
    uint8_t *pbyDest = pRequest->pMemory + pRequest->stIdx;
    size_t  stLengthToRead = pRequest->stLength - pRequest->stIdx;
    /* Read data from the pipe */
    pRequest->stTransferSize = R_USBH_ReadPipe(pRequest->pUSB,
                                               iPipeNumber,
                                               pbyDest,
                                               stLengthToRead);
    /* Clear the buffer ready status */
    R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                              iPipeNumber,
                              USBH_PIPE_BUFFER_READY);
    switch (pRequest->stTransferSize)
    {
        /* More data in FIFO than room for in destination memory */
        case -2UL:
        TRACE(("USBH_DATA_OVERRUN_ERROR_1\r\n"));
        /* Set the error code */
        pRequest->errorCode = USBH_DATA_OVERRUN_ERROR;
        /* Clear the buffer of any data */
        R_USBH_ClearPipeFifo(pRequest->pUSB, iPipeNumber);
        /* Complete the request */
        usbhCompleteInFifo(pRequest, iPipeNumber);
        break;
        /* SIE would not release the FIFO */
        case -1UL:
        /* Thanks to Martin Baker for fixing this one */
        TRACE(("USBH_FIFO_READ_ERROR\r\n"));
        /* Clear the buffer of any data */
        R_USBH_ClearPipeFifo(pRequest->pUSB, iPipeNumber);
        /* Enable the pipe for next packet */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);
        /* The transfer is to continue */
        TRACE(("usbhContinueInFifo: Continue\r\n"));
        return true;
        /* A packet was transferred */
        default:
        /* Show that this request is not idle */
        pRequest->pUsbHc->pPipeTrack[iPipeNumber].iFifoUsedCount++;
        /* Update the index */
        pRequest->stIdx += pRequest->stTransferSize;
        /* Transfer may have been completed by length */
        if ((pRequest->stIdx == pRequest->stLength)
        /* or a short packet */
        ||  (pRequest->stTransferSize < pRequest->pEndpoint->wPacketSize))
        {
            /* Set the error code */
            pRequest->errorCode = USBH_NO_ERROR;
            /* Complete the request */
            usbhCompleteInFifo(pRequest, iPipeNumber);
        }
        else
        {
            /* Enable the pipe for next packet */
            R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);
            /* The transfer is to continue */
            TRACE(("usbhContinueInFifo: Continue\r\n"));
            return true;
        }
        break;
    }
    return false;
}
/******************************************************************************
End of function  usbhContinueInFifo
******************************************************************************/

/******************************************************************************
Function Name: usbhCompleteInFifo
Description:   Function to complete an IN transfer by FIFO
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/
void usbhCompleteInFifo(PUSBTR pRequest, int iPipeNumber)
{
    /* Clear any pending interrupts */
    R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                              iPipeNumber,
                              (USBIP)(USBH_PIPE_BUFFER_READY |
                              USBH_PIPE_BUFFER_NOT_READY));
    /* Disable the interrupts */
    R_USBH_SetPipeInterrupt(pRequest->pUSB,
                            iPipeNumber,
                            (USBIP)(USBH_PIPE_BUFFER_READY |
                            USBH_PIPE_BUFFER_NOT_READY |
                            USBH_PIPE_INT_DISABLE));
    /* Update the endpoint data PID toggle bit */
    pRequest->pEndpoint->dataPID = R_USBH_GetPipePID(pRequest->pUSB,
                                                     iPipeNumber);
    /* Free the pipe for use by another transfer */
    usbhFreePipeNumber(pRequest->pUsbHc, iPipeNumber);
    /* Complete the request */
    usbhComplete(pRequest);
    TRACE(("usbhCompleteInFifo:\r\n\r\n"));
}
/******************************************************************************
End of function  usbhCompleteInFifo
******************************************************************************/

/******************************************************************************
Function Name: usbhCompleteByFIFO
Description:   Function to complete the transaction by FIFO
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number
Return value:  none
******************************************************************************/
void usbhCompleteByFIFO(PUSBTR  pRequest, int iPipeNumber)
{
    /* Enable the buffer ready interrupt to complete the transfer by FIFO &
       Enable the not ready interrupt for detection of a STALL condition */
    R_USBH_SetPipeInterrupt(pRequest->pUSB,
                            iPipeNumber,
                            (USBIP)(USBH_PIPE_BUFFER_READY |
                            USBH_PIPE_BUFFER_NOT_READY));
    /* Clear any pending interrupts */
    R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                              iPipeNumber,
                              (USBIP)(USBH_PIPE_BUFFER_READY |
                              USBH_PIPE_BUFFER_NOT_READY));
    /* Set the cancel function */
    pRequest->pCancel = usbhCancelInFifo;
    /* Disable the double buffer bit by reallocating the pipe */
    usbhFreePipeNumber(pRequest->pUsbHc, iPipeNumber);
    iPipeNumber = usbhAllocPipeNumber(pRequest);
    pRequest->pInternal = (void*)iPipeNumber;
    /* Enable the pipe for next packet */
    R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);
    /* The transfer is to continue and complete by FIFO */
    TRACE(("usbhCompleteByFIFO: Continue\r\n"));
}
/******************************************************************************
End of function  usbhCompleteByFIFO
******************************************************************************/

/******************************************************************************
Function Name: usbhCancelInFifo
Description:   Function to cancel a bulk in transfer
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  none
******************************************************************************/
void usbhCancelInFifo(PUSBTR pRequest)
{
    int     iPipeNumber = (int)pRequest->pInternal;
    if (iPipeNumber)
    {
        /* Clear the pending interrupts */
        R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                                  iPipeNumber,
                                  (USBIP)(USBH_PIPE_BUFFER_READY |
                                  USBH_PIPE_BUFFER_NOT_READY));
        /* Disable the interrupts */
        R_USBH_SetPipeInterrupt(pRequest->pUSB,
                                iPipeNumber,
                                (USBIP)(USBH_PIPE_BUFFER_READY |
                                USBH_PIPE_BUFFER_NOT_READY |
                                USBH_PIPE_INT_DISABLE));
        /* Set the pipe to NAK */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, false);
        /* Update the endpoint data PID toggle bit */
        pRequest->pEndpoint->dataPID = R_USBH_GetPipePID(pRequest->pUSB,
                                                         iPipeNumber);
        /* Free the pipe for use by another transfer */
        usbhFreePipeNumber(pRequest->pUsbHc, iPipeNumber);
        TRACE(("usbhCancelInFifo:\r\n"));
    }
    else
    {
        TRACE(("usbhCancelOutFifo: Invalid pipe number\r\n"));
    }
}
/******************************************************************************
End of function  usbhCancelInFifo
******************************************************************************/

/******************************************************************************
Function Name: usbhCompleteOutFifo
Description:   Function to complete an out FOFO transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  none
******************************************************************************/
void usbhCompleteOutFifo(PUSBTR pRequest, int iPipeNumber)
{
    /* Clear any pending interrupts */
    R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                              iPipeNumber,
                              (USBIP)(USBH_PIPE_BUFFER_EMPTY |
                              USBH_PIPE_BUFFER_NOT_READY));
    /* Disable the interrupts */
    R_USBH_SetPipeInterrupt(pRequest->pUSB,
                            iPipeNumber,
                            (USBIP)(USBH_PIPE_BUFFER_EMPTY |
                            USBH_PIPE_BUFFER_NOT_READY |
                            USBH_PIPE_INT_DISABLE));
    /* Update the endpoint data PID toggle bit */
    pRequest->pEndpoint->dataPID = R_USBH_GetPipePID(pRequest->pUSB,
                                                     iPipeNumber);
    /* Free the pipe for use by another transfer */
    usbhFreePipeNumber(pRequest->pUsbHc, iPipeNumber);
    /* Complete the transfer request */
    usbhComplete(pRequest);
    TRACE(("usbhCompleteOutFifo: Done\r\n\r\n"));
}
/******************************************************************************
End of function  usbhCompleteOutFifo
******************************************************************************/

/******************************************************************************
Function Name: usbhCancelOutFifo
Description:   Function to cancel an out FIFO transfer
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  none
******************************************************************************/
void usbhCancelOutFifo(PUSBTR pRequest)
{
    int     iPipeNumber = (int)pRequest->pInternal;
    if (iPipeNumber)
    {
        /* Clear the buffer empty interrupt */
        R_USBH_SetPipeInterrupt(pRequest->pUSB,
                                iPipeNumber,
                                (USBIP)(USBH_PIPE_BUFFER_EMPTY |
                                USBH_PIPE_BUFFER_NOT_READY |
                                USBH_PIPE_INT_DISABLE));
        /* Set the pipe to NAK */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, false);
        /* Update the endpoint data PID toggle bit */
        pRequest->pEndpoint->dataPID = R_USBH_GetPipePID(pRequest->pUSB,
                                                         iPipeNumber);
        /* Free the pipe for use by another transfer */
        usbhFreePipeNumber(pRequest->pUsbHc, iPipeNumber);
    }
    else
    {
        TRACE(("usbhCancelOutFifo: Invalid pipe number\r\n"));
    }
}
/******************************************************************************
End of function  usbhCancelOutFifo
******************************************************************************/

/******************************************************************************
Private Functions
******************************************************************************/

/******************************************************************************
Function Name: usbhAllocPipeFromRange
Description:   Function to allocate a pipe from the specified range if possible
               the same pipe is allocated to a request. 
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeStart - The starting pipe number
               IN  iPipeEnd - The ending pipe number
Return value:  The pipe number or -1 if no available pipe (or wrong transfer
               type)
******************************************************************************/
static int usbhAllocPipeFromRange(PUSBTR pRequest,
                                  int iPipeStart,
                                  int iPipeEnd)
{
    if (iPipeStart)
    {
        int     iPipeNumber = iPipeStart;
        PUSBEI  pEndpoint = pRequest->pEndpoint;
        PASGN   thisPipeDirection;
        struct _EPASSIGN *pEndpointAssign = pRequest->pUsbHc->pEndpointAssign;
        /* First look in the request cache */
        while (iPipeNumber <= iPipeEnd)
        {
            /* If it is here and not allocated then use the same pipe */
            if ((pEndpointAssign[iPipeNumber].pEndpointCache ==
                                                          pRequest->pEndpoint)
            &&  (!pEndpointAssign[iPipeNumber].bfAllocated))
            {
                pEndpointAssign[iPipeNumber].pRequest = pRequest;
                return usbhConfigurePipe(pRequest->pUsbHc,
                                         pEndpoint,
                                         iPipeNumber);
            }
            iPipeNumber++;
        }
        /* Second don't always allocate the same pipe */
        iPipeNumber = iPipeStart;
        while (iPipeNumber <= iPipeEnd)
        {
            /* Look for a pipe that has not been used before */
            if ((pEndpointAssign[iPipeNumber].pipeAssign == PIPE_NOT_ASSIGNED)
            &&  (!pEndpointAssign[iPipeNumber].bfAllocated))
            {
                pEndpointAssign[iPipeNumber].pRequest = pRequest;
                return usbhConfigurePipe(pRequest->pUsbHc,
                                         pEndpoint,
                                         iPipeNumber);
            }
            iPipeNumber++;
        }
        if (pEndpoint->transferDirection == USBH_IN)
        {
            thisPipeDirection = PIPE_IN;
        }
        else
        {
            thisPipeDirection = PIPE_OUT;
        }
        /* Thirdly look for a pipe which has been used in the same direction */
        iPipeNumber = iPipeStart;
        while (iPipeNumber <= iPipeEnd)
        {
            /* Look for a pipe which has been used in the same direction */
            if ((pEndpointAssign[iPipeNumber].pipeAssign == thisPipeDirection)
            &&  (!pEndpointAssign[iPipeNumber].bfAllocated))
            {
                pEndpointAssign[iPipeNumber].pRequest = pRequest;
                return usbhConfigurePipe(pRequest->pUsbHc,
                                         pEndpoint,
                                         iPipeNumber);
            }
            iPipeNumber++;
        }
        /* Finally look for the first free pipe */
        iPipeNumber = iPipeStart;
        while (iPipeNumber <= iPipeEnd)
        {
            if (!pEndpointAssign[iPipeNumber].bfAllocated)
            {
                pEndpointAssign[iPipeNumber].pRequest = pRequest;
                return usbhConfigurePipe(pRequest->pUsbHc,
                                         pEndpoint,
                                         iPipeNumber);
            }
            iPipeNumber++;
        }
    }
    /* There is no pipe available */
    return -1;
}
/******************************************************************************
End of function  usbhAllocPipeFromRange
******************************************************************************/

/******************************************************************************
Function Name: usbhConfigurePipe
Description:   Function to configure the pipe for the endpoint's transfer
               characteristics
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
               IN  pEndpoint - Pointer to the endpoint information
               IN  iPipeNumber - The index of the endpoint to configure
Return value:  The pipe number or -1 on error
******************************************************************************/
static int usbhConfigurePipe(PUSBHC pUsbHc, PUSBEI pEndpoint, int iPipeNumber)
{
    int         iResult;
    uint16_t    wPacketSize;
    /* Set the transfer direction */
    if (pEndpoint->transferDirection == USBH_OUT)
    {
        /* Show that this pipe has been used for OUT transactions */
        pUsbHc->pEndpointAssign[iPipeNumber].pipeAssign = PIPE_OUT;
    }
    else if (pEndpoint->transferDirection == USBH_IN)
    {
        /* Show that this pipe has been used for IN transactions */
        pUsbHc->pEndpointAssign[iPipeNumber].pipeAssign = PIPE_IN;
    }
    /* Set the transfer type */
    if (pEndpoint->transferType == USBH_ISOCHRONOUS)
    {
        if (!pUsbHc->pEndpointAssign[iPipeNumber].pRequest->pIsocPacketSize)
        {
            /* Set the packet size */
            wPacketSize = pEndpoint->wPacketSize;
        }
        else
        {
            /* Get a pointer to the packet size schedule */
            PUSBIV  pIsocPacketSize =
                pUsbHc->pEndpointAssign[iPipeNumber].pRequest->pIsocPacketSize;
            pIsocPacketSize->wPacketSize =
                pIsocPacketSize->pwPacketSizeList[pIsocPacketSize->iScheduleIndex];
            /* Set the packet size to the current required size */
            wPacketSize = pIsocPacketSize->wPacketSize;
        }
    }
    else
    {
        /* Set the packet size */
        wPacketSize = pEndpoint->wPacketSize;
    }
    /* Configure the hardware */
    iResult = R_USBH_ConfigurePipe(pUsbHc->pPort->pUSB,
                                   iPipeNumber,
                                   pEndpoint->transferType,
                                   pEndpoint->transferDirection,
                                   pEndpoint->pDevice->byAddress,
                                   pEndpoint->byEndpointNumber,
                                   pEndpoint->byInterval,
                                   wPacketSize);
    if (iResult == iPipeNumber)
    {
        /* Mark the pipe as allocated */
        pUsbHc->pEndpointAssign[iPipeNumber].bfAllocated = true;
        return iPipeNumber;
    }
    else
    {
        return -1;
    }
}
/******************************************************************************
End of function  usbhConfigurePipe
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
