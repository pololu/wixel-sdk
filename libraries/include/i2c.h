#ifndef _I2C_H
#define _I2C_H

#include <cc2511_types.h>

extern BIT i2cTimeoutOccurred;

void i2cInit(uint16 freqKHz, uint16 timeoutMs);

BIT i2cWriteByte(uint8 byte, uint8 sendStart, uint8 sendStop);
uint8 i2cReadByte(uint8 nack, uint8 sendStop);

#endif
