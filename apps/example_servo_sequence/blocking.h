// These are "responsible" blocking functions defined in blocking.c,
// which call frequentTasks() while they are blocking.

#ifndef _BLOCKING_H
#define _BLOCKING_H

void frequentTasks(void);

void waitMs(uint32 milliseconds);

void servosWaitWhileMoving(void);

uint8 usbComRxReceiveByteBlocking(void);

#endif