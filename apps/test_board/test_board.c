/* test_board: This is a simple test application that test the basic
 * features of board.h and time.h.
 * 
 * Note that this app does NOT implement a USB interface, so it will
 * not be recognized by the Wixel Configuration Utility and you can
 * not get it in to bootloader mode using a USB command.  However, it
 * does call boardService(), so you can get it in to bootloader mode
 * by shorting P2_2 to 3V3 (3.3 V).
 * 
 */

#include <board.h>
#include <time.h>

void updateLeds()
{
    LED_GREEN(timeMs >> 9 & 1);     // Blink the Green LED.
    LED_YELLOW(usbPowerPresent());  // Indicate USB power using the yellow LED.
    LED_RED(vinPowerPresent());     // Indicate VIN with the red LED.
}

void main()
{
    systemInit();
    while(1)
    {
        boardService();
        updateLeds();
    }
}
