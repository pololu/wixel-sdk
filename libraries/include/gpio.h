/*! \file gpio.h
 *
 * The <code>gpio.lib</code> library provides functions for using the CC2511's pins
 * as general purpose inputs or outputs (GPIO).  Every pin on the CC2511 that has a
 * name starting with <i>P</i> can be configured as a <i>digital input</i> or <i>digital output</i>.
 *
 * The functions in this library allow for simpler programmatic approaches to working
 * with digital I/O since you no longer have to deal with a multitude of pin-specific
 * registers.
 *
 * \section ports Ports
 *
 * The pins on the CC2511 are divided into three ports: Port 0 (P0), Port 1 (P1),
 * and Port 2 (P2).  Every pin's name is prefixed by the name of the port it is on.
 * For example, P0_3 starts with "P0" so it is a pin on Port 0.
 *
 * On the Wixel, none of the pins on Port 0 and Port 1 are tied to any on-board
 * hardware so they completely free to be used as GPIO.
 *
 * When the Wixel starts up, all the pins on Port 0 and Port 1 will be inputs with
 * internal pull-up resistors enabled <i>except</i> P1_0 and P1_1, which do not have
 * internal pull-up or pull-down resistors
 *
 * This library supports Port 2, but all of the Wixel's Port 2 pins are handled by the
 * functions declared in board.h so you should not need to manipulate them with this
 * library.
 *
 * \section pinparam The pinNumber parameter
 *
 * Most of the functions in this library take a pin number as their first
 * argument.  These numbers are computed by multiplying the first digit in the
 * pin name by ten and adding it to the second digit, as shown in the table below.
 *
 * <table>
 * <caption>CC2511 Pins</caption>
 * <tr><th>Pin</th><th>pinNumber parameter</th></tr>
 * <tr><td>P0_0</td><td>0</td></tr>
 * <tr><td>P0_1</td><td>1</td></tr>
 * <tr><td>P0_2</td><td>2</td></tr>
 * <tr><td>P0_3</td><td>3</td></tr>
 * <tr><td>P0_4</td><td>4</td></tr>
 * <tr><td>P0_5</td><td>5</td></tr>
 * <tr><td>P1_0</td><td>10</td></tr>
 * <tr><td>P1_1</td><td>11</td></tr>
 * <tr><td>P1_2</td><td>12</td></tr>
 * <tr><td>P1_3</td><td>13</td></tr>
 * <tr><td>P1_4</td><td>14</td></tr>
 * <tr><td>P1_5</td><td>15</td></tr>
 * <tr><td>P1_6</td><td>16</td></tr>
 * <tr><td>P1_7</td><td>17</td></tr>
 * <tr><td>P2_0</td><td>20</td></tr>
 * <tr><td>P2_1</td><td>21</td></tr>
 * <tr><td>P2_2</td><td>22</td></tr>
 * <tr><td>P2_3</td><td>23</td></tr>
 * <tr><td>P2_4</td><td>24\footnote</td></tr>
 * </table>
 *
 * \section interrupts Interrupts
 *
 * All the functions in this library are declarded as reentrant, which means it is
 * safe to call them in your main loop and also in your interrupt service routines
 * (ISRs).
 * However, if you are using these functions in an ISR, you should make sure that
 * have no code in your main loop that does a non-atomic read-modify-write operation
 * on any of the I/O registers that are changed in the interrupt.
 * The risk is that the interrupt could fire after while the read-modify-write
 * operation is in progress, after the register has been read but before it has been
 * written.  Then when the register is written by the main loop, the change made by
 * the ISR will be unintentionally lost.
 *
 * For example, it would be bad if you called <code>setDigitalOutput(10, 1)</code>
 * in an interrupt and in your main loop you had some code like:
\code
P1DIR = (P1DIR & MASK) | VALUE;
\endcode
 *
 * It is OK to have code like <code>P1DIR |= VALUE;</code> in your main loop because
 * that compiles to a single instruction, so it should be atomic.
 *
 * \section overhead Overhead
 *
 * Calling the functions in this library will be slower than manipulating the I/O
 * registers yourself, but the overhead should be roughly the same for each pin.
 *
 * This library (git revision 4de9ee1f) was tested with
 * SDCC 3.0.0 (#6037) and it was found that an I/O line could be toggled once
 * every 3.2 microseconds by calling setDigitalOutput() several times in a row.
 *
 * \section caveats Caveats
 *
 * To use your digital I/O pins correctly, there are several things you should be aware of:
 * - <b>Maximum voltage ratings:</b> Be sure to not expose your input pins to voltages
 *   outside their allowed range.  The voltage should not go below 0 V (GND) and should
 *   not exceed VDD (typically 3.3 V).  This means that you can not connect an input
 *   on the CC2511 directly to an output from a 5V system if the output ever drives
 *   high (5 V).  You can use a voltage divider circuit, level-shifter, or diode to
 *   overcome this limitation.
 * - <b>Drawing too much current from an output pin:</b> Be sure you do not attempt
 *   to draw too much current from your output pin; it may break.
 *   The amount of current that can be supplied by the CC2511's I/O pins is not
 *   well-documented by the manufacturer.  According to
 *   <a href="http://e2e.ti.com/support/low_power_rf/f/155/p/31555/319919.aspx">this forum post by a TI Employee</a>,
 *   regular I/O pins are designed to be able to source 4&nbsp;mA while P1_0 and P1_1 are designed for 20 mA.
 *   You can use a transistor to overcome this limitation.
 * - <b>Shorts</b>: Be sure that you do not connect a high output pin directly to a
 *   low output pin or to another high output pin that is driving to a different voltage.
 * - <b>Peripheral functions</b>: Many of the pins on the CC2511 can be configured to
 *   be used by a peripheral by setting the right bit in the P0SEL, P1SEL, or P2SEL
 *   register.  When a pin is being used by a peripheral, the functions in this
 *   library may not work.  For example, if you have enabled USART1 in Alternate
 *   Location 1, then you can not control the output value of P1_6 using these
 *   functions because P1_6 serves as the serial transmit (TX) line.
 */

#ifndef _GPIO_H
#define _GPIO_H

#include <cc2511_types.h>

/*! Represents a low voltage, also known as GND or 0 V. */
#define LOW   0

/*! Represents a high voltage, also known as 3V3 (typically 3.3 V). */
#define HIGH  1

/*! Specifies that the input pin should be a high-impedance input with no pull-up
 * or pull-down resistor enabled.  See setDigitalInput(). */
#define HIGH_IMPEDANCE  0

/*! Specifies that the pin should have a 20 kilohm pull-up or pull-down enabled.
 * See setDigitalInput(). */
#define PULLED          1

/*! \brief Configures the specified pin as a digital output.
\param pinNumber Should be one of the pin numbers listed in the table above (e.g. 12).
\param value Should be one of the following:
- #LOW (0): Drives the line low (GND, 0 V).
- #HIGH (1): Drives the line high (3V3, typically 3.3 V).

This function will not work if the pin has previously been configured as a
peripheral function pin; the bit for this pin in P0SEL/P1SEL/P2SEL must be 0.

This function first sets the output value, then it sets the pin direction.
For example, calling <code>setDigitalOutput(3, HIGH)</code> will have the same effect as
(but be slower than) this:

\code
P0_3 = 1;
P0DIR |= (1<<3);
\endcode */
void setDigitalOutput(uint8 pinNumber, BIT value) __reentrant;

/*! \brief Configures the specified pin as an input.
\param pinNumber Should be one of the pin numbers listed in the table above (e.g. 12).
\param pulled Should be one of the following:
- #HIGH_IMPEDANCE (0): Disables the internal pull-up and pull-down resistors on that pin.
- #PULLED (1): Enables an internal 20 kilohm pull-up or pull-down resistor on the pin.
  The type of resistor used is set at the port level.  By default, Port 0 and Port 1
  use pull-up resistors, but you can change those ports to use pull-down resistors
  calling setPort0PullType() or setPort1PullType().  You can not have pull-up and
  pull-down resistors enabled simultaneously for different pins on the same port.

This function first sets the pull type, then it sets the pin direction.
For example, calling <code>setDigitalInput(15, PULLED)</code> will have the same effect as
(but be slower than) this:

\code
P1SEL &= ~(1<<5);
P1DIR &= ~(1<<5);
\endcode

Note: The pins P1_0 and P1_1 do NOT have internal pull-up or pull-down resistors,
so the second argument to this function does not have any effect when configuring
either of those pins.

*/
void setDigitalInput(uint8 pinNumber, BIT pulled) __reentrant;

/*! \brief Returns the current input or output value of the pin.
 *
 * \param pinNumber Should be one of the pin numbers listed in the table above (e.g. 12).
 * \return #LOW (0) or #HIGH (1).
 *
 * The return value represents a digital reading of the voltage on the pin.
 * According to the "DC Characteristics" section of the CC2511 datasheet,
 * voltages below 30% of VDD (typically 0.99 V on the Wixel) will read as 0,
 * while voltages above 70% of VDD (typically 2.31 V on the Wixel)
 * will read as 1.
 *
 * This function is intended to be used for pins that are configured as inputs.
 * For a pin configured as an output, it can be used, but it might sometimes
 * give unexpected results in case the current voltage has not
 * reached the voltage that the pin is configured to drive it to.
 *
 * This function simply returns the bit value of the port.  For example,
 * calling <code>isPinHigh(14)</code> will have the effect as reading
 * <code>P1_4</code> (but the function call will be slower).
 * */
BIT isPinHigh(uint8 pinNumber) __reentrant;

/*! Selects whether Port 0 will have internal pull-down or pull-up resistors.
 *
 * \param pullType Specifies the voltage that the resistors will pull to.
 *   Should be either #LOW (0) or #HIGH (1).
 *
 * The resistors can be disabled individually for each pin using setDigitalInput(),
 * but it is impossible to have pull-up and pull-down resistors enabled simultaneously
 * for different pins on the same port.
 */
void setPort0PullType(BIT pullType) __reentrant;

/*! Same as setPort0PullType() except this function affects Port 1. */
void setPort1PullType(BIT pullType) __reentrant;

/*! Same as setPort0PullType() except this function affects Port 2.
 * This function is included for the sake of completeness, but it should not
 * be used on a Wixel because all of the pins on Port 2 are managed by the
 * functions declared in board.h. */
void setPort2PullType(BIT pullType) __reentrant;

#endif
