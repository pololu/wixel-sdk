#ifndef _GPIO_H
#define _GPIO_H

#include <cc2511_types.h>

#define LOW   0
#define HIGH  1

#define HIGH_IMPEDANCE  0
#define PULLED          1

void setDigitalOutput(uint8 pin, BIT value) __reentrant;

void setDigitalInput(uint8 pin, BIT pulled) __reentrant;

BIT isDigitalInputHigh(uint8 pin) __reentrant;

void setPeripheralOutput(uint8 pin) __reentrant;

void setPort0PullType(BIT pullType) __reentrant;

void setPort1PullType(BIT pullType) __reentrant;

void setPort2PullType(BIT pullType) __reentrant;

#endif
