/*! \file uart0.h
 *  The <code>uart.lib</code> library allows you to send and receive a stream
 *  of asynchronous serial bytes on USART0 and/or USART1 in UART mode.
 *  This library uses circular buffers and interrupts for both RX and TX
 *  so it is capable of sending
 *  and receiving a continuous stream of bytes with no gaps.
 *
 *  To use this library, you must include uart0.h or uart1.h in your app:
\code
#include <uart0.h>  // for UART 0
#include <uart1.h>  // for UART 1
\endcode
 *
 * Since this library uses interrupts, the include statement must be present
 * in the file that contains main().
 *
 * The API for using UART 1 is the same as the API for using UART 0 that is
 * documented here, except all the function and variable names begin with
 * "uart1" instead of "uart0".
 *
 * For UART0, this library uses Alternative Location 1: P0_3 is TX, P0_2 is RX.
 * For UART1, this library uses Alternative Location 2: P1_6 is TX, P1_7 is RX.
 * This library does not yet allow you to choose which UART location to use.
 */

#ifndef _UART0_H
#define _UART0_H

#include <cc2511_map.h>
#include <cc2511_types.h>
#include <com.h>

/*! Initializes the library.
 *
 * This must be called before any of other functions with names that
 * begin with "uart0". */
void uart0Init();

/*! Sets the baud rate.
 *
 * \param baudrate The baud rate, in bits per second (bps).  Must be between 23 and 1,500,000. */
void uart0SetBaudRate(uint32 baudrate);

/*! Sets the parity type of the serial port.
 *
 * \param parity Should be either #PARITY_NONE, #PARITY_ODD, #PARITY_EVEN, #PARITY_MARK,
 *  or #PARITY_SPACE.
 *
 *  The default is #PARITY_NONE.
 */
void uart0SetParity(uint8 parity);

/*! Sets the number of stop bits to be transmitted and checked for during reception.
 *
 * \param stopBits Should be either #STOP_BITS_1 or #STOP_BITS_2.
 *
 * The CC2511's UARTs do not actually support 1.5 stop bits, so if the argument to
 * this function is #STOP_BITS_1_5, the UART will be set to 1 stop bit instead.
 *
 * The CC2511's UARTs do not support having 2 stop bits very well, because the
 * the framing error bit (UxCSR.FE) is set 1 bit duration <b>after</b> the interrupt
 * is triggered.  Therefore, if stopBits is #STOP_BITS_2, this library may fail to
 * detect framing errors from the second stop bit.  Also, the next byte received
 * after the framing error occurred may be thrown out even if that byte is valid.
 *
 * The default is #STOP_BITS_1.
 */
void uart0SetStopBits(uint8 stopBits);

/*! \return The number of bytes available in the TX buffer.
 */
uint8 uart0TxAvailable(void);

/*! Adds a byte to the TX buffer, which means it will be sent on UART0's TX line later.
 * \param byte  The byte to send.
 *
 * This is a non-blocking function: you must call uart0TxAvailable() before calling this
 * function and be sure not to add too many bytes to the buffer.  The number of times you call
 * this should not exceed the last value returned by uart0TxAvailable().
 */
void uart0TxSendByte(uint8 byte);

/*! Adds bytes to the TX buffer, which means they will be sent on UART0's TX
 * line later.  This is a non-blocking function: you must call uart0TxAvailable()
 * before calling this function be sure not to add too many bytes to the buffer.
 * The \p size param should not exceed the last value returned by uart0TxAvailable().
 *
 * \param buffer  A pointer to the bytes to send.
 * \param size    The number of bytes to send.
 */
void uart0TxSend(const uint8 XDATA * buffer, uint8 size);

/*! \return The number of bytes in the RX buffer.
 *
 * You can use this function to see if any bytes have been received, and
 * then use uart0RxReceiveByte() to actually get the byte and process it.
 */
uint8 uart0RxAvailable(void);

/*! \return A byte from the RX buffer.
 *
 * This is a non-blocking function: you must call uart0RxAvailable() before calling
 * this function and be sure not to read too many bytes.  The number
 * of times you call this should not exceed the last value returned by
 * uart0RxAvailable().
 *
 * Bytes are returned in the order they were received on the RX line.
 *
 */
uint8 uart0RxReceiveByte(void);

/*! Transmit interrupt. */
ISR(UTX0, 0);

/*! Receive interrupt. */
ISR(URX0, 0);

/*! The library sets this to 1 whenever a parity error occurs. */
extern volatile BIT uart0RxParityErrorOccurred;

/*! The library sets this to 1 whenever a framing error occurs. */
extern volatile BIT uart0RxFramingErrorOccurred;

/*! The library sets this to 1 whenever a new byte arrives and
 * the RX buffer is full.
 * In that case, the byte will be discarded.*/
extern volatile BIT uart0RxBufferFullOccurred;

#endif /* UART_H_ */
