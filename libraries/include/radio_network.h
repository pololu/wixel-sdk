/*! \file radio_network.h
 * <code>radio_network.lib</code> is a library that provides a simple mechanism
 * for queuing the transmission and reception of RF packets on a device. It
 * does not ensure reliability.
 *
 * The packet format is:
 * LENGTH (1Byte) NEXT_NODE_ADDRESS(1Byte) DESTINATION ADDRESS(1Byte) SOURCE_ADDRESS(1Byte)
 * 
 * This library depends on <code>radio_mac.lib</code> and <code>radio_addres.lib</code>.
 * 
 * This has been developed during the course of Pervasive Systems (Prof. Fabio Salice)
 * at Politecnico di Milano, Como campus.
 *
 * by Carlo Bernaschina (B3rn475)
 * www.bernaschina.com
 */

#ifndef _RADIO_NETWORK
#define _RADIO_NETWORK

#include <cc2511_types.h>
#include <radio_mac.h>
#include <radio_address.h>
#include <time.h>

/*! Each packet can contain at most 20 bytes of payload.
 */
#define RADIO_NETWORK_PAYLOAD_SIZE 20

/*! Defines the frequency to use.  Valid values are from
 * 0 to 255.  To avoid interference, the channel numbers of
 * different Wixel networks operating in the should be at least
 * 2 apart.  (This is a Wixel App parameter; the user can set
 * it using the Wixel Configuration Utility.)
 */
extern int32 CODE param_radio_channel;

/*! Initializes the radio_queue library and the lower-level
 *  libraries that radio_queue depends on.  This must be called before
 *  any other functions in the library. */
void radioNetworkInit(void);

/*! \return The number of radio packet buffers that are currently free
 * (available to hold data).
 *
 * This function has no side effects. */
uint8 radioNetworkTxAvailable(void);

/*! \return The number of radio packet buffers that are currently busy
 * (holding a data packet that has not been successfully sent yet).
 *
 * This function has no side effects. */
uint8 radioNetworkTxQueued(void);

/*! Returns a pointer to the current packet, or 0 if no packet is available.
 * This function has no side effects.  To populate this packet, you should
 * write the length of the payload data (which must not exceed
 * RADIO_NETWORK_PAYLOAD_SIZE) to offset 0, and write the data starting at
 * offset 1.  After you have put this data in the packet, call
 * radioNetworkTxSendPacket() to actually queue the packet up to be sent on
 * the radio.
 * Example usage:
 * <pre>
 *   uint8 XDATA * packet = radioQueueTxCurrentPacket();
 *   if (packet != 0)
 *   {
 *       packet[0] = 3;   // must not exceed RADIO_NETWORK_PAYLOAD_SIZE
 *       //packet[1] will be fixed by radioNetworkFillAddress() with the address of the next node
 *       packet[2] = destination_address;
 *       //packet[3] will be fixed by radioNetworkFillAddress() with param_address from <code>radio_mac.lib</code>
 *       radioQueueTxSendPacket();
 *   }
 * </pre>
 */
uint8 XDATA * radioNetworkTxCurrentPacket(void);

/*! Sends the current TX packet.  See the documentation of
 * radioNetworkTxCurrentPacket() for details.
 */
void radioNetworkTxSendPacket(void);

/*! Fills the source and next node address.  See the documentation of
 * radioNetworkTxCurrentPacket() for details.
 */
void radioNetworkFillAddress(void);

/*! Returns a pointer to the current RX packet (the earliest packet received
 * by radio_network which has not been processed yet by higher-level code).
 * Returns 0 if there is no RX packet available.
 * The RX packet has the same format as the TX packet: the length of the
 * payload is at offset 0 and the data starts at offset 1.  When you are
 * done reading the packet, you should call radioQueueRxDoneWithPacket().
 * This frees the current packet buffer so it can receive another packet.
 */
uint8 XDATA * radioNetworkRxCurrentPacket(void);  // returns 0 if no packet is available.

/*! Frees the current RX packet so that you can advance to processing
 * the next one.  See the radioQueueRxCurrentPacket() documentation for details. */
void radioNetworkRxDoneWithPacket(void);

#endif
