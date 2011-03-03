// TODO: make the randomness not depend on the ADC.  Use the serial number instead.

/* random.c:  A library that uses the Random Number Generator of the CC251x to
 * generate pseudo random sequence of numbers.  A function is provided for
 * seeding the random number generator using the ADC, and another function is
 * provided for adding randomness in to the stream at other times.
 *
 * WARNING: The numbers generated are highly predictable if you know what the
 * previous number was.  (David made a scatter plot of current number vs.
 * previous number and it showed several curvy waves.)
 */

#include <cc2511_types.h>
#include <cc2511_map.h>
#include "random.h"

/* A note about RNDL: When we write to RANDL, what actually happens is that
 * the previous contents of RNDL moves to RNDH before the write happens.
 * If you write to RNDL once, you throw away the 8 bits that were in RNDH.
 * If you write to RNDL twice, you completely throw away all the previous randomness.
 */

static void writeRndlFromAdc()
{
    ADCIF = 0;               // Clear the flag.
    ADCCON3 = 0b10001110;    // Read the temperature sensor using VDD as a reference, with only 7 bits of resolution.
    while(!ADCIF){};         // Wait for the reading to finish.
    RNDL = ADCL;             // At this point, the 6 MSbs of ADCL are basically random.  Add them to the LFSR.
}

/* randomSeedFromADC: This function should be called to initialize the state of the
 * random number generator, otherwise known as the Linear Feedback Shift Register (LFSR).
 * This function throw away all of the previous state of the LFSR and replaces it with a
 * new random seed.  The random seed comes from two ADC readings of the internal temp
 * sensor.
 *
 * Side effects: This function changes ADCL, ADCH, ADCCON3, and ADCIF in order to use the
 * extra conversion feature of the ADC.  This may make it incompatible with other code that
 * uses the ADC if that
 */
void randomSeedFromAdc()
{
    writeRndlFromAdc();                      // Write to RNDL twice to seed the LFSR.
    writeRndlFromAdc();
    ADCCON1 = (ADCCON1 & ~0x0C) | 0x07;      // Start generating the first random number.
}

/* randomAddSeed: Adds another seed to the state of the random number generator.
 * If you do this regularly, it will help guarantee that no two Wixel's have the
 * same random number stream (at least not for long).
 */
void randomRefreshFromAdc()
{
    ADCIF = 0;                               // Clear the flag.
    ADCCON3 = 0b10001110;                    // Read the temperature sensor using VDD as a reference, with only 7 bits of resolution.
    while(!ADCIF){};                         // Wait for the reading to finish.

    while(ADCCON1 & 0x0C);                   // Wait for the last random number to finish.
    RNDL = RNDH ^ ADCL;                      // Add the seed, but don't throw away the randomness in RNDH.
    ADCCON1 = (ADCCON1 & ~0x0C) | 0x07;      // Start generating the next random number.
}

/* Returns a new random number.  Warning: this is not reentrant.  It is possible that this function
 * could occasionally return the same random number to an ISR caller as it returns to a main loop
 * caller.
 */
uint8 randomNumber()
{
    uint8 rand;
    while(ADCCON1 & 0x0C);                   // Wait for the random number to finish.
    rand = RNDL;                             // Get the random number.
    ADCCON1 = (ADCCON1 & ~0x0C) | 0x07;      // Start generating the next random number.
    return rand;
}
