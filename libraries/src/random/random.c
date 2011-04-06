#include <cc2511_types.h>
#include <cc2511_map.h>
#include <random.h>

/* Returns a new random number.  Warning: this is not reentrant.  It is possible that this function
 * could occasionally return the same random number to an ISR caller as it returns to a main loop
 * caller.
 */
uint8 randomNumber()
{
    uint8 rand;
    while(ADCCON1 & 0x0C);                   // Wait for the random number to finish.
    rand = RNDL;                             // Get the random number.
    ADCCON1 = (ADCCON1 & 0x30) | 0x07;       // Start generating the next random number.
    return rand;
}

void randomSeed(uint8 seed_msb, uint8 seed_lsb)
{
    // Rescue the random number from these two bad states: 0x0000 and 0x8003.
    // Without the code below, the random number generator could get stuck in
    // either of these states if it was seeded badly.
    if ((seed_lsb == 0 && seed_msb == 0) || (seed_lsb == 0x03 && seed_msb == 0x80))
    {
        seed_lsb = 0xAA;
    }

    RNDL = seed_msb;
    RNDL = seed_lsb;
    randomNumber();
    randomNumber();
    randomNumber();
}
