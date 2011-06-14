/*
 * spi0_master.h
 */

#ifndef SPI0_MASTER_H_
#define SPI0_MASTER_H_

#include <spi.h>

void spi0MasterInit(void);
void spi0MasterSetFrequency(uint32 freq);
void spi0MasterSetClockPolarity(BIT polarity);
void spi0MasterSetClockPhase(BIT phase);
void spi0MasterSetBitOrder(BIT bitOrder);
uint16 spi0MasterBytesLeft(void);
void spi0MasterTransfer(const uint8 XDATA * txBuffer, const uint8 XDATA * rxBuffer, uint16 size);

ISR(URX0, 1);

#endif /* SPI0_MASTER_H_ */
