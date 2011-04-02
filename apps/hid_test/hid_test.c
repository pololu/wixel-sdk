/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>
#include <time.h>

#include <usb.h>

void updateLeds()
{
    usbShowStatusWithGreenLed();
}


void main()
{
    systemInit();
    usbInit();

    while(1)
    {
        boardService();
        updateLeds();

        usbPoll();
    }
}
