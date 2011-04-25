#ifndef _I2C_H
#define _I2C_H

#include <cc2511_types.h>

extern BIT i2cTimeoutOccurred;

void i2cSetFrequency(uint16 freqKHz);
void i2cSetTimeout(uint16 timeoutMs);

void i2cStart();
void i2cStop();

BIT i2cWriteByte(uint8 byte, uint8 sendStart, uint8 sendStop);
uint8 i2cReadByte(uint8 nack, uint8 sendStop);

#endif
