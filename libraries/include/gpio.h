#ifndef GPIO_H_
#define GPIO_H_

#include <cc2511_types.h>

void setDigitalOutput(uint8 pin, BIT value) __reentrant;

#endif
