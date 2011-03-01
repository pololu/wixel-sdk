#ifndef _WIXEL_H
#define _WIXEL_H

#include <cc2511_map.h>
#include <cc2511_types.h>

#define LED_GREEN(v)        {((v) ? (P2DIR |= 0x10) : (P2DIR &= ~0x10));}
#define LED_YELLOW(v)       {((v) ? (P2DIR |= 0x04) : (P2DIR &= ~0x04));}
#define LED_RED(v)          {((v) ? (P2DIR |= 0x02) : (P2DIR &= ~0x02));}

#define LED_GREEN_STATE     ((P2DIR >> 4) & 1)
#define LED_YELLOW_STATE    ((P2DIR >> 2) & 1)
#define LED_RED_STATE       ((P2DIR >> 1) & 1)

#define LED_GREEN_TOGGLE()  {P2DIR ^= 0x10;}
#define LED_YELLOW_TOGGLE() {P2DIR ^= 0x04;}
#define LED_RED_TOGGLE()    {P2DIR ^= 0x02;}

void systemInit();
void boardIoInit();
void boardClockInit();

void boardService();

void boardStartBootloaderIfNeeded();
void boardStartBootloader();

BIT usbPowerPresent();
BIT vinPowerPresent();
void enableUsbPullup();
void disableUsbPullup();

void delayMicroseconds(uint8 ms);  // defined in delay.asm
void delayMs(uint16 count);

#endif
