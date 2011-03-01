#include <cc2511_map.h>
#include <wixel.h>
#include <usb_com.h>
#include <stdio.h>

int32 CODE param_blink_period = 500;

void updateLeds()
{
    static uint32 lastToggle = 0;

    usbShowStatusWithGreenLed();

    LED_YELLOW(0);

    // NOTE: The code below is bad because it is reading two bytes of timeMs,
    // and the interrupt that updates timeMs could fire between those two reads.
    if ((uint16)(timeMs - lastToggle) >= param_blink_period)
    {
        LED_RED(!LED_RED_STATE);
        lastToggle = timeMs;
    }
}

void receiveCommands()
{
    uint8 XDATA response[64];

    if (usbComRxAvailable() && usbComTxAvailable() >= 64)
    {
        uint8 byte;
        byte = usbComRxReceiveByte();
        if (byte == (uint8)'?')
        {
            uint8 length = sprintf(response, "? blink period = %d\r\n", (uint16)param_blink_period);
            usbComTxSend(response, length);
        }
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
        receiveCommands();
    }
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
