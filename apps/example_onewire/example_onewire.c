/** example_onewire: read Dallas Semiconductor OneWire devices.
Written by Russell Nelson <nelson@crynwr.com>. No copyright claimed.

== Description ==

Searches the OneWire bus for either a DS18B20 or DS1820. They differ by the id number (first byte) and by the format of the data they return. When it finds a DS18*20, it remembers the address and starts a sequence of temperature reads, once per second. It prints the temperature in degrees C via the USB as a serial port. Fire up a terminal and open up the serial port, or if you're on Linux, just: cat </dev/ttyACM0 and Bob's your uncle.

Does not use the wireless. No reason why not.

== Parameters ==

The pin number is defined in onewire_ports.h.

== Default pinout ==

The DS device(s) are connected to P0_0. Because the code uses programmed loops for timing, I didn't want to fiddle with the timing in the Arduino code, and was afraid that the gpio library would change the timing too much. Feel free to modify it to use gpio for extra flexibility.

 */

#include <cc2511_map.h>
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

#include "onewire.h"

const char *respondstr = NULL;

uint8 XDATA DS1820_addr[8];
uint8 is_DS18B20;
uint32 ds1820_time;

void start_DS1820()
{
  onewire_reset();
  onewire_select(DS1820_addr);
  onewire_write(0x44,0);         // start conversion, with parasite power off at the end
}

void setup_DS1820(void) {
  onewire_start();
  if ( !onewire_search(DS1820_addr)) {
    onewire_reset_search();
    delayMs(250);
    onewire_search(DS1820_addr);
  }
  
  if ( onewire_crc8( DS1820_addr, 7) != DS1820_addr[7]) {
      respondstr = "No OneWire devices found";
      return;
  }
  
  if ( DS1820_addr[0] == 0x10) {
    is_DS18B20 = FALSE;
  } else if ( DS1820_addr[0] == 0x28) {
    is_DS18B20 = TRUE;
  } else {
    respondstr = "No DS1820 found";
    return;
  }

  start_DS1820();
  ds1820_time = getMs();
}


// return the temperature in C multiplied by 16
int read_DS1820()
{
  uint8 i;
  uint8 present = 0;
  uint8 dataread[12];
  int temp_read;

  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = onewire_reset();
  onewire_select(DS1820_addr);    
  onewire_write(0xBE,0);         // Read Scratchpad
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    dataread[i] = onewire_read();
  }
  temp_read = ((dataread[1] << 8) | dataread[0]);
  if (!is_DS18B20) {
    // temp_read is currently in half degrees.
    temp_read *= 8.0;
    //
    temp_read += - ( 8 * (dataread[7]- dataread[6]) )/dataread[7];
  }
  return temp_read;
}


void updateLeds()
{
    usbShowStatusWithGreenLed();

    //LED_YELLOW_TOGGLE();
    //LED_YELLOW(1);

    LED_RED(0);
}

uint8 nibbleToAscii(uint8 nibble)
{
    nibble &= 0xF;
    if (nibble <= 0x9){ return '0' + nibble; }
    else{ return 'A' + (nibble - 0xA); }
}

void handleOneWire(void)
{
    int i;
    int newtemp = 0;
    int air_temp_c = 0;
    unsigned int decimals;

    if (getMs() > ds1820_time + 1000) {
        air_temp_c = read_DS1820();
        newtemp++;
        start_DS1820();
        ds1820_time = getMs();
    }

    if ((newtemp || respondstr) && usbComTxAvailable() >= 64)
    {
        const char *cp;
        uint8 XDATA response[64];
        uint8 responseLength = 0;

        decimals = (air_temp_c & 0xf) * 100;
        responseLength = sprintf(response, "%d.%02d", air_temp_c / 16, decimals / 16);
        response[responseLength++] = ',';
        for (i = 0; i < 8; i++) {
            response[responseLength++] = nibbleToAscii(DS1820_addr[i] >> 4);
            response[responseLength++] = nibbleToAscii(DS1820_addr[i]);
        }
        response[responseLength++] = ',';
        i = sizeof(response) - responseLength - 2;

        if (respondstr) {
            for (cp = respondstr; *respondstr && i--; respondstr++) {
                response[responseLength++] = *cp;
            }
        }
        respondstr = NULL;
        response[responseLength++] = '\r';
        response[responseLength++] = '\n';
        usbComTxSend(response, responseLength);
    }
}

void main()
{
    systemInit();

    usbInit();
    setup_DS1820();
    while(1)
    {
        boardService();
        updateLeds();
        usbComService();
        handleOneWire();
    }
}
