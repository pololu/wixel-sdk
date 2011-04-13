/*! \file com.h
 * Contains common code that is needed by usb_com.h, uart1.h, and uart0.h.
 *
 * The ACM_SERIAL_STATE_* defines all come from Table 31 of PSTN specification.
 * They serve to define the bits used in
 * usbComTxSignals() (see usb_com.h).
 *
 * The ACM_CONTROL_LINE_* defines all from the Table 18 of the PSTN specification.
 * They serve to define the bits returned by usbComRxSignals() (see usb_com.h).
 *
 * PSTN is a subclass of the USB CDC Device Class.  You can find the specification
 * of PSTN in PSTN120.pdf, available for download from USB Implementers Forum at
 * this url: http://www.usb.org/developers/devclass_docs
 *
 */

#ifndef _COM_H
#define _COM_H

/** UART State Bit Values from PSTN 1.20 Table 31. ****************************/

/*! State of receiver carrier detection mechanism of device.
 * Also known as CD or CdHolding. */
#define ACM_SERIAL_STATE_RX_CARRIER  (1<<0)

/*! State of transmission carrier.  Also known as DSR or DsrHolding. */
#define ACM_SERIAL_STATE_TX_CARRIER  (1<<1)

/*! State of break detection mechanism of the device. */
#define ACM_SERIAL_STATE_BREAK       (1<<2)

/*! State of ring signal detection of the device. */
#define ACM_SERIAL_STATE_RING_SIGNAL (1<<3)

/*! A framing error has occurred. */
#define ACM_SERIAL_STATE_FRAMING     (1<<4)

/*! A parity error has occurred. */
#define ACM_SERIAL_STATE_PARITY      (1<<5)

/*! Received data has been discarded due to overrun in the device. */
#define ACM_SERIAL_STATE_OVERRUN     (1<<6)

/*! These are the "irregular" signals, described in PSTN 1.20 Section 6.5.4.
 * These bits represent events that can be reported to the USB host.
 * They are more like interrupts than I/O lines.
 * See the usbComTxControlSignals() function in usb_com.h for more information. */
#define ACM_IRREGULAR_SIGNAL_MASK  (ACM_SERIAL_STATE_BREAK | ACM_SERIAL_STATE_RING_SIGNAL | ACM_SERIAL_STATE_FRAMING | ACM_SERIAL_STATE_PARITY | ACM_SERIAL_STATE_OVERRUN)

/*! Indicates to the CDE if DTE is present or not.
 * - 0 = Not Present
 * - 1 = Present
 * */
#define ACM_CONTROL_LINE_DTR 1

/*! Carrier control for half duplex modems.
 * - 0 = Deactivate carrier.
 * - 1 = Activate carrier. */
#define ACM_CONTROL_LINE_RTS 2

/*! Specifies the type of coding to use on an asynchronous serial line.
 *
 * This struct is defined in PSTN120.pdf in Table 17: Line Coding Structure.
 * PSTN120.pdf is available for download from USB Implementers Forum at
 * this url: http://www.usb.org/developers/devclass_docs */
typedef struct ACM_LINE_CODING
{
    /*! Baud rate, in bits per second. */
    unsigned long dwDTERate;

    /*! The number of stop bits.  Valid values are #STOP_BITS_1,
     * #STOP_BITS_1_5, and #STOP_BITS_2. */
    unsigned char bCharFormat;

    /*! The parity type.  Valid values are #PARITY_NONE, #PARITY_ODD,
     * #PARITY_EVEN, #PARITY_MARK, and #PARITY_SPACE.*/
    unsigned char bParityType;

    /*! The number of data bits in each byte.  Valid values
     * are 5, 6, 7, 8 and 16. */
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
 * the UART library may fail to detect framing errors
 * from the second stop bit when this option is used.
 * */
#define STOP_BITS_2     2

#endif
