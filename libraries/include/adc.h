/*! \file adc.h
 * The <code>adc.lib</code> library provides functions for using the CC2511's
 * Analog-to-Digital Converter (ADC).  The ADC can measure several
 * different things, including voltages on any of the six Port 0 pins, the
 * voltage on the VDD line (the 3V3 pin on the Wixel), and the temperature.
 *
 * The ADC has 14 channels:
 *
 * <table><caption>CC2511 ADC Channels</caption>
 * <tr><th>Channel Number</th><th>Name</th><th>Appropriate Function</tr>
 * <tr><td>0</td><td>AIN0 (P0_0)</td><td>adcRead()</td></tr>
 * <tr><td>1</td><td>AIN1 (P0_1)</td><td>adcRead()</td></tr>
 * <tr><td>2</td><td>AIN2 (P0_2)</td><td>adcRead()</td></tr>
 * <tr><td>3</td><td>AIN3 (P0_3)</td><td>adcRead()</td></tr>
 * <tr><td>4</td><td>AIN4 (P0_4)</td><td>adcRead()</td></tr>
 * <tr><td>5</td><td>AIN5 (P0_5)</td><td>adcRead()</td></tr>
 * <tr><td>8</td><td>AIN0 - AIN1</td><td>adcReadDifferential()</td></tr>
 * <tr><td>9</td><td>AIN2 - AIN3</td><td>adcReadDifferential()</td></tr>
 * <tr><td>10</td><td>AIN4 - AIN5</td><td>adcReadDifferential()</td></tr>
 * <tr><td>11</td><td>AIN6 - AIN7</td><td>adcReadDifferential()</td></tr>
 * <tr><td>12</td><td>GND</td><td></td></tr>
 * <tr><td>13</td><td>Internal 1.25 V Reference</td><td>adcRead()</td></tr>
 * <tr><td>14</td><td>Temperature Sensor</td><td>adcRead()</td></tr>
 * <tr><td>15</td><td>VDD/3</td><td></td></tr>
 * </table>
 *
 * \section channelparam The channel parameter
 *
 * Most functions in this library require a channel parameter to
 * specify which channel to use.  The value of this parameter should
 * be one of the channel numbers in the table above.
 *
 * You can also use the bitwise OR operator (|) to specify advanced options
 * in the channel parameter:
 * - By default, VDD is used as a reference but you can use the internal
 *   1.25 V source as a reference by specifying #ADC_REFERENCE_INTERNAL
 *   in the channel parameter.
 * - By default, the maximum ADC resolution (12 bits) is used, but you can
 *   use a different resolution by specifying #ADC_BITS_10, #ADC_BITS_9, or
 *   #ADC_BITS_7 in the channel parameter.
 *
 */

#ifndef _ADC_H
#define _ADC_H

/*! Specifies that the internal 1.25 voltage reference should be used.
 * This means that a value of 2047 corresponds to 1.25 V instead of
 * 3.3 V. */
#define ADC_REFERENCE_INTERNAL  0b10000000

/*! Specifies that the VDD line should be used as a voltage reference.
 * This means that a value of 2047 corresponds to VDD (usually 3.3 V).
 * This is the default setting. */
#define ADC_REFERENCE_VDD       0

/*! Specifies that the decimation rate should be 64, which gives
 * 7 bits of resolution.  With this setting, each conversion takes 20
 * microseconds.
 */
#define ADC_BITS_7              0b00110000

/*! Specifies that the decimation rate should be 128, which gives
 * 9 bits of resolution.  With this setting, each conversion takes 36
 * microseconds.
 */
#define ADC_BITS_9              0b00100000

/*! Specifies that the decimation rate should be 256, which gives
 * 10 bits of resolution.  With this setting, each conversion takes 68
 * microseconds.
 */
#define ADC_BITS_10             0b00010000

/*! Specifies that the decimation rate should be 512, which gives
 * 12 bits of resolution.  With this setting, each conversion takes 132
 * microseconds.  This is the default setting.
 */
#define ADC_BITS_12             0

/*! Reads the voltage on the specified channel.
 *
 * \param channel The number of the channel to measure (0-6 or 13-15).
 *   This parameter can also contain advanced options (see above).
 *
 * \return A number between 0 and 2047, where 0 represents a voltage
 *   of 0 V and 2047 represents a voltage equal to the selected
 *   voltage reference (usually 3.3 V).
 *
 * Example:
 * \code
uint16 result1, result2;
result1 = adcRead(3);  // Measures voltage on P0_3.
result2 = adcRead(4 | ADC_REFERENCE_INTERNAL | ADC_BITS_7);
 * \endcode
 *
 * This function returns an unsigned number so it is not appropriate for
 * differential channels.  See adcReadDifferential().
 *
 */
uint16 adcRead(uint8 channel);

/*! Reads the voltage difference on the specified differential channel.
 *
 * \param channel The number of the differential channel to measure (8-11).
 *    This parameter can also contain advanced options (see above).
 *
 * \return A number between -2048 and 2047.  A value of 2047 means that
 *    the voltage difference was equal to the selected voltage reference.
 *    A value of -2048 means that the voltage difference was equal to
 *    the negation of the selected voltage reference.
 *    A value of 0 means that the voltage difference was zero.
 *
 * Example:
 * \code
int16 diff1, diff2;
diff1 = adcReadDifferential(8);  // Measures (voltage of P0_0) - (voltage of P0_1).
diff2 = adcReadDifferential(9 | ADC_REFERENCE_INTERNAL | ADC_BITS_7);
 * \endcode
 */
int16 adcReadDifferential(uint8 channel);

/*! Reads the voltage of the VDD (3V3) line using the internal voltage
 * reference and returns the voltage of VDD in units of millivolts (mV). */
uint16 adcReadVddMillivolts();

/*! Sets the calibration parameter that is used by adcConvertToMillivolts().
 * \param vddMillivolts The voltage of the VDD line in units of millivolts.
 *
 * If your VDD is going to drop below 3.3 V, and you want to measure a
 * voltage in units of millivolts (as opposed to the raw ADC units)
 * you should run the following code regularly:
 *
\code
adcSetMillivoltCalibration(adcReadVddMillivolts());
\endcode
 */
void adcSetMillivoltCalibration(uint16 vddMillivolts);

/*! Converts an ADC result to millivolts.
 *
 * \param adcResult An ADC result between -2048 and 2047 that was
 *  measured using VDD as a reference.  You can obtain such a
 *  measurement by calling adcRead() or adcReadDifferential().
 *
 * \return The voltage in units of millivolts.
 *
 * By default, this function assumes that your VDD is at 3300 mV.
 * If you expect your VDD to go above or below that, or you want
 * more accurate results, you should use adcSetMillivoltCalibration().
 *
 * This function only applies to AD conversions where VDD was used as
 * a reference.  If you used the internal 1.25 V reference instead, you
 * can convert your result to millivolts by multiplying it by
 * 1250 and then dividing it by 2047. */
int16 adcConvertToMillivolts(int16 adcResult);

#endif
