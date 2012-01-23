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
#define ISR_URX()                   ISR(URX0, 0)
#define URXNIF                      URX0IF
#define URXNIE                      URX0IE
#define UNGCR                       U0GCR
#define UNBAUD                      U0BAUD
#define UNDBUF                      U0DBUF
#define spiNMasterInit              spi0MasterInit
#define spiNMasterSetFrequency      spi0MasterSetFrequency
#define spiNMasterSetClockPolarity  spi0MasterSetClockPolarity
#define spiNMasterSetClockPhase     spi0MasterSetClockPhase
#define spiNMasterSetBitOrder       spi0MasterSetBitOrder
#define spiNMasterBusy              spi0MasterBusy
#define spiNMasterBytesLeft         spi0MasterBytesLeft
#define spiNMasterTransfer          spi0MasterTransfer
#define spiNMasterSendByte          spi0MasterSendByte
#define spiNMasterReceiveByte       spi0MasterReceiveByte

#elif defined(SPI1)
#include <spi1_master.h>
#define INTERRUPT_PRIORITY_GROUP    3
#define ISR_URX()                   ISR(URX1, 0)
#define URXNIF                      URX1IF
#define URXNIE                      URX1IE
#define UNGCR                       U1GCR
#define UNBAUD                      U1BAUD
#define UNDBUF                      U1DBUF
#define spiNMasterInit              spi1MasterInit
#define spiNMasterSetFrequency      spi1MasterSetFrequency
#define spiNMasterSetClockPolarity  spi1MasterSetClockPolarity
#define spiNMasterSetClockPhase     spi1MasterSetClockPhase
#define spiNMasterSetBitOrder       spi1MasterSetBitOrder
#define spiNMasterBusy              spi1MasterBusy
#define spiNMasterBytesLeft         spi1MasterBytesLeft
#define spiNMasterTransfer          spi1MasterTransfer
#define spiNMasterSendByte          spi1MasterSendByte
#define spiNMasterReceiveByte       spi1MasterReceiveByte
#endif

// txPointer points to the last byte that was written to SPI.
static volatile const uint8 XDATA * DATA txPointer = 0;

// rxPointer points to the location to store the next byte received from SPI.
static volatile uint8 XDATA * DATA rxPointer = 0;

// bytesLeft is the number of bytes we still need to send to/receive from SPI.
static volatile uint16 DATA bytesLeft = 0;

void spiNMasterInit(void)
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

    // Assumption: The MODE and SLAVE bits in U0CSR/U1CSR are 0 (the default) so
    // the USART is already in SPI Master mode.

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
    UNGCR &= 0xE0; // preserve CPOL, CPHA, ORDER (7:5)
    UNGCR |= baudE; // UNGCR.BAUD_E (4:0)
    UNBAUD = baudMPlus256; // UNBAUD.BAUD_M (7:0) - only the lowest 8 bits of baudMPlus256 are used, so this is effectively baudMPlus256 - 256
}

void spiNMasterSetClockPolarity(BIT polarity)
{
    if (polarity == SPI_POLARITY_IDLE_LOW)
    {
        UNGCR &= ~(1<<7);   // SCK idle low (negative polarity)
    }
    else
    {
        UNGCR |= (1<<7);    // SCK idle high (positive polarity)
    }
}

void spiNMasterSetClockPhase(BIT phase)
{
    if (phase == SPI_PHASE_EDGE_LEADING)
    {
        UNGCR &= ~(1<<6);   // data centered on leading (first) edge - rising for idle low, falling for idle high
    }
    else
    {
        UNGCR |= (1<<6);    // data centered on trailing (second) edge - falling for idle low, rising for idle high
    }
}

void spiNMasterSetBitOrder(BIT bitOrder)
{
    if (bitOrder == SPI_BIT_ORDER_LSB_FIRST)
    {
        UNGCR &= ~(1<<5);   // LSB first
    }
    else
    {
        UNGCR |= (1<<5);    // MSB first
    }
}

BIT spiNMasterBusy(void)
{
    return URXNIE;
}

uint16 spiNMasterBytesLeft(void)
{
    uint16 bytes;

    // bytesLeft is 16 bits, so it takes more than one instruction to read. Disable interrupts so it's not updated while we do this
    URXNIE = 0;
    bytes = bytesLeft;
    if (bytes) URXNIE = 1;

    return bytes;
}

void spiNMasterTransfer(const uint8 XDATA * txBuffer, uint8 XDATA * rxBuffer, uint16 size)
{
    if (size)
    {
        txPointer = txBuffer;
        rxPointer = rxBuffer;
        bytesLeft = size;

        UNDBUF = *txBuffer; // transmit first byte
        URXNIE = 1;         // Enable RX interrupt.
    }
}

uint8 spiNMasterSendByte(uint8 XDATA byte)
{
    uint8 XDATA rxByte;

    rxPointer = &rxByte;
    bytesLeft = 1;

    UNDBUF = byte;
    URXNIE = 1; // Enable RX interrupt.

    while (bytesLeft);
    return rxByte;
}

uint8 spiNMasterReceiveByte(void)
{
    return spiNMasterSendByte(0xFF);
}

ISR_URX()
{
    URXNIF = 0;

    *rxPointer = UNDBUF;
    rxPointer++;
    bytesLeft--;

    if (bytesLeft)
    {
        txPointer++;
        UNDBUF = *txPointer;
    }
    else
    {
        URXNIE = 0;
    }
}
