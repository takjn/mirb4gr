/* INSERT LICENSE HERE */

#ifndef SYSIF_H_INCLUDED
#define SYSIF_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include "ddusbh.h"

/******************************************************************************
Typedef definitions
******************************************************************************/

typedef enum _SIGST
{
    SYSIF_SIGNAL_RESET = 0,
    SYSIF_SIGNAL_SET,
    SYSIF_SIGNAL_ERROR
} SIGST;

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Function Name: sysCreateSignal
* Description  : Function to create and assign the IO completion object to the
*                transfer request
* Arguments    : IN  pRequest - Pointer to the transfer request
* Return Value : 0 for success or error code
******************************************************************************/

extern  int sysCreateSignal(PUSBTR pRequest);

/*****************************************************************************
* Function Name: sysDestroySignal
* Description  : Function to destroy the IO completion objet
* Arguments    : IN  pRequest - Pointer to the transfer request
* Return Value : 0 for success or error code
******************************************************************************/

extern  void sysDestroySignal(PUSBTR pRequest);

/*****************************************************************************
* Function Name: sysSetSignal
* Description  : Function to set the IO completion object
* Arguments    : IN  pRequest - Pointer to the transfer request
* Return Value : none
******************************************************************************/

extern  void sysSetSignal(PUSBTR pRequest);

/*****************************************************************************
* Function Name: sysResetSignal
* Description  : Function to reset the IO completion object
* Arguments    : IN  pRequest - Pointer to the transfer request
* Return Value : none
******************************************************************************/

extern  void sysResetSignal(PUSBTR pRequest);

/*****************************************************************************
* Function Name: sysResetSignal
* Description  : Function to return the state of the signal object
* Arguments    : IN  pRequest - Pointer to the transfer request
* Return Value : The state of the signal
******************************************************************************/

extern  SIGST sysGetSignalState(PUSBTR pRequest);

/*****************************************************************************
* Function Name: sysWaitSignal
* Description  : Function to wait for the IO complete signal to be set
* Arguments    : IN  pRequest - Pointer to the transfer request
* Return Value : true if the object is in the signalled state
******************************************************************************/

extern  void sysWaitSignal(PUSBTR pRequest);

/*****************************************************************************
* Function Name: sysYield
* Description  : Function to yield to other tasks or threads
* Arguments    : none
* Return Value : none
******************************************************************************/

extern  void sysYield(void);

/*****************************************************************************
* Function Name: sysLock
* Description  : Function to lock a critical section
* Arguments    : IN  pvLockObj - Pointer to an operating system locking object
*                                or NULL if not requred
* Return Value : The lock value
******************************************************************************/

extern  int sysLock(void *pvLockObj);

/*****************************************************************************
* Function Name: sysUnlock
* Description  : Function to unlock a critical section
* Arguments    : IN  pvLockObj - Pointer to an operating system locking object
*                                or NULL if not requred
*                IN  iUnlock - The value returned from sysLock function
* Return Value : none
******************************************************************************/

extern  void sysUnlock(void *pvLockObj, int iUnlock);

/*****************************************************************************
Function Name: sysWaitAccess
Description:   Function to allow the same task to enter a critical section and
               other tasks wait
Arguments:     none
Return value:  none
*****************************************************************************/

extern  void sysWaitAccess(void);

/*****************************************************************************
Function Name: sysReleaseAccess
Description:   Function to allow other tasks to access a re-entrant critial
               section
Arguments:     none
Return value:  none
*****************************************************************************/

extern  void sysReleaseAccess(void);

#ifdef __cplusplus
}
#endif

#endif                              /* SYSIF_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
