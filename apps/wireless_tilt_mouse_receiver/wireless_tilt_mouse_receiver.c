/** wireless_tilt_mouse_receiver app:

Receives signals from the wireless_tilt_mouse app and reports them to the
computer using its USB HID interface.

See the wireless_tilt_mouse app (wireless_tilt_mouse.c) for details.
*/

#include <wixel.h>
#include <usb.h>
#include <usb_hid.h>
#include <radio_queue.h>

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

    while(1)
    {
        updateLeds();
        boardService();
        usbHidService();

        rxMouseState();
    }
}
