/*! \file uart0.h
 *  The <code>uart.lib</code> library allows you to send and receive a stream
 *  of asynchronous serial bytes on USART0 and/or USART1 in UART mode.
 *  This library uses a circular buffer and an interrupt for RX, and a
 *  circular buffer and an interrupt for TX, so it is capable of sending
 *  and receiving a continuous stream of bytes with no gaps.
 *
 *  To use this library, you must include uart0.h or uart1.h in your app:
 *  <pre>#include <uart0.h>
 *  #include <uart1.h>
 *  </pre>
 *
 *  Since this library uses interrupts, the
 */

#ifndef _UART0_H
#define _UART0_H

#include <cc2511_map.h>
#include <cc2511_types.h>
#include <com.h>

// NOTE: Do not change the value of PARITY_* definitions below;
// they must match Table 17 in the CDC PSTN Subclass Specification.

/*! Initializes the library.
 *
 * This must be called before any of other functions with names that
 * begin with "uart0".
 */
void uart0Init();

/*! Sets the baud rate.
 *
 * \param baudrate The baud rate, in bits per second (bps).  Must be between 23 and 495782.
 *
 * Note: Higher baud rates are achievable if you write directly to the U0CGR and U0BAUD
 * registers instead of using this function.
 */
void uart0SetBaudRate(uint32 baudrate);

/*! Sets the parity type of the serial port.
 *
 * \param parity Should be either PARITY_NONE, PARITY_ODD, PARITY_EVEN, PARITY_MARK,
 *  or PARITY_SPACE.  These values are consistent with USB CDC ACM protocol
 *  (see CDC PSTN Subclass Specification, Table 17).
 */
void uart0SetParity(uint8 parity);

/*! Sets the number of stop bits to be transmitted and checked for during reception.
 *
 * \param stopBits Should be either STOP_BITS_1 or STOP_BITS_2.  These values are
 * consistent with the USB CDC ACM protocol (see CDC PSTN Subclass Specification,
 * Table 17).
 *
 * The CC2511's UARTs do not actually support 1.5 stop bits, so if the argument to
 * this function is STOP_BITS_1_5, the UART will be set to 1 stop bit instead.
 *
 * The CC2511's UARTs do not support having 2 stop bits very well, because the
 * the framing error bit (UxCSR.FE) is set 1 bit duration *after* the interrupt
 * is triggered.  Therefore, if stopBits is set to 2, this library may fail to
 * detect framing errors from the second stop bit.  Also, the byte received after
 * the framing error occurred may be thrown out even if that byte is valid.
 */
void uart0SetStopBits(uint8 stopBits);

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
