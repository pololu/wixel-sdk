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

uint8 lastKeyCodeSent = 0;

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

// See keyboardService for an example of how to use this function correctly.
// Assumption: usbHidKeyboardInputUpdated == 0.  Otherwise, this function
// could clobber a keycode sitting in the buffer that has not been sent to
// the computer yet.
// Assumption: usbHidKeyBoardInput[1 through 5] are all zero.
// Assumption: usbHidKeyboardInput.modifiers is 0.
// NOTE: To send two identical characters, you must send a 0 in between.
void sendKeyCode(uint8 keyCode)
{
    lastKeyCodeSent = keyCode;
    usbHidKeyboardInput.keyCodes[0] = keyCode;

    // Tell the HID library to send the new keyboard state to the computer.
    usbHidKeyboardInputUpdated = 1;
}

// NOTE: This function only handles bouncing that occurs when the button is
// going from the not-pressed to pressed state.
BIT buttonGetSingleDebouncedPress()
{
    static BIT reportedThisButtonPress = 0;
    static uint8 lastTimeButtonWasNotPressed = 0;

    if (isPinHigh(2))
    {
        // The P0_2 "button" is not pressed.
        reportedThisButtonPress = 0;
        lastTimeButtonWasNotPressed = (uint8)getMs();
    }
    else if ((uint8)(getMs() - lastTimeButtonWasNotPressed) > 15)
    {
        // The P0_2 "button" has been pressed (or at least P0_2 is shorted
        // to ground) for 15 ms.

        if (!reportedThisButtonPress)
        {
            reportedThisButtonPress = 1;
            return 1;
        }
    }
    return 0;
}

void keyboardService()
{
    char CODE string[] = "hello world ";
    static uint8 charsLeftToSend = 0;
    static char XDATA * nextCharToSend;

    if (buttonGetSingleDebouncedPress() && charsLeftToSend == 0)
    {
        nextCharToSend = (char XDATA *)string;
        charsLeftToSend = sizeof(string)-1;
    }

    LED_RED(charsLeftToSend > 0);

    // Feed data to the HID library, one character at a time.
    if (charsLeftToSend && !usbHidKeyboardInputUpdated)
    {
        uint8 keyCode = hidAsciiCharToKeyCode(*nextCharToSend);

        if (keyCode != 0 && keyCode == lastKeyCodeSent)
        {
            // Avoid sending duplicate a duplicate keycode twice in a row,
            // otherwise the computer will just think the button was held down
            // a little longer and interpet the two characters as a single
            // key press.
            keyCode = 0;
        }
        else
        {
            nextCharToSend++;
            charsLeftToSend--;
        }

        sendKeyCode(keyCode);
    }

    if (charsLeftToSend == 0 && lastKeyCodeSent != 0 && !usbHidKeyboardInputUpdated)
    {
        sendKeyCode(0);
    }
}

void periodicTasks()
{
    updateLeds();
    boardService();
    usbHidService();
    updateMouseState();
    keyboardService();
}

void main()
{
    systemInit();
    usbInit();

    while(1)
    {
        periodicTasks();
    }
}
