#include <radio_address.h>
#include <cc2511_map.h>

void radioAddressInit()
{
    ADDR = param_address;
}

uint8 addressCheckHWConfiguration()
{
	return PKTCTRL1 & 0x03;
}

void setAddressCheckHWConfiguration(uint8 configuration)
{
	PKTCTRL1 = (PKTCTRL1 & 0xFD) | (configuration & 0x03);
}
