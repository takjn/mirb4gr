/* INSERT LICENSE HERE */

#include "Usb.h"

#include <string.h>

#define WAIT_ON_STATUS_STAGE
#define ASSERT(x)   ((void)0)
// DEFINITIONS ****************************************************************/

static void hwResetPort0(_Bool bfState);
static void hwEnablePort0(_Bool bfState);
static void hwSuspendPort0(_Bool bfState);
static uint32_t hwStatusPort0(void);
void hwPowerPort0(_Bool bfState);

static _Bool isRootReset(void);
static void rootReset(_Bool bfState);

// DECLARATIONS ***************************************************************/
/* Flags set in over current interrupt */
static _Bool    gbfOverCurrentPort0 = false;
/* Flags to show device attached status */
static _Bool    gbfAttached0 = false;
/* The data for the host controller driver */
static USBHC    gUsbHc0;
/* Group the port 0 control functions together */
static const USBPC gcRootPort0 =
{
    hwResetPort0,
    hwEnablePort0,
    hwSuspendPort0,
    hwStatusPort0,
    hwPowerPort0,
    0,
    &USB0
};

#ifdef __cplusplus
extern "C" {
#endif
extern USBHI gpUsbHub[USBH_MAX_HUBS];
extern USBPI gpUsbPort[USBH_MAX_PORTS];
extern USBDI gpUsbDevice[USBH_MAX_DEVICES];
extern USBEI gpUsbEndpoint[USBH_MAX_ENDPOINTS];
extern  PUSBDI usbhCreateEnumerationDeviceInformation(USBTS transferSpeed);
#ifdef __cplusplus
}
#endif

/** This module's name. */
const char *USB::MODULE = "USB_HOST";
bool USB::initialized = 0;

static uint8_t usb_error = 0;
static uint32_t usb_task_state = USB_DETACHED_SUBSTATE_INITIALIZE;
// IMPLEMENTATIONS ************************************************************/
enum
{
    /* Note we can use only these Endpoint numbers because arduino expects to complete a request. */
    EP_CONTROL_SETUP = 0,
    EP_CONTROL_IN,
    EP_CONTROL_OUT,
    EP_BULK_IN,
    EP_BULK_OUT,
    EP_ISOCHRONOUS_IN,
    EP_ISOCHRONOUS_OUT,
    EP_INTERRUPT_IN,
    EP_INTERRUPT_OUT,
    EP_LAST
};
/**
 * Default constructor.
 */
USB::USB()
{
    /* Address check */
    assert(&USBC.DPUSR0R.LONG==(void*)0xA0400);
    assert(&USBC.DPUSR1R.LONG==(void*)0xA0404);

    assert(&USB0.SYSCFG.WORD==(void*)0xA0000);
    assert(&USB0.SYSSTS0.WORD==(void*)0xA0004);
    assert(&USB0.DVSTCTR0.WORD==(void*)0xA0008);
    assert(&USB0.CFIFO==(void*)0xA0014);
    assert(&USB0.D0FIFO==(void*)0xA0018);
    assert(&USB0.D1FIFO==(void*)0xA001C);
    assert(&USB0.CFIFOSEL.WORD==(void*)0xA0020);
    assert(&USB0.CFIFOCTR.WORD==(void*)0xA0022);
    assert(&USB0.D0FIFOSEL.WORD==(void*)0xA0028);
    assert(&USB0.D0FIFOCTR.WORD==(void*)0xA002A);
    assert(&USB0.D1FIFOSEL.WORD==(void*)0xA002C);
    assert(&USB0.D1FIFOCTR.WORD==(void*)0xA002E);
    assert(&USB0.INTENB0.WORD==(void*)0xA0030);
    assert(&USB0.INTENB1.WORD==(void*)0xA0032);
    assert(&USB0.BRDYENB.WORD==(void*)0xA0036);
    assert(&USB0.NRDYENB.WORD==(void*)0xA0038);
    assert(&USB0.BEMPENB.WORD==(void*)0xA003A);
    assert(&USB0.SOFCFG.WORD==(void*)0xA003C);
    assert(&USB0.INTSTS0.WORD==(void*)0xA0040);
    assert(&USB0.INTSTS1.WORD==(void*)0xA0042);
    assert(&USB0.BRDYSTS.WORD==(void*)0xA0046);
    assert(&USB0.NRDYSTS.WORD==(void*)0xA0048);
    assert(&USB0.BEMPSTS.WORD==(void*)0xA004A);
    assert(&USB0.FRMNUM.WORD==(void*)0xA004C);
    assert(&USB0.DVCHGR.WORD==(void*)0xA004E);
    assert(&USB0.USBADDR.WORD==(void*)0xA0050);
    assert(&USB0.USBREQ.WORD==(void*)0xA0054);
    assert(&USB0.USBVAL==(void*)0xA0056);
    assert(&USB0.USBINDX==(void*)0xA0058);
    assert(&USB0.USBLENG==(void*)0xA005A);
    assert(&USB0.DCPCFG.WORD==(void*)0xA005C);
    assert(&USB0.DCPMAXP.WORD==(void*)0xA005E);
    assert(&USB0.DCPCTR.WORD==(void*)0xA0060);
    assert(&USB0.PIPESEL.WORD==(void*)0xA0064);
    assert(&USB0.PIPECFG.WORD==(void*)0xA0068);
    assert(&USB0.PIPEMAXP.WORD==(void*)0xA006C);
    assert(&USB0.PIPEPERI.WORD==(void*)0xA006E);
    assert(&USB0.PIPE1CTR.WORD==(void*)0xA0070);
    assert(&USB0.PIPE2CTR.WORD==(void*)0xA0072);
    assert(&USB0.PIPE3CTR.WORD==(void*)0xA0074);
    assert(&USB0.PIPE4CTR.WORD==(void*)0xA0076);
    assert(&USB0.PIPE5CTR.WORD==(void*)0xA0078);
    assert(&USB0.PIPE6CTR.WORD==(void*)0xA007A);
    assert(&USB0.PIPE7CTR.WORD==(void*)0xA007C);
    assert(&USB0.PIPE8CTR.WORD==(void*)0xA007E);
    assert(&USB0.PIPE9CTR.WORD==(void*)0xA0080);
    assert(&USB0.PIPE1TRE.WORD==(void*)0xA0090);
    assert(&USB0.PIPE1TRN==(void*)0xA0092);
    assert(&USB0.PIPE2TRE.WORD==(void*)0xA0094);
    assert(&USB0.PIPE2TRN==(void*)0xA0096);
    assert(&USB0.PIPE3TRE.WORD==(void*)0xA0098);
    assert(&USB0.PIPE3TRN==(void*)0xA009A);
    assert(&USB0.PIPE4TRE.WORD==(void*)0xA009C);
    assert(&USB0.PIPE4TRN==(void*)0xA009E);
    assert(&USB0.PIPE5TRE.WORD==(void*)0xA00A0);
    assert(&USB0.PIPE5TRN==(void*)0xA00A2);
    assert(&USB0.DEVADD0.WORD==(void*)0xA00D0);
    assert(&USB0.DEVADD1.WORD==(void*)0xA00D2);
    assert(&USB0.DEVADD2.WORD==(void*)0xA00D4);
    assert(&USB0.DEVADD3.WORD==(void*)0xA00D6);
    assert(&USB0.DEVADD4.WORD==(void*)0xA00D8);
    assert(&USB0.DEVADD5.WORD==(void*)0xA00DA);
    if(!initialized)
    {
        initialise();
    }
    initialized = true;
}

/**
 * Default destructor.
 */
USB::~USB()
{
#if !defined(TESTING)
    /* TODO: Halt the timer */
    delete enumTimer;
    /* Close the host driver */
    usbhClose(&gUsbHc0);
#endif
    /* Disable the interrupts */
    IPR(USB,USBR0) = 0;
    IPR(USB0,D0FIFO0) = 0;
    IPR(USB0,D1FIFO0) = 0;
    IPR(USB0,USBI0) = 0;
    /* Uninitialize the USB hardware */
    R_USBH_Uninitialise(&USB0);
    /* Power off the ports */
    USB0.DVSTCTR0.BIT.VBUSEN = 0;
}

uint8_t USB::getUsbTaskState(void) {
    return ( usb_task_state);
}

void USB::setUsbTaskState(uint8_t state) {
    usb_task_state = state;
}

void USB::vbusPower(VBUS_t state)
{
    if(state==vbus_on)
    {
        hwPowerPort0(true);
    }
    else if(state == vbus_off)
    {
        hwPowerPort0(false);
    }

}
/**
 * Polls connected USB devices for updates to their status.
 */
#if !defined(TESTING)
void USB::Task()
{
    device::linkDynamicDevices();
}
#else
void USB::Task(void)
{
    uint32_t rcode = 0;
    static uint32_t delay = 0;
    uint32_t lowspeed = 0;

    // Update USB task state on Vbus change
    if(gbfAttached0==true)
    {
        // Attached state
        if ((usb_task_state & USB_STATE_MASK) == USB_STATE_DETACHED)
        {
            delay = millis() + USB_SETTLE_DELAY;
            usb_task_state = USB_ATTACHED_SUBSTATE_SETTLE;
        }
    }
    else
    {
        // Disconnected state
        if ((usb_task_state & USB_STATE_MASK) != USB_STATE_DETACHED)
        {
            usb_task_state = USB_DETACHED_SUBSTATE_INITIALIZE;
            lowspeed = 0;
        }
    }

    // Poll connected devices (if required)
    for (uint32_t i = 0; i < USB_NUMDEVICES; ++i)
        if (devConfig[i])
            rcode = devConfig[i]->Poll();

    // Perform USB enumeration stage and clean up
    switch (usb_task_state)
    {
    case USB_DETACHED_SUBSTATE_INITIALIZE:
        // Init USB stack and driver
        hwPowerPort0(true);

        // Free all USB resources
        for (uint32_t i = 0; i < USB_NUMDEVICES; ++i)
            if (devConfig[i])
                rcode = devConfig[i]->Release();

        usb_task_state = USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE;
        break;

    case USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE:
        // Nothing to do
        break;

    case USB_DETACHED_SUBSTATE_ILLEGAL:
        // Nothing to do
        break;

    case USB_ATTACHED_SUBSTATE_SETTLE:
        // Settle time for just attached device
        if (delay < millis())
        {
            usb_task_state = USB_ATTACHED_SUBSTATE_RESET_DEVICE;
        }
        break;

    case USB_ATTACHED_SUBSTATE_RESET_DEVICE:
        rootReset(true);    // Trigger Bus Reset
        delay = HUB_PORT_RESET_DELAY + millis();
        usb_task_state = USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE;
        break;

    case USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE:
        if (isRootReset() && (delay < millis()) )
        {
            // Clear Bus Reset flag
            rootReset(false);

            //Enable downstream
            hwEnablePort0(true);
#if DISABLED
            // Enable Start Of Frame generation
            if(USB0.INTENB0.BIT.SOFE==0)
                USB0.INTENB0.BIT.SOFE = 1;
#endif
            usb_task_state = USB_ATTACHED_SUBSTATE_WAIT_SOF;

            // Wait 20ms after Bus Reset (USB spec)
            delay = millis() + 20;
        }
        break;

    case USB_ATTACHED_SUBSTATE_WAIT_SOF:
        // Wait for SOF received first
        if (delay < millis())
        {
            // 20ms waiting elapsed
            usb_task_state = USB_STATE_CONFIGURING;
        }
        break;

    case USB_STATE_CONFIGURING:
        rcode = Configuring(0, 0, lowspeed);

        if (rcode)
        {
            if (rcode != USB_DEV_CONFIG_ERROR_DEVICE_INIT_INCOMPLETE)
            {
                usb_error = rcode;
                usb_task_state = USB_STATE_ERROR;
            }
        }
        else
        {
            usb_task_state = USB_STATE_RUNNING;
        }
        break;

    case USB_STATE_RUNNING:
        break;

    case USB_STATE_ERROR:
        break;
    }
}
#endif
/**
 * Initialise the USB class.
 */
extern  void enumRun(void);
void USB::initialise()
{
    /* Configure the USB0 host pins(USB0_OVRCURA and USB0_VBUSEN) needed */
    // protection got turned off in hardware setup
//    MPC.PWPR.BIT.B0WI = 0;          // Enable write to PFSWE bit
//    MPC.PWPR.BIT.PFSWE = 1;         // Disable write protection to PFS registers
    /* Use this for GR-Sakura and RSK */
    PORT1.PDR.BIT.B6 = 1;           // P16 is an output pin
    PORT1.PCR.BIT.B6 = 1;           // P16 is an output pin
    MPC.P16PFS.BIT.PSEL = 0x12;     // Configure as USB0_VBUSEN pin
    PORT1.PMR.BIT.B6 = 1;           // Link the port pin to the peripheral

    /* Use this for GR-Sakura */
    //Added 2015.12.08 Yuuichi Akagawa
    /* Disable USB0_DPUPE */
    MPC.P14PFS.BIT.PSEL = 0;     // Configure as GPIO pin
    PORT1.PMR.BIT.B4 = 0;
    PORT1.PDR.BIT.B4 = 0;
    PORT1.PODR.BIT.B4 = 0;

    /* Short J12 and J14 on the sakura board. Following code will
     * enable the pull down resistor lines */
    PORT2.PDR.BIT.B2 = 1;     //required?(When Short J13 and J15)
    PORT2.PODR.BIT.B2 = 0;    //required?(When Short J13 and J15)

    PORT2.PDR.BIT.B5 = 1;     //required?(When Short J13 and J15)
    PORT2.PODR.BIT.B5 = 0;    //required?(When Short J13 and J15)

    // protection got turned off in hardware setup
//    MPC.PWPR.BIT.PFSWE = 0;         // Enable write protection to PFS registers
//    MPC.PWPR.BIT.B0WI = 0;          // Disable write to PFSWE bit
    /* Disable register protection */
//    SYSTEM.PRCR.WORD = 0xA50B; // protection got turned off in hardware setup
    /* Enable the clock to the USB module */
    MSTP(USB0) = 0;
    USBC.DPUSR0R.BIT.FIXPHY0     = 0;        // USB0  The outputs are fixed in normal mode and on return from deep software standby mode

    /* Enable USB0 */
//    USB0.SYSCFG.WORD = 0x0041;
    USB0.SYSCFG.WORD = 0;  //It temporarily disabled. Enable at R_USBH_Initialise()

    /* Enable register protection */
//    SYSTEM.PRCR.WORD = 0xA500; // protection got turned off in hardware setup
#if defined(TESTING)
    /* Open the host driver */
    usbhOpen(&gUsbHc0);
    /* Add the root ports to the driver */
    usbhAddRootPort(&gUsbHc0, (PUSBPC)&gcRootPort0);
#endif
    /* Initialize the USB hardware */
    R_USBH_Initialise(&USB0);
#if !defined(TESTING)
    /* Start the enumeration timer. (Starts on construction) */
    if(enumTimer==NULL)
    {
        enumTimer = new System_Timer(1,(void(*)(void*))enumRun,NULL,TIMER_RELOAD);
    }
#endif

    //Added 2015.12.08 Yuuichi Akagawa
    /* Clear all USB interrupts */
    USB0.INTSTS0.WORD = 0;
    USB0.INTSTS1.WORD = 0;

    /* Set USB interrupt priority */
    IPR(USB,USBR0) = 14;
    IPR(USB0,D0FIFO0) = 14;
    IPR(USB0,D1FIFO0) = 14;
    IPR(USB0,USBI0) = 14;

    /* Enable the attach interrupts for each root port*/
    IEN(USB0, USBI0) = 1;
    IEN(USB, USBR0) = 1;
    IEN(USB0, D0FIFO0) = 1;
    IEN(USB0, D1FIFO0) = 1;
    USB0.INTENB1.WORD |= BIT_11;
}
/******************************************************************************
Function Name: hwResetPort0
Description:   Function to reset root port 0
Arguments:     IN  bfState - true to reset
Return value:  none
 ******************************************************************************/
static void hwResetPort0(_Bool bfState)
{
    uint16_t wTemp = USB0.DVSTCTR0.WORD;
    if (bfState)
    {
        wTemp &= ~0xF010; // clears 15-12 and UACT
        wTemp |=  0x0040; // sets reset
    }
    else
    {
        wTemp &= ~0xF040; // clears 15-12 and RESET
        wTemp |=  0x0010; // RESUME & UACT
    }
    USB0.DVSTCTR0.WORD = wTemp;
}

static _Bool isRootReset(void)
{
    uint16_t wTemp = USB0.DVSTCTR0.WORD;
    if(wTemp&0x0007)
    {//DVSTCTR0.RHST[2:0] = 0b1xx ~= USB bus reset in progress.
        return true;
    }
    else
        return false;
}

static void rootReset(_Bool bfState)
{
    uint16_t wTemp = USB0.DVSTCTR0.WORD;
    if (bfState)
    {
        wTemp &= ~0xF010; // clears 15-12 and UACT
        wTemp |=  0x0040; // sets reset
    }
    else
    {
        wTemp &= ~0xF040; // clears 15-12 and RESET
        wTemp |=  0x0010; // RESUME & UACT
    }
    USB0.DVSTCTR0.WORD = wTemp;
}
/******************************************************************************
End of function  hwResetPort0
 ******************************************************************************/

/******************************************************************************
Function Name: hwEnablePort0
Description:   Function to enable / disable root port 0
Arguments:     IN  bfState - true to enable
Return value:
 ******************************************************************************/
static void hwEnablePort0(_Bool bfState)
{
    if (bfState)
    {
        /* must set UACT and RESUME Simutaneously */
        USB0.DVSTCTR0.BIT.UACT = 1;
    }
    else
    {
        USB0.DVSTCTR0.BIT.UACT = 0;
    }
}
/******************************************************************************
End of function  hwEnablePort0
 ******************************************************************************/

/******************************************************************************
Function Name: hwSuspendPort0
Description:   Function to suspend root port 0
Arguments:     IN  bfState - true to suspend
Return value:
 ******************************************************************************/
static void hwSuspendPort0(_Bool bfState)
{
    uint16_t wTemp = USB0.DVSTCTR0.WORD;
    if (bfState)
    {
        wTemp &= ~0xF010; // clears 15-12 and UACT
    }
    else
    {
        /* must set UACT and RESUME Simutaneously */
        wTemp &= ~0xF040; // clears 15-12 and RESET
        wTemp |=  0x0030; // RESUME & UACT
    }
}
/******************************************************************************
End of function  hwSuspendPort0
 ******************************************************************************/

/******************************************************************************
Function Name: hwStatusPort0
Description:   Function to get the status of port 0
Arguments:     none
Return value:  The status of the port as defined by
               USBH_HUB_HIGH_SPEED_DEVICE
               USBH_HUB_LOW_SPEED_DEVICE
               USBH_HUB_PORT_POWER
               USBH_HUB_PORT_RESET
               USBH_HUB_PORT_OVER_CURRENT
               USBH_HUB_PORT_SUSPEND
               USBH_HUB_PORT_ENABLED
               USBH_HUB_PORT_CONNECT_STATUS
 ******************************************************************************/
static uint32_t hwStatusPort0(void)
{
    uint32_t   dwPortStatus = 0;
    /* Check the power status */
    if (USB0.DVSTCTR0.BIT.VBUSEN)
    {
        dwPortStatus |= USBH_HUB_PORT_POWER;
    }
    /* Check the reset bits */
    if (USB0.DVSTCTR0.BIT.USBRST)
    {
        dwPortStatus |= USBH_HUB_PORT_RESET;
    }
    /* Set the attached device speed bits */
    switch (USB0.DVSTCTR0.BIT.RHST)
    {
    case 1:
        dwPortStatus |= (USBH_HUB_LOW_SPEED_DEVICE | USBH_HUB_PORT_CONNECT_STATUS);
        break;
    case 2:
        dwPortStatus |= (USBH_HUB_PORT_CONNECT_STATUS);

        break;
    case 3:
        dwPortStatus |= (USBH_HUB_HIGH_SPEED_DEVICE | USBH_HUB_PORT_CONNECT_STATUS);
        break;
    }
    /* Check the enabled state */
    if (USB0.DVSTCTR0.BIT.UACT)
    {
        dwPortStatus |= USBH_HUB_PORT_ENABLED;
    }
    else
    {
        dwPortStatus |= USBH_HUB_PORT_SUSPEND;
    }
    if (gbfAttached0)
    {
        dwPortStatus |= USBH_HUB_PORT_CONNECT_STATUS;
    }
    /* Check the over-current flag */
    if (gbfOverCurrentPort0)
    {
        dwPortStatus |= USBH_HUB_PORT_OVER_CURRENT;
    }
    return dwPortStatus;
}
/******************************************************************************
End of function  hwStatusPort0
 ******************************************************************************/

/******************************************************************************
Function Name: hwPowerPort0
Description:   Function to control the power to port 0
Arguments:     IN  bfState - true to switch the power ON
Return value:  none
 ******************************************************************************/
void hwPowerPort0(_Bool bfState)
{
    /* Clear the over current flag first */
    gbfOverCurrentPort0 = false;
    /* Set the port power */
    USB0.DVSTCTR0.BIT.VBUSEN = bfState;
}
/******************************************************************************
End of function  hwPowerPort0
 ******************************************************************************/

/* Definitions from USB Host shield */
EpInfo* USB::getEpInfoEntry(uint8_t addr, uint8_t ep) {
    UsbDevice *p = addrPool.GetUsbDevicePtr(addr);

    if(!p || !p->epinfo)
        return NULL;

    EpInfo *pep = p->epinfo;

    for(uint8_t i = 0; i < p->epcount; i++) {
        if((pep)->epAddr == ep)
            return pep;

        pep++;
    }
    return NULL;
}

/* set device table entry */
/* each device is different and has different number of endpoints. This function plugs endpoint record structure, defined in application, to devtable */
uint8_t USB::setEpInfoEntry(uint8_t addr, uint8_t epcount, EpInfo* eprecord_ptr) {
    if(!eprecord_ptr)
        return USB_ERROR_INVALID_ARGUMENT;

    UsbDevice *p = addrPool.GetUsbDevicePtr(addr);

    if(!p)
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

    p->address.devAddress = addr;
    p->epinfo = eprecord_ptr;
    p->epcount = epcount;

    return 0;
}
/**
 * \brief Set host pipe target address and set ppep pointer to the endpoint
 * structure matching the specified USB device address and endpoint.
 *
 * \param addr USB device address.
 * \param ep USB device endpoint number.
 * \param ppep Endpoint info structure pointer set by setPipeAddress.
 * \param nak_limit Maximum number of NAK permitted.
 *
 * \retval 0 on success.
 * \retval USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL device not found.
 * \retval USB_ERROR_EPINFO_IS_NULL no endpoint structure found for this device.
 * \retval USB_ERROR_EP_NOT_FOUND_IN_TBL the specified device endpoint cannot be found.
 */
uint8_t USB::SetAddress(uint8_t addr, uint8_t ep, EpInfo **ppep, uint16_t &nak_limit) {
    UsbDevice *p = addrPool.GetUsbDevicePtr(addr);

    if(!p)
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

    if(!p->epinfo)
        return USB_ERROR_EPINFO_IS_NULL;

    *ppep = getEpInfoEntry(addr, ep);

    if(!*ppep)
        return USB_ERROR_EP_NOT_FOUND_IN_TBL;

    nak_limit = (0x0001UL << (((*ppep)->bmNakPower > USB_NAK_MAX_POWER) ? USB_NAK_MAX_POWER : (*ppep)->bmNakPower));
    nak_limit--;


    //RX63N supports Full speed devices only
    USBTS transferSpeed = USBH_FULL; // transferSpeed is unused in GRSAKURA
#if 0//defined(TESTED_AND_OK)
    if(p!=NULL && p->address.bmHub==0)
    {   /* Set the SOF output for a low speed device */
        R_USBH_SetSOF_Speed(&USB0, transferSpeed);
    }

    uint8_t bmParent =  p->address.bmParent;
    if(!R_USBH_SetDevAddrCfg(&USB0, p->address.devAddress, (&bmParent), p->address.bmAddress, p->address.bmHub, transferSpeed))
        return USB_ERROR_SET_DEVICE_ADDRESS_CFG_FAIL;
#endif
    return 0;
}

/* IN transfer to arbitrary endpoint. Assumes PERADDR is set. Handles multiple packets if necessary. Transfers 'nbytes' bytes. */
/* Keep sending INs and writes data to memory area pointed by 'data'                                                           */

/* rcode 0 if no errors. rcode 01-0f is relayed from dispatchPkt(). Rcode f0 means RCVDAVIRQ error,
            fe USB xfer timeout */

USBTR   inTransferRequest;

uint8_t USB::inTransfer(uint8_t addr, uint8_t ep, uint16_t *nbytesptr, uint8_t* data) {
    //TODO: some upper level functions monitor nbytesptr for number of bytes transferred. Refer BTD::ACL_event_task.
    EpInfo *pep = NULL;
    UsbDevice *p = NULL;
    uint32_t nbytes = *nbytesptr;
//    uint32_t idleTime = 1;// idleTime is unused  in GRSAKURA
    uint16_t nak_limit = 0;

    *nbytesptr = 0;

    pep = getEpInfoEntry(addr, ep);
    p = addrPool.GetUsbDevicePtr(addr);

    if(!p)
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

    if(!p->epinfo)
        return USB_ERROR_EPINFO_IS_NULL;

    if(!pep)
        return USB_ERROR_EP_NOT_FOUND_IN_TBL;

    nak_limit = (0x0001UL << (((pep)->bmNakPower > USB_NAK_MAX_POWER) ? USB_NAK_MAX_POWER : (pep)->bmNakPower));

    PUSBDI pDevice = &gpUsbDevice[0];
    PUSBPI pPortInfo = &gpUsbPort[0];
    PUSBEI pInEpInfo = NULL;

    /* Select which address this transfer is directed to */
    pDevice->byAddress = p->address.devAddress;

    /* Select the speed of the transfer */
    if(p->lowspeed)
        pDevice->transferSpeed = USBH_SLOW;
    else
        pDevice->transferSpeed = USBH_FULL;

    /* Set up the port information */
    pPortInfo->pUsbHc = &gUsbHc0;
    pPortInfo->pRoot = (PUSBPC)&gcRootPort0;
    pPortInfo->pDevice = pDevice;
    pPortInfo->pUSB = &USB0;

    /* Setup the Port to use */
    pDevice->pPort = pPortInfo;

    /* Select which USBEI structure we will be using. */
    switch(pep->bmTransferType)
    {
    case USBH_BULK:
        pDevice->pEndpoint = &gpUsbEndpoint[EP_BULK_IN];
        pInEpInfo = pDevice->pEndpoint;
        pInEpInfo->transferType = USBH_BULK;
        break;

    case USBH_CONTROL:
        pDevice->pEndpoint = &gpUsbEndpoint[EP_CONTROL_IN];
        pInEpInfo = pDevice->pEndpoint;
        pInEpInfo->transferType = USBH_CONTROL;
        break;

    case USBH_INTERRUPT:
        pDevice->pEndpoint = &gpUsbEndpoint[EP_INTERRUPT_IN];
        pInEpInfo = pDevice->pEndpoint;
        pInEpInfo->transferType = USBH_INTERRUPT;
        break;

    case USBH_ISOCHRONOUS:
        pDevice->pEndpoint = &gpUsbEndpoint[EP_ISOCHRONOUS_IN];
        pInEpInfo = pDevice->pEndpoint;
        pInEpInfo->transferType = USBH_ISOCHRONOUS;
        break;

    default:
        return 1;
        break;
    }

    pInEpInfo->pDevice = pDevice;
//    pInEpInfo->dataPID = USBH_DATA0;
    pInEpInfo->byEndpointNumber = ep;
    pInEpInfo->transferDirection = USBH_IN;
    pInEpInfo->wPacketSize = pep->maxPktSize;

    /* Set up the signal */
    sysCreateSignal(&inTransferRequest);

    /* Queue the transfer */
    usbhStartTransfer(pDevice, &inTransferRequest, pInEpInfo,   (uint8_t *)data, nbytes, nak_limit);

    /* Wait for the Transfer Request to complete. */
    sysWaitSignal(&inTransferRequest);

    /* Provide the number of bytes transferred. */
    *nbytesptr += inTransferRequest.stIdx;

    /* Send out the error code. Should be USBH_NO_ERROR = 0, most of the time. Interrupt transfers can result in REQ_IDLE_TIME_OUT. */
#if defined(_DEBUG_)
    if(inTransferRequest.errorCode!=USBH_NO_ERROR)
        return (uint8_t)inTransferRequest.errorCode;
    else
        return 0;
#endif
    return (uint8_t)inTransferRequest.errorCode;
}

/* OUT transfer to arbitrary endpoint. Handles multiple packets if necessary. Transfers 'nbytes' bytes. */
/* Handles NAK bug per Maxim Application Note 4000 for single buffer transfer   */

/* rcode 0 if no errors. rcode 01-0f is relayed from HRSL                       */
USBTR outTransferRequest;
uint8_t USB::outTransfer(uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t* data) {
    //TODO: some upper level functions monitor nbytesptr for number of bytes transferred. Refer BTD::ACL_event_task.
        EpInfo *pep = NULL;
        UsbDevice *p = NULL;
        uint32_t idleTime = 1;// idleTime is unused  in GRSAKURA
        uint16_t nak_limit = 0;

        pep = getEpInfoEntry(addr, ep);
        p = addrPool.GetUsbDevicePtr(addr);

        if(!p)
            return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

        if(!p->epinfo)
            return USB_ERROR_EPINFO_IS_NULL;

        if(!pep)
            return USB_ERROR_EP_NOT_FOUND_IN_TBL;

        nak_limit = (0x0001UL << (((pep)->bmNakPower > USB_NAK_MAX_POWER) ? USB_NAK_MAX_POWER : (pep)->bmNakPower));

        PUSBDI pDevice = &gpUsbDevice[0];
        PUSBPI pPortInfo = &gpUsbPort[0];
        PUSBEI pOutEpInfo = NULL;

        /* Select which address this transfer is directed to */
        pDevice->byAddress = p->address.devAddress;

        /* Select the speed of the transfer */
        if(p->lowspeed)
            pDevice->transferSpeed = USBH_SLOW;
        else
            pDevice->transferSpeed = USBH_FULL;

        /* Set up the port information */
        pPortInfo->pUsbHc = &gUsbHc0;
        pPortInfo->pRoot = (PUSBPC)&gcRootPort0;
        pPortInfo->pDevice = pDevice;
        pPortInfo->pUSB = &USB0;

        /* Setup the Port to use */
        pDevice->pPort = pPortInfo;

        /* Select which USBEI structure we will be using. */
        switch(pep->bmTransferType)
        {
        case USBH_BULK:
            pDevice->pEndpoint = &gpUsbEndpoint[EP_BULK_OUT];
            pOutEpInfo = pDevice->pEndpoint;
            pOutEpInfo->transferType = USBH_BULK;
            break;

        case USBH_CONTROL:
            pDevice->pEndpoint = &gpUsbEndpoint[EP_CONTROL_OUT];
            pOutEpInfo = pDevice->pEndpoint;
            pOutEpInfo->transferType = USBH_CONTROL;
            break;

        case USBH_INTERRUPT:
            pDevice->pEndpoint = &gpUsbEndpoint[EP_INTERRUPT_OUT];
            pOutEpInfo = pDevice->pEndpoint;
            pOutEpInfo->transferType = USBH_INTERRUPT;
            break;

        case USBH_ISOCHRONOUS:
            pDevice->pEndpoint = &gpUsbEndpoint[EP_ISOCHRONOUS_OUT];
            pOutEpInfo = pDevice->pEndpoint;
            pOutEpInfo->transferType = USBH_ISOCHRONOUS;
            break;

        default:
            return 1;
            break;
        }

        pOutEpInfo->pDevice = pDevice;
    //    pOutEpInfo->dataPID = USBH_DATA0;
        pOutEpInfo->byEndpointNumber = ep;
        pOutEpInfo->transferDirection = USBH_OUT;
        pOutEpInfo->wPacketSize = pep->maxPktSize;

        /* Set up the signal */
        sysCreateSignal(&outTransferRequest);

        /* Queue the transfer */
        usbhStartTransfer(pDevice, &outTransferRequest, pOutEpInfo,   (uint8_t *)data, nbytes, nak_limit);

        /* Wait for the Transfer Request to complete. */
        sysWaitSignal(&outTransferRequest);

        /* Send out the error code. Should be USBH_NO_ERROR = 0, most of the time. Interrupt transfers can result in REQ_IDLE_TIME_OUT. */
        return (uint8_t)outTransferRequest.errorCode;
}

uint8_t USB::AttemptConfig(uint8_t driver, uint8_t parent, uint8_t port, bool lowspeed) {
    //printf("AttemptConfig: parent = %i, port = %i\r\n", parent, port);
    uint8_t retries = 0;// retries is unused  in GRSAKURA

//    again: // again is unused  in GRSAKURA
    uint8_t rcode = devConfig[driver]->ConfigureDevice(parent, port, lowspeed);
    if(rcode == USB_ERROR_CONFIG_REQUIRES_ADDITIONAL_RESET) {
        if(parent == 0) {
            // Send a bus reset on the root interface.
#if defined(GRSAKURA)
            hwResetPort0(true);     //Apply reset
#else
            regWr(rHCTL, bmBUSRST); //issue bus reset
#endif
            delay(102); // delay 102ms, compensate for clock inaccuracy.
#if defined(GRSAKURA)
            hwResetPort0(false);   //Disengage reset.
#endif
        } else {
            // reset parent port
            devConfig[parent]->ResetHubPort(port);
        }
    }
#if !defined(GRSAKURA)
    else if(rcode == hrJERR && retries < 3) { // Some devices returns this when plugged in - trying to initialize the device again usually works
        delay(100);
        retries++;
        goto again;
    }
#endif
    else if(rcode)
        return rcode;

    rcode = devConfig[driver]->Init(parent, port, lowspeed);
#if !defined(GRSAKURA)
    if(rcode == hrJERR && retries < 3) { // Some devices returns this when plugged in - trying to initialize the device again usually works
        delay(100);
        retries++;
        goto again;
    }
#endif
    if(rcode) {
        // Issue a bus reset, because the device may be in a limbo state
        if(parent == 0) {

#if defined(GRSAKURA)
            hwResetPort0(true);
#else
            // Send a bus reset on the root interface.
            regWr(rHCTL, bmBUSRST); //issue bus reset
#endif
            delay(102); // delay 102ms, compensate for clock inaccuracy.

#if defined(GRSAKURA)
            hwResetPort0(false);
#endif
        } else {
            // reset parent port
            devConfig[parent]->ResetHubPort(port);
        }
    }
    return rcode;
}



uint8_t USB::DefaultAddressing(uint8_t parent, uint8_t port, bool lowspeed) {
    //uint8_t       buf[12];
    uint8_t rcode;
    UsbDevice *p0 = NULL, *p = NULL;

    // Get pointer to pseudo device with address 0 assigned
    p0 = addrPool.GetUsbDevicePtr(0);

    if(!p0)
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

    if(!p0->epinfo)
        return USB_ERROR_EPINFO_IS_NULL;

    p0->lowspeed = (lowspeed) ? true : false;

    // Allocate new address according to device class
    uint8_t bAddress = addrPool.AllocAddress(parent, false, port);

    if(!bAddress)
        return USB_ERROR_OUT_OF_ADDRESS_SPACE_IN_POOL;

    p = addrPool.GetUsbDevicePtr(bAddress);

    if(!p)
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

    p->lowspeed = lowspeed;

    // Assign new address to the device
    rcode = setAddr(0, 0, bAddress);

    if(rcode) {
        addrPool.FreeAddress(bAddress);
        bAddress = 0;
        return rcode;
    }
    return 0;
};

uint8_t USB::ReleaseDevice(uint8_t addr) {
    if(!addr)
        return 0;

    for(uint8_t i = 0; i < USB_NUMDEVICES; i++) {
        if(!devConfig[i]) continue;
        if(devConfig[i]->GetAddress() == addr)
            return devConfig[i]->Release();
    }
    return 0;
}

uint8_t USB::setAddr(uint8_t oldaddr, uint8_t ep, uint8_t newaddr) {
    uint8_t rcode = ctrlReq(oldaddr, ep, bmREQ_SET, USB_REQUEST_SET_ADDRESS, newaddr, 0x00, 0x0000, 0x0000, 0x0000, NULL, NULL);
    //delay(2); //per USB 2.0 sect.9.2.6.3
    delay(300); // Older spec says you should wait at least 200ms
    return rcode;
}

//set configuration
uint8_t USB::setConf(uint8_t addr, uint8_t ep, uint8_t conf_value) {
    return ( ctrlReq(addr, ep, bmREQ_SET, USB_REQUEST_SET_CONFIGURATION, conf_value, 0x00, 0x0000, 0x0000, 0x0000, NULL, NULL));
}

//get device descriptor

uint8_t USB::getDevDescr(uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t* dataptr) {
    return ( ctrlReq(addr, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, 0x00, USB_DESCRIPTOR_DEVICE, 0x0000, nbytes, nbytes, dataptr, NULL));
}
//get configuration descriptor

uint8_t USB::getConfDescr(uint8_t addr, uint8_t ep, uint16_t nbytes, uint8_t conf, uint8_t* dataptr) {
    return ( ctrlReq(addr, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, conf, USB_DESCRIPTOR_CONFIGURATION, 0x0000, nbytes, nbytes, dataptr, NULL));
}

/* Requests Configuration Descriptor. Sends two Get Conf Descr requests. The first one gets the total length of all descriptors, then the second one requests this
 total length. The length of the first request can be shorter ( 4 bytes ), however, there are devices which won't work unless this length is set to 9 */
uint8_t buf[256];
uint8_t USB::getConfDescr(uint8_t addr, uint8_t ep, uint8_t conf, USBReadParser *p) {
    uint8_t bufSize = 255;

    USB_CONFIGURATION_DESCRIPTOR *ucd = reinterpret_cast<USB_CONFIGURATION_DESCRIPTOR *>(buf);

    uint8_t ret = getConfDescr(addr, ep, 9, conf, buf);

    if(ret)
        return ret;

    uint16_t total = ucd->wTotalLength;

    //USBTRACE2("\r\ntotal conf.size:", total);

    return ( ctrlReq(addr, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, conf, USB_DESCRIPTOR_CONFIGURATION, 0x0000, total, bufSize, buf, p));
}

//get string descriptor

uint8_t USB::getStrDescr(uint8_t addr, uint8_t ep, uint16_t ns, uint8_t index, uint16_t langid, uint8_t* dataptr) {
    return ( ctrlReq(addr, ep, bmREQ_GET_DESCR, USB_REQUEST_GET_DESCRIPTOR, index, USB_DESCRIPTOR_STRING, langid, ns, ns, dataptr, NULL));
}


/*
 * This is broken. We need to enumerate differently.
 * It causes major problems with several devices if detected in an unexpected order.
 *
 *
 * Oleg - I wouldn't do anything before the newly connected device is considered sane.
 * i.e.(delays are not indicated for brevity):
 * 1. reset
 * 2. GetDevDescr();
 * 3a. If ACK, continue with allocating address, addressing, etc.
 * 3b. Else reset again, count resets, stop at some number (5?).
 * 4. When max.number of resets is reached, toggle power/fail
 * If desired, this could be modified by performing two resets with GetDevDescr() in the middle - however, from my experience, if a device answers to GDD()
 * it doesn't need to be reset again
 * New steps proposal:
 * 1: get address pool instance. exit on fail
 * 2: pUsb->getDevDescr(0, 0, constBufSize, (uint8_t*)buf). exit on fail.
 * 3: bus reset, 100ms delay
 * 4: set address
 * 5: pUsb->setEpInfoEntry(bAddress, 1, epInfo), exit on fail
 * 6: while (configurations) {
 *              for(each configuration) {
 *                      for (each driver) {
 *                              6a: Ask device if it likes configuration. Returns 0 on OK.
 *                                      If successful, the driver configured device.
 *                                      The driver now owns the endpoints, and takes over managing them.
 *                                      The following will need codes:
 *                                          Everything went well, instance consumed, exit with success.
 *                                          Instance already in use, ignore it, try next driver.
 *                                          Not a supported device, ignore it, try next driver.
 *                                          Not a supported configuration for this device, ignore it, try next driver.
 *                                          Could not configure device, fatal, exit with fail.
 *                      }
 *              }
 *    }
 * 7: for(each driver) {
 *      7a: Ask device if it knows this VID/PID. Acts exactly like 6a, but using VID/PID
 * 8: if we get here, no driver likes the device plugged in, so exit failure.
 *
 */
uint8_t testBuffer[256] = {0xAA,0x55,0x33,0x22};
uint8_t USB::Configuring(uint8_t parent, uint8_t port, bool lowspeed) {
    //uint8_t bAddress = 0;
    //printf("Configuring: parent = %i, port = %i\r\n", parent, port);
    uint8_t devConfigIndex;
    uint8_t rcode = 0;
    uint8_t buf[sizeof (USB_DEVICE_DESCRIPTOR)];
    USB_DEVICE_DESCRIPTOR *udd = reinterpret_cast<USB_DEVICE_DESCRIPTOR *>(buf);
    UsbDevice *p = NULL;
    EpInfo *oldep_ptr = NULL;
    EpInfo epInfo;

    epInfo.epAddr = 0;
    epInfo.maxPktSize = 8;
    epInfo.epAttribs = 0;
    epInfo.bmNakPower = USB_NAK_MAX_POWER;

    //delay(2000);
    AddressPool &addrPool = GetAddressPool();
    // Get pointer to pseudo device with address 0 assigned
    p = addrPool.GetUsbDevicePtr(0);
    if(!p) {
        //printf("Configuring error: USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL\r\n");
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;
    }

    // Save old pointer to EP_RECORD of address 0
    oldep_ptr = p->epinfo;

    // Temporary assign new pointer to epInfo to p->epinfo in order to
    // avoid toggle inconsistence

    p->epinfo = &epInfo;

    p->lowspeed = lowspeed;

    // Clear device descriptor memory
    memset(buf,0,sizeof(buf));

    // Get the first 8 bytes of the descriptor.
    rcode = getDevDescr(0, 0, 8, (uint8_t*)buf);

    // Extract the maxPacket size for the default endpoint
    epInfo.maxPktSize = udd->bMaxPacketSize0;

    // Now we can get the whole descriptor
    rcode = getDevDescr(0, 0, sizeof (USB_DEVICE_DESCRIPTOR), (uint8_t*)buf);

    // Restore p->epinfo
    p->epinfo = oldep_ptr;

    if(rcode) {
        //printf("Configuring error: Can't get USB_DEVICE_DESCRIPTOR\r\n");
        return rcode;
    }

    // to-do?
    // Allocate new address according to device class
    //bAddress = addrPool.AllocAddress(parent, false, port);

    uint16_t vid = udd->idVendor;
    uint16_t pid = udd->idProduct;
    uint8_t klass = udd->bDeviceClass;
    uint8_t subklass = udd->bDeviceSubClass;
    // Attempt to configure if VID/PID or device class matches with a driver
    // Qualify with subclass too.
    //
    // VID/PID & class tests default to false for drivers not yet ported
    // subclass defaults to true, so you don't have to define it if you don't have to.
    //
    for(devConfigIndex = 0; devConfigIndex < USB_NUMDEVICES; devConfigIndex++) {
        if(!devConfig[devConfigIndex]) continue; // no driver
        if(devConfig[devConfigIndex]->GetAddress()) continue; // consumed
        if(devConfig[devConfigIndex]->DEVSUBCLASSOK(subklass) && (devConfig[devConfigIndex]->VIDPIDOK(vid, pid) || devConfig[devConfigIndex]->DEVCLASSOK(klass))) {
            rcode = AttemptConfig(devConfigIndex, parent, port, lowspeed);
            if(rcode != USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED)
                break;
        }
    }

    if(devConfigIndex < USB_NUMDEVICES) {
        return rcode;
    }


    // blindly attempt to configure
    for(devConfigIndex = 0; devConfigIndex < USB_NUMDEVICES; devConfigIndex++) {
        if(!devConfig[devConfigIndex]) continue;
        if(devConfig[devConfigIndex]->GetAddress()) continue; // consumed
        if(devConfig[devConfigIndex]->DEVSUBCLASSOK(subklass) && (devConfig[devConfigIndex]->VIDPIDOK(vid, pid) || devConfig[devConfigIndex]->DEVCLASSOK(klass))) continue; // If this is true it means it must have returned USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED above
        rcode = AttemptConfig(devConfigIndex, parent, port, lowspeed);

        //printf("ERROR ENUMERATING %2.2x\r\n", rcode);
        if(!(rcode == USB_DEV_CONFIG_ERROR_DEVICE_NOT_SUPPORTED || rcode == USB_ERROR_CLASS_INSTANCE_ALREADY_IN_USE)) {
            // in case of an error dev_index should be reset to 0
            //      in order to start from the very beginning the
            //      next time the program gets here
            //if (rcode != USB_DEV_CONFIG_ERROR_DEVICE_INIT_INCOMPLETE)
            //        devConfigIndex = 0;
            return rcode;
        }
    }
    // if we get here that means that the device class is not supported by any of registered classes
    rcode = DefaultAddressing(parent, port, lowspeed);

#if defined(TESTING)
//    delay(5000);
//    rcode = getDevDescr(1, 0, udd->bLength, (uint8_t*)testBuffer);
//    rcode = getConfDescr(1, 0, sizeof(USB_CONFIGURATION_DESCRIPTOR), 1, testBuffer + udd->bLength );
//    USB_CONFIGURATION_DESCRIPTOR *ucd = reinterpret_cast<USB_CONFIGURATION_DESCRIPTOR *>(testBuffer + udd->bLength);
//    rcode = getConfDescr(1, 0, ucd->wTotalLength, 1, testBuffer + udd->bLength );
//    USB_INTERFACE_DESCRIPTOR *uid1 = reinterpret_cast<USB_INTERFACE_DESCRIPTOR *>(testBuffer + udd->bLength + ucd->bLength);
//    USB_INTERFACE_DESCRIPTOR *uid2 = reinterpret_cast<USB_INTERFACE_DESCRIPTOR *>(testBuffer + udd->bLength + ucd->bLength + uid1->bLength);
//    USB_ENDPOINT_DESCRIPTOR *ued = reinterpret_cast<USB_ENDPOINT_DESCRIPTOR *>(testBuffer + udd->bLength + ucd->bLength + uid1->bLength + uid2->bLength);
//    rcode = setConf(1,0,udd->bNumConfigurations);
//    rcode = setAddr(1,0,2);
//    rcode = setConf(2,0,1);
#endif

    return rcode;
}
/**
 * \brief Send a control request.
 * Sets address, endpoint, fills control packet with necessary data, dispatches
 * control packet, and initiates bulk IN transfer depending on request.
 * \note All control requests MUST go to endpoint 0.
 * \param addr USB device address.
 * \param ep USB device endpoint number.
 * \param bmReqType Request direction.
 * \param bRequest Request type.
 * \param wValLo Value low.
 * \param wValHi Value high.
 * \param wInd Index field.
 * \param total Request length.
 * \param nbytes Number of bytes to read.
 * \param dataptr Data pointer.
 * \param p USB class reader.
 *
 * \return 0 on success, error code otherwise.
 * TODO FINISH THIS
 */

#if defined(TESTING)
USBDR   gDeviceRequest;

USBTR   setupRequest;
USBTR   dataRequest;
USBTR   statusRequest;

uint8_t USB::ctrlReq(uint8_t addr, uint8_t ep, uint8_t bmReqType, uint8_t bRequest, uint8_t wValLo, uint8_t wValHi,
                            uint16_t wInd, uint16_t total, uint16_t nbytes, uint8_t* dataptr, USBReadParser *p)
{
    PUSBEI  pStatus = NULL;
    /* If there is a data phase */
    PUSBEI  pData = NULL;

    PUSBEI  pSetup = NULL;

    EpInfo *pep = NULL;
    UsbDevice *pAddr = NULL;

    pep = getEpInfoEntry(addr, ep);
    pAddr = addrPool.GetUsbDevicePtr(addr);

    if(!pAddr)
        return USB_ERROR_ADDRESS_NOT_FOUND_IN_POOL;

    if(!pAddr->epinfo)
        return USB_ERROR_EPINFO_IS_NULL;

    if(!pep)
        return USB_ERROR_EP_NOT_FOUND_IN_TBL;

#if 1
    PUSBDI pDevice = &gpUsbDevice[0];
    PUSBPI pPortInfo = &gpUsbPort[0];

    pDevice->pControlSetup = &gpUsbEndpoint[EP_CONTROL_SETUP];
    pDevice->pControlIn = &gpUsbEndpoint[EP_CONTROL_IN];
    pDevice->pControlOut = &gpUsbEndpoint[EP_CONTROL_OUT];
    pDevice->byAddress = pAddr->address.devAddress;
    /* Select the speed of the transfer */
    if(pAddr->lowspeed)
        pDevice->transferSpeed = USBH_SLOW;
    else
        pDevice->transferSpeed = USBH_FULL;

    pSetup = pDevice->pControlSetup;

#else
    PUSBDI pDevice = usbhCreateEnumerationDeviceInformation(USBH_FULL);
#endif




    pPortInfo->pUsbHc = &gUsbHc0;
    pPortInfo->pRoot = (PUSBPC)&gcRootPort0;
    pPortInfo->pDevice = pDevice;
    pPortInfo->pUSB = &USB0;

    /* Setup the Port to use */
    pDevice->pPort = pPortInfo;

    gDeviceRequest.bmRequestType = bmReqType;
    gDeviceRequest.bRequest = bRequest;
    gDeviceRequest.wValue = ((uint16_t)(wValHi<<8)|(wValLo));
    gDeviceRequest.wIndex = wInd;
    gDeviceRequest.wLength = total;

    /* Re-initialize all of the information for the control packet so that the ISR can handle it correctly */

    /* Create the endpoint information for the setup packet ref: usbhMain.c */
    pSetup->pDevice = pDevice;
    pSetup->byEndpointNumber = ep;
    pSetup->transferDirection = USBH_SETUP;
    pSetup->transferType = USBH_CONTROL;
    pSetup->wPacketSize = pep->maxPktSize;

    /* Setup packets are always have a PID of data 0 */
    pSetup->dataPID = USBH_DATA0;

    /* Set up the signal */
    sysCreateSignal(&setupRequest);

    /* Start the setup packet transfer */
    usbhStartTransfer(pDevice, &setupRequest, pSetup,   (uint8_t *)&gDeviceRequest, 8, 100UL);

    if(total)
    {

        /* Select the endpoint to use for this request */
        if (bmReqType & USB_DEVICE_TO_HOST_MASK)
        {
            pData = pDevice->pControlIn;
            pData->pDevice = pDevice;
            pData->transferType = USBH_CONTROL;
            pData->transferDirection = USBH_IN;
            pData->wPacketSize = pep->maxPktSize;
            pData->byEndpointNumber = ep;

            /* The status phase is always in the opposite direction to the
                               data */
            pStatus = pDevice->pControlOut;
            pStatus->pDevice = pDevice;
            pStatus->transferType = USBH_CONTROL;
            pStatus->transferDirection = USBH_OUT;
            pStatus->wPacketSize = 0;
            pStatus->byEndpointNumber = ep;
        }
        else
        {
            pData = pDevice->pControlOut;
            pData->pDevice = pDevice;
            pData->transferType = USBH_CONTROL;
            pData->transferDirection = USBH_OUT;
            pData->wPacketSize = pep->maxPktSize;
            pData->byEndpointNumber = ep;
            /* The status phase is always in the opposite direction to the
                               data */
            pStatus  = pDevice->pControlIn;
            pStatus->pDevice = pDevice;
            pStatus->transferType = USBH_CONTROL;
            pStatus->transferDirection = USBH_IN;
            pStatus->wPacketSize = 0;
            pStatus->byEndpointNumber = ep;
        }
        /* Data phase always has a PID of data 1 */
                    pData->dataPID = USBH_DATA1;
                    /* Set up the signal */
                    sysCreateSignal(&dataRequest);
                    /* Start the data phase */
                    usbhStartTransfer(pDevice,
                                      &dataRequest,
                                      pData,
                                      dataptr,
                                      (size_t)gDeviceRequest.wLength,
                                      100UL);
                    /* Wait while request hasn't been sent out. */
//                    while(dataRequest.bfInProgress==false && dataRequest.errorCode==USBH_NO_ERROR);
    }
    else
    {
        /* Without a data phase the status phase is IN (Device to host) */
        pStatus = pDevice->pControlIn;
        pStatus->pDevice = pDevice;
        pStatus->transferType = USBH_CONTROL;
        pStatus->transferDirection = USBH_IN;
        pStatus->wPacketSize = 0;
        pStatus->byEndpointNumber = ep;

        dataRequest.errorCode = USBH_NO_ERROR;
    }
    /* Status phase always has a PID of data 1 */
    pStatus->dataPID = USBH_DATA1;

    /* Set up the signal */
    sysCreateSignal(&statusRequest);
    /* Start the status phase */
    usbhStartTransfer(pDevice,
            &statusRequest,
            pStatus,
            NULL,
            0,
            100UL);

    /* Wait for the Transfer Request to complete. */
    sysWaitSignal(&setupRequest);
    /* Wait for the Transfer Request to complete. */
    sysWaitSignal(&dataRequest);
    /* Wait for the Transfer Request to complete. */
    sysWaitSignal(&statusRequest);

    if(p)
    {
        ((USBReadParser*)p)->Parse(total, dataptr, 0);
    }

    if(setupRequest.errorCode!= USBH_NO_ERROR)
        return setupRequest.errorCode;

    if(dataRequest.errorCode!=USBH_NO_ERROR)
        return dataRequest.errorCode;

    return statusRequest.errorCode;

}
#endif//TESTING

/***********************************************************************************************************************/
/*                                                  INTERRUPT SERVICE ROUTINE                                          */
/***********************************************************************************************************************/
#ifdef __cplusplus
extern "C" {
//void INT_Excep_USB0_USBI0(void) __attribute__((interrupt));       //REA: ONR:20150204 Commented this out and added line below.
void InterruptHandler_USBHost(void);
}
#endif

//void INT_Excep_USB0_USBI0(void)       //REA: ONR:20150204 Commented this out. Renamed to below.
void InterruptHandler_USBHost(void)
{
    int     iMask = sysLock(NULL);
    /* Call the driver interrupt handler */
    usbhInterruptHandler(&gUsbHc0);
    /* Check over current condition for port 0 */
    if (USB0.INTSTS1.WORD & BIT_15)
    {
        /* Clear the flag by writing 0 to it */
        USB0.INTSTS1.WORD = (uint16_t)(~BIT_15);
        /* The over current input pin is active LOW */
        if (PORT1.PIDR.BIT.B4==0)
        {
            /* Set the flag to show an over current event has occured */
            gbfOverCurrentPort0 = true;
        }
    }
    /* Check for device attach port 0 */
    if (USB0.INTSTS1.WORD & USB0.INTENB1.WORD & BIT_11)
    {
        /* Clear the flag by writing 0 to it */
        USB0.INTSTS1.WORD = ~(BIT_11 | BIT_12);
        gbfAttached0 = true;
        /* Disable the attach interrupt */
        USB0.INTENB1.WORD &= ~BIT_11;
        /* Enable the detach interrupt and EOFERR */
        USB0.INTENB1.WORD |= (BIT_12 | BIT_6);
    }
    /* Check for device detatch port 0 */
    else if (USB0.INTSTS1.WORD & USB0.INTENB1.WORD & BIT_12)
    {
        /* Clear the flag by writing 0 to it */
        USB0.INTSTS1.WORD = ~(BIT_11 | BIT_12| BIT_6);
        gbfAttached0 = false;
        /* Disable the detach interrupt */
        USB0.INTENB1.WORD &= ~BIT_12;
        /* Enable the attach interrupt */
        USB0.INTENB1.WORD |= BIT_11;
    }
    /* Check the EOF Error Detection Interrupt */
    else if (USB0.INTSTS1.WORD & USB0.INTENB1.WORD & BIT_6)
    {
        /* Clear the flag by writing 0 to it */
        USB0.INTSTS1.WORD = ~(BIT_6);
        /* Make it look like the device has been detatched */
        gbfAttached0 = false;
    }
    sysUnlock(NULL, iMask);
}
