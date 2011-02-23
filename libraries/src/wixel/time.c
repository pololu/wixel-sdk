#include <cc2511_types.h>
#include <time.h>

/* timeMs: The approximate number of milliseconds since the processor
 * started.  NOTE: This is not very accurate.  The units of this time
 * measurement are actually closer to 1.00266 milliseconds.  If you need
 * millisecond timing more accurate than that, use Timer 1.
 * This variable is updated by T4_ISR.  */
PDATA volatile uint32 timeMs;

/*  void T4_ISR()
 * The interrupt service routine (ISR) for Timer 4.
 * This function is called every 1.00266 milliseconds, when
 * Timer 4 overflows.  It increments timeMs. */
void T4_ISR() __interrupt(T4_VECTOR) __using(1)
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

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
