/*! \file servo.h
 * The <code>servo.lib</code> library provides the ability to control up to 6
 * servos by generating digital pulses directly from your Wixel without the
 * need for a separate servo controller.
 *
 * This library uses the CC2511's Timer 1, so it will conflict with any other
 * library that uses Timer 1.
 *
 * With the exception of servosStop(), the functions in this library are
 * non-blocking.  Pulses are generated in the background by Timer 1 and its
 * interrupt service routine (ISR), which fires once every 2.73 milliseconds.
 *
 * This library uses hardware PWM from Timer 1 to generate the servo pulses,
 * so it can only generate servo pulses on the following pins:
 *
 * - P0_2
 * - P0_3
 * - P0_4
 * - P1_0
 * - P1_1
 * - P1_2
 *
 */

/*
 * \section servos Servos
 *
 *  http://www.pololu.com/blog/11/introduction-to-an-introduction-to-servos
 */

#ifndef _SERVO_H
#define _SERVO_H

#include <cc2511_map.h>
#include <cc2511_types.h>

#define SERVO_MAX_TARGET_MICROSECONDS  2500
#define SERVO_TICKS_PER_MICROSECOND    24


/*! Sets up the servo pins and the timer to be ready to send servo pulses.
 * This function should be called before any other functions in the library
 * are called.
 *
 * \param pins  A pointer to an array of pin numbers that specify which pins
 *   will be used to generate servo pulses.
 *   The pin numbers used in this array are the same as the pin numbers used
 *   in GPIO library (see gpio.h).  There should be no repetitions on this
 *   array, and each entry must be one of:
 *   - 2 (for P0_2)
 *   - 3 (for P0_3)
 *   - 4 (for P0_4)
 *   - 10 (for P1_0)
 *   - 11 (for P1_1)
 *   - 12 (for P1_2)
 *
 * \param numPins The size of the pin number array.
 *
 * If the <b>pins</b> parameter is 0 (a null pointer), then this function skips
 * the reinitialization of the pins and the internal data structures of the
 * library.
 * This means that the servo pin assignments, positions and speeds from before
 * will remain intact.
 *
 * Otherwise (if the <b>pins</b> parameter is non-zero),  the specified pins
 * will be configured as digital outputs, their targets will be initialized
 * to 0 (no pulses), and their speed limits will be initialized to 0
 * (no speed limit).
 *
 * By calling this function, you are defining the correspondence of the servo
 * numbers to pins.
 * The <b>servoNum</b> parameter in the other library functions can be thought
 * of as an index in the pins array.
 *
 * Example code:
 *
 * \code
uint8 CODE pins[] = {10, 12};  // Use P1_0 and P1_2 for servos.
servosStart((uint8 XDATA *)pins, sizeof(pins));
servoSetTarget(0, 1500);       // Affects pin P1_0
servoSetTarget(1, 1500);       // Affects pin P1_2
 * \endcode
 */
void servosStart(uint8 XDATA * pins, uint8 numPins);

/*! Stops sending servo pulses and turns off Timer 1.
 * After this function runs, the pins that were used for servo pulses will
 * all be configured as general-purpose digital outputs driving low.
 *
 * You can later restart the servo pulses by calling servoStop(). */
void servosStop(void);

/*! \returns 1 if the library is currently active and using Timer 1,
 * returns 0 if the library is stopped.
 *
 * Calling servosStart() changes this value to 1.
 * Calling servosStop() changes this value to 0.
 *
 * Timer 1 can be used for other purposes while the servo library is stopped.
 */
BIT servosStarted(void);

/*! This function sets a servo's target position in units of microseconds.
 *
 * \param servoNum  A servo number between 0 and 5.
 *   This number should be less than the associated <b>numPins</b> parameter
 *   used in the last call to servosStart().
 *
 * \param targetMicroseconds  The target position of the servo in units of
 *   microseconds.
 *   A typical servo responds to pulse widths between 1000 and 2000 us, so
 *   appropriate values for this parameter would be between 1000 and 2000.
 *   The allowed values for this parameter are 0-2500.
 *   A value of 0 means to stop sending pulses, and takes effect
 *   immediately regardless of speed limit for the channel.
 *
 * If the current target is 0, or the speed is 0 or targetMicroseconds parameter
 * is 0, then this function will have an immediate effect on the variable that
 * represents the position of the servo (see servoGetPosition()).
 * This allows you to do sequences of commands like:
 *
 * \code
servoSetSpeed(0, 0);
servoSetTarget(0, 1000);  // Immediately sets position to 1000.
servoSetSpeed(0, 400);
servoSetTarget(2000);     // Makes position to smoothly change from 1000 to 2000.
 * \endcode
 *
 * or
 *
 * \code
servoSetSpeed(0, 400);
servoSetTarget(0, 0);     // Immediately sets position to 0, despite the speed limit.
servoSetTarget(0, 1000);  // Immediately sets position to 1000, despite the speed limit.
servoSetTarget(0, 2000);  // Makes position smoothly change from 1000 to 2000.
 * \endcode
 *
 * These two sequences of commands each have the same effect, which is to immediately move
 * servo number 0 to position 1000 and then slowly move from there to position 2000.
 *
 * It is ok to call this function while the library is stopped.
 *
 * If you need more than 1-microsecond resolution, see servoSetTargetHighRes().
 */
void servoSetTarget(uint8 servoNum, uint16 targetMicroseconds);

uint16 servoGetTarget(uint8 servoNum);
void servoSetSpeed(uint8 servoNum, uint16 speed);
uint16 servoGetSpeed(uint8 servoNum);

uint16 servoGetPosition(uint8 servoNum);

/*! This is the high resolution version of servoSetTarget.
 * The units of \param target are 24ths of a microsecond, so a value of 24000
 * corresponds to 1000 microseconds. */
void servoSetTargetHighRes(uint8 servoNum, uint16 target);

uint16 servoGetTargetHighRes(uint8 servoNum);

uint16 servoGetPositionHighRes(uint8 servoNum);

ISR(T1, 0);

#endif
