/*! \file time.h
 * This module helps you keep track of time in milliseconds.
 * Calling timerInit() sets up a timer (Timer 4) to overflow every millisecond
 * (approximately).
 * You can read the time at any time by reading the timeMs variable.
 * For the interrupt to work, you must write
 * <pre>include <time.h></pre>
 * or
 * <pre>include <wixel.h></pre>
 * in the same source file that contains your main() function.
 * Also, you must call boardClockInit() or else the timing will
 * be wrong.
 */

#ifndef _WIXEL_TIME_H
#define _WIXEL_TIME_H

#include <cc2511_map.h>

/*! Initializes the library.  This sets up Timer 4 to tick (approximately)
 * every millisecond and enables the Timer 4 interrupt.  Note that you
 * will also have to call boardClockInit, to get the system clock running
 * at the right speed, otherwise the millisecond timing will be off by a
 * large factor. */
void timeInit();

/*! Returns the number of milliseconds that have elapsed since timeInit()
 * was called or since resetMs was last called.
 */
uint32 getMs();

/*! Resets the millisecond counter to zero.
 */
void resetMs();

// Ben replaced raw access to this volatile global with the safer accessor
// functions above on 110302
/*! The approximate number of milliseconds since timeInit was called
 * started.  This variable is updated by an interrupt (T4_ISR), so
 * you should NOT read multiple bytes from it: if you do so, the
 * interrupt might fire between two successive byte reads and it
 * might result in a carry between the two bytes, which could result
 * in an incorrect value being read.
 *
 * NOTE: This is not very accurate.  The units of this time
 * measurement are actually closer to 1.00266 milliseconds.  If you need
 * millisecond timing more accurate than that, use Timer 1. */
//extern volatile PDATA uint32 timeMs;

/*! This interrupt fires once per millisecond (approximately) and
 * increments timeMs. */
ISR(T4, 1);

#endif
