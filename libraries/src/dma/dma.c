#include <cc2511_map.h>
#include <cc2511_types.h>
#include <dma.h>

DMA14_CONFIG XDATA dmaConfig;

void dmaInit()
{
    DMA1CFG = (uint16)&dmaConfig;
}
