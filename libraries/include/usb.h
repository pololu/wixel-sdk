/*! \file usb.h
 * The <code>usb.lib</code> library takes care of setting up the USB module and
 * responding to standard device requests. This is a general purpose library that
 * can be used to implement many different kinds of USB device interfaces.
 *
 * If you are looking for a simple way to send and receive bytes over USB, please
 * see usb_com.h.
 *
 * Many of the constants defined here come from the
 * <a href="http://www.usb.org/developers/docs/">USB 2.0 Specification</a>.
 * In particular:
 * - The USB_RECIPIENT_* defines come from Table 9-2.
 * - The USB_REQUEST_TYPE_* defines come from Table 9-2.
 * - The other USB_REQUEST_* defines come from Table 9-4.
 * - The USB_DESCRIPTOR_TYPE_* defines come from Table 9-5.
 * - The USB_FEATURE_* defines come from Table 9-6.
 * - The USB_CONFIG_ATTR_* defines come from Table 9-10.
 * - The USB_TRANSFER_TYPE_* defines come from Table 9-13.
 * */

#ifndef _USB_H
#define _USB_H

#include <cc2511_types.h>

/*! This is the Vendor ID assigned to Pololu Corporation by the USB
 * Implementers Forum (USB-IF).
 * For information on getting your own Vendor ID, see
 * http://www.usb.org/developers/vendor/  */
#define USB_VENDOR_ID_POLOLU 0x1FFB

/*! The maximum size of the data packets sent and received on Endpoint 0
 * for control transfers. */
#define USB_EP0_PACKET_SIZE 32

/* USB CONSTANTS **************************************************************/

/*! This enum represents the different USB states defined in Section 9.1 of
 * the USB specification. */
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

// Values for usbSetupPacket.direction bit.
#define USB_OUT 0u
#define USB_IN  1u

// USB Endpoint addresses, to be used in descriptors
// To get the address of an IN endpoint, do: USB_ENDPOINT_ADDRESS_IN | 1
#define USB_ENDPOINT_ADDRESS_IN    0x80
#define USB_ENDPOINT_ADDRESS_OUT   0x00

// Endpoint transfer types, used in endpoint descriptors (USB Spec Table 9-13)
#define USB_TRANSFER_TYPE_CONTROL     0
#define USB_TRANSFER_TYPE_ISOCHRONOUS 1
#define USB_TRANSFER_TYPE_BULK        2
#define USB_TRANSFER_TYPE_INTERRUPT   3

/*! The USB language ID for English (US).
 * It comes from http://www.usb.org/developers/docs/USB_LANGIDs.pdf */
#define USB_LANGUAGE_EN_US 0x0409

/* USB STRUCTS ****************************************************************/

typedef union
{
    /*! These are the standard field names from Table 9-2 of USB 2.0 spec. */
    struct
    {
        uint8 bmRequestType;
        uint8 bRequest;
        uint16 wValue;
        uint16 wIndex;
        uint16 wLength;
    };

    /*! This struct breaks down bmRequestType into its components. */
    struct
    {
        unsigned recipient:5;   //see REQUEST_RECIPIENT_*
        unsigned requestType:2; //see REQUEST_TYPE_*
        unsigned direction:1;   //0=USB_OUT,1=USB_IN
    };
} USB_SETUP_PACKET;

/*! Standard Device Descriptor from USB Spec Table 9-8. */
typedef struct USB_DESCRIPTOR_DEVICE
{
    uint8  bLength;               /*!< sizeof(USB_DEVICE_DESCRIPTOR) */
    uint8  bDescriptorType;       /*!< Should be DESCRIPTOR_DEVICE. */
    uint16 bcdUSB;                /*!< USB Spec Release Number (BCD). */
    uint8  bDeviceClass;          /*!< Class code (assigned by the USB-IF). 0xFF-Vendor specific. */
    uint8  bDeviceSubClass;       /*!< Subclass code (assigned by the USB-IF). */
    uint8  bDeviceProtocol;       /*!< Protocol code (assigned by the USB-IF). 0xFF-Vendor specific. */
    uint8  bMaxPacketSize0;       /*!< Maximum packet size for endpoint 0. */
    uint16 idVendor;              /*!< Vendor ID (assigned by the USB-IF), use USB_VENDOR_ID_POLOLU */
    uint16 idProduct;             /*!< Product ID (assigned by the manufacturer). */
    uint16 bcdDevice;             /*!< Device release number (BCD). */
    uint8  iManufacturer;         /*!< Index of String Descriptor describing the manufacturer. */
    uint8  iProduct;              /*!< Index of String Descriptor describing the product. */
    uint8  iSerialNumber;         /*!< Index of String Descriptor with the device's serial number. */
    uint8  bNumConfigurations;    /*!< Number of possible configurations. */
} USB_DESCRIPTOR_DEVICE;

/*! Standard Configuration Descriptor from USB Spec Table 9-10 */
typedef struct USB_DESCRIPTOR_CONFIGURATION
{
    uint8  bLength;               /*!< sizeof(struct USB_CONFIGURATION_DESCRIPTOR) */
    uint8  bDescriptorType;       /*!< Should be DESCRIPTOR_CONFIGURATION */
    uint16 wTotalLength;          /*!< Total length of all the descriptors returned for this configuration. */
    uint8  bNumInterfaces;        /*!< Number of interfaces supported by this configuration. */
    uint8  bConfigurationValue;   /*!< Value to use as an argument to the REQUEST_SET_CONFIGURATION to select this configuration (should be 1 in this library) */
    uint8  iConfiguration;        /*!< Index of String Descriptor describing this configuration. */
    uint8  bmAttributes;          /*!< D7:1, D6:Self-powered, D5:Remote wakeup, D4-0:0 */
    uint8  bMaxPower;             /*!< Maximum power consumption from the bus in units of 2 mA (50 = 100 mA) */
} USB_DESCRIPTOR_CONFIGURATION;

/*! Interface Association Descriptor.
 * This was defined in Engineering Change Notice (ECN) #7, Table 9-Z,
 * not the USB specification. */
typedef struct USB_DESCRIPTOR_INTERFACE_ASSOCIATION
{
    uint8  bLength;               /*!< sizeof(struct USB_CONFIGURATION_DESCRIPTOR) */
    uint8  bDescriptorType;       /*!< Should be DESCRIPTOR_INTERFACE_ASSOCIATION */
    uint8  bFirstInterface;       /*!< Interface number of first interface */
    uint8  bInterfaceCount;       /*!< Number of contiguous interfaces in this function */
    uint8  bFunctionClass;        /*!< Function class. */
    uint8  bFunctionSubClass;     /*!< Function subclass. */
    uint8  bFunctionProtocol;     /*!< Function protocol. */
    uint8  iFunction;             /*!< Index of String Descriptor for this function. */
} USB_DESCRIPTOR_INTERFACE_ASSOCIATION;

/*! Standard Interface Descriptor from USB Spec Table 9-12. */
typedef struct USB_DESCRIPTOR_INTERFACE
{
    uint8  bLength;               /*!< sizeof(struct USB_INTERFACE_DESCRIPTOR) */
    uint8  bDescriptorType;       /*!< Should be DESCRIPTOR_INTERFACE */
    uint8  bInterfaceNumber;      /*!< Number of this interface.  Zero-based value in the array of interfaces supported by this configuration. */
    uint8  bAlternateSetting;     /*!< Value used to select this alternate setting (should be 0 in this library) */
    uint8  bNumEndpoints;         /*!< Number of endpoints used (excluded EP0) */
    uint8  bInterfaceClass;       /*!< Class code (assigned by USB-IF).  Zero is reserved, 0xFF is vendor specific. */
    uint8  bInterfaceSubClass;    /*!< Subclass code */
    uint8  bInterfaceProtocol;    /*!< Protocol code */
    uint8  iInterface;            /*!< Index of String Descriptor for this interface. */
} USB_DESCRIPTOR_INTERFACE;

/*! Standard Endpoint Descriptor from USB Spec Table 9-13. */
typedef struct USB_DESCRIPTOR_ENDPOINT
{
    uint8  bLength;               /*!< sizeof(USB_ENDPOINT_DESCRIPTOR) */
    uint8  bDescriptorType;       /*!< Should be DESCRIPTOR_ENDPOINT */
    uint8  bEndpointAddress;      /*!< D7: OUT(0) or IN(1).  D6-4: Zero.  D3-0: endpoint number */
    uint8  bmAttributes;          /*!< TRANSFER_TYPE_* (and other stuff for isochronous) */
    uint16 wMaxPacketSize;        /*!< Max packet size in bytes. */
    uint8  bInterval;             /*!< how often to poll */
} USB_DESCRIPTOR_ENDPOINT;

/* CC2511 USB CONSTANTS *******************************************************/

// USBCSOL register bit values
#define USBCSOL_OUTPKT_RDY   0x01

// USBCSIL register bit values
#define USBCSIL_INPKT_RDY    0x01
#define USBCSIL_PKT_PRESENT  0x02

/* HELPERS ********************************************************************/

/*! Defines a new USB string descriptor.
 *
 * \param name The variable name to use for the string descriptor.
 * \param char_count The number of characters in the string.
 *
 * This macro evaluates to a line of C code that defines a static #uint16 array
 * in #CODE space with the specified name.
 *
 * See the usb_cdc_acm.c for an example use. */
#define DEFINE_STRING_DESCRIPTOR(name,char_count,...) static uint16 CODE name[] = { (2*(char_count+1)) | (USB_DESCRIPTOR_TYPE_STRING<<8), __VA_ARGS__ };

/* PROTOTYPES DEFINED BY LIBUSB ***********************************************/

/*! The current USB Device State of this device.
 *
 * Higher-level code should avoid reading or writing from non-zero USB
 * endpoints if #usbDeviceState is not USB_STATE_CONFIGURED because the
 * non-zero endpoints are get initialized when the device enters the
 * configured state. */
extern enum USB_DEVICE_STATES XDATA usbDeviceState;

/*! The value of the last setup packet received from the host.
 * It is useful to read this in usbCallbackSetupHandler(),
 * usbCallbackControlWriteHandler(), and
 * usbCallbackClassDescriptorHandler().
 *  */
extern USB_SETUP_PACKET XDATA usbSetupPacket;

/*! Initializes the USB library.
 * This should be called before any other USB functions. */
void usbInit(void);

/*! Handles any USB events that needs to be handled.
 *
 * This function calls the usbCallback* functions when needed.
 *
 * This function should be called regularly (more often than every 50&nbsp;ms).
 */
void usbPoll(void);

/*! Tells the USB library to start a Control Read
 * (Device-to-Host) transfer.
 *
 * \param bytesCount The number of bytes to send to the USB Host.
 * \param source A pointer to the first byte to send.
 *
 * This should only be called from usbCallbackSetupHandler() or
 * usbCallbackClassDescriptorHandler().
 */
void usbControlRead(uint16 bytesCount, uint8 XDATA * source);

/*! Tells the USB library to start a Control Write
 * (Host-to-Device) transfer.
 *
 * \param bytesCount The number of bytes to read from the host.
 * \param source A pointer to the location that the bytes
 *   will be written to.
 *
 * This should only be called from usbCallbackSetupHandler() or
 * usbCallbackClassDescriptorHandler().
 */
void usbControlWrite(uint16 bytesCount, uint8 XDATA * source);

/*! Tells the USB library to acknowledge the request.
 * This is the right way to handle Control Write transfers with
 * no data.
 *
 * This should only be called from usbCallbackSetupHandler() or
 * usbCallbackClassDescriptorHandler().
 */
void usbControlAcknowledge(void);

/*! Tells the USB library that the request was invalid or failed,
 * so it should respond to the host with STALL packet.
 *
 * This should only be called from usbCallbackControlWriteHandler()
 * in order to reject a Control Write transfer after the data has
 * been received.
 *
 * It can also be used in usbCallbackSetupHandler() or
 * usbCallbackClassDescriptorHandler(), but it is not necessary
 * in those callbacks because the default behavior of the library
 * in that case is to stall. */
void usbControlStall(void);

/*! Configures the specified endpoint to do double-buffered IN
 * (device-to-host) transactions.
 *
 * This should only be called from usbCallbackInitEndpoints(). */
void usbInitEndpointIn(uint8 endpointNumber, uint8 maxPacketSize);

/*! Configures the specified endpoint to do double-buffered OUT
 * (host-to-device) transactions.
 *
 * This should only be called from usbCallbackInitEndpoints(). */
void usbInitEndpointOut(uint8 endpointNumber, uint8 maxPacketSize);

/*! Writes the specified data to a USB FIFO.
 * This is equivalent to writing data to the FIFO register (e.g. USBF4)
 * one byte at a time.
 * Please refer to the CC2511 datasheet to understand when you can and
 * can not write data to a USB FIFO. */
void usbWriteFifo(uint8 endpointNumber, uint8 count, const uint8 XDATA * buffer);

/*! Reads data from a USB FIFO and writes to the specified memory buffer.
 * This is equivalent to reading data from the FIFO register (e.g. USBF4)
 * one byte at a time.
 * Please refer to the CC2511 datasheet to understand when you can and
 * can not read data from a USB FIFO. */
void usbReadFifo(uint8 endpointNumber, uint8 count, uint8 XDATA * buffer);

/*! Returns 1 if we are connected to a USB bus that is suspended.
 * Returns 0 otherwise.
 *
 * You can use this to know when it is time to turn
 * off the peripherals and go into a power-saving mode. */
BIT usbSuspended(void);

/*! Sleeps until we receive USB resume signaling.
 * Uses PM1.
 *
 * Example usage:
\code
if (usbSuspended() && !selfPowered())
{
    // Here you must shut down anything that draws a lot of current
    // (except the USB pull-up resistor).  Power consumption during
    // suspend mode must be less than 0.5 mA or the device
    // will be out of spec (but usually nothing bad happens).
    usbSleep();
    // Here you should re-enable the things that were disabled.
}
\endcode */
void usbSleep(void);

/*! Direct access to this bit is provided for applications that
 * need to use the P0 interrupt and want USB suspend mode to work.  If you don't
 * fall into that category, please don't use this bit directly: instead you
 * should call usbSuspended() because that function is less likely to change in
 * future versions.  If you want to have a P0 interrupt AND USB Suspend, you
 * should write your ISR like this:
\code
ISR(P0INT, 0)
{
    // Handle the P0 flags you care about here, but you might want to
    // check PICTL first because if we are in suspend mode then the P0
    // interrupt settings may be different from what you set them to be.

    if (P0IFG & 0x80)  // Check USB_RESUME bit.
    {
        usbSuspendMode = 0;   // Causes usbSleep to exit sleep mode.
    }
    P0IFG = 0;   // Clear the flags so this interrupt doesn't run again.
    P0IF = 0;
}
\endcode
 * If your P0 ISR does NOT clear usbSuspendMode, then your device will not be
 * able to wake up out of suspend mode because when USB activity happens, your
 * ISR will (if it's well-written) clear P0IFG so usbSleep won't be able to
 * see that the P0IFG.USB_RESUME bit has been set, and it will keep on sleeping.
 */
extern volatile BIT usbSuspendMode;

/*! This function updates the green LED to reflect the current USB status.
 * If you call this function frequently enough, then the green LED will
 * exhibit these behaviors:
 * - Off when USB is disconnected or if we are in USB suspend mode
 * - On when we are connected to USB and are in the configured state
 *   (which usually means that the drivers are installed correctly),
 *   and flickering whenever there is USB activity.
 * - Blinking with a period of 1024 ms if we are connected to USB but
 *   not in the configured state yet.
 *
 * In order to make the flickering work properly, any library that
 * directly reads or writes from USB FIFOs
 *
 * Note: Calling this function will cause the USB library to have extra
 * dependencies that it normally would not have, because this function
 * accesses the system time and sets the green LED.  You might get a linker
 * error if those things are not available.
 *
 * See #usbActivityFlag for more information on the flickering.
 */
void usbShowStatusWithGreenLed(void);

/*! Libraries that directly access USB registers should set this bit to
 * 1 whenever they read or write data from USB.  This allows the
 * USB led to flicker when there is USB activity.  See
 * usbShowStatusWithGreenLed() for more information.
 *
 * Note: The USB HID library does not set this bit because that would
 * result in constant flickering of the green LED in typical HID
 * applications. */
extern volatile BIT usbActivityFlag;

/* HIGH-LEVEL CALLBACKS AND DATA STRUCTURES REQUIRED BY usb.c *****************/
// usb.c requires these high-level callbacks and data structures:

/*! The device's Device Descriptor.
 *
 * This must be defined by higher-level code.
 * See usb_cdc_acm.c for an example. */
extern USB_DESCRIPTOR_DEVICE CODE usbDeviceDescriptor;

/*! The number of string descriptors on this device.
 *
 * This must be defined by higher-level code.
 * See usb_cdc_acm.c for an example. */
extern uint8 CODE usbStringDescriptorCount;

/*! An array of pointers to the string descriptors.
 * The first entry corresponds to string descriptor 0 (the languages).
 * The second entry corresponds to string descriptor 1, etc.
 *
 * This must be defined by higher-level code.
 * See usb_cdc_acm.c for an example. */
extern uint16 CODE * CODE usbStringDescriptors[];

/*! This is called by usbPoll() whenever a new request (SETUP packet) is received
 * from the host that can not be handled by the USB library.
 *
 * This function should read #usbSetupPacket.
 * If it recognizes the request from the host it should call usbControlRead(),
 * usbControlWrite(), or usbControlAcknowledge().
 * If it does not recognize the request, it should simply return.
 * In this case, the USB library will respond to the host with a STALL packet.
 *
 * This function must be defined by higher-level code.
 * See usb_cdc_acm.c for an example. */
void usbCallbackSetupHandler(void);

/*! This is called by usbPoll() whenever a Get Descriptor request is received by
 * the host that can not be handled by the USB library.
 *
 * This function should read #usbSetupPacket.
 * If it recognizes the Get Descriptor request, it should call usbControlRead().
 * If it does not recognize the request, it should simply return.
 * In this case, the USB library will respond to the host with a STALL packet.
 *
 * This function must be defined by higher-level code.
 * See usb_hid.c for an example. */
void usbCallbackClassDescriptorHandler(void);

/*! This is called by usbPoll() when the device enters the Configured state.
 * This function should call usbInitEndpointIn() and usbInitEndpointOut()
 * to initialize all the non-zero endpoints that it uses.
 *
 * This function must be defined by higher-level code.
 * See usb_cdc_acm.c for an example. */
void usbCallbackInitEndpoints(void);

/*! This is called by usbPoll() when all the data for a Control Write
 * request has been received.
 *
 * This function should look at the data, perform any actions necessary,
 * and decide whether the data was valid.
 * If the data is invaild, this function can call usbControlStall() to
 * reject the request. */
void usbCallbackControlWriteHandler(void);

#endif
