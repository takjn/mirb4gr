/* INSERT LICENSE HERE */

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#if !defined(__GNUC__) && !defined(GRSAKURA)
#include <machine.h>
#endif
#include "sysif.h"

#include "iodefine.h"

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/*****************************************************************************
* Function Name: sysCreateSignal
* Description  : Function to create and assign the IO completion object to the
*                transfer request
* Arguments    : IN  pRequest - Pointer to the transfer request
* Return Value : 0 for success or error code
******************************************************************************/
int sysCreateSignal(PUSBTR pRequest)
{
    /* In an OS Free implementation the "pvComplete" variable is used as a
       boolean flag. So nothig is allocated */
    pRequest->ioSignal.pvComplete = (void*)false;
    return 0;
}
/*****************************************************************************
End of function  sysCreateSignal
******************************************************************************/

/*****************************************************************************
* Function Name: sysDestroySignal
* Description  : Function to destroy the IO completion objet
* Arguments    : IN  pRequest - Pointer to the transfer request
* Return Value : 0 for success or error code
******************************************************************************/
void sysDestroySignal(PUSBTR pRequest)
{
    /* In an OS Free implementation nothing is allocated or freed */
	(void)pRequest;
}
/*****************************************************************************
End of function  sysDestroySignal
******************************************************************************/

/*****************************************************************************
* Function Name: sysSetSignal
* Description  : Function to set the IO completion object
* Arguments    : IN  pRequest - Pointer to the transfer request
* Return Value : none
******************************************************************************/
void sysSetSignal(PUSBTR pRequest)
{
    /* Here the parameter itself is used as a flag to signal completion */
    pRequest->ioSignal.pvComplete = (void*)true;
}
/*****************************************************************************
End of function  sysSetSignal
******************************************************************************/

/*****************************************************************************
* Function Name: sysResetSignal
* Description  : Function to reset the IO completion object
* Arguments    : IN  pRequest - Pointer to the transfer request
* Return Value : none
******************************************************************************/
void sysResetSignal(PUSBTR pRequest)
{
    /* In an OS Free implementation the "pvComplete" variable is used as a flag
       to signal completion */
    pRequest->ioSignal.pvComplete = (void*)false;
}
/*****************************************************************************
End of function  sysResetSignal
******************************************************************************/

/*****************************************************************************
* Function Name: sysGetSignalState
* Description  : Function to return the state of the signal object
* Arguments    : IN  pRequest - Pointer to the transfer request
* Return Value : The state of the signal
******************************************************************************/
SIGST sysGetSignalState(PUSBTR pRequest)
{
    /* When the hub is removed then the SOF interrupt that runs the transfer
       timer stops. This means that the code can get stuck here!
       To prevent this check that there is a device attached to the root port.
       This is done by checking the attach interrupt enable setting - if it
       is enabled then there is no device attached. */
    if (pRequest->pUSB->INTENB1.WORD & BIT_11)
    {
        return SYSIF_SIGNAL_SET;
    }
    /* In an OS Free implementation the "pvComplete" variable is used as a flag
       to signal completion. */
    if (pRequest->ioSignal.pvComplete)
    {
        return SYSIF_SIGNAL_SET;
    }
    else
    {
        return SYSIF_SIGNAL_RESET;
    }
}
/*****************************************************************************
End of function  sysGetSignalState
******************************************************************************/

/*****************************************************************************
* Function Name: sysWaitSignal
* Description  : Function to wait for the IO complete signal to be set
* Arguments    : IN  pRequest - Pointer to the transfer request
* Return Value : true if the object is in the signalled state
******************************************************************************/
void sysWaitSignal(PUSBTR pRequest)
{
    /* In an OS Free implementation the "pvComplete" variable is used as a flag
       to signal completion */
    while (sysGetSignalState(pRequest) == SYSIF_SIGNAL_RESET)
    {
        /* Wait */;
    }
}
/*****************************************************************************
End of function  sysWaitSignal
******************************************************************************/

/*****************************************************************************
* Function Name: sysYield
* Description  : Function to yield to other tasks or threads
* Arguments    : none
* Return Value : none
******************************************************************************/
void sysYield(void)
{
    /* In an OS Free implementation there is no yield! */
}
/*****************************************************************************
End of function  sysYield
******************************************************************************/

/*****************************************************************************
* Function Name: sysLock
* Description  : Function to lock a critical section
* Arguments    : IN  pvLockObj - Pointer to an operating system locking object
*                                or NULL if not requred
* Return Value : The lock value
******************************************************************************/
int sysLock(void *pvLockObj)
{
    if (!pvLockObj)
    {
#if !defined(__GNUC__) && !defined(GRSAKURA)
        int iMask = (int)get_ipl();
        set_ipl(15);
#else
        int iMask = (int) (__builtin_rx_mvfc(0)>>24);
        __builtin_rx_mvtipl(15);
#endif
        return iMask;
    }
    else
    {
        return 0;
    }
}
/*****************************************************************************
End of function  sysLock
******************************************************************************/

/*****************************************************************************
* Function Name: sysUnlock
* Description  : Function to unlock a critical section
* Arguments    : IN  pvLockObj - Pointer to an operating system locking object
*                                or NULL if not requred
*                IN  iUnlock - The value returned from sysLock function
* Return Value : none
******************************************************************************/
void sysUnlock(void *pvLockObj, int iUnlock)
{
    if (!pvLockObj)
    {
#if !defined(__GNUC__) && !defined(GRSAKURA)
        set_ipl((signed long)iUnlock);
#else
        /* Get the PSW */
        int iMask = __builtin_rx_mvfc(0);
        /* Clear the IPL bits */
        iMask &= ~((0x0F)<<24);
        /* Set the new IPL value */
        iMask |= ((iUnlock&0x0F)<<24);
        /* Set the PSW */
        __builtin_rx_mvtc(0,iMask);
#endif
    }
}
/*****************************************************************************
End of function  sysUnlock
******************************************************************************/

/*****************************************************************************
Function Name: sysWaitAccess
Description:   Function to allow the same task to enter a critical section and
               other tasks wait
Arguments:     none
Return value:  none
*****************************************************************************/
void sysWaitAccess(void)
{
}
/*****************************************************************************
End of function  sysWaitAccess
******************************************************************************/

/*****************************************************************************
Function Name: sysReleaseAccess
Description:   Function to allow other tasks to access a re-entrant critial
               section
Arguments:     none
Return value:  none
*****************************************************************************/
void sysReleaseAccess(void)
{
}
/*****************************************************************************
End of function  sysReleaseAccess
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
