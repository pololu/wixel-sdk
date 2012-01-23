/*! \file spi1_master.h
 * For information about these functions, see spi0_master.h.
 * These functions do exactly the same thing as the functions
 * in uart0.h, except they apply to USART1 instead of USART0.
 */

#ifndef _SPI1_MASTER_H
#define _SPI1_MASTER_H

#include <cc2511_map.h>
#include <cc2511_types.h>
#include <spi.h>

void spi1MasterInit(void);
void spi1MasterSetFrequency(uint32 freq);
void spi1MasterSetClockPolarity(BIT polarity);
void spi1MasterSetClockPhase(BIT phase);
void spi1MasterSetBitOrder(BIT bitOrder);
BIT spi1MasterBusy(void);
uint16 spi1MasterBytesLeft(void);
void spi1MasterTransfer(const uint8 XDATA * txBuffer, uint8 XDATA * rxBuffer, uint16 size);
uint8 spi1MasterSendByte(uint8 XDATA byte);
uint8 spi1MasterReceiveByte(void);

ISR(URX1, 0);

#endif /* SPI1_MASTER_H_ */
