/* radio_link.c:
 *  This layer uses radio_mac.c in order to provide reliable ordered delivery and reception of
 *  a series of data packets between this device and another.  This is the layer that takes care
 *  of Ping/ACK/NAK packets, and handles the details of timing (however, the timing details
 *  are dependent on the performance/configuration of the MAC layer, so if the MAC layer
 *  changes then the timing details here may have to change.)
 *
 *  There is a distinction here between RF packets and data packets:  an RF packet is what gets
 *  transmitted by the radio.  A data packet is a piece of data that needs to be sent to the
 *  other device, and it might correspond to several RF packets because there are retries and
 *  ACKs.
 */

#include <radio_link.h>
#include <radio_registers.h>
#include <random.h>

/* PARAMETERS *****************************************************************/

int32 CODE param_radio_channel = 128;

/* PACKET VARIABLES AND DEFINES ***********************************************/

// Compute the max size of on-the-air packets.  This value is stored in the PKTLEN register.
#define RADIO_MAX_PACKET_SIZE  (RADIO_LINK_PAYLOAD_SIZE + RADIO_LINK_PACKET_HEADER_LENGTH)

// The link layer will add a one byte header to the beginning of each packet.
#define RADIO_LINK_PACKET_HEADER_LENGTH 1

#define RADIO_LINK_PACKET_LENGTH_OFFSET 0
#define RADIO_LINK_PACKET_TYPE_OFFSET   1

#define RADIO_LINK_PAYLOAD_TYPE_BIT_OFFSET 1
#define RADIO_LINK_PAYLOAD_TYPE_MASK       0b00011110

#define PACKET_TYPE_MASK  (3 << 6) // These are the bits that determine the packet type.
#define PACKET_TYPE_PING  (0 << 6) // If both bits are zero, it is just a Ping packet (with optional data).
#define PACKET_TYPE_NAK   (1 << 6) // A NAK packet (with optional data)
#define PACKET_TYPE_ACK   (2 << 6) // An ACK packet (with optional data)
#define PACKET_TYPE_RESET (3 << 6) // A Reset packet (the next packet transmitted by the sender of this packet will have a sequence number of 0)

/*  rxPackets:
 *  We need to be prepared at all times to receive a full packet from the other party,
 *  even if all we can do is NAK it.  Therefore, we need (at least) THREE buffers, so
 *  that two can be owned by the main loop while another is owned by the ISR and ready
 *  to receive the next packet.
 *
 *  If a packet is received and the main loop still owns the other two buffers,
 *  we respond with a NAK to the other device.
 *
 *  Ownership of the RX packet buffers is determined from radioLinkRxMainLoopIndex and radioLinkRxInterruptIndex.
 *  The main loop owns all the buffers from radioLinkRxMainLoopIndex to radioLinkRxInterruptIndex-1 inclusive.
 *  If the two indices are equal, then the main loop owns nothing.  Here are three examples:
 *
 *  radioLinkRxMainLoopIndex | radioLinkRxInterruptIndex | Buffers owned by main loop.
 *                0 |                0 | None
 *                0 |                1 | rxBuffer[0]
 *                0 |                2 | rxBuffer[0 and 1]
 */
#define RX_PACKET_COUNT  3
static volatile uint8 XDATA radioLinkRxPacket[RX_PACKET_COUNT][1 + RADIO_MAX_PACKET_SIZE + 2];  // The first byte is the length, 2nd byte is link header.
volatile uint8 DATA radioLinkRxMainLoopIndex = 0;   // The index of the next rxBuffer to read from the main loop.
volatile uint8 DATA radioLinkRxInterruptIndex = 0;  // The index of the next rxBuffer to write to when a packet comes from the radio.

/* txPackets are handled similarly */
#define TX_PACKET_COUNT 16
static volatile uint8 XDATA radioLinkTxPacket[TX_PACKET_COUNT][1 + RADIO_MAX_PACKET_SIZE];  // The first byte is the length, 2nd byte is link header.
volatile uint8 DATA radioLinkTxMainLoopIndex = 0;   // The index of the next txPacket to write to in the main loop.
volatile uint8 DATA radioLinkTxInterruptIndex = 0;  // The index of the current txPacket we are trying to send on the radio.

uint8 XDATA shortTxPacket[2];

// The number of times the current TX packet has been transmitted.
// Does NOT overflow.  If we have transmitting the current packet more than 255
// times, this variable will be 255.
uint8 DATA radioLinkTxCurrentPacketTries = 0;

static volatile BIT sendingReset = 0;
static volatile BIT acceptAnySequenceBit = 0;

volatile BIT radioLinkResetPacketReceived;

/* SEQUENCING VARIABLES *******************************************************/
/* Each data packet we transmit contains a bit that is either 0 or 1 called the
   sequence bit.  This bit changes every time we send a different data packet,
   so it allows the receiver to tell if we transmitted the same packet twice
   (which is what happens when the receiver's ACK packet was lost).  */

// rxSequenceBit is the value of the sequence bit for the LAST packet we received.
// If we receive a packet whose sequence bit has the same value, we will ACK it
// but not pass the data to the main loop.  This variable is only used in the ISR.
static volatile BIT rxSequenceBit;

// txSequenceBit is the value of the sequence bit for the NEXT packet we will
// send.
static volatile BIT txSequenceBit;


/* GENERAL VARIABLES **********************************************************/

volatile BIT radioLinkActivityOccurred;

/* GENERAL FUNCTIONS **********************************************************/

void radioLinkInit()
{
    randomSeedFromSerialNumber();

    rxSequenceBit = 1;

    txSequenceBit = 0;

    PKTLEN = RADIO_MAX_PACKET_SIZE;
    CHANNR = param_radio_channel;

    acceptAnySequenceBit = 1;
    radioMacInit();

    // Start trying to send a reset packet.
    sendingReset = 1;
    radioMacStrobe();
}

// Returns a random delay in units of 0.922 ms (the same units of radioMacRx).
// This is used to decide how long to wait before retransmitting.
// This is used to decide when to next transmit a queued data packet.
// If we have already tried sending this packet many times, then this function
// will return a much longer delay, in order to avoid overcrowding the airwaves for
// no reason.  This is similar to the idea of exponential backoff used in other
// communications protocols such as Ethernet:
// http://en.wikipedia.org/wiki/Exponential_backoff
static uint8 randomTxDelay()
{
    // 200 and 250 were chosen arbitrarily.
    return (radioLinkTxCurrentPacketTries > 200 ? 250 : 1) + (randomNumber() & 3);
}

BIT radioLinkConnected()
{
    return !sendingReset;
}

/* TX FUNCTIONS (called by higher-level code in main loop) ********************/

uint8 radioLinkTxAvailable(void)
{
    // Assumption: TX_PACKET_COUNT is a power of 2
    return (radioLinkTxInterruptIndex - radioLinkTxMainLoopIndex - 1) & (TX_PACKET_COUNT - 1);
}

uint8 radioLinkTxQueued(void)
{
    return (radioLinkTxMainLoopIndex - radioLinkTxInterruptIndex) & (TX_PACKET_COUNT - 1);
}

uint8 XDATA * radioLinkTxCurrentPacket()
{
    if (!radioLinkTxAvailable())
    {
        return 0;
    }

    return radioLinkTxPacket[radioLinkTxMainLoopIndex] + RADIO_LINK_PACKET_HEADER_LENGTH;
}

void radioLinkTxSendPacket(uint8 payloadType)
{
    // Now we set the length byte.
    radioLinkTxPacket[radioLinkTxMainLoopIndex][0] = radioLinkTxPacket[radioLinkTxMainLoopIndex][RADIO_LINK_PACKET_HEADER_LENGTH] + RADIO_LINK_PACKET_HEADER_LENGTH;

    // Put the payloadType into the packet header.
    radioLinkTxPacket[radioLinkTxMainLoopIndex][RADIO_LINK_PACKET_TYPE_OFFSET] = payloadType << RADIO_LINK_PAYLOAD_TYPE_BIT_OFFSET;

    // Update our index of which packet to populate in the main loop.
    if (radioLinkTxMainLoopIndex == TX_PACKET_COUNT - 1)
    {
        radioLinkTxMainLoopIndex = 0;
    }
    else
    {
        radioLinkTxMainLoopIndex++;
    }

    // Make sure that radioMacEventHandler runs soon so it can see this new data and send it.
    // This must be done LAST.
    radioMacStrobe();
}

/* RX FUNCTIONS (called by higher-level code in main loop) ********************/

uint8 XDATA * radioLinkRxCurrentPacket(void)
{
    if (radioLinkRxMainLoopIndex == radioLinkRxInterruptIndex)
    {
        return 0;
    }

    return radioLinkRxPacket[radioLinkRxMainLoopIndex] + RADIO_LINK_PACKET_HEADER_LENGTH;
}

uint8 radioLinkRxCurrentPayloadType(void)
{
    return radioLinkRxPacket[radioLinkRxMainLoopIndex][0];
}

void radioLinkRxDoneWithPacket(void)
{
    if (radioLinkRxMainLoopIndex == RX_PACKET_COUNT - 1)
    {
        radioLinkRxMainLoopIndex = 0;
    }
    else
    {
        radioLinkRxMainLoopIndex++;
    }
}

/* FUNCTIONS CALLED IN RF_ISR *************************************************/

static void txResetPacket()
{
    shortTxPacket[RADIO_LINK_PACKET_LENGTH_OFFSET] = 1;
    shortTxPacket[RADIO_LINK_PACKET_TYPE_OFFSET] = PACKET_TYPE_RESET;
    radioMacTx(shortTxPacket);
    if (radioLinkTxCurrentPacketTries < 255)
    {
        radioLinkTxCurrentPacketTries++;
    }
}

static void txDataPacket(uint8 packetType)
{
    radioLinkTxPacket[radioLinkTxInterruptIndex][RADIO_LINK_PACKET_TYPE_OFFSET] =
            (radioLinkTxPacket[radioLinkTxInterruptIndex][RADIO_LINK_PACKET_TYPE_OFFSET] & RADIO_LINK_PAYLOAD_TYPE_MASK) | packetType | txSequenceBit;
    radioMacTx(radioLinkTxPacket[radioLinkTxInterruptIndex]);
    if (radioLinkTxCurrentPacketTries < 255)
    {
        radioLinkTxCurrentPacketTries++;
    }
}

static void takeInitiative()
{
    if (sendingReset)
    {
        // Try to send a reset packet.
        txResetPacket();
        radioLinkActivityOccurred = 1;
    }
    else if (radioLinkTxInterruptIndex != radioLinkTxMainLoopIndex)
    {
        // Try to send the next data packet.
        txDataPacket(PACKET_TYPE_PING);
        radioLinkActivityOccurred = 1;
    }
    else
    {
        radioMacRx(radioLinkRxPacket[radioLinkRxInterruptIndex], 0);
    }
}

void radioMacEventHandler(uint8 event) // called by the MAC in an ISR
{
    if (event == RADIO_MAC_EVENT_STROBE)
    {
        takeInitiative();
        return;
    }
    else if (event == RADIO_MAC_EVENT_TX)
    {
        // We sent a packet, so now lets give the other party a chance to talk.
        radioMacRx(radioLinkRxPacket[radioLinkRxInterruptIndex], randomTxDelay());
        return;
    }
    else if (event == RADIO_MAC_EVENT_RX)
    {
        uint8 XDATA * currentRxPacket = radioLinkRxPacket[radioLinkRxInterruptIndex];

        if (!radioCrcPassed())
        {
            if (radioLinkTxInterruptIndex != radioLinkTxMainLoopIndex)
            {
                radioMacRx(currentRxPacket, randomTxDelay());
            }
            else
            {
                radioMacRx(currentRxPacket, 0);
            }
            return;
        }

        if ((currentRxPacket[RADIO_LINK_PACKET_TYPE_OFFSET] & PACKET_TYPE_MASK) == PACKET_TYPE_RESET)
        {
            // The other Wixel sent a Reset packet, which means the next packet it sends will have a sequence bit of 0.
            // So this Wixel should set its "previously received" sequence bit to 1 so it expects a 0 next.
            rxSequenceBit = 1;

            // Notify the higher-level code.
            radioLinkResetPacketReceived = 1;

            // Send an ACK
            shortTxPacket[RADIO_LINK_PACKET_LENGTH_OFFSET] = 1;
            shortTxPacket[RADIO_LINK_PACKET_TYPE_OFFSET] = PACKET_TYPE_ACK;
            radioMacTx(shortTxPacket);

            radioLinkActivityOccurred = 1;

            return;
        }

        if ((currentRxPacket[RADIO_LINK_PACKET_TYPE_OFFSET] & PACKET_TYPE_MASK) == PACKET_TYPE_ACK)
        {
            // The packet we received contained an acknowledgment.

            if (sendingReset)
            {
                // If we were sending a Reset packet, stop trying to resend it.
                sendingReset = 0;

                // Reset the transmission counter.
                radioLinkTxCurrentPacketTries = 0;

                // Make sure the next packet we transmit has a sequence bit of 0.
                txSequenceBit = 0;
            }
            else if (radioLinkTxInterruptIndex != radioLinkTxMainLoopIndex)
            {
                // Check to see if there is actually any TX packet that we were sending that
                // can be acknowledged.  This check should return true unless there is a bug
                // on the other Wixel.

                // Give ownership of the current TX packet back to the main loop by updated radioLinkTxInterruptIndex.
                if (radioLinkTxInterruptIndex == TX_PACKET_COUNT - 1)
                {
                    radioLinkTxInterruptIndex = 0;
                }
                else
                {
                    radioLinkTxInterruptIndex++;
                }

                // Reset the transmission counter.
                radioLinkTxCurrentPacketTries = 0;

                // The next packet we transmit will have a different sequence bit.
                txSequenceBit ^= 1;
            }
        }

        if (currentRxPacket[RADIO_LINK_PACKET_LENGTH_OFFSET] > RADIO_LINK_PACKET_HEADER_LENGTH)
        {
            // We received a packet that contains actual data.

            uint8 responsePacketType = PACKET_TYPE_ACK;

            if (acceptAnySequenceBit || (rxSequenceBit != (currentRxPacket[RADIO_LINK_PACKET_TYPE_OFFSET] & 1)))
            {
                // This packet is NOT a retransmission of the last packet we received.

                uint8 nextradioLinkRxInterruptIndex;

                // See if we can give the data to the main loop.
                if (radioLinkRxInterruptIndex == RX_PACKET_COUNT - 1)
                {
                    nextradioLinkRxInterruptIndex = 0;
                }
                else
                {
                    nextradioLinkRxInterruptIndex = radioLinkRxInterruptIndex + 1;
                }

                if (nextradioLinkRxInterruptIndex != radioLinkRxMainLoopIndex)
                {
                    // We can accept this packet and send an ACK!

                    uint8 payloadType;

                    // Set rxSequenceBit to match the sequence bit in the received packet
                    rxSequenceBit = (currentRxPacket[RADIO_LINK_PACKET_TYPE_OFFSET] & 1);
                    acceptAnySequenceBit = 0;

                    // Extract the payload type.
                    payloadType = (currentRxPacket[RADIO_LINK_PACKET_TYPE_OFFSET] & RADIO_LINK_PAYLOAD_TYPE_MASK) >> RADIO_LINK_PAYLOAD_TYPE_BIT_OFFSET;

                    // Set length byte that will be read by the higher-level code.
                    // (This overrides the 1-byte header.)
                    currentRxPacket[RADIO_LINK_PACKET_HEADER_LENGTH] = currentRxPacket[RADIO_LINK_PACKET_LENGTH_OFFSET] - RADIO_LINK_PACKET_HEADER_LENGTH;

                    // Set the payload type byte which will be read by radioLinkRxCurrentPayloadType().
                    // (This overrides the 1-byte RF packet length.)
                    currentRxPacket[0] = payloadType;

                    radioLinkRxInterruptIndex = nextradioLinkRxInterruptIndex;
                }
                else
                {
                    // The main loop is already using all of the other RX packet buffers,
                    // so we can't give this packet to the main loop and we will send a NAK.
                    responsePacketType = PACKET_TYPE_NAK;
                }

            }

            // Send an ACK or NAK to the other party.

            if (radioLinkTxInterruptIndex != radioLinkTxMainLoopIndex)
            {
                // Send some data along with the ACK or NAK.
                txDataPacket(responsePacketType);
            }
            else
            {
                // No data is available, so just send the ACK or NAK by itself.

                shortTxPacket[RADIO_LINK_PACKET_LENGTH_OFFSET] = 1;
                shortTxPacket[RADIO_LINK_PACKET_TYPE_OFFSET] = responsePacketType;
                radioMacTx(shortTxPacket);
            }

            radioLinkActivityOccurred = 1;
        }
        else
        {
            // TODO: if radioLinkTxCurrentPacketTries > 200 then we should probably just go
            // into RX mode here so we can avoid having this conversation 4 times per second:
            // DATA, NAK, DATA, NAK, DATA, NAK, DATA, NAK, DATA, NAK, ...
            // (It starts when the sender takes initiative and sends his (failing) data every
            // 250ms, and it ends as soon as one packet is lost.)
            takeInitiative();
        }
        return;
    }
    else if (event == RADIO_MAC_EVENT_RX_TIMEOUT)
    {
        takeInitiative();
        return;
    }
}
