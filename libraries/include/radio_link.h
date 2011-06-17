/*! \file radio_link.h
 * The <code>radio_link.lib</code> library provides reliable, ordered delivery
 * and reception of a series of data packets between two Wixels on the same frequency.
 * This is the library that takes
 * care of Ping/ACK/NAK packets and handles the details of timing.
 * This library defines radio packet memory buffers and controls access to those
 * buffers.
 *
 * This library does not work if there are more than two Wixels broadcasting
 * on the same channel.
 * For wireless communication between more than two Wixels, you can use
 * <code>radio_queue.lib</code> (see radio_queue.h).
 *
 * Similarly, this library also restricts the Wixels to only having one logical data pipe.
 * If you want to send some extra data that doesn't get NAKed, or gets NAKed at
 * different times then the regular data, you would need to replace this library with
 * something more complicated that keeps track of different streams and schedules them.
 *
 * This library depends on <code>radio_mac.lib</code>, which uses an interrupt.
 * For this library to work, you must write
 * <code>include <radio_link.h></code>
 * in the source file that contains your main() function.
 */

#ifndef _RADIO_LINK
#define _RADIO_LINK

#include <cc2511_types.h>
#include <radio_mac.h>

/*! Each packet can contain at most 18 bytes of payload.
 * This limit is imposed by the <code>radio_link.lib</code> library,
 * not the CC2511. */
#define RADIO_LINK_PAYLOAD_SIZE 18

/*! Each packet has a "Payload Type" attached to it,
 * which is a number between 0 and #RADIO_LINK_MAX_PAYLOAD_TYPE.
 * The meanings of the different payload types can be defined by
 * higher-level code. */
#define RADIO_LINK_MAX_PAYLOAD_TYPE 15

/*! Defines the frequency to use.  Valid values are from
 * 0 to 255.  To avoid interference, the channel numbers of
 * different Wixel pairs operating in the should be at least
 * 2 apart.  (This is a Wixel App parameter; the user can set
 * it using the Wixel Configuration Utility.) */
extern int32 CODE param_radio_channel;

/*! This bit allows the higher-level code to detect when a reset packet
 * is received.  It is set to 1 in an interrupt by the <code>radio_link.lib</code> library
 * whenever a reset packet is received.  The higher-level code should set
 * it to zero when it uses this information.
 *
 * A reset packet will be received whenever the other Wixel is reset for
 * any reason.  Multiple reset packets can also be received in quick
 * succession if the other device does not receive the acknowledgment
 * packet sent by this device (every radio packet has a chance of being
 * lost).  This situation is indistinguishable from the situation where
 * the other party is actually getting reset several times in quick
 * succession.
 *
 * If the higher-level code responds to this bit by sending an initialization
 * packet to the other device, then it should clear #radioLinkResetPacketReceived
 * BEFORE queueing the packet to be sent.  Otherwise, the following
 * sequence of events could occur:
 * -# Main Loop: This device detects a reset packet (#radioLinkResetPacketReceived==1).
 * -# Main Loop: This device queues initialization packet to be sent.
 * -# ISR: This device sends an initialization packet.
 * -# ISR: This device receives a reset packet because other device got reset.
 * -# This device clears the #radioLinkResetPacketReceived.
 *
 * By clearing the bit first, you guarantee that an initialization packet will
 * be sent AFTER the final reset packet is received.
 *
 * Example use:
 \code
 if (radioLinkResetPacketReceived && radioLinkTxAvailable())
 {
     uint8 XDATA * packet;

     // A reset packet from the other party was received, so send
     // a packet to initialize the connection.

     // Clear the flag.  This must be done BEFORE queueing the packet
     // to be sent, as discussed in radio_link.h
     radioLinkResetPacketReceived = 0;

     packet = radioLinkTxAvailable();
     packet[0] = INIT_PACKET_LENGTH;  // defined by higher-level code
     packet[1] = INIT_PACKET_PAYLOAD; // defined by higher-level code
     radioLinkTxSendPacket(INIT_PACKET_PAYLOAD_TYPE);
 }
 \endcode
 */
extern volatile BIT radioLinkResetPacketReceived;

/*! Initializes the <code>radio_link.lib</code> library and the lower-level
 *  libraries that it depends on.  This must be called before
 *  any other functions in the library. */
void radioLinkInit(void);

/*! \return The number of radio TX packet buffers that are currently free
 * (available to hold data). */
uint8 radioLinkTxAvailable(void);

/*! \return The number of radio TX packet buffers that are currently busy
 * (holding a data packet that has not been successfully sent yet). */
uint8 radioLinkTxQueued(void);

/*! \return A pointer to the current TX packet, or 0 if no packet is available.
 *
 * To populate this packet, you should
 * write the length of the payload data (which must not exceed
 * #RADIO_LINK_PAYLOAD_SIZE) to offset 0, and write the data starting at
 * offset 1.  After you have put this data in the packet, call
 * radioLinkTxSendPacket() to actually queue the packet up to be sent on
 * the radio.
 * Example usage:
\code
uint8 XDATA * packet = radioLinkTxCurrentPacket();
if (packet != 0)
{
    packet[0] = 3;   // payload length.  Must not exceed RADIO_LINK_PAYLOAD_SIZE.
    packet[1] = 'a';
    packet[2] = 'b';
    packet[3] = 'c';
    radioLinkTxSendPacket(0);
}
\endcode
 *
 * This function has no side effects.
 */
uint8 XDATA * radioLinkTxCurrentPacket(void);

/*! Sends the current TX packet.  See the documentation of
 * radioLinkTxCurrentPacket() for details.
 *
 * \param payloadType A number between 0 and RADIO_LINK_MAX_PACKET_TYPE that
 * will be attached to the packet.  This can be used to specify what type of
 * data the packet contains.
 * */
void radioLinkTxSendPacket(uint8 payloadType);

/*! \return A pointer to the current RX packet.
 *   This is the earliest packet received from the other Wixel
 *   which has not yet been processed yet by higher-level code.
 *   Returns 0 if there is no RX packet available.
 *
 * The RX packet has the same format as the TX packet: the length of the
 * payload is at offset 0 and the data starts at offset 1.
 *
 * When you are done reading the packet you should call
 * radioLinkRxDoneWithPacket() to advance to the next packet.
 */
uint8 XDATA * radioLinkRxCurrentPacket(void);  // returns 0 if no packet is available.

/*! \return The payload type of the current RX packet.  This number was the
 *   argument to radioLinkTxSendPacket() on the other Wixel.
 *
 * This should only be called if radioLinkRxCurrentPacket() recently returned
 * a non-zero pointer. */
uint8 radioLinkRxCurrentPayloadType(void);

/*! Frees the current RX packet so that you can advance to processing
 * the next one.  See the radioLinkRxCurrentPacket() documentation for details. */
void radioLinkRxDoneWithPacket(void);

/*! \return 1 if a connection to another Wixel has been established.
 *
 * Currently the radio_link library does not detect disconnection, so this value
 * will never change from 1 to 0. */
BIT radioLinkConnected(void);

/*! The library will set this bit to 1 whenever it receives a packet that
 * has payload data in it or sends a packet.
 * Higher-level code may check this bit and clear it. */
extern volatile BIT radioLinkActivityOccurred;

#endif
