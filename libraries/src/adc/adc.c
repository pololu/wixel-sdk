#include <cc2511_map.h>
#include <cc2511_types.h>
#include <adc.h>

uint16 adcRead(uint8 channel)
{
    int16 result;
    ADCIF = 0;               // Clear the flag.
    ADCCON3 = 0b10110000 | channel;
    while(!ADCIF){};         // Wait for the reading to finish.

    // The result of the conversion is now sitting in the ADCH and ADCL
    // registers.  Despite what the datasheet says, this result can
    // sometimes be negative, and bits 2 and 3 of ADCL are not always
    // zero (they seem to be pretty random).

    result = ADC;
    if (result < 0)
    {
        return 0;
    }
    else
    {
        return result >> 4;
    }
}
