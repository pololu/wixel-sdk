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
 * TODO: UART flow control?
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

void main()
{
    systemInit();
    usbInit();

    uart1Init();
    uart1SetBaudRate(9600);

    while(1)
    {
        boardService();
        updateLeds();
        usbComService();
        usbToUartService();
    }
}
