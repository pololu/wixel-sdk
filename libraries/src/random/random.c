#include <cc2511_types.h>
#include <cc2511_map.h>
#include "random.h"

/* Returns a new random number.  Warning: this is not reentrant.  It is possible that this function
 * could occasionally return the same random number to an ISR caller as it returns to a main loop
 * caller.
 */
uint8 randomNumber()
{
    uint8 rand;
    while(ADCCON1 & 0x0C);                   // Wait for the random number to finish.
    rand = RNDL;                             // Get the random number.
    ADCCON1 = (ADCCON1 & ~0x0F) | 0x07;      // Start generating the next random number.
    return rand;
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
