#include <cc2511_map.h>
#include <cc2511_types.h>
#include <adc.h>

uint16 adcRead(uint8 channel)
{
    ADCIF = 0;               // Clear the flag.
    ADCCON3 = 0b10110000 ^ channel;
    while(!ADCIF){};         // Wait for the reading to finish.

    if (ADCH & 0x80)
    {
        // Despite what the datasheet says, the result was negative.
        return 0;
    }
    else
    {
        // Note: Despite what the datasheet says, bits 2 and 3 of ADCL are not
        // always zero (they seem to be pretty random).  We throw them away
        // here.
        return ADC >> 4;
    }
}

int16 adcReadDifferential(uint8 channel)
{
    ADCIF = 0;               // Clear the flag.
    ADCCON3 = 0b10110000 ^ channel;
    while(!ADCIF){};         // Wait for the reading to finish.

    return (int16)ADC >> 4;
}
