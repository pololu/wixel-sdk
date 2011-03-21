/*! \file dma.h
 * This file provides basic functions and variables for the CC2511's
 * DMA controller. The implementation of these things is in
 * <code>dma.lib</code>.
 */

#ifndef _DMA_H_
#define _DMA_H_

#include <cc2511_map.h>

#define DMA_CHANNEL_RADIO  1

/*! Initializes the DMA1CFGL and DMA1CFGH registers. */
void dmaInit();

typedef struct DMA14_CONFIG
{
    volatile DMA_CONFIG radio;   // DMA channel: for RF purposes
    volatile DMA_CONFIG __1;     // DMA channel (unassigned)
    volatile DMA_CONFIG __2;     // DMA channel (unassigned)
    volatile DMA_CONFIG __3;     // DMA channel (unassigned)
} DMA14_CONFIG;

/*! This structure in XDATA holds the configuration options
 for DMA channels 1-4.  We have to do it this way because the
 CC2511's DMA controller expects the configurations of those
 channels to be next to eachother in memory.  The configuration
 of channel 0 can be anywhere.  */
extern DMA14_CONFIG XDATA dmaConfig;


#endif
