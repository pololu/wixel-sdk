/*! \file radio_link.h
 * <code>radio_link.lib</code> is a library provides reliable, ordered delivery
 * and reception of a series of data packets between
 * this device and another on the same frequency.  This is the layer that takes
 * care of Ping/ACK/NAK packets, and handles the details of timing.  This library
 * depends on <code>radio_mac.lib</code>.
 */

#ifndef _RADIO_LINK
#define _RADIO_LINK

#include <cc2511_types.h>
#include <radio_mac.h>

/*! Each packet can contain at most 18 bytes of payload. */
#define RADIO_LINK_PAYLOAD_SIZE 18

/*! Defines the frequency to use.  Valid values are from
 * 0 to 255.  To avoid interference, the channel numbers of
 * different Wixel pairs operating in the should be at least
 * 2 apart.  (This is a Wixel App parameter; the user can set
 * it using the Wixel Configuration Utility.)
 */
extern int32 CODE param_radio_channel;

/*! Initializes the radio_link library and the lower-level
 *  libraries that it radio_link depends on.  This must be called before
 *  any other functions in the library. */
void radioLinkInit(void);

/*! \return The number of radio packet buffers that are currently free
 * (available to hold data).
 *
 * This function has no side effects. */
uint8 radioLinkTxAvailable(void);

/*! \return The number of radio packet buffers that are currently busy
 * (holding a data packet that has not been successfully sent yet).
 *
 * This function has no side effects. */
uint8 radioLinkTxQueued(void);

/*! Returns a pointer to the current packet, or 0 if no packet is available.
 * This function has no side effects.  To populate this packet, you should
 * write the length of the payload data (which must not exceed
 * RADIO_LINK_PAYLOAD_SIZE) to offset 0, and write the data starting at
 * offset 1.  After you have put this data in the packet, call
 * radioLinkTxSendPacket to actually queue the packet up to be sent on
 * the radio.
 * Example usage:
 * <pre>
 *   uint8 XDATA * packet = radioLinkTxCurrentPacket();
 *   if (packet != 0)
 *   {
 *       packet[0] = 3;   // must not exceed RADIO_LINK_PAYLOAD_SIZE
 *       packet[1] = 'a';
 *       packet[2] = 'b';
 *       packet[3] = 'c';
 *       radioLinkTxSendPacket();
 *   }
 * </pre>
 */
uint8 XDATA * radioLinkTxCurrentPacket(void);
void radioLinkTxSendPacket(void);

/*! Returns a pointer to the current RX packet (the earliest packet received
 * by radio_link which has not been processed yet by higher-level code).
 * Returns 0 if there is no RX packet available.
 * The RX packet has the same format as the TX packet: the length of the
 * payload is at offset 0 and the data starts at offset 1.  When you are
 * done reading the packet, you should call radioLinkRxDoneWithPacket.
 * This frees the current packet buffer so it can receive another packet.
 */
uint8 XDATA * radioLinkRxCurrentPacket(void);  // returns 0 if no packet is available.
void radioLinkRxDoneWithPacket(void);

#endif
