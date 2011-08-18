/** wireless_adc_rx app:

This app wirelessly receives ADC (Analog-to-Digital Converter) readings from
Wixels running the wireless_adc_tx app, formats them in ASCII, and sends them
to the virtual COM port so you can see them on a PC.

You can use this app, along with the wirless_adc_tx transmitter app, to remotely
read the value of any voltage.  For example, the voltage could be from the
analog output of any sensor, such as an accelerometer, gyro, or distance sensor.

This app supports having multiple transmitters and multiple receivers.

NOTE: The Wixel's ADC inputs can only tolerate voltages between 0 (GND) and
3.3 V.  If you need to measure voltages over a wider range, you will need
additional circuitry (such as a voltage divider).

== Getting Started ==

First, load the wireless_adc_tx app onto one or more Wixels and connect each
analog voltage output you wish to measure to one of the 6 ADC channels present
on each Wixel: P0_0, P0_1, P0_2, P0_3, P0_4, or P0_5.  Make sure that the ground
(GND) pins of the Wixel and the device producing the analog voltage are connected,
and ensure that everything is sufficiently powered.

Second, load the wireless_adc_rx app onto a Wixel and connect it to a
PC via USB.  Open the COM port of that Wixel using a terminal program.
The terminal programs should print (display) a steady stream of lines that look
something like this:

07-37-8B-69 12697   226   226   1501   226   226   226

The first number (07-37-8B-69) is the serial number of the Wixel that
measured the votlages.
The second number is a 16-bit measure of the time when the receiver
processed the packet, in milliseconds.
The third through seventh numbers are the respective voltages on the
P0_0, P0_1, P0_2, P0_3, P0_4, and P0_5, in units of millivolts.

*/

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
