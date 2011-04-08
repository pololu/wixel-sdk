/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>
#include <time.h>

#include <usb.h>
#include <usb_hid.h>
#include <gpio.h>

#include <string.h>

void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(usbHidKeyboardOutput.leds & (1 << LED_CAPS_LOCK));
}

void periodicTasks()
{
    updateLeds();
    boardService();
    usbHidService();
}

uint8 charToKeyCode(char c)
{
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 4;
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 4;
    if (c == ' ')
        return 0x2C;

    return 0;
}


void main()
{
    uint8 i;
    char XDATA string[] = "hello world";

    systemInit();
    usbInit();

    while (1)
    {
        while (isPinHigh(0)) periodicTasks();
        while (!isPinHigh(0)) periodicTasks();

        LED_RED(1);
        for (i = 0; i < strlen(string); i++)
        {
            usbHidKeyboardInput.keyCodes[0] = charToKeyCode(string[i]);
            usbHidKeyboardInputUpdated = 1;
            while (usbHidKeyboardInputUpdated) periodicTasks();
            usbHidKeyboardInput.keyCodes[0] = 0;
            usbHidKeyboardInputUpdated = 1;
            while (usbHidKeyboardInputUpdated) periodicTasks();
        }
        LED_RED(0);
    }
}
