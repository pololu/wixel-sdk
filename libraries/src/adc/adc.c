#include <cc2511_map.h>
#include <cc2511_types.h>
#include <adc.h>

uint16 adcRead(uint8 channel)
{
    ADCIF = 0;               // Clear the flag.
    ADCCON3 = 0b10110000 | channel;
    while(!ADCIF){};         // Wait for the reading to finish.
    return ADC;
}
