/* i2c.c:  Functions for performing implementing master I2C communication
 * in software (the CC2511 does not have a hardware I2C module).
 */

/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>
#include <time.h>
#include <gpio.h>
#include <i2c.h>

#define I2C_PIN_SCL 10 // P1_0
#define I2C_PIN_SDA 11 // P1_1

uint16 XDATA halfPeriodUs = 5; // freq = 100 kHz
uint16 XDATA timeout = 10;
uint8 XDATA started = 0;
uint8 XDATA initialized = 0;

/* i2cTimeoutOccurred is the publicly readable error flag. It must be manually
 * cleared.
 * We have an internal timeout flag too so that e.g. i2cReadByte can abort if
 * i2cReadBit times out, but we can clear the internal flag at the beginning of
 * i2cReadByte so an earlier timeout doesn't affect a later call.
 */
BIT i2cTimeoutOccurred = 0;
BIT internalTimeoutOccurred = 0;


/** Functions *****************************************************************/
void i2cInit(uint16 freqKHz, uint16 timeoutMs)
{
    if (freqKHz == 0)
    {
        freqKHz = 1;
    }

    halfPeriodUs = 500 / freqKHz;
    if (halfPeriodUs == 0)
    {
        halfPeriodUs = 1;
    }

    timeout = timeoutMs;

    setDigitalInput(I2C_PIN_SCL, HIGH_IMPEDANCE);
    setDigitalInput(I2C_PIN_SDA, HIGH_IMPEDANCE);
    initialized = 1;
}

BIT i2cReadScl()
{
    setDigitalInput(I2C_PIN_SCL, HIGH_IMPEDANCE);
    return isPinHigh(I2C_PIN_SCL);
}

BIT i2cReadSda()
{
    setDigitalInput(I2C_PIN_SDA, HIGH_IMPEDANCE);
    return isPinHigh(I2C_PIN_SDA);
}

void i2cClearScl()
{
    setDigitalOutput(I2C_PIN_SCL, LOW);
}

void i2cClearSda()
{
    setDigitalOutput(I2C_PIN_SDA, LOW);
}

void i2cWaitForHighScl(uint16 timeoutMs)
{
    uint32 time = getMs();
    while (i2cReadScl() == 0)
    {
        if (getMs() - time > timeoutMs)
        {
            internalTimeoutOccurred = 1;
            return;
        }
    }
}

/* Generate an I2C STOP condition (P):
 *  SDA goes high while SCL is high
 */
void i2cStop()
{
    i2cClearSda(); // drive SDA low while SCL is low
    delayMicroseconds(halfPeriodUs);

    // handle clock stretching
    i2cWaitForHighScl(timeout);
    if (internalTimeoutOccurred)
    {
        i2cTimeoutOccurred = 1;
        return;
    }

    // SCL is now high
    i2cReadSda(); // let SDA line go high while SCL is high
    delayMicroseconds(halfPeriodUs);
    started = 0;
}

/* Generate an I2C START or repeated START condition (S):
 *  SDA goes low while SCL is high
 */
void i2cStart()
{
    // if started == 1, do a repeated start
    if (started)
    {
        i2cReadSda(); // let SDA line go high while SCL is low
        delayMicroseconds(halfPeriodUs);
    }

    // handle clock stretching
    i2cWaitForHighScl(timeout);
    if (internalTimeoutOccurred)
    {
        // SCL never went high; timeout error
        i2cTimeoutOccurred = 1;
        i2cStop();
        return;
    }

    // SCL is now high
    i2cClearSda(); // drive SDA low while SCL is high
    delayMicroseconds(halfPeriodUs);
    i2cClearScl(); // drive SCL low
    started = 1;
}

/* Write a bit to the I2C bus
 * It is assumed that SCL is low when this function starts.
 * SDA is set to the appropriate bit value while SCL is low, there is a
 * delay for half of the clock period while SDA stablizes, then SCL
 * is allowed to go high for the second half of the clock period, which
 * indicates the on SDA is valid.  This function drives SCL low again
 * before it returns.
 */
void i2cWriteBit(uint8 b)
{
    if (b)
    {
        i2cReadSda(); // let SDA go high
    }
    else
    {
        i2cClearSda(); // drive SDA low
    }
    delayMicroseconds(halfPeriodUs);

    // handle clock stretching
    i2cWaitForHighScl(timeout);
    if (internalTimeoutOccurred)
    {
        // SCL never went high; timeout error
        i2cTimeoutOccurred = 1;
        i2cStop();
        return;
    }

    // SCL is now high, data is valid
    delayMicroseconds(halfPeriodUs);
    i2cClearScl();                    // drive SCL low
}

/* Read a bit to the I2C bus
 * It is assumed that SCL is low when this function starts.
 * The master tristates SDA so the slave transmitter can control the state
 * and delays for half of the clock period (or longer if the slave is holding
 * SCL low).  It then lets SCL go high, records the state of the SDA line,
 * and delays for the second half of the clock period. This function drives SCL
 * low again before it returns. Return value is not meaningful if timeout
 * occurs.
 */
BIT i2cReadBit()
{
    BIT b;

    i2cReadSda(); // let slave transmitter control state of SDA line
    delayMicroseconds(halfPeriodUs);

    // handle clock stretching
    i2cWaitForHighScl(timeout);
    if (internalTimeoutOccurred)
    {
        // SCL never went high; timeout error
        i2cTimeoutOccurred = 1;
        i2cStop();
        return 0;
    }

    // SCL is now high, data is valid
    b = i2cReadSda(); // store state of SDA line now that SCL is high
    delayMicroseconds(halfPeriodUs);
    i2cClearScl(); // drive SCL low
    return b;
}

/* Write a byte to I2C bus. Return 0 if ack by the slave, 1 if nack.
 * The return value is not meaningful if a timeout occurs.
 * */
BIT i2cWriteByte(uint8 byte, uint8 sendStart, uint8 sendStop)
{
    uint8 i;
    uint8 nack;

    internalTimeoutOccurred = 0;

    if (sendStart)
    {
        i2cStart();
        if (internalTimeoutOccurred)
        {
            i2cTimeoutOccurred = 1;
            return 0;
        }
    }
    for (i = 0; i < 8; i++)
    {
        i2cWriteBit(byte & 0x80);
        if (internalTimeoutOccurred)
        {
            i2cTimeoutOccurred = 1;
            return 0;
        }
        byte <<= 1;
    }
    nack = i2cReadBit();
    if ((sendStop && nack != 255) || nack == 1)
    {
        i2cStop();
        if (internalTimeoutOccurred)
        {
            i2cTimeoutOccurred = 1;
            return 0;
        }
    }
    return nack;
}

/* Read a byte from I2C bus */
uint8 i2cReadByte(uint8 nack, uint8 sendStop)
{
    uint16 byte = 0;
    uint8 b, i;

    internalTimeoutOccurred = 0;

    for (i = 0; i < 8; i++)
    {
        b = i2cReadBit();
        if (internalTimeoutOccurred)
        {
            i2cTimeoutOccurred = 1;
            return 0;
        }
        byte = (byte << 1) | b;
    }
    i2cWriteBit(nack);
    if (sendStop)
    {
        i2cStop();
        if (internalTimeoutOccurred)
        {
            i2cTimeoutOccurred = 1;
            return 0;
        }
    }
    return byte;
}
