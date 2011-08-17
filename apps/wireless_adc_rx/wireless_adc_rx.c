/** Dependencies **************************************************************/
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <radio_queue.h>

#include <stdio.h>

/** Types *********************************************************************/

typedef struct adcReport
{
    uint8 length;
    uint8 serialNumber[4];
    uint16 readings[6];
} adcReport;

/** Functions *****************************************************************/
void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(0);
    LED_RED(0);
}

void putchar(char c)
{
    usbComTxSendByte(c);
}

void radioToUsbService()
{
    adcReport XDATA * rxPacket;

    // Check if there is a radio packet to report and space in the
    // USB TX buffers to report it.
    if ((rxPacket = (adcReport XDATA *)radioQueueRxCurrentPacket()) && usbComTxAvailable() >= 64)
    {
        // We received a packet from a Wixel, presumably one running
        // the wireless_adc_rx app.  Format it nicely and send it to
        // the USB host (PC).

        uint8 i;

        printf("%02X-%02X-%02X-%02X %5u",
               rxPacket->serialNumber[3],
               rxPacket->serialNumber[2],
               rxPacket->serialNumber[1],
               rxPacket->serialNumber[0],
               (uint16)getMs()
               );

        for(i = 0; i < 6; i++)
        {
            printf(" %5u", rxPacket->readings[i]);
        }

        putchar('\r');
        putchar('\n');

        radioQueueRxDoneWithPacket();
    }
}

void main(void)
{
    systemInit();
    usbInit();
    radioQueueInit();

    while(1)
    {
        updateLeds();
        boardService();
        usbComService();
        radioToUsbService();
    }
}
