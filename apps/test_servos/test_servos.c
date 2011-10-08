#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <servo.h>

static uint8 CODE servoPins[] = {14};

void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(0);
    T1CNTL;
    LED_RED(T1CNTH & 0x80 ? 0 : 1);
}

void updateServos()
{

}

void main()
{
    // OSC_PD=0: Power up both high-speed oscillators: XOSC and HS RCOSC.
    //SLEEP &= ~0x04;

    // Wait until the high speed crystal oscillator is stable (SLEEP.XOSC_STB=1)
    // This condition is required to use the radio or USB.
    // According to Table 6.4.2, the waiting should take about 650 microseconds.
    //while(!(SLEEP & 0x40));

    // OSC32K = 1: Use low power 32kHz oscillator for the 32 kHz signal.
    // OSC=0: Select high speed crystal oscillator for system clock (24 MHz).
    // TICKSPD=000: Set the timer tick speed to its fastest possible value.
    // CLKSPD=000: Set system clock speed to its fastest possible value (24 MHz).
    //    This is required for using the Forward Error Correction radio feature (which we don't use anymore).
    //CLKCON = 0x80;

    // Power down the HS RCOSC (the one that is not currently selected by
    // CLKCON.OSC).
    //SLEEP |= 0x04;

    //boardIoInit();
    //boardClockInit();
    //timeInit();
    //dmaInit();

    //usbInit();
    servosStart((uint8 XDATA *)servoPins, sizeof(servoPins));

    while(1)
    {
        __asm nop nop nop nop nop nop nop nop nop nop nop nop __endasm;
        __asm nop nop nop nop nop nop nop nop nop nop nop nop __endasm;
        __asm nop nop nop nop nop nop nop nop nop nop nop nop __endasm;
        //boardService();
        //usbComService();
        //updateLeds();
        //updateServos();
    }
}
