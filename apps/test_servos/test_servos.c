#include <cc2511_map.h>
#include <servo.h>

void main()
{
    // Set up 24 MHz clock.
    SLEEP &= ~0x04;
    while(!(SLEEP & 0x40));
    CLKCON = 0x80;
    SLEEP |= 0x04;

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
    }
}
