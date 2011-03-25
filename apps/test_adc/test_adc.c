#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <stdio.h>

// TODO: use VT100 commands to make a cool bar graph display
// TODO: add temperature and VDD readings

int32 CODE param_report_period_ms = 100;

int32 CODE param_input_mode = 0;

uint8 XDATA response[64];

void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(0);
    LED_RED(0);
}

void reportSender()
{
	static uint16 lastReport;
	uint8 i;
	uint16 result[6];

	if (getMs() - lastReport >= param_report_period_ms && usbComTxAvailable() >= sizeof(response))
	{
		uint8 reportLength;
		lastReport = (uint16)getMs();

		for(i = 0; i < 6; i++)
		{
		    result[i] = adcRead(i);
		}

		reportLength = sprintf(response, "%4d, %4d, %4d, %4d, %4d, %4d\r\n",
		        result[0], result[1], result[2], result[3], result[4], result[5]);
		usbComTxSend(response, reportLength);
	}
}

void processByte(uint8 byte)
{
	// TODO: add commands for doing extra ADC conversions
	byte;
}

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

void main()
{
    systemInit();
    usbInit();
    analogInputsInit();

    while(1)
    {
        boardService();
        updateLeds();
        usbComService();
        reportSender();
    }
}
