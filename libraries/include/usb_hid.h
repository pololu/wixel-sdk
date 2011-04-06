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

extern XDATA struct MOUSE_IN_REPORT mouseInReport;

void usbHidService(void);

#endif
