/** example_servo_sequence app:

Demonstrates how to write blocking code on the Wixel by showing
how to generate a timed sequence of movements.

Pin P0_2 is configured as a the servo pulse output.

Most of the functions in the Wixel SDK is non-blocking, which mean they return
within a few tens of microseconds or less.  For example, a non-blocking
function that blinks an LED on and off would looks like this:

    void updateRedLed()
    {
        LED_RED( (getMs() >> 9) & 1 );
    }

The equivalent blocking code would look like this:

    void blinkRedLed()
    {
        while(1)
        {
            LED_RED(0);
            waitMs(512);
            LED_RED(1);
            waitMs(512);
        }
    }

Blocking code can be easier to write and easier to understand, especially if it
is doing something complex.
*/

#include <cc2511_map.h>
#include <servo.h>
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>

#include "blocking.h"

// Prototypes for the example loop functions defined below.
void loop(void);
void loop_sequence_triggered_by_usb(void);
void loop_sequence_triggered_by_digital_input(void);

// Here we define what pins we will be using for servos.  Our choice is
// to just use one pin, P0_2, and designate it as servo 0.
// The servo library supports up to 6 servos.
uint8 CODE pins[] = {2};

// This function gets called frequently and takes care of any tasks that need
// to get done regularly, such as maintaining the Wixel's USB connection.
// All blocking functions call frequentTasks while they are blocking.
// There should be no blocking functions called from fequentTasks.
void frequentTasks()
{
    boardService();
    usbComService();
    usbShowStatusWithGreenLed();
}

/** This function is called when the Wixel starts up. */
void main()
{
    systemInit();
    usbInit();
    servosStart((uint8 XDATA *)pins, sizeof(pins));

    while(1)
    {
        frequentTasks();
        loop();
    }
}

/** Performs a simple sequence of servo movements.
    This is called in a loop, so the sequence will repeat indefinitely.

    Some special blocking functions used here are defined in blocking.c.
    The other functions are from the Wixel libraries and they are documented here:
    http://pololu.github.com/wixel-sdk/
 */
void loop()
{
    servoSetSpeed(0, 130);     // Set the speed of servo 0.
    servoSetTarget(0, 1000);   // Send servo 0 to position 1000 us.

    servosWaitWhileMoving();   // Wait for it to get there.

    LED_YELLOW(1);
    waitMs(2000);              // Wait 2 more seconds, with the yellow LED on.

    LED_YELLOW(0);
    servoSetTarget(0, 2000);
    servosWaitWhileMoving();

    servoSetTarget(0, 1500);
    servosWaitWhileMoving();

    servoSetTarget(0, 2000);
    servosWaitWhileMoving();
}

/** Other example loop functions **********************************************/
/** These other example loop functions below are provided to demonstrate more
 * things that the Wixel is capable of.
 * To use one of them, find the line above that says "loop();" and replace
 * "loop" with the name of the loop function below that you want to call.
 */

/** Waits for a character to be sent from USB and then triggers a sequence of
 *  servo movements depending on what character was received. */
void loop_sequence_triggered_by_usb()
{
    servoSetSpeed(0, 458);
    servoSetTarget(0, 1500);

    switch(usbComRxReceiveByteBlocking())
    {
    case 'a':
        // An 'a' character was received on the USB virtual COM port.  Perform sequence a.
        servoSetTarget(0, 2000);
        servosWaitWhileMoving();
        waitMs(1000);
        break;

    case 'b':
        // A 'b' character was received on the USB virtual COM port.  Perform sequence b.
        servoSetTarget(0, 1000);
        servosWaitWhileMoving();
        waitMs(1000);
        break;

    default:
        // An invalid character was received, so blink the red LED.
        LED_RED(1);
        waitMs(20);
        LED_RED(0);
        break;
    }
}

/** Waits for the voltage on pin P0_0 to go low, and then triggers a sequence of
 *  servo movements.
 *  By default P0_0 is configured as an input with a pull-up resistor.
 *  If you connect a normally-open button between P0_0 and one of the GND pins,
 *  then you can trigger the sequence of servo movements by pressing the button.
 *  */
void loop_sequence_triggered_by_digital_input()
{
    servoSetSpeed(0, 458);
    servoSetTarget(0, 1500);

    if (!isPinHigh(0)) // Measure voltage on P0_0
    {
        LED_YELLOW(1);
        servoSetTarget(0, 2000);
        servosWaitWhileMoving();

        waitMs(1000);

        servoSetTarget(0, 1500);
        servosWaitWhileMoving();

        LED_YELLOW(0);
    }
}
