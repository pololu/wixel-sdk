/*! \file radio_registers.h
 *
 * This header file provides a function for configuring the
 * radio registers (radioRegistersInit()) and also some small
 * functions for reading information from the radio.
 */

#ifndef _RADIO_REGISTERS_H
#define _RADIO_REGISTERS_H

#include <cc2511_types.h>

/*! Configures the CC2511's radio module using settings that have
 * been tested by Pololu and are known to work.
 *
 * In summary, these settings are:
 * - Data rate = 350 kbps
 * - Modulation = MSK
 * - Channel 0 frequency = 2403.47 MHz
 * - Channel spacing = 286.4 kHz
 * - Channel bandwidth = 600 kHz
 *
 * This function does not configure the PKTLEN, MCSM0, MCSM1, MCSM2, CHANNR,
 * or ADDR registers or the DMA:  That should be done by higher-level code.
 */
void radioRegistersInit();

/*! \return The Link Quality Indicator (LQI) of the last packet received.
 *
 * According to the CC2511F32 datasheet, the LQI is a metric of the quality of
 * the received signal.
 * "LQI is best used as a relative measurement of link quality (a high value indicates
 * a better link than what a low value does), since the value is dependent on the
 * modulation format." */
uint8 radioLqi();

/*! \return  The signal strength of the last packet received, if the
 * radio just exited RX mode.  If the radio is in RX mode, returns a
 * continuously-updated estimate of the signal level in the channel.
 * The units are dBm.
 *
 * The RSSI typically be between -100 and -10.
 * A higher number (closer to zero if negative) is better.
 * RSSI stands for Received Signal Strength Indication. */
int8 radioRssi();

/*! \return 1 if the last packet received has a correct CRC-16,
 * 0 otherwise.
 *
 * If this function returns 0, the data in the last packet received
 * was corrupted and should not be relied upon. */
BIT radioCrcPassed();

/*! An offset used by radioRssi() to calculate the RSSI.
 * According to Table 68 of the CC2511F32 datasheet, RSSI
 * offset for 250kbps is 71. */
#define RSSI_OFFSET 71

#endif /* RADIO_REGISTERS_H_ */
