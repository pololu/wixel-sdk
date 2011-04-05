/*
 * usb_hid.h
 *
 *  Created on: Mar 31, 2011
 *      Author: Kevin
 */

#ifndef USB_HID_H_
#define USB_HID_H_

#include <cc2511_types.h>

typedef struct MOUSE_IN_REPORT {
    uint8 buttons;
    int8 x;
    int8 y;
    int8 wheel;
};

extern XDATA struct MOUSE_IN_REPORT mouseInReport;

void usbHidService(void);

#endif /* USB_HID_H_ */
