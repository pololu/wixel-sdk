/* serial_ascii_binary app:
 * This app enables your Wixel to convert between hex ASCII characters
 * and raw binary byte data using the interfaces UART0 and UART1.
 *
 * The flow of bidirectional information can be described as:
 * binary byte data -> UART1 -> converted into ASCII hex digits -> UART0 -> ASCII hex digits
 * ASCII hex digits -> UART0 -> converted into binary byte data -> UART1 -> binary byte data
 *
 * When sending ASCII hex data to UART0, to signal the start of a data stream, send
 * the delimiting character 'H' first, and then proceed to send an even number
 * of hex digits.
 *
 * Example ASCII input on UART0:   'H','1','F','A','3'
 * Example binary output on UART1: 0x1F,0xA3
 *
 * Example binary input on UART1:  0xA2,0x35
 * Example ASCII output on UART0: 'H','A','2','H','3','5'
 *
 * For UART0, the ASCII interface, the following pins are used:
 *
 * P0_3: TX
 * P0_2: RX
 *
 * For UART1, the binary interface, the following pins are used:
 * P1_6: TX
 * P1_7: RX
 *
 */

#include <wixel.h>
#include <stdio.h>
#include <usb.h>
#include <usb_com.h>
#include <uart1.h>
#include <uart0.h>

#define IS_HEX_DIGIT(byte)(((byte) >= 'a' && (byte) <= 'f') || ((byte) >= '0' && (byte) <= '9'))


/** Parameters ****************************************************************/

// Set UART0 baud rate.
int32 CODE param_ascii_baud_rate = 9600;

// Set UART1 baud rate. 
int32 CODE param_binary_baud_rate = 9600;

/** Functions *****************************************************************/

uint8 hexDigitToNibble(uint8 digit)
{
  //Convert ASCII hex digit to lower case.
  digit |= 0x20;
  
  if(digit >= 'a' && digit <= 'f')
  {
    return digit - 'a' + 0x0A;
  }
  else if(digit >= '0' && digit <= '9')
  {
    return digit - '0';
  }
  
  // Invalid input.
  return 0xFF;  
}

// Take in a nibble and output a hex digit in ASCII.
uint8 nibbleToHexDigit(uint8 nibble)
{
  return nibble < 0x0A ? nibble + '0' : nibble + 'A' - 0x0A;
}

// Takes two ASCII characters representing hex nibbles as input from UART0 and
// outputs the corresponding binary byte to UART1.
void asciiToBinaryService()
{
    static uint8 count = 0;
    static uint8 firstHexDigit = 0;
    
    // Receive ASCII hex on UART0. Send bytes on UART1.
    while(uart0RxAvailable() && uart1TxAvailable())
    {
        uint8 byte = uart0RxReceiveByte();
        if (IS_HEX_DIGIT(byte))
        {
            if(count == 0)
            {
                firstHexDigit = byte;
                count = 1;
            }
            else
            {
                uart1TxSendByte((hexDigitToNibble(firstHexDigit) << 4) | hexDigitToNibble(byte));
                count = 0;
            }
        }
        else
        {
            count = 0;
        }
    }
}

// Reads arbitrary binary data from UART1 and sends it as ASCII to UART0.
void binaryToAsciiService()
{
    while(uart1RxAvailable() && uart0TxAvailable() >= 3)
    {
        uint8 byte = uart1RxReceiveByte();
        uart0TxSendByte('H');
        uart0TxSendByte(nibbleToHexDigit(byte >> 4));
        uart0TxSendByte(nibbleToHexDigit(byte & 0x0F));
    }
}

void main()
{
    systemInit();
    usbInit();

    uart0Init();
    uart1Init();
    
    uart0SetBaudRate(param_ascii_baud_rate);
    uart1SetBaudRate(param_binary_baud_rate);
    
    while(1)
    {
        boardService();
        usbComService();
        usbShowStatusWithGreenLed();
        
        // Receive and transmit binary bytes on UART1. 
        // Receive and transmit ASCII hex on UART0.
        asciiToBinaryService();
        binaryToAsciiService();
    }
}