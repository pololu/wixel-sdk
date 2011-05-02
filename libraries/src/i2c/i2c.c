/* i2c.c: A basic software implementation of a master node for I2C communication
 * (the CC2511 does not have a hardware I2C module). This library does not
 * support multi-master I2C buses.
 */

/* Dependencies ***************************************************************/

#include <cc2511_map.h>
#include <board.h>
#include <time.h>
#include <gpio.h>
#include <i2c.h>

/* Global Constants & Variables ***********************************************/

uint8 DATA i2cPinScl = 10; // P1_0
uint8 DATA i2cPinSda = 11; // P1_1

static uint16 XDATA halfPeriodUs = 5; // freq = 100 kHz
static uint16 XDATA timeout = 10;
static BIT started = 0;

/* i2cTimeoutOccurred is the publicly readable error flag. It must be manually
 * cleared.
 * We have an internal timeout flag too so that e.g. i2cReadByte can abort if
 * i2cReadBit times out, but we can clear the internal flag at the beginning of
 * i2cReadByte so an earlier timeout doesn't affect a later call.
 */
BIT i2cTimeoutOccurred = 0;
static BIT internalTimeoutOccurred = 0;


/* Functions ******************************************************************/

void i2cSetFrequency(uint16 freqKHz)
{
    // delayMicroseconds takes a uint8, so halfPeriodUs cannot be more than 255
    if (freqKHz < 2)
    {
        freqKHz = 2;
    }

    // force halfPeriodUs to round up so we don't use a higher frequency than what was chosen
    // TODO: implement a timing function with better resolution than delayMicroseconds to allow finer-grained frequency control?
    halfPeriodUs = (500 + freqKHz - 1) / freqKHz;
}

void i2cSetTimeout(uint16 timeoutMs)
{
    timeout = timeoutMs;
}

BIT i2cReadScl(void)
{
    setDigitalInput(i2cPinScl, HIGH_IMPEDANCE);
    return isPinHigh(i2cPinScl);
}

BIT i2cReadSda(void)
{
    setDigitalInput(i2cPinSda, HIGH_IMPEDANCE);
    return isPinHigh(i2cPinSda);
}

void i2cClearScl(void)
{
    setDigitalOutput(i2cPinScl, LOW);
}

void i2cClearSda(void)
{
    setDigitalOutput(i2cPinSda, LOW);
}

void i2cWaitForHighScl(uint16 timeoutMs)
{
    uint32 time = getMs();
    while (i2cReadScl() == 0)
    {
        if (getMs() - time > timeoutMs)
        {
            internalTimeoutOccurred = 1;
            i2cTimeoutOccurred = 1;
            started = 0;
            return;
        }
    }
}

/* Generate an I2C STOP condition (P):
 *  SDA goes high while SCL is high
 */
void i2cStop(void)
{
    i2cClearSda(); // drive SDA low while SCL is low
    delayMicroseconds(halfPeriodUs);

    // handle clock stretching
    i2cWaitForHighScl(timeout);
    if (internalTimeoutOccurred) return;

    // SCL is now high
    i2cReadSda(); // let SDA line go high while SCL is high
    delayMicroseconds(halfPeriodUs);
    started = 0;
}

/* Generate an I2C START or repeated START condition (S):
 *  SDA goes low while SCL is high
 */
void i2cStart(void)
{
    // if started == 1, do a repeated start
    if (started)
    {
        i2cReadSda(); // let SDA line go high while SCL is low
        delayMicroseconds(halfPeriodUs);
    }

    // handle clock stretching
    i2cWaitForHighScl(timeout);
    if (internalTimeoutOccurred) return;

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
void i2cWriteBit(BIT b)
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
    if (internalTimeoutOccurred) return;

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
BIT i2cReadBit(void)
{
    BIT b;

    i2cReadSda(); // let slave transmitter control state of SDA line
    delayMicroseconds(halfPeriodUs);

    // handle clock stretching
    i2cWaitForHighScl(timeout);
    if (internalTimeoutOccurred) return 0;

    // SCL is now high, data is valid
    b = i2cReadSda(); // store state of SDA line now that SCL is high
    delayMicroseconds(halfPeriodUs);
    i2cClearScl(); // drive SCL low
    return b;
}

/* Write a byte to I2C bus. Return 0 if ack by the slave, 1 if nack.
 * The return value is not meaningful if a timeout occurs.
 */
BIT i2cWriteByte(uint8 byte)
{
    uint8 i;
    BIT nack;

    internalTimeoutOccurred = 0;

    for (i = 0; i < 8; i++)
    {
        i2cWriteBit(byte & 0x80);
        if (internalTimeoutOccurred) return 0;
        byte <<= 1;
    }
    nack = i2cReadBit();
    if (internalTimeoutOccurred) return 0;

    if (nack)
    {
        i2cStop();
        if (internalTimeoutOccurred) return 0;
    }
    return nack;
}

/* Read a byte from I2C bus.
 * The return value is not meaningful if a timeout occurs.
 */
uint8 i2cReadByte(BIT nack)
{
    uint16 byte = 0;
    uint8 i;
    BIT b;

    internalTimeoutOccurred = 0;

    for (i = 0; i < 8; i++)
    {
        b = i2cReadBit();
        if (internalTimeoutOccurred) return 0;
        byte = (byte << 1) | b;
    }

    i2cWriteBit(nack);
    if (internalTimeoutOccurred) return 0;

    return byte;
}
