/** wireless_adc_tx app:

This app reads the voltages on its six analog inputs (P0_0, P0_1,
P0_2, P0_3, P0_4, and P0_5) and transmits them wirelessly.
*/

/** Dependencies **************************************************************/
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <radio_queue.h>


/** Parameters ****************************************************************/

int32 CODE param_input_mode = 0; // TODO: implement this

int32 CODE param_report_period_ms = 100;


/** Functions *****************************************************************/
void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(1);
}

void adcToRadioService()
{
    static uint16 lastTx = 0;

    uint8 XDATA * txPacket;

    if ((uint16)(getMs() - lastTx) > param_report_period_ms && (txPacket = radioQueueTxCurrentPacket()))
    {
        uint8 i;
        uint16 XDATA * ptr = (uint16 XDATA *)&txPacket[5];

        // Byte 0 is the length.
        txPacket[0] = 18;

        // Bytes 1-4 are the serial number.
        txPacket[1] = serialNumber[0];
        txPacket[2] = serialNumber[1];
        txPacket[3] = serialNumber[2];
        txPacket[4] = serialNumber[3];

        // Bytes 5-6 are the reading of the Internal 1.25 V reference (ch 13).
        *(ptr++) = adcRead(13);

        // Bytes 7-18 are the ADC readings on channels 0-6.        
        for (i = 0; i < 6; i++)
        {
            *(ptr++) = adcRead(i);
        }

        // TODO: Bytes 19-20 are the ADC reading of the temperature sensor (ch 14)

        radioQueueTxSendPacket();
        lastTx = getMs();

        LED_RED(!LED_RED_STATE);
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
        adcToRadioService();
    }
}
