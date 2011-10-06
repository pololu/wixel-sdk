/*
 * spi0_master.h
 */

#ifndef _SPI0_MASTER_H
#define _SPI0_MASTER_H

#include <cc2511_types.h>
#include <spi.h>

void spi0MasterInit(void);
void spi0MasterSetFrequency(uint32 freq);
void spi0MasterSetClockPolarity(BIT polarity);
void spi0MasterSetClockPhase(BIT phase);
void spi0MasterSetBitOrder(BIT bitOrder);
BIT spi0MasterBusy(void);
uint16 spi0MasterBytesLeft(void);
void spi0MasterTransfer(const uint8 XDATA * txBuffer, uint8 XDATA * rxBuffer, uint16 size);
uint8 spi0MasterSendByte(uint8 XDATA byte);
uint8 spi0MasterReceiveByte(void);

ISR(URX0, 1);

#endif /* SPI0_MASTER_H_ */
