/* INSERT LICENSE HERE */

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include "usbhDriverInternal.h"

/******************************************************************************
Macro definitions
******************************************************************************/

/* Comment this line out to turn ON module trace in this file */
#undef _TRACE_ON_

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)
#define ASSERT(x)	((void)0)
#endif

/******************************************************************************
Function Prototypes
******************************************************************************/

static void usbhScheduleControl(PUSBHC pUsbHc);
static void usbhScheduleInterrupt(PUSBHC pUsbHc);
static void usbhSheduleIsoc(PUSBHC pUsbHc);
static void usbhHandleOutTransfer(PUSBHC pUsbHc);
static void ushbHandleInTransfer(PUSBHC pUsbHc);
static void usbhHandleTransferError(PUSBHC pUsbHc);
static void usbhCheckIdleTimeOut(PUSBHC pUsbHc);

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/******************************************************************************
Function Name: usbhInterruptHandler
Description:   Function to handle the USB interrupt
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
Return value:  none
******************************************************************************/
void usbhInterruptHandler(PUSBHC pUsbHc)
{
    PUSB pUSB = pUsbHc->pPort->pUSB;
    /* Setup transaction error */
    if (R_USBH_InterruptStatus(pUSB, USBH_INTSTS_SETUP_ERROR, true))
    {
        /* Handle the error */
        ASSERT(pUsbHc->pCurrentControl);
        usbhControlCompleteSetup(pUSB,
                                 pUsbHc->pCurrentControl,
                                 USBH_NOT_RESPONDING_ERROR);
        pUsbHc->pCurrentControl = NULL;
    }
    /* Setup transaction complete */
    if (R_USBH_InterruptStatus(pUSB, USBH_INTSTS_SETUP_COMPLETE, true))
    {
        /* Handle the completion */
        ASSERT(pUsbHc->pCurrentControl);
        usbhControlCompleteSetup(pUSB,
                                 pUsbHc->pCurrentControl,
                                 USBH_NO_ERROR);
        pUsbHc->pCurrentControl = NULL;
    }
    /* Check the buffer not ready */
    if (R_USBH_InterruptStatus(pUSB, USBH_INTSTS_PIPE_BUFFER_NOT_READY, true))
    {
        do
        {
            /* Check for EP0 not ready */
            if (R_USBH_GetPipeInterrupt(pUSB,
                                        0,
                                        USBH_PIPE_BUFFER_NOT_READY,
                                        false))
            {
                /* Handle the error */
                ASSERT(pUsbHc->pCurrentControl);
                usbhControlDataError(pUSB, pUsbHc->pCurrentControl);
                pUsbHc->pCurrentControl = NULL;
            }
            /* Check for any other pipe not ready */
            if (R_USBH_GetPipeInterrupt(pUSB,
                                        USBH_PIPE_NUMBER_ANY,
                                        USBH_PIPE_BUFFER_NOT_READY,
                                        false))
            {
                /* Handle any transfer error */
                usbhHandleTransferError(pUsbHc);
            }
        } while (R_USBH_GetPipeInterrupt(pUSB,
                                         USBH_PIPE_NUMBER_ANY,
                                         USBH_PIPE_BUFFER_NOT_READY,
                                         true));
    }
    /* Check the buffer empty */
    if (R_USBH_InterruptStatus(pUSB, USBH_INTSTS_PIPE_BUFFER_EMPTY, true))
    {
        do
        {
            /* Check for EP0 empty */
            if (R_USBH_GetPipeInterrupt(pUSB,
                                        0,
                                        USBH_PIPE_BUFFER_EMPTY,
                                        false))
            {
                /* Handle control OUT data */
                ASSERT(pUsbHc->pCurrentControl);
                usbhControlOut(pUSB, pUsbHc->pCurrentControl);
            }
            /* Check for any othe pipe with an empty buffer */
            if (R_USBH_GetPipeInterrupt(pUSB,
                                        USBH_PIPE_NUMBER_ANY,
                                        USBH_PIPE_BUFFER_EMPTY,
                                        false))
            {
                /* Handle all other OUT transfers */
                usbhHandleOutTransfer(pUsbHc);
            }
        } while (R_USBH_GetPipeInterrupt(pUSB,
                                         USBH_PIPE_NUMBER_ANY,
                                         USBH_PIPE_BUFFER_EMPTY,
                                         true));
    }
    /* Check the buffer ready */
    if (R_USBH_InterruptStatus(pUSB, USBH_INTSTS_PIPE_BUFFER_READY, true))
    {
        do
        {
            /* Check for EP0 ready */
            if (R_USBH_GetPipeInterrupt(pUSB,
                                        0,
                                        USBH_PIPE_BUFFER_READY,
                                        false))
            {
                /* Handle control IN data */
                ASSERT(pUsbHc->pCurrentControl);
                usbhControlIn(pUSB, pUsbHc->pCurrentControl);
            }
            if (R_USBH_GetPipeInterrupt(pUSB,
                                        USBH_PIPE_NUMBER_ANY,
                                        USBH_PIPE_BUFFER_READY,
                                        false))
            {
                /* Handle all other IN transfers */
                ushbHandleInTransfer(pUsbHc);
            }
        } while (R_USBH_GetPipeInterrupt(pUSB,
                                         USBH_PIPE_NUMBER_ANY,
                                         USBH_PIPE_BUFFER_READY,
                                         true));
    }
    /* Start of frame - this flag is set at the start of each 1mS frame but
       only when there is a device attached to one or more root ports */
    if (R_USBH_InterruptStatus(pUSB, USBH_INTSTS_SOFR, true))
    {
        /* Schedule any isoc transfers */
        usbhSheduleIsoc(pUsbHc);
        /* Schedule any interrupt transfers */
        usbhScheduleInterrupt(pUsbHc);
        /* Check idle time-outs */
        usbhCheckIdleTimeOut(pUsbHc);
    }
    #if USBH_HIGH_SPEED_SUPPORT == 1
    {
        uint16_t    wMicroFrame;
        R_USBH_GetFrameNumber(pUSB, NULL, &wMicroFrame);
        if (!wMicroFrame)
        {
    #endif
    /* Check for any control transfers */
    usbhScheduleControl(pUsbHc);
    #if USBH_HIGH_SPEED_SUPPORT == 1
        }
    }
    #endif
    /* Schedule any new BULK transfers */
    usbhSchedule(pUsbHc);
}
/******************************************************************************
End of function  usbhInterruptHandler
******************************************************************************/

/******************************************************************************
Function Name: usbhSchedule
Description:   Function to schedule USB transfers
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
Return value:  none
******************************************************************************/
void usbhSchedule(PUSBHC pUsbHc)
{
    PUSBTR  pRequest;
    /* Check for any bulk transfers */
    pRequest = pUsbHc->pBulk;
    /* While there are requests on the list */
    while (pRequest)
    {
        /* See if this request needs to be started */
        if (!pRequest->bfInProgress)
        {
            if (!usbhStartBulkTransfer(pRequest))
            {
                break;
            }
        }
        /* Go to the next request */
        pRequest = pRequest->pNext;
    }
}
/******************************************************************************
End of function  usbhSchedule
******************************************************************************/

/******************************************************************************
Private global variables and functions
******************************************************************************/

/******************************************************************************
Function Name: usbhScheduleControl
Description:   Function to schedule control transfers
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
Return value:  none
******************************************************************************/
static void usbhScheduleControl(PUSBHC pUsbHc)
{
    if ((pUsbHc->pCurrentControl == NULL)
    &&  (pUsbHc->pControl)
    && (!pUsbHc->pControl->bfInProgress))
    {
        /* Only one control transfer can be performed at any one time */
        usbhControlRequest(pUsbHc->pControl);
        if (pUsbHc->pControl->bfInProgress)
        {
            pUsbHc->pCurrentControl = pUsbHc->pControl;
        }
    }
}
/******************************************************************************
End of function  usbhScheduleControl
******************************************************************************/

/******************************************************************************
Function Name: usbhScheduleInterrupt
Description:   Function to schedule interrutpt transfers
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
Return value:  none
******************************************************************************/
static void usbhScheduleInterrupt(PUSBHC pUsbHc)
{
    /* Check for any interrupt transfers */
    PUSBTR  pRequest = pUsbHc->pInterrupt;
    while (pRequest)
    {
        /* See if this request needs to be started */
        if (!pRequest->bfInProgress)
        {
            if (!usbhStartInterruptTransfer(pRequest))
            {
                break;
            }
        }
        pRequest = pRequest->pNext;
    }
}
/******************************************************************************
End of function  usbhScheduleInterrupt
******************************************************************************/

/******************************************************************************
Function Name: usbhSheduleIsoc
Description:   Funtion to check for any Isoc transfers to be scheduled
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
Return value:  none
******************************************************************************/
static void usbhSheduleIsoc(PUSBHC pUsbHc)
{
    /* Schedule isoc transfers */
    if (pUsbHc->pIsochronus)
    {
        /* Check for any isoc transfers */
        PUSBTR  pRequest = pUsbHc->pIsochronus;
        while (pRequest)
        {
            /* See if this request needs to be started */
            if (!pRequest->bfInProgress)
            {
                if (!usbhStartIsocTransfer(pRequest))
                {
                    break;
                }
            }
            pRequest = pRequest->pNext;
        }
    }
}
/******************************************************************************
End of function  usbhSheduleIsoc
******************************************************************************/

/******************************************************************************
Private Functions
******************************************************************************/

/******************************************************************************
Function Name: usbhHandleOutTransfer
Description:   Function to handle the buffer empty interrupt for OUT transfers
               other than the control pipe
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
Return value:  none
******************************************************************************/
static void usbhHandleOutTransfer(PUSBHC pUsbHc)
{
    PUSB pUSB = pUsbHc->pPort->pUSB;
    int iPipeNumber = 1;
    while (iPipeNumber < USBH_MAX_NUM_PIPES)
    {
        if (R_USBH_GetPipeInterrupt(pUSB,
                                    iPipeNumber,
                                    USBH_PIPE_BUFFER_EMPTY,
                                    true))
        {
            /* Get the request associated with this pipe number */
            PUSBTR pRequest = pUsbHc->pEndpointAssign[iPipeNumber].pRequest;
            if (pRequest)
            {
                switch (pRequest->pEndpoint->transferType)
                {
                    case USBH_BULK:     /* Handle bulk out transfers */
                    usbhBulkOut(pRequest, iPipeNumber);
                    break;

                    case USBH_INTERRUPT:/* Handle interrupt out transfers */
                    usbhInterruptOut(pRequest, iPipeNumber);
                    break;

                    case USBH_ISOCHRONOUS:/* Handle isoc out transfers */
                    usbhIsocOut(pRequest, iPipeNumber);
                    usbhSheduleIsoc(pUsbHc);
                    break;
                    
                    default:
                    usbhCancelTransfer(pRequest);
                    usbhFreePipeNumber(pUsbHc, iPipeNumber);
                    R_USBH_ClearPipeInterrupt(pUSB,
                                              iPipeNumber,
                                              USBH_PIPE_BUFFER_EMPTY);
                    R_USBH_SetPipeInterrupt(pUSB,
                                            iPipeNumber,
                                            (USBIP)(USBH_PIPE_BUFFER_EMPTY |
                                            USBH_PIPE_INT_DISABLE));
                    TRACE(("usbhHandleOutTransfer: "
                           "Unsupported transfer type %d\r\n",
                           pRequest->pEndpoint->transferType));
                    break;
                }
            }
            else
            {
                R_USBH_ClearPipeInterrupt(pUSB,
                                          iPipeNumber,
                                          USBH_PIPE_BUFFER_EMPTY);
                R_USBH_SetPipeInterrupt(pUSB,
                                        iPipeNumber,
                                        (USBIP)(USBH_PIPE_BUFFER_EMPTY |
                                        USBH_PIPE_INT_DISABLE));
            }
        }
        iPipeNumber++;
    }
}
/******************************************************************************
End of function  usbhHandleOutTransfer
******************************************************************************/

/******************************************************************************
Function Name: ushbHandleInTransfer
Description:   Function to handle the buffer ready interrupt for IN transfers
               other than the control pipe
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
Return value:  none
******************************************************************************/
static void ushbHandleInTransfer(PUSBHC pUsbHc)
{
    PUSB pUSB = pUsbHc->pPort->pUSB;
    int iPipeNumber = 1;
    while (iPipeNumber < USBH_MAX_NUM_PIPES)
    {
        if (R_USBH_GetPipeInterrupt(pUSB,
                                    iPipeNumber,
                                    USBH_PIPE_BUFFER_READY,
                                    true))
        {
            /* Get the request associated with this pipe number */
            PUSBTR pRequest = pUsbHc->pEndpointAssign[iPipeNumber].pRequest;
            if (pRequest)
            {
                switch (pRequest->pEndpoint->transferType)
                {
                    case USBH_BULK:     /* Handle bulk in transfers */
                    usbhBulkIn(pRequest, iPipeNumber);
                    break;

                    case USBH_INTERRUPT:/* Handle interrupt in transfers */
                    usbhInterruptIn(pRequest, iPipeNumber);
                    break;
                    
                    case USBH_ISOCHRONOUS:/* Handle isochronous in transfers */
                    usbhIsocIn(pRequest, iPipeNumber);
                    usbhSheduleIsoc(pUsbHc);
                    break;
                    
                    default:
                    usbhCancelTransfer(pRequest);
                    usbhFreePipeNumber(pUsbHc, iPipeNumber);
                    R_USBH_ClearPipeInterrupt(pUSB,
                                              iPipeNumber,
                                              USBH_PIPE_BUFFER_READY);
                    R_USBH_SetPipeInterrupt(pUSB,
                                            iPipeNumber,
                                            (USBIP)(USBH_PIPE_BUFFER_READY |
                                            USBH_PIPE_INT_DISABLE));
                    TRACE(("ushbHandleInTransfer: "
                           "Unsupported transfer pipe %d type %d\r\n",
                           iPipeNumber,
                           pRequest->pEndpoint->transferType));
                    break;
                }
            }
            else
            {
                R_USBH_ClearPipeInterrupt(pUSB,
                                          iPipeNumber,
                                          USBH_PIPE_BUFFER_READY);
                R_USBH_SetPipeInterrupt(pUSB,
                                        iPipeNumber,
                                        (USBIP)(USBH_PIPE_BUFFER_READY |
                                        USBH_PIPE_INT_DISABLE));
            }
        }
        iPipeNumber++;
    }
}
/******************************************************************************
End of function  ushbHandleInTransfer
******************************************************************************/

/******************************************************************************
Function Name: usbhHandleTransferError
Description:   Function to handle a transfer error for transfers other than
               the control pipe
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
Return value:  none
******************************************************************************/
static void usbhHandleTransferError(PUSBHC pUsbHc)
{
    PUSB pUSB = pUsbHc->pPort->pUSB;
    int iPipeNumber = 1;
    while (iPipeNumber < USBH_MAX_NUM_PIPES)
    {
        if (R_USBH_GetPipeInterrupt(pUSB,
                                    iPipeNumber,
                                    USBH_PIPE_BUFFER_NOT_READY,
                                    true))
        {
            /* Get the request associated with this pipe number */
            PUSBTR pRequest = pUsbHc->pEndpointAssign[iPipeNumber].pRequest;
            if (pRequest)
            {
                switch (pRequest->pEndpoint->transferType)
                {
                    /* Handle bulk or interrupt transfer error */
                    case USBH_BULK:
                    case USBH_INTERRUPT:
                    {
                        usbhTransferError(pRequest);
                        break;
                    }
                    case USBH_ISOCHRONOUS:/* Handle isochronous transfer error */
                    {
                        /* Isochronous transfers continue - errors or not */
                        if (pRequest->pEndpoint->transferDirection == USBH_OUT)
                        {
                            usbhIsocOut(pRequest, iPipeNumber);
                        }
                        else
                        {
                            usbhIsocIn(pRequest, iPipeNumber);
                        }
                        usbhSheduleIsoc(pUsbHc);
                        break;
                    }
                    default:
                    usbhCancelTransfer(pRequest);
                    usbhFreePipeNumber(pUsbHc, iPipeNumber);
                    R_USBH_ClearPipeInterrupt(pUSB,
                                              iPipeNumber,
                                              USBH_PIPE_BUFFER_NOT_READY);
                    R_USBH_SetPipeInterrupt(pUSB,
                                            iPipeNumber,
                                            (USBIP)(USBH_PIPE_BUFFER_NOT_READY |
                                            USBH_PIPE_INT_DISABLE));
                    TRACE(("usbhHandleTransferError: "
                           "Unsupported transfer type %d\r\n",
                           pRequest->pEndpoint->transferType));
                    break;
                }
            }
            else
            {
                R_USBH_ClearPipeInterrupt(pUSB,
                                          iPipeNumber,
                                          USBH_PIPE_BUFFER_NOT_READY);
                R_USBH_SetPipeInterrupt(pUSB,
                                        iPipeNumber,
                                        (USBIP)(USBH_PIPE_BUFFER_NOT_READY |
                                        USBH_PIPE_INT_DISABLE));
            }
        }
        iPipeNumber++;
    }
}
/******************************************************************************
End of function  usbhHandleTransferError
******************************************************************************/

/******************************************************************************
Function Name: usbhCheckIdleTimeOut
Description:   Function to check the transfer lists for inactivity time-out
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
Return value:  none
******************************************************************************/
static void usbhCheckIdleTimeOut(PUSBHC pUsbHc)
{
    /* Check isoc transfer for time-out */
    PUSBTR  pRequest = pUsbHc->pIsochronus;
    while (pRequest)
    {
        if (pRequest->bfInProgress)
        {
            /* The request internal pointer is only used to hold the pipe
               number */
            int     iPipeNumber = (int)pRequest->pInternal;
            /* Call the isochronous function to get the idle state */
            _Bool    bfIdle = usbhPipeIdle(pRequest, iPipeNumber);
            /* Now call the timer tick to cancel the request if it has been
               idle longer than the time period */
            usbhIdleTimerTick(pRequest, bfIdle);
        }
        /* Go to the next request */
        pRequest = pRequest->pNext;
    }
    /* Check control transfer for time-out */
    pRequest = pUsbHc->pControl;
    if ((pRequest)
    &&  (pRequest->bfInProgress))
    {
        usbhIdleTimerTick(pRequest, usbhControlPipeIdle(pRequest));
    }
    /* Check Interrupt list for time-out */
    pRequest = pUsbHc->pInterrupt;
    /* While there are requests on the list */
    while (pRequest)
    {
        if (pRequest->bfInProgress)
        {
            /* The request internal pointer is only used to hold the pipe
               number */
            int     iPipeNumber = (int)pRequest->pInternal;
            /* Call the interrupt function to get the idle state */
            _Bool    bfIdle = usbhPipeIdle(pRequest, iPipeNumber);
            /* Now call the timer tick to cancel the request if it has been
               idle longer than the time period */
            usbhIdleTimerTick(pRequest, bfIdle);
        }
        /* Go to the next request */
        pRequest = pRequest->pNext;
    }
    /* Check bulk list for time-out */
    pRequest = pUsbHc->pBulk;
    /* While there are requests on the list */
    while (pRequest)
    {
        if (pRequest->bfInProgress)
        {
            /* The request internal pointer is only used to hold the pipe
               number */
            int     iPipeNumber = (int)pRequest->pInternal;
            /* Call the bulk function to get the idle state */
            _Bool    bfIdle = usbhBulkPipeIdle(pRequest, iPipeNumber);
            /* Now call the timer tick to cancel the request if it has been
               idle longer than the time period */
            usbhIdleTimerTick(pRequest, bfIdle);
        }
        /* Go to the next request */
        pRequest = pRequest->pNext;
    }
}
/******************************************************************************
End of function  usbhCheckIdleTimeOut
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
