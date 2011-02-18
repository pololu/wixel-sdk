#ifndef _UART_H
#define _UART_H

#include <cc2511_map.h>
#include <cc2511_types.h>

void uart0Init();

void uart0SetBaudRate(uint32 baudrate);

uint8 uart0TxAvailable(void);
void uart0TxSendByte(uint8 byte);
void uart0TxSend(const uint8 XDATA * buffer, uint8 size);
ISR(UTX0, 1);

uint8 uart0RxAvailable(void);
uint8 uart0RxReceiveByte(void);
ISR(URX0, 1);

extern volatile BIT uart0RxParityErrorOccurred;
extern volatile BIT uart0RxFramingErrorOccurred;
extern volatile BIT uart0RxBufferFullOccurred;

#endif /* UART_H_ */
