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
#define SERIAL_MODE_AUTO        0
#define SERIAL_MODE_USB_RADIO   1
#define SERIAL_MODE_UART_RADIO  2
#define SERIAL_MODE_USB_UART    3

#define I2CDELAY                20        // units = us

int32 CODE param_serial_mode = SERIAL_MODE_AUTO;

int32 CODE param_baud_rate = 9600;


/** Functions *****************************************************************/

void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(vinPowerPresent());

    // Turn on the red LED if the radio is in the RX_OVERFLOW state.
    // There used to be several bugs in the radio libraries that would cause
    // the radio to go in to this state, but hopefully they are all fixed now.
    /*if (MARCSTATE == 0x11)
    {
        LED_RED(1);
    }
    else
    {
        LED_RED(0);
    }*/
}

uint8 currentSerialMode()
{
    if ((uint8)param_serial_mode > 0 && (uint8)param_serial_mode <= 3)
    {
        return (uint8)param_serial_mode;
    }

    if (usbPowerPresent())
    {
        if (vinPowerPresent())
        {
            return SERIAL_MODE_USB_UART;
        }
        else
        {
            return SERIAL_MODE_USB_RADIO;
        }
    }
    else
    {
        return SERIAL_MODE_UART_RADIO;
    }
}

void usbToRadioService()
{/*
    while (usbComRxAvailable())
    {
        usbComRxReceiveByte();
        i2cWriteByte(1, 0, 0xEE);
        i2cWriteByte(0, 0, 0xAA);
        i2cWriteByte(1, 0, 0xEF);
        i2cReadByte(0, 0);
        i2cReadByte(1, 1);
        LED_RED(!LED_RED_STATE);
    }

    while(usbComRxAvailable() && radioComTxAvailable())
    {
        radioComTxSendByte(usbComRxReceiveByte());
    }

    while(radioComRxAvailable() && usbComTxAvailable())
    {
        usbComTxSendByte(radioComRxReceiveByte());
    }*/
}

void uartToRadioService()
{
    while(uart1RxAvailable() && radioComTxAvailable())
    {
        radioComTxSendByte(uart1RxReceiveByte());
    }

    while(radioComRxAvailable() && uart1TxAvailable())
    {
        uart1TxSendByte(radioComRxReceiveByte());
    }
}

void usbToUartService()
{
    while(usbComRxAvailable() && uart1TxAvailable())
    {
        uart1TxSendByte(usbComRxReceiveByte());
    }

    while(uart1RxAvailable() && usbComTxAvailable())
    {
        usbComTxSendByte(uart1RxReceiveByte());
    }
}


uint16 cmpAccRead(uint8 reg)
{
    if (!i2cTimeoutOccurred) i2cWriteByte(0x30, 1, 0);
    if (!i2cTimeoutOccurred) i2cWriteByte(reg, 0, 0);
    if (!i2cTimeoutOccurred) i2cWriteByte(0x31, 1, 0);
    if (!i2cTimeoutOccurred)
    {
        return i2cReadByte(1, 1);
    }
    else
    {
        return 0;
    }
}

void cmpAccWrite(uint8 reg, uint8 val)
{
    if (!i2cTimeoutOccurred) i2cWriteByte(0x30, 1, 0);
    if (!i2cTimeoutOccurred) i2cWriteByte(reg, 0, 0);
    if (!i2cTimeoutOccurred) i2cWriteByte(val, 0, 1);
}

void putchar(char c)
{
    usbComTxSendByte(c);
}

void main()
{
    uint16 hi = 0, lo = 0;

    systemInit();
    usbInit();

    uart1Init();
    uart1SetBaudRate(param_baud_rate);

    P1_0 = 0;
    P1_1 = 0;

    i2cInit(100, 10); // 100 kHz, 10 ms timeout

    cmpAccWrite(0x20, 0x3f); //enable accelerometer

    while (1)
    {
        boardService();
        updateLeds();

        usbComService();



        if (usbComTxAvailable() >= 10)
        {
            hi = cmpAccRead(0x29);
            lo = cmpAccRead(0x28);

            if (i2cTimeoutOccurred)
            {
                printf("error!\r\n");
                i2cTimeoutOccurred = 0;
            }
            else
                printf("%6d\r\n", ((int16)hi << 8) | lo);
        }
    }
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
