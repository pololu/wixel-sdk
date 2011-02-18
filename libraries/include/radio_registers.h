#ifndef _RADIO_REGISTERS_H
#define _RADIO_REGISTERS_H

#include <cc2511_types.h>

void radioRegistersInit();

uint8 radioLqi();
int8 radioRssi();
BIT radioCrcPassed();

#define RSSI_OFFSET 71  // From Table 68: RSSI offset for 250kbps is 71.

#endif /* RADIO_REGISTERS_H_ */
