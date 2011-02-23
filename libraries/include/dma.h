#ifndef _DMA_H_
#define _DMA_H_

#include <cc2511_map.h>

#define DMA_CHANNEL_RADIO  1

void dmaInit();

typedef struct DMA14_CONFIG
{
    volatile DMA_CONFIG radio;   // DMA channel: for RF purposes
    volatile DMA_CONFIG __1;     // DMA channel (unassigned)
    volatile DMA_CONFIG __2;     // DMA channel (unassigned)
    volatile DMA_CONFIG __3;     // DMA channel (unassigned)
} DMA14_CONFIG;

extern DMA14_CONFIG XDATA dmaConfig;

// NOTE: DMA channel 0 is not in this struct because the CC2511 lets it is
// configuration be anywhere in memory.

#endif
