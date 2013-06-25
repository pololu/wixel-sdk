/** general_joystick.c:

 This configurable app allows one to build a custom HID joystick
 using the Wixel. The joystick appears as an HID device over a (wired) USB connection.
 
 Configurable features include:
 
    - Pull types (high or low) for ports 0 and 1
    - Pin enable options for P0_0 through P0_5 and P1_0 through P1_7
    - Assignable buttons to pins
    - Button inversion options
    - Axis inversion for {x,y,z,Rx,ry,rz}  
    - Axis enable options for {x,y,z,rx,ry,rz}
    - Assignable axis {x,y,z,rx,ry,rz} to pins P0_0 through P0_5 

 The app parameters are configured with the Wixel Configuration Utility
 and are loaded onto the Wixel. Once the Wixel is integrated into the 
 joystick circuit, the joystick can be plugged into the computer via 
 USB to mini cable.
 
 The joystick axes {x,y,z,rx,ry,rz} use analog pins P0_0 through P0_5.
 If the joystick does not need all these axes, then the analog pins
 can be used as digital buttons instead. The user can also reassign which axis
 uses which analog pin on the Wixel.

*/

#include <wixel.h>
#include <usb.h>
#include <usb_hid.h>                     

// Each pin can have a pull status of the following:
// HIGH_IMPEDANCE (0): Disables the internal pull-up and pull-down resistors on that pin.
// PULLED (1): Enables an internal 20 kOhm pull resistor for that pin.
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

// Assign which buttons belong to which pin.
// Buttons can only be assigned to pins
// P1_0 through P1_7.
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

// Invert individual buttons.
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

// Assign axis to pin.
int32 CODE param_X_axis_enable = 1;  // X-Axis
int32 CODE param_Y_axis_enable = 1;  // Y-Axis
int32 CODE param_Z_axis_enable = 1;  // Z-Axis
int32 CODE param_RX_axis_enable = 1;  // RX-Axis
int32 CODE param_RY_axis_enable = 1;  // RY-Axis
int32 CODE param_RZ_axis_enable = 1;  // RZ-Axis

// Invert axis enable options.
int32 CODE param_X_axis_invert = 0;  // X-Axis
int32 CODE param_Y_axis_invert = 0;  // Y-Axis
int32 CODE param_Z_axis_invert = 0;  // Z-Axis
int32 CODE param_RX_axis_invert = 0;  // RX-Axis
int32 CODE param_RY_axis_invert = 0;  // RY-Axis
int32 CODE param_RZ_axis_invert = 0;  // RZ-Axis

// Axis to pin assignment. Only pins P0_0 through P0_5
// can be used.
int32 CODE param_X_pin = 0;
int32 CODE param_Y_pin = 1;
int32 CODE param_Z_pin = 2;
int32 CODE param_RX_pin = 3;
int32 CODE param_RY_pin = 4;
int32 CODE param_RZ_pin = 5;

// Port pull type specifies if the port is pulled up or down.
int32 CODE param_port0_pull_type = HIGH;
int32 CODE param_port1_pull_type = HIGH;

void updateLeds()
{
    usbShowStatusWithGreenLed();
}

// Updates the axis position.
void axisUpdate(int32 analogChannel, int32 axisInvert, int32 axisEnable, int16 * axis)
{
    
    int16 analogValue;
    int8 lastValue;
    
    if(!axisEnable)
        return;
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
    axisUpdate(param_X_pin, param_X_axis_invert, param_X_axis_enable, &usbHidJoystickInput.x);
    // Update y-axis position.        
    axisUpdate(param_Y_pin, param_Y_axis_invert, param_Y_axis_enable, &usbHidJoystickInput.y);
    // Update z-axis position.       
    axisUpdate(param_Z_pin, param_Z_axis_invert, param_Z_axis_enable, &usbHidJoystickInput.z);
    // Update rx-axis position.        
    axisUpdate(param_RX_pin, param_RX_axis_invert, param_RX_axis_enable, &usbHidJoystickInput.rx);            
    // Update ry-axis position.       
    axisUpdate(param_RY_pin, param_RY_axis_invert, param_RY_axis_enable, &usbHidJoystickInput.ry);
    // Update rz-axis position.       
    axisUpdate(param_RZ_pin, param_RZ_axis_invert, param_RZ_axis_enable, &usbHidJoystickInput.rz);
    
    // Set button status.
    usbHidJoystickInput.buttons |= (isPinHigh(param_button1_pin) ^ param_button1_invert) << 1;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button2_pin) ^ param_button2_invert) << 2;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button3_pin) ^ param_button3_invert) << 3;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button4_pin) ^ param_button4_invert) << 4;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button5_pin) ^ param_button5_invert) << 5;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button6_pin) ^ param_button6_invert) << 6;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button7_pin) ^ param_button7_invert) << 7;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button8_pin) ^ param_button8_invert) << 8;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button9_pin) ^ param_button9_invert) << 9;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button10_pin) ^ param_button10_invert) << 10;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button11_pin) ^ param_button11_invert) << 11;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button12_pin) ^ param_button12_invert) << 12;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button13_pin) ^ param_button13_invert) << 13;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button14_pin) ^ param_button14_invert) << 14;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button15_pin) ^ param_button15_invert) << 15;
    usbHidJoystickInput.buttons |= (isPinHigh(param_button16_pin) ^ param_button16_invert) << 16;
    
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
    
    // Define enable status for pins on port 1.
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
