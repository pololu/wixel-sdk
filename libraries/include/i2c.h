/*! \file i2c.h
 * The <code>i2c.lib</code> library provides a basic software (bit-banging)
 * implementation of a master node for I<sup>2</sup>C communication (the CC2511
 * does not have a hardware I<sup>2</sup>C module). This library does not
 * support multi-master I<sup>2</sup>C buses.
 *
 * This library defaults to operation at 100 kHz with a 10 ms timeout, unless
 * these settings are changed with the i2cSetFrequency() and i2cSetTimeout()
 * functions.
 */

#ifndef _I2C_H
#define _I2C_H

#include <cc2511_types.h>

/*! This bit is a flag that is set whenever a timeout occurs on the
 * I<sup>2</sup>C bus waiting for the SCL line to go high. The flag must be
 * manually cleared after being read. The i2cSetTimeout() function can be used
 * to specify the timeout delay.
 */
extern BIT i2cTimeoutOccurred;

/*! Sets the I<sup>2</sup>C bus clock frequency. Because of rounding
 * inaccuracies and timing constraints, the actual frequency might be lower than
 * the selected frequency, but it is guaranteed never to be higher. The default
 * frequency is 100 kHz.
 *
 * \param freqKHz Frequency in kHz.
 */
void i2cSetFrequency(uint16 freqKHz);

/*! Sets the allowed delay before a low SCL line causes an I<sup>2</sup>C bus
 * timeout, which aborts the I<sup>2</sup>C transaction and sets the
 * #i2cTimeoutOccurred flag. The default timeout is 10 ms.
 *
 * \param timeoutMs Timeout in milliseconds.
 */
void i2cSetTimeout(uint16 timeoutMs);

/*! Generates an I<sup>2</sup>C START condition.
 */
void i2cStart();

/*! Generates an I<sup>2</sup>C STOP condition.
 */
void i2cStop();

/*! Writes a byte to the I<sup>2</sup>C bus (performs a master transmit).
 *
 * \param byte      The byte to be transmitted.
 * \param sendStart If 1, an I<sup>2</sup>C START condition will be generated
 *                  before this byte is transmitted. (This is equivalent to
 *                  calling i2cStart(), then i2cWriteByte() with sendStart = 0.)
 * \param sendStop  If 1, an I<sup>2</sup>C STOP condition will be generated
 *                  after this byte is transmitted. (This is equivalent to
 *                  calling i2cWriteByte() with sendStop = 0, then i2cStop().)
 *
 * \return  0 if an ACK is received from the slave device, 1 if a NACK is
 *          received. This return value is not meaningful if a timeout occurs
 *          during the write transaction (indicated by the #i2cTimeoutOccurred
 *          flag).
 */
BIT i2cWriteByte(uint8 byte, BIT sendStart, BIT sendStop);

/*! Reads a byte from the I<sup>2</sup>C bus (performs a master receive).
 *
 * \param nack      If 1, a NACK will be sent to the slave device instead of an
 *                  ACK after this byte is received. (This is used to signal
 *                  conclusion of a transfer from the slave to the master.)
 * \param sendStop  If 1, an I<sup>2</sup>C STOP condition will be generated
 *                  after this byte is received. (This is equivalent to calling
 *                  i2cReadByte() with sendStop = 0, then i2cStop().)
 *
 * \return  The byte received from the slave device. This return value is not
 *          meaningful if a timeout occurs during the read transaction
 *          (indicated by the #i2cTimeoutOccurred flag).
 */
uint8 i2cReadByte(BIT nack, BIT sendStop);

#endif
