/** test_radio_signal_rx app:

You can load test_radio_signal_rx onto one Wixel and load test_radio_signal_tx
onto another Wixel in order to test the quality of your radio signal.

The transmitter (the Wixel loaded with test_radio_signal_tx) will transmit a
burst of 100 packets every second.

The receivers(s) (the Wixel(s) loaded with test_radio_signal_rx) will listen for
the bursts.  Every time a burst is received, the receiver will send a report to
the USB virtual COM port with three statistics in it:
1) The number of packets successfully received.  This is your packet success
   percentage.  Bigger values are better, and a value of 100 is perfect.
   This is the most important statistic because it determines the amount of
   data that the radios can transfer per second.

2) Average signal strength of the packets received in dBm.  This will be a number
   typically between -100 and -10 that indicates how strong the signal is.  A
   higher number (closer to zero if negative) is better.

3) The average Link Quality Indicator (LQI) of the packets received.  According to
   the CC2511F32 datasheet, the LQI is a metric of the quality of the received signal.
   "LQI is best used as a relative measurement of link quality (a high value indicates
   a better link than what a low value does), since the value is dependent on the
   modulation format."

The radio_channel parameter determines what frequency will be used, and should be the
same in both the transmitter and receiver.
*/

#include <wixel.h>
#include <radio_registers.h>
#include <stdio.h>
#include <usb.h>
#include <usb_com.h>

int32 CODE param_radio_channel = 128;

// This definition should be the same in both test_radio_signal_tx.c and test_radio_signal_rx.c.
#define RADIO_PACKET_SIZE 16

static volatile XDATA uint8 packet[1 + RADIO_PACKET_SIZE + 2];

static uint8 DATA currentBurstId = 0;
static uint8 DATA packetsReceived = 0;
static int16 DATA rssiSum = 0;
static uint16 DATA lqiSum = 0;
static uint8 DATA crcErrors = 0;

static uint16 lastPacketReceivedTime = 0;

void updateLeds()
{
    usbShowStatusWithGreenLed();

    // Turn on the yellow LED if we are in the middle of receiving a burst.
    LED_YELLOW(packetsReceived > 0);

    LED_RED(0);
}

void perTestRxInit()
{
    radioRegistersInit();

    CHANNR = param_radio_channel;

    PKTLEN = RADIO_PACKET_SIZE;

    MCSM0 = 0x14;    // Auto-calibrate when going from idle to RX or TX.
    MCSM1 = 0x00;    // Disable CCA.  After RX, go to IDLE.  After TX, go to IDLE.
    // We leave MCSM2 at its default value.

    dmaConfig.radio.DC6 = 19; // WORDSIZE = 0, TMODE = 0, TRIG = 19

    dmaConfig.radio.SRCADDRH = XDATA_SFR_ADDRESS(RFD) >> 8;
    dmaConfig.radio.SRCADDRL = XDATA_SFR_ADDRESS(RFD);
    dmaConfig.radio.DESTADDRH = (unsigned int)packet >> 8;
    dmaConfig.radio.DESTADDRL = (unsigned int)packet;
    dmaConfig.radio.LENL = 1 + PKTLEN + 2;
    dmaConfig.radio.VLEN_LENH = 0b10000000; // Transfer length is FirstByte+3
    dmaConfig.radio.DC7 = 0x10; // SRCINC = 0, DESTINC = 1, IRQMASK = 0, M8 = 0, PRIORITY = 0

    DMAARM |= (1<<DMA_CHANNEL_RADIO);  // Arm DMA channel
    RFST = 2;                          // Switch radio to RX mode.
}

// RF -> report buffer
void receiveRadioBursts()
{
    if (RFIF & (1<<4))
    {
        if (radioCrcPassed())
        {
            if (packet[2] != currentBurstId)
            {
                currentBurstId = packet[2];

                packetsReceived = 0;
                rssiSum = 0;
                lqiSum = 0;
                crcErrors = 0;
            }

            lastPacketReceivedTime = (uint16)getMs();
            packetsReceived ++;
            rssiSum += radioRssi();
            lqiSum += radioLqi();
        }
        else
        {
            crcErrors ++;
        }

        RFIF &= ~(1<<4);                   // Clear IRQ_DONE
        DMAARM |= (1<<DMA_CHANNEL_RADIO);  // Arm DMA channel
        RFST = 2;                          // Switch radio to RX mode.
    }
}

void reportResults()
{
    static uint16 lastReportSentTime;

    if (usbComTxAvailable() >= 64)
    {
        if (packetsReceived)
        {
            if ((uint16)(getMs() - lastPacketReceivedTime) >= 300)
            {
                uint8 XDATA report[64];
                uint8 reportLength = sprintf(report, "%3d, %5d, %5d\r\n", packetsReceived, rssiSum/packetsReceived, lqiSum/packetsReceived);
                usbComTxSend(report, reportLength);

                lastReportSentTime = (uint16)getMs();
                packetsReceived = 0;
            }
        }
        else
        {
            if ((uint16)(getMs() - lastReportSentTime) >= 1100)
            {
                static uint8 CODE noSignal[] = "  0,     0,     0\r\n";
                usbComTxSend((uint8 XDATA *)noSignal, sizeof(noSignal));
                lastReportSentTime = (uint16)getMs();
            }
        }
    }
}

void main()
{
    systemInit();
    usbInit();
    perTestRxInit();
    usbInit();

    while(1)
    {
        boardService();
        usbComService();
        updateLeds();
        receiveRadioBursts();
        reportResults();
    }
}
