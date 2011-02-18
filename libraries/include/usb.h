#ifndef _USB_H
#define _USB_H

#include <cc2511_types.h>

#define USB_VENDOR_ID_POLOLU 0x1FFB
#define WIXEL_SERIAL_NUMBER_STRING_ADDR  0x3E6
#define USB_EP0_PACKET_SIZE 32

/* USB CONSTANTS **************************************************************/

enum USB_DEVICE_STATES
{
    USB_STATE_DETACHED = 0,
    USB_STATE_ATTACHED = 1,
    USB_STATE_POWERED = 2,
    USB_STATE_DEFAULT = 4,
    USB_STATE_ADDRESS = 8,
    USB_STATE_CONFIGURED = 16
};

// SETUP_PACKET.bRequest values from USB Spec Table 9-4
#define USB_REQUEST_GET_STATUS 0u
#define USB_REQUEST_CLEAR_FEATURE 1u
#define USB_REQUEST_SET_FEATURE 3u
#define USB_REQUEST_SET_ADDRESS 5u
#define USB_REQUEST_GET_DESCRIPTOR 6u
#define USB_REQUEST_SET_DESCRIPTOR 7u
#define USB_REQUEST_GET_CONFIGURATION 8u
#define USB_REQUEST_SET_CONFIGURATION 9u
#define USB_REQUEST_GET_INTERFACE 10u
#define USB_REQUEST_SET_INTERFACE 11u
#define USB_REQUEST_SYNCH_FRAME 12u

// SETUP_PACKET.request_type values from USB Spec Table 9-2
#define USB_REQUEST_TYPE_STANDARD 0u
#define USB_REQUEST_TYPE_CLASS 1u
#define USB_REQUEST_TYPE_VENDOR 2u

// SETUP_PACKET.recipient values from USB Spec Table 9-2
#define USB_RECIPIENT_DEVICE 0u
#define USB_RECIPIENT_INTERFACE 1u
#define USB_RECIPIENT_ENDPOINT 2u
#define USB_RECIPIENT_OTHER 3u

// Descriptor types from USB Spec Table 9-5
#define USB_DESCRIPTOR_TYPE_DEVICE 1u
#define USB_DESCRIPTOR_TYPE_CONFIGURATION 2u
#define USB_DESCRIPTOR_TYPE_STRING 3u
#define USB_DESCRIPTOR_TYPE_INTERFACE 4u
#define USB_DESCRIPTOR_TYPE_ENDPOINT 5u
#define USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER 6u
#define USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION 7u
#define USB_DESCRIPTOR_TYPE_INTERFACE_POWER 8u
#define USB_DESCRIPTOR_TYPE_ENDPOINT_CLASS_SPECIFIC 0x25u
#define USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION 0x0Bu

// setup_packet feature selectors, USB Spec Table 9-6, Standard Feature Selectors
#define USB_FEATURE_DEVICE_REMOTE_WAKEUP 1u
#define USB_FEATURE_ENDPOINT_HALT 0u
#define USB_FEATURE_TEST_MODE 2u

// Configuration Decriptor bmAttributes bitmasks (USB Spec Table 9-10)
#define USB_CONFIG_ATTR_DEFAULT       (1<<7) //Default Value (bit 7 must be set to 1)
#define USB_CONFIG_ATTR_SELF_POWERED  (1<<6) //Self-powered
#define USB_CONFIG_ATTR_REMOTE_WAKEUP (1<<5) //Remote Wakeup

// Values for USB_PM_SETUP_PACKET.direction bit.
#define USB_OUT 0u
#define USB_IN  1u

// USB Endpoint addresses, to be used in descriptors
// To get the address of an IN endpoint, do: USB_ENDPOINT_ADDRESS_IN | 1
#define USB_ENDPOINT_ADDRESS_IN    0x80
#define USB_ENDPOINT_ADDRESS_OUT   0x00

// Endpoint transfer types, used in endpoint descriptors
#define USB_TRANSFER_TYPE_CONTROL     0
#define USB_TRANSFER_TYPE_ISOCHRONOUS 1
#define USB_TRANSFER_TYPE_BULK        2
#define USB_TRANSFER_TYPE_INTERRUPT   3

// language ids from http://www.usb.org/developers/docs/USB_LANGIDs.pdf
#define USB_LANGUAGE_EN_US 0x0409

/* USB STRUCTS ****************************************************************/

typedef union
{
     // These are the standard field names from Table 9-2 of USB 2.0 spec.
    struct
    {
        uint8 bmRequestType;
        uint8 bRequest;
        uint16 wValue;
        uint16 wIndex;
        uint16 wLength;
    };

    // This struct breaks down bmRequestType in to its components.
    struct
    {
        unsigned recipient:5;   //see REQUEST_RECIPIENT_*
        unsigned requestType:2; //see REQUEST_TYPE_*
        unsigned direction:1;   //0=USB_OUT,1=USB_IN
    };
} USB_SETUP_PACKET;

// USB2.0 Table 9-8: Standard Device Descriptor
typedef struct USB_DESCRIPTOR_DEVICE
{
    uint8  bLength;               // sizeof(USB_DEVICE_DESCRIPTOR)
    uint8  bDescriptorType;       // DEVICE descriptor type (DESCRIPTOR_DEVICE).
    uint16 bcdUSB;                // USB Spec Release Number (BCD).
    uint8  bDeviceClass;          // Class code (assigned by the USB-IF). 0xFF-Vendor specific.
    uint8  bDeviceSubClass;       // Subclass code (assigned by the USB-IF).
    uint8  bDeviceProtocol;       // Protocol code (assigned by the USB-IF). 0xFF-Vendor specific.
    uint8  bMaxPacketSize0;       // Maximum packet size for endpoint 0.
    uint16 idVendor;              // Vendor ID (assigned by the USB-IF), use USB_VENDOR_ID_POLOLU
    uint16 idProduct;             // Product ID (assigned by the manufacturer).
    uint16 bcdDevice;             // Device release number (BCD).
    uint8  iManufacturer;         // Index of String Descriptor describing the manufacturer.
    uint8  iProduct;              // Index of String Descriptor describing the product.
    uint8  iSerialNumber;         // Index of String Descriptor with the device's serial number.
    uint8  bNumConfigurations;    // Number of possible configurations.
} USB_DESCRIPTOR_DEVICE;

typedef struct USB_DESCRIPTOR_CONFIGURATION // USB2.0 Table 9-10: Standard Configuration Descriptor
{
    uint8  bLength;               // sizeof(struct USB_CONFIGURATION_DESCRIPTOR)
    uint8  bDescriptorType;       // DESCRIPTOR_CONFIGURATION
    uint16 wTotalLength;          // Total length of all the descriptors returned for this configuration.
    uint8  bNumInterfaces;        // Number of interfaces supported by this configuration.
    uint8  bConfigurationValue;   // Value to use as an argument to the REQUEST_SET_CONFIGURATION to select this configuration (should be 1 in this library)
    uint8  iConfiguration;        // Index of String Descriptor describing this configuration.
    uint8  bmAttributes;          // D7:1, D6:Self-powered, D5:Remote wakeup, D4-0:0
    uint8  bMaxPower;             // Maximum power consumption from the bus in units of 2 mA (50 = 100 mA)
} USB_DESCRIPTOR_CONFIGURATION;

typedef struct USB_DESCRIPTOR_INTERFACE_ASSOCIATION // USB ECN: Interface Association Descriptor Table 9-Z
{
    uint8  bLength;               // sizeof(struct USB_CONFIGURATION_DESCRIPTOR)
    uint8  bDescriptorType;       // DESCRIPTOR_INTERFACE_ASSOCIATION
    uint8  bFirstInterface;       // Interface number of first interface
    uint8  bInterfaceCount;       // Number of contiguous interfaces in this function
    uint8  bFunctionClass;
    uint8  bFunctionSubClass;
    uint8  bFunctionProtocol;
    uint8  iFunction;             // Index of String Descriptor for this function.
} USB_DESCRIPTOR_INTERFACE_ASSOCIATION;

typedef struct USB_DESCRIPTOR_INTERFACE // USB2.0 Table 9-12: Standard Interface Descriptor
{
    uint8  bLength;               // sizeof(struct USB_INTERFACE_DESCRIPTOR)
    uint8  bDescriptorType;       // DESCRIPTOR_INTERFACE
    uint8  bInterfaceNumber;      // Number of this interface.  Zero-based value in the array of interfaces supported by this configuration.
    uint8  bAlternateSetting;     // Value used to select this alternate setting (should be 0 in this library)
    uint8  bNumEndpoints;         // Number of endpoints used (excluded EP0)
    uint8  bInterfaceClass;       // Class code (assigned by USB-IF).  Zero is reserved, 0xFF is vendor specific.
    uint8  bInterfaceSubClass;    // Subclass code
    uint8  bInterfaceProtocol;    // Protocol code
    uint8  iInterface;            // Index of String Descriptor for this interface.
} USB_DESCRIPTOR_INTERFACE;

typedef struct USB_DESCRIPTOR_ENDPOINT // USB2.0 Table 9-13: Standard Endpoint Descriptor
{
    uint8  bLength;               // sizeof(USB_ENDPOINT_DESCRIPTOR)
    uint8  bDescriptorType;       // DESCRIPTOR_ENDPOINT
    uint8  bEndpointAddress;      // D7: OUT(0) or IN(1).  D6-4: Zero.  D3-0: endpoint number
    uint8  bmAttributes;          // TRANSFER_TYPE_* (and other stuff for isochronous)
    uint16 wMaxPacketSize;        // bytes
    uint8  bInterval;             // how often to poll
} USB_DESCRIPTOR_ENDPOINT;

/* CC2511 USB CONSTANTS *******************************************************/

// USBCSOL register bit values
#define USBCSOL_OUTPKT_RDY   0x01

// USBCSIL register bit values
#define USBCSIL_INPKT_RDY    0x01
#define USBCSIL_PKT_PRESENT  0x02

/* HELPERS ********************************************************************/

#define DEFINE_STRING_DESCRIPTOR(name,letter_count,...) static uint16 CODE name[] = { (2*(letter_count+1)) | (USB_DESCRIPTOR_TYPE_STRING<<8), __VA_ARGS__ };

/* PROTOTYPES DEFINED BY LIBUSB ***********************************************/

extern enum USB_DEVICE_STATES XDATA usbDeviceState;
extern USB_SETUP_PACKET XDATA usbSetupPacket;

void usbInit();
void usbPoll();

void usbControlRead(uint16 bytesCount, uint8 XDATA * source);
void usbControlWrite(uint16 bytesCount, uint8 XDATA * source);
void usbControlAcknowledge();
void usbControlStall();

void usbInitEndpointIn(uint8 endpointNumber, uint8 maxPacketSize);
void usbInitEndpointOut(uint8 endpointNumber, uint8 maxPacketSize);

void usbWriteFifo(uint8 endpointNumber, uint8 count, const uint8 XDATA * buffer);
void usbReadFifo(uint8 endpointNumber, uint8 count, uint8 XDATA * buffer);

/* usbSuspended returns 1 if we are connected to a USB bus that is suspended.
 * It returns 0 otherwise.  You can use this to know when it is time to turn
 * off the peripherals and go in to a power-saving mode. */
BIT usbSuspended();

/* TODO: document usbSleep
 *
 * Example usage:
 * 		if (usbSuspended() && !selfPowered())
 *  	{
 *          // Here you must shut down anything that draws a lot of current
 *          // (except the USB pull-up resistor).  Power consumption during
 *          // suspend mode must be less than 0.5 mA or the device
 *          // will be out of spec (but usually nothing bad happens).
 *  		usbSleep();
 *  		// Here you should re-enable the things that were disabled.
 *  	}
 */
void usbSleep();

/* usbSuspendMode:  Direct access to this bit is provided for applications that
 * need to use the P0 interrupt and want USB suspend mode to work.  If you don't
 * fall in to that category, please don't use this bit directly: instead you
 * should call usbSuspended() because that function is less likely to change in
 * future versions.  If you want to have a P0 interrupt AND USB Suspend, you
 * should write your ISR like this:
 *
 * ISR(P0INT, 1)
 * {
 * 	   // Handle the P0 flags you care about here, but you might want to
 *     // check PICTL first because if we are in suspend mode then the P0
 *     // interrupt settings may be different from what you set them to be.
 *
 *     if (P0IFG & 0x80)  // Check USB_RESUME bit.
 *     {
 *         usbSuspendMode = 0;   // Causes usbSleep to exit sleep mode.
 *     }
 *     P0IFG = 0;   // Clear the flags so this interrupt doesn't run again.
 *     P0IF = 0;
 * }
 *
 * If your P0 ISR does NOT clear usbSuspendMode, then your device will not be
 * able to wake up out of suspend mode because when USB activity happens, your
 * ISR will (if it's well-written) clear P0IFG so usbSleep won't be able to
 * see that the P0IFG.USB_RESUME bit has been set, and it will keep on sleeping.
 */
extern volatile BIT usbSuspendMode;

/* usbShowStatusWithGreenLed:
 * This function updates the green LED to reflect the current USB status.
 * If you call this function frequently enough, then the green LED will
 * exhibit these behaviors:
 * - Off when USB is disconnected or if we are in USB suspend mode
 * - On when we are connected to USB and are in the configured state
 *   (which usually means that the drivers are installed correctly)
 * - Blinking with a period of 1024 ms if we are connected to USB but
 *   not in the configured state yet.
 *
 * Note: Calling this function will cause the usb library to have extra
 * dependencies that it normally would not have, because this function
 * accesses the system time and sets the green LED.  You might get a linker
 * error if those things are not available.
 */
void usbShowStatusWithGreenLed();

/* LOW-LEVEL HARDWARE PROYOTYPES REQUIRED BY usb.c ****************************/
// usb.c requires these low-level hardware functions to be available:

BIT usbPowerPresent();
BIT vinPowerPresent();
void enableUsbPullup();
void disableUsbPullup();

/* HIGH-LEVEL CALLBACKS AND DATA STRUCTURES REQUIRED BY usb.c *****************/
// usb.c requires these high-level callbacks and data structures:

extern USB_DESCRIPTOR_DEVICE CODE usbDeviceDescriptor;
extern uint8 CODE usbStringDescriptorCount;
extern uint16 CODE * CODE usbStringDescriptors[];
void usbCallbackSetupHandler();
void usbCallbackInitEndpoints();
void usbCallbackControlWriteHandler();

#endif
