/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>
#include <usb.h>
#include <usb_com.h>
#include <random.h>
#include <time.h>
#include <gpio.h>

#include "repeater_radio_link.h"

#define MAX_TX_INTERVAL 10 // maximum time between transmissions (ms)

#define PIN_COUNT 15
static uint8 CODE portPinMin[3] = {0, 0, 1};  // lowest pin in each port
static uint8 CODE portPinMax[3] = {5, 7, 1}; // highest pin in each port

// macros to determine whether a pin is an input or output based on its link param
#define IS_INPUT(pin)  (pinLink(pin) < 0)
#define IS_OUTPUT(pin) (pinLink(pin) > 0)

// list and count of input pins
static uint8 XDATA inPins[PIN_COUNT];
static uint8 inPinCount = 0;

// list and count of output pins
static uint8 XDATA outPins[PIN_COUNT];
static uint8 outPinCount = 0;

// only tx if we have at least one input; only rx if we have at least one output
static BIT txEnabled = 0;
static BIT rxEnabled = 0;

// pointers to link packets
static uint8 XDATA * txBuf;
static uint8 XDATA * rxBuf;

// In each byte of a buffer:
// bit 7 = pin value
// bits 6:0 = pin link
#define PIN_LINK_OFFSET 0
#define PIN_LINK_MASK 0x7F
#define PIN_VAL_OFFSET 7

/** Parameters ****************************************************************/
int32 CODE param_P0_0_link = -1;
int32 CODE param_P0_1_link = 0;
int32 CODE param_P0_2_link = 0;
int32 CODE param_P0_3_link = 0;
int32 CODE param_P0_4_link = 0;
int32 CODE param_P0_5_link = 0;
int32 CODE param_P1_0_link = 0;
int32 CODE param_P1_1_link = 0;
int32 CODE param_P1_2_link = 0;
int32 CODE param_P1_3_link = 0;
int32 CODE param_P1_4_link = 0;
int32 CODE param_P1_5_link = 0;
int32 CODE param_P1_6_link = 0;
int32 CODE param_P1_7_link = 0;
int32 CODE param_P2_1_link = 1; // red LED

/** Functions *****************************************************************/
void updateLeds()
{
    usbShowStatusWithGreenLed();

    //LED_YELLOW(vinPowerPresent());
}

int8 pinLink(uint8 pin)
{
    switch(pin)
    {
    case 0:  return param_P0_0_link;
    case 1:  return param_P0_1_link;
    case 2:  return param_P0_2_link;
    case 3:  return param_P0_3_link;
    case 4:  return param_P0_4_link;
    case 5:  return param_P0_5_link;
    case 10: return param_P1_0_link;
    case 11: return param_P1_1_link;
    case 12: return param_P1_2_link;
    case 13: return param_P1_3_link;
    case 14: return param_P1_4_link;
    case 15: return param_P1_5_link;
    case 16: return param_P1_6_link;
    case 17: return param_P1_7_link;
    case 21: return param_P2_1_link;
    }
    return 0;
}

void configurePins(void)
{
    uint8 port, pin, gpioPin;
    LED_YELLOW(0);
    inPinCount = outPinCount = 0;

    // Set all pulled pins to high
    // TODO: make this user-configurable
    setPort0PullType(HIGH);
    setPort1PullType(HIGH);
    setPort2PullType(LOW); // pulling port 2 high will cause problems (triggers bootloader entry)

    for (port = 0; port <= 2; port++)
    {
        for(pin = portPinMin[port]; pin <= portPinMax[port]; pin++)
        {
            gpioPin = port * 10 + pin;
            if (IS_OUTPUT(gpioPin))
            {
                setDigitalOutput(gpioPin, LOW);
                rxEnabled = 1;
                outPins[outPinCount++] = gpioPin;
            }
            else if (IS_INPUT(gpioPin))
            {
                txEnabled = 1;
                inPins[inPinCount++] = gpioPin;
            }
        }
    }
}

// read the states of input pins on this Wixel into a buffer
void readPins(uint8 XDATA * buf)
{
    uint8 pin;

    for (pin = 0; pin < inPinCount; pin++)
    {
        buf[pin] = (-pinLink(inPins[pin]) << PIN_LINK_OFFSET) | (isDigitalInputHigh(inPins[pin]) << PIN_VAL_OFFSET);
    }
}

// set the states of output pins on this Wixel based on values from a buffer
void setPins(uint8 XDATA * buf, uint8 byteCount)
{
    uint8 byte, pin;

    // loop over all bytes in packet
    for (byte = 0; byte < byteCount; byte++)
    {
        for (pin = 0; pin < outPinCount; pin++)
        {
            // check if this output pin's link matches the link in this packet
            if ((uint8)pinLink(outPins[pin]) == ((buf[byte] >> PIN_LINK_OFFSET) & PIN_LINK_MASK))
            {
                setDigitalOutput(outPins[pin], (buf[byte] >> PIN_VAL_OFFSET) & 1);
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
