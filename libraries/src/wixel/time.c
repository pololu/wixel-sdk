/* \file time.c
 *
 * This is the source file for the time component of <code>wixel.lib</code>.
 * For information on how to use these functions, see time.h.
 */

#include <cc2511_types.h>
#include <time.h>

PDATA volatile uint32 timeMs;

ISR(T4, 1)
{
    timeMs++;
    // T4CC0 ^= 1; // If we do this, then on average the interrupts will occur precisely 1.000 ms apart.
}

void timeInit()
{
    T4CC0 = 187;
    T4IE = 1;     // Enable Timer 4 interrupt.  (IEN1.T4IE=1)

    // DIV=111: 1:128 prescaler
    // START=1: Start the timer
    // OVFIM=1: Enable the overflow interrupt.
    // MODE=10: Modulo
    T4CTL = 0b11111010;

    EA = 1; // Globally enable interrupts (IEN0.EA=1).
}

void delayMs(uint16 milliseconds)
{
    // TODO: make this more accurate.
    // A great way would be to use the compare feature of Timer 4 and then
    // wait for the right number of compare events to happen, but then we
    // can't use that channel for PWM in the future.
    while(milliseconds--)
    {
        delayMicroseconds(250);
        delayMicroseconds(250);
        delayMicroseconds(250);
        delayMicroseconds(249); // there's some overhead, so only delay by 249 here
    }
}
