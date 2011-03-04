/*! \file random.h
 * <code>random.lib</code> is a library that uses the Random Number Generator
 * (otherwise known as the Linear Feedback Shift Register (LFSR) or the
 * CRC module)
 * of the CC251x to generate a pseudo random sequence of numbers.
 *
 * WARNING: The numbers generated are highly predictable if you know what the
 * previous number was.
 */

#ifndef _RANDOM_H
#define _RANDOM_H

#include <cc2511_types.h>

/*! Uses two noisy ADC readings (of the internal temperature sensor) to initialize the
 * state of the random number generator.
 * This function throws away all of the previous state of the random number generator.
 * You will generally want to call this function once at the beginning of your program.
 *
 * Side effects: This function changes ADCL, ADCH, ADCCON3, and ADCIF in order to use the
 * extra conversion feature of the ADC.  This may make it incompatible with other code that
 * uses the ADC.
 */
void randomSeedFromAdc(void);

/*! Uses the randomly-assigned 4-byte serial number of the Wixel to initialize the
 * state of the random number generator.
 * This function throws away all of the previous state of the random number generator.
 * You will generally want to call this function once at the beginning of your program.
 */
void randomSeedFromSerialNumber(void);


/*! \return a random number between 0 and 255.
 * Before calling this function, you should call randomSeedFromAdc or
 * randomSeedFromSerialNumber to initialize the random number generator.
 */
uint8 randomNumber(void);


/* Initializes the random number generator using the specified 16-bit seed.
 * \param seed_msb Any number between 0 and 255.
 * \param seed_lsb Any number between 0 and 255.
 *
 * The parameters to this function determine the sequence of random numbers
 * produces by the random number generator.
 */
void randomSeed(uint8 seed_msb, uint8 seed_lsb);

#endif
