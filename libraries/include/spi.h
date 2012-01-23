/*! \file spi.h
 * This file defines constants used in spi0_master.h and spi1_master.h.
 */

#ifndef _SPI_H
#define _SPI_H

/*! The SCK line will be low when no data is being transferred. */
#define SPI_POLARITY_IDLE_LOW   0
/*! The SCK line will be high when no data is being transferred. */
#define SPI_POLARITY_IDLE_HIGH  1

/*! Data is sampled/transmitted on the leading edge, when the clock line transitions from idle to active. */
#define SPI_PHASE_EDGE_LEADING  0
/*! Data is sampled/transmitted on the trailing edge, when the clock like transitions from active to idle. */
#define SPI_PHASE_EDGE_TRAILING 1

/*! The most-significant bit is transmitted first. */
#define SPI_BIT_ORDER_MSB_FIRST 0

/*! The least-significant bit is transmitted first. */
#define SPI_BIT_ORDER_LSB_FIRST 1


#endif /* SPI_H_ */
