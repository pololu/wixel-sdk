/*! \file radio_com.h
 * A library that uses radio_link.c to send and receive reliable streams of bytes
 * from another Wixel that is operating on the same radio frequency.
 * Also supports sending 8 control signals to the other Wixel and receiving 8
 * control signals from the other Wixel.
 */

#ifndef _RADIO_COM_H_
#define _RADIO_COM_H_

void radioComInit();

/*! This is a configuration option for the radio_com library that can be set
 * by higher-level code.  The default value is 0.
 *
 * When this bit is 1, the radio_com library guarantees that the higher-level code
 * receives notifications of changed control signals via radioComRxControlSignals()
 * and receives data bytes via radioComRxAvailable()/radioComRxReceiveByte() in the
 * <b>same order</b> that these two pieces of information were received from the radio.
 * If this bit is 1, higher-level code must call radioComRxControlSignals() regularly, or
 * else it will not be able to receive bytes using
 * radioComRxAvailable()/radioComRxReceiveByte().
 *
 * When this bit is 0, the higher-level code does not have to call radioComRxControlSignals()
 * regularly.
 *
 * We recommend that if you call radioComRxControlSignals(), you should call it regularly
 * and set radioComRxEnforceOrdering to 1 at the beginning of your program.  If you are not
 * using the control signals, you can leave this bit at 0. */
extern BIT radioComRxEnforceOrdering;

uint8 radioComRxAvailable(void);
uint8 radioComRxReceiveByte(void);

void radioComTxService(void);  // This must be called regularly if you are sending data.

uint8 radioComTxAvailable(void);
void radioComTxSendByte(uint8 byte);
void radioComTxSend(const uint8 XDATA * buffer, uint8 size);

void radioComTxControlSignals(uint8 controlSignals);
uint8 radioComRxControlSignals();

#endif /* RADIO_COM_H_ */
