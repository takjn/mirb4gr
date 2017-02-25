/* INSERT LICENSE HERE */

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include <string.h>
#if !defined(__GNUC__) && !defined(GRSAKURA)
#include <machine.h>
#endif

#include "r_usbh_driver.h"
#include "iodefine.h"

#ifndef _TRACE_ON_
#undef TRACE
#define TRACE(x)	((void)0)
#define ASSERT(x)	((void)0)
#endif

/******************************************************************************
Constant macro definitions
******************************************************************************/

/* DMA only available on pipes 1, 2, 3, 4 and 5 - Pipes 6 - 9 the DMA can 
   only fill one packet (Interrupt transfers) and is not supported by this
   driver. */
#define USBH_NUM_DMA_ENABLED_PIPES  5

/* FIFO Configuration */
#define USB_PIPEBUF_BUFSIZE(x)      ((uint16_t)(((x) / 64) - 1) << 10)
#define USB_PIPEBUF_BUFNMB(x)       ((uint16_t)((x) & 0xFF))

#define USB_FIFO_CONFIG_SIZE        (sizeof(gcFifoConfig) /\
                                     sizeof(struct _FIFOCFG))
/* CFIFOSEL register */
#define USB_CFIFOSEL_RCNT           BIT_15
#define USB_CFIFOSEL_REW            BIT_14
#define USB_CFIFOSEL_MBW(x)         ((uint16_t)(((x) & 0x03) << 10))
#define USB_CFIFOSEL_BIG_END        BIT_8
#define USB_CFIFOSEL_ISEL           BIT_5
#define USB_CFIFOSEL_CURPIPE(x)     ((uint16_t)((uint16_t)x & 0x0F))

/* CFIFOCTR register */
#define USB_CFIFOCTR_BVAL           BIT_15
#define USB_CFIFOCTR_BCLR           BIT_14
#define USB_CFIFOCTR_FRDY           BIT_13
#define USB_CFIFOCTR_DTLN(x)        ((uint16_t)((x) & 0x0FFF))

/* CFIFOSIE register */
#define USB_CFIFOSIE_TGL            BIT_15
#define USB_CFIFOSIE_SCLR           BIT_14
#define USB_CFIFOSIE_SBUSY          BIT_13

/* D0FIFOSEL register */
#define USB_D0FIFOSEL_RCNT          BIT_15
#define USB_D0FIFOSEL_REW           BIT_14
#define USB_D0FIFOSEL_DCLRM         BIT_13
#define USB_D0FIFOSEL_DREQE         BIT_12
#define USB_D0FIFOSEL_MBW(x)        ((uint16_t)(((x) & 0x03) << 10))
#define USB_D0FIFOSEL_BIG_END       BIT_8
#define USB_D0FIFOSEL_CURPIPE(x)    ((uint16_t)((uint16_t)x & 0x0F))

/* D0FIFOCTR register */
#define USB_D0FIFOCTR_BVAL          BIT_15
#define USB_D0FIFOCTR_BCLR          BIT_14
#define USB_D0FIFOCTR_FRDY          BIT_13
#define USB_D0FIFOCTR_DTLN          ((uint16_t)((x) & 0x0FFF))

/* D1FIFOSEL register */
#define USB_D1FIFOSEL_RCNT          BIT_15
#define USB_D1FIFOSEL_REW           BIT_14
#define USB_D1FIFOSEL_DCLRM         BIT_13
#define USB_D1FIFOSEL_DREQE         BIT_12
#define USB_D1FIFOSEL_MBW(x)        ((uint16_t)(((x) & 0x03) << 10))
#define USB_D1FIFOSEL_BIG_END       BIT_8
#define USB_D1FIFOSEL_CURPIPE(x)    ((uint16_t)((uint16_t)x & 0x0F))

/* D1FIFOCTR register */
#define USB_D1FIFOCTR_BVAL          BIT_15
#define USB_D1FIFOCTR_BCLR          BIT_14
#define USB_D1FIFOCTR_FRDY          BIT_13
#define USB_D1FIFOCTR_DTLN          ((uint16_t)((x) & 0x0FFF))

/* INTENB0 register */
/* For the Full speed peripheral on the RX devices */
#define USB_INTENB0_OVRCRE          BIT_15
/* For the high speed peripheral on the SH devices */
#define USB_INTENB0_VBSE            BIT_15
#define USB_INTENB0_RSME            BIT_14
#define USB_INTENB0_SOFE            BIT_13
#define USB_INTENB0_DVSE            BIT_12
#define USB_INTENB0_CTRE            BIT_11
#define USB_INTENB0_BEMPE           BIT_10
#define USB_INTENB0_NRDYE           BIT_9
#define USB_INTENB0_BRDYE           BIT_8

/* INTSTS0 register */
#define USB_INTSTS0_VBINT           BIT_15
#define USB_INTSTS0_RSME            BIT_14
#define USB_INTSTS0_SOFR            BIT_13
#define USB_INTSTS0_DVST            BIT_12
#define USB_INTSTS0_CTRT            BIT_11
#define USB_INTSTS0_BEMP            BIT_10
#define USB_INTSTS0_NRDY            BIT_9
#define USB_INTSTS0_BRDY            BIT_8
#define USB_INTSTS0_VBSTS           BIT_7
#define USB_INTSTS0_VALID           BIT_3

/* INTSTS1 register */
#define USB_INTSTS1_BCHGE           BIT_14
#define USB_INTSTS1_DTCH            BIT_12
#define USB_INTSTS1_ATTCH           BIT_11
#define USB_INTSTS1_NRDY            BIT_9
#define USB_INTSTS1_BRDY            BIT_8
#define USB_INTSTS1_EOFERR          BIT_6
#define USB_INTSTS1_SIGN            BIT_5
#define USB_INTSTS1_SACK            BIT_4

/******************************************************************************
Function  macro definitions
******************************************************************************/

#define USB_DEVADD(p,n)             ((PDEVADDR)(((uint8_t*)p) + gpstDevAddr[n]))
#define USB_PIPECTR(p,n)            ((PPIPECTR)(((uint8_t*)p) + gpstPipeCtr[n]))
#define USB_PIPETRE(p,n)            ((PPIPETRE)(((uint8_t*)p) + gpstPipeTre[n]))
#define USB_PIPETRN(p,n)            ((PIPENTRN)(((uint8_t*)p) + gpstPipeTrn[n]))
/* Modification of the PIPESEL, CFIFOSEL, D0FIFOSEL and D1FIFOSEL registers
   causes the function of other registers to change.
   Read back from the register to make sure that out of order execution
   does not mean other registeres are accesse before this one is set. */
#define USB_PIPESEL(p,n)            do {p->PIPESEL.BIT.PIPESEL = n;}\
                                    while (p->PIPESEL.BIT.PIPESEL != n)
#define USB_CFIFOSEL(p,n)           r_usbh_cfiosel(p, n)
#define USB_D0FIFOSEL(p,n)          do {p->D0FIFOSEL.WORD = n;}\
                                    while (p->D0FIFOSEL.WORD != n)
#define USB_D1FIFOSEL(p,n)          do {p->D1FIFOSEL.WORD = n;}\
                                    while (p->D1FIFOSEL.WORD != n)

/******************************************************************************
Typedef definitions
******************************************************************************/

#if defined( __RX )
#pragma bit_order left
#endif

#if defined(__RX_LITTLE_ENDIAN__)
/* Define the structure of the Device Address Configuration Register */
typedef volatile union _DEVADDR
{
    uint16_t  WORD;
    struct {
        uint16_t RTPOR:1;
        uint16_t :5;
        uint16_t USBSPD:2;
        uint16_t HUBPORT:3;
        uint16_t UPPHUB:4;
        uint16_t :1;
    } BIT;
} DEVADDR,
*PDEVADDR;
/* Define a structure of pointers to the pipe control registers */
typedef volatile struct _PIPEREG
{
    /* Pipe control regiters 1..9 */
    volatile union _PIPENCTR
    {
        uint16_t WORD;
        struct {
            uint16_t PID:2;
            uint16_t :3;
            uint16_t PBUSY:1;
            uint16_t SQMON:1;
            uint16_t SQSET:1;
            uint16_t SQCLR:1;
            uint16_t ACLRM:1;
            uint16_t ATREPM:1;        /* Not applicable to pipes 6..9 */
            uint16_t :1;
            uint16_t CSSTS:1;
            uint16_t CSCLR:1;
            uint16_t INBUFM:1;        /* Not applicable to pipes 6..9 */
            uint16_t BSTS:1;
        } BIT;
    } *PPIPECTR;
    /* Pipe transaction counter enable registers 1..5 */
    volatile union _PIPENTRE
    {
        uint16_t WORD;
        struct {
            uint16_t :8;
            uint16_t TRCLR:1;
            uint16_t TRENB:1;
            uint16_t :6;
        } BIT;
    } *PPIPETRE;
    /* Pipe transaction counter register 1..5 */
    volatile uint16_t *PIPENTRN;
} PIPEREG,
*PPIPEREG;
/* The structure of the pipe control registers */
typedef volatile union
{
    uint16_t WORD;
    struct {
        uint16_t PID:2;
        uint16_t :3;
        uint16_t PBUSY:1;
        uint16_t SQMON:1;
        uint16_t SQSET:1;
        uint16_t SQCLR:1;
        uint16_t ACLRM:1;
        uint16_t ATREPM:1;        /* Not applicable to pipes 6..9 */
        uint16_t :1;
        uint16_t CSSTS:1;
        uint16_t CSCLR:1;
        uint16_t INBUFM:1;        /* Not applicable to pipes 6..9 */
        uint16_t BSTS:1;
    } BIT;
} *PPIPECTR;
/* Define the structure of the DMA transaction control register */
typedef volatile union
{
    uint16_t WORD;
    struct {
        uint16_t :8;
        uint16_t TRCLR:1;
        uint16_t TRENB:1;
        uint16_t :6;
    } BIT;
} *PPIPETRE;
/* The structure of the DMA transaction counter register */
typedef volatile uint16_t *PIPENTRN;
#else
/* Define the structure of the Device Address Configuration Register */
typedef volatile union _DEVADDR
{
    uint16_t  WORD;
    struct {
        uint16_t :1;
        uint16_t UPPHUB:4;
        uint16_t HUBPORT:3;
        uint16_t USBSPD:2;
        uint16_t :5;
        uint16_t RTPOR:1;
    } BIT;
} DEVADDR,
*PDEVADDR;
/* Define a structure of pointers to the pipe control registers */
typedef volatile struct _PIPEREG
{
    /* Pipe control regiters 1..9 */
    volatile union _PIPENCTR
    {
        uint16_t WORD;
        struct {
            uint16_t BSTS:1;
            uint16_t INBUFM:1;        /* Not applicable to pipes 6..9 */
            uint16_t CSCLR:1;
            uint16_t CSSTS:1; 
            uint16_t :1;
            uint16_t ATREPM:1;        /* Not applicable to pipes 6..9 */
            uint16_t ACLRM:1;
            uint16_t SQCLR:1;
            uint16_t SQSET:1;
            uint16_t SQMON:1;
            uint16_t PBUSY:1;
            uint16_t :3;
            uint16_t PID:2;
        } BIT;
    } *PPIPECTR;
    /* Pipe transaction counter enable registers 1..5 */
    volatile union _PIPENTRE
    {
        uint16_t WORD;
        struct {
            uint16_t :6;
            uint16_t TRENB:1;
            uint16_t TRCLR:1;
            uint16_t :8;
        } BIT;
    } *PPIPETRE;
    /* Pipe transaction counter register 1..5 */
    volatile uint16_t *PIPENTRN;
} PIPEREG,
*PPIPEREG;
/* The structure of the pipe control registers */
typedef volatile union
{
    uint16_t WORD;
    struct {
        uint16_t BSTS:1;
        uint16_t INBUFM:1;        /* Not applicable to pipes 6..9 */
        uint16_t CSCLR:1;
        uint16_t CSSTS:1;
        uint16_t :1;
        uint16_t ATREPM:1;        /* Not applicable to pipes 6..9 */
        uint16_t ACLRM:1;
        uint16_t SQCLR:1;
        uint16_t SQSET:1;
        uint16_t SQMON:1;
        uint16_t PBUSY:1;
        uint16_t :3;
        uint16_t PID:2;
    } BIT;
} *PPIPECTR;
/* Define the structure of the DMA transaction control register */
typedef volatile union
{
    uint16_t WORD;
    struct {
        uint16_t :6;
        uint16_t TRENB:1;
        uint16_t TRCLR:1;
        uint16_t :8;
    } BIT;
} *PPIPETRE;
/* The structure of the DMA transaction counter register */
typedef volatile uint16_t *PIPENTRN;
#endif

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/

/******************************************************************************
Private Constant Data
******************************************************************************/
#if USBH_HIGH_SPEED_SUPPORT == 1
/* Define the the FIFO buffer configuration */
static const struct _FIFOCFG
{
    /* FIFO buffer size and number settings */
    uint16_t    wPipeBuf;

} gcFifoConfig[] =
{
    /* Pipe 0 (not in table) always uses buffer number 0..3 */
    /* Pipes 6..9 have fixed FIFO locations taking 4,5,6 & 7 */
    /* Therefore start buffer allocation at 8. In this case
       1kByte of FIFO has been assigned to pipes 1 to 5 and the
       buffer size set to 512. This means they can all operate as
       double buffered high speed bulk endpoints */
    /* Pipe 1 - ISOC & BULK */
    /* Buffer Size */
    USB_PIPEBUF_BUFSIZE(512) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB(8),
    /* Pipe 2 - ISOC & BULK */
    /* Buffer Size */
    USB_PIPEBUF_BUFSIZE(512) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB(24),
    /* Pipe 3 - BULK */
    /* Buffer Size */
    USB_PIPEBUF_BUFSIZE(512) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB(40),
    /* Pipe 4 - BULK */
    /* Buffer Size */
    USB_PIPEBUF_BUFSIZE(512) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB(56),
    /* Pipe 5 - BULK */
    /* Buffer Size */
    USB_PIPEBUF_BUFSIZE(512) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB(72),
    /* Pipe 6 - INTERRUPT - Fixed to buffer 4 */
    /* Buffer Size */
    USB_PIPEBUF_BUFSIZE(64) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB(4),
    /* Pipe 7 - INTERRUPT - Fixed to buffer 5 */
    /* Buffer Size */
    USB_PIPEBUF_BUFSIZE(64) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB(5),
    /* Pipe 8 - INTERRUPT - Fixed to buffer 6 */
    /* Buffer Size */
    USB_PIPEBUF_BUFSIZE(64) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB(6),
    /* Pipe 9 - INTERRUPT - Fixed to buffer 7 */
    /* Buffer Size */
    USB_PIPEBUF_BUFSIZE(64) |
    /* Buffer Number */
    USB_PIPEBUF_BUFNMB(7),
};
#endif

/* Table of offsets to the device address registers */
#if USBH_HIGH_SPEED_SUPPORT == 1
static const size_t gpstDevAddr[] =
{
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD0.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD1.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD2.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD3.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD4.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD5.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD6.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD7.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD8.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD9.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADDA.WORD
};
#else
/* Full speed only variant only supports one hub and 4 devices */
static const volatile size_t gpstDevAddr[] =
{
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD0.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD1.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD2.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD3.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD4.WORD,
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DEVADD5.WORD,
};
#endif

static const size_t gpstPipeCtr[] =
{
    /* Pipe 0 is not valid */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).DCPCTR.WORD,
    /* Pipe 1 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE1CTR.WORD,
    /* Pipe 2 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE2CTR.WORD,
    /* Pipe 3 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE3CTR.WORD,
    /* Pipe 4 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE4CTR.WORD,
    /* Pipe 5 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE5CTR.WORD,
    /* Pipe 6 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE6CTR.WORD,
    /* Pipe 7 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE7CTR.WORD,
    /* Pipe 8 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE8CTR.WORD,
    /* Pipe 9 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE9CTR.WORD,
};

static const size_t gpstPipeTre[] =
{
    /* Pipe 0 - Control pipe does not have a transaction counter */
    0UL,
    /* Pipe 1 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE1TRE.WORD,
    /* Pipe 2 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE2TRE.WORD,
    /* Pipe 3 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE3TRE.WORD,
    /* Pipe 4 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE4TRE.WORD,
    /* Pipe 5 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE5TRE.WORD,
    /* Pipe 6 - Pipes for interrupt transfer don't have transaction counters */
    0UL,
    /* Pipe 7 */
    0UL,
    /* Pipe 8 */
    0UL,
    /* Pipe 9 */
    0UL,
};

static const size_t gpstPipeTrn[] =
{
    /* Pipe 0 - Control pipe does not have a transaction counter */
    0UL,
    /* Pipe 1 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE1TRN,
    /* Pipe 2 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE2TRN,
    /* Pipe 3 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE3TRN,
    /* Pipe 4 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE4TRN,
    /* Pipe 5 */
    (size_t)&(*(struct USBH_HOST_STRUCT_TAG*)0).PIPE5TRN,
    /* Pipe 6 - Pipes for interrupt transfer don't have transaction counters */
    0UL,
    /* Pipe 7 */
    0UL,
    /* Pipe 8 */
    0UL,
    /* Pipe 9 */
    0UL
};

/******************************************************************************
Private global variables and functions
******************************************************************************/

#if USBH_HIGH_SPEED_SUPPORT == 1
static void r_usbh_config_fifo(PUSB pUSB);
#endif
static int r_usbh_wait_fifo(PUSB pUSB, int iPipeNumber, int iCountOut);
static void r_usbh_init_pipes(PUSB pUSB);
static void r_usbh_write_fifo(PUSB pUSB, void *pvSrc, size_t stLength);
static void r_usbh_read_fifo(PUSB pUSB, void *pvDest, size_t stLength);
static void r_usbh_cfiosel(PUSB pUSB, uint16_t usCFIFOSEL);

/******************************************************************************
Renesas Abstracted Host Driver API functions
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_Initialise
Description:   Function to initialise the USB peripheral for operation as the
               Host Controller
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
Return value:  none
******************************************************************************/
void R_USBH_Initialise(PUSB pUSB)
{
#if USBH_HIGH_SPEED_SUPPORT == 1
    /* Set AC Switching Charactristics (from HW Manual) */
    pUSB->USBACSWR1.BIT.PREENA = 1;
    /* Enable host controller function */
    pUSB->SYSCFG.WORD = (BIT_10 | BIT_7 | BIT_6 | BIT_5 | BIT_0);
    pUSB->SYSCFG.WORD |= (BIT_7 | BIT_5);
    #if USBH_NUM_ROOT_PORTS == 2
    pUSB->SYSCFG1.WORD |= (BIT_7 | BIT_5);
    #endif
    /* Configure the pipe FIFO buffer assignment */
    r_usbh_config_fifo(pUSB);
    /* Initialise the pipes */
    r_usbh_init_pipes(pUSB);
    /* Enable the interrupts */
    pUSB->INTENB0.WORD = USB_INTENB0_SOFE
                       | USB_INTENB0_BEMPE
                       | USB_INTENB0_BRDYE;
    pUSB->INTENB1.WORD = 0;
    #if USBH_NUM_ROOT_PORTS == 2
    pUSB->INTENB2.WORD = 0;
    #endif
#else
    /* Driver configuration sanity check */
    #if USBH_NUM_ROOT_PORTS == 2
        /* This is to help people who see an RSK with two ports and assume
           that support for the second port is obtained by setting the
           USBH_NUM_ROOT_PORTS define to 2. Two root ports are supported
           by passing the functions in this driver a pointer (pUSB) to the
           appropriate host controller peripheral. */
        #error "Two root ports are not supported by the full speed variant"
    #endif
    /* Enable host controller function - BIT_7 for High Speed Enable removed */
    pUSB->SYSCFG.WORD = (BIT_10 | BIT_6 | BIT_5);
    /* HM 27.2.1 this bit should be set to 1 after setting
       the SYSCFG.DRPD bit to 1, eliminating LNST bit chattering, and checking
       that the USB bus state has been settled.*/
    //while( pUSB->SYSSTS0.BIT.LNST != 0);
    int i, buf1, buf2, buf3;

    do{
    	buf1 = pUSB->SYSSTS0.BIT.LNST;
    	for(i = 0; i < 500; i++);
    	buf2 = pUSB->SYSSTS0.BIT.LNST;
    	for(i = 0; i < 500; i++);
    	buf3 = pUSB->SYSSTS0.BIT.LNST;
    	for(i = 0; i < 500; i++);
    }while(buf1!=buf2 && buf2!=buf3 && buf3!=buf1);

    pUSB->SYSCFG.WORD |= (BIT_0);
    /* Initialise the pipes */
    r_usbh_init_pipes(pUSB);
    /* Enable the interrupts */
    pUSB->INTENB0.WORD = USB_INTENB0_SOFE
                       | USB_INTENB0_BEMPE
                       | USB_INTENB0_BRDYE
                       | USB_INTENB0_OVRCRE;
    pUSB->INTENB1.WORD = 0;

#endif
}
/******************************************************************************
End of function  usbInitialise
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_Uninitialise
Description:   Function to uninitialise the USB peripheral for operation as the
               host controller
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
Return value:  none
******************************************************************************/
void R_USBH_Uninitialise(PUSB pUSB)
{
    #if USBH_HIGH_SPEED_SUPPORT == 1
    /* Disable host controller function */
    pUSB->SYSCFG.WORD &= ~(BIT_10 | BIT_7 | BIT_6 | BIT_5 | BIT_0);
    pUSB->SYSCFG.WORD &= ~(BIT_7 | BIT_5);
    #else
    /* Disable host controller function */
    pUSB->SYSCFG.WORD &= ~(BIT_10 | BIT_6 | BIT_0);
    pUSB->SYSCFG.WORD &= ~(BIT_5);
    #endif
    #if USBH_NUM_ROOT_PORTS == 2
    pUSB->SYSCFG1.WORD &= ~(BIT_7 | BIT_5);
    #endif
    /* Disable the interrupts */
    pUSB->INTENB0.WORD = 0;
    pUSB->INTENB1.WORD = 0;
    #if USBH_NUM_ROOT_PORTS == 2
    pUSB->INTENB2.WORD = 0;
    #endif
}
/******************************************************************************
End of function  R_USBH_Uninitialise
******************************************************************************/

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
int R_USBH_ConfigurePipe(PUSB       pUSB,
                         int        iPipeNumber,
                         USBTT      transferType,
                         USBDIR     transferDirection,
                         uint8_t    byDeviceAddress,
                         uint8_t    byEndpointNumber,
                         uint8_t    byInterval,
                         uint16_t   wPacketSize)
{
    /* This function is not for the control pipe */
    if (iPipeNumber <= 0)
    {
        return -1;
    }
    /* Set pipe PID to NAK before setting the configuration */
    USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;
    /* Select the pipe */
    USB_PIPESEL(pUSB, iPipeNumber);
    /* Set the transfer direction */
    if (transferDirection == USBH_OUT)
    {
        pUSB->PIPECFG.BIT.DIR = 1;
        /* Switch double buffering on for bulk OUT transfers */
        #if USBH_HIGH_SPEED_SUPPORT == 1
        /*
            ** POSSIBLE BUG IN FULL SPEED VARIANT OF 597 IP **
            There is a possible bug in the Full speed variant of the 597 IP.
            When double buffering is enabled and a data of length equal to the
            value set in the PIPEMAXP MXPS field is written to the FIFO
            spurious data is transmitted. Please check errata's and ECN & remove
            conditional compile when fixed.
        */
        if (transferType == USBH_BULK)
        {
            pUSB->PIPECFG.BIT.DBLB = 1;
        }
        #endif
    }
    else if (transferDirection == USBH_IN)
    {
        /* Clear the pipe FIFO in preparation for transfer */
        pUSB->CFIFOCTR.BIT.BCLR = 1;
        /* Set the transfer direction */
        pUSB->PIPECFG.BIT.DIR = 0;
        /* Check for an isochronus transfer */
        if (transferType == USBH_ISOCHRONOUS)
        {
            /* Set the Isochronus in buffer flush bit */
            pUSB->PIPEPERI.BIT.IFIS = 1;
        }
    }
    else
    {
        /* This function does not work for control functions */
        return -1;
    }
    /* Set the transfer type */
    switch (transferType)
    {
        case USBH_ISOCHRONOUS:
        pUSB->PIPECFG.BIT.TYPE = 3;
        /* Set the isoc schedule interval */
        pUSB->PIPEPERI.BIT.IITV = (byInterval - 1);
        /* Set the packet size */
        pUSB->PIPEMAXP.BIT.MXPS = wPacketSize;
        break;
        case USBH_BULK:
        pUSB->PIPEPERI.WORD = 0;
        pUSB->PIPECFG.BIT.TYPE = 1;
        /* Set the packet size */
        pUSB->PIPEMAXP.BIT.MXPS = wPacketSize;
        break;
        case USBH_INTERRUPT:
        pUSB->PIPECFG.BIT.TYPE = 2;
        /* Set the interrupt schedule interval */
        pUSB->PIPEPERI.BIT.IITV = (byInterval - 1);
        /* Set the packet size */
        pUSB->PIPEMAXP.BIT.MXPS = wPacketSize;
        break;
        default:
        /* This function does not work for control functions */
        return -1;
    }
    /* Set the endpoint number */
    pUSB->PIPECFG.BIT.EPNUM = byEndpointNumber;
    /* Set the device address */
    pUSB->PIPEMAXP.BIT.DEVSEL = byDeviceAddress;
    return iPipeNumber;
}
/******************************************************************************
End of function  R_USBH_ConfigurePipe
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_UnconfigurePipe
Description:   Function to unconfigure a pipe so it can be used for another
               transfer
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe to unconfigure
Return value:  none
******************************************************************************/
void R_USBH_UnconfigurePipe(PUSB pUSB, int iPipeNumber)
{
    if (iPipeNumber > 0)
    {
        /* Disable the pipe */
        USB_PIPECTR(pUSB, iPipeNumber)->WORD = 0;
        /* Clear the FIFO */
        R_USBH_ClearPipeFifo(pUSB, iPipeNumber);
        /* Select the pipe */
        USB_PIPESEL(pUSB, iPipeNumber);
        /* Clear the settings */
        pUSB->PIPECFG.WORD = 0;
        pUSB->PIPEPERI.WORD = 0;
        pUSB->PIPEMAXP.WORD = 0;
    }
}
/******************************************************************************
End of function  R_USBH_UnconfigurePipe
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_ClearPipeFifo
Description:   Function to clear the pipe FIFO
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe to clear
Return value:  none
******************************************************************************/
void R_USBH_ClearPipeFifo(PUSB pUSB, int iPipeNumber)
{
    if (iPipeNumber > 0)
    {
        /* If the pipe is active */
        if (USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID)
        {
            int iCountOut = 1000;
            /* Make sure that it is not busy */
            while (USB_PIPECTR(pUSB, iPipeNumber)->BIT.PBUSY)
            {
                if (iCountOut)
                {
                    iCountOut--;
                }
                else
                {
                    break;
                }
            }
        }
        USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 1;
        USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 0;
    }
    else if (iPipeNumber == 0)
    {
        /* No express method for the DCP, select DCP */
        #if USBH_FIFO_BIT_WIDTH == 32
        USB_CFIFOSEL(pUSB, USB_CFIFOSEL_MBW(2));
        #else
        USB_CFIFOSEL(pUSB, USB_CFIFOSEL_MBW(1));
        #endif
        /* Set the buffer clear bit */
        pUSB->CFIFOCTR.BIT.BCLR = 1;
    }
}
/******************************************************************************
End of function  R_USBH_ClearPipeFifo
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_SetPipePID
Description:   Function to set the pipe PID
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
               IN  dataPID - The Data PID setting
Return value:  none
******************************************************************************/
void R_USBH_SetPipePID(PUSB pUSB, int iPipeNumber, USBDP dataPID)
{
    if (iPipeNumber > 0)
    {
        /* Set the pipe to NAK */
        USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;
        /* Set the endpoint PID */
        if (dataPID == USBH_DATA1)
        {
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.SQSET = 1;
        }
        else
        {
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.SQCLR = 1;
        }
    }
    else if (iPipeNumber == 0)
    {
        /* Disable the control pipe */
        pUSB->DCPCTR.BIT.PID = 0;
        /* Set the data PID */
        if (dataPID == USBH_DATA1)
        {
            pUSB->DCPCTR.BIT.SQSET = 1;
        }
        else
        {
            pUSB->DCPCTR.BIT.SQCLR = 1;
        }
    }
}
/******************************************************************************
End of function  R_USBH_SetPipePID
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_GetPipePID
Description:   Function to get the pipe PID
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
Return value:  The state of the data PID
******************************************************************************/
USBDP R_USBH_GetPipePID(PUSB pUSB, int iPipeNumber)
{
    if (iPipeNumber > 0)
    {
        if (USB_PIPECTR(pUSB, iPipeNumber)->BIT.SQMON)
        {
            return USBH_DATA1;
        }
        else
        {
            return USBH_DATA0;
        }
    }
    else if (iPipeNumber == 0)
    {
        if (pUSB->DCPCTR.BIT.SQMON)
        {
            return USBH_DATA1;
        }
        else
        {
            return USBH_DATA0;
        }
    }
    return USBH_DATA0;
}
/******************************************************************************
End of function  R_USBH_GetPipePID
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_GetPipeStall
Description:   Function to get the pipe stall state
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
Return value:  true if the pipe has received a stall token
******************************************************************************/
_Bool R_USBH_GetPipeStall(PUSB pUSB, int iPipeNumber)
{
    if (iPipeNumber > 0)
    {
        if (USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID > 1)
        {
            return true;
        }
    }
    else if (iPipeNumber == 0)
    {
        if (pUSB->DCPCTR.BIT.PID > 1)
        {
            return true;
        }
    }
    return false;
}
/******************************************************************************
End of function  R_USBH_GetPipeStall
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_EnablePipe
Description:   Function to Enable (ACK) or Disable (NAK) the pipe
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
               IN  bfEnable - True to enable transfers on the pipe
Return value:  none
******************************************************************************/
void R_USBH_EnablePipe(PUSB pUSB, int iPipeNumber, _Bool bfEnable)
{
       if (bfEnable)
        {
            /* Enable the pipe */
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 1;
        }
        else
        {
            /* Disable the pipe */
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;
        }
}
/******************************************************************************
End of function  R_USBH_EnablePipe
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_SetPipeInterrupt
Description:   Function to set or clear the desired pipe interrupts
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
               IN  buffIntType - The interrupt type
Return value:  none
******************************************************************************/
void R_USBH_SetPipeInterrupt(PUSB  pUSB,
                             int   iPipeNumber,
                             USBIP buffIntType)
{
    uint16_t    wISet, wIClear;
    if (iPipeNumber >= 0)
    {
        wISet = (uint16_t)(1 << iPipeNumber);
    }
    else
    {
        wISet = 0x3FF;
    }
    wIClear = (uint16_t)~wISet;
    /* Pipe buffer empty interrupt */
    if (buffIntType & USBH_PIPE_BUFFER_EMPTY)
    {
        if (buffIntType & USBH_PIPE_INT_DISABLE)
        {
            pUSB->BEMPENB.WORD &= wIClear;
        }
        else
        {
            pUSB->BEMPENB.WORD |= wISet;
        }
    }
    /* Pipe buffer ready interrupt */
    if (buffIntType & USBH_PIPE_BUFFER_READY)
    {
        if (buffIntType & USBH_PIPE_INT_DISABLE)
        {
            pUSB->BRDYENB.WORD &= wIClear;
        }
        else
        {
            pUSB->BRDYENB.WORD |= wISet;
        }
    }
    /* Pipe buffer not ready interrupt */
    if (buffIntType & USBH_PIPE_BUFFER_NOT_READY)
    {
        if (buffIntType & USBH_PIPE_INT_DISABLE)
        {
            pUSB->NRDYENB.WORD &= wIClear;
        }
        else
        {
            pUSB->NRDYENB.WORD |= wISet;
        }
    }    
}
/******************************************************************************
End of function  R_USBH_SetPipeInterrupt
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_ClearPipeInterrupt
Description:   Function to clear the desired pipe interrupt status flag
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
               IN  buffIntType - The interrupt type
Return value:  none
******************************************************************************/
void R_USBH_ClearPipeInterrupt(PUSB  pUSB,
                               int   iPipeNumber,
                               USBIP buffIntType)
{
    uint16_t    wIClear = (uint16_t)~((uint16_t)(1 << iPipeNumber));
    /* Pipe buffer empty interrupt */
    if (buffIntType & USBH_PIPE_BUFFER_EMPTY)
    {
        pUSB->BEMPSTS.WORD = wIClear;
    }
    /* Pipe buffer ready interrupt */
    if (buffIntType & USBH_PIPE_BUFFER_READY)
    {
        pUSB->BRDYSTS.WORD = wIClear;
    }
    /* Pipe buffer not ready interrupt */
    if (buffIntType & USBH_PIPE_BUFFER_NOT_READY)
    {
        pUSB->NRDYSTS.WORD = wIClear;
    }
}
/******************************************************************************
End of function  R_USBH_ClearPipeInterrupt
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_GetPipeInterrupt
Description:   Function to get the state of the pipe interrupt status flag
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe or USBH_PIPE_NUMBER_ANY
               IN  buffIntType - The interrupt type
               IN  bfQualify - Set true to qualify with enabled interrupts
Return value:  true if the flag is set.
******************************************************************************/
_Bool R_USBH_GetPipeInterrupt(PUSB  pUSB,
                              int   iPipeNumber,
                              USBIP buffIntType,
                              _Bool bfQualify)
{
    if (iPipeNumber >= 0)
    {
        uint16_t    wIMask = (uint16_t)(1 << iPipeNumber);
        if (bfQualify)
        {
            switch (buffIntType)
            {
                case USBH_PIPE_BUFFER_EMPTY:
                if (wIMask & pUSB->BEMPSTS.WORD & pUSB->BEMPENB.WORD)
                {
                    return true;
                }
                break;
                case USBH_PIPE_BUFFER_READY:
                if (wIMask & pUSB->BRDYSTS.WORD & pUSB->BRDYENB.WORD)
                {
                    return true;
                }
                break;
                case USBH_PIPE_BUFFER_NOT_READY:
                if (wIMask & pUSB->NRDYSTS.WORD & pUSB->NRDYENB.WORD)
                {
                    return true;
                }
                break;
                default:

                break;
            }
            return false;
        }
        else
        {
            switch (buffIntType)
            {
                case USBH_PIPE_BUFFER_EMPTY:
                if (wIMask & pUSB->BEMPSTS.WORD)
                {
                    return true;
                }
                break;
                case USBH_PIPE_BUFFER_READY:
                if (wIMask & pUSB->BRDYSTS.WORD)
                {
                    return true;
                }
                break;
                case USBH_PIPE_BUFFER_NOT_READY:
                if (wIMask & pUSB->NRDYSTS.WORD)
                {
                    return true;
                }
                break;
                default:

                    break;
            }
            return false;
        }
    }
    else
    {
        if (bfQualify)
        {
            switch (buffIntType)
            {
                case USBH_PIPE_BUFFER_EMPTY:
                if (pUSB->BEMPSTS.WORD & pUSB->BEMPENB.WORD)
                {
                    return true;
                }
                break;
                case USBH_PIPE_BUFFER_READY:
                if (pUSB->BRDYSTS.WORD & pUSB->BRDYENB.WORD)
                {
                    return true;
                }
                break;
                case USBH_PIPE_BUFFER_NOT_READY:
                if (pUSB->NRDYSTS.WORD & pUSB->NRDYENB.WORD)
                {
                    return true;
                }
                break;
                default:

                    break;
            }
            return false;
        }
        else
        {
            switch (buffIntType)
            {
                case USBH_PIPE_BUFFER_EMPTY:
                if (pUSB->BEMPSTS.WORD)
                {
                    return true;
                }
                break;
                case USBH_PIPE_BUFFER_READY:
                if (pUSB->BRDYSTS.WORD)
                {
                    return true;
                }
                break;
                case USBH_PIPE_BUFFER_NOT_READY:
                if (pUSB->NRDYSTS.WORD)
                {
                    return true;
                }
                break;
                default:

                    break;
            }
            return false;
        }
    }
}
/******************************************************************************
End of function  R_USBH_GetPipeInterrupt
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_InterruptStatus
Description:   Function to get the interrupt status
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  intStatus - The interrupt status to get
               IN  bfAck - true to acknowledge the status
Return value:  true if the flag is set.
******************************************************************************/
_Bool R_USBH_InterruptStatus(PUSB  pUSB,
                             USBIS intStatus,
                             _Bool bfAck)
{
    switch (intStatus)
    {
        case USBH_INTSTS_SOFR:
        {
            if (pUSB->INTSTS0.WORD & USB_INTSTS0_SOFR)
            {
                if (bfAck)
                {
                    pUSB->INTSTS0.WORD = ~USB_INTSTS0_SOFR;
                }
                return true;
            }
            break;
        }
        case USBH_INTSTS_PIPE_BUFFER_EMPTY:
        {
            if (pUSB->INTSTS0.WORD & USB_INTSTS0_BEMP)
            {
                return true;
            }
            break;
        }
        case USBH_INTSTS_PIPE_BUFFER_READY:
        {
            if (pUSB->INTSTS0.WORD & USB_INTSTS0_BRDY)
            {
                return true;
            }
            break;
        }
        case USBH_INTSTS_PIPE_BUFFER_NOT_READY:
        {
            if (pUSB->INTSTS0.WORD & USB_INTSTS0_NRDY)
            {
                return true;
            }
            break;
        }
        case USBH_INTSTS_SETUP_ERROR:
        {
            if (pUSB->INTSTS1.WORD & USB_INTSTS1_SIGN)
            {
                if (bfAck)
                {
                    pUSB->INTSTS1.WORD = ~USB_INTSTS1_SIGN;
                }
                return true;
            }
            break;
        }
        case USBH_INTSTS_SETUP_COMPLETE:
        {
            if (pUSB->INTSTS1.WORD & USB_INTSTS1_SACK)
            {
                if (bfAck)
                {
                    pUSB->INTSTS1.WORD = ~USB_INTSTS1_SACK;
                }
                return true;
            }
            break;
        }
    }
    return  false;
}
/******************************************************************************
End of function  R_USBH_InterruptStatus
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_GetFrameNumber
Description:   Function to get the frame and micro frame numbers
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               OUT pwFrame - Pointer to the destinaton frame number
               OUT pwMicroFrame - Pointer to the destination micro frame number
Return value:  none
******************************************************************************/
void R_USBH_GetFrameNumber(PUSB     pUSB,
                           uint16_t *pwFrame,
                           uint16_t *pwMicroFrame)
{
    if (pwFrame)
    {
        *pwFrame = (uint16_t)pUSB->FRMNUM.BIT.FRNM;
    }
    if (pwMicroFrame)
    {
        #if USBH_HIGH_SPEED_SUPPORT == 1
        *pwMicroFrame = (uint16_t)pUSB->UFRMNUM.BIT.UFRNM;
        #else
        *pwMicroFrame = 0;
        #endif
    }
}
/******************************************************************************
End of function  R_USBH_GetFrameNumber
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_WritePipe
Description:   Function to write to an endpoint pipe FIFO
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The pipe number to write to
               IN  pbySrc - Pointer to the source memory (FIFO size alligned)
               IN  stLength - The length to write
Return value:  The number of bytes written or -1 on FIFO access error
******************************************************************************/
size_t R_USBH_WritePipe(PUSB       pUSB,
                        int        iPipeNumber,
                        uint8_t    *pbySrc,
                        size_t     stLength)
{
    size_t      stMaxPacket;
    uint16_t    usCFIFOSEL;
    int         iTrys = 2;
    /* If we are not writing to the DCP */
    if (iPipeNumber > 0)
    {
        /* Select the endpoint FIFO for desired access width */
        usCFIFOSEL = (uint16_t)(
                   #if USBH_FIFO_BIT_WIDTH == 32
                     USB_CFIFOSEL_MBW(2)
                   #else
                     USB_CFIFOSEL_MBW(1)
                   #endif
                   #ifdef _BIG
                   | USB_CFIFOSEL_BIG_END
                   #endif
                   | USB_CFIFOSEL_CURPIPE(iPipeNumber));
        USB_CFIFOSEL(pUSB, usCFIFOSEL);
    }
    else
    {
        usCFIFOSEL = (uint16_t)(
                   #if USBH_FIFO_BIT_WIDTH == 32
                     (USB_CFIFOSEL_MBW(2)
                   #else
                     (USB_CFIFOSEL_MBW(1)
                   #endif
                   #ifdef _BIG
                   | USB_CFIFOSEL_BIG_END
                   #endif
                   | USB_CFIFOSEL_ISEL));
        /* For control pipe clear the buffer - this does both SIE and CPU
           side */
        do
        {
            pUSB->DCPCTR.BIT.PID = 0;
            pUSB->CFIFOCTR.BIT.BCLR = 1;
        } while (pUSB->DCPCTR.BIT.PBUSY);

        USB_CFIFOSEL(pUSB, usCFIFOSEL);
    }
    /* Wait for access to the FIFO */
    while (r_usbh_wait_fifo(pUSB, iPipeNumber, 50))
    {
        /* Give it two attempts */
        iTrys--;
        if (!iTrys)
        {
            /* Module would not release FIFO */
            ASSERT(0);
            return -1UL;
        }
        else
        {
            USB_CFIFOSEL(pUSB, usCFIFOSEL);
        }
    }
    /* Check length */
    if (stLength)
    {
        USB_PIPESEL(pUSB, iPipeNumber);
        /* Get the maximum packet size */
        if (iPipeNumber)
        {
            stMaxPacket = pUSB->PIPEMAXP.BIT.MXPS;
        }
        else
        {
            stMaxPacket = pUSB->DCPMAXP.BIT.MXPS;
        }
        /* Write at maximum one packet */
        if (stLength > stMaxPacket)
        {
            stLength = stMaxPacket;
        }
        /* Write the data into the FIFO */
        r_usbh_write_fifo(pUSB, pbySrc, stLength);
        /* If it is a short packet then the buffer must be validated */
        if (stLength < stMaxPacket)
        {
            pUSB->CFIFOCTR.WORD |= USB_CFIFOCTR_BVAL;
        }
    }
    else
    {
        /* Send zero length packet */
        pUSB->CFIFOCTR.WORD = (USB_CFIFOCTR_BVAL | USB_CFIFOCTR_BCLR);
    }
    return stLength;
}
/******************************************************************************
End of function  R_USBH_WritePipe
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_DmaWritePipe
Description:   Function to configure a pipe for the DMA to write to it
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The pipe to configure
               IN  wNumPackets - The number of packets to be written
Return value:  0 for success -1 on error
******************************************************************************/
int R_USBH_DmaWritePipe(PUSB pUSB, int iPipeNumber, uint16_t wNumPackets)
{
    if ((iPipeNumber > 0)
    &&  (iPipeNumber <= USBH_NUM_DMA_ENABLED_PIPES))
    {
        uint16_t    usDFIFOSEL;
        /* Clear the transaction counter */
        USB_PIPETRE(pUSB, iPipeNumber)->BIT.TRCLR = 1;
        USB_PIPETRE(pUSB, iPipeNumber)->BIT.TRCLR = 0;
        /* Set the transaction count */
        *USB_PIPETRN(pUSB, iPipeNumber) = wNumPackets;
        /* Set the DFIFOSEL register */
        usDFIFOSEL = (uint16_t)(
                   #if USBH_FIFO_BIT_WIDTH == 32
                     USB_D0FIFOSEL_MBW(2)
                   #else
                     USB_D0FIFOSEL_MBW(1)
                   #endif
                   #ifdef _BIG
                   | USB_D0FIFOSEL_BIG_END
                   #endif
                   | USB_D0FIFOSEL_CURPIPE(iPipeNumber));
        USB_D0FIFOSEL(pUSB, usDFIFOSEL);
        /* Enable DMA on this FIFO */
        pUSB->D0FIFOSEL.WORD |= USB_D0FIFOSEL_DREQE;
        /* This has only been seen in the SH7205 variant with two root ports */
        #if USBH_NUM_ROOT_PORTS == 2
        /* Enable Transfer End Sampling - specific setting to dual core DMA */
        pUSB->D0FBCFG.WORD = BIT_4;
        #endif
        /* Enable the transaction counter */
        USB_PIPETRE(pUSB, iPipeNumber)->BIT.TRENB = 1;
        return 0;
    }
    return -1;
}
/******************************************************************************
End of function  R_USBH_DmaWritePipe
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_StopDmaWritePipe
Description:   Function to unconfigure a pipe from a DMA write
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
Return value:  none
******************************************************************************/
void R_USBH_StopDmaWritePipe(PUSB pUSB)
{
    #if USBH_FIFO_BIT_WIDTH == 32
    USB_D0FIFOSEL(pUSB, USB_CFIFOSEL_MBW(2));
    #else
    USB_D0FIFOSEL(pUSB, USB_CFIFOSEL_MBW(1));
    #endif
}
/******************************************************************************
End of function  R_USBH_StopDmaWritePipe
******************************************************************************/

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
size_t R_USBH_ReadPipe(PUSB     pUSB,
                       int      iPipeNumber,
                       uint8_t  *pbyDest,
                       size_t   stLength)
{
    size_t      stAvailable;
    int         iReadFIFOStuck = 100;
    uint16_t    usCFIFOSEL = (uint16_t)(
                           #if USBH_FIFO_BIT_WIDTH == 32
                             USB_CFIFOSEL_MBW(2)
                           #else
                             USB_CFIFOSEL_MBW(1)
                           #endif
                           #ifdef _BIG
                           | USB_CFIFOSEL_BIG_END
                           #endif
                           | USB_CFIFOSEL_CURPIPE(iPipeNumber));
    /* Make sure that the pipe is not busy */
    while (USB_PIPECTR(pUSB, iPipeNumber)->BIT.PBUSY)
    {
        /* Wait for the pipe to stop action */
    }
    /* Select the FIFO */
    USB_CFIFOSEL(pUSB, usCFIFOSEL);
    /* Wait for access to FIFO */
    while ((pUSB->CFIFOSEL.BIT.CURPIPE != iPipeNumber)
    ||     ((pUSB->CFIFOCTR.WORD & USB_CFIFOCTR_FRDY) == 0))
    {
        USB_CFIFOSEL(pUSB, usCFIFOSEL);
        /* Prevent from getting stuck in this loop */
        iReadFIFOStuck--;
        if (!iReadFIFOStuck)
        {
            ASSERT(iReadFIFOStuck);
            /* Clear the pipe */
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;
            pUSB->CFIFOCTR.BIT.BCLR = 1;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 1;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 0;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 1;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 0;
            return -1UL;
        }
    }
    USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;
    /* Get the length of data available to read */
    stAvailable = pUSB->CFIFOCTR.BIT.DTLN;
    if (stAvailable)
    {
        /* Only read the quantity of data in the FIFO */
        if (stLength >= stAvailable)
        {
            /* Read the data from the FIFO */
			if(	stLength > 4)
			{
// rcsays
//				sprintf(g_data_str, "R_USBH_ReadPipe: calling r_usbh_read_fifo (pbyDest 0x%08x, stLength = %d)\r\n",pbyDest, stLength);
//				sprintf(g_data_str, "%d\r\n", stLength);
				TRACE((g_data_str));
			}
            stLength = stAvailable;
            r_usbh_read_fifo(pUSB, pbyDest, stLength);
        }
        else
        {
            /* There is more data in the FIFO than available!
               This is an overrun error */
            return -2UL;
        }
    }
    else
    {
        /* We have read a zero length packet & need to acknowledge it */
        pUSB->CFIFOCTR.BIT.BCLR = 1;
        stLength = 0UL;
    }
    return stLength;
}
/******************************************************************************
End of function  R_USBH_ReadPipe
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_DataInFIFO
Description:   Function to check the FIFO for more received data
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The pipe number to check
Return value:  The number of bytes in the FIFO or NULL on error
******************************************************************************/
int R_USBH_DataInFIFO(PUSB pUSB, int iPipeNumber)
{
    int         iReadFIFOStuck = 100;
    uint16_t    usCFIFOSEL = (uint16_t)(
                           #if USBH_FIFO_BIT_WIDTH == 32
                             USB_CFIFOSEL_MBW(2)
                           #else
                             USB_CFIFOSEL_MBW(1)
                           #endif
                           #ifdef _BIG
                           | USB_CFIFOSEL_BIG_END
                           #endif
                           | USB_CFIFOSEL_CURPIPE(iPipeNumber));
    /* Stop any further transactions */
    USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;
    /* Make sure that the pipe is not busy */
    while (USB_PIPECTR(pUSB, iPipeNumber)->BIT.PBUSY)
    {
        /* Wait for the pipe to stop action */
    }
    /* Select the FIFO */
    USB_CFIFOSEL(pUSB, usCFIFOSEL);
    /* Wait for access to FIFO */
    while ((pUSB->CFIFOSEL.BIT.CURPIPE != iPipeNumber)
    ||     ((pUSB->CFIFOCTR.WORD & USB_CFIFOCTR_FRDY) == 0))
    {
        /* Prevent from getting stuck in this loop */
        iReadFIFOStuck--;
        if (!iReadFIFOStuck)
        {
            return -1;
        }
    }
    return (int)pUSB->CFIFOCTR.BIT.DTLN;
}
/******************************************************************************
End of function  R_USBH_DataInFIFO
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_DmaReadPipe
Description:   Function to configure a pipe for the DMA to read from it
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The pipe to configure
               IN  wNumPackets - The number of packets to be read
Return value:  0 for success -1 on error
******************************************************************************/
int R_USBH_DmaReadPipe(PUSB pUSB, int iPipeNumber, uint16_t wNumPackets)
{
    /* DMA only available on pipes 1, 2, 3, 4 and 5 - Pipes 6 - 9 the DMA can 
       only fill one packet (Interrupt transfers) and another function 
       should be written to handle DMA transfers on these pipes */
    if ((iPipeNumber > 0)
    &&  (iPipeNumber < 6))
    {
        uint16_t    usDFIFOSEL;
        /* Select the pipe */
        USB_PIPESEL(pUSB, iPipeNumber);
        /* Enable double buffering for DMA */
        #if USBH_HIGH_SPEED_SUPPORT == 1
        /*
            ** POSSIBLE BUG IN FULL SPEED VARIANT OF 597 IP **
            There is a possible bug in the Full speed variant of the 597 IP.
            When double buffering is enabled and a data of length equal to the
            value set in the PIPEMAXP MXPS field is read from the FIFO
            the same data is read twice . Please check errata's and ECN &
            remove conditional compile when fixed.
        */
        pUSB->PIPECFG.BIT.DBLB = 1;
        #endif
        /* Clear the transaction counter */
        USB_PIPETRE(pUSB, iPipeNumber)->BIT.TRCLR = 1;
        USB_PIPETRE(pUSB, iPipeNumber)->BIT.TRCLR = 0;
        /* Set the transaction count */
        *USB_PIPETRN(pUSB, iPipeNumber) = wNumPackets;
        /* Set the DFIFOSEL register */
        usDFIFOSEL = (uint16_t)(
                   #if USBH_FIFO_BIT_WIDTH == 32
                     USB_D0FIFOSEL_MBW(2)
                   #else
                     USB_D0FIFOSEL_MBW(1)
                   #endif
                   #ifdef _BIG
                   | USB_D0FIFOSEL_BIG_END
                   #endif
                   | USB_D0FIFOSEL_CURPIPE(iPipeNumber));
        USB_D1FIFOSEL(pUSB, usDFIFOSEL);
        /* Enable DMA on this FIFO */
        pUSB->D1FIFOSEL.WORD |= USB_D1FIFOSEL_DREQE;
        #if USBH_HIGH_SPEED_SUPPORT == 1
        /* Enable Transfer End Sampling */
        pUSB->D1FBCFG.WORD = BIT_4;
        #endif
        /* Enable the transaction counter */
        USB_PIPETRE(pUSB, iPipeNumber)->BIT.TRENB = 1;
        return 0;
    }
    return -1;
}
/******************************************************************************
End of function  R_USBH_DmaReadPipe
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_StopDmaReadPipe
Description:   Function to unconfigure a pipe from a DMA read
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
Return value:  none
******************************************************************************/
void R_USBH_StopDmaReadPipe(PUSB pUSB)
{
    #if USBH_FIFO_BIT_WIDTH == 32
    USB_D1FIFOSEL(pUSB, USB_CFIFOSEL_MBW(2));
    #else
    USB_D1FIFOSEL(pUSB, USB_CFIFOSEL_MBW(1));
    #endif
}
/******************************************************************************
End of function  R_USBH_StopDmaReadPipe
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_DmaFIFO
Description:   Function to get a pointer to the DMA FIFO
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  transferDirection - The direction of transfer
Return value:  Pointer to the DMA FIFO
******************************************************************************/
void *R_USBH_DmaFIFO(PUSB pUSB, USBDIR transferDirection)
{
    if (transferDirection)
    {
        return (void*)&pUSB->D1FIFO;
    }
    else
    {
        return (void*)&pUSB->D0FIFO;
    }
}
/******************************************************************************
End of function  R_USBH_DmaFIFO
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_DmaTransac
Description:   Function to get the value of the transaction counter
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
Return value:  The value of the transaction counter
******************************************************************************/
uint16_t R_USBH_DmaTransac(PUSB pUSB, int iPipeNumber)
{
    uint16_t    usTransac = 0;
    if ((iPipeNumber > 0)
    &&  (iPipeNumber <= USBH_NUM_DMA_ENABLED_PIPES))
    {
        usTransac = *USB_PIPETRN(pUSB, iPipeNumber);
    }
    return usTransac;
}
/******************************************************************************
End of function  R_USBH_DmaTransac
******************************************************************************/

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
_Bool R_USBH_SetDevAddrCfg(PUSB     pUSB,
                           uint8_t  byDeviceAddress,
                           uint8_t  *pbyHubAddress,
                           uint8_t  byHubPort,
                           uint8_t  byRootPort,
                           USBTS    transferSpeed)
{
    if (byDeviceAddress <= USBH_MAX_NUM_PIPES)
    {
        /* Get a pointer to the appropriate register */
        volatile PDEVADDR pDEVADDR = USB_DEVADD(pUSB, byDeviceAddress);
        /* Set the device transfer speed */
        switch (transferSpeed)
        {
            default:
            case USBH_FULL:
            pDEVADDR->BIT.USBSPD = 2;
            break;
            case USBH_SLOW:
            pDEVADDR->BIT.USBSPD = 1;
            break;
            case USBH_HIGH:
            pDEVADDR->BIT.USBSPD = 3;
            break;
        }
        /* If the device is connected to a hub */
        if (pbyHubAddress)
        {
            /* Set the address of the hub */
            pDEVADDR->BIT.UPPHUB = *pbyHubAddress;
            /* Set the index of the port on the hub */
            pDEVADDR->BIT.HUBPORT = byHubPort;
            /* Set the index of the root port - hubs are only supported on the root ports */
            pDEVADDR->BIT.RTPOR = byRootPort;
        }
        else
        {
            /* Set to 0 when on a root port */
            pDEVADDR->BIT.UPPHUB = 0;
            pDEVADDR->BIT.HUBPORT = 0;
            pDEVADDR->BIT.RTPOR = byRootPort;
        }
        return true;
    }
    return false;
}
/******************************************************************************
End of function  R_USBH_SetDevAddrCfg
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_PipeReady
Description:   Function to check to see if the pipe is ready
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The number of the pipe
Return value:  true if the pipe is ready for use
******************************************************************************/
_Bool R_USBH_PipeReady(PUSB pUSB, int iPipeNumber)
{
    if (iPipeNumber > 0)
    {
        if (USB_PIPECTR(pUSB, iPipeNumber)->BIT.PBUSY)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else if (iPipeNumber == 0)
    {
        if (pUSB->DCPCTR.BIT.PBUSY)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}
/******************************************************************************
End of function  R_USBH_PipeReady
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_ClearSetupRequest
Description:   Function to clear the setup request
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
Return value:  none
******************************************************************************/
void R_USBH_ClearSetupRequest(PUSB pUSB)
{
    pUSB->DCPCTR.BIT.SUREQCLR = 1;
#if USBH_HIGH_SPEED_SUPPORT == 1
    pUSB->DCPCTR.BIT.PINGE = 1;
#endif
}
/******************************************************************************
End of function  R_USBH_ClearSetupRequest
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_EnableSetupInterrupts
Description:   Function to clear the setup request
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  bfEnable - true to enable false to disable
Return value:  none
******************************************************************************/
void R_USBH_EnableSetupInterrupts(PUSB pUSB, _Bool bfEnable)
{
    if (bfEnable)
    {
        /* Enable the setup transaction interrupts */
        pUSB->INTENB1.BIT.SIGNE = 1;
        pUSB->INTENB1.BIT.SACKE = 1;
    }
    else
    {
        /* Disable the interrupt requests */
        pUSB->INTENB1.BIT.SIGNE = 0;
        pUSB->INTENB1.BIT.SACKE = 0;
    }
}
/******************************************************************************
End of function  R_USBH_EnableSetupInterrupts
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_SetSOF_Speed
Description:   Function to set the SOF speed to match the device speed
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  transferSpeed - Select the speed of the device.
Return value:  none
******************************************************************************/
void R_USBH_SetSOF_Speed(PUSB pUSB, USBTS transferSpeed)
{
    /* Set the SOF output for a low speed device */
    if (transferSpeed == USBH_SLOW)
    {
        pUSB->SOFCFG.BIT.TRNENSEL = 1;
    }
    else
    {
        // pUSB->SOFCFG.BIT.TRNENSEL = 0;
        pUSB->SOFCFG.BIT.TRNENSEL = 1;
    }
}
/******************************************************************************
End of function  R_USBH_SetSOF_Speed
******************************************************************************/

/******************************************************************************
Function Name: R_USBH_SetControlPipeDirection
Description:   Function to set the direction of transfer of the control pipe
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
               IN  transferDirection - the direction of transfer
Return value:  none
******************************************************************************/
void R_USBH_SetControlPipeDirection(PUSB pUSB, USBDIR transferDirection)
{
    if (transferDirection)
    {
        pUSB->DCPCFG.BIT.DIR = 0;
    }
    else
    {
        pUSB->DCPCFG.BIT.DIR = 1;
    }
}
/******************************************************************************
End of function  R_USBH_SetControlPipeDirection
******************************************************************************/

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
void R_USBH_SendSetupPacket(PUSB        pUSB,
                            uint8_t     byAddress,
                            uint16_t    wPacketSize,
                            uint8_t     bmRequestType,
                            uint8_t     bRequest,
                            uint16_t    wValue,
                            uint16_t    wIndex,
                            uint16_t    wLength)
{
    /* Make sure that the pipe PID is set to NAK */
    pUSB->DCPCTR.BIT.PID = 0;
    /* Set the device address */
    pUSB->DCPMAXP.BIT.DEVSEL = byAddress;
    /* Set the endpoint packet size */
    pUSB->DCPMAXP.BIT.MXPS = wPacketSize;
    /* Put the setup data into the registers */
    pUSB->USBREQ.BIT.BMREQUESTTYPE = bmRequestType;
    pUSB->USBREQ.BIT.BREQUEST = bRequest;
    pUSB->USBVAL = wValue;
    pUSB->USBINDX = wIndex;
    pUSB->USBLENG = wLength;
    /* Submit the request */
    pUSB->DCPCTR.BIT.SUREQ = 1;
}
/******************************************************************************
End of function  R_USBH_SendSetupPacket
******************************************************************************/

/******************************************************************************
Private Functions
******************************************************************************/

/******************************************************************************
Function Name: r_usbh_wait_fifo
Description:   Function to wait for the FRDY bit to be set indicating that the
               FIFO is ready for access
Parameters:    IN  pUSB - Pointer to the Host Controller hardware
               IN  iPipeNumber - The pipe number
               IN  iCountOut - The number of times to check the FRDY flag
Return value:  0 for success
******************************************************************************/
static int r_usbh_wait_fifo(PUSB pUSB, int iPipeNumber, int iCountOut)
{
    /* Wait for access to FIFO */
    while ((pUSB->CFIFOCTR.WORD & USB_CFIFOCTR_FRDY) == 0)
    {
        /* Prevent from getting stuck in this loop */
        iCountOut--;
        if (!iCountOut)
        {
            /* Clear the pipe */
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;
            pUSB->CFIFOCTR.BIT.BCLR = 1;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 1;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 0;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 1;
            USB_PIPECTR(pUSB, iPipeNumber)->BIT.ACLRM = 0;
            return -1;
        }
    }
    return 0;
}
/******************************************************************************
End of function  r_usbh_wait_fifo
******************************************************************************/

/******************************************************************************
Function Name: r_usbh_config_fifo
Description:   Function to configure the FIFO buffer allocation
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
Return value:  none
******************************************************************************/
#if USBH_HIGH_SPEED_SUPPORT == 1
static void r_usbh_config_fifo(PUSB pUSB)
{
    int    iEndpoint;
    for (iEndpoint = 0; iEndpoint < (int)USB_FIFO_CONFIG_SIZE; iEndpoint++)
    {
        /* Select the pipe */
        USB_PIPESEL(pUSB, iEndpoint + 1);
        /* Set the buffer parameters */
        pUSB->PIPEBUF.WORD = gcFifoConfig[iEndpoint].wPipeBuf;
    }
}
#endif
/******************************************************************************
End of function  r_usbh_config_fifo
******************************************************************************/

/******************************************************************************
Function Name: r_usbh_init_pipes
Description:   Function to initialise the pipe assignment data
Arguments:     IN  pUSB - Pointer to the Host Controller hardware
Return value:  none
******************************************************************************/
static void r_usbh_init_pipes(PUSB pUSB)
{
    int iPipeNumber;
    /* Disable the control pipe */
    pUSB->DCPCTR.BIT.PID = 0;
    /* Disable the remainder of the pipes */
    for (iPipeNumber = 1; iPipeNumber < USBH_MAX_NUM_PIPES; iPipeNumber++)
    {
        /* Disable the pipe */
        USB_PIPECTR(pUSB, iPipeNumber)->BIT.PID = 0;
    }
}
/******************************************************************************
End of function  r_usbh_init_pipes
******************************************************************************/

/******************************************************************************
Function Name: r_usbh_write_fifo
Description:   Function to write to a FIFO
Parameters:    IN  pUSB - Pointer to the Host Controller hardware
               IN  pvSrc - Pointer to the source memory
               IN  stLength - The number of bytes to write
Return value:  none
******************************************************************************/
static void r_usbh_write_fifo(PUSB pUSB, void *pvSrc, size_t stLength)
{
    size_t      stBytes;
#if USBH_FIFO_BIT_WIDTH == 32
    size_t      stDwords;
    /* Peripheral with 32 bit access to FIFO */
    uint32_t    *pulSrc = pvSrc;
    if ((size_t)pulSrc & 3UL)
    {
        TRACE(("r_usbh_write_fifo: invalid pointer alignment\r\n"));
        return;
    }
    /* Calculate the number of DWORDS */
    stDwords = stLength >> 2;
    /* Calculate the number of BYTES */
    stBytes = stLength - (stDwords << 2);
    /* Write the DWORDs */
    while (stDwords--)
    {
        pUSB->CFIFO.LONG = *pulSrc++;
    }
    /* If there are bytes */
    if (stBytes)
    {
        uint8_t     *pbySrc = (uint8_t*)pulSrc;
        uint8_t     *pbyDest = (uint8_t*)&pUSB->CFIFO;
        /* Select byte access */
        pUSB->CFIFOSEL.WORD &= ~(BIT_11 | BIT_10);
        /* Write the bytes */
        while (stBytes--)
        {
            *pbyDest = *pbySrc++;
        }
    }
#else
    size_t      stWords;
#if defined(USE_ORIGNAL)
    /* Peripheral with 16 bit access to FIFO */
    uint16_t    *pusSrc = pvSrc;
    if ((size_t)pusSrc & 1UL)
    {
        TRACE(("r_usbh_write_fifo: invalid pointer alignment\r\n"));
        return;
    }
    /* Calculate the number of WORDS */
    stWords = stLength >> 1;
    /* Calculate the number of BYTES */
    stBytes = stLength - (stWords << 1);
    /* Write the words */
    while (stWords--)
    {
        //pUSB->CFIFO.WORD = *pusSrc++;
        pUSB->CFIFO = *pusSrc++;
    }
    /* If there are bytes */
    if (stBytes)
    {
        uint8_t *pbySrc = (uint8_t*)pusSrc;
        uint8_t *pbyDest = (uint8_t*)&pUSB->CFIFO;
        /* Select byte access */
        pUSB->CFIFOSEL.WORD &= ~(BIT_10);
        /* Write the bytes */
        while (stBytes--)
        {
            *pbyDest = *pbySrc++;
        }
    }
#else //!defined(USE_ORIGINAL)
    /* Peripheral with 16 bit access to FIFO */
        uint16_t    *pusSrc = pvSrc;
        /* Calculate the number of WORDS */
        stWords = stLength >> 1;
        /* Calculate the number of BYTES */
        stBytes = stLength - (stWords << 1);
        /* Write the words */
        while (stWords--)
        {
            //pUSB->CFIFO.WORD = *pusSrc++;
            pUSB->CFIFO = *pusSrc++;
        }
        /* If there are bytes */
        if (stBytes)
        {
            uint8_t *pbySrc = (uint8_t*)pusSrc;
            uint8_t *pbyDest = (uint8_t*)&pUSB->CFIFO;
            /* Select byte access */
            pUSB->CFIFOSEL.WORD &= ~(BIT_10);
            /* Write the bytes */
            while (stBytes--)
            {
                *pbyDest = *pbySrc++;
            }
        }

#endif //defined(USE_ORIGINAL)

#endif
}
/******************************************************************************
End of function  r_usbh_write_fifo
******************************************************************************/

/******************************************************************************
Function Name: r_usbh_read_fifo
Description:   Function to read from a FIFO
Parameters:    IN  pUSB - Pointer to the Host Controller hardware
               OUT pvDest - Pointer to the destination memory
               IN  stLength - The number of bytes to read
Return value:  none
******************************************************************************/
static void r_usbh_read_fifo(PUSB pUSB, void *pvDest, size_t stLength)
{
#if USBH_FIFO_BIT_WIDTH == 32
    uint32_t    *pulDest = pvDest;
    if ((size_t)pulDest & 3UL)
    {
        TRACE(("r_usbh_read_fifo: invalid pointer alignment\r\n"));
        return;
    }
    /* Adjust the length for a number of DWORDS */
    stLength = (stLength + 3) >> 2;
    while (stLength--)
    {
        *pulDest++ = pUSB->CFIFO.LONG;
    }
#else

#if defined(USE_ORIGINAL)
    uint16_t    *pusDest = pvDest;

// rcsays
//	sprintf(g_data_str, "r_usbh_read_fifo: checking pusDest (pvDest 0x%08x)\r\n",pvDest);
//	TRACE((g_data_str));

    //TODO: enable this code. Why does this need data pointer to be on an even boundary?
    if ((size_t)pusDest & 1UL)
    {
        TRACE(("r_usbh_read_fifo: invalid pointer alignment\r\n"));
        return;
    }

    /* Adjust the length for a number of WORDS */
    stLength = (stLength + 1) >> 1;
    while (stLength--)
    {
        //*pusDest++ = pUSB->CFIFO.WORD;
        *pusDest++ = pUSB->CFIFO;
    }
#else   //!defined(USE_ORIGINAL)
    uint16_t    *pusDest = pvDest;
    size_t stWords = 0;
    size_t stBytes = 0;
    /* Calculate the number of WORDS */
    stWords = stLength >> 1;
    /* Calculate the number of BYTES */
    stBytes = stLength - (stWords << 1);

    while (stWords--)
    {
        //*pusDest++ = pUSB->CFIFO.WORD;
        *pusDest++ = pUSB->CFIFO;
    }
    /* If there are bytes */
    if (stBytes)
    {
        uint8_t *pbySrc = (uint8_t*)&pUSB->CFIFO;
        uint8_t *pbyDest = (uint8_t*)pusDest;
        /* Select byte access */
        pUSB->CFIFOSEL.WORD &= ~(BIT_10);
        /* Write the bytes */
        while (stBytes--)
        {
            *pbyDest = *pbySrc++;
        }
    }

#endif  //defined(USE_ORIGINAL)

#endif
}
/******************************************************************************
End of function  r_usbh_read_fifo
******************************************************************************/

/*****************************************************************************
* Function Name: r_usbh_cfiosel
* Description  : Function to select the pipe 
* Arguments    : IN  pUSB - Pointer to the USB hardware
*                IN  usCFIFOSEL - The value for the CFIFOSEL register
* Return Value : none
******************************************************************************/
static void r_usbh_cfiosel(PUSB pUSB, uint16_t usCFIFOSEL)
{
    /* Under some circumstances if the pipe FIFO is assigned to the Serial
       Interface Engine the pipe number will not be set. Prevent from getting
       stuck in this loop */
    int iCount = 10;
    do
    {
        pUSB->CFIFOSEL.WORD = usCFIFOSEL;
        if (iCount)
        {
            iCount--;
        }
        else
        {
            break;
        }
    } while (pUSB->CFIFOSEL.WORD != usCFIFOSEL);
}
/*****************************************************************************
End of function  r_usbh_cfiosel
******************************************************************************/

/******************************************************************************
End  Of File
******************************************************************************/
