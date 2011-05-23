/** test_hid_app:

This is app tests the USB Human Interface Device (HID) library that allows
the Wixel to emulate a mouse, keyboard, and joystick simultaneously.

The yellow LED shows whether Caps Lock is turned on.  This might not work if
the USB host is a Linux or Mac OS machine.

The code in keyboardService() demonstrates how to send a sequence of characters
to the computer as fast as possible.


== Default Pinout ==

P0_0 = Left Mouse Button input (active low, pulled up internally)
P0_1 = Right Mouse Button input (active low, pulled up internally)
P0_2 = Button to trigger keyboard input (active low, pulled up internally)


== Parameters ==

move_cursor: Setting this to 1 will make the app move the cursor in a square
    path.
  
move_mouse_wheel: Setting this to 1 will make the app move the virtual mouse
    wheel up and down.

move_joystick:  Settings this to 1 will make the app move the virtual joystick
    and press all the joystick buttons in sequence.
*/

#include <wixel.h>
#include <usb.h>
#include <usb_hid.h>

int32 CODE param_move_cursor = 0;
int32 CODE param_move_mouse_wheel = 0;
int32 CODE param_move_joystick = 0;

uint8 lastKeyCodeSent = 0;

void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(usbHidKeyboardOutput.leds & (1 << LED_CAPS_LOCK));

    // To see the Num Lock or Caps Lock state, you can use these lines instead of the above:
    //LED_YELLOW(usbHidKeyboardOutput.leds & (1 << LED_NUM_LOCK));
    //LED_YELLOW(usbHidKeyboardOutput.leds & (1 << LED_SCROLL_LOCK));

    // NOTE: Reading the Caps Lock, Num Lock, and Scroll Lock states might not work
    // if the USB host is a Linux or Mac OS machine.
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

void joystickService()
{
    uint16 time;
    uint8 tmp;

    if (!param_move_joystick){ return; }

    time = (uint16)getMs();
    tmp = (uint8)(time >> 3);

    usbHidJoystickInput.buttons = 1 << (time >> 8 & 0xF);

    usbHidJoystickInput.x = 0;
    usbHidJoystickInput.y = 0;
    usbHidJoystickInput.z = 0;
    usbHidJoystickInput.rx = 0;
    usbHidJoystickInput.ry = 0;
    usbHidJoystickInput.rz = 0;

    switch(time >> 11 & 7)
    {
    case 0: usbHidJoystickInput.x = tmp; break;
    case 1: usbHidJoystickInput.y = tmp; break;
    case 2: usbHidJoystickInput.z = tmp; break;
    case 3: usbHidJoystickInput.rx = tmp; break;
    case 4: usbHidJoystickInput.ry = tmp; break;
    case 5: usbHidJoystickInput.rz = tmp; break;
    }

    usbHidJoystickInputUpdated = 1;
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
    /* char CODE test[] = "!\"#$%&'()*+,-./0123456789:;<=>? "
            "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_ "
            "`abcdefghijklmnopqrstuvwxyz{|}~"; */
    char CODE greeting[] = "hello world ";
    static uint8 charsLeftToSend = 0;
    static char XDATA * nextCharToSend;

    if (buttonGetSingleDebouncedPress() && charsLeftToSend == 0)
    {
        nextCharToSend = (char XDATA *)greeting;
        charsLeftToSend = sizeof(greeting)-1;

        // Uncomment the 'test' string above and the following line to test more characters.
        //nextCharToSend = (char XDATA *)test; charsLeftToSend = sizeof(test)-1;
    }

    LED_RED(charsLeftToSend > 0);

    // Feed data to the HID library, one character at a time.
    if (charsLeftToSend && !usbHidKeyboardInputUpdated)
    {
        uint8 keyCode = usbHidKeyCodeFromAsciiChar(*nextCharToSend);

        if (keyCode != 0 && keyCode == lastKeyCodeSent)
        {
            // If we need to send the same character twice in a row,
            // send a 0 between them so the compute registers it as
            // two different separate key strokes.
            keyCode = 0;
        }
        else
        {
            nextCharToSend++;
            charsLeftToSend--;
        }

        sendKeyCode(keyCode);
    }

    // Send a 0 to signal the release of the last key.
    if (charsLeftToSend == 0 && lastKeyCodeSent != 0 && !usbHidKeyboardInputUpdated)
    {
        sendKeyCode(0);
    }
}

void main()
{
    systemInit();
    usbInit();

    while(1)
    {
        updateLeds();
        boardService();
        usbHidService();
        updateMouseState();
        keyboardService();
        joystickService();
    }
}
