/* INSERT LICENSE HERE */

#ifndef USBHOSTAPI_H_INCLUDED
#define USBHOSTAPI_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
*******************************************************************************/

#include "./utilities/sysif.h"

/*******************************************************************************
Function Prototypes
*******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
Function Name: usbhOpen
Description:   Function to open the USB host driver
Arguments:     IN  pUsbHc - Pointer to the host controller data
Return value:  true for success or false if not enough room in the host array
******************************************************************************/

extern  _Bool usbhOpen(PUSBHC pUsbHc);

/*****************************************************************************
Function Name: usbhClose
Description:   Function to close the USB host driver
Arguments:     IN  pUsbHc - Pointer to the host controller data
Return value:  none
*****************************************************************************/

extern  void usbhClose(PUSBHC pUsbHc);

/******************************************************************************
Function Name: enumRun
Description:   Function to run the enumerator, should be called at 1mS
               intervals.
Arguments:     none
Return value:  none
******************************************************************************/

extern  void enumRun(void);

/******************************************************************************
Function Name: usbhAddRootPort
Description:   Function to add a root port to the driver
Arguments:     IN  pUsbHc - Pointer to the host controller data
               IN  pRoot - Pointer to the port control functions for this port
Return value:  true if the port was added
******************************************************************************/

extern  _Bool usbhAddRootPort(PUSBHC pUsbHc, const PUSBPC pRoot);

/******************************************************************************
Function Name: usbhInterruptHandler
Description:   Function to handle the USB interrupt
Arguments:     IN  pUsbHc - Pointer to the Host Controller data
Return value:  none
******************************************************************************/

extern  void usbhInterruptHandler(PUSBHC pUsbHc);

#ifdef __cplusplus
}
#endif

#endif                              /* USBHOSTAPI_H_INCLUDED */

/******************************************************************************
End  Of File
*******************************************************************************/
