#ifndef _RANDOM_H
#define _RANDOM_H

#include <cc2511_types.h>

void randomSeedFromAdc();
void randomRefreshFromAdc();
uint8 randomNumber();

#endif
