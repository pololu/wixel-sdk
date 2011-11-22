/** test_random app:

This application tests the random number generator.

It makes the yellow LED blink randomly.

You can also connect to it using the virtual COM port and send commands to it.
Each command is a single character:
'r' : Generate and random number and print it.
's' : Seed RNG from serial number (discards previous state of RNG).
'a' : Seed RNG from ADC (discards previous state of RNG).
'0' : Seed RNG with 0x0000.
'1' : Seed RNG with 0xFFFF.
'8' : Seed RNG with 0x8003.
'Y' : Start blinking the yellow LED randomly.  (Every other command disables
      the yellow LED's blinking so it doesn't interfere with the sequence of
      numbers.)
 */

#include <cc2511_map.h>
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <stdio.h>
#include <random.h>

uint32 nextToggle = 0;

BIT blinkYellow = 1;

void updateLeds()
{
    usbShowStatusWithGreenLed();

    // NOTE: The code below is bad because it is reading two bytes of timeMs,
    // and the interrupt that updates timeMs could fire between those two reads.
    if (blinkYellow)
    {
        uint32 time = getMs();
        if (time >= nextToggle)
        {
            LED_YELLOW_TOGGLE();
            nextToggle = time + randomNumber();
        }
    }
    else
    {
        LED_YELLOW(1);
    }

    LED_RED(0);
}

uint8 nibbleToAscii(uint8 nibble)
{
    nibble &= 0xF;
    if (nibble <= 0x9){ return '0' + nibble; }
    else{ return 'A' + (nibble - 0xA); }
}

void receiveCommands()
{
    if (usbComRxAvailable() && usbComTxAvailable() >= 64)
    {
        uint8 XDATA response[64];
        uint8 responseLength;
        uint8 byte, rand;
        byte = usbComRxReceiveByte();

        // By default, echo back the byte that was send.
        response[0] = byte;
        responseLength = 1;

        // By default, stop blinking the yellow LED because it will
        // affect the sequence of random numbers reported to the COM port.
        blinkYellow = 0;
        switch(byte)
        {
        case 'Y': blinkYellow = 1; break;
        case 's': randomSeedFromSerialNumber(); break;
        case 'a': randomSeedFromAdc(); break;
        case '0': randomSeed(0,0); break;
        case '1': randomSeed(0xFF, 0xFF); break;
        case '8': randomSeed(0x80, 0x03); break;
        case 'r':
            rand = randomNumber();

            // Wait for the NEXT random number to finish so we can read RNDH and RNDL.
            while(ADCCON1 & 0x0C);

            response[responseLength++] = ',';
            response[responseLength++] = nibbleToAscii(rand >> 4);
            response[responseLength++] = nibbleToAscii(rand);
            response[responseLength++] = ',';
            response[responseLength++] = (rand & 0x80) ? '1' : '0';
            response[responseLength++] = (rand & 0x40) ? '1' : '0';
            response[responseLength++] = (rand & 0x20) ? '1' : '0';
            response[responseLength++] = (rand & 0x10) ? '1' : '0';
            response[responseLength++] = (rand & 0x08) ? '1' : '0';
            response[responseLength++] = (rand & 0x04) ? '1' : '0';
            response[responseLength++] = (rand & 0x02) ? '1' : '0';
            response[responseLength++] = (rand & 0x01) ? '1' : '0';
            response[responseLength++] = ',';
            response[responseLength++] = nibbleToAscii(RNDH >> 4);
            response[responseLength++] = nibbleToAscii(RNDH);
            response[responseLength++] = nibbleToAscii(RNDL >> 4);
            response[responseLength++] = nibbleToAscii(RNDL);
            response[responseLength++] = '\r';
            response[responseLength++] = '\n';
            break;
        default: response[0] = '?'; break;
        }
        usbComTxSend(response, responseLength);
    }
}

void main()
{
    systemInit();

    usbInit();
    randomSeedFromSerialNumber();
    while(1)
    {
        boardService();
        updateLeds();
        usbComService();
        receiveCommands();
    }
}
