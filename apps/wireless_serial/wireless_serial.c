// TODO: fix bug: USB power is only being detected once at the beginning and never detected again!

/* wireless_serial:
 *
 *
 * TODO: Support for USB CDC ACM control signals.
 * TODO: UART flow control.
 * TODO: Better radio protocol (see TODOs in radio_link.c).
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
    //usbShowStatusWithGreenLed();
	LED_GREEN(usbPowerPresent());

    if(vinPowerPresent()){ LED_YELLOW(1); }

    LED_RED(timeMs & 1);

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
