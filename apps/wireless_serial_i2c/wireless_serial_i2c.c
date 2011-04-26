/* wireless_serial app:
 *
 * Pin out:
 * P0_3 = TX
 * P0_2 = RX
 * P1_7 = Radio Transmit Debug Signal
 * P1_6 = Radio Receive Debug Signal
 *
 * == Overview ==
 * This app allows you to connect two Wixels together to make a wireless,
 * bidirectional, lossless serial link.  The Wixels must be on the same radio
 * channel, and all other pairs of Wixels must be at least 2 channels away.
 *
 * == Technical Description ==
 * This device appears to the USB host as a Virtual COM Port, with USB product
 * ID 0x2200.  It uses the radio_link library to do wireless communication.
 *
 * There are three basic serial modes that can be selected:
 * 1) USB-to-Radio: Bytes from the USB virtual COM port get sent to the
 *    radio and vice versa.
 * 2) UART-to-Radio: Bytes from the UART's RX line get sent to the radio
 *    and bytes from the radio get sent to the UART's TX line.
 * 3) USB-to-UART: Just like a normal USB-to-Serial adapter, bytes from
 *    the virtual COM port get sent on the UART's TX line and bytes from
 *    the UART's RX line get sent to the virtual COM port.
 *
 * You can select which serial mode you want to use by setting the serial_mode
 * parameter to the appropriate number (using the numbers above).  Or, you can
 * leave the serial mode at 0 (which is the default).  If the serial_mode is 0,
 * then the Wixel will automatically choose a serial mode based on how it is
 * being powered, and it will switch between the different serial modes on the
 * fly.
 *
 * Power Source | Serial Mode
 * --------------------------
 * USB only     | USB-to-Radio
 * VIN only     | UART-to-Radio
 * USB and VIN  | USB-to-UART
 *
 * == Parameters ==
 *   serial_mode   : Selects the serial mode or auto mode (0-3).
 *   baud_rate     : The baud rate to use for the UART, in bits per second.
 *   radio_channel : See description in radio_link.h.
 *
 * == Example Uses ==
 * 1) This application can be used to make a wireless serial link between two
 *    microcontrollers, with no USB involved.  To do this, use the UART-to-Radio
 *    mode on both Wixels.
 *
 * 2) This application can be used to make a wireless serial link between a
 *    computer and a microcontroller.  Use USB-to-Radio mode on the Wixel that
 *    is connected to the computer and use UART-to-Radio mode on the Wixel
 *    that is connected to the microcontroller.
 *
 * 3) If you are doing option 2 and using the the auto-detect serial mode
 *    (serial_mode = 0), then you have the option to (at any time) plug a USB
 *    cable directly in to the Wixel that is connected to your microcontroller
 *    to establish a more direct (wired) serial connection with the
 *    microcontroller.  (You would, of course, also have to switch to the other
 *    COM port when you do this.)
 */

// TODO: try harder to enable a pull-up on the RX line.  Right now, we get junk
// on the RX line from 60 Hz noise if you connect one end of a cable to the RX line.

/*
 * TODO: Support for USB CDC ACM control signals.
 * TODO: use LEDs to give feedback about sending/receiving bytes.
 * TODO: UART flow control.
 * TODO: Better radio protocol (see TODOs in radio_link.c).
 * TODO: Obey CDC-ACM Set Line Coding commands:
 *       In USB-UART mode this would let the user change the baud rate at run-time.
 *       In USB-RADIO mode, bauds 0-255 would correspond to radio channels.
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
#define SERIAL_MODE_RADIO_I2C 0
#define SERIAL_MODE_UART_I2C  1
#define SERIAL_MODE_USB_I2C   2

int32 CODE param_serial_mode = SERIAL_MODE_RADIO_I2C;
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

uint8 (*rxAvailableFunction)(void) = NULL;
uint8 (*rxReceiveByteFunction)(void) = NULL;
uint8 (*txAvailableFunction)(void) = NULL;
void (*txSendByteFunction)(uint8) = NULL;

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
            nack = i2cWriteByte(byte, 0, 0);
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
            nack = i2cWriteByte(byte, 0, 0);
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

    byte = i2cReadByte((dataLength == 1), 0);
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
                if (started && (uint16)(getMs() - lastCmd) > param_cmd_timeout_ms)
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

    if (param_serial_mode == SERIAL_MODE_RADIO_I2C)
    {
        radioComInit();
    }
    else if  (param_serial_mode == SERIAL_MODE_UART_I2C)
    {
        uart1Init();
        uart1SetBaudRate(param_baud_rate);
    }

    i2cSetFrequency(param_I2C_freq_kHz);
    i2cSetTimeout(param_I2C_timeout_ms);

    switch(param_serial_mode)
    {
    case SERIAL_MODE_RADIO_I2C:
        rxAvailableFunction   = radioComRxAvailable;
        rxReceiveByteFunction = radioComRxReceiveByte;
        txAvailableFunction   = radioComTxAvailable;
        txSendByteFunction    = radioComTxSendByte;
        break;
    case SERIAL_MODE_UART_I2C:
        rxAvailableFunction   = uart1RxAvailable;
        rxReceiveByteFunction = uart1RxReceiveByte;
        txAvailableFunction   = uart1TxAvailable;
        txSendByteFunction    = uart1TxSendByte;
        break;
    case SERIAL_MODE_USB_I2C:
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

        if (param_serial_mode == SERIAL_MODE_RADIO_I2C)
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
