/*! \file radio_queue.h
 * <code>radio_queue.lib</code> is a library that provides a simple mechanism
 * for queuing the transmission and reception of RF packets on a device. It
 * does not ensure reliability, nor does it specify a format for the packet
 * contents, other than requiring the first byte of the packet to contain its
 * length. This library depends on <code>radio_mac.lib</code>.
 */

#ifndef _RADIO_QUEUE
#define _RADIO_QUEUE

#include <cc2511_types.h>
#include <radio_mac.h>

/*! Each packet can contain at most 19 bytes of payload. (This was chosen to
 * match radio_link's 18-byte payload + 1-byte header.) */
#define RADIO_QUEUE_PAYLOAD_SIZE 19

/*! Defines the frequency to use.  Valid values are from
 * 0 to 255.  To avoid interference, the channel numbers of
 * different Wixel pairs operating in the should be at least
 * 2 apart.  (This is a Wixel App parameter; the user can set
 * it using the Wixel Configuration Utility.)
 */
extern int32 CODE param_radio_channel;

/*! If this variable is set to 1, received packets will be added to the RX queue
 * even if they have CRC errors. This means that you will get corrupt and
 * spurious data in addition to good data, but it can be useful for applications
 * such as an RF packet sniffer. This variable has a value of 0 by default.
 */
extern BIT radioQueueAllowCrcErrors;

/*! Initializes the radio_queue library and the lower-level
 *  libraries that radio_queue depends on.  This must be called before
 *  any other functions in the library. */
void radioQueueInit(void);

/*! \return The number of radio packet buffers that are currently free
 * (available to hold data).
 *
 * This function has no side effects. */
uint8 radioQueueTxAvailable(void);

/*! \return The number of radio packet buffers that are currently busy
 * (holding a data packet that has not been successfully sent yet).
 *
 * This function has no side effects. */
uint8 radioQueueTxQueued(void);

/*! Returns a pointer to the current packet, or 0 if no packet is available.
 * This function has no side effects.  To populate this packet, you should
 * write the length of the payload data (which must not exceed
 * RADIO_QUEUE_PAYLOAD_SIZE) to offset 0, and write the data starting at
 * offset 1.  After you have put this data in the packet, call
 * radioQueueTxSendPacket() to actually queue the packet up to be sent on
 * the radio.
 * Example usage:
 * <pre>
 *   uint8 XDATA * packet = radioQueueTxCurrentPacket();
 *   if (packet != 0)
 *   {
 *       packet[0] = 3;   // must not exceed RADIO_QUEUE_PAYLOAD_SIZE
 *       packet[1] = 'a';
 *       packet[2] = 'b';
 *       packet[3] = 'c';
 *       radioQueueTxSendPacket();
 *   }
 * </pre>
 */
uint8 XDATA * radioQueueTxCurrentPacket(void);

/*! Sends the current TX packet.  See the documentation of
 * radioQueueTxCurrentPacket() for details.
 */
void radioQueueTxSendPacket(void);

/*! Returns a pointer to the current RX packet (the earliest packet received
 * by radio_queue which has not been processed yet by higher-level code).
 * Returns 0 if there is no RX packet available.
 * The RX packet has the same format as the TX packet: the length of the
 * payload is at offset 0 and the data starts at offset 1.  When you are
 * done reading the packet, you should call radioQueueRxDoneWithPacket().
 * This frees the current packet buffer so it can receive another packet.
 */
uint8 XDATA * radioQueueRxCurrentPacket(void);  // returns 0 if no packet is available.

/*! Frees the current RX packet so that you can advance to processing
 * the next one.  See the radioQueueRxCurrentPacket() documentation for details. */
void radioQueueRxDoneWithPacket(void);

#endif
