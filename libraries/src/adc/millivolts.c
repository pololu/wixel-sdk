#include <cc2511_types.h>
#include "adc.h"

static uint16 millivoltCalibration = 3300;

uint16 adcReadVddMillivolts()
{
    //return adcRead(15|ADC_REFERENCE_INTERNAL);
    return ((uint32)adcRead(15|ADC_REFERENCE_INTERNAL)*3750 + 1023) / 2047;
}

void adcSetMillivoltCalibration(uint16 vddMillivolts)
{
    millivoltCalibration = vddMillivolts;
}

int16 adcConvertToMillivolts(int16 adcResult)
{
    return ((int32)adcResult * millivoltCalibration + 1023) / 2047;
}
