/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>
#include <time.h>

#include <usb.h>
#include <usb_hid.h>
#include <random.h>

void updateLeds()
{
    usbShowStatusWithGreenLed();
}


void main()
{
    uint8 last = 0;

    systemInit();
    usbInit();
    randomSeedFromAdc();

    while(1)
    {
        boardService();
        updateLeds();

        usbHidService();

        if ((uint8)(getMs() - last) > 250)
        {
            mouseInReport.x = (int8)randomNumber() >> 6;
            mouseInReport.y = (int8)randomNumber() >> 6;
            last = getMs();
        }
    }
}
