#ifndef _SERVO_H
#define _SERVO_H

#include <cc2511_map.h>
#include <cc2511_types.h>

void servosStart(uint8 XDATA * pins, uint8 num_pins);
void servosStop(void);

void servoSetTarget(uint8 servo_num, uint16 target);
uint16 servoGetTarget(uint8 servo_num);

uint16 servoGetPosition(uint8 servo_num);

void servoSetSpeed(uint8 servo_num, uint16 speed);
uint16 servoGetSpeed(uint8 servo_num);

ISR(T1, 1);


#endif
