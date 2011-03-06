/*! \file random_from_sernum.c
 * See random.h for more information about this library.
 */

#include <cc2511_map.h>
#include <random.h>
#include <board.h>

void randomSeedFromSerialNumber(void)
{
    // The random number generator only has 16 bits of state, while the
    // serial number is 32 bits.  No matter what we do here, there will be
    // a 1-in-2^16 chance that two Wixels with different serial numbers
    // start up with their random number generators in the same state.
    // So there is no point in reading all 4 bytes of the serial number.

    randomSeed(serialNumber[0], serialNumber[1]);
}
