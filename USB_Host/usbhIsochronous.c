/* INSERT LICENSE HERE */

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include "usbhDriverInternal.h"
//#include "hwDmaIf.h"
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
#define TRACE(x)		((void) 0)
#define ASSERT(x)		((void) 0)
#endif

/******************************************************************************
Function Prototypes
******************************************************************************/

static _Bool usbhStartIsocInTransfer(PUSBTR pRequest, int iPipeNumber);
static _Bool usbhStartIsocOutTransfer(PUSBTR pRequest, int iPipeNumber);
static void usbhContinueIsocOutFifo(PUSBTR pRequest, int iPipeNumber);
static size_t usbhWriteIsocPipe(PUSB    pUSB,
                                int     iPipeNumber,
                                uint8_t *pbySrc,
                                size_t  stLength,
                                PUSBIV  pIsocPacket);
static uint16_t usbhGetIsocPacketSize(PUSBTR pRequest);

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/******************************************************************************
Function Name: usbhStartIsocTransfer
Description:   Function to start an Isoc transfer
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  true if the transfer was started
******************************************************************************/
_Bool usbhStartIsocTransfer(PUSBTR pRequest)
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
                /* Start the isoc in transfer */
                return usbhIsocIn(pRequest, iPipeNumber);
            }
            else
            {
                /* Start the isoc out transfer */
                _Bool    bfResult = usbhIsocOut(pRequest, iPipeNumber);
                /* Check to see if the transfer was started */
                if (!bfResult)
                {
                    /* Free the pipe */
                    TRACE(("usbhStartIsocTransfer: Not Started\r\n"));
                    
                    usbhFreePipeNumber(pRequest->pUsbHc, iPipeNumber);
                }
                return bfResult;
            }
        }
        TRACE(("usbhStartIsocTransfer: No Pipe Available\r\n"));
    }
    return false;
}
/******************************************************************************
End of function  usbhStartIsocTransfer
******************************************************************************/

/******************************************************************************
Function Name: usbhIsocIn
Description:   Function to handle an Isoc in transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/
_Bool usbhIsocIn(PUSBTR pRequest, int iPipeNumber)
{
    if (pRequest)
    {
        /* Disable the pipe */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, false);
        /* If it is not in progress then start it */
        if (pRequest->bfInProgress == false)
        {
            return usbhStartIsocInTransfer(pRequest, iPipeNumber);
        }
        else
        {
            return usbhContinueInFifo(pRequest, iPipeNumber);
        }
    }
    return false;
}
/******************************************************************************
End of function  usbhIsocIn
******************************************************************************/

/******************************************************************************
Function Name: usbhIsocOut
Description:   Function to handle an Isoc out transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/
_Bool usbhIsocOut(PUSBTR pRequest, int iPipeNumber)
{
    if (pRequest)
    {
        /* Disable the pipe */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, false);
        /* If it is not in progress then start it */
        if ((pRequest->bfInProgress == false)
        &&  (R_USBH_PipeReady(pRequest->pUSB, iPipeNumber)))
        {
            return usbhStartIsocOutTransfer(pRequest, iPipeNumber);
        }
        /* Continue or complete a FIFO transfer and complete a DMA transfer */
        else
        {
            if (R_USBH_PipeReady(pRequest->pUSB, iPipeNumber))
            {
                usbhContinueIsocOutFifo(pRequest, iPipeNumber);
            }
            return true;
        }
    }
    return false;
}
/******************************************************************************
End of function  usbhIsocOut
******************************************************************************/

/******************************************************************************
Private global variables and functions
******************************************************************************/

/******************************************************************************
Function Name: usbhStartIsocInTransfer
Description:   Function to start an Isoc in transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true
******************************************************************************/
static _Bool usbhStartIsocInTransfer(PUSBTR pRequest, int iPipeNumber)
{
    /* Set the cancel function */
    pRequest->pCancel = usbhCancelInFifo;
    /* Enable the buffer ready interrupt */
    R_USBH_SetPipeInterrupt(pRequest->pUSB,
                            iPipeNumber,
                            (USBIP)(USBH_PIPE_BUFFER_READY |
    /* Enable the not ready interrupt for detection of a STALL condition */
                            USBH_PIPE_BUFFER_NOT_READY));
    TRACE(("usbhStartIsocInTransfer: Started FIFO\r\n"));
    /* Set the pipe endpoint to buffer to start transfer */
    R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);
    /* Show the request is in progress */
    pRequest->bfInProgress = true;
    return true;
}
/******************************************************************************
End of function  usbhStartIsocInTransfer
******************************************************************************/

/******************************************************************************
Function Name: usbhStartIsocOutTransfer
Description:   Function to start an Isoc OUT transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is started
******************************************************************************/
static _Bool usbhStartIsocOutTransfer(PUSBTR pRequest, int iPipeNumber)
{
    PUSB        pUSB = pRequest->pUSB;
    size_t  stLengthWritten;
    /* Set the cancel function */
    pRequest->pCancel = usbhCancelOutFifo;
    /* Write into the pipe FIFO */
    stLengthWritten = usbhWriteIsocPipe(pUSB,
                                        iPipeNumber,
                                        pRequest->pMemory,
                                        pRequest->stLength,
                                        pRequest->pIsocPacketSize);
    /* Check that it wrote OK */
    if (stLengthWritten != -1U)
    {
        /* Enable the buffer empty interrupt &
           Enable the not ready interrupt for detection of a STALL condition */
        R_USBH_SetPipeInterrupt(pRequest->pUSB,
                                iPipeNumber,
                                (USBIP)(USBH_PIPE_BUFFER_EMPTY |
                                USBH_PIPE_BUFFER_NOT_READY));
        /* Clear any pending interrupts */
        R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                                  iPipeNumber,
                                  (USBIP)(USBH_PIPE_BUFFER_EMPTY |
                                  USBH_PIPE_BUFFER_NOT_READY));
        /* Set the current transfer size */
        pRequest->stTransferSize = stLengthWritten;
        /* Set the pipe endpoint to buffer to start transfer */
        R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);
        /* The transfer has been started or is in progress */
        pRequest->bfInProgress = true;
        TRACE(("usbhIsocOut: Started\r\n"));
        return true;
    }
   return false;
}
/******************************************************************************
End of function  usbhStartIsocOutTransfer
******************************************************************************/

/******************************************************************************
Function Name: usbhContinueIsocOutFifo
Description:   Function to continue an Isoc out FIFO transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  none
******************************************************************************/
static void usbhContinueIsocOutFifo(PUSBTR pRequest, int iPipeNumber)
{
    PUSB        pUSB = pRequest->pUSB;
    /* Add on the length transferred */
    pRequest->stIdx += pRequest->stTransferSize;
    /* Check for completion of the transfer */
    if (pRequest->stIdx >= pRequest->stLength)
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
        stLengthWritten = usbhWriteIsocPipe(pUSB,
                                            iPipeNumber,
                                            pbySrc,
                                            stLengthToWrite,
                                            pRequest->pIsocPacketSize);
        if (stLengthWritten != -1U)
        {
            /* Clear any pending interrupts */
            R_USBH_ClearPipeInterrupt(pRequest->pUSB,
                                      iPipeNumber,
                                      (USBIP)(USBH_PIPE_BUFFER_EMPTY |
                                      USBH_PIPE_BUFFER_NOT_READY));
            /* Set the current transfer size */
            pRequest->stTransferSize = stLengthWritten;
            /* Set the pipe endpoint to buffer to start transfer */
            R_USBH_EnablePipe(pRequest->pUSB, iPipeNumber, true);
            TRACE(("usbhIsocOut: Continue\r\n"));
        }
        else
        {
            /* Error accessing FIFO for write */
            pRequest->errorCode = USBH_FIFO_WRITE_ERROR;
            TRACE(("usbhIsocOut: Error accessing FIFO\r\n"));
        }
    }
}
/******************************************************************************
End of function  usbhContinueIsocOutFifo
******************************************************************************/

/*****************************************************************************
Function Name: usbhWriteIsocPipe
Description:   Function to write to an isoc pipe
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe to write to
               IN  pbySrc - Pointer to the data to write
               IN  stLength - The length to write
               IN  pIsocPacket - Pointer to the packet size schedule
Return value:  The number of bytes written
*****************************************************************************/
static size_t usbhWriteIsocPipe(PUSB    pUSB,
                                int     iPipeNumber,
                                uint8_t *pbySrc,
                                size_t  stLength,
                                PUSBIV  pIsocPacket)
{
    size_t  stResult;
    if (pIsocPacket)
    {
        size_t  stScheduledSize;
        /* Get the packet size */
        pIsocPacket->wPacketSize =
                    pIsocPacket->pwPacketSizeList[pIsocPacket->iScheduleIndex];
        stScheduledSize = (size_t)pIsocPacket->wPacketSize;
        if (stLength < stScheduledSize)
        {
            stScheduledSize = stLength;
        }
        /* Write to the FIFO */
        stResult = R_USBH_WritePipe(pUSB,
                                    iPipeNumber,
                                    pbySrc,
                                    stScheduledSize);
        /* Increment to the next packet size for the next transfer */
        pIsocPacket->iScheduleIndex++;
        /* Wrap back to 0 at the end of list */
        pIsocPacket->iScheduleIndex %= pIsocPacket->iListLength;
    }
    else
    {
        stResult = R_USBH_WritePipe(pUSB,
                                    iPipeNumber,
                                    pbySrc,
                                    stLength);
    }
    return stResult;
}
/*****************************************************************************
End of function  usbhWriteIsocPipe
******************************************************************************/

/******************************************************************************
Function Name: usbhGetIsocPacketSize
Description:   Function to get the endpoint packet size
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  The size of the endpoint packet
******************************************************************************/
static uint16_t usbhGetIsocPacketSize(PUSBTR pRequest)
{
    if (pRequest->pIsocPacketSize)
    {
        return pRequest->pIsocPacketSize->wPacketSize;
    }
    else
    {
        return pRequest->pEndpoint->wPacketSize;
    }
}
/******************************************************************************
End of function  usbhGetIsocPacketSize
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
