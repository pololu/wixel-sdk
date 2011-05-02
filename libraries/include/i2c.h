/*! \file i2c.h
 * The <code>i2c.lib</code> library provides a basic software (bit-banging)
 * implementation of a master node for I<sup>2</sup>C communication (the CC2511
 * does not have a hardware I<sup>2</sup>C module). This library does not
 * support multi-master I<sup>2</sup>C buses.
 *
 * By default, the SCL pin is assigned to P1_0, the SDA pin is
 * assigned to P1_1, and the bus frequency is 100 kHz with a 10 ms timeout.
 */

#ifndef _I2C_H
#define _I2C_H

#include <cc2511_types.h>

/*! Number of the pin to use as the SCL (clock) line of the I<sup>2</sup>C bus.
 * See the gpio.h documentation for pin number values.
 */
extern uint8 DATA i2cPinScl;

/*! Number of the pin to use as the SDA (data) line of the I<sup>2</sup>C bus.
 * See the gpio.h documentation for pin number values.
 */
extern uint8 DATA i2cPinSda;

/*! This bit is a flag that is set whenever a timeout occurs on the
 * I<sup>2</sup>C bus waiting for the SCL line to go high. The flag must be
 * manually cleared after being read. The i2cSetTimeout() function can be used
 * to specify the timeout delay.
 */
extern BIT i2cTimeoutOccurred;

/*! Sets the I<sup>2</sup>C bus clock frequency. This implementation limits the
 * range of possible frequencies to 2-500 kHz; because of rounding inaccuracies and timing constraints, the actual frequency might be lower than
 * the selected frequency, but it is guaranteed never to be higher. The default
 * frequency is 100 kHz. Common I<sup>2</sup>C speeds are 10 kHz (low speed),
 * 100 kHz (standard), and 400 kHz (high speed).
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
void i2cStart(void);

/*! Generates an I<sup>2</sup>C STOP condition.
 */
void i2cStop(void);

/*! Writes a byte to the I<sup>2</sup>C bus (performs a master transmit).
 *
 * \param byte      The byte to be transmitted.
 *
 * \return  0 if an ACK is received from the slave device, 1 if a NACK is
 *          received. This return value is not meaningful if a timeout occurs
 *          during the write transaction (indicated by the #i2cTimeoutOccurred
 *          flag).
 */
BIT i2cWriteByte(uint8 byte);

/*! Reads a byte from the I<sup>2</sup>C bus (performs a master receive).
 *
 * \param nack      If 1, a NACK will be sent to the slave device instead of an
 *                  ACK after this byte is received. (This is used to signal
 *                  conclusion of a transfer from the slave to the master.)
 *
 * \return  The byte received from the slave device. This return value is not
 *          meaningful if a timeout occurs during the read transaction
 *          (indicated by the #i2cTimeoutOccurred flag).
 */
uint8 i2cReadByte(BIT nack);

#endif
