/* i2c.c:  Functions for performing implementing master I2C communication
 * in software (the CC2511 does not have a hardware I2C module).
 */

/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>
#include <time.h>
#include <i2c.h>



uint16 halfPeriodUs = 5;		// freq = 100 kHz
uint8 started = 0;
uint8 initialized = 0;



/** Functions *****************************************************************/
void i2cInit(uint16 kHzFreq)
{
	if (kHzFreq == 0)
		kHzFreq = 1;
	halfPeriodUs = 500 / kHzFreq;
	if (halfPeriodUs == 0)
		halfPeriodUs = 1;

	P1DIR &= ~(1 << 0) & ~(1 << 1);	// make sure P1_0 and P1_1 are inputs
    P1_0 = 0;						// P1_0 (SCL) drives low when set as output
    P1_1 = 0;						// P1_1 (SDA) drives low when set as output
    initialized = 1;
}

// SCL is P1_0
uint8 i2cReadScl()
{
	P1DIR &= ~(1 << 0);				// make P1_0 (SCL) a tristated input
	return P1_0;					// return input value of P1_0
}

// SDA is P1_1
uint8 i2cReadSda()
{
	P1DIR &= ~(1 << 1);				// make P1_1 (SDA) a tristated input
	return P1_1;					// return input value of P1_1
}

void i2cClearScl()
{
	P1DIR |= 1 << 0;				// make P1_0 (SCL) a driving-low output
}

void i2cClearSda()
{
	P1DIR |= 1 << 1;				// make P1_1 (SDA) a driving-low output
}

uint8 i2cWaitForHighScl(uint16 timeoutMs)
{
	uint32 time = getMs();
	while (i2cReadScl() == 0)
	{
		if (getMs() - time > timeoutMs)
			return 255;
	}
	return 0;
}

/* Generate an I2C START or repeated START condition (S):
 *  SDA goes low while SCL is high
 */
uint8 i2cStart()
{
	// if started == 1, do a repeated start
	if (started)
	{
		i2cReadSda();			// let SDA line go high while SCL is low
		delayMicroseconds(halfPeriodUs);
	}
	if (i2cWaitForHighScl(10))			// handle clock stretching
	{
		i2cStop();
		return 255;					// SCL never went high; timeout error
	}
	// SCL is now high
	/*if (i2cReadSda() == 0)
	{
		ARBITRATION_LOST();
	}*/
	i2cClearSda();		// drive SDA low while SCL is high
	delayMicroseconds(halfPeriodUs);
	i2cClearScl();		// drive SCL low
	started = 1;
	return 0;
}

/* Generate an I2C STOP condition (P):
 *  SDA goes high while SCL is high
 */
void i2cStop()
{
	i2cClearSda();		// drive SDA low while SCL is low
	delayMicroseconds(halfPeriodUs);
	i2cWaitForHighScl(10);	// let SCL go high, handle clock stretching
	// TODO: detect timeout and indicate an error

	/*if (i2cReadSda() == 0)
	{
		ARBITRATION_LOST();
	}*/
	i2cReadSda();		// let SDA line go high while SCL is high
	delayMicroseconds(halfPeriodUs);
	started = 0;
}

/* Write a bit to the I2C bus
 * It is assumed that SCL is low when this function starts.
 * SDA is set to the appropriate bit value while SCL is low, there is a
 * delay for half of the clock period while SDA stablizes, then SCL
 * is allowed to go high for the second half of the clock period, which
 * indicates the on SDA is valid.  This function drives SCL low again
 * before it returns.
 */
uint8 i2cWriteBit(uint8 b)
{
	if (b)
	{
		i2cReadSda();	// let SDA go high
	}
	else
	{
		i2cClearSda();	// drive SDA low
	}
	delayMicroseconds(halfPeriodUs);
	if (i2cWaitForHighScl(10))			// handle clock stretching
	{
		i2cStop();
		return 255;					// SCL never went high; timeout error
	}
	// SCL is now high, data is valid
	/*if (bitVal && readSDA() == 0)
	{
		ARBITRATION_LOST();
	}*/
	delayMicroseconds(halfPeriodUs);
	i2cClearScl();					// drive SCL low
	return 0;
}

/* Read a bit to the I2C bus
 * It is assumed that SCL is low when this function starts.
 * The master tristates SDA so the slave transmitter can control the state
 * and delays for half of the clock period (or longer if the slave is holding
 * SCL low).  It then lets SCL go high, records the state of the SDA line,
 * and delays for the second half of the clock period. This function drives SCL
 * low again before it returns.  If the function times out waiting for the
 * slave to let SCL go high, the return value is 255.  Otherwise, it is the
 * value of the read bit (0 or 1).
 */
uint8 i2cReadBit()
{
	uint8 b;
	i2cReadSda();	// let slave transmitter control state of SDA line
	delayMicroseconds(halfPeriodUs);
	if (i2cWaitForHighScl(10))			// handle clock stretching
	{
		i2cStop();
		return 255;					// SCL never went high; timeout error
	}
	// SCL is now high, data is valid
	b = i2cReadSda();	// store state of SDA line now that SCL is high
	delayMicroseconds(halfPeriodUs);
	i2cClearScl();					// drive SCL low
	return b;
}

/* Write a byte to I2C bus. Return 0 if ack by the slave */
uint8 i2cWriteByte(uint8 byte, uint8 sendStart, uint8 sendStop)
{
	uint8 i;
	uint8 nack;

	if (sendStart)
	{
		if (i2cStart())
			return 255;
	}
	for (i = 0; i < 8; i++)
	{
		if (i2cWriteBit(byte & 0x80))
			return 255;
		byte <<= 1;
	}
	nack = i2cReadBit();
	if ((sendStop && nack != 255) || nack == 1)
	{
		i2cStop();
	}
	return nack;
}

/* Read a byte from I2C bus */
uint16 i2cReadByte(uint8 nack, uint8 sendStop)
{
	uint8 byte = 0;
	uint8 i;
	for (i = 0; i < 8; i++)
	{
		byte <<= 1;
		byte |= i2cReadBit();
	}
	i2cWriteBit(nack);
	if (sendStop)
	{
		i2cStop();
	}
	return byte;
}
