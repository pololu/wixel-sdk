/* wireless_serial app:
 * This app allows you to connect two Wixels together to make a wireless,
 * bidirectional, lossless serial link.  
 * See description.txt or the Wixel User's Guide for more information.
 */

/*
 * TODO: To avoid damage, don't enable nDTR and nRTS outputs by default.
 * TODO: use LEDs to give feedback about sending/receiving bytes.
 * TODO: UART flow control.
 * TODO: Obey CDC-ACM Set Line Coding commands:
 *       In USB-RADIO mode, bauds 0-255 would correspond to radio channels.
 * TODO: shut down radio when we are in a different serial mode
 * TODO: make the heartbeat blinks on the Wixels be synchronized (will require
 *       major changes to the radio_link library)
 * TODO: turn on red LED or flash it if the Wixel is in a mode that requires USB
 *       but has not reached the USB Configured State (this avoids the problem of
 *       having 0 LEDs on when the Wixel is in USB-UART mode and self powered)
 */

/** Dependencies **************************************************************/
#include <wixel.h>

#include <usb.h>
#include <usb_com.h>

#include <radio_com.h>
#include <radio_link.h>

#include <uart1.h>


/** Parameters ****************************************************************/
#define SERIAL_MODE_AUTO        0
#define SERIAL_MODE_USB_RADIO   1
#define SERIAL_MODE_UART_RADIO  2
#define SERIAL_MODE_USB_UART    3
int32 CODE param_serial_mode = SERIAL_MODE_AUTO;

int32 CODE param_baud_rate = 9600;

int32 CODE param_nDTR_pin = 10;
int32 CODE param_nRTS_pin = 11;
int32 CODE param_nDSR_pin = 12;
int32 CODE param_nCD_pin = 13;

int32 CODE param_DTR_pin = -1;
int32 CODE param_RTS_pin = -1;
int32 CODE param_DSR_pin = -1;
int32 CODE param_CD_pin = -1;

int32 CODE param_arduino_DTR_pin = 0;

// Approximate number of milliseconds to disable UART's receiver for after a
// framing error is encountered.
// Valid values are 0-250.
// A value of 0 disables the feature (the UART's receiver will not be disabled).
// The actual number of milliseconds that the receiver is disabled for will be
// between param_framing_error_ms and param_framing_error_ms + 1.
int32 CODE param_framing_error_ms = 0;

/** Global Variables **********************************************************/

// This bit is 1 if the UART's receiver has been disabled due to a framing error.
// This bit should be equal to !U1CSR.RE, but we need this variable because we
// don't want to be reading U1CSR in the main loop, because reading it might
// cause the FE or ERR bits to be cleared and then the ISR
// would not receive notice of those errors.
BIT uartRxDisabled = 0;

uint8 DATA currentSerialMode;

BIT framingErrorActive = 0;

BIT errorOccurredRecently = 0;
uint8 lastErrorTime;

/** Functions *****************************************************************/

void updateLeds()
{
    static BIT dimYellowLed = 0;
    static uint16 lastRadioActivityTime;
    uint16 now;

    usbShowStatusWithGreenLed();

    now = (uint16)getMs();

    if (currentSerialMode == SERIAL_MODE_USB_UART)
    {
        // The radio is not being used, so turn off the yellow LED.
        LED_YELLOW(0);
    }
    else if (!radioLinkConnected())
    {
        // We have not connected to another device wirelessly yet, so do a
        // 50% blink with a period of 1024 ms.
        LED_YELLOW(now & 0x200 ? 1 : 0);
    }
    else
    {
        // We have connected.

        if ((now & 0x3FF) <= 20)
        {
            // Do a heartbeat every 1024ms for 21ms.
            LED_YELLOW(1);
        }
        else if (dimYellowLed)
        {
            static uint8 DATA count;
            count++;
            LED_YELLOW((count & 0x7)==0);
        }
        else
        {
            LED_YELLOW(0);
        }
    }

    if (radioLinkActivityOccurred)
    {
        radioLinkActivityOccurred = 0;
        dimYellowLed ^= 1;
        //dimYellowLed = 1;
        lastRadioActivityTime = now;
    }

    if ((uint16)(now - lastRadioActivityTime) > 32)
    {
        dimYellowLed = 0;
    }

    if ((uint8)(now - lastErrorTime) > 100)
    {
        errorOccurredRecently = 0;
    }

    LED_RED(errorOccurredRecently || uartRxDisabled);
}

/* Returns the logical values of the input control signal pins.
   Bit 0 is DSR.
   Bit 1 is CD. */
uint8 ioRxSignals()
{
    uint8 signals = 0;

    if ((param_CD_pin >= 0 && isPinHigh(param_CD_pin)) ||
            (param_nCD_pin >= 0 && !isPinHigh(param_nCD_pin)))
    {
        signals |= 2;
    }

    if ((param_DSR_pin >= 0 && isPinHigh(param_DSR_pin)) ||
            (param_nDSR_pin >= 0 && !isPinHigh(param_nDSR_pin)))
    {
        signals |= 1;
    }

    return signals;
}

/* Sets the logical values of the output control signal pins.
   This should be called frequently (not just when the values change).
   Bit 0 is DTR.
   Bit 1 is RTS. */
void ioTxSignals(uint8 signals)
{
    static uint8 nTrstPulseStartTime;
    static uint8 lastSignals;

    // Inverted outputs
    setDigitalOutput(param_nDTR_pin, (signals & ACM_CONTROL_LINE_DTR) ? 0 : 1);
    setDigitalOutput(param_nRTS_pin, (signals & ACM_CONTROL_LINE_RTS) ? 0 : 1);

    // Non-inverted outputs.
    setDigitalOutput(param_DTR_pin, (signals & ACM_CONTROL_LINE_DTR) ? 1 : 0);
    setDigitalOutput(param_RTS_pin, (signals & ACM_CONTROL_LINE_RTS) ? 1 : 0);

    // Arduino DTR pin.
    if (!(lastSignals & ACM_CONTROL_LINE_DTR) && (signals & ACM_CONTROL_LINE_DTR))
    {
        // We just made a falling edge on the nDTR line, so start a 1-2ms high pulse
        // on the nTRST line.
        setDigitalOutput(param_arduino_DTR_pin, HIGH);
        nTrstPulseStartTime = getMs();
    }
    else if ((uint8)(getMs() - nTrstPulseStartTime) >= 2)
    {
        setDigitalOutput(param_arduino_DTR_pin, LOW);
    }

    lastSignals = signals;
}

void errorOccurred()
{
    lastErrorTime = (uint8)getMs();
    errorOccurredRecently = 1;
}

void errorService()
{
    static uint8 lastRxLowTime;

    if (uart1RxBufferFullOccurred)
    {
        uart1RxBufferFullOccurred = 0;
        errorOccurred();
    }

    if (uart1RxFramingErrorOccurred)
    {
        uart1RxFramingErrorOccurred = 0;

        // A framing error occurred.
        framingErrorActive = 1;
        errorOccurred();

        if (param_framing_error_ms > 0)
        {
            // Disable the UART's receiver.
            U1CSR &= ~0x40;    // U1CSR.RE = 0.  Disables reception of bytes on the UART.
            uartRxDisabled = 1;
            lastRxLowTime = (uint8)getMs();  // Initialize lastRxLowTime even if the line isn't low right now.
        }
    }

    if (framingErrorActive)
    {
        if (!isPinHigh(17))
        {
            errorOccurred();
        }
        else
        {
            framingErrorActive = 0;
        }
    }

    if (uartRxDisabled)
    {
        if (!isPinHigh(17))
        {
            // The line is low.
            lastRxLowTime = (uint8)getMs();
        }
        else if ((uint8)(getMs() - lastRxLowTime) > param_framing_error_ms)
        {
            // The line has been high for long enough, so re-enable the receiver.
            U1CSR |= 0x40;
            uartRxDisabled = 0;
        }
    }
}

void updateSerialMode()
{
    if ((uint8)param_serial_mode > 0 && (uint8)param_serial_mode <= 3)
    {
        currentSerialMode = (uint8)param_serial_mode;
        return;
    }

    if (usbPowerPresent())
    {
        if (vinPowerPresent())
        {
            currentSerialMode = SERIAL_MODE_USB_UART;
        }
        else
        {
            currentSerialMode = SERIAL_MODE_USB_RADIO;
        }
    }
    else
    {
        currentSerialMode = SERIAL_MODE_UART_RADIO;
    }
}

void usbToRadioService()
{
    uint8 signals;

    // Data
    while(usbComRxAvailable() && radioComTxAvailable())
    {
        radioComTxSendByte(usbComRxReceiveByte());
    }

    while(radioComRxAvailable() && usbComTxAvailable())
    {
        usbComTxSendByte(radioComRxReceiveByte());
    }

    // Control Signals

    radioComTxControlSignals(usbComRxControlSignals() & 3);

    // Need to switch bits 0 and 1 so that DTR pairs up with DSR.
    signals = radioComRxControlSignals();
    usbComTxControlSignals( ((signals & 1) ? 2 : 0) | ((signals & 2) ? 1 : 0));
}

void uartToRadioService()
{
    // Data
    while(uart1RxAvailable() && radioComTxAvailable())
    {
        radioComTxSendByte(uart1RxReceiveByte());
    }

    while(radioComRxAvailable() && uart1TxAvailable())
    {
        uart1TxSendByte(radioComRxReceiveByte());
    }

    // Control Signals.
    ioTxSignals(radioComRxControlSignals());
    radioComTxControlSignals(ioRxSignals());
}

void usbToUartService()
{
    uint8 signals;

    // Data
    while(usbComRxAvailable() && uart1TxAvailable())
    {
        uart1TxSendByte(usbComRxReceiveByte());
    }

    while(uart1RxAvailable() && usbComTxAvailable())
    {
        usbComTxSendByte(uart1RxReceiveByte());
    }

    ioTxSignals(usbComRxControlSignals());

    // Need to switch bits 0 and 1 so that DTR pairs up with DSR.
    signals = ioRxSignals();
    usbComTxControlSignals( ((signals & 1) ? 2 : 0) | ((signals & 2) ? 1 : 0));

    // TODO: report framing, parity, and overrun errors to the USB host here
}

void main()
{
    systemInit();

    setDigitalOutput(param_arduino_DTR_pin, LOW);
    ioTxSignals(0);

    usbInit();

    uart1Init();
    uart1SetBaudRate(param_baud_rate);

    if (param_serial_mode != SERIAL_MODE_USB_UART)
    {
        radioComRxEnforceOrdering = 1;
        radioComInit();
    }

    // Set up P1_5 to be the radio's TX debug signal.
    P1DIR |= (1<<5);
    IOCFG0 = 0b011011; // P1_5 = PA_PD (TX mode)

    while(1)
    {
        updateSerialMode();
        boardService();
        updateLeds();
        errorService();

        if (param_serial_mode != SERIAL_MODE_USB_UART)
        {
            radioComTxService();
        }

        usbComService();

        switch(currentSerialMode)
        {
        case SERIAL_MODE_USB_RADIO:  usbToRadioService();  break;
        case SERIAL_MODE_UART_RADIO: uartToRadioService(); break;
        case SERIAL_MODE_USB_UART:   usbToUartService();   break;
        }
    }
}
