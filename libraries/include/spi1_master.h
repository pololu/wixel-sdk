/*
 * spi1_master.h
 */

#ifndef _SPI1_MASTER_H
#define _SPI1_MASTER_H

#include <cc2511_types.h>
#include <spi.h>

void spi1MasterInit(void);
void spi1MasterSetFrequency(uint32 freq);
void spi1MasterSetClockPolarity(BIT polarity);
void spi1MasterSetClockPhase(BIT phase);
void spi1MasterSetBitOrder(BIT bitOrder);
uint16 spi1MasterBytesLeft(void);
void spi1MasterTransfer(const uint8 XDATA * txBuffer, uint8 XDATA * rxBuffer, uint16 size);

ISR(URX1, 1);

#endif /* SPI1_MASTER_H_ */
