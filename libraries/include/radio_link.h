#ifndef _RADIO_LINK
#define _RADIO_LINK

#include "cc2511_types.h"
#include "radio_mac.h"

// TODO: isolate the higher level code from these offsets and header lengths.

// Each packet can contain at most 18 bytes of payload.
#define RADIO_LINK_MAX_PACKET_SIZE  18

// The link layer will add a one byte header to the beginning of each packet.
#define RADIO_LINK_PACKET_HEADER_LENGTH 1

// Compute the max size of on-the-air packets.  This value is stored in the PKTLEN register.
#define RADIO_MAX_PACKET_SIZE  (RADIO_LINK_MAX_PACKET_SIZE + RADIO_LINK_PACKET_HEADER_LENGTH)

// When a packet is manipulated, you should use these offsets to access the length of
// the packet and the data in the packet.
#define RADIO_LINK_PACKET_LENGTH_OFFSET  0
#define RADIO_LINK_PACKET_DATA_OFFSET    (RADIO_LINK_PACKET_HEADER_LENGTH + 1)

void radioLinkInit(void);

uint8 radioLinkTxAvailable(void);
uint8 XDATA * radioLinkTxCurrentPacket(void);  // returns 0 if no packet is available
void radioLinkTxSendPacket(uint8 size);        // max size is RADIO_LINK_MAX_PACKET_SIZE

uint8 XDATA * radioLinkRxCurrentPacket(void);  // returns 0 if no packet is available.
void radioLinkRxDoneWithPacket(void);

#endif
