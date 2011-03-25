#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <stdio.h>

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

	if ((uint16)(getMs() - lastReport) >= 500 && usbComTxAvailable() >= sizeof(response))
	{
		uint8 reportLength;
		uint16 result;
		lastReport = (uint16)getMs();
		result = adcRead(0);
		reportLength = sprintf(response, "%d  %02x%02x\r\n", result, ADCH, ADCL);
		usbComTxSend(response, reportLength);
	}
}

void processByte(uint8 byte)
{
	// TODO: add commands for doing extra ADC conversions
	byte;
}

/** Checks for new bytes available on the USB virtual COM port
 * and processes all that are available. */
void processBytesFromUsb()
{
    uint8 bytesLeft = usbComRxAvailable();
    while(bytesLeft && usbComTxAvailable() >= sizeof(response))
    {
        processByte(usbComRxReceiveByte());
        bytesLeft--;
    }
}

void main()
{
    systemInit();
    usbInit();

    while(1)
    {
        boardService();
        updateLeds();
        usbComService();
        processBytesFromUsb();
        reportSender();
    }
}
