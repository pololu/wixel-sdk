/*! \file radio_com.h
 * A library that uses radio_link.c to send and receive reliable streams of bytes
 * from another Wixel that is operating on the same radio frequency.
 */

#ifndef _RADIO_COM_H_
#define _RADIO_COM_H_

void radioComInit();

uint8 radioComRxAvailable(void);
uint8 radioComRxReceiveByte(void);

void radioComTxService(void);  // This must be called regularly if you are sending data.

uint8 radioComTxAvailable(void);
void radioComTxSendByte(uint8 byte);
void radioComTxSend(const uint8 XDATA * buffer, uint8 size);

void radioComTxControlSignals(uint8 controlSignals);
uint8 radioComRxControlSignals();

#endif /* RADIO_COM_H_ */
