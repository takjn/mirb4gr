/******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized.
* This software is owned by Renesas Electronics Corporation and is  protected
* under all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES
* REGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY,
* INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR  A
* PARTICULAR PURPOSE AND NON-INFRINGEMENT.  ALL SUCH WARRANTIES ARE  EXPRESSLY
* DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE  LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES
* FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS
* AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this
* software and to discontinue the availability of this software.
* By using this software, you agree to the additional terms and
* conditions found by accessing the following link:
* http://www.renesas.com/disclaimer
*******************************************************************************
* Copyright (C) 2010 Renesas Electronics Corporation. All rights reserved.
******************************************************************************
* File Name    : hwDmaIf.c
* Version      : 1.0
* Device(s)    : RX62N
* Tool-Chain   : Renesas RX V1+
* OS           : None
* H/W Platform : RSKRX62N
* Description  : The hardware interface functions for the DMA. The functions
*                here could abstract from the DMA hardware providing a simple
*                interface for DMA usage. This is beyond the scope of this
*                sample code so it has been made as simple as possible.
*                For the RX the DMA controller is not able to handle single
*                transfers larger than 65535. Therefore when the DMA channel is
*                allocated the HW driver may trigger multiple DMA transfers.
*******************************************************************************
* History      : DD.MM.YYYY Ver. Description
*              : 01.08.2009 1.00 MAB First Release
******************************************************************************/

/******************************************************************************
  WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
  OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
  SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
******************************************************************************/

/******************************************************************************
User Includes
******************************************************************************/
#if !defined(__GNUC__) && !defined(GRSAKURA)
#include <machine.h>
#else
/* MAX must have higher priority than USB Host. */
#define USBH_MAX_INTERRUPT_PRIORITY USBH_DMA_INTERRUPT_PRIORITY
/* DMA must have of same priority as the USB Host */
#define USBH_DMA_INTERRUPT_PRIORITY USBH_INTERRUPT_PRIORITY
/* USB Host must have higher priority than timer for enumerator */
#define USBH_INTERRUPT_PRIORITY     2
/* Over current interrupt to set OC flag and disconnect power */
#define USBH_OVER_CURRENT_PRIORITY  1
#endif
#include "iodefine.h"
#include "hwDmaIf.h"
#include "./utilities/sysif.h"

/******************************************************************************
Function Macros
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

static size_t dmaCalcSegmentSizeMode2(size_t stLength, uint16_t wBlockSize);


/******************************************************************************
Global Variables
******************************************************************************/

static int giChannelMap = 0;
/* The parameters for the call-back functions */
static void *gpvParamCh0 = NULL;
static void *gpvParamCh1 = NULL;
/* The call back function pointers */
static void(*gpfComplete0)(void *) = NULL;
static void(*gpfComplete1)(void *) = NULL;
/* The length of the transfers */
static size_t gstLengthCh0 = 0UL;
static size_t gstTransferLengthCh0 = 0UL;
static uint16_t gwBlockSizeCh0 = 0;
static uint16_t gwNumBlocksCh0 = 0;
static size_t gstLengthCh1 = 0UL;
static size_t gstTransferLengthCh1 = 0UL;
static uint16_t gwBlockSizeCh1 = 0;
static uint16_t gwNumBlocksCh1 = 0;

/******************************************************************************
Public Functions
******************************************************************************/

/******************************************************************************
Function Name: dmaAlloc
Description:   Function to allocate a DMA channel - has no real function but to
               keep track of DMA allocation
Arguments:     IN  iChannel - The desired DMA channel number
Return value:  0 for success or -1 on error
******************************************************************************/
int dmaAlloc(int iChannel)
{
    #if 1
    int iMask = sysLock(NULL);
    if (iChannel < 4)
    {
        int iBitMap = (1 << iChannel);
        /* Check for channel already in use */
        if (giChannelMap & iBitMap)
        {
            sysUnlock(NULL, iMask);
            return -1;
        }
        /* Assign the channel */
        giChannelMap |= iBitMap;
        /* Check that the DMAC module is enabled */
        sysUnlock(NULL, iMask);
        return 0;
    }
    sysUnlock(NULL, iMask);
    #else
		TRACE(("DMA Allocation Disabled\r\n"));
    #endif
    return -1;
}
/******************************************************************************
End of function  dmaAlloc
******************************************************************************/

/******************************************************************************
Function Name: dmaFree
Description:   Function to free a DMA channel
Arguments:     IN  iChannel - The DMA channel to free
Return value:  0 for success or -1 on error
******************************************************************************/
int dmaFree(int iChannel)
{
    int iMask = sysLock(NULL);
    if (iChannel < 4)
    {
        int iBitMap = (1 << iChannel);
        /* Check that the channel is in use */
        if (giChannelMap & iBitMap)
        {
            giChannelMap &= ~iBitMap;
            sysUnlock(NULL, iMask);
            return 0;
        }
    }
    sysUnlock(NULL, iMask);
    return -1;
}
/******************************************************************************
End of function  dmaFree
******************************************************************************/

/******************************************************************************
Function Name: dmaStartUsbOutCh0
Description:   Function to start DMA channel 0 for a USB OUT transfer
               This is where the DMAC writes to the designated pipe FIFO.
               In this implementation the assignment is DMA Channel 0 always
               uses the USB D0FIFO.
Arguments:     IN  pvSrc - Pointer to the 4 byte alligned source memory
               IN  stLength - The length of data to transfer
               IN  pFIFO - Pointer to the destination FIFO
               IN  pvParam - Pointer to pass to the completion routine
               IN  pfComplete - Pointer to the completion routine
Return value:  none
******************************************************************************/
void dmaStartUsbOutCh0(void     *pvSrc,
                       size_t   stLength,
                       void     *pFIFO,
                       void     *pvParam,
                       void (*pfComplete)(void *pvParam))
{
    PUSBTR      pRequest = (PUSBTR)pvParam;
    int iMask = sysLock(NULL);
    /* Disable the DMA channel */
    DMAC0.DMCNT.BIT.DTE = 0;
    IEN(DMAC, DMAC0I) = 0;
    /* Disable the interrupst of the specific USB peripheral */
    if (pRequest->pUSB == &USB0)
    {
        IEN(USB0, D0FIFO0)= 0;
        /* Set the DMA source */
        ICU.DMRSR0 = VECT_USB0_D0FIFO0;
    }
    else
    {
        IEN(USB1, D0FIFO1)= 0;
        /* Set the DMA source */
        ICU.DMRSR0 = VECT_USB1_D0FIFO1;
    }    
    /* Set the pipe number and completion routine */
    gpvParamCh0 = pvParam;
    gpfComplete0 = pfComplete;
    gwBlockSizeCh0 = pRequest->pEndpoint->wPacketSize;
    /* Calculate the length that can be transferred in mode 2 */
    gstTransferLengthCh0 = dmaCalcSegmentSizeMode2(stLength, gwBlockSizeCh0);
    /* Set the remainder which will need to be processed by another transfer */
    gstLengthCh0 = stLength - gstTransferLengthCh0;
    gwNumBlocksCh0 = (uint16_t)(gstTransferLengthCh0 / gwBlockSizeCh0);
    /* The upper level of the Host Controller driver will have configured
       the hardware for the DMA transfer. It does not know that this DMA
       has a short address range. Reconfigure the USB Host Controller
       to have the new shorter transfer length */
    R_USBH_StopDmaWritePipe(pRequest->pUSB);
    R_USBH_DmaWritePipe(pRequest->pUSB,
                        (int)pRequest->pInternal,
                        gwNumBlocksCh0);
    #if 0
    /* Fields for understanding only */
    /* Destination Address Extended Repeat Area */
    DMAC0.DMAMD.BIT.DARA = 0;
    /* Destination Address Update Mode : Fixed */
    DMAC0.DMAMD.BIT.DM = 0;
    /* Source Address Extended Repeat Area */
    DMAC0.DMAMD.BIT.SARA = 0;
    /* Source Address Update Mode : Increment */
    DMAC0.DMAMD.BIT.SM = 2;
    TRACE(("DMAC0.DMAMD 0x%X\r\n", DMAC0.DMAMD.WORD));
    #else
    DMAC0.DMAMD.WORD = 0x8000;
    #endif
    #if 0
    /* Fields for understanding only */
    /* Request source Section : Peripheral */
    DMAC0.DMTMD.BIT.DCTG = 1;
    /* Transfer Size Select : 16bit (FIFO Size) */
    DMAC0.DMTMD.BIT.SZ = 1;
    /* Repeat Area Select : No Repeat */
    DMAC0.DMTMD.BIT.DTS = 2;
    /* Transfer Mode Select : Block transfers */
    DMAC0.DMTMD.BIT.MD = 2;
    TRACE(("DMAC0.DMTMD 0x%X\r\n", DMAC0.DMTMD.WORD));
    #else
    DMAC0.DMTMD.WORD = 0xA101;
    #endif
    /* Set the source address */
    DMAC0.DMSAR = pvSrc;
    /* Set the destination address register */
    DMAC0.DMDAR = pFIFO;
    /* Set the Block Count Register (/ 2 because of 16bit transfer) */
    DMAC0.DMCRA = (uint32_t)((gwBlockSizeCh0 / 2) << 16) | (gwBlockSizeCh0 / 2);
    /* Set the Block Transfer Count Register */
    DMAC0.DMCRB = gwNumBlocksCh0;
    /* No requirement to clear the activation source interrupt */
    DMAC1.DMCSL.BIT.DISEL = 0;
    #if 0
    /* Fields for understanding only */
    /* Destination Address Extended Repeat Area Overflow Interrupt Enable */
    DMAC0.DMINT.BIT.DARIE = 0;
    /* Source Address Extended Repeat Area Overflow Interrupt Enable */
    DMAC0.DMINT.BIT.SARIE = 0;
    /* Repeat Size End Interrupt Enable */
    DMAC0.DMINT.BIT.RPTIE = 0;
    /* Transfer Escape End Interrupt Enable */
    DMAC0.DMINT.BIT.ESIE = 0;
    /* Transfer End Interrupt Enable */
    DMAC0.DMINT.BIT.DTIE = 1;
    TRACE(("DMAC0.DMINT 0x%X\r\n", DMAC0.DMINT.BYTE));
    #else
    DMAC0.DMINT.BYTE = 0x10;
    #endif
    /* Set the interrupt priority */
    IPR(DMAC, DMAC0I) = USBH_DMA_INTERRUPT_PRIORITY;
    /* Enable the DMA channel */
    DMAC0.DMCNT.BIT.DTE = 1;
    /* Start the DMA transfer */
    DMAC.DMAST.BIT.DMST = 1;
    /* Enable the peripheral interrupt */
    if (pRequest->pUSB == &USB0)
    {
        IEN(USB0, D0FIFO0)= 1;
    }
    else
    {
        IEN(USB1, D0FIFO1)= 1;
    }  
    sysUnlock(NULL, iMask);
}
/******************************************************************************
End of function  dmaStartUsbOutCh0
******************************************************************************/

/******************************************************************************
Function Name: dmaGetUsbOutCh0Count
Description:   Function to get the current DMA transfer count
Parameters:    none
Return value:  The value of the transfer count register
******************************************************************************/
unsigned long dmaGetUsbOutCh0Count(void)
{
    /* This is only to detect transfer stop for purposes of time-out, however
       the number that is returned must be changing while the DMA working */
    return (unsigned long)(gstLengthCh0 + (DMAC0.DMCRB * gwNumBlocksCh0));
}
/******************************************************************************
End of function  dmaGetUsbOutCh0Count
******************************************************************************/

/******************************************************************************
Function Name: dmaStopUsbOutCh0
Description:   Function to stop a USB OUT DMA transfer on channel 0
Parameters:    none
Return value:  none
******************************************************************************/
void dmaStopUsbOutCh0(void)
{
    /* Stop the DMA channel */
    DMAC0.DMCNT.BIT.DTE = 0;
}
/******************************************************************************
End of function  dmaStopUsbOutCh0
******************************************************************************/

/******************************************************************************
Function Name: dmaStartUsbInCh1
Description:   Function to start DMA channel 1 for a USB IN transfer
               This is where the DMAC writes to the designated pipe FIFO.
               In this implementation the assignment is DMA Channel 1 always
               uses the USB D1FIFO.
Arguments:     IN  pvDest - Pointer to the 4 byte alligned destination memory
               IN  stLength - The length of data to transfer
               IN  pFIFO - Pointer to the source FIFO  
               IN  pvParam - Pointer to pass to the completion routine
               IN  pfComplete - Pointer to the completion routine
Return value:  none
******************************************************************************/
void dmaStartUsbInCh1(void     *pvDest,
                      size_t   stLength,
                      void     *pFIFO,
                      void     *pvParam,
                      void (*pfComplete)(void *pvParam))
{
    PUSBTR      pRequest = (PUSBTR)pvParam;
    int iMask = sysLock(NULL);
    /* Disable the DMA channel */
    DMAC1.DMCNT.BIT.DTE = 0;
    IEN(DMAC, DMAC1I) = 0;
    /* Disable the interrupt of the specific USB peripheral */
    if (pRequest->pUSB == &USB0)
    {
        IEN(USB0, D1FIFO0)= 0;
        /* Set the DMA source */
        ICU.DMRSR1 = VECT_USB0_D1FIFO0;
    }
    else
    {
        IEN(USB1, D1FIFO1)= 0;
        /* Set the DMA source */
        ICU.DMRSR1 = VECT_USB1_D1FIFO1;
    }
    /* Set the pipe number and completion routine */
    gpvParamCh1 = pvParam;
    gpfComplete1 = pfComplete;
    gwBlockSizeCh1 = pRequest->pEndpoint->wPacketSize;
    /* Calculate the length that can be transferred in mode 2 */
    gstTransferLengthCh1 = dmaCalcSegmentSizeMode2(stLength, gwBlockSizeCh1);
    /* Set the remainder which will need to be processed by another transfer */
    gstLengthCh1 = stLength - gstTransferLengthCh1;
    gwNumBlocksCh1 = (uint16_t)(gstTransferLengthCh1 / gwBlockSizeCh1);
    /* The upper level of the Host Controller driver will have configured
       the hardware for the DMA transfer. It does not know that this DMA
       has a short address range. Reconfigure the USB Host Controller
       to have the new shorter transfer length */
    R_USBH_StopDmaReadPipe(pRequest->pUSB);
    R_USBH_DmaReadPipe(pRequest->pUSB,
                       (int)pRequest->pInternal,
                       gwNumBlocksCh1);
    #if 0
    /* Fields for understanding only */
    /* Destination Address Extended Repeat Area */
    DMAC1.DMAMD.BIT.DARA = 0;
    /* Destination Address Update Mode : Increment*/
    DMAC1.DMAMD.BIT.DM = 2;
    /* Source Address Extended Repeat Area */
    DMAC1.DMAMD.BIT.SARA = 0;
    /* Source Address Update Mode : Fixed*/
    DMAC1.DMAMD.BIT.SM = 0;
    TRACE(("DMAC1.DMAMD 0x%X\r\n", DMAC1.DMAMD.WORD));
    #else
    DMAC1.DMAMD.WORD = 0x0080;
    #endif
    #if 0
    /* Fields for understanding only */
    /* Request source Section : Peripheral */
    DMAC1.DMTMD.BIT.DCTG = 1;
    /* Transfer Size Select : 16bit (FIFO Size)*/
    DMAC1.DMTMD.BIT.SZ = 1;
    /* Repeat Area Select : No Repeat */
    DMAC1.DMTMD.BIT.DTS = 2;
    /* Transfer Mode Select : Block transfers */
    DMAC1.DMTMD.BIT.MD = 2;
    TRACE(("DMAC0.DMTMD 0x%X\r\n", DMAC0.DMTMD.WORD));
    #else
    DMAC1.DMTMD.WORD = 0xA101;
    #endif
    /* Set the destination address */
    DMAC1.DMSAR = pFIFO;
    /* Set the destination address register */
    DMAC1.DMDAR = pvDest;
    /* Set the Block Count Register (/ 2 because of 16bit transfer) */
    DMAC1.DMCRA = (uint32_t)((gwBlockSizeCh1 / 2) << 16) | (gwBlockSizeCh1 / 2);
    /* Set the Block Transfer Count Register */
    DMAC1.DMCRB = gwNumBlocksCh1;
    /* No requirement to clear the activation source interrupt */
    DMAC1.DMCSL.BIT.DISEL = 0;
    #if 0
    /* Fields for understanding only */
    /* Destination Address Extended Repeat Area Overflow Interrupt Enable */
    DMAC1.DMINT.BIT.DARIE = 0;
    /* Source Address Extended Repeat Area Overflow Interrupt Enable */
    DMAC1.DMINT.BIT.SARIE = 0;
    /* Repeat Size End Interrupt Enable */
    DMAC1.DMINT.BIT.RPTIE = 0;
    /* Transfer Escape End Interrupt Enable */
    DMAC1.DMINT.BIT.ESIE = 0;
    /* Transfer End Interrupt Enable */
    DMAC1.DMINT.BIT.DTIE = 1;
    TRACE(("DMAC1.DMINT 0x%X\r\n", DMAC1.DMINT.BYTE));
    #else
    DMAC1.DMINT.BYTE = 0x10;
    #endif
    /* Set the interrupt priority */
    IPR(DMAC, DMAC1I) = USBH_DMA_INTERRUPT_PRIORITY;
    /* Enable the DMA channel */
    DMAC1.DMCNT.BIT.DTE = 1;
    /* Start the DMA transfer */
    DMAC.DMAST.BIT.DMST = 1;
    /* Enable the peripheral interrupt */
    if (pRequest->pUSB == &USB0)
    {
        IEN(USB0, D1FIFO0)= 1;
    }
    else
    {
        IEN(USB1, D1FIFO1)= 1;
    }
    sysUnlock(NULL, iMask);
}
/******************************************************************************
End of function  dmaStartUsbInCh1
******************************************************************************/

/******************************************************************************
Function Name: dmaGetUsbInCh1Count
Description:   Function to get the USB DMA transfer on channel 1
Parameters:    none
Return value:  The value of the transfer count register
******************************************************************************/
unsigned long dmaGetUsbInCh1Count(void)
{
    /* This is only to detect transfer stop for purposes of time-out, however
       the number that is returned must be changing while the DMA working */
    return (unsigned long)(gstLengthCh1 + (DMAC1.DMCRB * gwNumBlocksCh1));
}
/******************************************************************************
End of function  dmaGetUsbInCh1Count
******************************************************************************/

/******************************************************************************
Function Name: dmaStopUsbInCh1
Description:   Function to stop a USB OUT DMA transfer on channel 1
Parameters:    none
Return value:  none
******************************************************************************/
void dmaStopUsbInCh1(void)
{
    /* Disable the DMA channel */
    DMAC1.DMCNT.BIT.DTE = 0;
}
/******************************************************************************
End of function  dmaStopUsbInCh1
******************************************************************************/

/******************************************************************************
Private Functions
******************************************************************************/

/*****************************************************************************
* Function Name: dmaCalcSegmentSizeMode2
* Description  : Function to calculate the transfer size that the DMA is
*                capable of in mode 2.
* Arguments    : IN  stLength - The total length of data to transfer
*                IN  wBlockSize - The size of each block
* Return Value : The length of data that can be transferred in one hit
******************************************************************************/
static size_t dmaCalcSegmentSizeMode2(size_t stLength, uint16_t wBlockSize)
{
    /* DMCRAH specifies the block size and DMCRAL functions as a 10-bit block
       size counter. Nice */
    size_t  stBlockTransfer = (0x3FFUL * wBlockSize);
    if (stLength > stBlockTransfer)
    {
        return stBlockTransfer;
    }
    else
    {
        return stLength;
    }
}
/*****************************************************************************
End of function  dmaCalcSegmentSizeMode2
******************************************************************************/

/*****************************************************************************
* Function Name: dmaCompleteUsbCh0Out
* Description  : Function to complete the USB DMA out transfer
* Arguments    : none
* Return Value : none
******************************************************************************/
static void dmaCompleteUsbCh0Out(void)
{
    int iMask = sysLock(NULL);
    /* Check to see if there is more data to be transferred */
    if (gstLengthCh0)
    {
        /* Start another transfer continuing with the same source and
           destination addresses */
        dmaStartUsbOutCh0(DMAC0.DMSAR,
                          gstLengthCh0,
                          DMAC0.DMDAR,
                          gpvParamCh0,
                          gpfComplete0);
    }
    else
    {
        /* If there is a completion routine */
        if (gpfComplete0)
        {
            /* Call it */
            gpfComplete0(gpvParamCh0);
        }
    }
    sysUnlock(NULL, iMask);
}
/*****************************************************************************
End of function  dmaCompleteUsbCh0Out
******************************************************************************/

/*****************************************************************************
* Function Name: dmaCompleteUsbCh1In
* Description  : Function to complete the USB DMA in transfer
* Arguments    : none
* Return Value : none
******************************************************************************/
static void dmaCompleteUsbCh1In(void)
{
    int iMask = sysLock(NULL);
    /*  Software Countermeasure for DMA Restrictions are not required because
        the following conditions are satisfied:
        1. DMA/DTC is in block transfer mode when DISEL = 0
        2. The number of receive bocks matches the value set in the USB
           so that another transfer request is not generated until this
           interrupt handler is vectored */
    /* Check to see if there is more data to be transferred */
    if (gstLengthCh1)
    {
        /* Start another transfer continuing with the same source and
           destination addresses */
        dmaStartUsbInCh1(DMAC1.DMDAR,
                         gstLengthCh1,
                         DMAC1.DMSAR,
                         gpvParamCh1,
                         gpfComplete1);
    }
    else
    {
        /* If there is a completion routine */
        if (gpfComplete1)
        {
            /* Call it */
            gpfComplete1(gpvParamCh1);
        }
    }
    sysUnlock(NULL, iMask);
}
/*****************************************************************************
End of function  dmaCompleteUsbCh1In
******************************************************************************/

/******************************************************************************
Function Name: INT_D0FIFO0_COMPLETE
Description:   USB_D0FIFO0 (USB OUT  - host to device) end of transfer
               interrupt
Parameters:    none
Return value:  none
******************************************************************************/
#if defined( __RX )
#pragma interrupt (INT_D0FIFO0_COMPLETE(enable, vect=VECT_USB0_D0FIFO0))
#elif defined(__GNUC__) || defined(GRSAKURA)
void INT_Excep_USB0_D0FIFO0(void) __attribute__((interrupt));
#endif

void INT_Excep_USB0_D0FIFO0(void)
{
    dmaCompleteUsbCh0Out();
}
/******************************************************************************
End of function  INT_D0FIFO0_COMPLETE
******************************************************************************/

/******************************************************************************
Function Name: INT_D1FIFO0_COMPLETE
Description:   USB_D1FIFO0 (USB IN  - device to host) end of transfer
               interrupt
Parameters:    none
Return value:  none
******************************************************************************/
#if defined( __RX )
#pragma interrupt (INT_D1FIFO0_COMPLETE(enable, vect=VECT_USB0_D1FIFO0))
#elif defined(__GNUC__) || defined(GRSAKURA)
void INT_Excep_USB0_D1FIFO0(void) __attribute__((interrupt));
#endif
void INT_Excep_USB0_D1FIFO0(void)
{
    dmaCompleteUsbCh1In();
}
/******************************************************************************
End of function  INT_D1FIFO0_COMPLETE
******************************************************************************/

/******************************************************************************
Function Name: INT_D0FIFO1_COMPLETE
Description:   USB_D0FIFO1 (USB OUT  - host to device) end of transfer
               interrupt
Parameters:    none
Return value:  none
******************************************************************************/
#if defined( __RX )
#pragma interrupt (INT_D0FIFO1_COMPLETE(enable, vect=VECT_USB1_D0FIFO1))
#elif defined(__GNUC__) || defined(GRSAKURA)

#endif
void INT_D0FIFO1_COMPLETE(void)
{
    dmaCompleteUsbCh0Out();
}
/******************************************************************************
End of function  INT_D0FIFO1_COMPLETE
******************************************************************************/

/******************************************************************************
Function Name: INT_D1FIFO1_COMPLETE
Description:   USB_D1FIFO1 (USB IN  - device to host) end of transfer
               interrupt
Parameters:    none
Return value:  none
******************************************************************************/
#if defined( __RX )
#pragma interrupt (INT_D1FIFO1_COMPLETE(enable, vect=VECT_USB1_D1FIFO1))
#endif
void INT_D1FIFO1_COMPLETE(void)
{
    dmaCompleteUsbCh1In();
}
/******************************************************************************
End of function  INT_D1FIFO1_COMPLETE
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
