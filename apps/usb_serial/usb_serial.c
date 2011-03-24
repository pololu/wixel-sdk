/* usb_serial app:
 *
 * == Pinout ==
 *
 * == Overview ==
 *
 * == Technical Description ==
 *
 * == Parameters ==
 *
 * == Example Uses ==
 */

/*
 * TODO: Support for USB CDC ACM control signals.
 * TODO: use LEDs to give feedback about USB activity.
 * TODO: UART flow control
 * TODO: give feedback about framing and parity errors
 */

/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>
#include <random.h>
#include <time.h>

#include <usb.h>
#include <usb_com.h>

#include <uart1.h>

/** Functions *****************************************************************/
void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(0);
    LED_RED(0);
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

void lineCodingChanged()
{
    uart1SetBaudRate(usbComLineCoding.dwDTERate);
    uart1SetParity(usbComLineCoding.bParityType);
    uart1SetStopBits(usbComLineCoding.bCharFormat);
}

void main()
{
    systemInit();
    usbInit();
    usbComLineCodingChangeHandler = &lineCodingChanged;

    uart1Init();
    lineCodingChanged();

    while(1)
    {
        boardService();
        updateLeds();
        usbComService();
        usbToUartService();
    }
}
