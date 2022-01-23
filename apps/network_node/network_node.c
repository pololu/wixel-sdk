/** network_node app:
 *
 *This app allows you to usa a wixel as a network relay (using radio_network).
 *
 * by Carlo Bernaschina (B3rn475)
 * www.bernaschina.com
*/

/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>
#include <usb.h>
#include <usb_com.h>
#include <random.h>
#include <time.h>
#include <radio_network.h>
#include <radio_address.h>
#include <adc.h>
#include <stdio.h>

/** Functions *****************************************************************/
void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(vinPowerPresent());
    LED_RED(radioNetworkRxCurrentPacket());
}

void main(void)
{
    systemInit();
    usbInit();
    radioNetworkInit();
    
    while(1)
    {
        updateLeds();
        boardService();
        usbComService();
        if (radioNetworkRxCurrentPacket()){
            radioNetworkRxDoneWithPacket();
        }
    }
}
