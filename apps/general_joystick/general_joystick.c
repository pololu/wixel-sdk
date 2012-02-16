/** general_joystick_app:

== Default Pinout ==

== Parameters ==

*/

#include <wixel.h>
#include <usb.h>
#include <usb_hid.h>

void updateLeds()
{
    usbShowStatusWithGreenLed();
}

uint8 axisUpdate(uint16 value, uint8 last_value)
{
    uint8 shifted = ((uint16)(value >> 3) - 128);
    if(shifted != last_value)
        usbHidJoystickInputUpdated = 1;
    return shifted;
}

void joystickService()
{
    uint16 last_buttons = usbHidJoystickInput.buttons;

    usbHidJoystickInput.buttons = isPinHigh(10);
    usbHidJoystickInput.buttons |= isPinHigh(11) << 1;
    usbHidJoystickInput.buttons |= isPinHigh(12) << 2;
    usbHidJoystickInput.buttons |= isPinHigh(13) << 3;
    usbHidJoystickInput.buttons |= isPinHigh(14) << 4;
    usbHidJoystickInput.buttons |= isPinHigh(15) << 5;
    usbHidJoystickInput.buttons |= isPinHigh(16) << 6;
    usbHidJoystickInput.buttons |= isPinHigh(17) << 7;

    // inverted values on buttons 8-15
    usbHidJoystickInput.buttons |= !isPinHigh(10) << 8;
    usbHidJoystickInput.buttons |= !isPinHigh(11) << 9;
    usbHidJoystickInput.buttons |= !isPinHigh(12) << 10;
    usbHidJoystickInput.buttons |= !isPinHigh(13) << 11;
    usbHidJoystickInput.buttons |= !isPinHigh(14) << 12;
    usbHidJoystickInput.buttons |= !isPinHigh(15) << 13;
    usbHidJoystickInput.buttons |= !isPinHigh(16) << 14;
    usbHidJoystickInput.buttons |= !isPinHigh(17) << 15;

    if(last_buttons != usbHidJoystickInput.buttons)
        usbHidJoystickInputUpdated = 1;

    usbHidJoystickInput.x = axisUpdate(adcRead(0), usbHidJoystickInput.x);
    usbHidJoystickInput.y = axisUpdate(adcRead(1), usbHidJoystickInput.y);
    usbHidJoystickInput.z = axisUpdate(adcRead(2), usbHidJoystickInput.z);
    usbHidJoystickInput.rx = axisUpdate(adcRead(3), usbHidJoystickInput.rx);
    usbHidJoystickInput.ry = axisUpdate(adcRead(4), usbHidJoystickInput.ry);
    usbHidJoystickInput.rz = axisUpdate(adcRead(5), usbHidJoystickInput.rz);
}

void setup()
{
    setDigitalInput(0, HIGH_IMPEDANCE);
    setDigitalInput(1, HIGH_IMPEDANCE);
    setDigitalInput(2, HIGH_IMPEDANCE);
    setDigitalInput(3, HIGH_IMPEDANCE);
    setDigitalInput(4, HIGH_IMPEDANCE);
    setDigitalInput(5, HIGH_IMPEDANCE);

    setPort1PullType(HIGH);
    setDigitalInput(10, PULLED);
    setDigitalInput(11, PULLED);
    setDigitalInput(12, PULLED);
    setDigitalInput(13, PULLED);
    setDigitalInput(14, PULLED);
    setDigitalInput(15, PULLED);
    setDigitalInput(16, PULLED);
    setDigitalInput(17, PULLED);
}

void main()
{
    systemInit();
    usbInit();
    setup();

    while(1)
    {
        updateLeds();
        boardService();
        usbHidService();
        joystickService();
    }
}
