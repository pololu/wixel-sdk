#include <wixel.h>
#include <usb.h>
#include <usb_hid.h>

void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(usbHidKeyboardOutput.leds & (1 << LED_CAPS_LOCK));
}

void mouseService()
{
    uint8 dir = getMs() >> 8 & 3;

    usbHidMouseInput.x = 0;
    usbHidMouseInput.y = 0;

    switch(dir)
    {
    case 0: usbHidMouseInput.x = 2; break;
    case 1: usbHidMouseInput.y = 2; break;
    case 2: usbHidMouseInput.x = -2; break;
    case 3: usbHidMouseInput.y = -2; break;
    }

    usbHidMouseInput.buttons = 0;
    if (!isPinHigh(0))
    {
        // The left mouse button is pressed.
        usbHidMouseInput.buttons |= (1<<MOUSE_BUTTON_LEFT);
    }
    if (!isPinHigh(1))
    {
        // The right mouse button is pressed.
        usbHidMouseInput.buttons |= (1<<MOUSE_BUTTON_RIGHT);
    }

    usbHidMouseInputUpdated = 1;
}

void periodicTasks()
{
    updateLeds();
    boardService();
    usbHidService();
    mouseService();
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
        for (i = 0; i < sizeof(string)-1; i++)
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
