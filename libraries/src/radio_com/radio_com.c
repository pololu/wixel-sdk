#include <radio_link.h>
#include <radio_com.h>

#define PAYLOAD_TYPE_DATA 0
#define PAYLOAD_TYPE_CONTROL_SIGNALS 1

BIT radioComRxEnforceOrdering = 0;

static uint8 DATA txBytesLoaded = 0;
static uint8 DATA rxBytesLeft = 0;

static uint8 XDATA * DATA rxPointer = 0;
static uint8 XDATA * DATA txPointer = 0;
static uint8 XDATA * DATA packetPointer = 0;

static uint8 radioComRxSignals = 0;
static uint8 radioComTxSignals = 0;
static uint8 lastRxSignals = 0; // The last RX signals sent to the higher-level code.
static BIT sendSignalsSoon = 0; // 1 iff we should transmit control signals soon

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

/** RX FUNCTIONS **************************************************************/

#define WAITING_TO_REPORT_RX_SIGNALS (radioComRxEnforceOrdering && radioComRxSignals != lastRxSignals)

static void receiveMorePackets(void)
{
    uint8 XDATA * packet;

    if (rxBytesLeft != 0)
    {
        // There are bytes available.  The higher-level code should
        // call radioComRxReceiveByte to get them.
        return;
    }

    if (WAITING_TO_REPORT_RX_SIGNALS)
    {
        // The higher-level code needs to call radioComRxSignals before
        // we feed it any more data.
        return;
    }

    // Each iteration of this loop processes one packet received on the radio.
    // This loop stops when we are out of packets or when we received a packet
    // that contains some information that the higher-level code needs to process.
    while(packet = radioLinkRxCurrentPacket())
    {
        switch(radioLinkRxCurrentPayloadType())
        {
        case PAYLOAD_TYPE_DATA:
            // We received some data.  Populate rxPointer and rxBytesLeft.
            // The data can be retreived with radioComRxAvailable and racioComRxReceiveByte().

            // Assumption: radioLink doesn't ever return zero-length packets,
            // so rxBytesLeft is non-zero now and we don't have to worry about
            // discard zero-length packets in radio_com.c.
            rxBytesLeft = packet[0];  // Read the packet length.
            rxPointer = packet+1;              // Make rxPointer point to the data.
            return;

        case PAYLOAD_TYPE_CONTROL_SIGNALS:
            // We received a command to set the control signals.
            radioComRxSignals = packet[1];

            radioLinkRxDoneWithPacket();

            if (WAITING_TO_REPORT_RX_SIGNALS)
            {
                // The higher-level code has not seen these values for the control
                // signals yet, so stop processing packets.
                // The higher-level code can access these values by calling radioComRxControlSignals().
                return;
            }

            // It was a redundant command so don't do anything special.
            // Keep processing packets.
            break;
        }
    }
}

// NOTE: This function only returns the number of bytes available in the CURRENT PACKET.
// It doesn't look at all the packets received, and it doesn't count the data that is
// queued on the other Wixel.  Therefore, it is never recommended to write some kind of
// program that waits for radioComRxAvailable to reach some value greater than 1: it might
// never reach that value.
uint8 radioComRxAvailable(void)
{
    receiveMorePackets();
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

uint8 radioComRxControlSignals(void)
{
    receiveMorePackets();
    lastRxSignals = radioComRxSignals;
    return lastRxSignals;
}

/** TX FUNCTIONS **************************************************************/

static void radioComSendDataNow()
{
    *packetPointer = txBytesLoaded;
    radioLinkTxSendPacket(PAYLOAD_TYPE_DATA);
    txBytesLoaded = 0;
}

static void radioComSendControlSignalsNow()
{
    // Assumption: txBytesLoaded is 0 (we are not in the middle of populating a data packet)
    // Assumption: radioLinkTxAvailable() >= 1

    uint8 XDATA * packet;

    packet = radioLinkTxCurrentPacket();
    packet[0] = 1;   // Payload length is one byte.
    packet[1] = radioComTxSignals;
    sendSignalsSoon = 0;
    radioLinkTxSendPacket(PAYLOAD_TYPE_CONTROL_SIGNALS);
}

void radioComTxService(void)
{
    if (radioLinkResetPacketReceived)
    {
        // The other device has sent us a reset packet, which means it has been
        // reset.  We should send the state of the control signals to it.
        radioLinkResetPacketReceived = 0;
        sendSignalsSoon = 1;
    }

    if (sendSignalsSoon)
    {
        // We want to send the control signals ASAP.

        // NOTE: The if statement below could probably be moved to radioComTxControlSignals()
        // and then you could add the assumption here that txBytesLoaded is 0
        // (we are not in the middle of populating a data packet).
        if (txBytesLoaded != 0)
        {
            // There is normal data that needs to be sent before the control signals,
            // so send it now.
            radioComSendDataNow();
        }

        if (radioLinkTxAvailable())
        {
            radioComSendControlSignalsNow();
        }
    }
    else
    {
        // We don't need to send control signals ASAP, so we use the normal policy
        // for sending data: only send a non-full packet if the number of packets
        // queued in the lower level drops below the TX_QUEUE_THRESHOLD.

        if (txBytesLoaded != 0 && radioLinkTxQueued() <= TX_QUEUE_THRESHOLD)
        {
            radioComSendDataNow();
        }
    }
}

uint8 radioComTxAvailable(void)
{
    if (sendSignalsSoon)
    {
        // We want to send the control signals ASAP, but have not yet been able to
        // queue a packet for them.  Return 0 because we don't want to accept any
        // more data bytes until we queue up those control signals.  This is part of
        // the plan to ensure that everything is processed in the right order.
        return 0;
    }
    else
    {
        // Assumption: If txBytesLoaded is non-zero, radioLinkTxAvailable will be non-zero,
        // so the subtraction below does not overflow.
        // Assumption: The multiplication below does not overflow ever.
        return radioLinkTxAvailable()*RADIO_LINK_PAYLOAD_SIZE - txBytesLoaded;
    }
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
        radioComSendDataNow();
    }
}

// If we are in the middle of building a packet, send it.
void radioComTxControlSignals(uint8 controlSignals)
{
    if(controlSignals != radioComTxSignals)
    {
        radioComTxSignals = controlSignals;
        sendSignalsSoon = 1;
        radioComTxService();
    }
}
