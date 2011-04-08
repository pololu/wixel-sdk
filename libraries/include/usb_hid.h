#ifndef _USB_HID_H
#define _USB_HID_H

#include "usb_hid_constants.h"
#include <cc2511_types.h>

struct HID_KEYBOARD_OUT_REPORT
{
    uint8 leds;
};

struct HID_KEYBOARD_IN_REPORT
{
    uint8 modifiers;
    uint8; // reserved
    uint8 keyCodes[6];
};

struct HID_MOUSE_IN_REPORT
{
    uint8 buttons;
    int8 x;
    int8 y;
    int8 wheel;
};

extern struct HID_KEYBOARD_OUT_REPORT XDATA usbHidKeyboardOutput;
extern struct HID_KEYBOARD_IN_REPORT XDATA usbHidKeyboardInput;
extern struct HID_MOUSE_IN_REPORT XDATA usbHidMouseInput;

extern BIT usbHidKeyboardInputUpdated;
extern BIT usbHidMouseInputUpdated;

void usbHidService(void);

uint8 hidAsciiCharToKeyCode(char asciiChar);

#endif
