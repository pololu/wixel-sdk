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
static volatile uint8 DATA rxMainLoopIndex = 0;   // The index of the next rxBuffer to read from the main loop.
static volatile uint8 DATA rxInterruptIndex = 0;  // The index of the next rxBuffer to write to when a packet comes from the radio.

/* txPackets are handed the same way. */
#define TX_PACKET_COUNT 16
static volatile uint8 XDATA radioLinkTxPacket[TX_PACKET_COUNT][1 + RADIO_MAX_PACKET_SIZE];  // The first byte is the length, 2nd byte is link header.
static volatile uint8 DATA txMainLoopIndex = 0;            // The index of the next txPacket to write to in the main loop.
static volatile uint8 DATA txInterruptIndex = 0;  // The index of the current txPacket we are trying to send on the radio.

uint8 XDATA shortTxPacket[2];


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

/* GENERAL FUNCTIONS **********************************************************/

void radioLinkInit()
{
	uint8 i;

    // Initialize the first byte of each TX buffer.  TODO: remove this
	for (i = 0; i < TX_PACKET_COUNT; i++)
	{
		radioLinkTxPacket[i][RADIO_LINK_PACKET_LENGTH_OFFSET] = 0;
	}

    rxSequenceBit = 1;

    txSequenceBit = 0;

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
	// Assumption: TX_PACKET_COUNT is a power of 2
	// TODO: check the assembly here to make sure it is reasonable
	return (txInterruptIndex - txMainLoopIndex - 1) & (TX_PACKET_COUNT - 1);
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
    // Now we set the length byte.  TODO: let the higher level code set the length byte?
    radioLinkTxPacket[txMainLoopIndex][RADIO_LINK_PACKET_LENGTH_OFFSET] = size + RADIO_LINK_PACKET_HEADER_LENGTH;

    // Make sure that radioMacEventHandler runs soon so it can see this new data and send it.
    radioMacStrobe();

    // Update our index of which packet to populate in the main loop.
    if (txMainLoopIndex == TX_PACKET_COUNT - 1)
    {
    	txMainLoopIndex = 0;
    }
    else
    {
    	txMainLoopIndex++;
    }
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

static void takeInitiative()
{
    if (txInterruptIndex != txMainLoopIndex)
    {
        // Try to send the next data packet.
        radioLinkTxPacket[txInterruptIndex][RADIO_LINK_PACKET_TYPE_OFFSET] = PACKET_TYPE_PING | txSequenceBit;
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
            if (txInterruptIndex != txMainLoopIndex)
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

    		// Check to see if there is actually any TX packet that we were sending that
        	// can be acknowledged.  This check should return true unless there is a bug
        	// on the other Wixel.
        	if (txInterruptIndex != txMainLoopIndex)
        	{
        		// Give ownership of the current TX packet back to the main loop by updated txInterruptIndex.
        		if (txInterruptIndex == TX_PACKET_COUNT - 1)
        		{
        			txInterruptIndex = 0;
        		}
        		else
        		{
        			txInterruptIndex++;
        		}

        		// The next packet we transmit will have a different sequence bit.
        		txSequenceBit ^= 1;
        	}
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

            if (txInterruptIndex != txMainLoopIndex)
            {
                // Send some data along with the ACK or NAK.
                radioLinkTxPacket[txInterruptIndex][RADIO_LINK_PACKET_TYPE_OFFSET] = responsePacketType | txInterruptIndex; // TODO: fix bug on this line
                radioMacTx(radioLinkTxPacket[txInterruptIndex]);
            }
            else
            {
                // No data is available, so just send the ACK or NAK by itself.

                shortTxPacket[RADIO_LINK_PACKET_LENGTH_OFFSET] = 1;
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
