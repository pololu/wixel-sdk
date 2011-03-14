#include <cc2511_map.h>
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <stdio.h>

int32 CODE param_blink_period_ms = 500;

BIT yellowLedOn = 1;

void updateLeds()
{
    static uint32 lastToggle = 0;

    usbShowStatusWithGreenLed();

    LED_YELLOW(yellowLedOn);

    // NOTE: The code below is bad because it is reading two bytes of timeMs,
    // and the interrupt that updates timeMs could fire between those two reads.
    if ((uint16)(timeMs - lastToggle) >= param_blink_period_ms)
    {
        LED_RED(!LED_RED_STATE);
        lastToggle = timeMs;
    }
}

void receiveCommands()
{
    if (usbComRxAvailable() && usbComTxAvailable() >= 64)
    {
        uint8 XDATA response[64];
        uint8 byte;
        uint8 length;
        byte = usbComRxReceiveByte();
        switch(byte)
        {
        // Commands for toggling the yellow LED.
        case 'y':
        case 0x81:
        	yellowLedOn = !yellowLedOn;
        	break;

        // Commands for getting the blink_period parameter.
        case 'b':
            length = sprintf(response, "blink period = %d ms\r\n", (uint16)param_blink_period_ms);
            usbComTxSend(response, length);
            break;
        case 0x82:
        	response[0] = param_blink_period_ms & 0xFF;
        	response[1] = param_blink_period_ms >> 8 & 0xFF;
        	usbComTxSend(response, 2);
        	break;
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
