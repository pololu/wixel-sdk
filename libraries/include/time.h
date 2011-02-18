/* Header file for the Time module of Wixel library.
 * This module helps you keep track of time in milliseconds.
 * Calling timerInit() sets up a timer (Timer 4) to overflow every millisecond.
 * You can read the time at any time by reading the timeMs variable.
 * For this module to work, you must write "include <libwixel/time.h>" in the
 * same file that contains main().  Also, this module depends on the timer tick
 * speed being set to 24 MHz, which is done by wixelClockInit().
 * 
 */

#ifndef _WIXEL_TIME_H
#define _WIXEL_TIME_H

#include <cc2511_map.h>

void timeInit();
extern volatile PDATA uint32 timeMs;
void T4_ISR() __interrupt(T4_VECTOR);

#endif
