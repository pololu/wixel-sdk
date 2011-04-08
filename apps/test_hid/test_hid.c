/* pinout:
 *
 * P0_0 = Left Mouse Button input
 * P0_1 = Right Mouse Button input
 *
 * P1_0 = Num Lock Output
 * P1_1 = Scroll Lock Output
 * P2_2 = Caps Lock Output (yellow LED)
 */

#include <wixel.h>
#include <usb.h>
#include <usb_hid.h>

int32 CODE param_move_cursor = 1;
int32 CODE param_move_mouse_wheel = 1;

void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(usbHidKeyboardOutput.leds & (1 << LED_CAPS_LOCK));

    setDigitalOutput(10, usbHidKeyboardOutput.leds & (1 << LED_NUM_LOCK));
    setDigitalOutput(11, usbHidKeyboardOutput.leds & (1 << LED_SCROLL_LOCK));
}

void updateMouseState()
{
    usbHidMouseInput.x = 0;
    usbHidMouseInput.y = 0;
    if (param_move_cursor)
    {
        uint8 direction = getMs() >> 9 & 3;
        switch(direction)
        {
        case 0: usbHidMouseInput.x = 3; break;
        case 1: usbHidMouseInput.y = 3; break;
        case 2: usbHidMouseInput.x = -3; break;
        case 3: usbHidMouseInput.y = -3; break;
        }
    }

    if (param_move_mouse_wheel)
    {
        uint8 direction = getMs() >> 10 & 1;
        if (direction)
        {
            usbHidMouseInput.wheel = -1;
        }
        else
        {
            usbHidMouseInput.wheel = 1;
        }
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
    updateMouseState();
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
        while (isPinHigh(2)) periodicTasks();
        while (!isPinHigh(2)) periodicTasks();

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
