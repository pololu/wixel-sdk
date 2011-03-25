#include "repeater_radio_link.h"
#include "repeater_radio_com.h"

#include <string.h>

static uint8 DATA txBytesLoaded = 0;
static uint8 DATA rxBytesLeft = 0;

static uint8 XDATA * DATA rxPointer = 0;
static uint8 XDATA * DATA txPointer = 0;
static uint8 XDATA * DATA packetPointer = 0;

// For highest throughput, we want to send as much data in each packet
// as possible.  But for lower latency, we sometimes need to send packets
// that are NOT full.
// This library will only send non-full packets if the number of packets
// currently queued to be sent is small.  Specifically, that number must
// not exceed TX_QUEUE_THRESHOLD.
// A higher threshold means that there will be more under-populated packets
// at the beginning of a data transfer (which is bad), but slightly reduces
// the importance of calling radioComTxService often (which can be good).
#define TX_QUEUE_THRESHOLD  1

void radioComInit()
{
    radioLinkInit();
}

// NOTE: This function only returns the number of bytes available in the CURRENT PACKET.
// It doesn't look at all the packets received, and it doesn't count the data that is
// queued on the other Wixel.  Therefore, it is never recommended to write some kind of
// program that waits for radioComRxAvailable to reach some value greater than 1: it might
// never reach that value.
uint8 radioComRxAvailable(void)
{
    if (rxBytesLeft != 0)
    {
        return rxBytesLeft;
    }

    rxPointer = radioLinkRxCurrentPacket();

    if (rxPointer == 0)
    {
        return 0;
    }

    rxBytesLeft = rxPointer[0];  // Read the packet length.
    rxPointer += 1;              // Make rxPointer point to the data.

    // Assumption: radioLink doesn't ever return zero-length packets,
    // so rxBytesLeft is non-zero now and we don't have to worry about
    // discard zero-length packets in radio_com.c.

    return rxBytesLeft;
}

// Reads up to <size> bytes from current packet into buffer and discards the
// rest. Returns the number of bytes actually read.
// Assumption: The user recently called radioComRxAvailable and it returned
// a non-zero value.
uint8 radioComRxReceivePacket(uint8 XDATA * buffer, uint8 size)
{
    if (size > rxBytesLeft)
        size = rxBytesLeft;

    memcpy(buffer, rxPointer, size);
    rxBytesLeft = 0;

    radioLinkRxDoneWithPacket();

    return size;
}

static void radioComSendPacketNow()
{
    *packetPointer = txBytesLoaded;
    radioLinkTxSendPacket();
    txBytesLoaded = 0;
}

uint8 radioComTxAvailable(void)
{
    // Assumption: If txBytesLoaded is non-zero, radioLinkTxAvailable will be non-zero,
    // so the subtraction below does not overflow.
    // Assumption: The multiplication below does not overflow ever.
    return radioLinkTxAvailable() * RADIO_LINK_PAYLOAD_SIZE - txBytesLoaded;
}

// Copies up to <size> bytes from buffer and sends packet immediately. Returns
// the number of bytes actually sent.
// Assumption: The user called radioComTxAvailable recently and it returned a non-zero value.
uint8 radioComTxSendPacket(const uint8 XDATA * buffer, uint8 size)
{
    if (size > (RADIO_LINK_PAYLOAD_SIZE - txBytesLoaded))
        size = RADIO_LINK_PAYLOAD_SIZE - txBytesLoaded;

    if (txBytesLoaded == 0)
    {
        txPointer = packetPointer = radioLinkTxCurrentPacket();
    }

    memcpy(txPointer + 1, buffer, size);
    txPointer += size;
    txBytesLoaded += size;

    radioComSendPacketNow();

    return size;
}
