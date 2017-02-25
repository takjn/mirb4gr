/* INSERT LICENSE HERE */

#ifndef _USBHOST_TYPEDEFINE_H
#define _USBHOST_TYPEDEFINE_H

#ifdef __RX
#include <stdbool.h>
#include <stdint.h>
#define _Bool	bool
#endif

#if defined(__GNUC__)||defined(GRSAKURA)
#include <stdint.h>
#include <stdbool.h>
#endif

#ifndef BIT_0                       /* set bits */
#define BIT_0                       0x1
#define BIT_1                       0x2
#define BIT_2                       0x4
#define BIT_3                       0x8
#define BIT_4                       0x10
#define BIT_5                       0x20
#define BIT_6                       0x40
#define BIT_7                       0x80

#define BIT_8                       0x100
#define BIT_9                       0x200
#define BIT_10                      0x400
#define BIT_11                      0x800
#define BIT_12                      0x1000
#define BIT_13                      0x2000
#define BIT_14                      0x4000
#define BIT_15                      0x8000

#define BIT_16                      0x10000L
#define BIT_17                      0x20000L
#define BIT_18                      0x40000L
#define BIT_19                      0x80000L
#define BIT_20                      0x100000L
#define BIT_21                      0x200000L
#define BIT_22                      0x400000L
#define BIT_23                      0x800000L

#define BIT_24                      0x1000000L
#define BIT_25                      0x2000000L
#define BIT_26                      0x4000000L
#define BIT_27                      0x8000000L
#define BIT_28                      0x10000000L
#define BIT_29                      0x20000000L
#define BIT_30                      0x40000000L
#define BIT_31                      0x80000000L
#endif

#ifndef SWAPWORD
#define SWAPWORD(x)           (uint16_t)((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF))
#endif

#ifndef MAKEWORD
#define MAKEWORD(a, b)        ((uint16_t) (((uint8_t) (a)) |\
                              ((uint16_t) ((uint8_t) (b))) << 8))
#endif

#ifndef true                        /* true and false */
#define true                        (_Bool)1
#endif

#ifndef false
#define false                       (_Bool)0
#endif

typedef enum _CTLCODE
{
    CTL_FILE_SEEK = 0,
    CTL_FILE_SIZE,
    CTL_GET_LAST_ERROR,
    CTL_GET_ERROR_STRING,
    CTL_GET_VERSION,
    CTL_USB_ATTACHED,
    CTL_OVERLAPPED_READ,
    CTL_OVERLAPPED_READ_RESULT,
    CTL_OVERLAPPED_READ2,
    CTL_OVERLAPPED_READ_RESULT2,
    CTL_OVERLAPPED_READ3,
    CTL_OVERLAPPED_READ_RESULT3,
    CTL_OVERLAPPED_READ4,
    CTL_OVERLAPPED_READ_RESULT4,
    CTL_OVERLAPPED_WRITE,
    CTL_OVERLAPPED_WRITE_RESULT,
    CTL_OVERLAPPED_WRITE2,
    CTL_OVERLAPPED_WRITE_RESULT2,
    CTL_OVERLAPPED_WRITE3,
    CTL_OVERLAPPED_WRITE_RESULT3,
    CTL_OVERLAPPED_WRITE4,
    CTL_OVERLAPPED_WRITE_RESULT4,
    CTL_CANCEL_OVERLAPPED_READ,
    CTL_CANCEL_OVERLAPPED_READ2,
    CTL_CANCEL_OVERLAPPED_READ3,
    CTL_CANCEL_OVERLAPPED_READ4,
    CTL_CANCEL_OVERLAPPED_WRITE,
    CTL_CANCEL_OVERLAPPED_WRITE2,
    CTL_CANCEL_OVERLAPPED_WRITE3,
    CTL_CANCEL_OVERLAPPED_WRITE4,
    CTL_SET_TIME_OUT,
    CTL_GET_TIME_OUT,
    CTL_USB_MS_RESET,
    CTL_USB_MS_GET_MAX_LUN,
    CTL_USB_MS_CLEAR_BULK_IN_STALL,
    CTL_SCI_SET_CONFIGURATION,
    CTL_SCI_GET_CONFIGURATION,
    CTL_SCI_GET_LINE_STATUS,
    CTL_SCI_PURGE_BUFFERS,
    CTL_SCI_SET_BREAK,
    CTL_SCI_CLEAR_BREAK,
    CTL_GET_RX_BUFFER_COUNT,
    CTL_GET_DATE,
    CTL_SET_DATE,
    CTL_STD_IN_ADD_FILE_DESCIPTOR,
    CTL_STD_MERGE_KEY_PRESS,
    CTL_GET_MOUSE_DATA,
    CTL_GET_SPEAKER_VOLUME,
    CTL_SET_SPEAKER_VOLUME,
    CTL_INC_SPEAKER_VOLUME,
    CTL_DEC_SPEAKER_VOLUME,
    CTL_SET_SPEAKER_MUTE,
    CTL_GET_SPEAKER_SAMPLE_RATE

    /* TODO: add device specific control functions here */

} CTLCODE;

#endif	//_USBHOST_TYPEDEFINE_H
