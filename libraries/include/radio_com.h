/*! \file radio_com.h
 * The <code>radio_com.lib</code> library provides reliable, ordered
 * delivery and reception of a stream of bytes between two devices.
 * This library depends on <code>radio_link.lib</code>.
 * For many applications, this library is easier to use than
 * <code>radio_link.lib</code> because this library takes care of
 * dividing the stream of bytes into packets.
 *
 * This library depends on <code>radio_link.lib</code>, which depends on
 * <code>radio_mac.lib</code>, which uses an interrupt.
 * For this library to work, you must write
 * <code>include <radio_com.h></code>
 * in the source file that contains your main() function.
 *
 * This library has the same limitations as <code>radio_link.lib</code>:
 * It does not work if there are more than two Wixels broadcasting
 * on the same channel.
 * For wireless communication between more than two Wixels, you can use
 * <code>radio_queue.lib</code> (see radio_queue.h).
 *
 * This library also supports sending 8 control signals to the other Wixel
 * and receiving 8 control signals from the other Wixel.
 */

#ifndef _RADIO_COM_H_
#define _RADIO_COM_H_

#include <radio_link.h>

/*! Initializes the <code>radio_com.lib</code> library and the
 * lower-level libraries that it depends on.
 * This must be called before any of the other radioCom* functions. */
void radioComInit(void);

/*! This is a configuration option for the <code>radio_com.lib</code> library that
 * can be set by higher-level code.
 * The default value is 0.
 *
 * When this bit is 1, the radio_com library guarantees that the higher-level code
 * receives notifications of changed control signals via radioComRxControlSignals()
 * and receives data bytes via radioComRxAvailable() and radioComRxReceiveByte() in the
 * <b>same order</b> that these two pieces of information were received from the radio.
 * If this bit is 1, higher-level code must call radioComRxControlSignals() regularly, or
 * else it will not be able to receive bytes using
 * radioComRxAvailable() and radioComRxReceiveByte().
 *
 * When this bit is 0, the higher-level code does not have to call radioComRxControlSignals()
 * regularly.
 *
 * We recommend that if you call radioComRxControlSignals(), you should call it regularly
 * and set #radioComRxEnforceOrdering to 1 at the beginning of your program.  If you are not
 * using the control signals, you should leave this bit at 0. */
extern BIT radioComRxEnforceOrdering;

/*! \return The number of bytes in the RX buffer.
 *
 * You can use this function to see if any bytes have been received, and then
 * use radioComRxReceiveByte() to actually get the byte and process it. */
uint8 radioComRxAvailable(void);

/*! \return A byte from the RX buffer.
 *
 * Bytes are returned in the order they were received from the other Wixel.
 *
 * This is a non-blocking function: you must call radioComRxAvailable() before calling
 * this function and be sure not to read too many bytes.
 * The number of times you call this should not exceed the last value returned by
 * radioComRxAvailable(). */
uint8 radioComRxReceiveByte(void);

/*! This function must be called regularly if you want to send data
 * or control signals to the other Wixel. */
void radioComTxService(void);

/*! \return The number of bytes available in the TX buffer. */
uint8 radioComTxAvailable(void);

/*! Adds a byte to the TX buffer, which means it will be eventually
 * sent to the other Wixel over the radio.
 *
 * \param byte  The byte to send.
 *
 * This is a non-blocking function: you must call radioComTxAvailable() before calling this
 * function and be sure not to add too many bytes to the buffer.  The number of times you call
 * this should not exceed the last value returned by radioComTxAvailable().
 *
 * If you call this function, you must also call radioComTxService() regularly. */
void radioComTxSendByte(uint8 byte);

/*! \param controlSignals The state of the eight virtual TX control signals.
 *   Each bit represents a different control signal.
 *
 * The values of these eight control lines are transmitted to the
 * other Wixel.
 *
 * Note that these signals (the TX signals) are independent of the
 * signals received from the other Wixel (the RX signals).
 *
 * The meaning of these control signals (e.g. their mapping to RS-232 control
 * signals) is determined by higher-level code. */
void radioComTxControlSignals(uint8 controlSignals);

/*! \return The state of the eight virtual RX control signals.
 *   Each bit represents a different control signal.
 *
 * The values of these eight control lines are zero by default,
 * but they can be set wirelessly by the other Wixel.
 *
 * Note that these signals (the RX signals) are independent of the
 * signals transmitted to other Wixel (the TX signals).
 *
 * The meaning of these control signals (e.g. their mapping to RS-232 control
 * signals) is determined by higher-level code. */
uint8 radioComRxControlSignals(void);

#endif /* RADIO_COM_H_ */
