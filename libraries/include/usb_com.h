/*! \file usb_com.h
 * The <code>usb_com.lib</code> library implements a virtual COM/serial port
 * over USB using the CDC ACM class.  See also com.h.
 */

#ifndef _USB_COM_H
#define _USB_COM_H

#include <time.h>
#include <com.h>

typedef void (HandlerFunction)(void);

/*! This function returns the current state of the virtual
 * RTS and DTR control lines.
 * The states of these lines are controlled by the USB Host.
 *
 * The default values are RTS=0 and DTR=0.  In Windows, when a typical
 * terminal program opens the COM port, both RTS and DTR go to 1.  When
 * the program closes the port, DTR goes to 0 but RTS remains at 1.
 *
 * The bits of this byte are defined in com.h:
 * - Bit 0: #ACM_CONTROL_LINE_DTR
 * - Bit 1: #ACM_CONTROL_LINE_RTS
 * - Bit 2-7: Reserved (should always be zero)
 *
 * Example:
\code
if (usbComRxControlSignals() & ACM_CONTROL_LINE_DTR)
{
    // DTR is 1, which traditionally means the DTE (host) is present.
}
else
{
    // DTR is 0, which traditionally means the DTE (host) is not present.
}
if (usbComRxControlSignals() & ACM_CONTROL_LINE_RTS)
{
    // RTS is 1, which traditionally means "Activate carrier" or
    // tells DCE (device) to prepare to accept data from DTE (host).
}
else
{
    // RTS is 0, which traditionally means "Deactivate carrier".
}
\endcode
 */
uint8 usbComRxControlSignals(void);

/*! Sets the state of the virtual CD and DSR control lines.
 * The value of these lines control lines are reported back to the
 * USB host.
 *
 * The valid bits of the \p signals parameter are defined in com.h:
 * - Bit 0:   #ACM_SERIAL_STATE_RX_CARRIER, a.k.a. CD or CdHolding
 * - Bit 1:   #ACM_SERIAL_STATE_TX_CARRIER, a.k.a. DSR or DsrHolding
 * - Bit 2-7: Reserved (should be zero).
 * */
void usbComTxControlSignals(uint8 signals);

/*! Allows you to report certain events to the USB host.
 * Unlike CD and DSR, which represent the state of a control line,
 * these represent events that happen at a particular
 * time.
 *
 * The valid bits of the \p signalEvents parameter are defined in com.h:
 * - Bit 2: #ACM_SERIAL_STATE_BREAK
 * - Bit 3: #ACM_SERIAL_STATE_RING_SIGNAL
 * - Bit 4: #ACM_SERIAL_STATE_FRAMING
 * - Bit 5: #ACM_SERIAL_STATE_PARITY
 * - Bit 6: #ACM_SERIAL_STATE_OVERRUN
 * - Bits 0, 1, and 7: Reserved (should be zero).
 *
 * You can report multiple events with one call to this function.
 *
 * Example use:
\code
if (uart0RxParityErrorOccurred)
{
    // A parity error occurred on UART 1.
    uart0RxParityErrorOccurred = 0;  // Clear the flag.
    usbComTxControlSignalEvents(ACM_SERIAL_STATE_PARITY);  // Report it to the USB host.
}
\endcode
 */
void usbComTxControlSignalEvents(uint8 signalEvents);

/*! The current line coding.  This includes information such as the desired baud rate,
 * and is controlled by the USB host. */
extern ACM_LINE_CODING XDATA usbComLineCoding;

/*! A pointer to a function that will be called whenever #usbComLineCoding gets set
 * by the USB host. */
extern HandlerFunction * usbComLineCodingChangeHandler;

/*! This function should be called regularly (at least every 50&nbsp;ms) if you are
 * using this library.
 * One of the things this function does is call usbPoll(). */
void usbComService(void);    // This should be called regularly.

/*! \return The number of bytes in the RX buffer that can be received
 *   immediately.
 *
 * You can use this function to see if any bytes have been received, and then
 * use usbComRxReceiveByte() to actually get the byte and process it.
 *
 * The return value of this function might be lower than the actual number of
 * bytes that the USB host is trying to send.
 * Higher-level code should not count on the return value of this function
 * reaching anything higher than 1. */
uint8 usbComRxAvailable(void);

/*! \return A byte from the RX buffer.
 *
 * Bytes are returned in the order they were received from the USB host.
 *
 * This is a non-blocking function: you must call usbComRxAvailable() before calling
 * this function and be sure not to read too many bytes.
 * The number of times you call this should not exceed the last value returned by
 * usbComRxAvailable(). */
uint8 usbComRxReceiveByte(void);

/*! Reads the specified number of bytes from USB and stores them in memory.
 *
 * \param buffer The buffer to store the data in.
 * \param size The number of bytes to read.
 *
 * This is a non-blocking function: you must call usbComRxAvailable() before calling
 * this function and be sure not to read too many bytes.
 * The \p size parameter should not exceed the last value returned by
 * usbComRxAvailable().
 *
 * See also usbComRxReceiveByte(). */
void usbComRxReceive(const uint8 XDATA * buffer, uint8 size);

/*! \return The number of bytes available in the TX buffers.
 *
 * The <code>usb_cdc_acm.lib</code> library uses a double-buffered endpoint
 * with 64-byte buffers, so if the USB host keeps reading data from the device
 * then this function will eventually return 128. */
uint8 usbComTxAvailable(void);

/*! Adds a byte to the TX buffer, which means it will be eventually
 * sent to the USB host.
 *
 * \param byte  The byte to send.
 *
 * This is a non-blocking function: you must call usbComTxAvailable() before calling this
 * function and be sure not to add too many bytes to the buffer.  The number of times you call
 * this should not exceed the last value returned by usbComTxAvailable(). */
void usbComTxSendByte(uint8 byte);

/*! Adds bytes to the TX buffers, which means they will be eventually
 * sent to the USB host.
 *
 * \param buffer A pointer to the bytes to send.
 * \param size The number of bytes to send.
 *
 * This is a non-blocking function: you must call usbComTxAvailable() before calling this
 * function and be sure not to add too many bytes to the buffer.
 * The \p size parameter should not exceed the last value returned by usbComTxAvailable(). */
void usbComTxSend(const uint8 XDATA * buffer, uint8 size);

#endif
