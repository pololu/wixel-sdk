/** \file spi_master.c
 * This is the main source file for <code>spi_master.c</code>.  See spi_master.h for
 * information on how to use this library.
 */

#include <cc2511_map.h>
#include <cc2511_types.h>

#if defined(__CDT_PARSER__)
#define SPI0
#endif

#if defined(SPI0)
#include <spi0_master.h>
#define INTERRUPT_PRIORITY_GROUP    2
#define ISR_URX()  void ISR_URX0()  __interrupt(URX0_VECTOR) __using(1)
#define ISR_UTX()  void ISR_UTX0()  __interrupt(UTX0_VECTOR) __using(1)
#define UTXNIF                      UTX0IF
#define URXNIF                      URX0IF
#define URXNIE                      URX0IE
#define UNCSR                       U0CSR
#define UNGCR                       U0GCR
#define UNUCR                       U0UCR
#define UNBAUD                      U0BAUD
#define UNDBUF                      U0DBUF
#define BV_UTXNIE                   (1<<2)
#define uartNRxParityErrorOccurred  uart0RxParityErrorOccurred
#define uartNRxFramingErrorOccurred uart0RxFramingErrorOccurred
#define uartNRxBufferFullOccurred   uart0RxBufferFullOccurred
#define uartNRxAvailable            uart0RxAvailable
#define uartNTxAvailable            uart0TxAvailable
#define uartNInit                   uart0Init
#define uartNSetBaudRate            uart0SetBaudRate
#define uartNSetParity              uart0SetParity
#define uartNSetStopBits            uart0SetStopBits
#define uartNTxSend                 uart0TxSend
#define uartNRxReceiveByte          uart0RxReceiveByte
#define uartNTxSend                 uart0TxSend
#define uartNTxSendByte             uart0TxSendByte

#elif defined(SPI1)
#include <spi1_master.h>
#define INTERRUPT_PRIORITY_GROUP     3
#define ISR_URX()  void ISR_URX1() __interrupt(URX1_VECTOR) __using(1)
#define ISR_UTX()  void ISR_UTX1() __interrupt(UTX1_VECTOR) __using(1)
#define UTXNIF                       UTX1IF
#define URXNIF                       URX1IF
#define URXNIE                       URX1IE
#define UNCSR                        U1CSR
#define UNGCR                        U1GCR
#define UNUCR                        U1UCR
#define UNBAUD                       U1BAUD
#define UNDBUF                       U1DBUF
#define BV_UTXNIE                    (1<<3)
#define uartNRxParityErrorOccurred   uart1RxParityErrorOccurred
#define uartNRxFramingErrorOccurred  uart1RxFramingErrorOccurred
#define uartNRxBufferFullOccurred    uart1RxBufferFullOccurred
#define uartNRxAvailable             uart1RxAvailable
#define uartNTxAvailable             uart1TxAvailable
#define uartNInit                    uart1Init
#define uartNSetBaudRate             uart1SetBaudRate
#define uartNSetParity               uart1SetParity
#define uartNSetStopBits             uart1SetStopBits
#define uartNTxSend                  uart1TxSend
#define uartNRxReceiveByte           uart1RxReceiveByte
#define uartNTxSend                  uart1TxSend
#define uartNTxSendByte              uart1TxSendByte
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
    /* From datasheet Table 50 */

    /* USART0 SPI Alt. 1:
     *                     SCK  = P0_5
     *                     MOSI = P0_3
     *                     MISO = P0_2
     */

    /* USART1 SPI Alt. 2:
     *                    SCK  = P1_5
     *                   MOSI  = P1_6
     *                   MISO  = P1_7
     */

    /* 12.14.2.1: In SPI master mode, only the MOSI, MISO, and SCK should be
     * configured as peripherals (see Section 12.4.6.1 and Section 12.4.6.2). If
     * the external slave requires a slave select signal (SSN) this can be
     * implemented by using a general-purpose I/O pin and control from SW.
     */

    // Note: We do NOT set the mode of the RX pin to "peripheral function"
    // because that seems to have no benefits, and is actually bad because
    // it disables the internal pull-up resistor.

#ifdef SPI0
    P2DIR &= ~0xC0;  // P2DIR.PRIP0 (7:6) = 00 : USART0 takes priority over USART1.
    PERCFG &= ~0x01; // PERCFG.U0CFG (0) = 0 (Alt. 1) : USART0 uses alt. location 1.
#else
    P2SEL |= 0x40;   // USART1 takes priority over USART0 on Port 1.
    PERCFG |= 0x02;  // PERCFG.U1CFG (1) = 1 (Alt. 2) : USART1 uses alt. location 2.
#endif

    // Set the mode of the SCK and MOSI pins to "peripheral function".
#ifdef SPI0
    P0SEL |= ((1<<5) | (1<<3)); // P0SEL.SELP0_5 = 1, P0SEL.SELP0_3 = 1
#else
    P1SEL |= ((1<<5) | (1<<6)); // P1SEL.SELP1_5 = 1, P1SEL.SELP1_6 = 1
#endif

    // Below, we set the priority of the RX and TX interrupts to be 1 (second lowest priority).
    // They need to be higher than the RF interrupt because that one could take a long time.
    // The SPI0 interrupts are grouped with the T2 interrupt, so its priority also gets set.
    // The SPI1 interrupts are grouped with the T3 interrupts, so its priority also gets set.
    IP0 |= (1<<INTERRUPT_PRIORITY_GROUP);
    IP1 &= ~(1<<INTERRUPT_PRIORITY_GROUP);

    URXNIF = 0; // Clear RX flag.
    EA = 1;     // Enable interrupts in general.
}

void spiNMasterSetFrequency(uint32 freq)
{
    uint32 baudMPlus256;
    uint8 baudE = 0;

    // max baud rate is 3000000 (F/8); min is 23 (baudM = 1)
    if (freq < 23 || freq > 3000000)
        return;

    // 495782 is the largest value that will not overflow the following calculation
    while (freq > 495782)
    {
        baudE++;
        freq /= 2;
    }

    // calculate baud rate - see datasheet 12.14.3
    // this is derived from (baudM + 256) = baud * 2^28 / 24000000
    baudMPlus256 = (freq * 11) + (freq * 8663 / 46875);

    // get baudMPlus256 into the range 256-511 (so BAUD_M is in the range 0-255)
    while (baudMPlus256 > 0x1ff)
    {
        baudE++;
        baudMPlus256 /= 2;
    }
    UNGCR = baudE; // UNGCR.BAUD_E (4:0)
    UNBAUD = baudMPlus256; // UNBAUD.BAUD_M (7:0) - only the lowest 8 bits of baudMPlus256 are used, so this is effectively baudMPlus256 - 256
}

void uartNSetParity(uint8 parity)
{
    // parity     D9    BIT9    PARITY
    // 0 None     x     0       x
    // 1 Odd      1     1       1
    // 2 Even     0     1       1
    // 3 Mark     1     1       0
    // 4 Space    0     1       0

    uint8 tmp = 0;

    switch(parity)
    {
    case PARITY_ODD:   tmp = 0b111 << 3; break;
    case PARITY_EVEN:  tmp = 0b011 << 3; break;
    case PARITY_MARK:  tmp = 0b110 << 3; break;
    case PARITY_SPACE: tmp = 0b010 << 3; break;
    }

    UNUCR = (UNUCR & 0b01000111) | tmp;
}

void uartNSetStopBits(uint8 stopBits)
{
    if (stopBits == STOP_BITS_2)
    {
        UNUCR |= (1<<2);    // 2 stop bits
    }
    else
    {
        UNUCR &= ~(1<<2);   // 1 stop bit
        // NOTE: An argument of STOP_BITS_1_5 is treated the same as STOP_BITS_1.
    }
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

        IEN2 |= BV_UTXNIE; // Enable TX interrupt
    }
}

void uartNTxSendByte(uint8 byte)
{
    // Assumption: uartNTxAvailable() was recently called and it returned a non-zero number.

    uartTxBuffer[uartTxBufferMainLoopIndex] = byte;
    uartTxBufferMainLoopIndex = (uartTxBufferMainLoopIndex + 1) & (sizeof(uartTxBuffer) - 1);

    IEN2 |= BV_UTXNIE; // Enable TX interrupt
}

uint8 uartNRxAvailable(void)
{
    return UART_RX_BUFFER_USED_BYTES();
}

uint8 uartNRxReceiveByte(void)
{
    // Assumption: uartNRxAvailable was recently called and it returned a non-zero value.

    uint8 byte = uartRxBuffer[uartRxBufferMainLoopIndex];
    uartRxBufferMainLoopIndex = (uartRxBufferMainLoopIndex + 1) & (sizeof(uartRxBuffer) - 1);
    return byte;
}

ISR_UTX()
{
    // A byte has just started transmitting on TX and there is room in
    // the UART's hardware buffer for us to add another byte.

    if (uartTxBufferInterruptIndex != uartTxBufferMainLoopIndex)
    {
        // There more bytes available in our software buffer, so send
        // the next byte.

        UTXNIF = 0;

        UNDBUF = uartTxBuffer[uartTxBufferInterruptIndex];
        uartTxBufferInterruptIndex = (uartTxBufferInterruptIndex + 1) & (sizeof(uartTxBuffer) - 1);
    }
    else
    {
        // There are no more bytes to send in our buffer, so disable the TX interrupt.
        IEN2 &= ~BV_UTXNIE;
    }
}

ISR_URX()
{
    URXNIF = 0;

    // check for frame and parity errors
    if (!(UNCSR & 0x18)) // UNCSR.FE (4) == 0; UNCSR.ERR (3) == 0
    {
        // There were no errors.

        if (UART_RX_BUFFER_FREE_BYTES())
        {
            // The software RX buffer has space, so add this new byte to the buffer.
            uartRxBuffer[uartRxBufferInterruptIndex] = UNDBUF;
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
        if (UNCSR & 0x10) // UNCSR.FE (4) == 1
        {
            uartNRxFramingErrorOccurred = 1;
        }
        if (UNCSR & 0x08) // UNCSR.ERR (3) == 1
        {
            uartNRxParityErrorOccurred = 1;
        }
    }
}
