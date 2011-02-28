/* radio_mac.h:
 * Header file for the Radio MAC (Media Access Control) layer.
 * This layer takes care of sending and receiving packets.
 * Later we will probably add other features to it, such as
 * serial-number MAC addresses.
 */

#ifndef _RADIO_MAC_H_
#define _RADIO_MAC_H_

#include <cc2511_map.h>
#include <cc2511_types.h>

// The units of the "timeout" parameter to radioMacRx, in units of microseconds.
#define RADIO_TIMEOUT_UNIT 25      // It's more like 25.3906.  This is determined by RX_TIME and WOR_RES settings.


// Radio MAC states
#define RADIO_MAC_STATE_OFF      0
#define RADIO_MAC_STATE_IDLE     1
#define RADIO_MAC_STATE_RX       2
#define RADIO_MAC_STATE_TX       3

// Radio MAC events
#define RADIO_MAC_EVENT_TX                  30    // We finished transmitting the packet.
#define RADIO_MAC_EVENT_RX_INSTEAD_OF_TX    31    // We were trying to transmit a packet, but the channel was not clear and we received one instead.
#define RADIO_MAC_EVENT_RX                  32    // We received a packet normally.
#define RADIO_MAC_EVENT_RX_TIMEOUT          33    // We were trying to receive a packet normally, but the timeout period expired.
#define RADIO_MAC_EVENT_STROBE              34    // The user set radioMacStrobe=1 in order to get the ISR to run.

void radioMacInit(void);

void radioMacStrobe(void);        // Forces radioMacEventHandler to run soon.

// Current state of the MAC.
extern volatile uint8 DATA radioMacState;

// State-changing functions.  These should not be called if radioMacState == RADIO_MAC_STATE_TX.
void radioMacTx(uint8 XDATA * packet);
void radioMacRx(uint8 XDATA * packet, uint16 timeout);

// This function is called by the Mac in an ISR when an important event
// happens (and puts the MAC in the idle state).  This function should be
// defined by a higher-level layer.  Ideally this function will call one of the
// state-changing functions immediately, but it could choose to simply record
// data about the event and act on it later.
void radioMacEventHandler(uint8 event);

// Error reporting variables
extern volatile BIT radioRxOverflowOccurred;
extern volatile BIT radioTxUnderflowOccurred;
extern volatile uint8 DATA radioBadMarcState;

ISR(RF, 1);

#endif /* RADIO_H_ */
