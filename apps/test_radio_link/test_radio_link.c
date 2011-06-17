/** test_radio_link app:

This app lets you test the radio_link library.  This app is mainly intended for
people who are debugging the library.
*/

#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <radio_link.h>
#include <random.h>
#include <stdio.h>

// These prototypes allow us to access the internals of radio_link.  These are not
// available in radio_link.h because normal applications should never use them.
extern volatile uint8 DATA radioLinkRxMainLoopIndex;   // The index of the next rxBuffer to read from the main loop.
extern volatile uint8 DATA radioLinkRxInterruptIndex;  // The index of the next rxBuffer to write to when a packet comes from the radio.
extern volatile uint8 DATA radioLinkTxMainLoopIndex;   // The index of the next txPacket to write to in the main loop.
extern volatile uint8 DATA radioLinkTxInterruptIndex;  // The index of the current txPacket we are trying to send on the radio.

void updateLeds()
{
    usbShowStatusWithGreenLed();

    if (MARCSTATE == 0x11)
    {
        LED_RED(1);
    }
    else
    {
        LED_RED(0);
    }

    LED_YELLOW(radioLinkConnected());
}

uint8 nibbleToAscii(uint8 nibble)
{
    nibble &= 0xF;
    if (nibble <= 0x9){ return '0' + nibble; }
    else{ return 'A' + (nibble - 0xA); }
}

void radioToUsb()
{
    uint8 XDATA buffer[128];
    uint8 length;
    uint8 i;
    uint8 XDATA * packet;
    static uint8 CODE resetString[] = "RX: RESET\r\n";

    if ((packet = radioLinkRxCurrentPacket()) && usbComTxAvailable() >= packet[0]*2 + 30)
    {
        length = sprintf(buffer, "RX: %2d ", radioLinkRxCurrentPayloadType());
        for (i = 0; i < packet[0]; i++)
        {
            buffer[length++] = nibbleToAscii(packet[1+i] >> 4);
            buffer[length++] = nibbleToAscii(packet[1+i]);
        }

        buffer[length++] = '\r';
        buffer[length++] = '\n';

        radioLinkRxDoneWithPacket();
        usbComTxSend(buffer, length);
    }

    // Report if we receive a reset packet.
    // NOTE: If multiple reset packets are received faster than we can report them,
    // then this code might only succeed in reporting some of them.
    if (radioLinkResetPacketReceived && usbComTxAvailable() >= sizeof(resetString)-1)
    {
        radioLinkResetPacketReceived = 0;
        usbComTxSend((uint8 XDATA *)resetString, sizeof(resetString)-1);
    }

}

void handleCommands()
{
    uint8 XDATA txNotAvailable[] = "TX not available!\r\n";
    uint8 XDATA response[128];
    uint8 responseLength;
    static uint8 payloadType = 0;

    if (usbComRxAvailable() && usbComTxAvailable() >= 50)
    {
        uint8 byte = usbComRxReceiveByte();
        if (byte == (uint8)'?')
        {
            responseLength = sprintf(response, "? RX=%d/%d, TX=%d/%d, M=%02x\r\n",
                    radioLinkRxMainLoopIndex, radioLinkRxInterruptIndex,
                    radioLinkTxMainLoopIndex, radioLinkTxInterruptIndex, MARCSTATE);
            usbComTxSend(response, responseLength);
        }
        else if (byte >= (uint8)'a' && byte <= (uint8)'g')
        {
            uint8 XDATA * packet = radioLinkTxCurrentPacket();
            if (packet == 0)
            {
                usbComTxSend(txNotAvailable, sizeof(txNotAvailable));
            }
            else
            {
                packet[0] = 3; // Packet length
                packet[1] = byte;
                packet[2] = byte + 1;
                packet[3] = byte + 2;
                radioLinkTxSendPacket(payloadType);
                responseLength = sprintf(response, "TX: %2d %02x%02x%02x\r\n", payloadType, packet[1], packet[2], packet[3]);
                usbComTxSend(response, responseLength);
                if (payloadType == RADIO_LINK_MAX_PAYLOAD_TYPE)
                {
                    payloadType = 0;
                }
                else
                {
                    payloadType++;
                }
            }
        }
    }
}

void main()
{
    systemInit();
    usbInit();

    radioLinkInit();
    randomSeedFromAdc();

    // Set up P1_6 to be the RX debug signal and P1_7 to be the TX debug signal.
    P1DIR |= (1<<6) | (1<<7);
    IOCFG1 = 0b001000; // P1_6 = Preamble Quality Reached
    IOCFG2 = 0b011011; // P1_7 = PA_PD (TX mode)

    while(1)
    {
        boardService();
        updateLeds();
        radioToUsb();
        handleCommands();
        usbComService();
    }
}
