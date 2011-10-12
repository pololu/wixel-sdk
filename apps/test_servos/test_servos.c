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

    servoSetSpeed(0, 400);
    servoSetSpeed(1, 400);
    servoSetSpeed(2, 400);
    servoSetSpeed(3, 400);
    servoSetSpeed(4, 210);

    servoSetTarget(1, 1000);
    servoSetTarget(2, 1520);
    servoSetTarget(3, 1540);
    servoSetTarget(4, 1560);

    servoSetSpeed(5, 0);
    servoSetTarget(5, 1000);
    servoSetSpeed(5, 1);
    servoSetTarget(5, 2000);

    while(1)
    {
        boardService();
        usbComService();
        usbShowStatusWithGreenLed();

        if (getMs() >> 11 & 1)
        {
            servoSetTarget(0, 1000);
            servoSetTargetHighRes(1, 26400);
            servoSetTargetHighRes(2, 26401);
            servoSetTarget(3, 0);
            servoSetTarget(4, 1100);
        }
        else
        {
            servoSetTarget(0, 2000);
            servoSetTargetHighRes(1, 26400);
            servoSetTargetHighRes(2, 26401);
            servoSetTarget(3, 1600);
            servoSetTarget(4, 1900);
        }
    }
}
