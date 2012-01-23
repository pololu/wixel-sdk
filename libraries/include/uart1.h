/** \file uart1.h
 * For information about these functions, see uart0.h.
 * These functions/variables do exactly the same thing as the functions
 * in uart0.h, except they apply to USART1 instead of USART0.
 */

#ifndef _UART1_H
#define _UART1_H

#include <cc2511_map.h>
#include <cc2511_types.h>
#include <com.h>

void uart1Init();
void uart1SetBaudRate(uint32 baudrate);
void uart1SetParity(uint8 parity);
void uart1SetStopBits(uint8 stopBits);
uint8 uart1TxAvailable(void);
void uart1TxSendByte(uint8 byte);
void uart1TxSend(const uint8 XDATA * buffer, uint8 size);
uint8 uart1RxAvailable(void);
uint8 uart1RxReceiveByte(void);
ISR(UTX1, 0);
ISR(URX1, 0);
extern volatile BIT uart1RxParityErrorOccurred;
extern volatile BIT uart1RxFramingErrorOccurred;
extern volatile BIT uart1RxBufferFullOccurred;

#endif /* UART_H_ */
