/* INSERT LICENSE HERE */

#ifndef R_USBH_DRIVER_H_INCLUDED
#define R_USBH_DRIVER_H_INCLUDED

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include "usbhost_typedefine.h"
#include "usbhConfig.h"

/*****************************************************************************
Macro definitions
******************************************************************************/

#define USBH_PIPE_NUMBER_ANY        -1
#define USBH_MAX_NUM_PIPES          10

/******************************************************************************
Typedef definitions
******************************************************************************/

/* Transfer Type */
typedef enum _USBTT
{
    USBH_CONTROL = 0,
    USBH_ISOCHRONOUS = 1,
    USBH_BULK = 2,
    USBH_INTERRUPT = 3
} USBTT;

/* Transfer Direction */
typedef enum _USBDIR
{
    /* Host to device */
    USBH_OUT = 0,
    /* Device to Host */
    USBH_IN = 1,
    /* Setup packet */
    USBH_SETUP = 2
} USBDIR;

/* Transfer Speed */
typedef enum _USBTS
{
    /* 12MB/S */
    USBH_FULL = 0,
    /* 1.5MB/S */
    USBH_SLOW = 1,
    /* 480MB/S */
    USBH_HIGH = 2,
    /* No device attached */
    USBH_NO_DEVICE = 3
} USBTS;

/* Data PID (Identifier toggles on each transaction) */
typedef enum _USBDP
{
    USBH_DATA0 = 0,
    USBH_DATA1
} USBDP;

/* Interrupt types for pipe buffer status */
typedef enum _USBIP
{
    USBH_PIPE_INT_ENABLE = 0,
    USBH_PIPE_INT_DISABLE = BIT_0,
    USBH_PIPE_BUFFER_EMPTY = BIT_1,
    USBH_PIPE_BUFFER_READY = BIT_2,
    USBH_PIPE_BUFFER_NOT_READY = BIT_3
} USBIP;

/* Interrupt types for peripheral status */
typedef enum _USBIS
{
    USBH_INTSTS_SOFR = 0,
    USBH_INTSTS_PIPE_BUFFER_EMPTY,
    USBH_INTSTS_PIPE_BUFFER_READY,
    USBH_INTSTS_PIPE_BUFFER_NOT_READY,
    USBH_INTSTS_SETUP_ERROR,
    USBH_INTSTS_SETUP_COMPLETE
} USBIS;

/* Pointer to the USB Host Controller hardware. This is encapsulated so upper
   levels of the host stack cannot access the hardware directly and MUST go
   through this Abstracted Driver API. The iodefine.h file MUST not be
   included in upper levels of the driver since this will break the
   encapsulation */
#if defined(__RX)
typedef volatile __evenaccess struct USBH_HOST_STRUCT_TAG *PUSB;
#else
typedef volatile struct USBH_HOST_STRUCT_TAG *PUSB;
#endif

/*****************************************************************************
Function Prototypes
******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
Function Name: R_USBH_Initialise
Description:   Function to initialise the USB peripheral for operation as the
               host controller
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
Return value:  none
******************************************************************************/

extern  void R_USBH_Initialise(PUSB pUSB);

/******************************************************************************
Function Name: R_USBH_Uninitialise
Description:   Function to uninitialise the USB peripheral for operation as the
               Host Controller
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
Return value:  none
******************************************************************************/

extern  void R_USBH_Uninitialise(PUSB pUSB);

/******************************************************************************
Function Name: R_USBH_ConfigurePipe
Description:   Function to configure the pipe for the endpoint's transfer
               characteristics
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The pipe number to configure
               IN  transferType - The type of transfer
               IN  transferDirection - The direction of transfer
               IN  byDeviceAddress - The address of the device
               IN  byEndpointNumber - The endpoint address with the direction
                                      bit masked
               IN  byInterval - The endpoint poll interval for Isochronus and
                                interrupt transfers only
               IN  wPacketSize - The endpoint packet size
Return value:  The pipe number or -1 on error
******************************************************************************/

extern  int R_USBH_ConfigurePipe(PUSB       pUSB,
                                 int        iPipeNumber,
                                 USBTT      transferType,
                                 USBDIR     transferDirection,
                                 uint8_t    byDeviceAddress,
                                 uint8_t    byEndpointNumber,
                                 uint8_t    byInterval,
                                 uint16_t   wPacketSize);

/******************************************************************************
Function Name: R_USBH_UnconfigurePipe
Description:   Function to unconfigure a pipe so it can be used for another
               transfer
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe to unconfigure
Return value:  none
******************************************************************************/

extern  void R_USBH_UnconfigurePipe(PUSB pUSB, int iPipeNumber);

/******************************************************************************
Function Name: R_USBH_ClearPipeFifo
Description:   Function to clear the pipe FIFO
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe to clear
Return value:  none
******************************************************************************/

extern  void R_USBH_ClearPipeFifo(PUSB pUSB, int iPipeNumber);

/******************************************************************************
Function Name: R_USBH_SetPipePID
Description:   Function to set the pipe PID
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
               IN  dataPID - The Data PID setting
Return value:  none
******************************************************************************/

extern  void R_USBH_SetPipePID(PUSB pUSB, int iPipeNumber, USBDP dataPID);

/******************************************************************************
Function Name: R_USBH_GetPipePID
Description:   Function to get the pipe PID
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
Return value:  The state of the data PID
******************************************************************************/

extern  USBDP R_USBH_GetPipePID(PUSB pUSB, int iPipeNumber);

/******************************************************************************
Function Name: R_USBH_GetPipeStall
Description:   Function to get the pipe stall state
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
Return value:  true if the pipe has received a stall token
******************************************************************************/

extern  _Bool R_USBH_GetPipeStall(PUSB pUSB, int iPipeNumber);

/******************************************************************************
Function Name: R_USBH_EnablePipe
Description:   Function to Enable (ACK) or Disable (NAK) the pipe
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
               IN  bfEnable - True to enable transfers on the pipe
Return value:  none
******************************************************************************/

extern  void R_USBH_EnablePipe(PUSB pUSB, int iPipeNumber, _Bool bfEnable);

/******************************************************************************
Function Name: R_USBH_SetPipeInterrupt
Description:   Function to set or clear the desired pipe interrupts
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
               IN  buffIntType - The interrupt type
Return value:  none
******************************************************************************/

extern  void R_USBH_SetPipeInterrupt(PUSB  pUSB,
                                     int   iPipeNumber,
                                     USBIP buffIntType);

/******************************************************************************
Function Name: R_USBH_ClearPipeInterrupt
Description:   Function to clear the desired pipe interrupt status flag
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
               IN  buffIntType - The interrupt type
Return value:  none
******************************************************************************/

extern  void R_USBH_ClearPipeInterrupt(PUSB  pUSB,
                                       int   iPipeNumber,
                                       USBIP buffIntType);

/******************************************************************************
Function Name: R_USBH_GetPipeInterrupt
Description:   Function to get the state of the pipe interrupt status flag
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe or USBH_PIPE_NUMBER_ANY
               IN  buffIntType - The interrupt type
               IN  bfQualify - Set true to qualify with enabled interrupts
Return value:  none
******************************************************************************/

extern  _Bool R_USBH_GetPipeInterrupt(PUSB  pUSB,
                                      int   iPipeNumber,
                                      USBIP buffIntType,
                                      _Bool bfQualify);

/******************************************************************************
Function Name: R_USBH_InterruptStatus
Description:   Function to get the interrupt status
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  intStatus - The interrupt status to get
               IN  bfAck - true to acknowledge the status
Return value:  none
******************************************************************************/

extern  _Bool R_USBH_InterruptStatus(PUSB  pUSB,
                                     USBIS intStatus,
                                     _Bool bfAck);

/******************************************************************************
Function Name: R_USBH_GetFrameNumber
Description:   Function to get the frame and micro frame numbers
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               OUT pwFrame - Pointer to the destinaton frame number
               OUT pwMicroFrame - Pointer to the destination micro frame number
Return value:  none
******************************************************************************/

extern  void R_USBH_GetFrameNumber(PUSB     pUSB,
                                   uint16_t *pwFrame,
                                   uint16_t *pwMicroFrame);

/******************************************************************************
Function Name: R_USBH_WritePipe
Description:   Function to write to an endpoint pipe FIFO
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The pipe number to write to
               IN  pbySrc - Pointer to the source memory (FIFO size alligned)
               IN  stLength - The length to write
Return value:  The number of bytes written or -1 on FIFO access error
******************************************************************************/

extern  size_t R_USBH_WritePipe(PUSB        pUSB,
                                int         iPipeNumber,
                                uint8_t     *pbySrc,
                                size_t      stLength);

/******************************************************************************
Function Name: R_USBH_DmaWritePipe
Description:   Function to configure a pipe for the DMA to write to it
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The pipe to configure
               IN  wNumPackets - The number of packets to be written
Return value:  0 for success -1 on error
******************************************************************************/

extern  int R_USBH_DmaWritePipe(PUSB        pUSB,
                                int         iPipeNumber,
                                uint16_t    wNumPackets);

/******************************************************************************
Function Name: R_USBH_StopDmaWritePipe
Description:   Function to unconfigure a pipe from a DMA write
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
Return value:  none
******************************************************************************/

extern  void R_USBH_StopDmaWritePipe(PUSB pUSB);

/******************************************************************************
Function Name: R_USBH_ReadPipe
Description:   Function to read from an endpoint pipe FIFO
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The pipe number to read from
               IN  pbyDest - Pointer to the destination memory
                   (FIFO size aligned)
               IN  stLength - The length of the destination memory
NOTE: This function uses 16 or 32 bit access so make sure that the memory is
      appropriately aligned. Make sure there is a multiple of two or four
      bytes in length too.
Return value:  The number of bytes read from the FIFO or -1 on FIFO access
               error and -2 on overrun error
******************************************************************************/

extern  size_t R_USBH_ReadPipe(PUSB     pUSB,
                               int      iPipeNumber,
                               uint8_t  *pbyDest,
                               size_t   stLength);

/******************************************************************************
Function Name: R_USBH_DataInFIFO
Description:   Function to check the FIFO for more received data
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
Return value:  true if there is more data in the FIFO to be read
******************************************************************************/

extern  int R_USBH_DataInFIFO(PUSB pUSB, int iPipeNumber);

/******************************************************************************
Function Name: R_USBH_DmaReadPipe
Description:   Function to configure a pipe for the DMA to read from it
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The pipe to configure
               IN  wNumPackets - The number of packets to be read
Return value:  0 for success -1 on error
******************************************************************************/

extern  int R_USBH_DmaReadPipe(PUSB     pUSB,
                               int      iPipeNumber,
                               uint16_t wNumPackets);

/******************************************************************************
Function Name: R_USBH_StopDmaReadPipe
Description:   Function to unconfigure a pipe from a DMA read
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
Return value:  none
******************************************************************************/

extern  void R_USBH_StopDmaReadPipe(PUSB pUSB);

/******************************************************************************
Function Name: R_USBH_DmaFIFO
Description:   Function to get a pointer to the DMA FIFO
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  transferDirection - The direction of transfer
Return value:  Pointer to the DMA FIFO
******************************************************************************/

extern  void *R_USBH_DmaFIFO(PUSB pUSB, USBDIR transferDirection);

/******************************************************************************
Function Name: R_USBH_DmaTransac
Description:   Function to get the value of the transaction counter
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
Return value:  The value of the transaction counter
******************************************************************************/

extern  uint16_t R_USBH_DmaTransac(PUSB pUSB, int iPipeNumber);

/******************************************************************************
Function Name: R_USBH_SetDevAddrCfg
Description:   Function to set the appropriate Device Address Configuration
               Register for a given device
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  byDeviceAddress - The address of the device
               IN  pbyHubAddress - If the device is on a hub port then this
                                   must point to the address of the hub
               IN  byHubPort - If the device is on a hub then this must be set
                               to the index of the port on the hub that the
                               device is attached to
               IN  byRootPort - The index of the root port - if it is a 1 port
                                device this must be set to 0
               IN  transferSpeed - The transfer speed of the device
Return value:  true if succesful
******************************************************************************/

extern  _Bool R_USBH_SetDevAddrCfg(PUSB     pUSB,
                                   uint8_t  byDeviceAddress,
                                   uint8_t  *pbyHubAddress,
                                   uint8_t  byHubPort,
                                   uint8_t  byRootPort,
                                   USBTS    transferSpeed);

/******************************************************************************
Function Name: R_USBH_PipeReady
Description:   Function to check to see if the pipe is ready
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
Return value:  true if the pipe is ready for use
******************************************************************************/

extern  _Bool R_USBH_PipeReady(PUSB pUSB, int iPipeNumber);

/******************************************************************************
Function Name: R_USBH_ClearSetupRequest
Description:   Function to clear the setup request
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
Return value:  none
******************************************************************************/

extern  void R_USBH_ClearSetupRequest(PUSB pUSB);

/******************************************************************************
Function Name: R_USBH_EnableSetupInterrupts
Description:   Function to clear the setup request
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  bfEnable - true to enable false to disable
Return value:  none
******************************************************************************/

extern  void R_USBH_EnableSetupInterrupts(PUSB pUSB, _Bool bfEnable);

/******************************************************************************
Function Name: R_USBH_SetSOF_Speed
Description:   Function to set the SOF speed to match the device speed
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  bfEnable - true to enable false to disable
Return value:  none
******************************************************************************/

extern  void R_USBH_SetSOF_Speed(PUSB pUSB, USBTS transferSpeed);

/******************************************************************************
Function Name: R_USBH_SetControlPipeDirection
Description:   Function to set the direction of transfer of the control pipe
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  transferDirection - the direction of transfer
Return value:  none
******************************************************************************/

extern  void R_USBH_SetControlPipeDirection(PUSB    pUSB,
                                            USBDIR  transferDirection);

/******************************************************************************
Function Name: R_USBH_SendSetupPacket
Description:   Function to send a setup packet
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  byAddress - The address of the device to receive the packet
               IN  wPacketSize - The size of the device's endpoint
               IN  bmRequestType - The request bit map
               IN  bRequest - The request
               IN  wValue - The request value
               IN  wIndex - The request index
               IN  wLength - The request length
Return value:  none
******************************************************************************/

extern  void R_USBH_SendSetupPacket(PUSB        pUSB,
                                    uint8_t     byAddress,
                                    uint16_t    wPacketSize,
                                    uint8_t     bmRequestType,
                                    uint8_t     bRequest,
                                    uint16_t    wValue,
                                    uint16_t    wIndex,
                                    uint16_t    wLength);

#ifdef __cplusplus
}
#endif

#endif /* R_USBH_DRIVER_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
