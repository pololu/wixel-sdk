/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>

#include <usb.h>
#include <usb_com.h>

#include <random.h>
#include <time.h>

#include "repeater_radio_link.h"

#define PIN_COUNT 15
#define MAX_TX_INTERVAL 10 // maximum time between transmissions (ms)

#define IS_INPUT(port, pin) (P##port##Links[pin] < 0)
#define IS_OUTPUT(port, pin) (P##port##Links[pin] > 0)

// In each byte of a buffer:
// bit 7 = pin value
// bits 6:0 = pin link
#define PIN_LINK_OFFSET 0
#define PIN_LINK_MASK 0x7F
#define PIN_VAL_OFFSET 7

uint8 XDATA * rxBuf;
uint8 XDATA * txBuf;

typedef struct PORTPIN
{
    uint8 port;
    uint8 pin;
} PORTPIN;

PORTPIN XDATA inPins[PIN_COUNT];
uint8 inPinCount = 0;

PORTPIN XDATA outPins[PIN_COUNT];
uint8 outPinCount = 0;

static BIT rxEnabled = 0;
static BIT txEnabled = 0;

#define ABS(x) (((x) < 0) ? -(x) : (x))

/** Parameters ****************************************************************/
// defined in pin_params.s
extern int32 XDATA P0Links[6];
extern int32 XDATA P1Links[8];
extern int32 XDATA P2Links[2];

/** Functions *****************************************************************/
void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(vinPowerPresent());
}

uint8 pinLink(PORTPIN XDATA * portpin)
{
    int8 link;

    switch(portpin->port)
    {
        case 0:  link = P0Links[portpin->pin]; break;
        case 1:  link = P1Links[portpin->pin]; break;
        case 2:  link = P2Links[portpin->pin]; break;
        default: return 0; // invalid
    }
    return ABS(link);
}

// TODO: move general digital I/O functions to a library

BIT pinVal(PORTPIN XDATA * portpin)
{
    switch(portpin->port)
    {
        case 0:  return (P0 >> portpin->pin) & 1;
        case 1:  return (P1 >> portpin->pin) & 1;
        case 2:  return (P2 >> portpin->pin) & 1;
        default: return 0; // invalid
    }
}

void setPinVal(PORTPIN XDATA * portpin, BIT val)
{
    uint8 pin = portpin->pin;

    switch(portpin->port)
    {
        case 0:
            switch(portpin->pin)
            {
                case 0: P0_0 = val; return;
                case 1: P0_1 = val; return;
                case 2: P0_2 = val; return;
                case 3: P0_3 = val; return;
                case 4: P0_4 = val; return;
                case 5: P0_5 = val; return;
                case 6: P0_6 = val; return;
                case 7: P0_7 = val; return;
                default: return;
            }
        case 1:
            switch(portpin->pin)
            {
                case 0: P1_0 = val; return;
                case 1: P1_1 = val; return;
                case 2: P1_2 = val; return;
                case 3: P1_3 = val; return;
                case 4: P1_4 = val; return;
                case 5: P1_5 = val; return;
                default: return;
            }
        case 2:
            switch(portpin->pin)
            {
                case 1: P2_1 = val; return;
                default: return;
            }
        default: return;
    }
}

void configurePins(void)
{
    uint8 i;

    inPinCount = outPinCount = 0;

    // port 0 pins
    for (i = 0; i <= 5; i++)
    {
        if (IS_OUTPUT(0, i))
        {
            P0DIR |= (1 << i);
            rxEnabled = 1;

            outPins[outPinCount].port = 0;
            outPins[outPinCount].pin = i;
            outPinCount++;
        }
        else if (IS_INPUT(0, i))
        {
            txEnabled = 1;

            inPins[inPinCount].port = 0;
            inPins[inPinCount].pin = i;
            inPinCount++;
        }
    }

    // port 1 pins
    for (i = 0; i <= 7; i++)
    {
        if (IS_OUTPUT(1, i))
        {
            P1DIR |= (1 << i);
            rxEnabled = 1;

            outPins[outPinCount].port = 1;
            outPins[outPinCount].pin = i;
            outPinCount++;
        }
        else if (IS_INPUT(1, i))
        {
            txEnabled = 1;

            inPins[inPinCount].port = 1;
            inPins[inPinCount].pin = i;
            inPinCount++;
        }
    }

    // port 2 pins
    for (i = 1; i <= 1; i++)
    {
        if (IS_OUTPUT(2, i))
        {
            P2DIR |= (1 << i);
            rxEnabled = 1;

            outPins[outPinCount].port = 2;
            outPins[outPinCount].pin = i;
            outPinCount++;
        }
        else if (IS_INPUT(2, i))
        {
            txEnabled = 1;

            inPins[inPinCount].port = 2;
            inPins[inPinCount].pin = i;
            inPinCount++;
        }
    }
}

// read the states of input pins on this Wixel into a buffer
void readPins(uint8 XDATA * buf)
{
    uint8 i;

    for (i = 0; i < inPinCount; i++)
    {
        buf[i] = (pinLink(&inPins[i]) << PIN_LINK_OFFSET) | (pinVal(&inPins[i]) << PIN_VAL_OFFSET);
    }
}

// set the states of output pins on this Wixel based on values from a buffer
void setPins(uint8 XDATA * buf, uint8 pinCount)
{
    uint8 i, j;

    // loop over all bytes in packet
    for (i = 0; i < pinCount; i++)
    {
        // loop over all output pins
        for (j = 0; j < outPinCount; j++)
        {
            // check if this output pin's link matches the link in this packet
            if (pinLink(&outPins[j]) == ((buf[i] >> PIN_LINK_OFFSET) & PIN_LINK_MASK))
            {
                setPinVal(&outPins[j], (buf[i] >> PIN_VAL_OFFSET) & 1);
            }
        }
    }
}

void main(void)
{
    uint8 lastTx = 0;

    systemInit();
    usbInit();

    repeaterRadioLinkInit();
    randomSeedFromSerialNumber();

    configurePins();

    while(1)
    {
        updateLeds();
        boardService();
        usbComService();

        // receive pin states from another Wixel and set our output pins
        if (rxEnabled && (rxBuf = repeaterRadioLinkRxCurrentPacket()))
        {
            setPins(rxBuf + 1, *rxBuf);
            repeaterRadioLinkRxDoneWithPacket();
        }

        // read our input pins and transmit pin states to other Wixel(s)
        if (txEnabled && (uint8)(getMs() - lastTx) > MAX_TX_INTERVAL && (txBuf = repeaterRadioLinkTxCurrentPacket()))
        {
            readPins(txBuf + 1);
            *txBuf = inPinCount;
            repeaterRadioLinkTxSendPacket();

            lastTx = getMs();
        }
    }
}
