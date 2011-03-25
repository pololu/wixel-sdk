/*! \file radio_com.h
 * A library that uses radio_link.c to send and receive reliable streams of bytes
 * from another Wixel that is operating on the same radio frequency.
 */

#ifndef _RADIO_COM_H_
#define _RADIO_COM_H_

void radioComInit();

uint8 radioComRxAvailable(void);
uint8 radioComRxReceivePacket(uint8 XDATA * buffer, uint8 size);

uint8 radioComTxAvailable(void);
uint8 radioComTxSendPacket(const uint8 XDATA * buffer, uint8 size);

#endif /* RADIO_COM_H_ */
