/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>

#include <usb.h>
#include <usb_com.h>

#include <random.h>
#include <time.h>

#include "repeater_radio_com.h"
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
#define PIN_VAL_MASK 0x80

uint8 XDATA rxBuf[PIN_COUNT];
uint8 XDATA txBuf[PIN_COUNT];

typedef struct PORTPIN
{
    uint8 port;
    uint8 pin;
} PORTPIN;

PORTPIN inPins[PIN_COUNT];
uint8 inPinCount = 0;

PORTPIN outPins[PIN_COUNT];
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

    //LED_YELLOW(vinPowerPresent());
}

uint8 pinLink(PORTPIN * portpin)
{
    switch(portpin->port)
    {
        case 1:
            return ABS(P1Links[portpin->pin]);
        case 2:
            return ABS(P2Links[portpin->pin]);
        default:
            return ABS(P0Links[portpin->pin]);
    }
}

BIT pinVal(PORTPIN * portpin)
{
    switch(portpin->port)
    {
        case 1:  return (P1 >> portpin->pin) & 0x1;
        case 2:  return (P2 >> portpin->pin) & 0x1;
        default: return (P0 >> portpin->pin) & 0x1;
    }
}

void setPinVal(PORTPIN * portpin, uint8 val)
{
    if (val)
    {
        switch(portpin->port)
        {
            case 1:
                P1 |= (1 << portpin->pin); return;
            case 2:
                P2 |= (1 << portpin->pin); return;
            default:
                P0 |= (1 << portpin->pin); return;
        }
    }
    else
    {
        switch(portpin->port)
        {
            case 1:
                P1 &= ~(1 << portpin->pin); return;
            case 2:
                P2 &= ~(1 << portpin->pin); return;
            default:
                P0 &= ~(1 << portpin->pin); return;
        }
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
            txBuf[inPinCount] = (pinLink(&inPins[inPinCount]) & PIN_LINK_MASK) << PIN_LINK_OFFSET;
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
            txBuf[inPinCount] = (pinLink(&inPins[inPinCount]) & PIN_LINK_MASK) << PIN_LINK_OFFSET;
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
            txBuf[inPinCount] = (pinLink(&inPins[inPinCount]) & PIN_LINK_MASK) << PIN_LINK_OFFSET;
            inPinCount++;
        }
    }
}

uint8 readPins(uint8 XDATA * buf)
{
    uint8 i, changed = 0;

    for (i = 0; i < inPinCount; i++)
    {
        BIT val;
        val = pinVal(&inPins[i]);

        if ((buf[i] >> PIN_VAL_OFFSET & 1) != val)
        {
            buf[i] = (buf[i] & PIN_LINK_MASK) | (val << PIN_VAL_OFFSET);
            changed = 1;
        }
    }
    return changed;
}

void setPins(uint8 XDATA * buf, uint8 pinCount)
{
    uint8 i, j;

    for (i = 0; i < pinCount; i++)
    {
        for (j = 0; j < outPinCount; j++)
        {
            if (pinLink(&outPins[j]) == (buf[i] & PIN_LINK_MASK) >> PIN_LINK_OFFSET)
            {
                setPinVal(&outPins[j], (buf[i] & PIN_VAL_MASK) >> PIN_VAL_OFFSET);
            }
        }
    }
}

void main(void)
{
    uint8 lastTx = 0, rxPinCount;

    systemInit();
    usbInit();

    radioComInit();
    randomSeedFromSerialNumber();

    configurePins();

    while(1)
    {
        if (txEnabled)
        {
            LED_YELLOW(1);
        }
        else
        {
            LED_YELLOW(0);
        }

        updateLeds();
        boardService();
        usbComService();

        if (rxEnabled && radioComRxAvailable())
        {
            rxPinCount = radioComRxReceivePacket(rxBuf, PIN_COUNT);
            setPins(rxBuf, rxPinCount);
        }
        if (txEnabled && radioComTxAvailable() && (readPins(txBuf) || ((uint8)(getMs() - lastTx) > MAX_TX_INTERVAL)))
        {
            radioComTxSendPacket(txBuf, inPinCount);
            lastTx = getMs();
        }
    }
}
