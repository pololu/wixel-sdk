/*
 * spi1_master.h
 */

#ifndef SPI1_MASTER_H_
#define SPI1_MASTER_H_

#include <spi.h>

void spi1MasterInit(void);
void spi1MasterSetFrequency(uint32 freq);
void spi1MasterSetClockPolarity(BIT polarity);
void spi1MasterSetClockPhase(BIT phase);
void spi1MasterSetBitOrder(BIT bitOrder);
uint16 spi1MasterBytesLeft(void);
void spi1MasterTransfer(const uint8 XDATA * txBuffer, const uint8 XDATA * rxBuffer, uint16 size);

ISR(URX0, 1);

#endif /* SPI1_MASTER_H_ */
