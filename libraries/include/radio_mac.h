/*! \file radio_mac.h
 * The <code>radio_mac.lib</code> library takes care of sending and
 * receiving packets.
 * It handles the details of setting up the radio interrupt and
 * DMA configuration.
 * It uses <code>radio_registers.lib</code> to configure the radio.
 *
 * When a radio-related event happens, <code>radio_mac.lib</code>
 * reports the event to higher-level code by calling radioMacEventHandler()
 * in an ISR.  The higher-level code can then decide what to do next by
 * calling radioMacTx() or radioMacRx() from the event handler.
 *
 * This library does not currently support any way of turning off the
 * radio to save power.
 *
 * This library defines an ISR, so radio_mac.h must be included in the
 * file that defines main() in order for this library to work.
 *
 * This library uses an interrupt.
 * For this library to work, you must write
 * <code>include <radio_mac.h></code>
 * in the source file that contains your main() function.
 */

#ifndef _RADIO_MAC_H_
#define _RADIO_MAC_H_

#include <cc2511_map.h>
#include <cc2511_types.h>

/*! See the documentation for radioMacEventHandler(). */
#define RADIO_MAC_EVENT_TX                  30
/*! See the documentation for radioMacEventHandler(). */
#define RADIO_MAC_EVENT_RX                  31
/*! See the documentation for radioMacEventHandler(). */
#define RADIO_MAC_EVENT_RX_TIMEOUT          32
/*! See the documentation for radioMacEventHandler(). */
#define RADIO_MAC_EVENT_STROBE              33

/*! Initializes the radio.
 * This involves calling radioRegistersInit().
 * This should be called before any other radioMac functions are called. */
void radioMacInit(void);

/*! Forces the radioMacEventHandler() to run soon.
 *
 * This function triggers an artificial radio interrupt.
 * If the radio is not in the middle of transmitting, receiving,
 * or waiting for a packet with a short RX timeout, then
 * radioMacEventHandler() will be called with an argument of
 * RADIO_MAC_EVENT_STROBE.
 * Otherwise, radioMacStrobe will be called soon with a different
 * argument.
 *
 * The idea behind this function is that higher-level code running
 * in the main loop would call it whenever it has placed new
 * data into a buffer, in order to wake up the interrupt-based
 * code to so it can use the new data. */
void radioMacStrobe(void);

/*! Sets up the radio to transmit a packet.
 *
 * \param packet A pointer to the packet to transmit.
 *   The first byte (packet[0]) should be the length of the payload in bytes.
 *   The payload should start at packet[1].
 *
 * This function will only work if it is called from radioMacEventHandler(). */
void radioMacTx(uint8 XDATA * packet);

/*! Sets up the radio to receive a packet.
 *
 * \param packet A pointer to the location to store the packet.
 * \param timeout The timeout period, in units of 1 ms.
 *   If a packet has not been received in this time, then a
 *   RADIO_MAC_EVENT_RX_TIMEOUT event will happen.
 *   Set this parameter to 0 to disable the timeout.
 *
 * Later, after the packet has been received, packet[0] will contain the
 * length of the packet payload in bytes, and the payload will
 * start at packet[1].
 * Also, two status bytes will be appended at the end of the packet
 * as described in Tables 64 and 65 of the CC2511 datasheet.
 */
void radioMacRx(uint8 XDATA * packet, uint8 timeout);

/*! This is a callback function that should be defined by higher-level code.
 *
 * This function is called in the RF ISR whenever a radio-related event happens.
 * This function should decide what the radio will do next by calling either
 * radioMacTx() or radioMacRx().
 *
 * \param event The event that just happened.  This will be one of:
 * - #RADIO_MAC_EVENT_TX: The radio just finished transmitting a packet.
 * - #RADIO_MAC_EVENT_RX: The radio just finished receiving a packet.
 * - #RADIO_MAC_EVENT_RX_TIMEOUT: The radio was listening for a packet and
 *   nothing was received within the timeout period.
 * - #RADIO_MAC_EVENT_STROBE: The function radioMacStrobe() was called.
 *
 * Note: Not every call to radioMacStrobe() results in a call to
 * radioMacEventHandler with argument RADIO_MAC_EVENT_STROBE.
 *
 * See radio_queue.c or radio_link.c for examples of how to define
 * radioMacEventHandler().
 */
void radioMacEventHandler(uint8 event);

/*! The library will set this bit to 1 when an RX overflow occurs.
 *
 * An RX overflow is an error that indicates that incoming data was
 * not read from the radio fast enough.
 * This should not happen.
 */
extern volatile BIT radioRxOverflowOccurred;

/*! The library will set this bit to 1 when a TX underflow occurs.
 *
 * A TX underflow is an error that indicates that outgoing data was
 * not written to the radio fast enough.
 * This should not happen. */
extern volatile BIT radioTxUnderflowOccurred;

/*! The radio's Interrupt Service Routine (ISR). */
ISR(RF, 0);

#endif /* RADIO_H_ */
