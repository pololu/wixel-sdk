#ifndef _USB_HID_H
#define _USB_HID_H

#include <cc2511_types.h>

// TODO: better name for this struct, like HID_MOUSE_REPORT?
struct MOUSE_IN_REPORT
{
    uint8 buttons;
    int8 x;
    int8 y;
    int8 wheel;
};

typedef struct KEYBOARD_IN_REPORT {
    uint8 modifiers;
    uint8 reserved;
    uint8 keyCodes[6];
};

extern XDATA struct MOUSE_IN_REPORT hidMouseInReport;
extern XDATA struct KEYBOARD_IN_REPORT hidKeyboardInReport;

void usbHidService(void);

#endif
