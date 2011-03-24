/*! \file usb_com.h
 * The <code>usb_com.lib</code> library implements a virtual COM/serial port
 * over USB using the CDC ACM class.  See also com.h.
 */

#ifndef _USB_COM_H
#define _USB_COM_H

#include <com.h>

typedef void (HandlerFunction)(void);

/*! This byte contains the current state of the RTS and DTR control lines.
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
if (usbComControlLineState & ACM_CONTROL_LINE_DTR)
{
    // DTR is 1, which traditionally means the DTE (host) is present.
}
else
{
    // DTR is 0, which traditionally means the DTE (host) is not present.
}
if (usbComControlLineState & ACM_CONTROL_LINE_RTS)
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
extern uint8 usbComControlLineState;

/*! The low byte of the UART State Bitmap defined in Table 31 of PSTN1.20.
 * Application code can set this value whenever it wants.
 * usbComService will take care of sending the value of this variable
 * to the USB host when the device is first enumerated and whenever the
 * value changes.
 *
 * The bits of this byte are defined in com.h:
 * - Bit 0: #ACM_SERIAL_STATE_RX_CARRIER, a.k.a. CD or CdHolding
 * - Bit 1: #ACM_SERIAL_STATE_TX_CARRIER, a.k.a. DSR or DsrHolding
 * - Bit 2: #ACM_SERIAL_STATE_BREAK
 * - Bit 3: #ACM_SERIAL_STATE_RING_SIGNAL
 * - Bit 4: #ACM_SERIAL_STATE_FRAMING
 * - Bit 5: #ACM_SERIAL_STATE_PARITY
 * - Bit 6: #ACM_SERIAL_STATE_OVERRUN
 * - Bit 7: Reserved (should always be zero)
 * */
extern uint8 usbComSerialState;

extern ACM_LINE_CODING XDATA usbComLineCoding;
extern HandlerFunction * usbComLineCodingChangeHandler;

void usbComInit(void);
void usbComService(void);    // This should be called regularly.

uint8 usbComRxAvailable(void);
uint8 usbComRxReceiveByte(void);
void usbComRxReceive(const uint8 XDATA * buffer, uint8 size);

uint8 usbComTxAvailable(void);
void usbComTxSendByte(uint8 byte);
void usbComTxSend(const uint8 XDATA * buffer, uint8 size);

#endif
