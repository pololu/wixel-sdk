/*! \file usb_hid.h
 * The <code>usb_hid.lib</code> library implements a composite USB device
 * containing a keyboard interface and a mouse interface using the
 * Human Interface Device (HID) class.
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
 * the \b keyboard interface.
 * If the Wixel is connected to a Windows machine, you can use this struct to
 * determine whether the Caps Lock, Num Lock, or Scroll Lock options are active.
 * This might not work on Linux or Mac OS computers. */
typedef struct HID_KEYBOARD_OUT_REPORT
{
    /*! Keyboard LED indicator data. Each bit contains the state of one
     * indicator (1 = on, 0 = off), with the lowest bit representing Num Lock
     * (usage ID 0x01) and the highest bit representing Do Not Disturb (usage ID
     * 0x08) in the LED usage Page. See usb_hid_constants.h for the meaning
     * of each bit. The keyboard's HID Report Descriptor is defined as
     * <code>keyboardReportDescriptor</code> in usb_hid.c.
     *
     * Example usage:
\code
if (usbHidKeyboardOutput.leds & (1<<LED_CAPS_LOCK))
{
    // The Caps Lock LED is on.
}
\endcode
     *
     * This might not work on Linux or Mac OS computers.
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

/*! \struct HID_JOYSTICK_IN_REPORT
 * This struct contains \b input data sent in HID reports from the
 * device's joystick interface to the host. */
typedef struct HID_JOYSTICK_IN_REPORT
{
    int16 x;      /*!< Joystick X axis position.  Valid values are from -32767 to 32767. */
    int16 y;      /*!< Joystick Y axis position.  Valid values are from -32767 to 32767. */
    int16 z;      /*!< Joystick Z axis position.  Valid values are from -32767 to 32767. */
    int16 rx;     /*!< Joystick's rotation about the X axis.  Valid values are from -32767 to 32767. */
    int16 ry;     /*!< Joystick's rotation about the Y axis.  Valid values are from -32767 to 32767. */
    int16 rz;     /*!< Joystick's rotation about the Z axis.  Valid values are from -32767 to 32767. */
    int16 slider; /*!< Joystick's slider position.  Valid values are from -32767 to 32767. */
    int16 dial;   /*!< Joystick's dial position.  Valid values are from -32767 to 32767. */

    uint32 buttons; /*!< A bit map that specifies which buttons are pressed. */
} HID_JOYSTICK_IN_REPORT;

/*! Contains \b output data received by the \b keyboard interface from the host.
 * If the Wixel is connected to a Windows machine, you can use this variable to
 * determine whether the Caps Lock, Num Lock, or Scroll Lock options are active.
 * This might not work on Linux or Mac OS computers.
 * See HID_KEYBOARD_OUT_REPORT for details. */
extern HID_KEYBOARD_OUT_REPORT XDATA usbHidKeyboardOutput;

/*! Contains \b input data to be sent from the \b keyboard interface to the host.
 * You can use this variable to send key presses to the computer.
 * After writing data to this struct, set #usbHidKeyboardInputUpdated to 1 to
 * tell the HID library to send that data to the computer.
 * See HID_KEYBOARD_IN_REPORT for details. */
extern HID_KEYBOARD_IN_REPORT XDATA usbHidKeyboardInput;

/*! Contains \b input data to be sent from the \b mouse interface to the host.
 * You can use this variable to send X, Y and mouse wheel position changes to
 * the computer and report the state of the mouse buttons.
 * After writing data to this variable, set #usbHidMouseInputUpdated to 1 to
 * tell the HID library to send that data to the computer.
 * See HID_MOUSE_IN_REPORT for details. */
extern HID_MOUSE_IN_REPORT XDATA usbHidMouseInput;

/*! Contains \b input data sent in HID reports from the
 * device's joystick interface to the host.
 * You can use this variable to send joystick position and button data to
 * the USB host.
 * After writing data to this variable, set #usbHidJoystickInputUpdated to 1
 * to tell the HID library to send that data to the computer. */
extern HID_JOYSTICK_IN_REPORT XDATA usbHidJoystickInput;

/*! After writing data to #usbHidKeyboardInput, set this bit to 1 to trigger an HID
 * report to be sent from the keyboard interface to the host. This bit is cleared
 * by the library once the report is sent. */
extern BIT usbHidKeyboardInputUpdated;

/*! After writing data to #usbHidMouseInput, set this bit to 1 to trigger an HID
 * report to be sent from the mouse interface to the host. This bit is cleared by the
 * library once the report is sent. */
extern BIT usbHidMouseInputUpdated;

/*! After writing data to #usbHidJoystickInput, set this bit to 1 to trigger an HID
 * report to be sent from the joystick interface to the host. This bit is cleared by the
 * library once the report is sent. */
extern BIT usbHidJoystickInputUpdated;

/*! This must be called regularly if you are implementing an HID device. */
void usbHidService(void);

/*! Converts an ASCII-encoded character into the corresponding HID Key Code,
 * suitable for the keyCodes array in HID_KEYBOARD_IN_REPORT.
 * Note that many pairs of ASCII characters map to the same key code because
 * they are on the same key.
 * For example, both '4' and '$' map to 0x21 (KEY_4).
 * To send a dollar sign to the computer, you must set the shift bit of the
 * modifiers byte in the HID_KEYBOARD_IN_REPORT. */
uint8 usbHidKeyCodeFromAsciiChar(char asciiChar);

#endif
