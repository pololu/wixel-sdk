/*! \file dma.h
 * The <code>dma.lib</code> library provides basic functions and variables for
 * the CC2511's DMA controller.
 * DMA provides a fast way to copy blocks of data from one memory region or
 * peripheral to another.
 */

#ifndef _DMA_H_
#define _DMA_H_

#include <cc2511_map.h>

/*! Initializes the DMA1CFGL and DMA1CFGH registers to point
 * to ::dmaConfig.
 *
 * This function is called by systemInit(). */
void dmaInit(void);

/*! This is the number of the DMA channel we have chosen to use for
 * transmitting and receiving radio packets. */
#define DMA_CHANNEL_RADIO  1

/*! This struct consists of 4 DMA config registers
 * for DMA channels 1-4. */
typedef struct DMA14_CONFIG
{
    /*! This is the DMA configuration struct for DMA channel 1,
     * which we have chosen to use for transmitting and receiving
     * radio packets. */
    volatile DMA_CONFIG radio;

    /*! Config struct for DMA channel 2 (unassigned) */
    volatile DMA_CONFIG _2;

    /*! Config struct for DMA channel 3 (unassigned) */
    volatile DMA_CONFIG _3;

    /*! Config struct for DMA channel 4 (unassigned) */
    volatile DMA_CONFIG _4;
} DMA14_CONFIG;

/*! This structure in XDATA holds the configuration options
 for DMA channels 1-4.  We have to do it this way because the
 CC2511's DMA controller expects the configurations of those
 channels to be next to each other in memory.  The configuration
 of channel 0 can be anywhere.  You must call dmaInit()
 (or systemInit()) for this struct to work. */
extern DMA14_CONFIG XDATA dmaConfig;

#endif
