/** wireless_adc_rx app:

This app wirelessly receives ADC (Analog-to-Digital Converter) readings from
Wixels running the wireless_adc_tx app, formats them in ASCII, and sends them
to the virtual COM port so they can be displayed or processed on a PC.

You can use this app, along with the wireless_adc_tx transmitter app, to wirelessly
read the value of any voltage.  For example, the voltage could be from the
analog output of any sensor, such as an accelerometer, gyro, or distance sensor.

This app supports having multiple transmitters and multiple receivers.

NOTE: The Wixel's ADC inputs can only tolerate voltages between 0 (GND) and
3.3 V.  To measure voltages over a wider range, you will need
additional circuitry (such as a voltage divider).

== Getting Started ==

First, load the wireless_adc_tx app onto one or more Wixels and connect each
analog voltage output you wish to measure to one of the 6 ADC channels present
on each Wixel: P0_0, P0_1, P0_2, P0_3, P0_4, or P0_5.  Make sure that the ground
(GND) pins of the Wixel and the device producing the analog voltage are connected,
and ensure that everything is sufficiently powered.

Next, load the wireless_adc_rx app onto a Wixel and connect it to a
PC via USB.  Open the COM port of that Wixel using a terminal program.
The terminal programs should print (display) a steady stream of lines that have
the following format:

07-37-8B-69 12697   226   226   1501  2931   226   226
 (Serial)   (Time)  (0)   (1)    (2)   (3)   (4)   (5)

(Serial) The serial number of the Wixel that measured the voltages.
(Time)   A 16-bit measure of the time when the receiver processed the report, in
         milliseconds (ms).
(0-5)    The voltages measured on P0_0, P0_1, P0_2, P0_3, P0_4, and P0_5 in
         units of millivolts (mV).

To use this data in your application, you can either write PC software that
reads the data from the COM port and does something with it, or you can
modify this app.

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
        // the wireless_adc_tx app.  Format it nicely and send it to
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
