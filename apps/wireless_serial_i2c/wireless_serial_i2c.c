/* wireless_serial app:
 *
 * Pin out:
 * P0_3 = TX
 * P0_2 = RX
 * P1_7 = Radio Transmit Debug Signal
 * P1_6 = Radio Receive Debug Signal
 *
 * == Overview ==
 * This app allows you to connect two Wixels together to make a wireless,
 * bidirectional, lossless serial link.  The Wixels must be on the same radio
 * channel, and all other pairs of Wixels must be at least 2 channels away.
 *
 * == Technical Description ==
 * This device appears to the USB host as a Virtual COM Port, with USB product
 * ID 0x2200.  It uses the radio_link library to do wireless communication.
 *
 * There are three basic serial modes that can be selected:
 * 1) USB-to-Radio: Bytes from the USB virtual COM port get sent to the
 *    radio and vice versa.
 * 2) UART-to-Radio: Bytes from the UART's RX line get sent to the radio
 *    and bytes from the radio get sent to the UART's TX line.
 * 3) USB-to-UART: Just like a normal USB-to-Serial adapter, bytes from
 *    the virtual COM port get sent on the UART's TX line and bytes from
 *    the UART's RX line get sent to the virtual COM port.
 *
 * You can select which serial mode you want to use by setting the serial_mode
 * parameter to the appropriate number (using the numbers above).  Or, you can
 * leave the serial mode at 0 (which is the default).  If the serial_mode is 0,
 * then the Wixel will automatically choose a serial mode based on how it is
 * being powered, and it will switch between the different serial modes on the
 * fly.
 *
 * Power Source | Serial Mode
 * --------------------------
 * USB only     | USB-to-Radio
 * VIN only     | UART-to-Radio
 * USB and VIN  | USB-to-UART
 *
 * == Parameters ==
 *   serial_mode   : Selects the serial mode or auto mode (0-3).
 *   baud_rate     : The baud rate to use for the UART, in bits per second.
 *   radio_channel : See description in radio_link.h.
 *
 * == Example Uses ==
 * 1) This application can be used to make a wireless serial link between two
 *    microcontrollers, with no USB involved.  To do this, use the UART-to-Radio
 *    mode on both Wixels.
 *
 * 2) This application can be used to make a wireless serial link between a
 *    computer and a microcontroller.  Use USB-to-Radio mode on the Wixel that
 *    is connected to the computer and use UART-to-Radio mode on the Wixel
 *    that is connected to the microcontroller.
 *
 * 3) If you are doing option 2 and using the the auto-detect serial mode
 *    (serial_mode = 0), then you have the option to (at any time) plug a USB
 *    cable directly in to the Wixel that is connected to your microcontroller
 *    to establish a more direct (wired) serial connection with the
 *    microcontroller.  (You would, of course, also have to switch to the other
 *    COM port when you do this.)
 */

// TODO: try harder to enable a pull-up on the RX line.  Right now, we get junk
// on the RX line from 60 Hz noise if you connect one end of a cable to the RX line.

/*
 * TODO: Support for USB CDC ACM control signals.
 * TODO: use LEDs to give feedback about sending/receiving bytes.
 * TODO: UART flow control.
 * TODO: Better radio protocol (see TODOs in radio_link.c).
 * TODO: Obey CDC-ACM Set Line Coding commands:
 *       In USB-UART mode this would let the user change the baud rate at run-time.
 *       In USB-RADIO mode, bauds 0-255 would correspond to radio channels.
 */

/** Dependencies **************************************************************/
#include <cc2511_map.h>
#include <board.h>
#include <random.h>
#include <time.h>

#include <usb.h>
#include <usb_com.h>

#include <radio_com.h>
#include <radio_link.h>

#include <uart.h>


/** Parameters ****************************************************************/
#define SERIAL_MODE_AUTO        0
#define SERIAL_MODE_USB_RADIO   1
#define SERIAL_MODE_UART_RADIO  2
#define SERIAL_MODE_USB_UART    3

#define I2CDELAY				20		// units = us

int32 CODE param_serial_mode = SERIAL_MODE_AUTO;

int32 CODE param_baud_rate = 9600;


/** Functions *****************************************************************/
// SCL is P1_0
uint8 readSCL(void)
{
	P1DIR &= ~(1<<0);          // Make the line an input.
	return P1_0;
}

// SDA is P1_1
uint8 readSDA(void)
{
	P1DIR &= ~(1<<1);          // Make the line an input.
	return P1_1;
}

void clearSCL()
{
	P1DIR |= (1<<0);          // Make the line an output.
}

void clearSDA()
{
	P1DIR |= (1<<1);          // Make the line an output.
}

void waitForHighSCL(uint16 timeoutMs)
{
	uint32 time = getMs();
	while (readSCL() == 0 && getMs() - time < timeoutMs);
}


/* Global Data */
uint8 start = 0;

void i2c_start_cond()
{
	/* if start == 1, do a restart cond */
	if (start)
	{
		/* set SDA to 1 */
		readSDA();
		delayMicroseconds(I2CDELAY/2);
		/* Clock stretching */
		waitForHighSCL(100);
	}
	/*if (readSDA() == 0)
	{
		ARBITRATION_LOST();
	}*/
	/* SCL is high, set SDA from 1 to 0 */
	clearSDA();
	delayMicroseconds(I2CDELAY/2);
	clearSCL();
	start = 1;
}

void i2c_stop_cond()
{
	/* set SDA to 0 */
	clearSDA();
	delayMicroseconds(I2CDELAY/2);
	/* Clock stretching */
	waitForHighSCL(100);
	/* SCL is high, set SDA from 0 to 1 */
	/*if (readSDA() == 0)
	{
		ARBITRATION_LOST();
	}*/
	readSDA();
	delayMicroseconds(I2CDELAY/2);
	start = 0;
}

/* Write a bit to I2C bus */
void i2c_write_bit(uint8 bitVal)
{
	if (bitVal)
	{
		readSDA();
	}
	else
	{
		clearSDA();
	}
	delayMicroseconds(I2CDELAY/2);
	/* Clock stretching */
	waitForHighSCL(100);
	/* SCL is high, now data is valid */
	/* If SDA is high, check that nobody else is driving SDA */
	/*if (bitVal && readSDA() == 0)
	{
		ARBITRATION_LOST();
	}*/
	delayMicroseconds(I2CDELAY/2);
	clearSCL();
}

/* Read a bit from I2C bus */
uint8 i2c_read_bit()
{
	uint8 bitVal;
	/* Let the slave drive data */
	readSDA();
	delayMicroseconds(I2CDELAY/2);
	/* Clock stretching */
	waitForHighSCL(100);
	/* SCL is high, now data is valid */
	bitVal = readSDA();
	delayMicroseconds(I2CDELAY/2);
	clearSCL();
	return bitVal;
}

/* Write a byte to I2C bus. Return 0 if ack by the slave */
uint8 i2c_write_byte(uint8 send_start, uint8 send_stop, uint8 byte)
{
	uint8 bitNum;
	uint8 nack;
	if (send_start)
	{
		i2c_start_cond();
	}
	for (bitNum = 0; bitNum < 8; bitNum++)
	{
		i2c_write_bit(byte & 0x80);
		byte <<= 1;
	}
	nack = i2c_read_bit();
	if (send_stop)
	{
		i2c_stop_cond();
	}
	return nack;
}

/* Read a byte from I2C bus */
uint8 i2c_read_byte(uint8 nack, uint8 send_stop)
{
	uint8 byte = 0;
	uint8 bitNum;
	for (bitNum = 0; bitNum < 8; bitNum++)
	{
		byte <<= 1;
		byte |= i2c_read_bit();
	}
	i2c_write_bit(nack);
	if (send_stop)
	{
		i2c_stop_cond();
	}
	return byte;
}


void updateLeds()
{
    usbShowStatusWithGreenLed();

    LED_YELLOW(vinPowerPresent());

    // Turn on the red LED if the radio is in the RX_OVERFLOW state.
    // There used to be several bugs in the radio libraries that would cause
    // the radio to go in to this state, but hopefully they are all fixed now.
    /*if (MARCSTATE == 0x11)
    {
        LED_RED(1);
    }
    else
    {
        LED_RED(0);
    }*/
}

uint8 currentSerialMode()
{
	if ((uint8)param_serial_mode > 0 && (uint8)param_serial_mode <= 3)
	{
		return (uint8)param_serial_mode;
	}

    if (usbPowerPresent())
    {
        if (vinPowerPresent())
        {
        	return SERIAL_MODE_USB_UART;
        }
        else
        {
        	return SERIAL_MODE_USB_RADIO;
        }
    }
    else
    {
    	return SERIAL_MODE_UART_RADIO;
    }
}

void usbToRadioService()
{/*
	while (usbComRxAvailable())
	{
		usbComRxReceiveByte();
		i2c_write_byte(1, 0, 0xEE);
		i2c_write_byte(0, 0, 0xAA);
		i2c_write_byte(1, 0, 0xEF);
		i2c_read_byte(0, 0);
		i2c_read_byte(1, 1);
		LED_RED(!LED_RED_STATE);
	}

    while(usbComRxAvailable() && radioComTxAvailable())
    {
        radioComTxSendByte(usbComRxReceiveByte());
    }

    while(radioComRxAvailable() && usbComTxAvailable())
    {
        usbComTxSendByte(radioComRxReceiveByte());
    }*/
}

void uartToRadioService()
{
    while(uart0RxAvailable() && radioComTxAvailable())
    {
        radioComTxSendByte(uart0RxReceiveByte());
    }

    while(radioComRxAvailable() && uart0TxAvailable())
    {
        uart0TxSendByte(radioComRxReceiveByte());
    }
}

void usbToUartService()
{
    while(usbComRxAvailable() && uart0TxAvailable())
    {
        uart0TxSendByte(usbComRxReceiveByte());
    }

    while(uart0RxAvailable() && usbComTxAvailable())
    {
        usbComTxSendByte(uart0RxReceiveByte());
    }
}


uint16 i2c_read_bmp085_data(uint8 reg)
{
	i2c_write_byte(1, 0, 0xEE);			// device address with LSB set (write)
	i2c_write_byte(0, 0, reg);			// write the register address to read
	i2c_write_byte(1, 0, 0xEF);			// device address with LSb cleared (read)
	return (uint16)i2c_read_byte(0, 0) * 256 + i2c_read_byte(1, 1);
}

void i2c_write_bmp085_ctrl(uint8 reg, uint8 val)
{
	i2c_write_byte(1, 0, 0xEE);
	i2c_write_byte(0, 0, reg);
	i2c_write_byte(0, 1, val);
}

void main()
{
    uint16 AC1;
    uint16 AC2;
    uint16 AC3;
    uint16 AC4;
    uint16 AC5;
    uint16 AC6;
    uint16 B1;
    uint16 B2;
    uint16 MB;
    uint16 MC;
    uint16 MD;

    uint32 UT;
    uint32 UP;

    uint32 X1;
    uint32 X2;
    uint32 B5;
    uint32 T;

    systemInit();
    usbInit();

    uart0Init();
    uart0SetBaudRate(param_baud_rate);

    radioComInit();
    randomSeedFromAdc();

    // Set up P1_6 to be the RX debug signal and P1_7 to be the TX debug signal.
    P1DIR |= (1<<6) | (1<<7);
    IOCFG1 = 0b001000; // P1_6 = Preamble Quality Reached
    IOCFG2 = 0b011011; // P1_7 = PA_PD (TX mode)

    P1_0 = 0;
    P1_1 = 0;

    delayMs(20);

    // read calibration coefficients
    AC1 = i2c_read_bmp085_data(0xAA);
    AC2 = i2c_read_bmp085_data(0xAC);
    AC3 = i2c_read_bmp085_data(0xAE);
    AC4 = i2c_read_bmp085_data(0xB0);
    AC5 = i2c_read_bmp085_data(0xB2);
    AC6 = i2c_read_bmp085_data(0xB4);
    B1 = i2c_read_bmp085_data(0xB6);
    B2 = i2c_read_bmp085_data(0xB8);
    MB = i2c_read_bmp085_data(0xBA);
    MC = i2c_read_bmp085_data(0xBC);
    MD = i2c_read_bmp085_data(0xBE);

    while(1)
    {
        boardService();
        updateLeds();

        radioComTxService();
        usbComService();

        i2c_write_bmp085_ctrl(0xF4, 0x2E);
        delayMs(5);
        UT = i2c_read_bmp085_data(0xF6);
        i2c_write_bmp085_ctrl(0xF4, 0x34);		// osrs = 0
        delayMs(5);
        UP = i2c_read_bmp085_data(0xF6);

        while (usbComRxAvailable())
		{
        	usbComRxReceiveByte();
        	usbComTxSendByte(UT >> 8);
        	usbComTxSendByte(UT & 0x7F);
        	usbComTxSendByte(UP >> 8);
        	usbComTxSendByte(UP & 0x7F);
		}
/*
        switch(currentSerialMode())
        {
        case SERIAL_MODE_USB_RADIO:  usbToRadioService();  break;
        case SERIAL_MODE_UART_RADIO: uartToRadioService(); break;
        case SERIAL_MODE_USB_UART:   usbToUartService();   break;
        }
*/
    }
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
