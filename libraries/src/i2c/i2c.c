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

#define I2C_SCL_TIMEOUT 10 // ms
#define I2C_ERROR 255

uint16 halfPeriodUs = 5;        // freq = 100 kHz
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

    setDigitalInput(I2C_PIN_SCL, HIGH_IMPEDANCE);
    setDigitalInput(I2C_PIN_SDA, HIGH_IMPEDANCE);
    initialized = 1;
}

uint8 i2cReadScl()
{
    setDigitalInput(I2C_PIN_SCL, HIGH_IMPEDANCE);
    return isPinHigh(I2C_PIN_SCL);
}

uint8 i2cReadSda()
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

uint8 i2cWaitForHighScl(uint16 timeoutMs)
{
    uint32 time = getMs();
    while (i2cReadScl() == 0)
    {
        if (getMs() - time > timeoutMs)
            return I2C_ERROR;
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
        i2cReadSda();            // let SDA line go high while SCL is low
        delayMicroseconds(halfPeriodUs);
    }
    if (i2cWaitForHighScl(I2C_SCL_TIMEOUT))            // handle clock stretching
    {
        i2cStop();
        return I2C_ERROR;                    // SCL never went high; timeout error
    }
    // SCL is now high
    i2cClearSda();        // drive SDA low while SCL is high
    delayMicroseconds(halfPeriodUs);
    i2cClearScl();        // drive SCL low
    started = 1;
    return 0;
}

/* Generate an I2C STOP condition (P):
 *  SDA goes high while SCL is high
 */
uint8 i2cStop()
{
    i2cClearSda();        // drive SDA low while SCL is low
    delayMicroseconds(halfPeriodUs);
    if (i2cWaitForHighScl(I2C_SCL_TIMEOUT))          // handle clock stretching
    {
        return I2C_ERROR;
    }
    // SCL is now high
    i2cReadSda();        // let SDA line go high while SCL is high
    delayMicroseconds(halfPeriodUs);
    started = 0;
    return 0;
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
        i2cReadSda();    // let SDA go high
    }
    else
    {
        i2cClearSda();    // drive SDA low
    }
    delayMicroseconds(halfPeriodUs);
    if (i2cWaitForHighScl(I2C_SCL_TIMEOUT))            // handle clock stretching
    {
        i2cStop();
        return I2C_ERROR;                    // SCL never went high; timeout error
    }
    // SCL is now high, data is valid
    delayMicroseconds(halfPeriodUs);
    i2cClearScl();                    // drive SCL low
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
    i2cReadSda();    // let slave transmitter control state of SDA line
    delayMicroseconds(halfPeriodUs);
    if (i2cWaitForHighScl(I2C_SCL_TIMEOUT))            // handle clock stretching
    {
        i2cStop();
        return I2C_ERROR;                    // SCL never went high; timeout error
    }
    // SCL is now high, data is valid
    b = i2cReadSda();    // store state of SDA line now that SCL is high
    delayMicroseconds(halfPeriodUs);
    i2cClearScl();                    // drive SCL low
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
        {
            return I2C_ERROR;
        }
    }
    for (i = 0; i < 8; i++)
    {
        if (i2cWriteBit(byte & 0x80))
            return I2C_ERROR;
        byte <<= 1;
    }
    nack = i2cReadBit();
    if ((sendStop && nack != 255) || nack == 1)
    {
        if (i2cStop())
        {
            return I2C_ERROR;
        }
    }
    return nack;
}

/* Read a byte from I2C bus */
uint16 i2cReadByte(uint8 nack, uint8 sendStop)
{
    uint16 byte = 0;
    uint8 b, i;
    for (i = 0; i < 8; i++)
    {
        b = i2cReadBit();
        if (b == I2C_ERROR)
        {
            return (uint16)I2C_ERROR << 8;
        }
        byte = (byte << 1) | b;
    }
    i2cWriteBit(nack);
    if (sendStop)
    {
        if (i2cStop())
        {
            byte |= ((uint16)I2C_ERROR << 8);
        }
    }
    return byte;
}
