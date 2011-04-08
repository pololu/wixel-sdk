#include <cc2511_map.h>
#include <board.h>
#include <time.h>

#include <usb.h>
#include <usb_hid.h>
#include <adc.h>
#include <gpio.h>
#include <radio_queue.h>
#include <random.h>

void updateLeds()
{
    usbShowStatusWithGreenLed();
}

void rxMouseState(void)
{
    uint8 XDATA * rxBuf;

    if (rxBuf = radioQueueRxCurrentPacket())
    {
        usbHidMouseInput.x = rxBuf[1];
        usbHidMouseInput.y =  rxBuf[2];
        usbHidMouseInput.buttons = rxBuf[3];
        usbHidMouseInputUpdated = 1;
        radioQueueRxDoneWithPacket();
    }
}

void main()
{
    systemInit();
    usbInit();

    radioQueueInit();
    randomSeedFromSerialNumber();

    while(1)
    {
        updateLeds();
        boardService();
        usbHidService();

        rxMouseState();
    }
}
