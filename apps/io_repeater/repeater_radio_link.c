// TODO: make this a standard library (radio_packets?)

#include "repeater_radio_link.h"
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

#define PACKET_TYPE_MASK  (3 << 6) // These are the bits that determine the packet type.
#define PACKET_TYPE_PING  (0 << 6) // If both bits are zero, it is just a Ping packet (with optional data).

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
static volatile uint8 DATA radioLinkRxMainLoopIndex = 0;   // The index of the next rxBuffer to read from the main loop.
static volatile uint8 DATA radioLinkRxInterruptIndex = 0;  // The index of the next rxBuffer to write to when a packet comes from the radio.

/* txPackets are handled similarly */
#define TX_PACKET_COUNT 16
static volatile uint8 XDATA radioLinkTxPacket[TX_PACKET_COUNT][1 + RADIO_MAX_PACKET_SIZE];  // The first byte is the length, 2nd byte is link header.
static volatile uint8 DATA radioLinkTxMainLoopIndex = 0;   // The index of the next txPacket to write to in the main loop.
static volatile uint8 DATA radioLinkTxInterruptIndex = 0;  // The index of the current txPacket we are trying to send on the radio.

/* GENERAL FUNCTIONS **********************************************************/

void repeaterRadioLinkInit()
{
    PKTLEN = RADIO_MAX_PACKET_SIZE;
    CHANNR = param_radio_channel;

    radioMacInit();
    radioMacStrobe();
}

// Returns a random delay in units of 0.922 ms (the same units of radioMacRx).
// This is used to decide when to next transmit a queued data packet.
static uint8 repeaterRandomTxDelay()
{
    return 1 + (randomNumber() & 3);
}

/* TX FUNCTIONS (called by higher-level code in main loop) ********************/

uint8 repeaterRadioLinkTxAvailable(void)
{
    // Assumption: TX_PACKET_COUNT is a power of 2
    return (radioLinkTxInterruptIndex - radioLinkTxMainLoopIndex - 1) & (TX_PACKET_COUNT - 1);
}

uint8 repeaterRadioLinkTxQueued(void)
{
    return (radioLinkTxMainLoopIndex - radioLinkTxInterruptIndex) & (TX_PACKET_COUNT - 1);
}

uint8 XDATA * repeaterRadioLinkTxCurrentPacket()
{
    if (!repeaterRadioLinkTxAvailable())
    {
        return 0;
    }

    return radioLinkTxPacket[radioLinkTxMainLoopIndex] + RADIO_LINK_PACKET_HEADER_LENGTH;
}

void repeaterRadioLinkTxSendPacket(void)
{
    // Now we set the length byte.
    radioLinkTxPacket[radioLinkTxMainLoopIndex][0] = radioLinkTxPacket[radioLinkTxMainLoopIndex][RADIO_LINK_PACKET_HEADER_LENGTH] + RADIO_LINK_PACKET_HEADER_LENGTH;

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

uint8 XDATA * repeaterRadioLinkRxCurrentPacket(void)
{
    if (radioLinkRxMainLoopIndex == radioLinkRxInterruptIndex)
    {
        return 0;
    }
    return radioLinkRxPacket[radioLinkRxMainLoopIndex] + RADIO_LINK_PACKET_HEADER_LENGTH;
}

void repeaterRadioLinkRxDoneWithPacket(void)
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

static void repeaterTxDataPacket()
{
    radioLinkTxPacket[radioLinkTxInterruptIndex][RADIO_LINK_PACKET_TYPE_OFFSET] = PACKET_TYPE_PING;
    radioMacTx(radioLinkTxPacket[radioLinkTxInterruptIndex]);
}

static void repeaterTakeInitiative()
{
   if (radioLinkTxInterruptIndex != radioLinkTxMainLoopIndex)
    {
        // Try to send the next data packet.
       repeaterTxDataPacket();
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
        repeaterTakeInitiative();
        return;
    }
    else if (event == RADIO_MAC_EVENT_TX)
    {
        // Give ownership of the current TX packet back to the main loop by updated radioLinkTxInterruptIndex.
        if (radioLinkTxInterruptIndex == TX_PACKET_COUNT - 1)
        {
            radioLinkTxInterruptIndex = 0;
        }
        else
        {
            radioLinkTxInterruptIndex++;
        }

        // We sent a packet, so now lets give the other party a chance to talk.
        radioMacRx(radioLinkRxPacket[radioLinkRxInterruptIndex], repeaterRandomTxDelay());
        return;
    }
    else if (event == RADIO_MAC_EVENT_RX)
    {
        uint8 XDATA * currentRxPacket = radioLinkRxPacket[radioLinkRxInterruptIndex];

        if (!radioCrcPassed())
        {
            if (radioLinkTxInterruptIndex != radioLinkTxMainLoopIndex)
            {
                radioMacRx(currentRxPacket, repeaterRandomTxDelay());
            }
            else
            {
                radioMacRx(currentRxPacket, 0);
            }
            return;
        }

        if (currentRxPacket[RADIO_LINK_PACKET_LENGTH_OFFSET] > RADIO_LINK_PACKET_HEADER_LENGTH)
        {
            // We received a packet that contains actual data.

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
                // We can accept this packet!

                // Set length byte that will be read by the higher-level code.
                // (This overrides the 1-byte header.)
                currentRxPacket[RADIO_LINK_PACKET_HEADER_LENGTH] = currentRxPacket[RADIO_LINK_PACKET_LENGTH_OFFSET] - RADIO_LINK_PACKET_HEADER_LENGTH;

                radioLinkRxInterruptIndex = nextradioLinkRxInterruptIndex;
            }
        }

        repeaterTakeInitiative();
        return;
    }
    else if (event == RADIO_MAC_EVENT_RX_TIMEOUT)
    {
        repeaterTakeInitiative();
        return;
    }
}
