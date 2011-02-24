// test2: David is trying to do something more advanced, which requires
// a little more coordination between Wixels.  The first step will be
// to allow them to agree on time slots.

#include <cc2511_map.h>
#include <board.h>
#include <usb.h>
#include <usb_com.h>
#include <radio_com.h>
#include <radio_link.h>
#include <random.h>
#include <uart.h>

void blinkLeds()
{
    usbShowStatusWithGreenLed();

    if(vinPowerPresent()){ LED_YELLOW(1); }

    if (MARCSTATE == 0x11)
    {
        LED_RED(1);
    }
    else
    {
        LED_RED(0);
    }
}

uint8 nibbleToAscii(uint8 nibble)
{
    nibble &= 0xF;
    if (nibble <= 0x9){ return '0' + nibble; }
    else{ return 'A' + (nibble - 0xA); }
}

// Stops the CPU until another interrupt occurs.
// (Such as the Timer 4 interrupt, which happens every millisecond.)
// TODO: put the code that actually does PCON.IDLE=1 in to XDATA (or better yet, DATA).
//    Set it to be a NOP or RET from any interrupt that wants the main loop to
//    fully execute after the interrupt has run.  For example, there
//    should be a USB interrupt that does this.
//    Set it to the correct code just before returning from sleepMode0.
void sleepMode0()
{
    // Put the device to sleep by following the recommended pseudo code in the datasheet section 12.1.3:
    SLEEP = (SLEEP & ~3) | 0;    // SLEEP.MODE = 0 : Selects Power Mode 0 (PM0).
    __asm nop __endasm;
    __asm nop __endasm;
    __asm nop __endasm;
    PCON |= 1; // PCON.IDLE = 1 : Actually go to sleep.
    __asm nop __endasm; // tmphax
    __asm nop __endasm; // tmphax
    __asm nop __endasm; // tmphax
}

/**
void radioLoopBack() // TODO: why does this just not work at all?? (radioComRxAvailable seems to always be 0)
{
    while(radioComRxAvailable() && radioComTxAvailable())
    {
        radioComTxSendByte(radioComRxReceiveByte());
        LED_RED_TOGGLE();
    }
}**/

void radioToUsbService()
{
    while(radioComRxAvailable() && usbComTxAvailable())
    {
        usbComTxSendByte(radioComRxReceiveByte());
    }

    while(usbComRxAvailable() && radioComTxAvailable())
    {
        radioComTxSendByte(usbComRxReceiveByte());
    }
}

void radioToUartService()
{
    while(radioComRxAvailable() && uart0TxAvailable())
    {
        uart0TxSendByte(radioComRxReceiveByte());
    }

    while(uart0RxAvailable() && radioComTxAvailable())
    {
        radioComTxSendByte(uart0RxReceiveByte());
    }
}

void main()
{
    wixelInit();
    usbInit();
    uart0Init();

    uart0SetBaudRate(115200);

    radioComInit();
    randomSeedFromAdc();

    // Set up P1_6 to be the RX debug signal and P1_7 to be the TX debug signal.
    P1DIR |= (1<<6) | (1<<7);
    IOCFG1 = 0b001000; // P1_6 = Preamble Quality Reached
    IOCFG2 = 0b011011; // P1_7 = PA_PD (TX mode)

    while(1)
    {
        wixelService();
        blinkLeds();

        radioComTxService();

        if (vinPowerPresent())
        {
            radioToUartService();
        }
        else
        {
            usbComService();
            radioToUsbService();
        }

        // TODO: when switching between the modes above, you should probably flush the buffers that aren't being used

        //TODO: sleepMode0();

        //TODO: if (usbSuspended())
        //{
        //  usbSleep();
        //}
    }
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **


