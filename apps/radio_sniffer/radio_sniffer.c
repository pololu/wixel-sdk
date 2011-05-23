/** radio_sniffer app:

This app shows the radio packets being transmitted by other Wixels on the
same channel.


== Description ==

A Wixel running this app appears to the USB host as a Virtual COM Port,
with USB product ID 0x2200.  To view the output of this app, connect to
the Wixel's virtual COM port using a terminal program.  Be sure to set your
terminal's line width to 120 characters or more to avoid line wrapping.
 
The app uses the radio_queue libray to receive packets.  It does not
transmit any packets.

The output from this app takes the following format:

147> "hello world!"       ! R: -50 L: 104 s:0 PING  p:0 0D0068656C6C6F20776F726C64212A68
 (1)      (2)            (3)  (4)    (5)  (6)  (7)  (8)    (9)

(1) index (line number)
(2) ASCII representation of packet contents (unprintable bytes are replaced with '?')
(3) '!' indicates packet failed CRC check
(4) RSSI
(5) LQI
(6) sequence bit (only applies to RF communications using radio_link)
(7) packet type (only applies to RF communications using radio_link)
(8) payload type (only applies to RF communications using radio_link)
(9) hexadecimal representation of raw packet contents, including length byte
    and any header bytes at beginning

The red LED indicates activity on the radio channel (packets being received).
Since every radio packet has a chance of being lost, there is no guarantee
that this app will pick up all the packets being sent, and some of
what it does pick up will be corrupted (indicated by a failed CRC check).


== Parameters ==

radio_channel: See description in radio_link.h.
*/

/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>
#include <random.h>
#include <time.h>

#include <usb.h>
#include <usb_com.h>
#include <radio_queue.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/** Functions *****************************************************************/
void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(radioQueueRxCurrentPacket());

    LED_RED(0);
}

// This is called by printf and printPacket.
void putchar(char c)
{
    usbComTxSendByte(c);
}

char nibbleToAscii(uint8 nibble)
{
    nibble &= 0xF;
    if (nibble <= 0x9){ return '0' + nibble; }
    else{ return 'A' + (nibble - 0xA); }
}

void printPacket(uint8 XDATA * pkt)
{
    static uint16 pkt_count = 0;
    uint8 j, len;

    // index
    printf("%3d> ", pkt_count++);
    if (pkt_count > 999)
        pkt_count = 0;

    len = pkt[0];

    // ASCII packet data
    putchar('"');
    for (j = 1; j <= len; j++)
    {
        if (isprint(pkt[j]))
        {
            putchar(pkt[j]);
        }
        else
        {
            putchar('?');
        }
    }
    putchar('"');

    // pad with spaces
    for (; j <= RADIO_QUEUE_PAYLOAD_SIZE; j++)
    {
        putchar(' ');
    }
    putchar(' ');

    // CRC
    putchar((pkt[len + 2] & 0x80) ? ' ' : '!');
    putchar(' ');

    // RSSI, LQI
    printf("R:%4d ", (int8)(pkt[len + 1])/2 - 71);
    printf("L:%4d ", pkt[len + 2] & 0x7F);

    // sequence number
    printf("s:%1d ", pkt[1] & 0x1);

    // packet type
    switch((pkt[1] >> 6) & 3)
    {
        case 0: printf("PING  "); break;
        case 1: printf("NAK   "); break;
        case 2: printf("ACK   "); break;
        case 3: printf("RESET "); break;
    }

    // payload type
    putchar('p');
    putchar(':');
    putchar(nibbleToAscii(pkt[1] >> 1 & 0xF));
    putchar(' ');
    
    // packet contents in hex
    for(j = 0; j < len + 1; j++)  // add 1 for length byte
    {
        putchar(nibbleToAscii(pkt[j] >> 4));
        putchar(nibbleToAscii(pkt[j]));
    }
    putchar('\r');
    putchar('\n');
}

void printPacketIfNeeded()
{
    uint8 XDATA * packet;
    if ((packet = radioQueueRxCurrentPacket()) && usbComTxAvailable() >= 128)
    {
        printPacket(packet);
        radioQueueRxDoneWithPacket();
    }
}

void main()
{
    systemInit();
    usbInit();

    radioQueueInit();
    radioQueueAllowCrcErrors = 1;

    while(1)
    {
        boardService();
        updateLeds();
        usbComService();
        printPacketIfNeeded();
    }
}
