/** wireless_adc_tx app:

This app reads the voltages on its six analog inputs (P0_0, P0_1,
P0_2, P0_3, P0_4, and P0_5) and transmits them wirelessly.

The packet transmitted also contains the serial number of the Wixel,
allowing multiple transmitters to talk to the same receiver.

For more information about how to use this app, see the documentation in
apps/wireless_adc_rx/wireless_adc_rx.c.
*/

// TODO: Add a few ms of randomness to the timing so that we don't get into a
// situation where two Wixels are interfering with each other for every packet?

/** Dependencies **************************************************************/
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <radio_queue.h>


/** Parameters ****************************************************************/

int32 CODE param_input_mode = 0;

int32 CODE param_report_period_ms = 20;


/** Functions *****************************************************************/
void analogInputsInit()
{
    switch(param_input_mode)
    {
    case 1: // Enable pull-up resistors for all pins on Port 0.
        // This shouldn't be necessary because the pull-ups are enabled by default.
        P2INP &= ~(1<<5);  // PDUP0 = 0: Pull-ups on Port 0.
        P0INP = 0;
        break;

    case -1: // Enable pull-down resistors for all pins on Port 0.
        P2INP |= (1<<5);   // PDUP0 = 1: Pull-downs on Port 0.
        P0INP = 0;         // This line should not be necessary because P0SEL is 0 on reset.
        break;

    default: // Disable pull-ups and pull-downs for all pins on Port 0.
        P0INP = 0x3F;
        break;
    }
}

void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(1);
    LED_RED(0);
}

// This function should be called regularly.
// It takes care of reading the ADC values and sending them
// to the radio when appropriate.
void adcToRadioService()
{
    static uint16 lastTx = 0;

    uint8 XDATA * txPacket;

    // Check to see if it is time to send a report and
    // if there is a radio TX buffer available.
    if ((uint16)(getMs() - lastTx) >= param_report_period_ms && (txPacket = radioQueueTxCurrentPacket()))
    {
        // Both of those conditions are true, so send a report.

        uint8 i;
        uint16 XDATA * ptr = (uint16 XDATA *)&txPacket[5];

        // This should be done before all the ADC readings, which take about 3 ms.
        lastTx = getMs();

        // Byte 0 is the length.
        txPacket[0] = 16;

        // Bytes 1-4 are the serial number.
        txPacket[1] = serialNumber[0];
        txPacket[2] = serialNumber[1];
        txPacket[3] = serialNumber[2];
        txPacket[4] = serialNumber[3];

        adcSetMillivoltCalibration(adcReadVddMillivolts());

        // Bytes 5-16 are the ADC readings on channels 0-6.        
        for (i = 0; i < 6; i++)
        {
            *(ptr++) = adcConvertToMillivolts(adcRead(i));
        }

        radioQueueTxSendPacket();
    }
}

void main(void)
{
    systemInit();
    analogInputsInit();
    usbInit();
    radioQueueInit();

    while(1)
    {
        updateLeds();
        boardService();
        usbComService();
        adcToRadioService();
    }
}
