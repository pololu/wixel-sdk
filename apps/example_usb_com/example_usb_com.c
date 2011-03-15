#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <stdio.h>

#define COMMAND_TOGGLE_YELLOW_LED   0x81

BIT yellowLedOn = 0;

BIT serialProtocolError = 0;

uint8 commandByte;
uint8 dataBytes[32];
uint8 dataBytesLeft = 0;
uint8 dataBytesReceived;

uint16 x = 0x3FFF;

void updateLeds()
{
    static uint32 lastToggle = 0;

    usbShowStatusWithGreenLed();

    LED_YELLOW(yellowLedOn);

    LED_RED(0);
}

void executeCommand()
{
	uint8 response[2];
	
	switch(commandByte)
	{
	case COMMAND_TOGGLE_YELLOW_LED:
		yellowLedOn ^= 1;
		break;
		
	case COMMAND_GET_X:
		response[0] = x & 0xFF;
		response[1] = x >> 8 & 0xFF;
		break;
		
	case COMMAND_SET_X:
		x = dataBytes[0] + (dataBytes[1] << 7);
		break;
	}
}

/* Processes a new byte received from the USB virtual COM port.
 * Populates the commandByte
 * 
 * NOTE: This function is complicated because it handles receiving multi-byte commands.
 * If all of your commands are just 1 byte, then you should use something much
 * simpler like this:
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
        
        commandByteReceived = byteReceived;
        dataBytesReceived = 0;
        dataBytesLeft = 0;
        
        // Look at the command byte to figure out if it is valid, and
        // determine how many data bytes to expect.
        switch(commandByteReceived)
        {
            case COMMAND_TOGGLE_YELLOW_LED:
                dataBytesLeft = 0;
                break;

            default:
                // Received an invalid command byte.
                serialProtocolError = 1;
                return;
            }
        }
    
    	if (dataBytesLeft==0)
    	{
    		// We have received a complete multi-byte command.
        	executeCommand();
    	}
    }
    else
    {
        // We received a data byte.

        if (dataBytesLeft == 0)
        {
            // We weren't expecting a data byte.
            serialProtocolError = 1;
            return;
        }
        
        dataBytes[dataBytesReceived] = byteReceived;
        dataBytesLeft--;
        dataBytesReceived++;
        
        if (dataBytesLeft==0)
        {
        	// We have received a complete multi-byte command.
            executeCommand();
        }
    }
}

/* Checks for new bytes available on the USB virtual COM port
 * and processes all that are available. */
void processBytesFromUsb()
{
    uint8 bytesLeft = usbComRxAvailable();
    while(bytesLeft && usbComTxAvailable() >= 64)
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
