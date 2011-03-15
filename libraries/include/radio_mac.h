/*! \file radio_mac.h
 * The <code>radio_mac.lib</code> library takes care of sending and
 * receiving packets.  When a radio-related event happens, radio_mac
 * reports the event to higher-level code by calling radioMacEventHandler
 * in an ISR, and the higher-level code can decide what to do next by
 * calling various radioMac functions declared here.
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
#define RADIO_MAC_EVENT_RX                  31    // We received a packet normally.
#define RADIO_MAC_EVENT_RX_TIMEOUT          32    // We were trying to receive a packet normally, but the timeout period expired.
#define RADIO_MAC_EVENT_STROBE              33    // The user set radioMacStrobe=1 in order to get the ISR to run.

void radioMacInit(void);

void radioMacStrobe(void);        // Forces radioMacEventHandler to run soon.

// Current state of the MAC.
extern volatile uint8 DATA radioMacState;

// State-changing functions.  These should not be called if radioMacState == RADIO_MAC_STATE_TX.
void radioMacTx(uint8 XDATA * packet);
void radioMacRx(uint8 XDATA * packet, uint8 timeout);

/*! This function is called by the Mac in an ISR when an important event
 * happens (and puts the MAC in the idle state).  This function should be
 * defined by a higher-level layer.  Ideally this function will call one of the
 * state-changing functions immediately, but it could choose to simply record
 * data about the event and act on it later.
 */
void radioMacEventHandler(uint8 event);

// Error reporting variables
extern volatile BIT radioRxOverflowOccurred;
extern volatile BIT radioTxUnderflowOccurred;
extern volatile uint8 DATA radioBadMarcState;

ISR(RF, 1);

#endif /* RADIO_H_ */
