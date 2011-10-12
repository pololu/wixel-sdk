#include <cc2511_map.h>
#include <servo.h>
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>

uint8 CODE pins[] = {2, 3, 4, 12, 11, 10};

void myServosInit()
{
    servosStart((uint8 XDATA *)pins, sizeof(pins));

    servoSetSpeed(0, 400);
    servoSetSpeed(1, 400);
    servoSetSpeed(2, 400);
    servoSetSpeed(3, 400);
    servoSetSpeed(4, 0);

    servoSetSpeed(5, 0);
    servoSetTarget(5, 1100);
    servoSetSpeed(5, 1);
    servoSetTarget(5, 1900);
}

void updateServos()
{
    if (getMs() >> 11 & 1)
    {
        servoSetTarget(0, 1100);
        servoSetTarget(1, 1500);
        servoSetTarget(2, 1500);
        servoSetTarget(3, 0);
        servoSetTarget(4, 1100);
        LED_YELLOW(0);
    }
    else
    {
        servoSetTarget(0, 1900);
        servoSetTarget(1, 1900);
        servoSetTarget(2, 1100);
        servoSetTarget(3, 1600);
        servoSetTarget(4, 1900);
        LED_YELLOW(1);
    }
}

void receiveCommands()
{
    if (usbComRxAvailable() == 0){ return; }
    switch(usbComRxReceiveByte())
    {
    case 's':  // Start/Stop
        if (servosStarted())
        {
            servosStop();
        }
        else
        {
            myServosInit();
        }
        break;
    }
}

void main()
{
    systemInit();
    usbInit();
    myServosInit();

    while(1)
    {
        boardService();
        usbComService();
        usbShowStatusWithGreenLed();
        updateServos();
        receiveCommands();
    }
}
