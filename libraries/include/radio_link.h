/*! \file radio_link.h
 * <code>radio_link.lib</code> is a library that provides reliable, ordered delivery
 * and reception of a series of data packets between two Wixels on the same frequency.
 * This is the layer that takes
 * care of Ping/ACK/NAK packets, and handles the details of timing.
 * This layer defines the RF packet memory buffers used, and controls access to those
 * buffers.
 *
 * This layer only supports communication between a pair of Wixels.  If you wanted
 * to make a system/network with more than 2 Wixels, you would have to replace this
 * layer.
 *
 * Similarly, this layer also restricts us to only having one logical data pipe.
 * If you wanted to send some extra data that doesn't get NAKed, or gets NAKed at
 * different times then the regular data, you would need to replace this layer with
 * something more complicated that keeps track of different streams and schedules them.
 *
 * This layer does not correspond cleanly to any of the layers in the OSI Model.
 * It combines portions of the Data Link Layer (#2), Network Layer (#3), and
 * Transport Layer (#4).
 *
 * This library depends on <code>radio_mac.lib</code>.
 */

#ifndef _RADIO_LINK
#define _RADIO_LINK

#include <cc2511_types.h>
#include <radio_mac.h>

/*! Each packet can contain at most 18 bytes of payload. */
#define RADIO_LINK_PAYLOAD_SIZE 18

/*! Each packet can also have a "Payload Type" attached to it,
 * which is a number between 0 and RADIO_LINK_MAX_PAYLOAD_TYPE. */
#define RADIO_LINK_MAX_PAYLOAD_TYPE 15

/*! Defines the frequency to use.  Valid values are from
 * 0 to 255.  To avoid interference, the channel numbers of
 * different Wixel pairs operating in the should be at least
 * 2 apart.  (This is a Wixel App parameter; the user can set
 * it using the Wixel Configuration Utility.) */
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
 *       packet[0] = 3;   // payload length.  Must not exceed RADIO_LINK_PAYLOAD_SIZE.
 *       packet[1] = 'a';
 *       packet[2] = 'b';
 *       packet[3] = 'c';
 *       radioLinkTxSendPacket(0);
 *   }
 * </pre>
 */
uint8 XDATA * radioLinkTxCurrentPacket(void);

/*! Sends the current TX packet.  See radioLinkTxCurrentPacket() for
 * details.
 *
 * \param payloadType A number between 0 and RADIO_LINK_MAX_PACKET_TYPE that
 * will be attached to the packet.  This can be used to specify what type of
 * data the packet contains.
 * */
void radioLinkTxSendPacket(uint8 payloadType);

/*! Returns a pointer to the current RX packet (the earliest packet received
 * by radio_link which has not been processed yet by higher-level code).
 * Returns 0 if there is no RX packet available.
 * The RX packet has the same format as the TX packet: the length of the
 * payload is at offset 0 and the data starts at offset 1.  When you are
 * done reading the packet, you should call radioLinkRxDoneWithPacket().
 * This frees the current packet buffer so it can receive another packet.
 */
uint8 XDATA * radioLinkRxCurrentPacket(void);  // returns 0 if no packet is available.

/*!
 * \return The payload type of the current RX packet.  This number was the
 * argument to radioLinkTxsendPacket() on the other Wixel.
 * This should only be called if radioLinkRxCurrentPacket recently returned
 * a non-zero pointer. */
uint8 radioLinkRxCurrentPayloadType(void);

/*! Frees the current RX packet buffer so that you can move on to processing
 * the next one.  See radioLinkRxCurrentPacket() for details. */
void radioLinkRxDoneWithPacket(void);

#endif
