#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <servo.h>

static uint8 CODE servoPins[] = { 14 };

void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(1);
    T1CNTL;
    LED_RED(T1CNTH & 0x80 ? 0 : 1);
}

void updateServos()
{
    
}

void main()
{
    systemInit();
    usbInit();
    servosStart((uint8 XDATA *)servoPins, sizeof(servoPins));

    while(1)
    {
        boardService();
        usbComService();
        updateLeds();
        updateServos();
    }
}
