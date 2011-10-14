// board.c: Basic function and variables for interacting with the
// hardware on the Wixel.  Includes LED, power detection, and common
// timing/delay functions.

// TODO: Allow for getting into bootloader mode when the yellow LED is on
//    (need to turn it off for a brief time).
// TODO: only go into bootloader mode if there is USB power
// TODO: add a section of the library for using the watchdog timer
// TODO: let delayMicroseconds take a 16-bit argument
// TODO: WHY does this interrupt only result in a 6 us pulse?
//   ISR(P0INT, 1)
//   {
//     P1_0 ^= 1;
//     delayMicroseconds(40);
//     P1_0 ^= 1;
//   }

#include <cc2511_map.h>
#include <cc2511_types.h>
#include <board.h>
#include <time.h>
#include <dma.h>

/* vbusHigh:  0 if VBUS is low (USB not connected).
 *            1 if VBUS is high (USB connected).
 *  This variable is updated by wixelDetectVbus(). */
static BIT vbusHighBit;

void systemInit()
{
    boardIoInit();
    boardClockInit();
    timeInit();
    dmaInit();
}

void boardService()
{
    boardStartBootloaderIfNeeded();
}

/* Starts up the external 48 MHz oscillator and configures
 * the processor to run at 24 MHz.
 */
void boardClockInit()
{
    // OSC_PD=0: Power up both high-speed oscillators: XOSC and HS RCOSC.
    SLEEP &= ~0x04;

    // Wait until the high speed crystal oscillator is stable (SLEEP.XOSC_STB=1)
    // This condition is required to use the radio or USB.
    // According to Table 6.4.2, the waiting should take about 650 microseconds.
    while(!(SLEEP & 0x40));

    // OSC32K = 1: Use low power 32kHz oscillator for the 32 kHz signal.
    // OSC=0: Select high speed crystal oscillator for system clock (24 MHz).
    // TICKSPD=000: Set the timer tick speed to its fastest possible value.
    // CLKSPD=000: Set system clock speed to its fastest possible value (24 MHz).
    //    This is required for using the Forward Error Correction radio feature (which we don't use anymore).
    CLKCON = 0x80;

    // Power down the HS RCOSC (the one that is not currently selected by
    // CLKCON.OSC).
    SLEEP |= 0x04;

    // Enable pre-fetching of instructions from flash,
    // which makes the code execute much faster.
    MEMCTR = 0;
}

void boardIoInit()
{
    P2DIR = 0;           // Make all the Port 2 pins be inputs.
    P2 = 0b00000110;     // P2_1 = 1: drive the red LED line high LATER (when LED_RED(1) is called)
                         // P2_2 = 1: drive the yellow LED line high LATER (when LED_YELLOW(1) is called)
                         // P2_4 = 0: drive the VBUS_IN/GREEN_LED line low LATER (when LED_GREEN(1) is called)
    P2INP = 0b10011001;  // Pull down LED pins (P2_2, P2_1), and tristate the other Port 2 pins.
}

/* Checks to see if USB is connected (VBUS_IN line is high).
 * The check is only performed if it has not been performed within the last 25 milliseconds.
 * (USB spec says we have to detect power loss within 100 ms).
 * This function updates the bit variable vbusHigh. */
static void boardDetectVbus()
{
    static uint8 lastCheck = 128;
    if ((uint8)(getMs() - lastCheck) > 25)
    {
        BIT savedState = (P2DIR >> 4) & 1;
        if (savedState == 0)
        {
            P2DIR |= (1<<4);       // Drive the VBUS_IN low
            delayMicroseconds(2);
        }
        P2INP &= ~(1<<4);          // Set input mode to pull-down
        P2DIR &= ~(1<<4);          // Make the line an input.
        delayMicroseconds(1);

        vbusHighBit = P2_4;           // Measure the voltage.

        P2INP |= (1<<4);           // Set input mode to tristate.
        if (savedState)
        {
            P2DIR |= (1<<4);  // LED was on previously so turn it back on.
        }

        lastCheck = getMs();
    }
}

void boardStartBootloader()
{
    EA = 0;             // Disable interrupts.

    DMAARM = 0x9F;      // Disarm all DMA channels.

    delayMicroseconds(10);  // Probably not necessary anymore.  Used to be 10 milliseconds.

    // Make all the IO lines be inputs.  That's going to happen later in
    // the bootloader anyway.  We might as well do it now so that any devices
    // such as motors stop running right away.  This also signals to the USB host
    // that we are disconnecting.
    P0DIR = P1DIR = P2DIR = 0;
    P0SEL = P1SEL = P2SEL = 0;

    delayMs(100);       // Delay to give the USB host a chance to detect that we disconnected.
    __asm ljmp 6 __endasm;
}

void boardStartBootloaderIfNeeded()
{
    if (!(P2DIR & (1<<2)))       // If the yellow LED is off...
    {
        delayMicroseconds(10);
        if (P2_2)
        {
            boardStartBootloader();
        }
    }
}

BIT usbPowerPresent()
{
    boardDetectVbus();
    return vbusHighBit;
}

BIT vinPowerPresent()
{
    return P2_3;       // Read VIN_IN.
}

void disableUsbPullup()
{
    P2DIR &= ~(1<<0);  // Make P2_0 be a floating input.
}

void enableUsbPullup()
{
    P2_0 = 1;
    P2DIR |= (1<<0);   // Drive P2_0 high.
}
