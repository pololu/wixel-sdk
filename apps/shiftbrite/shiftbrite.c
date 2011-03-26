#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <stdio.h>

int32 CODE param_blink_period_ms = 500;

uint32 lastToggle = 0;
void sendRGB(uint16 r, uint16 g, uint16 b);

void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(0);

    if ((uint16)(getMs() - lastToggle) >= (param_blink_period_ms/2))
    {
        LED_RED(!LED_RED_STATE);
        lastToggle = getMs();
        sendRGB(1023,1023,1023);
    }
}

void toggleLatch()
{
    delayMs(1);
    P1_7 = 1;
    delayMs(1);
    P1_7 = 0;
    delayMs(1);
}

void sendBit(BIT value)
{
    P1_6 = value;
    delayMs(1);
    P1_5 = 1;
    delayMs(1);
    P1_5 = 0;
    delayMs(1);
}

void sendRGB(uint16 r, uint16 g, uint16 b)
{
    uint16 mask = 512;
    sendBit(0);
    sendBit(0);
    while(mask)
    {
        sendBit((mask & b) ? 1 : 0);
        mask >>= 1;
    }
    mask = 512;
    while(mask)
    {
        sendBit((mask & r) ? 1 : 0);
        mask >>= 1;
    }
    mask = 512;
    while(mask)
    {
        sendBit((mask & g) ? 1 : 0);
        mask >>= 1;
    }

    toggleLatch();
}

void main()
{
    systemInit();
    usbInit();

    P1DIR |= (1<<4); // P1_4 = !Enable
    P1DIR |= (1<<5); // P1_5 = Clock
    P1DIR |= (1<<6); // P1_6 = Data
    P1DIR |= (1<<7); // P1_7 = Latch
    P1_4 = 0; // enable shiftbrites
    P1_5 = 0; // clock low

    while(1)
    {
        boardService();
        updateLeds();
        usbComService();
    }
}
