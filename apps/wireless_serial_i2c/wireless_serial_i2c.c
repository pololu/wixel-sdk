/* wireless_serial_i2c app:
 *
 * Pin out:
 * P0_3 = TX
 * P0_2 = RX
 * P1_0 = I2C SCL
 * P1_1 = I2C SDA
 *
 * This app allows you use a Wixel as an I2C bus master, controlled by a
 * wireless, UART, or USB interface.

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
int32 CODE param_I2C_freq_kHz = 100;
int32 CODE param_I2C_timeout_ms = 10;
int32 CODE param_cmd_timeout_ms = 500;

/** Global Constants & Variables **********************************************/

uint16 lastCmd = 0;

#define CMD_START      'S'
#define CMD_STOP       'P'
#define CMD_GET_ERRORS 'E'

#define ERR_I2C_NACK_ADDRESS 0x1
#define ERR_I2C_NACK_DATA    0x2
#define ERR_I2C_TIMEOUT      0x8

static uint8 errors = 0;

static uint8 response = 0;
static BIT returnResponse = 0;

static BIT started = 0;
static BIT addrSet = 0;
static BIT dataDirIsRead = 0;
static BIT lengthSet = 0;
static uint8 dataLength = 0;

uint8 (*rxAvailableFunction)(void)   = NULL;
uint8 (*rxReceiveByteFunction)(void) = NULL;
uint8 (*txAvailableFunction)(void)   = NULL;
void  (*txSendByteFunction)(uint8)   = NULL;

/** Functions *****************************************************************/

void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(vinPowerPresent());
    LED_RED(errors);
}

void parseCmd(uint8 byte)
{
    BIT nack;

    if (!started)
    {
        if ((char)byte == CMD_START)
        {
            // send start
            i2cStart();
            addrSet = 0;
            lengthSet = 0;
            started = 1;
        }
        else if ((char)byte == CMD_GET_ERRORS)
        {
            response = errors;
            returnResponse = 1;
            errors = 0;
        }
    }
    else
    {
        if (!addrSet)
        {
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
            addrSet = 1;
        }
        else if (!lengthSet)
        {
            // store data length
            dataLength = byte;
            lengthSet = 1;
        }
        else if (!dataDirIsRead && dataLength)
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
            dataLength--;
        }
        else if ((char)byte == CMD_START)
        {
            // repeated start
            i2cStart();
            addrSet = 0;
            lengthSet = 0;
        }
        else if ((char)byte == CMD_STOP)
        {
            i2cStop();
            started = 0;
        }
        else if ((char)byte == CMD_GET_ERRORS)
        {
            response = errors;
            returnResponse = 1;
            errors = 0;
        }
    }
}

void i2cRead()
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
    dataLength--;
    returnResponse = 1;
}


void i2cService()
{
    // don't try to process I2C if there's a response waiting to be returned on serial
    if (!returnResponse)
    {

        if (dataDirIsRead && lengthSet && dataLength)
        {
            // if we're doing an I2C read, handle that
            i2cRead();
        }
        else
        {
            // check if a command is available on serial
            if (rxAvailableFunction())
            {
                parseCmd(rxReceiveByteFunction());
                lastCmd = getMs();
            }
            else
            {
                if (started && (param_cmd_timeout_ms > 0) && ((uint16)(getMs() - lastCmd) > param_cmd_timeout_ms))
                {
                    // command timeout
                    i2cStop();
                    started = 0;
                }
            }
        }
    }

    if (returnResponse && txAvailableFunction())
    {
        txSendByteFunction(response);
        returnResponse = 0;
    }
}

void main()
{
    systemInit();
    usbInit();

    if (param_bridge_mode == BRIDGE_MODE_RADIO_I2C)
    {
        radioComInit();
    }
    else if  (param_bridge_mode == BRIDGE_MODE_UART_I2C)
    {
        uart1Init();
        uart1SetBaudRate(param_baud_rate);
    }

    i2cSetFrequency(param_I2C_freq_kHz);
    i2cSetTimeout(param_I2C_timeout_ms);

    switch(param_bridge_mode)
    {
    case BRIDGE_MODE_RADIO_I2C:
        rxAvailableFunction   = radioComRxAvailable;
        rxReceiveByteFunction = radioComRxReceiveByte;
        txAvailableFunction   = radioComTxAvailable;
        txSendByteFunction    = radioComTxSendByte;
        break;
    case BRIDGE_MODE_UART_I2C:
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

        if (param_bridge_mode == BRIDGE_MODE_RADIO_I2C)
        {
            radioComTxService();
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
