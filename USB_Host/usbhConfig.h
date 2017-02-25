/* INSERT LICENSE HERE */

#ifndef USBHCONFIG_H_INCLUDED
#define USBHCONFIG_H_INCLUDED

/******************************************************************************
Macro definitions
******************************************************************************/

/* Define the USB Host controller hardware structure tag here */
#define USBH_HOST_STRUCT_TAG        st_usb0
/* Define the number of root ports 1 or 2 on each host controller */
#define USBH_NUM_ROOT_PORTS         1
/* Define high speed support 1 for SH on chip peripherals.
                   Currently 0 for RX on chip peripherals */
#define USBH_HIGH_SPEED_SUPPORT     0
/* Define the FIFO access size only 32 and 16 are valid here.
   Usually 32 for SH on chip peripherals.
   Currenly 16 for RX on chip peripherals */
#define USBH_FIFO_BIT_WIDTH         16
/* The maximum number of host controllers supported */
#define USBH_MAX_CONTROLLERS        1
/* The number of tiers of hubs supported */
#define USBH_MAX_TIER               1
/* The maximum number of hubs that can be connected */
#define USBH_MAX_HUBS               2
/* The maximum number of ports */
#define USBH_MAX_PORTS              10
/* The maximum number of devices */
#define USBH_MAX_DEVICES            10
#if USBH_HIGH_SPEED_SUPPORT == 1
#define USBH_MAX_ADDRESS            10
#else
#define USBH_MAX_ADDRESS            5
#endif
/* The maximum number of endpoints */
#define USBH_MAX_ENDPOINTS          64

#endif                              /* USBHCONFIG_H_INCLUDED */

/******************************************************************************
End  Of File
******************************************************************************/
