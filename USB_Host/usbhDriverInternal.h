/* INSERT LICENSE HERE */

#ifndef USBHDRIVERINTERNAL_H_INCLUDED
#define USBHDRIVERINTERNAL_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "./utilities/sysif.h"
#include "usbHostApi.h"
#include "usbhDeviceApi.h"

/******************************************************************************
Macro definitions
******************************************************************************/

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
Function Prototypes from usbhMain.c
******************************************************************************/

/******************************************************************************
Function Name: usbhSchedule (passed to the usbhOpen function)
Description:   Function to schedule USB transfers
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
Return value:  none
******************************************************************************/

extern  void usbhSchedule(PUSBHC pUsbHc);

/******************************************************************************
Function Name: usbhComplete
Description:   Function to complete a transfer
Arguments:     pRequest - Pointer to the request to complete
Return value:  true if the request was completed
******************************************************************************/

extern  _Bool usbhComplete(PUSBTR pRequest);

/******************************************************************************
Function Name: usbhIdleTimerTick
Description:   Function to perform an Idle time-out function and should be
               called at the start of each 1mS frame. When If the request's
               idle time is exceeded then the request is completed with the
               error code.
               REQ_IDLE_TIME_OUT
Arguments:     IN  pRequest - Pointer to the request
               IN  bfIdle - true if the request has been idle in the last frame
Return value:  none
******************************************************************************/

extern  void usbhIdleTimerTick(PUSBTR pRequest, _Bool bfIdle);

/******************************************************************************
Function Prototypes from usbhPipe.c
******************************************************************************/

/******************************************************************************
Function Name: usbhAllocPipeNumber
Description:   Function to get a pipe number (1 to 9) suitable for the endpoint
               and configures the pipe control register accordingly
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  The pipe number or -1 if no available pipe
               (or wrong transfer type)
******************************************************************************/

extern  int usbhAllocPipeNumber(PUSBTR pRequest);

/******************************************************************************
Function Name: usbhFreePipeNumber
Description:   Function to free a pipe so it can be used for another transfer
Arguments:     IN  pUsbHc - Pointer to Host Controller data
               IN  iPipeNumber - The number of the pipe
Return value:  none
******************************************************************************/

extern void usbhFreePipeNumber(PUSBHC pUsbHc, int iPipeNumber);
/******************************************************************************
Function Name: usbhPipeIdle
Description:   Function to return true if pipe has been idle since the last
               call
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The number of the pipe to check
Return value:  true if the pipe has been idle
******************************************************************************/

extern  _Bool usbhPipeIdle(PUSBTR pRequest, int iPipeNumber);

/******************************************************************************
Function Name: usbhContinueInFifo
Description:   Function to continue an IN FIFO transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/

extern  _Bool usbhContinueInFifo(PUSBTR pRequest, int iPipeNumber);

/******************************************************************************
Function Name: usbhCompleteInFifo
Description:   Function to complete an IN transfer by FIFO
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/

extern  void usbhCompleteInFifo(PUSBTR pRequest, int iPipeNumber);

/******************************************************************************
Function Name: usbhCompleteByFIFO
Description:   Function to complete the transaction by FIFO
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number
Return value:  none
******************************************************************************/

extern  void usbhCompleteByFIFO(PUSBTR  pRequest, int iPipeNumber);

/******************************************************************************
Function Name: usbhCancelInFifo
Description:   Function to cancel a bulk in transfer
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  none
******************************************************************************/

extern void usbhCancelInFifo(PUSBTR pRequest);

/******************************************************************************
Function Name: usbhCompleteOutFifo
Description:   Function to complete an out FOFO transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  none
******************************************************************************/

extern  void usbhCompleteOutFifo(PUSBTR pRequest, int iPipeNumber);

/******************************************************************************
Function Name: usbhCancelOutFifo
Description:   Function to cancel an out FIFO transfer
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  none
******************************************************************************/

extern  void usbhCancelOutFifo(PUSBTR pRequest);

/******************************************************************************
Function Prototypes from usbhControl.c
******************************************************************************/

/******************************************************************************
Function Name: usbhControlRequest
Description:   Function to process a control request
Arguments:     IN  ppRequest - Pointer to the transfer request list
Return value:  none
******************************************************************************/

extern  void usbhControlRequest(PUSBTR pRequest);

/******************************************************************************
Function Name: usbhControlCompleteSetup
Description:   Function to complete a setup transaction from the SACK and SIGN
               interrupts
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  pRequest - Pointer to the request
               IN  errorCode - The error code
Return value:  none
******************************************************************************/

extern  void usbhControlCompleteSetup(PUSB      pUSB,
                                      PUSBTR    pRequest,
                                      USBEC     errorCode);

/******************************************************************************
Function Name: usbhControlIn
Description:   Function to process data from the control IN phase from the
               PIPE0BRDYE interrupt
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  pRequest - Pointer to the transfer request list
Return value:  none
******************************************************************************/

extern  void usbhControlIn(PUSB pUSB, PUSBTR pRequest);

/******************************************************************************
Function Name: usbhControlOut
Description:   Function to process the data from a control OUT phase
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  pRequest - Pointer to the transfer request list
Return value:  none
******************************************************************************/

extern  void usbhControlOut(PUSB pUSB, PUSBTR pRequest);

/******************************************************************************
Function Name: usbhControlDataError
Description:   Function to handle an error in the data phase of a control
               transfer
Arguments:     IN  pUSB - Pointer to the Host Controller
               IN  pRequest - Pointer to the request
Return value:  none
******************************************************************************/

extern  void usbhControlDataError(PUSB pUSB, PUSBTR pRequest);

/******************************************************************************
Function Name: usbhControlPipeIdle
Description:   Function to return true if pipe has been idle since the last
               call
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  true if the pipe has been idle
******************************************************************************/

extern  _Bool usbhControlPipeIdle(PUSBTR pRequest);

/******************************************************************************
Function Prototypes from usbhBulk.c
******************************************************************************/

/******************************************************************************
Function Name: usbhStartBulkTransfer
Description:   Function to start a Bulk transfer
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  true if the transfer was started
******************************************************************************/

extern  _Bool usbhStartBulkTransfer(PUSBTR pRequest);

/******************************************************************************
Function Name: usbhBulkOut
Description:   Function to handle a bulk out transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/

extern  _Bool usbhBulkOut(PUSBTR pRequest, int iPipeNumber);

/******************************************************************************
Function Name: usbhBulkIn
Description:   Function to handle a bulk in transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/

extern  _Bool usbhBulkIn(PUSBTR pRequest, int iPipeNumber);

/******************************************************************************
Function Name: usbhTransferError
Description:   Function to handle a transfer error
Arguments:     none
Return value:  none
******************************************************************************/

extern  void usbhTransferError(PUSBTR pRequest);

/******************************************************************************
Function Name: usbhBulkPipeIdle
Description:   Function to return true if pipe has been idle since the last
               call
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The number of the pipe to check
Return value:  true if the pipe has been idle
******************************************************************************/

extern  _Bool usbhBulkPipeIdle(PUSBTR pRequest, int iPipeNumber);

/******************************************************************************
Function Prototypes from usbhInterrupt.c
******************************************************************************/

/******************************************************************************
Function Name: usbhStartInterruptTransfer
Description:   Function to start an interrupt transfer
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  true if the transfer was started
******************************************************************************/

extern  _Bool usbhStartInterruptTransfer(PUSBTR pRequest);

/******************************************************************************
Function Name: usbhInterruptIn
Description:   Function to handle an interrupt in transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/

extern  _Bool usbhInterruptIn(PUSBTR pRequest, int iPipeNumber);

/******************************************************************************
Function Name: usbhInterruptOut
Description:   Function to handle an interrupt out transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/

extern  _Bool usbhInterruptOut(PUSBTR pRequest, int iPipeNumber);

/******************************************************************************
Function Prototypes from usbhIsochronous.c
******************************************************************************/

/******************************************************************************
Function Name: usbhStartIsocTransfer
Description:   Function to start a Isoc transfer
Arguments:     IN  pRequest - Pointer to the transfer request
Return value:  true if the transfer was started
******************************************************************************/

extern  _Bool usbhStartIsocTransfer(PUSBTR pRequest);

/******************************************************************************
Function Name: usbhIsocOut
Description:   Function to handle an Isoc out transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/

extern  _Bool usbhIsocOut(PUSBTR pRequest, int iPipeNumber);

/******************************************************************************
Function Name: usbhIsocIn
Description:   Function to handle an Isoc in transfer
Arguments:     IN  pRequest - Pointer to the transfer request
               IN  iPipeNumber - The pipe number to use
Return value:  true if the transfer is in progress
******************************************************************************/

extern  _Bool usbhIsocIn(PUSBTR pRequest, int iPipeNumber);

#ifdef __cplusplus
}
#endif

#endif /* USBHDRIVERINTERNAL_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
