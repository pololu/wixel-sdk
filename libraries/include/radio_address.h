/*! \file radio_address.h
 *
 * This header file provides a function for configuring the
 * radio address (radioAddressInit()) and also some small
 * functions like edit broadcast configurations.
 */

#ifndef _RADIO_ADDRESS_H
#define _RADIO_ADDRESS_H

#include <cc2511_types.h>

#define NoAddressCheck 0x0
#define NoBroadcastCheck 0x1
#define OneBroadcastCheck 0x2
#define DoubleBroadcastCheck 0x3

/*! Defines the address this wixel responds to. Valid values are
 * from 0 to 255.
 * if the Address Check is set to NoAddressCheck this parameter is useless
 * if the Address Check is set to NoBroadcastCheck all the available values are allowed
 * if the Address Check is set to OneBroadcastCheck only the range from 1 to 255 is available
 * if the Address Check is set to DoubleBroadcastCheck only the range from 1 to 254 is available
 */
extern int32 CODE param_address;

/*! Configures the CC2511's radio module using the address provided as parameter
 *
 * This function configures the ADDR register
 */
void radioAddressInit();

/*! \return a the corrent configuration of the Hardware Address Check
 *
 * Valid values are
 * NoAddressCheck 0x0
 * NoBroadcastCheck 0x1
 * OneBroadcastCheck 0x2
 * DoubleBroadcastCheck 0x3
 */
uint8 addressCheckHWConfiguration();

/*! Allow to change the configuration for hardware address check
 *
 * Valid values are
 * NoAddressCheck 0x0
 * NoBroadcastCheck 0x1
 * OneBroadcastCheck 0x2
 * DoubleBroadcastCheck 0x3
 */
void setAddressCheckHWConfiguration(uint8 configuration);

#endif /* _RADIO_ADDRESS_H */
