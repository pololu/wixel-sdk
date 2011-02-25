#ifndef _RADIO_LINK
#define _RADIO_LINK

#include "cc2511_types.h"
#include "radio_mac.h"

// Each packet can contain at most 18 bytes of payload.
#define RADIO_LINK_PAYLOAD_SIZE 18

// radioLinkInit: Initializes the radio_link library and the lower-level
// libraries that it radio_link depends on.  This must be called before
// any other functions in the library.
void radioLinkInit(void);

// radioLinkTxAvailable: Returns the number of radio packets buffers
// currently available for sending packets.  This function has no
// side effects.
uint8 radioLinkTxAvailable(void);

// radioLinkTxCurrentPacket:
// Returns a pointer to the current packet, or 0 if no packet is available.
// This function has no side effects.  To populate this packet, you should
// write the length of the payload data (which must not exceed
// RADIO_LINK_PAYLOAD_SIZE) to offset 0, and write the data starting at
// offset 1.  After you have put this data in the packet, call
// radioLinkTxSendPacket to actually queue the packet up to be sent on
// the radio.
// Example usage:
//   uint8 XDATA * packet = radioLinkTxCurrentPacket();
//   if (packet != 0)
//   {
//       packet[0] = 3;   // must not exceed RADIO_LINK_PAYLOAD_SIZE
//       packet[1] = 'a';
//       packet[2] = 'b';
//       packet[3] = 'c';
//       radioLinkTxSendPacket();
//   }
uint8 XDATA * radioLinkTxCurrentPacket(void);
void radioLinkTxSendPacket(void);

// radioLinkRxCurrentPacket:
// Returns a pointer to the current RX packet (the earliest packet received
// by radio_link which has not been processed yet by higher-level code).
// Returns 0 if there is no RX packet available.
// The RX packet has the same format as the TX packet: the length of the
// payload is at offset 0 and the data starts at offset 1.  When you are
// done reading the packet, you should call radioLinkRxDoneWithPacket.
// This frees the current packet buffer so it can receive another packet.
uint8 XDATA * radioLinkRxCurrentPacket(void);  // returns 0 if no packet is available.
void radioLinkRxDoneWithPacket(void);

#endif
