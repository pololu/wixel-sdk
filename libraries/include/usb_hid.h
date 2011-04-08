/*! \file usb_hid.h
 * The <code>usb_hid.lib</code> library implements a composite USB device
 * containing a keyboard interface and a mouse interface using the HID class.
 *
 * You can find the specification of the USB HID device class in HID1_11.pdf,
 * available for download from USB Implementers Forum at this url:
 * http://www.usb.org/developers/hidpage
 *
 * A complete list of Usage tags used in HID reports, which define the format of
 * the input and output data used by this library, can be found in Hut1_12.pdf
 * on the same page.
 *
 * (A listing of all class specification documents is at
 * http://www.usb.org/devclass_docs)
 */

#ifndef _USB_HID_H
#define _USB_HID_H

#include "usb_hid_constants.h"
#include <cc2511_types.h>

/*! \struct HID_KEYBOARD_OUT_REPORT
 * This struct contains the \b output data sent in HID reports from the host to
 * the \b keyboard interface. */
typedef struct HID_KEYBOARD_OUT_REPORT
{
    /*! Keyboard LED indicator data. Each bit contains the state of one
     * indicator (1 = on, 0 = off), with the lowest bit representing Num Lock
     * (usage ID 0x01) and the highest bit representing Do Not Disturb (usage ID
     * 0x08) in the LED usage Page. See usb_hid_constants.h for the meaning
     * of each bit. The keyboard's HID Report Descriptor is defined as
     * <code>keyboardReportDescriptor</code> in usb_hid.c.
     */
    uint8 leds;
} HID_KEYBOARD_OUT_REPORT;

/*! \struct HID_KEYBOARD_IN_REPORT
 * This struct contains the \b input data sent in HID reports from the \b
 * keyboard interface to the host. */
typedef struct HID_KEYBOARD_IN_REPORT
{
    /*! Keyboard modifier key data: Each bit contains the state of one modifier
     * key  (1 = pressed, 0 = not pressed), with the lowest bit representing
     * Left Control (usage ID 0xE0) and the highest bit representing Right GUI
     * (usage ID 0xE7) in the Keyboard/Keypad usage page. See \ref
     * usb_hid_constants.h for the meaning of each bit. The keyboard's HID
     * Report Descriptor is defined as <code>keyboardReportDescriptor</code> in
     * usb_hid.c. */
    uint8 modifiers;
    uint8 _reserved;
    /*! Keyboard key code data: Each byte contains the key code of one key that
     * is currently pressed (0 = no key). Up to 6 pressed keys at a time can be
     * reported in this way. See \ref usb_hid_constants.h for possible key code
     * values. The keyboard's HID Report Descriptor is defined as
     * <code>keyboardReportDescriptor</code> in usb_hid.c. */
    uint8 keyCodes[6];
} HID_KEYBOARD_IN_REPORT;

/*! \struct HID_MOUSE_IN_REPORT
 * This struct contains the \b output data sent in HID reports from the host to
 * the \b keyboard interface. */
typedef struct HID_MOUSE_IN_REPORT
{
    /*! Mouse button data: Each bit contains the state of one button (1 =
     * pressed, 0 = not pressed), with the lowest bit representing Button 1
     * (usage ID 0x01) and the highest bit representing Button 8 (usage ID
     * 0x08). Buttons 1, 2, and 3 usually correspond to the left, right, and
     * middle buttons on a mouse, respectively. See \ref usb_hid_constants.h for
     * the meaning of each bit. The mouse's HID Report Descriptor is defined as
     * <code>mouseReportDescriptor</code> in usb_hid.c. */
    uint8 buttons;
    /*! Mouse X axis data: This byte contains an 8-bit signed value that
     * represents a change (relative offset) in the horizontal position of the
     * mouse cursor. */
    int8 x;
    /*! Mouse Y axis data: This byte contains an 8-bit signed value that
     * represents a change (relative offset) in the vertical position of the
     * mouse cursor. */
    int8 y;
    /*! Mouse wheel data: This byte contains an 8-bit signed value that
     * represents a change (relative offset) in the position of the mouse wheel.
     */
    int8 wheel;
} HID_MOUSE_IN_REPORT;

/*! The \b output data received by the \b keyboard interface from the host is
 * read from this variable. See HID_KEYBOARD_OUT_REPORT for details. */
extern HID_KEYBOARD_OUT_REPORT XDATA usbHidKeyboardOutput;
/*! The \b input data to be sent from the \b keyboard interface to the host is
 * written to this variable. See HID_KEYBOARD_IN_REPORT for details. */
extern HID_KEYBOARD_IN_REPORT XDATA usbHidKeyboardInput;
/*! The \b input data to be sent from the \b mouse interface to the host is
 * written to this variable. See HID_MOUSE_IN_REPORT for details. */
extern HID_MOUSE_IN_REPORT XDATA usbHidMouseInput;

/*! After writing data in usbHidKeyboardInput, set this bit to trigger an HID
 * report to be sent from the keyboard interface to the host. It is cleared
 * by the library once the report is sent. */
extern BIT usbHidKeyboardInputUpdated;
/*! After writing data in usbHidMouseInput, set this bit to trigger an HID
 * report to be sent from the mouse interface to the host. It is cleared by the
 * library once the report is sent. */
extern BIT usbHidMouseInputUpdated;

/*! This must be called regularly if you are implementing an HID device. */
void usbHidService(void);

#endif
