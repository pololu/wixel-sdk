/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>
#include <time.h>

#include <usb.h>
#include <usb_hid.h>
#include <adc.h>
#include <gpio.h>

void updateLeds()
{
    usbShowStatusWithGreenLed();
}

void main()
{
    systemInit();
    usbInit();

    setDigitalInput(1, HIGH_IMPEDANCE);
    setDigitalInput(2, HIGH_IMPEDANCE);

    while(1)
    {
        boardService();
        updateLeds();

        usbHidService();

        usbHidMouseInput.x = -((int16)adcRead(2 | ADC_BITS_7) - 1024) / 64;
        usbHidMouseInput.y = ((int16)adcRead(1 | ADC_BITS_7) - 1024) / 64;
        usbHidMouseInput.buttons = !isPinHigh(0);
        usbHidMouseInputUpdated = 1;
    }
}
