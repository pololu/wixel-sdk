/** \file uart1.h
 * For information about these functions, see uart0.h.
 * These functions/variables do exactly the same thing as the functions
 * in uart0.h, except they apply to USART0 instead of USART1.
 */

#ifndef _UART1_H
#define _UART1_H

#include <cc2511_map.h>
#include <cc2511_types.h>

void uart1Init();
void uart1SetBaudRate(uint32 baudrate);
uint8 uart1TxAvailable(void);
void uart1TxSendByte(uint8 byte);
void uart1TxSend(const uint8 XDATA * buffer, uint8 size);
uint8 uart1RxAvailable(void);
uint8 uart1RxReceiveByte(void);
ISR(UTX1, 1);
ISR(URX1, 1);
extern volatile BIT uart1RxParityErrorOccurred;
extern volatile BIT uart1RxFramingErrorOccurred;
extern volatile BIT uart1RxBufferFullOccurred;

#endif /* UART_H_ */
