/*! \file board.h
 * This file provides basic functions for manipulating the
 * hardware on the Wixel, such as the LEDs, power detection pins,
 * and the pullup on the USB D+ line.  It also has functions
 * related to the Wixel's bootloader.  The implementations of these
 * functions are in <code>wixel.lib</code>.
 */

#ifndef _WIXEL_H
#define _WIXEL_H

#include <cc2511_map.h>
#include <cc2511_types.h>

/*! The 32-bit serial number of this device.  The Wixel's serial number is
 * stored in the bootloader's flash section. */
extern uint8 CODE serialNumber[4];

/*! The 32-bit serial number of this device represented in USB String
 * Descriptor format as specified in the USB Specification. */
extern uint16 CODE serialNumberStringDescriptor[];

/*! Turns the green LED on if the argument is non-zero, otherwise
 * turns it off.
 * Note that the green LED is powered from USB so it will not actually
 * emit light when the USB cable is disconnected. */
#define LED_GREEN(v)        {((v) ? (P2DIR |= 0x10) : (P2DIR &= ~0x10));}

/*! Turns the yellow LED on if the argument is non-zero, otherwise
 * turns it off. */
#define LED_YELLOW(v)       {((v) ? (P2DIR |= 0x04) : (P2DIR &= ~0x04));}

/*! Turns the red LED on if the argument is non-zero, otherwise
 * turns it off. */
#define LED_RED(v)          {((v) ? (P2DIR |= 0x02) : (P2DIR &= ~0x02));}

/*! \return 1 if the green LED is on, 0 otherwise.
 *
 * Note that the green LED is powered from USB so it will not actually emit
 * light when the USB cable is disconnected, but this macro may still return
 * 1 in that case.
 */
#define LED_GREEN_STATE     ((P2DIR >> 4) & 1)

/*! \return 1 if the yellow LED is on, 0 otherwise. */
#define LED_YELLOW_STATE    ((P2DIR >> 2) & 1)

/*! \return 1 if the red LED is on, 0 otherwise. */
#define LED_RED_STATE       ((P2DIR >> 1) & 1)

/*! Toggles the state of the green LED.
 * This is slightly more efficient than
 * <code>LED_GREEN(!LED_GREEN_STATE);</code> */
#define LED_GREEN_TOGGLE()  {P2DIR ^= 0x10;}

/*! Toggles the state of the yellow LED.
 * This is slightly more efficient than
 * <code>LED_YELLOW(!LED_YELLOW_STATE);</code> */
#define LED_YELLOW_TOGGLE() {P2DIR ^= 0x04;}

/*! Toggles the state of the red LED.
 * This is slightly more efficient than
 * <code>LED_RED(!LED_RED_STATE);</code> */
#define LED_RED_TOGGLE()    {P2DIR ^= 0x02;}

/*! Initializes the board's I/O lines, clock and other basic things.
 * You will typically want to call this function at the very beginning
 * of main() before you do anything else.
 * This is a simple function which just calls several other initialization
 * functions so you can look at the documentation of those other functions
 * to find out exactly what this one does.
 *
 * This function calls:
 *
 * -# boardIoInit()
 * -# boardClockInit()
 * -# timeInit()
 * -# dmaInit()
 */
void systemInit();

/*! Initializes the board's I/O lines.  Specifically, this function:
 * - Initializes the P2 register.
 * - Enables pull-down resistors for the red and yellow LED pins and
 *   disables pull-down resistors for the other port 2 pins.
 *
 * This function is called by systemInit().
 */
void boardIoInit();

/*! Initializes the board's clock and conifgures the CPU and the
 * timers to run as fast as possible.
 *
 * This function is called by systemInit().
 * */
void boardClockInit();

/*! Takes care of any board-related tasks that need to be performed
 * regularly.
 * Right now all this function does is call boardStartBootloaderIfNeeded()
 * so you should call this function regularly if you want to be able to
 * jump to the bootloader from the application by shorting P2_2 to 3V3. */
void boardService();

/*! Checks to see if the yellow LED line (P2_2) is connected to 3V3.
 * If they are connected, then it starts the bootloader by calling
 * boardStartBootloader.
 * This function is called by boardService.
 * If you call this function regularly, then it provides a relatively
 * easy way to get into bootloader mode if you can't do it with a
 * USB command.
 * Currently this function only works while the yellow LED is off. */
void boardStartBootloaderIfNeeded();

/*! Shuts down the application and starts the bootloader. */
void boardStartBootloader();

/*! \return 1 if USB power (VBUS) is detected, 0 otherwise.
 *
 * This function relies on getMs, so you must call timeInit() before
 * calling this.  If this function returns 1, it means that the USB
 * cable is plugged in. */
BIT usbPowerPresent();

/*! \return 1 if VIN power is detected, 0 otherwise. */
BIT vinPowerPresent();

/*! Enables the 1500 &Ohm; pull-up the USB D+ line.  This
 * signals to the USB host that a device has been attached.
 * This function is called by <code>usb.lib</code> (see usb.h);
 * if you are using that library you should not call this
 * function yourself. */
void enableUsbPullup();

/*! Disables the 1500 &Ohm; pull-up the USB D+ line.  This
 * signals to the USB host that this device has been detached.
 * This function is called by <code>usb.lib</code> (see usb.h);
 * if you are using that library you should not call this
 * function yourself. */
void disableUsbPullup();

#endif
