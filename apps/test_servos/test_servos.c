#include <cc2511_map.h>
#include <servo.h>
#include <wixel.h>

void mainLoop();

void main()
{
    // Set up 24 MHz clock.
    //boardIoInit();

    P2 = 0b00000110;     // P2_1 = 1: drive the red LED line high LATER (when LED_RED(1) is called)
                         // P2_2 = 1: drive the yellow LED line high LATER (when LED_YELLOW(1) is called)
                         // P2_4 = 0: drive the VBUS_IN/GREEN_LED line low LATER (when LED_GREEN(1) is called)
    P2INP = 0b10011001;  // Pull down LED pins (P2_2, P2_1), and tristate the other Port 2 pins.

    // OSC_PD=0: Power up both high-speed oscillators: XOSC and HS RCOSC.
    SLEEP &= ~0x04;

    // Wait until the high speed crystal oscillator is stable (SLEEP.XOSC_STB=1)
    // This condition is required to use the radio or USB.
    // According to Table 6.4.2, the waiting should take about 650 microseconds.
    while(!(SLEEP & 0x40));

    // OSC32K = 1: Use low power 32kHz oscillator for the 32 kHz signal.
    // OSC=0: Select high speed crystal oscillator for system clock (24 MHz).
    // TICKSPD=000: Set the timer tick speed to its fastest possible value.
    // CLKSPD=000: Set system clock speed to its fastest possible value (24 MHz).
    //    This is required for using the Forward Error Correction radio feature (which we don't use anymore).
    CLKCON = 0x80;

    // Power down the HS RCOSC (the one that is not currently selected by
    // CLKCON.OSC).
    SLEEP |= 0x04;

    MEMCTR = 2;


    //boardClockInit();
    //dmaInit();

    // Set up hardware PWM.
    PERCFG |= (1<<6);  // PERCFG.T1CFG = 1:  Move Timer 1 to Alt. 2 location (P1_0, P1_1, P1_2)
    P1SEL |= (1<<1);
    P1DIR |= (1<<1);

    T1CC0 = 60000;         // Period = 60000/24 = 2500 microseconds.
    T1CCTL1 = 0b01011100;  // Timer 1 Channel 1: Compare mode with interrupt enabled.  Toggle output on match.
    T1CTL = 0b00000010;    // Timer 1: Module mode, repeatedly counts from 0 to T1CC0.

    T1CC1 = 2400;

    IP0 |= (1<<1);
    IP1 |= (1<<1);
    T1IE = 1; // Enable the Timer 1 interrupt.
    EA = 1;   // Enable interrupts in general.

    P1DIR |= (1<<4);

    while(1)
    {
        boardService();
    }

    //__asm nop __endasm;

    //mainLoop();
}
