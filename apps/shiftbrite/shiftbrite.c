/** shiftbrite app:

This app allows you to wirelessly control a chain of one more ShiftBrite RGB LED
modules.

For complete documentation and a precompiled version of this app, see the
"Shiftbrite App" section of the Pololu Wixel User's Guide:
http://www.pololu.com/docs/0J46
*/

#include <cc2511_map.h>
#include <board.h>
#include <random.h>
#include <time.h>
#include <radio_com.h>
#include <radio_link.h>
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <stdio.h>
#include <random.h>

#define SHIFTBRITE_LATCH P1_7
#define SHIFTBRITE_DATA P1_6
#define SHIFTBRITE_CLOCK P1_5
#define SHIFTBRITE_DISABLE P1_4

// parameters
int32 CODE param_input_bits = 8;
int32 CODE param_echo_on = 1;

uint32 lastSend = 0;

// These variables are used to flicker the yellow LED when data is received over the radio.
BIT radioBlinkActive = 0;
uint8 radioLastActivity;
uint8 radioBlinkStart;

// converts 0-9, a-f, or A-F into the corresponding hex value
uint8 hexCharToByte(char c)
{
    if(c >= '0' && c <= '9')
    {
        return c-'0';
    }
    if(c >= 'a' && c <= 'f')
    {
        return c-'a'+10;
    }
    if(c >= 'A' && c <= 'F')
    {
        return c-'A'+10;
    }
    return 0; // default
}

// coverts a string of hex digits into the corresponding hex value
uint16 hex(char *s, uint8 len)
{
    uint16 ret = 0;
    uint8 i;
    for(i=0;i<len;i++)
    {
        ret <<= 4;
        ret += hexCharToByte(s[i]);
    }
    return ret;
}

void toggleLatch()
{
    SHIFTBRITE_LATCH = 1;
    delayMicroseconds(1);
    SHIFTBRITE_LATCH = 0;
    delayMicroseconds(1);
    SHIFTBRITE_DISABLE = 0; // enable shiftbrites
}

void sendBit(BIT value)
{
    SHIFTBRITE_DATA = value;
    delayMicroseconds(1);
    SHIFTBRITE_CLOCK = 1;
    delayMicroseconds(1);
    SHIFTBRITE_CLOCK = 0;
    delayMicroseconds(1);
}

void sendRGB(uint16 r, uint16 g, uint16 b)
{
    uint16 mask = 512;
    sendBit(0);
    sendBit(0);
    while(mask)
    {
        sendBit((mask & b) ? 1 : 0);
        mask >>= 1;
    }
    mask = 512;
    while(mask)
    {
        sendBit((mask & r) ? 1 : 0);
        mask >>= 1;
    }
    mask = 512;
    while(mask)
    {
        sendBit((mask & g) ? 1 : 0);
        mask >>= 1;
    }
}

// limits value to lie between min and max
int32 restrictRange(int32 value, int32 min, int32 max)
{
    if(value < min)
        return min;
    if(value > max)
        return max;
    return value;
}

void shiftbriteProcessByte(char c)
{
    static char rgb[12]; // big enough to hold 4 hex digits times three colors
    static uint8 i = 0;
    static const uint8 input_bits = restrictRange(param_input_bits,1,16); // allow up to 16 bits = 4 hex digits
    static const uint8 hex_chars_per_color = ((input_bits-1) >> 2) + 1; // 1-4 bits = 1; 5-8 bits = 2; etc.
    static const int8 shift = 10 - input_bits; // amount to shift to create the output

    if(c == '\r' || c == '\n')
    {
        i = 0;
        toggleLatch();
    }
    else
    {

        rgb[i] = c;
        i++;
        
        if(i == hex_chars_per_color*3)
        {

            i = 0;

            if(shift > 0)
            {
                sendRGB(
                    hex(rgb,                      hex_chars_per_color) << shift,
                    hex(rgb+hex_chars_per_color,  hex_chars_per_color) << shift,
                    hex(rgb+hex_chars_per_color*2,hex_chars_per_color) << shift
                    );
            }
            else
            {
                sendRGB(
                    hex(rgb,                      hex_chars_per_color) >> -shift,
                    hex(rgb+hex_chars_per_color,  hex_chars_per_color) >> -shift,
                    hex(rgb+hex_chars_per_color*2,hex_chars_per_color) >> -shift
                    );
            }
        }
    }
}

void shiftbriteService()
{
    if (!radioComRxAvailable()){ return; }

    do
    {
        char c = radioComRxReceiveByte();
        if(radioComTxAvailable() && param_echo_on)
        {
            radioComTxSendByte(c);
        }
        shiftbriteProcessByte(c);
    }
    while(radioComRxAvailable());

    // Record the time that the radio activity occurred.
    radioLastActivity = (uint8)getMs();

    // If we are not already blinking to indicate radio activity,
    // start blinking.
    if (!radioBlinkActive)
    {
        radioBlinkActive = 1;
        radioBlinkStart = radioLastActivity;
    }
}

void shiftbriteInit()
{
    SHIFTBRITE_DISABLE = 1; // disable shiftbrites until a valid color is sent
    SHIFTBRITE_CLOCK = 0; // clock low
    SHIFTBRITE_LATCH = 0; // prevent unintended latching
    P1DIR |= (1<<4); // P1_4 = Disable !Enable
    P1DIR |= (1<<5); // P1_5 = Clock
    P1DIR |= (1<<6); // P1_6 = Data
    P1DIR |= (1<<7); // P1_7 = Latch
}

void updateLeds()
{
    uint8 time = (uint8)getMs();

    if (radioBlinkActive)
    {
        // Make the yellow LED flicker because we recently received some data over the radio.
        LED_YELLOW((uint8)(time - radioBlinkStart) & 64);

        if ((uint8)(time - radioLastActivity) > 96)
        {
            // Stop flickering after 96 ms.
            radioBlinkActive = 0;
        }
    }
    else
    {
        // The yellow LED is normally on.
        LED_YELLOW(1);
    }

    usbShowStatusWithGreenLed();

}

void main()
{
    systemInit();
    usbInit();
    shiftbriteInit();

    radioComInit();
    
    while(1)
    {
        boardService();
        updateLeds();

        radioComTxService();
        usbComService();
        
        shiftbriteService();
    }
}
