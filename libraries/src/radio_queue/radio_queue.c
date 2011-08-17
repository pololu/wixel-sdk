/* radio_queue.c:
 *  This layer builds on top of radio_mac.c to provide a mechanism for queueing
 *  RF packets to be sent and packets that are received by this device. It does
 *  not ensure reliability or specify the format of the packets, except that the
 *  first byte of the packet must contain its length.
 *
 *  This layer does not transmit packets as quickly as possible; instead, it
 *  listens for incoming packets for a random interval of 1-4 ms between sending
 *  packets.
 *
 *  This layer defines the RF packet memory buffers used, and controls access to
 *  those buffers.
 *
 *  Radio_queue is essentially a stripped-down version of the radio_link
 *  library, so radio_link is a good alternative if you want a more specialized
 *  implementation with more features.
 */

#include <radio_queue.h>
#include <radio_registers.h>
#include <random.h>

/* PARAMETERS *****************************************************************/

int32 CODE param_radio_channel = 128;

/* PACKET VARIABLES AND DEFINES ***********************************************/

// Compute the max size of on-the-air packets.  This value is stored in the PKTLEN register.
#define RADIO_MAX_PACKET_SIZE  (RADIO_QUEUE_PAYLOAD_SIZE)

#define RADIO_QUEUE_PACKET_LENGTH_OFFSET 0

/*  rxPackets:
 *  We need to be prepared at all times to receive a full packet from another
 *  party, even if we cannot give it to the main loop.  Therefore, we need (at
 *  least) THREE buffers, so that two can be owned by the main loop while
 *  another is owned by the ISR and ready to receive the next packet.
 *
 *  If a packet is received and the main loop still owns the other two buffers,
 *  we discard it.
 *
 *  Ownership of the RX packet buffers is determined from radioQueueRxMainLoopIndex and radioQueueRxInterruptIndex.
 *  The main loop owns all the buffers from radioQueueRxMainLoopIndex to radioQueueRxInterruptIndex-1 inclusive.
 *  If the two indices are equal, then the main loop owns nothing.  Here are three examples:
 *
 *  radioQueueRxMainLoopIndex | radioQueueRxInterruptIndex | Buffers owned by main loop.
 *                0 |                0 | None
 *                0 |                1 | rxBuffer[0]
 *                0 |                2 | rxBuffer[0 and 1]
 */
#define RX_PACKET_COUNT  3
static volatile uint8 XDATA radioQueueRxPacket[RX_PACKET_COUNT][1 + RADIO_MAX_PACKET_SIZE + 2];  // The first byte is the length.
static volatile uint8 DATA radioQueueRxMainLoopIndex = 0;   // The index of the next rxBuffer to read from the main loop.
static volatile uint8 DATA radioQueueRxInterruptIndex = 0;  // The index of the next rxBuffer to write to when a packet comes from the radio.

/* txPackets are handled similarly */
#define TX_PACKET_COUNT 16
static volatile uint8 XDATA radioQueueTxPacket[TX_PACKET_COUNT][1 + RADIO_MAX_PACKET_SIZE];  // The first byte is the length.
static volatile uint8 DATA radioQueueTxMainLoopIndex = 0;   // The index of the next txPacket to write to in the main loop.
static volatile uint8 DATA radioQueueTxInterruptIndex = 0;  // The index of the current txPacket we are trying to send on the radio.

BIT radioQueueAllowCrcErrors = 0;

/* GENERAL FUNCTIONS **********************************************************/

void radioQueueInit()
{
    randomSeedFromSerialNumber();

    PKTLEN = RADIO_MAX_PACKET_SIZE;
    CHANNR = param_radio_channel;

    radioMacInit();
    radioMacStrobe();
}

// Returns a random delay in units of 0.922 ms (the same units of radioMacRx).
// This is used to decide when to next transmit a queued data packet.
static uint8 randomTxDelay()
{
    return 1 + (randomNumber() & 3);
}

/* TX FUNCTIONS (called by higher-level code in main loop) ********************/

uint8 radioQueueTxAvailable(void)
{
    // Assumption: TX_PACKET_COUNT is a power of 2
    return (radioQueueTxInterruptIndex - radioQueueTxMainLoopIndex - 1) & (TX_PACKET_COUNT - 1);
}

uint8 radioQueueTxQueued(void)
{
    return (radioQueueTxMainLoopIndex - radioQueueTxInterruptIndex) & (TX_PACKET_COUNT - 1);
}

uint8 XDATA * radioQueueTxCurrentPacket()
{
    if (!radioQueueTxAvailable())
    {
        return 0;
    }

    return radioQueueTxPacket[radioQueueTxMainLoopIndex];
}

void radioQueueTxSendPacket(void)
{
    // Update our index of which packet to populate in the main loop.
    if (radioQueueTxMainLoopIndex == TX_PACKET_COUNT - 1)
    {
        radioQueueTxMainLoopIndex = 0;
    }
    else
    {
        radioQueueTxMainLoopIndex++;
    }

    // Make sure that radioMacEventHandler runs soon so it can see this new data and send it.
    // This must be done LAST.
    radioMacStrobe();
}

/* RX FUNCTIONS (called by higher-level code in main loop) ********************/

uint8 XDATA * radioQueueRxCurrentPacket(void)
{
    if (radioQueueRxMainLoopIndex == radioQueueRxInterruptIndex)
    {
        return 0;
    }
    return radioQueueRxPacket[radioQueueRxMainLoopIndex];
}

void radioQueueRxDoneWithPacket(void)
{
    if (radioQueueRxMainLoopIndex == RX_PACKET_COUNT - 1)
    {
        radioQueueRxMainLoopIndex = 0;
    }
    else
    {
        radioQueueRxMainLoopIndex++;
    }
}

/* FUNCTIONS CALLED IN RF_ISR *************************************************/

static void takeInitiative()
{
    if (radioQueueTxInterruptIndex != radioQueueTxMainLoopIndex)
    {
        // Try to send the next data packet.
        radioMacTx(radioQueueTxPacket[radioQueueTxInterruptIndex]);
    }
    else
    {
        radioMacRx(radioQueueRxPacket[radioQueueRxInterruptIndex], 0);
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
        // Give ownership of the current TX packet back to the main loop by updated radioQueueTxInterruptIndex.
        if (radioQueueTxInterruptIndex == TX_PACKET_COUNT - 1)
        {
            radioQueueTxInterruptIndex = 0;
        }
        else
        {
            radioQueueTxInterruptIndex++;
        }

        // We sent a packet, so now let's give another party a chance to talk.
        radioMacRx(radioQueueRxPacket[radioQueueRxInterruptIndex], randomTxDelay());
        return;
    }
    else if (event == RADIO_MAC_EVENT_RX)
    {
        uint8 XDATA * currentRxPacket = radioQueueRxPacket[radioQueueRxInterruptIndex];

        if (!radioQueueAllowCrcErrors && !radioCrcPassed())
        {
            if (radioQueueTxInterruptIndex != radioQueueTxMainLoopIndex)
            {
                radioMacRx(currentRxPacket, randomTxDelay());
            }
            else
            {
                radioMacRx(currentRxPacket, 0);
            }
            return;
        }

        if (currentRxPacket[RADIO_QUEUE_PACKET_LENGTH_OFFSET] > 0)
        {
            // We received a packet that contains actual data.

            uint8 nextradioQueueRxInterruptIndex;

            // See if we can give the data to the main loop.
            if (radioQueueRxInterruptIndex == RX_PACKET_COUNT - 1)
            {
                nextradioQueueRxInterruptIndex = 0;
            }
            else
            {
                nextradioQueueRxInterruptIndex = radioQueueRxInterruptIndex + 1;
            }

            if (nextradioQueueRxInterruptIndex != radioQueueRxMainLoopIndex)
            {
                // We can accept this packet!
                radioQueueRxInterruptIndex = nextradioQueueRxInterruptIndex;
            }
        }

        takeInitiative();
        return;
    }
    else if (event == RADIO_MAC_EVENT_RX_TIMEOUT)
    {
        takeInitiative();
        return;
    }
}
