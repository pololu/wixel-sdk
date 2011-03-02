// test_radio_link.c: A little program that lets you test the radio_link library.

#include <cc2511_map.h>
#include <board.h>
#include <usb.h>
#include <usb_com.h>
#include <radio_link.h>
#include <random.h>
#include <stdio.h>

// These prototypes allow us to access the internals of raido_link.  These are not
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

    uint8 XDATA * packet = radioLinkRxCurrentPacket();
    if (packet == 0){ return; }

    if (usbComTxAvailable() < packet[0]*2 + 30){ return; }

    length = sprintf(buffer, "RX: ");
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

void handleCommands()
{
    uint8 XDATA txNotAvailable[] = "TX not available!\r\n";
    uint8 XDATA response[128];
    uint8 responseLength;
    if (usbComRxAvailable() && usbComTxAvailable() >= 50)
    {
        uint8 byte = usbComRxReceiveByte();
        if (byte == (uint8)'?')
        {
            responseLength = sprintf(response, "? RX=%d/%d, TX=%d/%d\r\n",
                    radioLinkRxMainLoopIndex, radioLinkRxInterruptIndex,
                    radioLinkTxMainLoopIndex, radioLinkTxInterruptIndex);
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
                radioLinkTxSendPacket();
                responseLength = sprintf(response, "TX: %02x%02x%02x\r\n", packet[1], packet[2], packet[3]);
                usbComTxSend(response, responseLength);
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

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
