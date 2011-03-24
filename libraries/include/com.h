/*! \file com.h
 * Contains common code that is needed by usb_com.h, uart1.h, and uart0.h.
 */

#ifndef _COM_H
#define _COM_H

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

/*! No parity. Each serial byte will only have 8 bits. */
#define PARITY_NONE 0

/*! Odd parity.  The total number of data bits that are 1 will be odd. */
#define PARITY_ODD 1

/*! Even parity. The total number of data bits that are 1 will be even. */
#define PARITY_EVEN 2

/*! Mark parity.  Ninth bit of each byte will be logical 1. */
#define PARITY_MARK 3

/*! Mark parity.  Ninth bit of each byte will be logical 0. */
#define PARITY_SPACE 4

/*! The end of each byte will have 1 stop bit. */
#define STOP_BITS_1     0

/*! The end of each serial byte will have 1.5 stop bits.
 * The CC2511's UARTs do not actually support this option. */
#define STOP_BITS_1_5   1

/*! The end of each serial byte will have 2 stop bits.
 * The CC2511's UARTs do not support this option very well;
 * our the uart library may fail to detect framing errors
 * from the second stop bit when this option is used.
 * */
#define STOP_BITS_2     2

#endif
