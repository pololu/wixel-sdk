#include <cc2511_map.h>
#include <servo.h>
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>

uint8 CODE pins[] = {10, 11, 12, 2, 3, 4};

void main()
{
    systemInit();
    usbInit();

    servosStart((uint8 XDATA *)pins, sizeof(pins));

    while(1)
    {
        boardService();
        usbComService();
        if (getMs() >> 9 & 1)
        {
            servoSetTarget(0, 1000);
        }
        else
        {
            servoSetTarget(0, 2000);
        }
    }
}
