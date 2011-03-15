#include <cc2511_map.h>
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <stdio.h>

int32 CODE param_blink_period_ms = 1000;

uint32 lastToggle = 0;

void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(0);

    // NOTE: The code below is bad because it is reading two bytes of timeMs,
    // and the interrupt that updates timeMs could fire between those two reads.
    if ((uint16)(getMs() - lastToggle) >= param_blink_period_ms)
    {
        LED_RED(!LED_RED_STATE);
        lastToggle = getMs();
    }
}

void main()
{
    systemInit();
    usbInit();

    while(1)
    {
        boardService();
        updateLeds();
        usbComService();
    }
}
