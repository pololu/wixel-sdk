/** wireless_tilt_mouse app:

Allows you to make a wireless tilt mouse using two Wixels, an accelerometer,
and two pushbuttons (optional).

One Wixel should be running the wireless_tilt_mouse_receiver app and be
connected to a computer via USB.  The other Wixel should be running this app,
and be connected to the accelerometer and two optional pushbuttons.

For complete documentation and a precompiled version of this app, see the
"Wireless Tilt Mouse App" section of the Pololu Wixel User's Guide:
http://www.pololu.com/docs/0J46

== Pinout ==

P0_1 = Mouse vertical analog input
P0_2 = Mouse horizontal analog input
P1_2 = left mouse button input (pulled high; low means button is pressed)
P1_7 = left mouse button input (pulled high; low means button is pressed)


== Parameters ==

invert_x:  Default is 0.  Set to 1 to invert the X axis movement.
invert_y:  Default is 0.  Set to 1 to invert the Y axis movement.
speed: Controls how fast the mouse moves.  Default is 100.

*/

#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <usb_hid_constants.h>
#include <radio_queue.h>
#include <math.h>

#define TX_INTERVAL 10 // time between transmissions (ms)

int32 CODE param_invert_x = 0;
int32 CODE param_invert_y = 0;
int32 CODE param_speed = 100;

void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(vinPowerPresent());
}

void txMouseState()
{
    uint8 XDATA * txBuf;
    static uint8 lastTx = 0;

    if ((uint8)(getMs() - lastTx) > TX_INTERVAL && (txBuf = radioQueueTxCurrentPacket()))
    {
        float fx, fy, multiplier;
        int8 x, y;

        fx = -((float)adcRead(2 | ADC_BITS_12) - 1024) / 128;  // fx = Acceleration in X direction (floating point)
        fy =  ((float)adcRead(1 | ADC_BITS_12) - 1024) / 128;  // fy = Acceleration in Y direction
        multiplier = sqrtf(fx * fx + fy * fy) * param_speed/100;

        // Compute the x and y mouse change values to send.
        x = (int8)(fx * multiplier);
        y = (int8)(fy * multiplier);
        if (param_invert_x){ x = -x; }
        if (param_invert_y){ y = -y; }

        // Construct a packet and transmit it on the radio.
        txBuf[0] = 3;  // Packet length in bytes.
        txBuf[1] = x;
        txBuf[2] = y;
        txBuf[3] = (!isPinHigh(12) << MOUSE_BUTTON_LEFT) | (!isPinHigh(17) << MOUSE_BUTTON_RIGHT);
        radioQueueTxSendPacket();

        lastTx = getMs();
    }
}

void main()
{
    systemInit();
    usbInit();

    radioQueueInit();

    // disable pull-ups on accelerometer outputs
    setDigitalInput(1, HIGH_IMPEDANCE);
    setDigitalInput(2, HIGH_IMPEDANCE);

    while(1)
    {
        updateLeds();
        boardService();
        usbComService();

        txMouseState();
    }
}
