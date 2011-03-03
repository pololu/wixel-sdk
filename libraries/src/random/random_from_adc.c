/*! \file random_from_adc.c
 * See random.h for more information.
 */

#include <cc2511_types.h>
#include <cc2511_map.h>
#include "random.h"

/* A note about RNDL: When we write to RNDL, what actually happens is that
 * the previous contents of RNDL moves to RNDH before the write happens.
 * If you write to RNDL once, you throw away the 8 bits that were in RNDH.
 * If you write to RNDL twice, you completely throw away all the previous randomness.
 */


static void adcReadTemp()
{
    ADCIF = 0;               // Clear the flag.
    ADCCON3 = 0b10001110;    // Read the temperature sensor using VDD as a reference, with only 7 bits of resolution.
    while(!ADCIF){};         // Wait for the reading to finish.

}

static void writeRndlFromAdc()
{
	adcReadTemp();
    RNDL = ADCL;             // At this point, the 6 MSbs of ADCL are basically random.  Add them to the LFSR.
}

void randomSeedFromAdc()
{
    writeRndlFromAdc();                      // Write to RNDL twice to seed the LFSR.
    writeRndlFromAdc();
    randomNumber();
    randomNumber();
    randomNumber();
}

// The function below was commented out because it was not being used and might
// not be needed:
/* Adds another seed to the state of the random number generator.
 * If you do this regularly, it will help guarantee that no two Wixels have the
 * same random number stream (at least not for long).
 */
/*
void randomRefreshFromAdc()
{
	adcReadTemp();

    while(ADCCON1 & 0x0C);                   // Wait for the last random number to finish.
    RNDL = RNDH ^ ADCL;                      // Add the seed, but don't throw away the randomness in RNDH.
    ADCCON1 = (ADCCON1 & ~0x0C) | 0x07;      // Start generating the next random number.
}*/

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
