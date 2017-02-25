/* INSERT LICENSE HERE */

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "usbhDriverInternal.h"
#if !defined(__GNUC__) && !defined(GRSAKURA)
#include "machine.h"
#endif
/******************************************************************************
Macro definitions
******************************************************************************/

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#endif

/******************************************************************************
Function Prototypes
******************************************************************************/

static _Bool usbhStartInterruptInTransfer(PUSBTR pRequest, int iPipeNumber);
static _Bool usbhContinueInterruptInFifo(PUSBTR pRequest, int iPipeNumber);
static void usbhCompleteInterruptInFifo(PUSBTR pRequest, int iPipeNumber);
static _Bool usbhStartInterruptOutTransfer(PUSBTR pRequest, int iPipeNumber);
static void usbhContinueInterruptOutFifo(PUSBTR pRequest, int iPipeNumber);

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/******************************************************************************
Function Name: usbhStartInterruptTransfer
Description:   Function to start an interrupt transfer
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  true if the transfer was started
******************************************************************************/
_Bool usbhStartInterruptTransfer(PUSBTR pRequest)
{
    if ((pRequest)
    &&  (pRequest->bfInProgress == false))
    {
        /* Allocate a pipe for this transfer */
        int iPipeNumber = usbhAllocPipeNumber(pRequest);
        /* If there is a pipe available */
        if (iPipeNumber != -1)
        {
            PUSBEI  pEndpoint = pRequest->pEndpoint;
            /* Set the pipe number */
            pRequest->pInternal = (void*)iPipeNumber;
            /* Initialise the FIFO used count - for idle time-out */
            pRequest->pUsbHc->pPipeTrack[iPipeNumber].iFifoUsedCount = 0;
            /* Set the pipe to NAK */
            R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, false);
            /* Set the PID */
            R_USBH_SetPipePID(pRequest->pUSB, iPipeNumber, pEndpoint->dataPID);
            /* Check the transfer direction */
            if (pEndpoint->transferDirection == USBH_IN)
            {
                /* Start the interrupt in transfer */
                return usbhInterruptIn(pRequest, iPipeNumber);
            }
            else
            {
                /* Start the interrupt out transfer */
                _Bool    bfResult = usbhInterruptOut(pRequest, iPipeNumber);
                /* Check to see if the transfer was started */
                if (!bfResult)
                {
                    /* Free the pipe */
                    
                    usbhFreePipeNumber(pRequest->pUsbHc, iPipeNumber);
                }
                return bfResult;
            }
        }
        TRACE(("usbhStartInterruptTransfer: Failed to start transfer\r\n"));
    }
    return false;
}
/******************************************************************************
End of function  usbhStartInterruptTransfer
******************************************************************************/

/******************************************************************************
Function Name: usbhInterruptIn
Description:   Function to handle an interrupt in transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/
_Bool usbhInterruptIn(PUSBTR pRequest, int iPipeNumber)
{
    if (pRequest)
    {
        /* Disable the pipe */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, false);
        /* If it is not in progress then start it */
        if (pRequest->bfInProgress == false)
        {
            return usbhStartInterruptInTransfer(pRequest, iPipeNumber);
        }
        else
        {
            return usbhContinueInterruptInFifo(pRequest, iPipeNumber);
        }
    }
    return false;
}
/******************************************************************************
End of function  usbhInterruptIn
******************************************************************************/

/******************************************************************************
Function Name: usbhInterruptOut
Description:   Function to handle an interrupt out transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/
_Bool usbhInterruptOut(PUSBTR pRequest, int iPipeNumber)
{
    if (pRequest)
    {
        /* Disable the pipe */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, false);
        /* If it is not in progress then start it */
        if ((pRequest->bfInProgress == false)
        &&  (R_USBH_PipeReady(pRequest->pUSB, iPipeNumber)))
        {
            return usbhStartInterruptOutTransfer(pRequest, iPipeNumber);
        }
        /* Continue or complete a FIFO transfer */
        else
        {
            if (R_USBH_PipeReady(pRequest->pUSB, iPipeNumber))
            {
                usbhContinueInterruptOutFifo(pRequest, iPipeNumber);
            }
            return true;
        }
    }
    return false;
}
/******************************************************************************
End of function  usbhInterruptOut
******************************************************************************/

/******************************************************************************
Private global variables and functions
******************************************************************************/

/******************************************************************************
Function Name: usbhStartInterruptInTransfer
Description:   Function to start an interrupt in transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true
******************************************************************************/
static _Bool usbhStartInterruptInTransfer(PUSBTR pRequest, int iPipeNumber)
{
    /* Set the cancel function */
    pRequest->pCancel = usbhCancelInFifo;
    /* Enable the buffer ready & not ready interrupts */
    R_USBH_SetPipeInterrupt(pRequest->pUSB,
                            iPipeNumber,
                            USBH_PIPE_BUFFER_READY |
                            USBH_PIPE_BUFFER_NOT_READY);
    TRACE(("usbhStartInterruptInTransfer: Started FIFO\r\n"));
    /* Set the pipe endpoint to buffer to start transfer */
    R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);
    /* Show the request is in progress */
    pRequest->bfInProgress = true;
    return true;
}
/******************************************************************************
End of function  usbhStartInterruptInTransfer
******************************************************************************/

/******************************************************************************
Function Name: usbhContinueInterruptInFifo
Description:   Function to continue an interrupt IN FIFO transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/
static _Bool usbhContinueInterruptInFifo(PUSBTR pRequest, int iPipeNumber)
{
    PUSB        pUSB = pRequest->pUSB;
    uint8_t     *pbyDest = pRequest->pMemory + pRequest->stIdx;
    size_t      stLengthToRead = pRequest->stLength - pRequest->stIdx;
    /* Read data from the pipe */
    pRequest->stTransferSize = R_USBH_ReadPipe(pUSB,
                                            iPipeNumber,
                                            pbyDest,
                                            stLengthToRead);
    /* Clear the buffer ready status */
    R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                              iPipeNumber,
                              USBH_PIPE_BUFFER_READY);
    switch (pRequest->stTransferSize)
    {
        /* More data in FIFO than in room for in destination memory */
        case -2UL:
        TRACE(("USBH_DATA_OVERRUN_ERROR_3\r\n"));
        /* Set the error code */
        pRequest->errorCode = USBH_DATA_OVERRUN_ERROR;
        /* Clear the buffer of any data */
        R_USBH_ClearPipeFifo(pRequest->pUSB, iPipeNumber);
        /* Complete the request */
        usbhCompleteInterruptInFifo(pRequest, iPipeNumber);
        break;
        /* SIE would not release the FIFO */
        case -1UL:
        /* Thanks to Martin Baker for fixing this one */
        TRACE(("USBH_FIFO_READ_ERROR\r\n"));
        /* Clear the buffer of any data */
        R_USBH_ClearPipeFifo(pUSB, iPipeNumber);
        /* Enable the pipe for next packet */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);
        /* The transfer is to continue */
        TRACE(("usbhContinueInterruptInFifo: Continue\r\n"));
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
            usbhCompleteInterruptInFifo(pRequest, iPipeNumber);
        }
        else
        {
            /* Enable the pipe for next packet */
            R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);
            /* The transfer is to continue */
            TRACE(("usbhContinueInterruptInFifo: Continue\r\n"));
            return true;
        }
        break;
    }
    return false;
}
/******************************************************************************
End of function  usbhContinueInterruptInFifo
******************************************************************************/

/******************************************************************************
Function Name: usbhCompleteInterruptInFifo
Description:   
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/
static void usbhCompleteInterruptInFifo(PUSBTR pRequest, int iPipeNumber)
{
    /* Clear any pending interrupts */
    R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                              iPipeNumber,
                              USBH_PIPE_BUFFER_READY |
                              USBH_PIPE_BUFFER_NOT_READY);
    /* Disable the interrupts */
    R_USBH_SetPipeInterrupt(pRequest->pUSB,
                            iPipeNumber,
                            USBH_PIPE_BUFFER_READY |
                            USBH_PIPE_BUFFER_NOT_READY |
                            USBH_PIPE_INT_DISABLE);
    /* Update the endpoint data PID toggle bit */
    pRequest->pEndpoint->dataPID = R_USBH_GetPipePID(pRequest->pUSB,
                                                     iPipeNumber);
    /* Free the pipe for use by another transfer */
    usbhFreePipeNumber(pRequest->pUsbHc, iPipeNumber);
    /* Complete the request */
    usbhComplete(pRequest);
    TRACE(("usbhCompleteInterruptInFifo:\r\n\r\n"));
}
/******************************************************************************
End of function  usbhCompleteInterruptInFifo
******************************************************************************/

/******************************************************************************
Function Name: usbhStartInterruptOutTransfer
Description:   Function to start an interrupt OUT transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is started
******************************************************************************/
static _Bool usbhStartInterruptOutTransfer(PUSBTR pRequest, int iPipeNumber)
{
    PUSB        pUSB = pRequest->pUSB;
    size_t  stLengthWritten;
    /* Set the cancel function */
    pRequest->pCancel = usbhCancelOutFifo;
    /* Write into the pipe FIFO */
    stLengthWritten = R_USBH_WritePipe(pUSB,
                                       iPipeNumber,
                                       pRequest->pMemory,
                                       pRequest->stLength);
    /* Check that it wrote OK */
    if (stLengthWritten != -1U)
    {
        /* Enable the buffer empty interrupt &
           Enable the not ready interrupt for detection of a STALL condition */
        R_USBH_SetPipeInterrupt(pRequest->pUSB,
                                iPipeNumber,
                                USBH_PIPE_BUFFER_EMPTY |
                                USBH_PIPE_BUFFER_NOT_READY);
        /* Clear any pending interrupts */
        R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                                  iPipeNumber,
                                  USBH_PIPE_BUFFER_EMPTY |
                                  USBH_PIPE_BUFFER_NOT_READY);
        /* Set the current transfer size */
        pRequest->stTransferSize = stLengthWritten;
        /* Set the pipe endpoint to buffer to start transfer */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);
        /* The transfer has been started or is in progress */
        pRequest->bfInProgress = true;
        TRACE(("usbhInterruptOut: Started\r\n"));
        return true;
    }
    return false;
}
/******************************************************************************
End of function  usbhStartInterruptOutTransfer
******************************************************************************/

/******************************************************************************
Function Name: usbhContinueInterruptOutFifo
Description:   Function to continue an interrupt out FIFO transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  none
******************************************************************************/
static void usbhContinueInterruptOutFifo(PUSBTR pRequest, int iPipeNumber)
{
    /* Clear the empty interrupt */
    R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                              iPipeNumber,
                              USBH_PIPE_BUFFER_EMPTY |
                              USBH_PIPE_BUFFER_NOT_READY);
    /* Add on the length transferred */
    pRequest->stIdx += pRequest->stTransferSize;
    /* Check for a short packet */
    if ((pRequest->stTransferSize < pRequest->pEndpoint->wPacketSize)
    /* Or all of the data has been written */
    ||  (pRequest->stIdx == pRequest->stLength))
    {
        /* The transfer is complete */
        usbhCompleteOutFifo(pRequest, iPipeNumber);
    }
    else
    {
        size_t  stLengthToWrite, stLengthWritten;
        uint8_t *   pbySrc;
        /* Calculate the length to write */
        stLengthToWrite = pRequest->stLength - pRequest->stIdx;
        /* Calculate the source address for the next transfer */
        pbySrc = pRequest->pMemory + pRequest->stIdx;
        /* Write into the pipe FIFO */
        stLengthWritten = R_USBH_WritePipe(pRequest->pUSB,
                                           iPipeNumber,
                                           pbySrc,
                                           stLengthToWrite);
        if (stLengthWritten != -1U)
        {
            /* Set the current transfer size */
            pRequest->stTransferSize = stLengthWritten;
            /* Set the pipe endpoint to buffer to start transfer */
            R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);
            TRACE(("usbhInterruptOut: Continue\r\n"));
        }
        else
        {
            /* Error accessing FIFO for write */
            pRequest->errorCode = USBH_FIFO_WRITE_ERROR;
            TRACE(("usbhInterruptOut: Error accessing FIFO\r\n"));
        }
    }
}
/******************************************************************************
End of function  usbhContinueInterruptOutFifo
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
