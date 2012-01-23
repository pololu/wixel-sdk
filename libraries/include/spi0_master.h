/*! \file spi0_master.h
 *
 * The <code>spi_master.lib</code> library allows you to do SPI master
 * communication using USART0 and/or USART1.
 * This library uses interrupts to transfer data so it is capable of
 * sending/receiving in the background while other tasks are performed.
 *
 * To use this library, you must include spi0_master.h or spi1_master.h
 * in your app:
\code
#include <spi0_master.h>
#include <spi1_master.h>
\endcode
 *
 * Since this library uses interrupts, the include statement must be present
 * in the file that contains main().
 *
 * The API for using USART1 is the same as the API for using USART0 that is
 * documented here, except all the function and variable names begin with
 * "spi1Master" instead of "spi0Master".
 *
 * For USART0, this library uses Alternative Location 1: P0_5 is SCK,
 * P0_3 is MOSI, and P0_2 is MISO.
 *
 * For USART1, this library uses Alternative Location 2: P1_5 is SCK,
 * P1_6 is MOSI, P1_7 is MISO.
 *
 * This library does not yet allow you to choose which alternative
 * location to use.
 *
 * Please note that this library only supports SPI <em>master</em>
 * communication; MOSI and SCK are outputs and MISO is an input.
 */

#ifndef _SPI0_MASTER_H
#define _SPI0_MASTER_H

#include <cc2511_map.h>
#include <cc2511_types.h>
#include <spi.h>

/*! Initializes the library.
 *
 * This must be called before any other functions with names that
 * begin with "spi0Master".
 *
 * After calling this, it is recommended to call
 * spi0MasterSetFrequency(), spi0MasterSetClockPolarity(),
 * spi0MasterSetClockPhase(), and spi0MasterSetBitOrder()
 * to set the other parameters of the SPI communication.
 */
void spi0MasterInit(void);

/*! Sets the frequency of the clock signal on SCK.
 *
 * \param freq The frequency, in bits per second.  Must be between 23 and 3,000,000.
 */
void spi0MasterSetFrequency(uint32 freq);

/*! Sets the clock polarity (the bit named CPOL in U0GCR).
 * This bit can be used to invert the signal on SCK.
 * Valid values are:
 *
 * - #SPI_POLARITY_IDLE_LOW (0): The SCK line will be low
 *   when no data is being transferred.
 * - #SPI_POLARITY_IDLE_HIGH (1): The SCK line will be high
 *   when no data is being transferred.
 *
 * For more information, see Figure 41 (SPI Dataflow) in the
 * CC2511F32 datasheet.
 */
void spi0MasterSetClockPolarity(BIT polarity);

/*! Sets the clock phase (the bit named CPHA in U0GCR).
 * This bit controls the phase of the clock,
 * which determines what type of transition is occurring when
 * the data is sampled/transmitted.
 * Valid values are:
 *
 * - #SPI_PHASE_EDGE_LEADING (0): Data is sampled/transmitted
 *   on the leading edge, when the clock line transitions
 *   from idle to active.
 * - #SPI_PHASE_EDGE_TRAILING (1): Data is sampled/transmitted
 *   on the trailing edge, when the clock like transitions
 *   from active to idle.
 *
 * For more information, see Figure 41 (SPI Dataflow) in the
 * CC2511F32 datasheet. */
void spi0MasterSetClockPhase(BIT phase);

/*! Sets the bit order for transfers.
 * Valid values are:
 *
 * - #SPI_BIT_ORDER_MSB_FIRST: The most-significant bit is transmitted first.
 * - #SPI_BIT_ORDER_LSB_FIRST: The least-significant bit is transmitted first. */
void spi0MasterSetBitOrder(BIT bitOrder);

/*! \return 1 if the library is busy transferring of data, 0 if it
    is not busy.

    This is equivalent to <code>spi0MasterBytesLeft() != 0</code>
    but it is faster and doesn't affect the speed of the transfer. */
BIT spi0MasterBusy(void);

/*! \return The number of bytes left to transfer in the current transfer.
 *     If 0, it means there is no current transfer.
 *
 *  This function temporarily disables the interrupt used by this library
 *  to transfer data, so calling this function frequently could reduce the
 *  speed that data is transferred.
 *  If possible, try to use spi0MasterBusy() instead of this function. */
uint16 spi0MasterBytesLeft(void);

/*! Starts a new transfer of data.
 * The transfer will be carried out in the background by interrupts, allowing
 * other tasks to be performed simultaneously.
 * This is a non-blocking function.
 *
 * \param txBuffer A pointer to a buffer holding the bytes to be sent to the SPI slave.
 * \param rxBuffer A pointer to a buffer to hold bytes received by the SPI slave
 *   during this transfer.  This may be equal to txBuffer, which would cause the
 *   transmitted data to be overwritten with received data.
 * \param size The number of bytes to transmit/receive.
 *
 * This function should not be called if the library is busy doing a transfer
 * (i.e. spi0MasterBusy() returns 1).
 *
 * In SPI, every transfer of data consists of one byte going from the master to
 * the slave and one byte going from the slave to the master.
 * Not every byte is meaningful: often dummy data is inserted.
 * This function initiates a transmission of several bytes from the master
 * (the Wixel) to the slave.
 * During this transmission, an equal number of bytes will be received
 * and stored in the RX buffer. */
void spi0MasterTransfer(const uint8 XDATA * txBuffer, uint8 XDATA * rxBuffer, uint16 size);

/*! Transmits one byte to the SPI slave, simultaneously receiving a byte from
 * the slave.  This is a synchronous, blocking function so be careful about using
 * it in apps that have regular tasks to perform.
 *
 * This function should not be called if the library is busy doing a transfer
 * (i.e. spi0MasterBusy() returns 1).
 *
 * \param byte The byte to send to the slave.
 * \return The byte received from the slave.
 */
uint8 spi0MasterSendByte(uint8 XDATA byte);

/*! This is equivalent to:
\code
spi0MasterSendByte(0xFF)
\endcode
*/
uint8 spi0MasterReceiveByte(void);

/*! A prototype for the USART0 interrupt. */
ISR(URX0, 0);

#endif /* SPI0_MASTER_H_ */
