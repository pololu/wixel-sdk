/** example_usb_com app:

This example app shows how to process and respond to commands received from
a USB virtual COM port.  The command protocol is documented below.


== Serial Command Protocol ==

Command Name: Toggle Yellow LED
Protocol:
  Computer sends 0x81 OR the ASCII encoding of character 'y' (0x79).

Command Name: Get X
Protocol:
  Computer sends 0x82
  Wixel sends back a 2-byte response that contains the current value X as a
  2-byte little-endian integer.

Command Name: Set X
Protocol:
  Computer sends 0x83, low7Bit, upper7Bits

Command Name: Clear Error
Protocol:
  Computer sends 0x84.

Command Name: Get Time
Protocol:
  Computer sends the ASCII encoding of the character 't' (0x74).
  Wixel sends back an ASCII string that includes the number of
  milliseconds that the Wixel has been running for.
*/

#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <stdio.h>

#define COMMAND_TOGGLE_YELLOW_LED   0x81
#define COMMAND_GET_X               0x82
#define COMMAND_SET_X               0x83
#define COMMAND_CLEAR_ERROR         0x84

/* VARIABLES ******************************************************************/

/** A variable that the user can get or set using serial commands. */
uint16 x = 0x3FFF;

/** True if the yellow LED should currently be on. */
BIT yellowLedOn = 0;

/** True if a serial protocol error occurred recently. */
BIT serialProtocolError = 0;

/** The binary command byte received from the computer. */
uint8 commandByte;

/** The binary data bytes received from the computer. */
uint8 dataBytes[32];

/** The number of data bytes we are waiting for to complete the current command.
 * If this is zero, it means we are not currently in the middle of processing
 * a binary command. */
uint8 dataBytesLeft = 0;

/** The number of data bytes received so far for the current binary command.
 * If dataBytesLeft==0, this is undefined. */
uint8 dataBytesReceived;

/** A temporary buffer used for composing responses to the computer before
 * they are sent.  Must be bigger than the longest possible response to any
 * command.
 */
uint8 XDATA response[32];

/* FUNCTIONS ******************************************************************/

void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(yellowLedOn);
    LED_RED(serialProtocolError);
}

void executeCommand()
{
    switch(commandByte)
    {
    case COMMAND_TOGGLE_YELLOW_LED:
        yellowLedOn ^= 1;
        break;
        
    case COMMAND_GET_X:
        response[0] = x & 0xFF;
        response[1] = x >> 8 & 0xFF;
        usbComTxSend(response, 2);   // Assumption: usbComTxAvailable() returned >= 2 recently.
        break;
        
    case COMMAND_SET_X:
        x = dataBytes[0] + (dataBytes[1] << 7);
        break;

    case COMMAND_CLEAR_ERROR:
        serialProtocolError = 0;
        break;
    }
}

/** Processes a new byte received from the USB virtual COM port.
 * 
 * NOTE: This function is complicated because it handles receiving multi-byte commands.
 * If all of your commands are just 1 byte, then you should remove executeCommand and
 * replace processByte with something much simpler like this:
 *     switch(byteReceived)
 *     {
 *         case COMMAND_A: actionA(); break;
 *         case COMMAND_B: actionB(); break;
 *         case COMMAND_C: actionC(); break;
 *         default: serialProtocolError = 1; break;
 *     }
 * */
void processByte(uint8 byteReceived)
{
    if (byteReceived & 0x80)
    {
        // We received a command byte.
        
        if (dataBytesLeft > 0)
        {
            serialProtocolError = 1;
        }
        
        commandByte = byteReceived;
        dataBytesReceived = 0;
        dataBytesLeft = 0;
        
        // Look at the command byte to figure out if it is valid, and
        // determine how many data bytes to expect.
        switch(commandByte)
        {
            case COMMAND_GET_X:
            case COMMAND_TOGGLE_YELLOW_LED:
            case COMMAND_CLEAR_ERROR:
                dataBytesLeft = 0;
                break;

            case COMMAND_SET_X:
                dataBytesLeft = 2;
                break;

            default:
                // Received an invalid command byte.
                serialProtocolError = 1;
                return;
        }
    
        if (dataBytesLeft==0)
        {
            // We have received a single-byte command.
            executeCommand();
        }
    }
    else if (dataBytesLeft > 0)
    {
        // We received a data byte for a binary command.

        dataBytes[dataBytesReceived] = byteReceived;
        dataBytesLeft--;
        dataBytesReceived++;
        
        if (dataBytesLeft==0)
        {
            // We have received the last byte of a multi-byte command.
            executeCommand();
        }
    }
    else
    {
        // We received a byte that is less than 128 and it is not part of
        // a binary command.  Maybe it is an ASCII command

        uint8 responseLength;
        uint32 time;
        switch(byteReceived)
        {
        case 't':
            time = getMs();
            // SDCC's default sprintf doesn't seem to support 32-bit ints, so we will
            // split getMs into two parts and print it in hex.
            responseLength = sprintf(response, "time=0x%04x%04x\r\n", (uint16)(time >> 16), (uint16)time);
            usbComTxSend(response, responseLength);
            break;

        case 'y':
            yellowLedOn ^= 1;
            break;
        }
    }
}

/** Checks for new bytes available on the USB virtual COM port
 * and processes all that are available. */
void processBytesFromUsb()
{
    uint8 bytesLeft = usbComRxAvailable();
    while(bytesLeft && usbComTxAvailable() >= sizeof(response))
    {
        processByte(usbComRxReceiveByte());
        bytesLeft--;
    }
}

void main()
{
    systemInit();
    usbInit();

    while(1)
    {
        boardService();
        updateLeds();
        usbComService();
        processBytesFromUsb();
    }
}
