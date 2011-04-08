#include <cc2511_map.h>
#include <board.h>
#include <time.h>

#include <gpio.h>
#include <adc.h>
#include <usb.h>
#include <usb_com.h>
#include <usb_hid.h> // for HID constants
#include <random.h>
#include <radio_queue.h>
#include <math.h>

#define MAX_TX_INTERVAL 10 // maximum time between transmissions (ms)

void updateLeds()
{
    usbShowStatusWithGreenLed();
    LED_YELLOW(vinPowerPresent());
}

void txMouseState()
{
    float x, y, magnitude;
    uint8 XDATA * txBuf;
    uint8 lastTx = 0;

    if ((uint8)(getMs() - lastTx) > MAX_TX_INTERVAL && (txBuf = radioQueueTxCurrentPacket()))
    {
        x = -((float)adcRead(2 | ADC_BITS_7) - 1024) / 128;
        y =  ((float)adcRead(1 | ADC_BITS_7) - 1024) / 128;
        magnitude = sqrtf(x * x + y * y);
        txBuf[1] = (int8)(x * magnitude);
        txBuf[2] = (int8)(y * magnitude);
        txBuf[3] = !isPinHigh(0) << MOUSE_BUTTON_LEFT;
        *txBuf = 3; // set packet length byte
        radioQueueTxSendPacket();

        lastTx = getMs();
    }
}

void main()
{
    systemInit();
    usbInit();

    radioQueueInit();
    randomSeedFromSerialNumber();

    // disable pull-ups on inputs from accelerometer
    setDigitalInput(1, HIGH_IMPEDANCE);
    setDigitalInput(2, HIGH_IMPEDANCE);

    while(1)
    {
        updateLeds();
        boardService();
        usbComService();

        txMouseState();
    }
}
