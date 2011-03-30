/* radio_sniffer app:
 *
 * Pin out:
 * P1_5 = Radio Transmit Debug Signal
 *
 * == Overview ==
 * This app shows the radio packets being transmitted by other Wixels on the
 * same channel.
 *
 * == Technical Description ==
 * This device appears to the USB host as a Virtual COM Port, with USB product
 * ID 0x2200.
 *
 * The app uses custom radio_link and radio_com libraries that allow it to
 * listen to packets without interfering with communications (it does not
 * transmit anything, including ACKs). The libraries also allow direct access to
 * the full packet contents.
 *
 * The output from this app takes the following format:
 *
 * 147> "hello world!"       ! R: -50 L: 104 s:0 PING  0D0068656C6C6F20776F726C64212A68
 *  (1)      (2)            (3)  (4)    (5)  (6)  (7)      (8)
 *
 * (1) index (line number)
 * (2) ASCII representation of packet contents (unprintable bytes are replaced with '?')
 * (3) '!' indicates packet failed CRC check
 * (4) RSSI
 * (5) LQI
 * (6) sequence bit (only applies to RF communications using radio_link)
 * (7) packet type (only applies to RF communications using radio_link)
 * (8) hexadecimal representation of raw packet contents including status bytes
 *
 * The red LED indicates activity on the radio channel (packets being received).
 * Because the sniffer cannot acknowledge successful reception of data, there
 * is no guarantee that it will pick up all the packets being sent, and some of
 * what it does pick up will be corrupted (indicated by a failed CRC check).
 *
 * == Parameters ==
 *   radio_channel : See description in radio_link.h.
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

/** Parameters ****************************************************************/


/** Functions *****************************************************************/
void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(radioQueueRxCurrentPacket());
}

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

void parsePacket(uint8 XDATA * pkt, char XDATA * buf)
{

    static uint16 pkt_count = 0;
    uint8 i, j, len;
    // index
    i = sprintf(buf, "%3d> ", pkt_count++);
    if (pkt_count > 999)
        pkt_count = 0;

    len = pkt[0];

    // ASCII packet data
    if (len > 0)
    {
        buf[i++] = '"';
        for (j = 1; j <= len; j++)
        {
            if (isprint(pkt[j]))
                buf[i++] = pkt[j];
            else
                buf[i++] = '?';
        }
        buf[i++] = '"';

        // pad with spaces
        for (; j <= RADIO_QUEUE_PAYLOAD_SIZE; j++)
        {
            buf[i++] = ' ';
        }
    }
    else
    {
        for (j = 0; j < (RADIO_QUEUE_PAYLOAD_SIZE + 2); j++)
        {
            buf[i++] = ' ';
        }
    }
    buf[i++] = ' ';

    // CRC
    if (!(pkt[len + 2] & 0x80))
        buf[i++] = '!';
    else
        buf[i++] = ' ';
    buf[i++] = ' ';

    // RSSI, LQI
    i += sprintf(buf + i, "R:%4d ", (int8)(pkt[len + 1])/2 - 71);
    i += sprintf(buf + i, "L:%4d ", pkt[len + 2] & 0x7F);

    // sequence number
    i += sprintf(buf + i, "s:%1d ", pkt[1] & 0x1);

    // packet type
    switch(pkt[1] >> 6)
    {
        case 0:
            i += sprintf(buf + i, "PING  ");
            break;
        case 1:
            i += sprintf(buf + i, "NAK   ");
            break;
        case 2:
            i += sprintf(buf + i, "ACK   ");
            break;
        case 3:
            i += sprintf(buf + i, "RESET ");
            break;
        default:
            i += sprintf(buf + i, "?     ");
            break;
    }

    // packet contents in hex
    for(j = 0; j < RADIO_QUEUE_PAYLOAD_SIZE + 3; j++)  // add 1 for length byte and 2 for status bytes
    {
        if (j < (len + 3))
        {
            buf[i++] = nibbleToAscii(pkt[j] >> 4);
            buf[i++] = nibbleToAscii(pkt[j]);
        }
        else
        {
            buf[i++] = ' ';
            buf[i++] = ' ';
        }
    }
    buf[i++] = '\r';
    buf[i++] = '\n';
}

void main()
{
    uint8 XDATA * pkt;
    char XDATA buf[255];
    uint8 charsToPrint = 0;

    systemInit();
    usbInit();

    radioQueueInit();
    randomSeedFromSerialNumber();

    // Set up P1_5 to be the radio's TX debug signal.
    P1DIR |= (1<<5);
    IOCFG0 = 0b011011; // P1_5 = PA_PD (TX mode)

    while(1)
    {
        boardService();
        updateLeds();

        usbComService();

        // if we aren't currently waiting to print a packet, check if there is a new packet and parse it
        if (!charsToPrint && (pkt = radioQueueRxCurrentPacket()))
        {
            parsePacket(pkt, buf);
            charsToPrint = strlen(buf);
        }

        // if we have a packet to print, try to print it
        if (charsToPrint && (usbComTxAvailable() >= charsToPrint))
        {
            printf(buf);
            charsToPrint = 0;
            radioQueueRxDoneWithPacket();
        }
    }
}
