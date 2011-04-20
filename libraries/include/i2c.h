#ifndef _I2C_H
#define _I2C_H

#include <cc2511_types.h>

void i2cInit(uint16 kHzFreq);

uint8 i2cReadScl(void);
uint8 i2cReadSda(void);

void i2cClearScl(void);
void i2cClearSda(void);

uint8 i2cWaitForHighScl(uint16 timeoutMs);

uint8 i2cStart(void);
uint8 i2cStop(void);

uint8 i2cWriteBit(uint8 b);
uint8 i2cReadBit(void);

uint8 i2cWriteByte(uint8 byte, uint8 sendStart, uint8 sendStop);
uint16 i2cReadByte(uint8 nack, uint8 sendStop);

#define I2C_READ_DATA_MASK  0x00FF
#define I2C_READ_ERROR_MASK 0xFF00

#endif
