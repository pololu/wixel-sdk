/* wireless_serial:
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
 * There are three operating modes based on how the Wixel is powered:
 * 1) If the Wixel is powered from USB only, then the it operates in
 *    USB-to-Radio mode.  Bytes from the USB virtual COM port get sent to the
 *    radio and vice versa.
 * 2) If the Wixel is powered from VIN only, then the it operates in
 *    UART-to-Radio mode.  Bytes from the UART's RX line get sent to the radio
 *    and bytes from the radio get sent to the UART's TX line.
 * 3) If the Wixel is powered from both USB and VIN, then the it operates
 *    in USB-to-UART mode.
 *
 * The app can switch between all three of these modes on the fly.
 *
 * == Parameters ==
 *   param_baud_rate : The baud rate to use for the UART, in bits per second.
 *   param_radio_channel : See description in radio_link.h.
 *
 * == Example Uses ==
 * 1) This application can be used to make a wireless serial link between two
 * microcontrollers, with no USB involved.  Simply power both Wixels from
 * VIN and don't plug in USB.
 *
 * 2) This application can be used to make a wireless serial link between a
 * computer and a microcontroller/robot.  You can put the exact same app on both
 * Wixels, and connect one Wixel to your computer via USB and connect the
 * other Wixel to your microcontroller and power it with VIN.
 *
 * 3) If you are doing option 2, you can plug a USB cable directly in to your
 *    robot at any time to put your robot's Wixel in to USB-to-UART mode.  This
 *    would allow you to transfer data faster than using the wireless connection.
 */

/*
 * TODO: Support for USB CDC ACM control signals.
 * TODO: UART flow control.
 * TODO: Better radio protocol (see TODOs in radio_link.c).
 * TODO: Obey CDC-ACM Set Line Coding commands:
 *       In USB-UART mode this would let the user change the baud rate at run-time.
 *       In USB-RADIO mode, bauds 0-255 would correspond to radio channels.
 */

#include <cc2511_map.h>
#include <board.h>
#include <random.h>
#include <time.h>

#include <usb.h>
#include <usb_com.h>

#include <radio_com.h>
#include <radio_link.h>

#include <uart.h>

int32 CODE param_baud_rate = 9600;

void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(vinPowerPresent());

    // Turn on the red LED if the radio is in the RX_OVERFLOW state.
    // There used to be several bugs in the radio libraries that would cause
    // the radio to go in to this state, but hopefully they are all fixed now.
    if (MARCSTATE == 0x11)
    {
        LED_RED(1);
    }
    else
    {
        LED_RED(0);
    }
}

void usbToRadioService()
{
    while(usbComRxAvailable() && radioComTxAvailable())
    {
        radioComTxSendByte(usbComRxReceiveByte());
    }

    while(radioComRxAvailable() && usbComTxAvailable())
    {
        usbComTxSendByte(radioComRxReceiveByte());
    }
}

void uartToRadioService()
{
    while(uart0RxAvailable() && radioComTxAvailable())
    {
        radioComTxSendByte(uart0RxReceiveByte());
    }

    while(radioComRxAvailable() && uart0TxAvailable())
    {
        uart0TxSendByte(radioComRxReceiveByte());
    }
}

void usbToUartService()
{
    while(usbComRxAvailable() && uart0TxAvailable())
    {
        uart0TxSendByte(usbComRxReceiveByte());
    }

    while(uart0RxAvailable() && usbComTxAvailable())
    {
        usbComTxSendByte(uart0RxReceiveByte());
    }
}

void main()
{
    systemInit();
    usbInit();

    uart0Init();
    uart0SetBaudRate(param_baud_rate);

    radioComInit();
    randomSeedFromAdc();

    // Set up P1_6 to be the RX debug signal and P1_7 to be the TX debug signal.
    P1DIR |= (1<<6) | (1<<7);
    IOCFG1 = 0b001000; // P1_6 = Preamble Quality Reached
    IOCFG2 = 0b011011; // P1_7 = PA_PD (TX mode)

    while(1)
    {
        boardService();
        updateLeds();

        radioComTxService();
        usbComService();

        if (usbPowerPresent())
        {
            if (vinPowerPresent())
            {
                usbToUartService();
            }
            else
            {
                usbToRadioService();
            }
        }
        else
        {
            uartToRadioService();
        }
    }
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
