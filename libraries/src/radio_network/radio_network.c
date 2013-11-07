/* radio_networkc:
 *  This layer builds on top of radio_mac.c and radio_address.c to provide a mechanism for queueing
 *  RF packets to be sent and packets that are received by this device. It does
 *  not ensure reliability.
 *
 *  The packet format is:
 *  LENGTH (1Byte) NEXT_NODE_ADDRESS(1Byte) DESTINATION ADDRESS(1Byte) SOURCE_ADDRESS(1Byte)
 *
 *  This layer does not transmit packets as quickly as possible; instead, it
 *  listens for incoming packets for a random interval of 1-4 ms between sending
 *  packets.
 *
 *  This layer defines the RF packet memory buffers used, and controls access to
 *  those buffers.
 *
 *  Radio_network is essentially an advanced version of the radio_queue
 *  library, so radio_advanced is a good alternative if you do not neet network management but just queueing.
 */

#include <radio_network.h>
#include <radio_registers.h>
#include <random.h>
#include <string.h> //needed by memset


/* PARAMETERS *****************************************************************/

int32 CODE param_radio_channel = 128;

/* PACKET VARIABLES AND DEFINES ***********************************************/

// Compute the max size of on-the-air packets.  This value is stored in the PKTLEN register.
// 20 byte payload + 1 next node address + 1 destination address + 1 source address
#define RADIO_MAX_PACKET_SIZE  (RADIO_NETWORK_PAYLOAD_SIZE + 3)

#define RADIO_NETWORK_PACKET_LENGTH_OFFSET 0

#define HEADER_LENGTH 3
#define NODE_ADDRESS RADIO_NETWORK_PACKET_LENGTH_OFFSET+1
#define DESTINATION_ADDRESS RADIO_NETWORK_PACKET_LENGTH_OFFSET+2
#define SOURCE_ADDRESS RADIO_NETWORK_PACKET_LENGTH_OFFSET+3

/*  rxPackets:
 *  We need to be prepared at all times to receive a full packet from another
 *  party, even if we cannot give it to the main loop.  Therefore, we need (at
 *  least) THREE buffers, so that two can be owned by the main loop while
 *  another is owned by the ISR and ready to receive the next packet.
 *
 *  If a packet is received and the main loop still owns the other two buffers,
 *  we discard it.
 *
 *  Ownership of the RX packet buffers is determined from radioNetworkRxMainLoopIndex and radioNetworkRxInterruptIndex.
 *  The main loop owns all the buffers from radioNetworkRxMainLoopIndex to radioNetworkRxInterruptIndex-1 inclusive.
 *  If the two indices are equal, then the main loop owns nothing.  Here are three examples:
 *
 *  radioNetworkRxMainLoopIndex | radioNetworkRxInterruptIndex | Buffers owned by main loop.
 *                0 |                0 | None
 *                0 |                1 | rxBuffer[0]
 *                0 |                2 | rxBuffer[0 and 1]
 *
 */
#define RX_PACKET_COUNT  3
static volatile uint8 XDATA radioNetworkRxPacket[RX_PACKET_COUNT][1 + RADIO_MAX_PACKET_SIZE + 2];  // The first byte is the length.
static volatile uint8 DATA radioNetworkRxMainLoopIndex = 0;   // The index of the next rxBuffer to read from the main loop.
static volatile uint8 DATA radioNetworkRxInterruptIndex = 0;  // The index of the next rxBuffer to write to when a packet comes from the radio.

/* txPackets are handled similarly (this are the packets that start from this node)*/
#define TX_PACKET_COUNT 8
static volatile uint8 XDATA radioNetworkTxPacket[TX_PACKET_COUNT][1 + RADIO_MAX_PACKET_SIZE];  // The first byte is the length.
static volatile uint8 DATA radioNetworkTxMainLoopIndex = 0;   // The index of the next txPacket to write to in the main loop.
static volatile uint8 DATA radioNetworkTxInterruptIndex = 0;  // The index of the current txPacket we are trying to send on the radio.

/* txPackets are handled similarly (this are the packets that just pass through this node for routing)*/
#define TX_PACKET_EXTERNAL_COUNT 8
static volatile uint8 XDATA radioExternalTxPacket[TX_PACKET_COUNT][1 + RADIO_MAX_PACKET_SIZE];  // The first byte is the length.
static volatile uint8 DATA radioExternalTxManagerIndex = 0;   // The index of the next txPacket to write to in the routing manager.
static volatile uint8 DATA radioExternalTxInterruptIndex = 0;  // The index of the current txPacket we are trying to send on the radio.

/* Routing table
 * address => [next_node, options]
 * next_node = 0 => not reachable
 * next_node == address => directly connected
 *
 * Options: TTL (count down to avoid to keep dead routes) HOP COUNT (to allow to choose the shortest path)
 */
#define ROUTING_TABLE_RECORD_SIZE 2
//256 - 1 (zero) -1 (255) -1 (this node)
#define ROUTING_TABLE_RECORD_COUNT 254
#define ROUTING_TABLE_ADDRESS_OFFSET 0
#define ROUTING_TABLE_OPTIONS_OFFSET 1
#define ROUTING_TABLE_TTL_OFFSET 0
#define ROUTING_TABLE_TTL_MASK 0x03
#define ROUTING_TABLE_TTL_MAX 0x03
#define ROUTING_TABLE_HOP_COUNT_OFFSET 2
#define ROUTING_TABLE_HOP_COUNT_MASK 0xFC
#define ROUTING_TABLE_HOP_COUNT_MAX 0x3F
#define GET_HOP_COUNT(X) (((X) & ROUTING_TABLE_HOP_COUNT_MASK) >> ROUTING_TABLE_HOP_COUNT_OFFSET)
#define SET_HOP_COUNT(ORIGINAL, X) ((((X) << ROUTING_TABLE_HOP_COUNT_OFFSET) & ROUTING_TABLE_HOP_COUNT_MASK) | (ORIGINAL & ROUTING_TABLE_TTL_MASK))
#define GET_TTL(X) (((X) & ROUTING_TABLE_TTL_MASK) >> ROUTING_TABLE_TTL_OFFSET)
#define SET_TTL(ORIGINAL, X) ((((X) << ROUTING_TABLE_TTL_OFFSET) & ROUTING_TABLE_TTL_MASK) | (ORIGINAL & ROUTING_TABLE_HOP_COUNT_MASK))
static volatile uint8 XDATA radioNetworkRoutingTable[ROUTING_TABLE_RECORD_COUNT][ROUTING_TABLE_RECORD_SIZE];
static volatile uint32 DATA lastUpdate = 0;
static volatile uint8 DATA nextIndex = 0;

//protocols
#define ROUTING_TABLE 0

/* This allow the TX event to remember if the last packet was from MAIN_LOOP or from the ROTING MANAGER */
#define MAIN_LOOP_PACKET 0
#define EXTERNAL_PACKET 1
BIT lastPacket = MAIN_LOOP_PACKET;

/* GENERAL FUNCTIONS **********************************************************/

void radioNetworkInit()
{
    randomSeedFromSerialNumber();

    PKTLEN = RADIO_MAX_PACKET_SIZE;
    CHANNR = param_radio_channel;
    memset(radioNetworkRoutingTable,0, ROUTING_TABLE_RECORD_SIZE * ROUTING_TABLE_RECORD_COUNT);
    
    radioMacInit();
    radioAddressInit();
    setAddressCheckHWConfiguration(OneBroadcastCheck);
    radioMacStrobe();
    timeInit();
    lastUpdate = getMs();
    nextIndex = 0;
}

// Returns a random delay in units of 0.922 ms (the same units of radioMacRx).
// This is used to decide when to next transmit a queued data packet.
static uint8 randomTxDelay()
{
    return 1 + (randomNumber() & 3);
}

/* TX FUNCTIONS (called by higher-level code in main loop) ********************/

uint8 radioNetworkTxAvailable(void)
{
    // Assumption: TX_PACKET_COUNT is a power of 2
    return (radioNetworkTxInterruptIndex - radioNetworkTxMainLoopIndex - 1) & (TX_PACKET_COUNT - 1);
}

uint8 radioNetworkTxQueued(void)
{
    return (radioNetworkTxMainLoopIndex - radioNetworkTxInterruptIndex) & (TX_PACKET_COUNT - 1);
}

uint8 XDATA * radioNetworkTxCurrentPacket()
{
    if (!radioNetworkTxAvailable())
    {
        return 0;
    }

    return radioNetworkTxPacket[radioNetworkTxMainLoopIndex];
}

/* TX FUNCTIONS (called by external packate manager code) ********************/

uint8 radioExternalTxAvailable(void)
{
    // Assumption: TX_PACKET_COUNT is a power of 2
    return (radioExternalTxInterruptIndex - radioExternalTxManagerIndex - 1) & (TX_PACKET_EXTERNAL_COUNT - 1);
}

uint8 radioExternalTxQueued(void)
{
    return (radioExternalTxManagerIndex - radioExternalTxInterruptIndex) & (TX_PACKET_COUNT - 1);
}

uint8 XDATA * radioExternalTxCurrentPacket()
{
    if (!radioExternalTxAvailable())
    {
        return 0;
    }

    return radioExternalTxPacket[radioExternalTxManagerIndex];
}

void radioNetworkTxSendPacket(void)
{
    uint8 XDATA * currentTxPacket = radioNetworkRxPacket[radioNetworkTxMainLoopIndex];
    //if (currentTxPacket[NODE_ADDRESS] != param_address) // do not send packet to myself
    //{
        // Update our index of which packet to populate in the main loop.
        if (radioNetworkTxMainLoopIndex == TX_PACKET_COUNT - 1)
        {
            radioNetworkTxMainLoopIndex = 0;
        }
        else
        {
            radioNetworkTxMainLoopIndex++;
        }
    //}

    // Make sure that radioMacEventHandler runs soon so it can see this new data and send it.
    // This must be done LAST.
    radioMacStrobe();
}

void radioExternalTxSendPacket(void)
{
    uint8 XDATA * currentTxPacket = radioExternalTxPacket[radioExternalTxManagerIndex];
    if (currentTxPacket[NODE_ADDRESS] != (uint8)param_address) // do not send packet to myself
    {
        // Update our index of which packet to populate in the main loop.
        if (radioExternalTxManagerIndex == TX_PACKET_EXTERNAL_COUNT - 1)
        {
            radioExternalTxManagerIndex = 0;
        }
        else
        {
            radioExternalTxManagerIndex++;
        }
    }
    // Make sure that radioMacEventHandler runs soon so it can see this new data and send it.
    // This must be done LAST.
    radioMacStrobe();
}

/* RX FUNCTIONS (called by higher-level code in main loop) ********************/

uint8 XDATA * radioNetworkRxCurrentPacket(void)
{
    if (radioNetworkRxMainLoopIndex == radioNetworkRxInterruptIndex)
    {
        return 0;
    }
    return radioNetworkRxPacket[radioNetworkRxMainLoopIndex];
}

void radioNetworkRxDoneWithPacket(void)
{
    if (radioNetworkRxMainLoopIndex == RX_PACKET_COUNT - 1)
    {
        radioNetworkRxMainLoopIndex = 0;
    }
    else
    {
        radioNetworkRxMainLoopIndex++;
    }
}

/* FUNCTIONS CALLED IN RF_ISR *************************************************/

BIT radioNetworkService();

static void takeInitiative()
{
    //Network packates have priority
    if (radioExternalTxInterruptIndex != radioExternalTxManagerIndex)
    {
        lastPacket = EXTERNAL_PACKET;
        // Try to send the next data packet.
        radioMacTx(radioExternalTxPacket[radioExternalTxInterruptIndex]);
    }
    else if (radioNetworkTxInterruptIndex != radioNetworkTxMainLoopIndex)
    {
        lastPacket = MAIN_LOOP_PACKET;
        // Try to send the next data packet.
        radioMacTx(radioNetworkTxPacket[radioNetworkTxInterruptIndex]);
    }
    else
    {
        if (!radioNetworkService()) //try network services
        {
            radioMacRx(radioNetworkRxPacket[radioNetworkRxInterruptIndex], 0);
        }
    }
}

BIT managePacket(uint8 *packet);

void radioMacEventHandler(uint8 event) // called by the MAC in an ISR
{
    if (event == RADIO_MAC_EVENT_STROBE)
    {
        takeInitiative();
        return;
    }
    else if (event == RADIO_MAC_EVENT_TX)
    {
        if (lastPacket == MAIN_LOOP_PACKET)
        {
            // Give ownership of the current TX packet back to the main loop by updated radioNetworkTxInterruptIndex.
            if (radioNetworkTxInterruptIndex == TX_PACKET_COUNT - 1)
            {
                radioNetworkTxInterruptIndex = 0;
            }
            else
            {
                radioNetworkTxInterruptIndex++;
            }
        } else { //EXTERNAL_PACKET
            if (radioExternalTxInterruptIndex == TX_PACKET_EXTERNAL_COUNT - 1)
            {
                radioExternalTxInterruptIndex = 0;
            }
            else
            {
                radioExternalTxInterruptIndex++;
            }
        }

        // We sent a packet, so now let's give another party a chance to talk.
        radioMacRx(radioNetworkRxPacket[radioNetworkRxInterruptIndex], randomTxDelay());
        return;
    }
    else if (event == RADIO_MAC_EVENT_RX)
    {
        uint8 XDATA * currentRxPacket = radioNetworkRxPacket[radioNetworkRxInterruptIndex];
        if (!radioCrcPassed())
        {
            if (radioNetworkTxInterruptIndex != radioNetworkTxMainLoopIndex && radioExternalTxInterruptIndex != radioExternalTxManagerIndex)
            {
                radioMacRx(currentRxPacket, randomTxDelay());
            }
            else
            {
                radioMacRx(currentRxPacket, 0);
            }
            return;
        }

        if (currentRxPacket[RADIO_NETWORK_PACKET_LENGTH_OFFSET] > 2) // 3 address
        {
            // We received a packet that contains actual data.
            //filter packets used by the network manager
            if (!managePacket(currentRxPacket))
            {
                //it was not a network package
                uint8 nextradioNetworkRxInterruptIndex;
                // See if we can give the data to the main loop.
                if (radioNetworkRxInterruptIndex == RX_PACKET_COUNT - 1)
                {
                    nextradioNetworkRxInterruptIndex = 0;
                }
                else
                {
                    nextradioNetworkRxInterruptIndex = radioNetworkRxInterruptIndex + 1;
                }

                if (nextradioNetworkRxInterruptIndex != radioNetworkRxMainLoopIndex)
                {
                    // We can accept this packet!
                    radioNetworkRxInterruptIndex = nextradioNetworkRxInterruptIndex;
                }
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

BIT isNodeReachable(uint8 address)
{
    if (address == param_address) return 0; //Very strange
    if (address == 0) return 0; //Very strange
    if (address > param_address) address -= 1; // the current node is not inside the table
    address -= 1; //zero is not stored
    if (radioNetworkRoutingTable[address][ROUTING_TABLE_ADDRESS_OFFSET] == 0) return 0; //Not reachable I'll stop it during TX
    return 1;
}

uint8 getNextNodeAddress(uint8 address)
{
    if (address == param_address) return param_address; //Very strange in this way i can filter during TX;
    if (address == 0) return param_address; //Very strange in this way i can filter during TX;
    if (address > param_address) address -= 1; // the current node is not inside the table
    address -= 1; //zero is not stored;
    if (radioNetworkRoutingTable[address][ROUTING_TABLE_ADDRESS_OFFSET] == 0) return param_address; //Not reachable I'll stop it during TX
    return radioNetworkRoutingTable[address][ROUTING_TABLE_ADDRESS_OFFSET];
}

void radioNetworkFillAddress(void)
{
    radioNetworkTxPacket[radioNetworkTxMainLoopIndex][SOURCE_ADDRESS] = param_address;
    radioNetworkTxPacket[radioNetworkTxMainLoopIndex][NODE_ADDRESS] = getNextNodeAddress(radioNetworkTxPacket[radioNetworkTxMainLoopIndex][DESTINATION_ADDRESS]);
}

BIT managePacket(uint8 *packet)
{
    uint8 address;
    uint8 i;
    // 0 Broadcast is used by the network manager
    if (packet[NODE_ADDRESS] == 0)
    {
        // DESTINATION_ADDRESS contains options
        if (packet[DESTINATION_ADDRESS] == ROUTING_TABLE) // possible protocol
        {
            if (nextIndex >= ROUTING_TABLE_RECORD_COUNT) // I'm in the middle of table update I'll avoid problems with the buffer
            {
                address = packet[SOURCE_ADDRESS];
                if (address > param_address)
                {
                    address -= 2; // the current node is not inside the table
                }
                else
                {
                    address -= 1;
                }
                radioNetworkRoutingTable[address][ROUTING_TABLE_ADDRESS_OFFSET] = packet[SOURCE_ADDRESS];
                radioNetworkRoutingTable[address][ROUTING_TABLE_OPTIONS_OFFSET] = ROUTING_TABLE_TTL_MAX; //no hop ttl max
                for (i = HEADER_LENGTH + 1; i + ROUTING_TABLE_OPTIONS_OFFSET <= packet[RADIO_NETWORK_PACKET_LENGTH_OFFSET]; i += ROUTING_TABLE_RECORD_SIZE)
                {
                    address = packet[i + ROUTING_TABLE_ADDRESS_OFFSET];
                    if (address == param_address) continue; //this node is not checked
                    if (address > param_address)
                    {
                        address -= 2; // the current node is not inside the table
                    }
                    else
                    { 
                        address -= 1;
                    }
                    if (radioNetworkRoutingTable[address][ROUTING_TABLE_ADDRESS_OFFSET] == 0) // no available routes
                    {
                        radioNetworkRoutingTable[address][ROUTING_TABLE_ADDRESS_OFFSET] = packet[SOURCE_ADDRESS]; //the source address
                        radioNetworkRoutingTable[address][ROUTING_TABLE_OPTIONS_OFFSET] = (packet[i + ROUTING_TABLE_OPTIONS_OFFSET] & ROUTING_TABLE_HOP_COUNT_MASK) | ROUTING_TABLE_TTL_MAX; //hop from packet ttl max
                    }
                    else
                    {
                        if (radioNetworkRoutingTable[address][ROUTING_TABLE_ADDRESS_OFFSET] != packet[i + ROUTING_TABLE_ADDRESS_OFFSET]) //not directly connected
                        {
                            if (GET_HOP_COUNT(radioNetworkRoutingTable[address][ROUTING_TABLE_ADDRESS_OFFSET]) >= GET_HOP_COUNT(packet[i + ROUTING_TABLE_OPTIONS_OFFSET])) //the shorter or the newer
                            {
                                radioNetworkRoutingTable[address][ROUTING_TABLE_ADDRESS_OFFSET] = packet[SOURCE_ADDRESS]; //the source address
                                radioNetworkRoutingTable[address][ROUTING_TABLE_OPTIONS_OFFSET] = (packet[i + ROUTING_TABLE_OPTIONS_OFFSET] & ROUTING_TABLE_HOP_COUNT_MASK) | ROUTING_TABLE_TTL_MAX;//hop from packet ttl max
                            }
                        }
                    }
                }
            }
        }
        return 1;
    }
    if (packet[DESTINATION_ADDRESS] != param_address) //not for this node
    {
        if (isNodeReachable(packet[DESTINATION_ADDRESS]))
        {
            uint8 *txPacket = radioExternalTxCurrentPacket();
            if (txPacket)
            {
                packet[NODE_ADDRESS] = getNextNodeAddress(packet[DESTINATION_ADDRESS]); //change the next node address;
                memcpy(txPacket, packet, packet[RADIO_NETWORK_PACKET_LENGTH_OFFSET]+1);
                radioExternalTxSendPacket();
            }
        }
        return 1;
    }
    return 0;
}

BIT radioNetworkService(void)
{
    uint8 index;
    uint8 ttl;
    if (getMs() - lastUpdate > 20000) // 20 seconds
    {
        if (nextIndex >= ROUTING_TABLE_RECORD_COUNT)
        {
            for (index = 0; index < ROUTING_TABLE_RECORD_COUNT; index++)
            {
                if (radioNetworkRoutingTable[index][ROUTING_TABLE_ADDRESS_OFFSET] == 0) continue; // not reachable
                ttl = GET_TTL(radioNetworkRoutingTable[index][ROUTING_TABLE_OPTIONS_OFFSET]);
                if (ttl == 0) // it is dead
                {
                    radioNetworkRoutingTable[index][ROUTING_TABLE_ADDRESS_OFFSET] = 0; //not reachable;
                }
                else
                {
                    radioNetworkRoutingTable[index][ROUTING_TABLE_OPTIONS_OFFSET] = SET_TTL(radioNetworkRoutingTable[index][ROUTING_TABLE_OPTIONS_OFFSET], ttl - 1);
                }
            }
            nextIndex = 0;
        }
        else
        {
            uint8 *packet = radioExternalTxCurrentPacket();
            while (packet != 0 && (nextIndex < ROUTING_TABLE_RECORD_COUNT))
            {
                uint8 len = 3;
                packet[NODE_ADDRESS] = 0;
                packet[DESTINATION_ADDRESS] = ROUTING_TABLE;
                packet[SOURCE_ADDRESS] = param_address;
                while (len + ROUTING_TABLE_RECORD_SIZE <= RADIO_MAX_PACKET_SIZE && (nextIndex < ROUTING_TABLE_RECORD_COUNT)) //there is space for another element
                {
                    uint8 address;
                    uint8 hop;
                    while (
                        (radioNetworkRoutingTable[nextIndex][ROUTING_TABLE_ADDRESS_OFFSET] == 0 
                        || GET_HOP_COUNT(radioNetworkRoutingTable[nextIndex][ROUTING_TABLE_OPTIONS_OFFSET]) >= ROUTING_TABLE_HOP_COUNT_MAX
                        ) 
                        && (nextIndex < ROUTING_TABLE_RECORD_COUNT)
                    )
                    {
                        nextIndex++;
                    }
                    if (nextIndex == ROUTING_TABLE_RECORD_COUNT) break; //ended
                    address = nextIndex + 1;
                    if (address >= param_address) address++;
                    packet[len + 1 + ROUTING_TABLE_ADDRESS_OFFSET] = address;
                    hop = GET_HOP_COUNT(radioNetworkRoutingTable[nextIndex][ROUTING_TABLE_OPTIONS_OFFSET]);
                    packet[len + 1 + ROUTING_TABLE_OPTIONS_OFFSET] = SET_HOP_COUNT(0, hop + 1);
                    len += ROUTING_TABLE_RECORD_SIZE;
                    nextIndex++;
                }
                packet[RADIO_NETWORK_PACKET_LENGTH_OFFSET] = len;
                radioExternalTxSendPacket();
                packet = radioExternalTxCurrentPacket();
            }
            if (nextIndex == ROUTING_TABLE_RECORD_COUNT)
            {
                lastUpdate = getMs();
            }
            return 1;
        }
    }
    return 0;
}
