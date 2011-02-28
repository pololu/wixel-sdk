/*! \file uart.h
 *  The <code>uart.lib</code> library allows you to send and receive a stream
 *  of asynchronous serial bytes on USART0 in UART mode.
 *  This library uses a circular buffer and an interrupt for RX, and a
 *  circular buffer and an interrupt for TX, so it is capable of sending
 *  and receiving a constant stream of bytes without interruption.
 *
 *  To use this library, you must include uart.h in your program:
 *
 *  <pre>
 *  #include <uart.h>
 *  </pre>
 */

#ifndef _UART_H
#define _UART_H

#include <cc2511_map.h>
#include <cc2511_types.h>

/*! Initializes the library.
 *
 * This must be called before any of other library functions.
 */
void uart0Init();

/*! Sets the baud rate.
 *
 * \param baud The baud rate, in bits per second (bps).  Must be between 23 and 495782.
 *
 * Note: Higher baud rates are achievable if you write directly to the U0CGR and U0BAUD
 * registers instead of using this function.
 */
void uart0SetBaudRate(uint32 baudrate);

/*! \return The number of bytes available in the TX buffer.
 */
uint8 uart0TxAvailable(void);

/*! Adds a byte to the TX buffer, which means it will be sent on UART0's TX line later.
 * \param byte  The byte to send.
 *
 * This is a non-blocking function: you must call uart0TxAvailable before calling this
 * function and be sure not to add too many bytes to the buffer (the number of times you call
 * this should not exceed the last value returned by uart0TxAvailable).
 */
void uart0TxSendByte(uint8 byte);

/*! Adds bytes to the TX buffer, which means they will be sent on UART0's TX
 * line later.  This is a non-blocking function: you must call uart0TxAvailable
 * before calling this function be sure not to add too many bytes to the buffer
 * (the size param should not exceed the last value returned by uart0TxAvailable).
 *
 * \param buffer  A pointer to the bytes to send.
 * \param size    The number of bytes to send.
 */
void uart0TxSend(const uint8 XDATA * buffer, uint8 size);

/*! \return The number of bytes in the RX buffer.
 *
 * You can use this function to see if any bytes have been received, and
 * then use uart0RxReceiveByte to actually get the byte and process it.
 */
uint8 uart0RxAvailable(void);

/*! \return A byte from the RX buffer.
 *
 * Bytes are returned in the order they were received on the RX line.
 * This is a non-blocking function: you must call uart0RxAvailable before calling
 * this function and be sure not to read too many bytes to the buffer (the number
 * of times you call this should not exceed the last value returned by
 * uart0RxAvailable).
 */
uint8 uart0RxReceiveByte(void);

/*! Transmit interrupt. */
ISR(UTX0, 1);

/*! Receive interrupt. */
ISR(URX0, 1);

extern volatile BIT uart0RxParityErrorOccurred;
extern volatile BIT uart0RxFramingErrorOccurred;
extern volatile BIT uart0RxBufferFullOccurred;

#endif /* UART_H_ */
