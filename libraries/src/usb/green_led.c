#include <usb.h>
#include <board.h>
#include <time.h>

// USBFRML is the USB frame number, which is a convenient millisecond
// time base that we can use when we are the configured USB state
// and not in suspend mode.

BIT usbBlinkActive = 0;
uint8 usbLastActivity;
uint8 usbBlinkStart;

void usbShowStatusWithGreenLed()
{
    if (usbActivityFlag)
    {
        // Some USB activity happened recently.
        usbActivityFlag = 0;

        // Record the time that the USB activity occurred.
        usbLastActivity = USBFRML;

        // If we are not already blinking to indicate USB activity,
        // start blinking.
        if (!usbBlinkActive)
        {
            usbBlinkActive = 1;
            usbBlinkStart = USBFRML;
        }
    }

    if (usbSuspended() || usbDeviceState == USB_STATE_DETACHED)
    {
        LED_GREEN(0);                         // We aren't connected to USB, or we are in suspend
                                              // mode, so turn off the LED.
    }
    else if (usbDeviceState == USB_STATE_CONFIGURED)
    {
        if (usbBlinkActive)
        {
            LED_GREEN((USBFRML - usbBlinkStart) & 64);

            if ((uint8)(USBFRML - usbLastActivity) > 96)
            {
                usbBlinkActive = 0;
            }
        }
        else
        {
            LED_GREEN(1);  // On solid because we are properly connected.
        }
    }
    else
    {
        // We do not use USBFRML for timing here because it might not be reliable
        // before we reach the configured state.
        LED_GREEN(getMs() >> 9 & 1);           // Blink with a period of 1024 ms.
    }
}
