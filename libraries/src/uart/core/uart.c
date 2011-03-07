/** \file uart.c
 * This is the main source file for <code>uart.lib</code>.  See uart.h for
 * information on how to use this library.
 */

#include "uart.h"
#include <cc2511_map.h>
#include <cc2511_types.h>

#if defined(UART0) || defined(__CDT_PARSER__)
#define N 0
#elif defined(UART1)
#define N 1
#else
#error "No UART specified."
#endif

#ifdef UART0
#define ISR_URX()  void ISR_URX0() __interrupt(URX0_VECTOR) __using(1)
#define ISR_UTX()  void ISR_UTX0() __interrupt(UTX0_VECTOR) __using(1)
#define uartNRxParityErrorOccurred  uart0RxParityErorrOccurred
#define uartNRxFramingErrorOccurred uart0RxParityErorrOccurred
#define uartNRxBufferFullOccurred   uart0RxParityErorrOccurred
#define uartNRxAvailable            uart0RxAvailable
#define uartNInit                   uart0Init
#else
#define ISR_URX()  void ISR_URX1() __interrupt(URX1_VECTOR) __using(1)
#define ISR_UTX()  void ISR_UTX1() __interrupt(UTX1_VECTOR) __using(1)
#define uartNRxParityErrorOccurred   uart1RxParityErorrOccurred
#define uartNRxFramingErrorOccurred  uart1RxParityErorrOccurred
#define uartNRxBufferFullOccurred    uart1RxParityErorrOccurred
#define uartNRxAvailable             uart1RxAvailable
#define uartNInit                    uart1Init
#endif

static volatile uint8 XDATA uartTxBuffer[256];         // sizeof(uartTxBuffer) must be a power of two
static volatile uint8 DATA uartTxBufferMainLoopIndex;  // Index of next byte main loop will write.
static volatile uint8 DATA uartTxBufferInterruptIndex; // Index of next byte interrupt will read.

#define UART_TX_BUFFER_FREE_BYTES() ((uartTxBufferInterruptIndex - uartTxBufferMainLoopIndex - 1) & (sizeof(uartTxBuffer) - 1))

static volatile uint8 XDATA uartRxBuffer[256];     // sizeof(uartRxBuffer) must be a power of two
static volatile uint8 DATA uartRxBufferMainLoopIndex;  // Index of next byte main loop will read.
static volatile uint8 DATA uartRxBufferInterruptIndex; // Index of next byte interrupt will write.

#define UART_RX_BUFFER_FREE_BYTES() ((uartRxBufferMainLoopIndex - uartRxBufferInterruptIndex - 1) & (sizeof(uartRxBuffer) - 1))
#define UART_RX_BUFFER_USED_BYTES() ((uartRxBufferInterruptIndex - uartRxBufferMainLoopIndex) & (sizeof(uartRxBuffer) - 1))

volatile BIT uartNRxParityErrorOccurred;
volatile BIT uartNRxFramingErrorOccurred;
volatile BIT uartNRxBufferFullOccurred;

void uartNInit(void)
{
    /* USART0 UART Alt. 1: RTS = P0_5
     *                     CTS = P0_4
     *                     TX  = P0_3
     *                     RX  = P0_2
     */

    // set P0 priority
    P2DIR &= ~0xC0; // P2DIR.PRIP0 (7:6) = 00 (USART0 over USART1)

    // set uartN I/O location to P0
    PERCFG &= ~0x01; // PERCFG.U0CFG (0) = 0 (Alt. 1)

    // Set P0_3/TX to be a "peripheral function" pin instead of GPIO.
    // Note: We do NOT do that same for P0_2/RX because that seems to have
    // no benefits, and is actually bad because it disables the internal
    // pull-up resistor.
    P0SEL |= (1<<3); // P0SEL.SELP0_3 = 1

    // make sure ADC doesn't use this
    //ADCCFG &= ~(0x0c); // ADCCFG.ADCCFG[3:2] = 0 (ADC input disabled)

    U0UCR |= 0x80; // U0UCR.FLUSH (7) = 1

    // set tx flag so the interrupt fires when we enable it for the first time
    UTX0IF = 1;

    // initialize tx buffer
    uartTxBufferMainLoopIndex = 0;
    uartTxBufferInterruptIndex = 0;

    // clear rx flag
    URX0IF = 0;

    // initialize rx buffer
    uartRxBufferMainLoopIndex = 0;
    uartRxBufferInterruptIndex = 0;
    uartNRxParityErrorOccurred = 0;
    uartNRxFramingErrorOccurred = 0;
    uartNRxBufferFullOccurred = 0;

    // configure USART0 to enable UART and receiver
    U0CSR |= 0xc0; // U0CSR.MODE (7) = 1 (UART mode); U0CSR.RE (6) = 1 (receiver enabled)

    // Set the priority of the uartN RX and TX interrupts to be 1 (second lowest priority).
    // They need to be higher than the RF interrupt because that one could take a long time.
    // This code also sets the priority of the T2 interrupt, because it is grouped with uartN.
    IP0 |= (1<<2);
    IP1 &= ~(1<<2);

    // enable rx interrupt
    URX0IE = 1;

    // enable interrupts in general
    EA = 1;
}

void uartNSetBaudRate(uint32 baud)
{
    uint32 baudMPlus256;
    uint8 baudE = 0;

    // actual max baud rate is 1500000 (F/16) - we are limited by the way we calculate the exponent and mantissa
    if (baud < 23 || baud > 495782)
        return;

    // calculate baud rate - see datasheet 12.14.3
    // this is derived from (baudM + 256) = baud * 2^28 / 24000000
    baudMPlus256 = (baud * 11) + (baud * 8663 / 46875);

    // get baudMPlus256 into the range 256-511 (so BAUD_M is in the range 0-255)
    while (baudMPlus256 > 0x1ff)
    {
        baudE++;
        baudMPlus256 /= 2;
    }
    U0GCR = baudE; // U0GCR.BAUD_E (4:0)
    U0BAUD = baudMPlus256; // U0BAUD.BAUD_M (7:0) - only the lowest 8 bits of baudMPlus256 are used, so this is effectively baudMPlus256 - 256
}

uint8 uartNTxAvailable(void)
{
    return UART_TX_BUFFER_FREE_BYTES();
}

void uartNTxSend(const uint8 XDATA * buffer, uint8 size)
{
    // Assumption: uartNTxSend() was recently called and it returned a number at least as big as 'size'.
    // TODO: after DMA memcpy is implemented, use it to make this function faster

    while (size)
    {
        uartTxBuffer[uartTxBufferMainLoopIndex] = *buffer;

        buffer++;
        uartTxBufferMainLoopIndex = (uartTxBufferMainLoopIndex + 1) & (sizeof(uartTxBuffer) - 1);
        size--;

        // Enable UART TX interrupt
        IEN2 |= 0x04; // IEN2.UTX0IE (2)
    }
}

void uartNTxSendByte(uint8 byte)
{
    // Assumption: uartNTxAvailable() was recently called and it returned a non-zero number.

    uartTxBuffer[uartTxBufferMainLoopIndex] = byte;
    uartTxBufferMainLoopIndex = (uartTxBufferMainLoopIndex + 1) & (sizeof(uartTxBuffer) - 1);

    // Enable UART TX interrupt
    IEN2 |= 0x04; // IEN2.UTX0IE (2)
}

uint8 uartNRxAvaialable(void)
{
    return UART_RX_BUFFER_USED_BYTES();
}

uint8 uartNRxReceiveByte(void)
{
    // Assumption: uartNRxAvaialable was recently called and it returned a non-zero value.

    uint8 byte = uartRxBuffer[uartRxBufferMainLoopIndex];
    uartRxBufferMainLoopIndex = (uartRxBufferMainLoopIndex + 1) & (sizeof(uartRxBuffer) - 1);
    return byte;
}

ISR_URX()
{
    // A byte has just started transmitting on TX and there is room in
    // the UART's hardware buffer for us to add another byte.

    if (uartTxBufferInterruptIndex != uartTxBufferMainLoopIndex)
    {
        // There more bytes available in our software buffer, so send
        // the next byte.

        UTX0IF = 0;

        U0DBUF = uartTxBuffer[uartTxBufferInterruptIndex];
        uartTxBufferInterruptIndex = (uartTxBufferInterruptIndex + 1) & (sizeof(uartTxBuffer) - 1);
    }
    else
    {
        // There are no more bytes to send in our buffer, so disable this interrupt.
        IEN2 &= ~0x04; // IEN2.UTX0IE (2) = 0
    }
}

ISR_UTX()
{
    URX0IF = 0;

    // check for frame and parity errors
    if (!(U0CSR & 0x18)) // U0CSR.FE (4) == 0; U0CSR.ERR (3) == 0
    {
        // There were no errors.

        if (UART_RX_BUFFER_FREE_BYTES())
        {
            // The software RX buffer has space, so add this new byte to the buffer.
            uartRxBuffer[uartRxBufferInterruptIndex] = U0DBUF;
            uartRxBufferInterruptIndex = (uartRxBufferInterruptIndex + 1) & (sizeof(uartRxBuffer) - 1);
        }
        else
        {
            // The buffer is full, so discard the received byte and report and overflow error.
            uartNRxBufferFullOccurred = 1;
        }
    }
    else
    {
        if (U0CSR & 0x10) // U0CSR.FE (4) == 1
        {
            uartNRxFramingErrorOccurred = 1;
        }
        if (U0CSR & 0x08) // U0CSR.ERR (3) == 1
        {
            uartNRxParityErrorOccurred = 1;
        }
    }
}
