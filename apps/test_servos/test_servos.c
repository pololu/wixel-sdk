/** test_servos app:
 *
 * This app tests the servo library by using it to transmit servo pulses on
 * all six available pins: P0_2, P0_3, P0_4, P1_2, P1_1, and P1_0.
 *
 * This is mainly intended for people who are changing the servo library.  If
 * you just want to use the library to control a servo from a Wixel, see the
 * example_servo_sequence app.
 */

#include <cc2511_map.h>
#include <servo.h>
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>

// Here we define what pins we will be using for servos.  Our choice is:
// Servo 0 = P0_2
// Servo 1 = P0_3
// Servo 2 = P0_4
// Servo 3 = P1_2
// Servo 4 = P1_1
// Servo 5 = P1_0
uint8 CODE pins[] = {2, 3, 4, 12, 11, 10};

void myServosInit()
{
    // Start the servo library.
    servosStart((uint8 XDATA *)pins, sizeof(pins));

    // Set the speeds of servos 0-4.
    servoSetSpeed(0, 300);
    servoSetSpeed(1, 300);
    servoSetSpeed(2, 300);
    servoSetSpeed(3, 300);
    servoSetSpeed(4, 0);      // Not actually necessary because default speed is 0 (no speed limit).

    // Set servo 5 up to move very slowly from 1000 to 2000 us.
    // This will take about 459 seconds.
    servoSetSpeed(5, 0);      // Not actually necessary because default speed is 0 (no speed limit).
    servoSetTarget(5, 1000);
    servoSetSpeed(5, 1);
    servoSetTarget(5, 2000);
}

void updateServos()
{
    if (getMs() >> 11 & 1)
    {
        // For 2048 ms, the code in this block will be called.
        // Then for the next 2048 ms, the code in the "else" block will be called.
        // The pattern repeats every 4096 ms.

        servoSetTarget(0, 1000);  // Send servo 0 to position 1000 us.
        servoSetTarget(1, 1500);
        servoSetTarget(2, 1500);
        servoSetTarget(3, 0);
        servoSetTarget(4, 1900);

        LED_YELLOW(0);
    }
    else
    {
        servoSetTarget(0, 2000);
        servoSetTarget(1, 2000);
        servoSetTarget(2, 1000);
        servoSetTarget(3, 1700);
        servoSetTarget(4, 1100);

        LED_YELLOW(1);
    }
}

// Takes care of receiving commands from the USB virtual COM port.
void receiveCommands()
{
    if (usbComRxAvailable() == 0){ return; }
    switch(usbComRxReceiveByte())
    {
    case 's':  // Start/Stop
        if (servosStarted())
        {
            servosStop();
        }
        else
        {
            servosStart(0, 0);
        }
        break;
    }
}

void main()
{
    systemInit();
    usbInit();
    myServosInit();

    while(1)
    {
        boardService();
        usbComService();
        usbShowStatusWithGreenLed();
        updateServos();
        receiveCommands();

        // The red LED will be on if any of the servos are moving.
        LED_RED(servosMoving());
    }
}
