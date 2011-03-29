#include <radio_link.h>
#include <radio_com.h>

#define PAYLOAD_TYPE_DATA 0
#define PAYLOAD_TYPE_CONTROL_SIGNALS 1

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

// Assumption: The user recently called radioComRxAvailable and it returned
// a non-zero value.
uint8 radioComRxReceiveByte(void)
{
    uint8 tmp = *rxPointer;   // Read a byte from the current RX packet.
    rxPointer++;              // Update pointer and counter.
    rxBytesLeft--;

    if (rxBytesLeft == 0)     // If there are no bytes left in this packet...
    {
        radioLinkRxDoneWithPacket();  // Tell the radio link layer we are done with it so we can receive more.
    }

    return tmp;
}

static void radioComSendPacketNow()
{
    *packetPointer = txBytesLoaded;
    radioLinkTxSendPacket(PAYLOAD_TYPE_DATA);
    txBytesLoaded = 0;
}

void radioComTxService(void)
{
    if (txBytesLoaded != 0 && radioLinkTxQueued() <= TX_QUEUE_THRESHOLD)
    {
        radioComSendPacketNow();
    }
}


uint8 radioComTxAvailable(void)
{
    // Assumption: If txBytesLoaded is non-zero, radioLinkTxAvailable will be non-zero,
    // so the subtraction below does not overflow.
    // Assumption: The multiplication below does not overflow ever.
    return radioLinkTxAvailable()*RADIO_LINK_PAYLOAD_SIZE - txBytesLoaded;
}

void radioComTxSendByte(uint8 byte)
{
    // Assumption: The user called radioComTxAvailable recently and it returned a non-zero value.
    if (txBytesLoaded == 0)
    {
        txPointer = packetPointer = radioLinkTxCurrentPacket();
    }

    txPointer++;
    *txPointer = byte;
    txBytesLoaded++;

    if (txBytesLoaded == RADIO_LINK_PAYLOAD_SIZE)
    {
        radioComSendPacketNow();
    }
}

void radioComTxSend(const uint8 XDATA * buffer, uint8 size);
