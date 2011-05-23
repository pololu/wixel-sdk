/** test_radio_signal_tx app:

This app is meant to go along with the test_radio_signal_rx app.
See the comments in test_radio_signal_rx.c for information on how
to use this app.
*/

#include <wixel.h>
#include <radio_registers.h>
#include <usb.h>
#include <usb_com.h>

int32 CODE param_radio_channel = 128;

// This definition should be the same in both test_radio_signal_tx.c and test_radio_signal_rx.c.
#define RADIO_PACKET_SIZE 16

static volatile XDATA uint8 packet[1 + RADIO_PACKET_SIZE];

static uint8 currentBurstId = 0;
static uint8 packetsSent = 0;
static uint16 lastBurst = 0;

void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(0);

    // Turn on the red LED if we are sending a burst.
    LED_RED((uint16)(packetsSent < 100));
}

void perTestTxInit()
{
    uint8 i;

    radioRegistersInit();

    CHANNR = param_radio_channel;

    PKTLEN = RADIO_PACKET_SIZE;
    
    MCSM0 = 0x14;    // Auto-calibrate when going from idle to RX or TX.
    MCSM1 = 0x00;    // Disable CCA.  After RX, go to IDLE.  After TX, go to IDLE.
    // We leave MCSM2 at its default value.

    IOCFG2 = 0b011011; // put out a PA_PD signal on P1_7 (active low when the radio is in TX mode)

    dmaConfig.radio.DC6 = 19; // WORDSIZE = 0, TMODE = 0, TRIG = 19

    dmaConfig.radio.SRCADDRH = (unsigned int)packet >> 8;
    dmaConfig.radio.SRCADDRL = (unsigned int)packet;
    dmaConfig.radio.DESTADDRH = XDATA_SFR_ADDRESS(RFD) >> 8;
    dmaConfig.radio.DESTADDRL = XDATA_SFR_ADDRESS(RFD);
    dmaConfig.radio.LENL = 1 + RADIO_PACKET_SIZE;
    dmaConfig.radio.VLEN_LENH = 0b00100000; // Transfer length is FirstByte+1
    dmaConfig.radio.DC7 = 0x40; // SRCINC = 1, DESTINC = 0, IRQMASK = 0, M8 = 0, PRIORITY = 0
    
    for(i = 1; i < sizeof(packet); i++)
    {
        packet[i] = 'A' + i;
    }
    packet[0] = RADIO_PACKET_SIZE;

    RFST = 4;  // Switch radio to Idle mode.
}

void sendRadioBursts()
{
    uint16 time = (uint16)getMs();

    if ((uint16)(time - lastBurst) > 1000)
    {
        lastBurst = time;

        currentBurstId++;
        packetsSent = 0;
    }

    if (packetsSent < 100 && (MARCSTATE == 1))
    {
        packet[1] = packetsSent & 1;
        packet[2] = currentBurstId;
        packet[3] = packetsSent;
        packetsSent++;

        RFIF &= ~(1<<4);                   // Clear IRQ_DONE
        DMAARM |= (1<<DMA_CHANNEL_RADIO);  // Arm DMA channel
        RFST = 3;                          // Switch radio to TX
    }

}

void main()
{
    systemInit();
    usbInit();
    perTestTxInit();

    while(1)
    {
        boardService();
        updateLeds();
        usbComService();
        sendRadioBursts();
    }
}
