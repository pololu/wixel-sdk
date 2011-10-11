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
    servoSetSpeed(4, 400);
    servoSetSpeed(5, 400);


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

        if (getMs() >> 11 & 1)
        {
            servoSetTarget(0, 1000);
            servoSetTarget(1, 1100);
            servoSetTarget(2, 1200);
            servoSetTarget(3, 1300);
            servoSetTarget(4, 1400);
            servoSetTarget(5, 1100);
        }
        else
        {
            servoSetTarget(0, 2000);
            servoSetTarget(1, 1900);
            servoSetTarget(2, 1800);
            servoSetTarget(3, 1600);
            servoSetTarget(4, 1500);
            servoSetTarget(5, 1900);
        }
    }
}
