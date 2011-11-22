/** serial_i2c app:

This app turns a Wixel into a serial-to-I2C bridge, acting as a master
controller on a single-master I2C bus. To perform I2C operations, another
device can issue serial ASCII commands to the Wixel on its radio, UART, or USB
interface.

For complete documentation and a precompiled version of this app, see the
"Serial-to-I2C App" section of the Pololu Wixel User's Guide:
http://www.pololu.com/docs/0J46
 

== Default pinout ==

P1_0 = I2C SCL
P1_1 = I2C SDA
P0_3 = UART TX
P0_2 = UART RX
*/

/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>
#include <random.h>
#include <time.h>

#include <usb.h>
#include <usb_com.h>

#include <radio_com.h>
#include <radio_link.h>

#include <uart1.h>
#include <i2c.h>

#include <stdio.h>

/** Parameters ****************************************************************/

#define BRIDGE_MODE_RADIO_I2C 0
#define BRIDGE_MODE_UART_I2C  1
#define BRIDGE_MODE_USB_I2C   2

int32 CODE param_bridge_mode = BRIDGE_MODE_RADIO_I2C;
int32 CODE param_baud_rate = 9600;
int32 CODE param_I2C_SCL_pin = 10;
int32 CODE param_I2C_SDA_pin = 11;
int32 CODE param_I2C_freq_kHz = 100;
int32 CODE param_I2C_timeout_ms = 10;
int32 CODE param_cmd_timeout_ms = 500;

/** Global Constants & Variables **********************************************/

uint16 lastCmd = 0;

// ASCII commands
#define CMD_START      'S'
#define CMD_STOP       'P'
#define CMD_GET_ERRORS 'E'

// error flags
#define ERR_I2C_NACK_ADDRESS (1 << 0)
#define ERR_I2C_NACK_DATA    (1 << 1)
#define ERR_I2C_TIMEOUT      (1 << 2)
#define ERR_CMD_INVALID      (1 << 3)
#define ERR_CMD_TIMEOUT      (1 << 4)
#define ERR_UART_OVERFLOW    (1 << 5)
#define ERR_UART_FRAMING     (1 << 6)

static uint8 errors = 0;

static uint8 response = 0;
static BIT returnResponse = 0;

enum i2cState {IDLE, GET_ADDR, GET_LEN, GET_DATA};
enum i2cState state = IDLE;

static BIT started = 0;
static BIT dataDirIsRead = 0;
static uint8 dataLength = 0;

// function pointers to selected serial interface
uint8 (*rxAvailableFunction)(void)   = NULL;
uint8 (*rxReceiveByteFunction)(void) = NULL;
uint8 (*txAvailableFunction)(void)   = NULL;
void  (*txSendByteFunction)(uint8)   = NULL;

/** Functions *****************************************************************/

void updateLeds(void)
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(vinPowerPresent());
    LED_RED(errors);
}

void parseCmd(uint8 byte)
{
    BIT nack;

    switch (state)
    {
    case IDLE:
        switch ((char)byte)
        {
        case CMD_GET_ERRORS:
            response = errors;
            returnResponse = 1;
            errors = 0;
            break;

        case CMD_START:
            i2cStart();
            started = 1;
            state = GET_ADDR;
            break;

        case CMD_STOP:
            if (started)
            {
                i2cStop();
                started = 0;
                break;
            }
            // if not started, fall through to error
            // no break here  (this comment is required to suppress Eclipse Code Analysis warning)

        default:
            errors |= ERR_CMD_INVALID;
            break;
        }
        break;

    case GET_ADDR:
        // write slave address
        dataDirIsRead = byte & 1; // lowest bit of slave address determines direction (0 = write, 1 = read)
        nack = i2cWriteByte(byte);

        if (i2cTimeoutOccurred)
        {
            errors |= ERR_I2C_TIMEOUT;
            i2cTimeoutOccurred = 0;
        }
        else if (nack)
        {
            errors |= ERR_I2C_NACK_ADDRESS;
        }
        state = GET_LEN;
        break;

    case GET_LEN:
        // store data length
        dataLength = byte;
        state = (dataLength > 0) ? GET_DATA : IDLE;
        break;

    case GET_DATA:
        if (dataDirIsRead)
        {
            // this should never happen, because i2cService should not call parseCmd until i2cRead has read all response bytes and set state = IDLE
            errors |= ERR_CMD_INVALID;
        }
        else
        {
            // write data
            nack = i2cWriteByte(byte);
            if (i2cTimeoutOccurred)
            {
                errors |= ERR_I2C_TIMEOUT;
                i2cTimeoutOccurred = 0;
            }
            else if (nack)
            {
                errors |= ERR_I2C_NACK_DATA;
            }

            if (--dataLength == 0)
            {
                state = IDLE;
            }
        }
        break;
    }
}

void i2cRead(void)
{
    uint8 byte;

    // read one byte; send a nack if this is the last byte
    byte = i2cReadByte(dataLength == 1);
    if (i2cTimeoutOccurred)
    {
        errors |= ERR_I2C_TIMEOUT;
        i2cTimeoutOccurred = 0;
        response = 0;
    }
    else
    {
        response = byte;
    }

    if (--dataLength == 0)
    {
        state = IDLE;
    }
    returnResponse = 1;
}


void i2cService(void)
{
    // Only try to process I2C if there isn't a response still waiting to be returned on serial.
    if (!returnResponse)
    {
        if (dataDirIsRead && state == GET_DATA)
        {
            // We are doing an I2C read, so handle that.
            i2cRead();
        }
        else if (rxAvailableFunction())
        {
            // A command byte is available, so process it.
            parseCmd(rxReceiveByteFunction());
            lastCmd = getMs(); // store the time of the last byte received
        }
        else if (started && (param_cmd_timeout_ms > 0) && ((uint16)(getMs() - lastCmd) > param_cmd_timeout_ms))
        {
            // The current command has timed out.
            i2cStop();
            started = 0;
            errors |= ERR_CMD_TIMEOUT;
        }
    }

    if (returnResponse && txAvailableFunction())
    {
        txSendByteFunction(response);
        returnResponse = 0;
    }
}

void uartCheckErrors(void)
{
    if (uart1RxBufferFullOccurred)
    {
        errors |= ERR_UART_OVERFLOW;
        uart1RxBufferFullOccurred = 0;
    }
    if (uart1RxFramingErrorOccurred)
    {
        errors |= ERR_UART_FRAMING;
        uart1RxFramingErrorOccurred = 0;
    }
}

void main(void)
{
    systemInit();
    usbInit();

    i2cPinScl = param_I2C_SCL_pin;
    i2cPinSda = param_I2C_SDA_pin;

    i2cSetFrequency(param_I2C_freq_kHz);
    i2cSetTimeout(param_I2C_timeout_ms);

    switch (param_bridge_mode)
    {
    case BRIDGE_MODE_RADIO_I2C:
        radioComInit();
        rxAvailableFunction   = radioComRxAvailable;
        rxReceiveByteFunction = radioComRxReceiveByte;
        txAvailableFunction   = radioComTxAvailable;
        txSendByteFunction    = radioComTxSendByte;
        break;
    case BRIDGE_MODE_UART_I2C:
        uart1Init();
        uart1SetBaudRate(param_baud_rate);
        rxAvailableFunction   = uart1RxAvailable;
        rxReceiveByteFunction = uart1RxReceiveByte;
        txAvailableFunction   = uart1TxAvailable;
        txSendByteFunction    = uart1TxSendByte;
        break;
    case BRIDGE_MODE_USB_I2C:
        rxAvailableFunction   = usbComRxAvailable;
        rxReceiveByteFunction = usbComRxReceiveByte;
        txAvailableFunction   = usbComTxAvailable;
        txSendByteFunction    = usbComTxSendByte;
        break;
    }

    while (1)
    {
        boardService();
        updateLeds();

        switch (param_bridge_mode)
        {
        case BRIDGE_MODE_RADIO_I2C:
            radioComTxService();
            break;

        case BRIDGE_MODE_UART_I2C:
            uartCheckErrors();
            break;
        }

        usbComService();

        i2cService();
    }
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
