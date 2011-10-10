#include <cc2511_map.h>
#include <servo.h>
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>

uint8 CODE pins[] = {2, 3, 4, 12, 11, 10};

void main()
{
    systemInit();
    usbInit();

    servosStart((uint8 XDATA *)pins, sizeof(pins));

    servoSetTarget(1, 24000);
    servoSetTarget(2, 1520);
    servoSetTarget(3, 1540);
    servoSetTarget(4, 1560);
    servoSetTarget(5, 1580);

    while(1)
    {
        boardService();
        usbComService();
        usbShowStatusWithGreenLed();

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
