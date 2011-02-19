/*  Library for sending and receiving a stream of serial bytes on UART0.
 *  This library uses a circular buffer and an interrupt for RX, and a
 *  circular buffer and an interrupt for TX, so it is capable of sending
 *  and receiving a constant stream of bytes without interruption.
 */

// TODO: why does the loopback test (serialPortTester.csproj) fail when we have this setup:
// Computer -> Wixel )))) Wixel with RX tied to TX
// It works for small transmissions, but if you try to send more than about 140 or so  bytes,
// a bunch of zeroes are received!  I suspect there might be something wrong with this library,
// but it could also be a bug in the radio libraries or usbCom.

#include "uart.h"
#include <cc2511_map.h>
#include <cc2511_types.h>

static volatile uint8 XDATA uartTxBuffer[64];     // sizeof(uartTxBuffer) must be a power of two
static volatile uint8 DATA uartTxBufferMainLoopIndex;  // main loop writes here
static volatile uint8 DATA uartTxBufferInterruptIndex; // tx interrupt reads here

#define UART_TX_BUFFER_FREE_BYTES() ((uartTxBufferInterruptIndex - uartTxBufferMainLoopIndex - 1) & (sizeof(uartTxBuffer) - 1))

static volatile uint8 XDATA uartRxBuffer[64];     // sizeof(uartRxBuffer) must be a power of two
static volatile uint8 DATA uartRxBufferMainLoopIndex;  // main loop reads here
static volatile uint8 DATA uartRxBufferInterruptIndex; // rx interrupt writes here

#define UART_RX_BUFFER_FREE_BYTES() ((uartRxBufferMainLoopIndex - uartRxBufferInterruptIndex - 1) & (sizeof(uartRxBuffer) - 1))
#define UART_RX_BUFFER_USED_BYTES() ((uartRxBufferInterruptIndex - uartRxBufferMainLoopIndex) & (sizeof(uartRxBuffer) - 1))

volatile BIT uart0RxParityErrorOccurred;
volatile BIT uart0RxFramingErrorOccurred;
volatile BIT uart0RxBufferFullOccurred;

void uart0SetBaudRate(uint32 baud)
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

void uart0Init()
{
	/* USART0 UART Alt. 1: RTS = P0_5
	 *                     CTS = P0_4
	 *                     TX  = P0_3
	 *                     RX  = P0_2
	 */

	// set P0 priority
	P2DIR &= ~0xC0; // P2DIR.PRIP0 (7:6) = 00 (USART0 over USART1)

	// set UART0 I/O location to P0
	PERCFG &= ~0x01; // PERCFG.U0CFG (0) = 0 (Alt. 1)

	// enable tx and rx on P0_3 & P0_2
	P0SEL |= 0x0c; // P0SEL.SELP0[3:2] = 1 (peripheral function)

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
	uart0RxParityErrorOccurred = 0;
	uart0RxFramingErrorOccurred = 0;
	uart0RxBufferFullOccurred = 0;

	// configure USART0 to enable UART and receiver
	U0CSR |= 0xc0; // U0CSR.MODE (7) = 1 (UART mode); U0CSR.RE (6) = 1 (receiver enabled)

	// enable rx interrupt
	URX0IE = 1;

	// enable interrupts in general
	EA = 1;
}

uint8 uart0TxAvailable()
{
	return UART_TX_BUFFER_FREE_BYTES();
}
void uart0TxSend(const uint8 XDATA * buffer, uint8 size)
{
	// Assumption: uart0TxSend() was recently called and it returned a number at least as big as 'size'.
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

void uart0TxSendByte(uint8 byte)
{
	// Assumption: uart0TxAvailable() was recently called and it returned a non-zero number.

    uartTxBuffer[uartTxBufferMainLoopIndex] = byte;
    uartTxBufferMainLoopIndex = (uartTxBufferMainLoopIndex + 1) & (sizeof(uartTxBuffer) - 1);

	// Enable UART TX interrupt
	IEN2 |= 0x04; // IEN2.UTX0IE (2)
}

// Transmit interrupt.
ISR(UTX0, 1)
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

uint8 uart0RxAvailable()
{
	return UART_RX_BUFFER_USED_BYTES();
}

uint8 uart0RxReceiveByte()
{
	// Assumption: uart0RxAvailable was recently called and it returned a non-zero value.

	uint8 byte = uartRxBuffer[uartRxBufferMainLoopIndex];
    uartRxBufferMainLoopIndex = (uartRxBufferMainLoopIndex + 1) & (sizeof(uartRxBuffer) - 1);
    return byte;
}

// Receive interrupt.
ISR(URX0, 1)
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
    		uart0RxBufferFullOccurred = 1;
    	}
    }
    else
    {
    	if (U0CSR & 0x10) // U0CSR.FE (4) == 1
    	{
    		uart0RxFramingErrorOccurred = 1;
    	}
    	if (U0CSR & 0x08) // U0CSR.ERR (3) == 1
    	{
    		uart0RxParityErrorOccurred = 1;
    	}
    }
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
