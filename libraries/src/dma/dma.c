#include <cc2511_map.h>
#include <cc2511_types.h>
#include "dma.h"

// This structure in XDATA holds the configuration options
// for DMA channels 1-4.  The reason we have to do it this way is
// because the CC2511's DMA controller expects the configurations
// of those channels to be next to eachother in memory.
DMA14_CONFIG XDATA dmaConfig;

void dmaInit()
{
    DMA1CFG = (uint16)&dmaConfig;
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
