/* joystick app:
 *
 * This app allows you to build a custom USB joystick using the Wixel.
 * The joystick appears as a USB HID (Human Interface Device) class
 * device over a (wired) USB connection.
 *
 * Configuration options include:
 *
 * - Mapping of joystick buttons to digital I/O pins
 * - Mapping of joystick axes to analog inputs
 * - Button and axis inversion options
 * - Pull-up/pull-down enable for individual buttons; pull direction 
 *    selectable for all buttons on each port
 *
 * Six analog joystick axes (x, y, z, rx, ry, rz) are mapped to analog
 * pins P0_0 through P0_5 by default. If you don't need all these
 * axes, then the analog pins can be used as digital inputs for
 * additional buttons instead.
 *
 */

#include <wixel.h>
#include <usb.h>
#include <usb_hid.h>

// Button to pin assignment (-1 to disable).
// See gpio.h documentation for details on pin numbers.
int32 CODE param_button1_pin = 12;
int32 CODE param_button2_pin = 13;
int32 CODE param_button3_pin = 14;
int32 CODE param_button4_pin = 15;
int32 CODE param_button5_pin = 16;
int32 CODE param_button6_pin = 17;
int32 CODE param_button7_pin = -1;
int32 CODE param_button8_pin = -1;
int32 CODE param_button9_pin = -1;
int32 CODE param_button10_pin = -1;
int32 CODE param_button11_pin = -1;
int32 CODE param_button12_pin = -1;
int32 CODE param_button13_pin = -1;
int32 CODE param_button14_pin = -1;
int32 CODE param_button15_pin = -1;
int32 CODE param_button16_pin = -1;

// Invert individual button inputs.
// 0: Not inverted (logic low = released; logic high = pressed)
// 1: Inverted (logic high = released; logic low = pressed)
int32 CODE param_button1_invert = 1;
int32 CODE param_button2_invert = 1;
int32 CODE param_button3_invert = 1;
int32 CODE param_button4_invert = 1;
int32 CODE param_button5_invert = 1;
int32 CODE param_button6_invert = 1;
int32 CODE param_button7_invert = 1;
int32 CODE param_button8_invert = 1;
int32 CODE param_button9_invert = 1;
int32 CODE param_button10_invert = 1;
int32 CODE param_button11_invert = 1;
int32 CODE param_button12_invert = 1;
int32 CODE param_button13_invert = 1;
int32 CODE param_button14_invert = 1;
int32 CODE param_button15_invert = 1;
int32 CODE param_button16_invert = 1;

// Axis to ADC channel assignment (-1 to disable).
// Normally, you'll probably want to use channels 0-5 for enabled
//  axes (for the voltages on P0_0 through P0_5), but see adc.h
//  documentation for other options.
int32 CODE param_X_channel = 0;
int32 CODE param_Y_channel = 1;
int32 CODE param_Z_channel = 2;
int32 CODE param_RX_channel = 3;
int32 CODE param_RY_channel = 4;
int32 CODE param_RZ_channel = 5;

// Invert analog axes.
// 0: Not inverted (axis position is proportional to voltage)
// 1: Inverted (axis position is inversely proportional to voltage) 
int32 CODE param_X_invert = 0;
int32 CODE param_Y_invert = 0;
int32 CODE param_Z_invert = 0;
int32 CODE param_RX_invert = 0;
int32 CODE param_RY_invert = 0;
int32 CODE param_RZ_invert = 0;

// Pin pull status.
// 0: High impedance (disables the internal pull-up and pull-down resistors on that pin)
// 1: Pulled (enables an internal 20 kOhm pull resistor for that pin)
// Note that P1_0 and P1_1 don't have pull-ups/pull-downs.
// Port 0 pull-ups/pull-downs are disabled by default to prevent them
//  from interfering from analog readings.
int32 CODE param_P0_0_pull_enable = 0;
int32 CODE param_P0_1_pull_enable = 0;
int32 CODE param_P0_2_pull_enable = 0;
int32 CODE param_P0_3_pull_enable = 0;
int32 CODE param_P0_4_pull_enable = 0;
int32 CODE param_P0_5_pull_enable = 0;

int32 CODE param_P1_2_pull_enable = 1;
int32 CODE param_P1_3_pull_enable = 1;
int32 CODE param_P1_4_pull_enable = 1;
int32 CODE param_P1_5_pull_enable = 1;
int32 CODE param_P1_6_pull_enable = 1;
int32 CODE param_P1_7_pull_enable = 1;

// Port pull type; set for all pins on each port.
// 0: Low (pull-down enabled)
// 1: High (pull-up enabled)
int32 CODE param_port0_pull_type = 1;
int32 CODE param_port1_pull_type = 1;

void updateLeds()
{
    usbShowStatusWithGreenLed();
}

// Updates the axis position.
void axisUpdate(int32 analogChannel, int32 axisInvert, int16 * axis)
{
    int16 analogValue;
    int16 lastValue;

    if(0 > analogChannel || analogChannel > 5)
        return;

    analogValue = adcRead(analogChannel);
    lastValue = *axis;
    *axis = ((axisInvert ? 2047 - analogValue : analogValue) << 5) - 32767;

    if(lastValue != *axis)
        usbHidJoystickInputUpdated = 1;
}

void joystickService()
{
    uint16 lastButtons = usbHidJoystickInput.buttons;
    usbHidJoystickInput.buttons = 0;

    // Update x-axis position.
    axisUpdate(param_X_channel, param_X_invert, &usbHidJoystickInput.x);
    // Update y-axis position.
    axisUpdate(param_Y_channel, param_Y_invert, &usbHidJoystickInput.y);
    // Update z-axis position.
    axisUpdate(param_Z_channel, param_Z_invert, &usbHidJoystickInput.z);
    // Update rx-axis position.
    axisUpdate(param_RX_channel, param_RX_invert, &usbHidJoystickInput.rx);
    // Update ry-axis position.
    axisUpdate(param_RY_channel, param_RY_invert, &usbHidJoystickInput.ry);
    // Update rz-axis position.
    axisUpdate(param_RZ_channel, param_RZ_invert, &usbHidJoystickInput.rz);

    // Set button status.
    usbHidJoystickInput.buttons |= (param_button1_pin != -1 & (isPinHigh(param_button1_pin) ^ param_button1_invert)) << 0;
    usbHidJoystickInput.buttons |= (param_button2_pin != -1 & (isPinHigh(param_button2_pin) ^ param_button2_invert)) << 1;
    usbHidJoystickInput.buttons |= (param_button3_pin != -1 & (isPinHigh(param_button3_pin) ^ param_button3_invert)) << 2;
    usbHidJoystickInput.buttons |= (param_button4_pin != -1 & (isPinHigh(param_button4_pin) ^ param_button4_invert)) << 3;
    usbHidJoystickInput.buttons |= (param_button5_pin != -1 & (isPinHigh(param_button5_pin) ^ param_button5_invert)) << 4;
    usbHidJoystickInput.buttons |= (param_button6_pin != -1 & (isPinHigh(param_button6_pin) ^ param_button6_invert)) << 5;
    usbHidJoystickInput.buttons |= (param_button7_pin != -1 & (isPinHigh(param_button7_pin) ^ param_button7_invert)) << 6;
    usbHidJoystickInput.buttons |= (param_button8_pin != -1 & (isPinHigh(param_button8_pin) ^ param_button8_invert)) << 7;
    usbHidJoystickInput.buttons |= (param_button9_pin != -1 & (isPinHigh(param_button9_pin) ^ param_button9_invert)) << 8;
    usbHidJoystickInput.buttons |= (param_button10_pin != -1 & (isPinHigh(param_button10_pin) ^ param_button10_invert)) << 9;
    usbHidJoystickInput.buttons |= (param_button11_pin != -1 & (isPinHigh(param_button11_pin) ^ param_button11_invert)) << 10;
    usbHidJoystickInput.buttons |= (param_button12_pin != -1 & (isPinHigh(param_button12_pin) ^ param_button12_invert)) << 11;
    usbHidJoystickInput.buttons |= (param_button13_pin != -1 & (isPinHigh(param_button13_pin) ^ param_button13_invert)) << 12;
    usbHidJoystickInput.buttons |= (param_button14_pin != -1 & (isPinHigh(param_button14_pin) ^ param_button14_invert)) << 13;
    usbHidJoystickInput.buttons |= (param_button15_pin != -1 & (isPinHigh(param_button15_pin) ^ param_button15_invert)) << 14;
    usbHidJoystickInput.buttons |= (param_button16_pin != -1 & (isPinHigh(param_button16_pin) ^ param_button16_invert)) << 15;

    if(lastButtons != usbHidJoystickInput.buttons)
        usbHidJoystickInputUpdated = 1;
}

void setup()
{
    // Set pull type for port 0 to either pull up or pull down.
    setPort0PullType(param_port0_pull_type);

    // Define enable status for pins on port 0
    setDigitalInput(0, param_P0_0_pull_enable);
    setDigitalInput(1, param_P0_1_pull_enable);
    setDigitalInput(2, param_P0_2_pull_enable);
    setDigitalInput(3, param_P0_3_pull_enable);
    setDigitalInput(4, param_P0_4_pull_enable);
    setDigitalInput(5, param_P0_5_pull_enable);

    // Set pull type for port 1 to either pull up or pull down.
    setPort1PullType(param_port1_pull_type);

    // Define enable status for pins on port 1 (P1_0 and P1_1 don't have pull-ups/pull-downs)
    setDigitalInput(12, param_P1_2_pull_enable);
    setDigitalInput(13, param_P1_3_pull_enable);
    setDigitalInput(14, param_P1_4_pull_enable);
    setDigitalInput(15, param_P1_5_pull_enable);
    setDigitalInput(16, param_P1_6_pull_enable);
    setDigitalInput(17, param_P1_7_pull_enable);
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
