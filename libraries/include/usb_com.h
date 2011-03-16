/*! \file usb_com.h
 * The <code>usb_com.lib</code> library implements a virtual COM/serial port
 * over USB using the CDC ACM class.
 */

#ifndef _USB_COM_H
#define _USB_COM_H

// UART State Bit Values from PSTN 1.20 Table 31 .
#define ACM_SERIAL_STATE_RX_CARRIER  (1<<0)  // CD,  CdHolding
#define ACM_SERIAL_STATE_TX_CARRIER  (1<<1)  // DSR, DsrHolding
#define ACM_SERIAL_STATE_BREAK       (1<<2)
#define ACM_SERIAL_STATE_RING_SIGNAL (1<<3)
#define ACM_SERIAL_STATE_FRAMING     (1<<4)
#define ACM_SERIAL_STATE_PARITY      (1<<5)
#define ACM_SERIAL_STATE_OVERRUN     (1<<6)

// PSTN1.20 Table 17: Line Coding Structure
typedef struct ACM_LINE_CODING
{
    unsigned long dwDTERate;
    unsigned char bCharFormat;
    unsigned char bParityType;
    unsigned char bDataBits;
} ACM_LINE_CODING;


/* Prototypes defined by the CDC ACM module ***********************************/

extern uint8 usbComControlLineState;
extern ACM_LINE_CODING XDATA usbComLineCoding;

void usbComInit(void);
void usbComService(void);    // This should be called regularly.

uint8 usbComRxAvailable(void);
uint8 usbComRxReceiveByte(void);
void usbComRxReceive(const uint8 XDATA * buffer, uint8 size);

uint8 usbComTxAvailable(void);
void usbComTxSendByte(uint8 byte);
void usbComTxSend(const uint8 XDATA * buffer, uint8 size);

#endif
