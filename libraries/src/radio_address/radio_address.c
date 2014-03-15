/* radio_address.c:
 * this is just an utility library that allow to activate the Hardware Address Check 
 * functionality of the processor
 * 
 * by Carlo Bernaschina (B3rn475)
 * www.bernaschina.com
 */
#include <radio_address.h>
#include <cc2511_map.h>

/* PARAMETERS *****************************************************************/

int32 CODE param_address = 0;

/* GENERAL FUNCTIONS **********************************************************/

// Initialize the address from the parameter
void radioAddressInit()
{
    ADDR = param_address;
}

// Returns the current Address Check configuration
uint8 addressCheckHWConfiguration()
{
	return PKTCTRL1 & 0x03;
}

// Sets the Address Check configuration
void setAddressCheckHWConfiguration(uint8 configuration)
{
	PKTCTRL1 = (PKTCTRL1 & 0xFD) | (configuration & 0x03);
}
