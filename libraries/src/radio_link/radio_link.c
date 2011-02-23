// TODO: make some way for the devices to synchronize their sequence numbers so when a
//  device starts up, it can guarantee that its first data packet won't be ignored by
//  the other party?

/* radio_link.c:
 *  This layer uses radio_mac.c in order to provide reliable ordered delivery and reception of
 *  a series of data packets between this device and another.  This is the layer that takes care
 *  of Ping/ACK/NAK packets, and handles the details of timing (however, the timing details
 *  are dependent on the performance/configuration of the MAC layer, so if the MAC layer
 *  changes then the timing details here may have to change.)
 *
 *  This layer defines the RF packet memory buffers used, and controls access to those buffers.
 *
 *  This layer also takes care of presence detection by sending out regular Ping packets even
 *  if there is no data packet to transmit.
 *
 *  This is the layer that restricts us to only talk to one device.  (If you wanted to make a
 *  network with more than 2 devices, you would have to replace this layer with something more
 *  complicated that has addressing.)
 *
 *  Similarly, this layer also restricts us to only having one logical data pipe.  If you wanted
 *  to send some extra data that doesn't get NAKed, or gets NAKed at different times then the
 *  regular data, you would need to replace this layer with something more complicated that
 *  keeps track of different streams and schedules them in a reasonable way.
 *
 *  There is a distinction here between RF packets and data packets:  an RF packet is what gets
 *  transmitted by the radio.  A data packet is a piece of data that needs to be sent to the
 *  other device, and it might correspond to several RF packets because there are retries and
 *  ACKs.
 *
 *  This layer does not correspond cleanly to any of the layers in the OSI Model.
 *  It combines portions of the Data Link Layer (#2), Network Layer (#3), and
 *  Transport Layer (#4).
 */

#include <radio_link.h>
#include <radio_registers.h>
#include <random.h>

#define RADIO_LINK_PACKET_TYPE_OFFSET   1

#define PACKET_TYPE_MASK (3 << 6) // These are the bits that determine the packet type.
#define PACKET_TYPE_PING (0 << 6) // If both bits are zero, it is just a Ping packet (with optional data).
#define PACKET_TYPE_NAK  (1 << 6) // A NAK packet (with optional data)
#define PACKET_TYPE_ACK  (2 << 6) // An ACK packet (with optional data)

uint8 XDATA shortTxPacket[] = {1, 0};  // later we want to send ACKs with data using the same buffer

/*  Ownership of the txPackets is determined by the length byte: if it is non-zero, then
 *  this packet needs to be sent by the ISR and the main loop should not touch it.
 *  If the length byte is zero, then this packet needs to be populated by the main loop
 *  and the ISR should not send it yet.
 *  txMainLoopIndex and txInterruptIndex just serve to keep track of which buffer needs
 *  to be handled next. */
static volatile XDATA uint8 radioLinkTxPacket[2][1 + RADIO_MAX_PACKET_SIZE];  // The first byte is the length, 2nd byte is link header.
BIT txMainLoopIndex = 0;            // The index of the next txPacket to write to in the main loop.
static volatile BIT txInterruptIndex = 0;  // The index of the current txPacket we are trying to send on the radio.

volatile uint8 DATA tmphaxB = 0;

/*  rxPackets are handled differently because we need to be prepared at all times to
 *  receive a full packet from the other party, even if all we can do is NAK it.
 *  Therefore, we need a total of THREE buffers, so that two can be owned by the
 *  main loop while another is owned by the ISR and ready to receive the next packet.
 *
 *  If a packet is received and the main loop still owns the other two buffers,
 *  we respond with a NAK to the other party.
 *
 *  Ownership of the RX packet buffers is determined from rxMainLoopIndex and rxInterruptIndex.
 *  The main loop owns all the buffers from rxMainLoopIndex to rxInterruptIndex-1 inclusive.
 *  If the two indices are equal, then the main loop owns nothing.  Here are three examples:
 *
 *  rxMainLoopIndex | rxInterruptIndex | Buffers owned by main loop.
 *                0 |                0 | None
 *                0 |                1 | rxBuffer[0]
 *                0 |                2 | rxBuffer[0 and 1]
 */
#define RX_PACKET_COUNT  3
static volatile uint8 XDATA radioLinkRxPacket[RX_PACKET_COUNT][1 + RADIO_MAX_PACKET_SIZE + 2];  // The first byte is the length, 2nd byte is link header.
volatile uint8 DATA rxMainLoopIndex = 0;   // The 0-2 index of the next rxBuffer to read from the main loop.
volatile uint8 DATA rxInterruptIndex = 0;  // The 0-2 index of the next rxBuffer to write to when a packet comes from the radio.

/* SEQUENCING VARIABLES *******************************************************/
/* Each data packet we transmit contains a bit that is either 0 or 1 called the
   sequence bit.  This bit changes every time we send a different data packet,
   so it allows the receiver to tell if we transmitted the same packet twice
   (which is what happens when the receiver's ACK packet was lost).  */

// rxSequenceBit is the value of the sequence bit for the LAST packet we received.
// If we receive a packet whose sequence bit has the same value, we will ACK it
// but not pass the data to the main loop.  This variable is only used in the ISR.
volatile BIT rxSequenceBit;

/* GENERAL FUNCTIONS **********************************************************/

void radioLinkInit()
{
    // Initialize the first byte of each TX buffer: this indicates the that both buffers
    // are owned by the main loop.
    radioLinkTxPacket[0][RADIO_LINK_PACKET_LENGTH_OFFSET] = 0;
    radioLinkTxPacket[1][RADIO_LINK_PACKET_LENGTH_OFFSET] = 0;

    rxSequenceBit = 1;

    PKTLEN = RADIO_MAX_PACKET_SIZE;

    radioMacInit();
    radioMacStrobe();
}

// Returns a random delay in units of 50 microseconds (the same units of radioMacRx).
// This is used to decide how long to wait before retransmitting.
// Eventually we might want to make this delay depend on the number of times the
// packet has been sent already, so we can do some sort of exponential backoff.
static uint16 randomDelay()
{
    // WARNING: You can't do multiplication here because it might call a non-reentrant
    // 16-bit math function.  We can't do that in an ISR!  Also, if you ever change this
    // function, you should check it to make sure it isn't calling a non-reentrant math
    // function.
    return (2 + (randomNumber() & 15)) << 4;
}

/* TX FUNCTIONS (called by higher-level code in main loop) ********************/

uint8 radioLinkTxAvailable(void)
{
    // If the length byte of a TX packet buffer is zero, then the buffer is available
    // for us to populate.

    return ((radioLinkTxPacket[0][RADIO_LINK_PACKET_LENGTH_OFFSET] == 0)
            + (radioLinkTxPacket[1][RADIO_LINK_PACKET_LENGTH_OFFSET] == 0));
}

uint8 XDATA * radioLinkTxCurrentPacket()
{
    if (!radioLinkTxAvailable())
    {
        return 0;
    }

    return radioLinkTxPacket[txMainLoopIndex];
}

void radioLinkTxSendPacket(uint8 size)
{
    // Assumption: the user has added data to the current TX packet, but has NOT set the
    // length byte.

    // Now we set the length byte.  This is the signal to the ISR that the packet buffer now
    // belongs to the ISR.  The main loop must not touch this buffer any more until the ISR
    // sets the length byte to 0.
    radioLinkTxPacket[txMainLoopIndex][RADIO_LINK_PACKET_LENGTH_OFFSET] = size + RADIO_LINK_PACKET_HEADER_LENGTH;

    // Make sure that radioMacEventHandler runs soon so it can see this new data and send it.
    radioMacStrobe();

    // Update our index of which packet to populate in the main loop.
    txMainLoopIndex ^= 1;
}

/* RX FUNCTIONS (called by higher-level code in main loop) ********************/

XDATA uint8 * radioLinkRxCurrentPacket(void)
{
    if (rxMainLoopIndex == rxInterruptIndex)
    {
        return 0;
    }

    return radioLinkRxPacket[rxMainLoopIndex];
}

void radioLinkRxDoneWithPacket(void)
{
    if (rxMainLoopIndex == RX_PACKET_COUNT - 1)
    {
        rxMainLoopIndex = 0;
    }
    else
    {
        rxMainLoopIndex++;
    }
}

/* FUNCTIONS CALLED IN RF_ISR *************************************************/

void takeInitiative()
{
    if (radioLinkTxPacket[txInterruptIndex][RADIO_LINK_PACKET_LENGTH_OFFSET] != 0)
    {
        // Try to send the next data packet.
        radioLinkTxPacket[txInterruptIndex][RADIO_LINK_PACKET_TYPE_OFFSET] = PACKET_TYPE_PING | txInterruptIndex;
        radioMacTx(radioLinkTxPacket[txInterruptIndex]);
    }
    else
    {

        radioMacRx(radioLinkRxPacket[rxInterruptIndex], 0);
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
        radioMacRx(radioLinkRxPacket[rxInterruptIndex], randomDelay());
        return;
    }
    else if (event == RADIO_MAC_EVENT_RX)
    {
        uint8 XDATA * currentRxPacket = radioLinkRxPacket[rxInterruptIndex];

        if (!radioCrcPassed())
        {
            if (radioLinkTxPacket[txInterruptIndex][RADIO_LINK_PACKET_LENGTH_OFFSET] != 0)
            {
                radioMacRx(currentRxPacket, randomDelay());
            }
            else
            {
                radioMacRx(currentRxPacket, 0);
            }
            return;
        }

        if ((currentRxPacket[RADIO_LINK_PACKET_TYPE_OFFSET] & PACKET_TYPE_MASK) == PACKET_TYPE_ACK)
        {
            // The packet we received contained an acknowledgment.

            // NOTE: Maybe we should detect whether this acknowledgment could possibly be valid before
            // acting on it.  If radioLinkTxPacket[txInterruptIndex][RADIO_LINK_PACKET_LENGTH_OFFSET] == 0
            // then this is acknowledgment can not be valid.

            // Give ownership of the current TX packet back to the main loop.
            radioLinkTxPacket[txInterruptIndex][RADIO_LINK_PACKET_LENGTH_OFFSET] = 0;

            // The next packet we transmit will come from the other buffer.
            txInterruptIndex ^= 1;
        }

        if (currentRxPacket[RADIO_LINK_PACKET_LENGTH_OFFSET] > RADIO_LINK_PACKET_HEADER_LENGTH)
        {
            // We received a packet that contains actual data.

            uint8 responsePacketType = PACKET_TYPE_ACK;

            if (rxSequenceBit != (currentRxPacket[RADIO_LINK_PACKET_TYPE_OFFSET] & 1))
            {
                // This packet is NOT a retransmission of the last packet we received.

                uint8 nextRxInterruptIndex;

                // See if we can give the data to the main loop.
                if (rxInterruptIndex == RX_PACKET_COUNT - 1)
                {
                    nextRxInterruptIndex = 0;
                }
                else
                {
                    nextRxInterruptIndex = rxInterruptIndex + 1;
                }

                if(nextRxInterruptIndex != rxMainLoopIndex)
                {
                    // We can accept this packet and send an ACK!

                    // Decrease the size byte because the higher level code doesn't know about the header added by this layer.
                    currentRxPacket[RADIO_LINK_PACKET_LENGTH_OFFSET] -= RADIO_LINK_PACKET_HEADER_LENGTH;

                    rxSequenceBit ^= 1;

                    rxInterruptIndex = nextRxInterruptIndex;
                }
                else
                {
                    // The main loop is already using all of the other RX packet buffers,
                    // so we can't give this packet to the main loop and we will send a NAK.
                    responsePacketType = PACKET_TYPE_NAK;
                }

            }

            // Send an ACK or NAK to the other party.

            if (radioLinkTxPacket[txInterruptIndex][RADIO_LINK_PACKET_LENGTH_OFFSET] != 0)
            {
                // Send some data along with the ACK or NAK.
                radioLinkTxPacket[txInterruptIndex][RADIO_LINK_PACKET_TYPE_OFFSET] = responsePacketType | txInterruptIndex;
                radioMacTx(radioLinkTxPacket[txInterruptIndex]);
            }
            else
            {
                // No data is available, so just send the ACK or NAK by itself.

                // Assumption: The length byte of the shortTxPacket is 1.
                shortTxPacket[RADIO_LINK_PACKET_LENGTH_OFFSET] = 1;  // tmphax
                shortTxPacket[RADIO_LINK_PACKET_TYPE_OFFSET] = responsePacketType;
                radioMacTx(shortTxPacket);
            }
        }
        else
        {
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

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
