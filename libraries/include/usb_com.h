/*! \file usb_com.h
 * The <code>usb_com.lib</code> library implements a virtual COM/serial port
 * over USB using the CDC ACM class.
 */

#ifndef _USB_COM_H
#define _USB_COM_H

#include <com.h>

typedef void (HandlerFunction)(void);

/* Prototypes defined by the CDC ACM module ***********************************/

extern uint8 usbComControlLineState;
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
