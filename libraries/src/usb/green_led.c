#include "usb.h"
#include "board.h"
#include "time.h"

void usbShowStatusWithGreenLed()
{
    if (usbSuspended() || usbDeviceState == USB_STATE_DETACHED)
    {
    	LED_GREEN(0);                         // We aren't connected to USB, or we are in suspend
    	                                      // mode, so turn off the LED.
    }
    else if (usbDeviceState == USB_STATE_CONFIGURED)
    {
        LED_GREEN(1);                         // On solid because we are properly connected.
    }
    else
    {
        LED_GREEN(timeMs >> 9 & 1);           // Blink with a period of 1024 ms.
    }
}
