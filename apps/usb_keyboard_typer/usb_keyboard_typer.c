#include <wixel.h>
#include <usb.h>
#include <usb_hid.h>
#include "data.h"

uint8 lastKeyCodeSent = 0;
BIT sending = 0;
char XDATA * nextCharToSend;
BIT started = 0;

void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(!sending);
    LED_RED(sending);
}

uint8 usbHidModifiersFromAsciiChar(char c)
{
    if ( (c >= 0x21 && c <= 0x26) ||
             (c >= 0x28 && c <= 0x2B) ||
             (c == ':') ||
             (c == '<') ||
             (c >= 0x3E && c <= 0x5A) ||
             (c >= 0x5E && c <= 0x5F) ||
             (c == '|') ||
             (c == '~')
       )
    {
        return (1<<MODIFIER_SHIFT_LEFT);
    }
    else
    {
        return 0;
    }
}

// See keyboardService for an example of how to use this function correctly.
// Assumption: usbHidKeyboardInputUpdated == 0.  Otherwise, this function
// could clobber a keycode sitting in the buffer that has not been sent to
// the computer yet.
// Assumption: usbHidKeyBoardInput[1 through 5] are all zero.
// Assumption: usbHidKeyboardInput.modifiers is 0.
// NOTE: To send two identical characters, you must send a 0 in between.
void sendKeyCode(uint8 keyCode, uint8 modifiers)
{
    lastKeyCodeSent = keyCode;
    usbHidKeyboardInput.keyCodes[0] = keyCode;
    usbHidKeyboardInput.modifiers = modifiers;

    // Tell the HID library to send the new keyboard state to the computer.
    usbHidKeyboardInputUpdated = 1;
}

void keyboardService()
{
    if (getMs() >= 5000 && !started && !sending)
    {
        started = 1;
        sending = 1;
        nextCharToSend = (char XDATA *)payload_data;
    }

    // Feed data to the HID library, one character at a time.
    if (sending && !usbHidKeyboardInputUpdated)
    {
        uint8 nextChar = *nextCharToSend;
        if (nextChar == 0)
        {
            sending = 0;
        }
        else
        {
            uint8 keyCode = usbHidKeyCodeFromAsciiChar(*nextCharToSend);
            uint8 nextChar = *nextCharToSend;

            if (keyCode != 0 && keyCode == lastKeyCodeSent)
            {
                // If we need to send the same character twice in a row,
                // send a 0 between them so the computer registers it as
                // two different separate key strokes.
                keyCode = 0;
            }
            else
            {
                nextCharToSend++;
            }

            sendKeyCode(keyCode, usbHidModifiersFromAsciiChar(nextChar));
        }
    }

    // Send a 0 to signal the release of the last key.
    if (!sending && lastKeyCodeSent != 0 && !usbHidKeyboardInputUpdated)
    {
        sendKeyCode(0, 0);
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
        keyboardService();
    }
}
