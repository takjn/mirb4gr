/******************************************************************************
* DISCLAIMER
* Please refer to http://www.renesas.com/disclaimer
******************************************************************************
* Copyright (C) 2010 Renesas Electronics Corporation. All rights reserved.    
*******************************************************************************
* File Name    : hwDmaIf.h
* Version      : 1.0
* Device(s)    : Renesas RX62N
* Tool-Chain   : Renesas RX
* OS           : None
* H/W Platform : RX62N
* Description  : The hardware interface functions for the DMA. The functions
*                here could abstract from the DMA hardware providing a simple
*                interface for DMA usage. This is beyond the scope of this
*                sample code so it has been made as simple as possible.
*******************************************************************************
* History : DD.MM.YYYY Version Description
*         : 05.08.2010 1.00    First Release
******************************************************************************/

/******************************************************************************
  WARNING!  IN ACCORDANCE WITH THE USER LICENCE THIS CODE MUST NOT BE CONVEYED
  OR REDISTRIBUTED IN COMBINATION WITH ANY SOFTWARE LICENSED UNDER TERMS THE
  SAME AS OR SIMILAR TO THE GNU GENERAL PUBLIC LICENCE
******************************************************************************/

#ifndef HWDMAIF_H_INCLUDED
#define HWDMAIF_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/

#include <string.h>

/******************************************************************************
Macro definitions
******************************************************************************/

#define DMA_CHANNEL_USBH_OUT        0
#define DMA_CHANNEL_USBH_IN         1

/******************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
Function Name: dmaAlloc
Description:   Function to allocate a DMA channel - to keep track of DMA
               allocation
Arguments:     IN  iChannel - The desired DMA channel number
Return value:  0 for success or -1 on error
******************************************************************************/

extern  int dmaAlloc(int iChannel);

/******************************************************************************
Function Name: dmaFree
Description:   Function to free a DMA channel
Arguments:     IN  iChannel - The DMA channel to free
Return value:  0 for success or -1 on error
******************************************************************************/
extern  int dmaFree(int iChannel);

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

extern  void dmaStartUsbOutCh0(void     *pvSrc,
                               size_t   stLength,
                               void     *pFIFO,
                               void     *pvParam,
                               void (*pfComplete)(void *pvParam));

/******************************************************************************
Function Name: dmaGetUsbOutCh0Count
Description:   Function to get the current DMA transfer count
Arguments:     none
Return value:  The value of the transfer count register
******************************************************************************/

extern  unsigned long dmaGetUsbOutCh0Count(void);

/******************************************************************************
Function Name: dmaStopUsbOutCh0
Description:   Function to stop a USB OUT DMA transfer on channel 0
Arguments:     none
Return value:  none
******************************************************************************/

extern  void dmaStopUsbOutCh0(void);

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

extern  void dmaStartUsbInCh1(void     *pvDest,
                              size_t   stLength,
                              void     *pFIFO,
                              void     *pvParam,
                              void (*pfComplete)(void *pvParam));

/******************************************************************************
Function Name: dmaGetUsbInCh1Count
Description:   Function to get the USB DMA transfer on channel 1
Arguments:     none
Return value:  The value of the transfer count register
******************************************************************************/

extern  unsigned long dmaGetUsbInCh1Count(void);

/******************************************************************************
Function Name: dmaStopUsbInCh1
Description:   Function to stop a USB OUT DMA transfer on channel 1
Arguments:     none
Return value:  none
******************************************************************************/

extern  void dmaStopUsbInCh1(void);

#ifdef __cplusplus
}
#endif

#endif /* HWDMAIF_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/