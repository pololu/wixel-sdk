#include <usb_hid.h>
#include <usb.h>
#include <board.h>
#include <time.h>

/* HID Library Configuration **************************************************/

#define HID_DATA_ENDPOINT             1
#define HID_IN_PACKET_SIZE            8
#define HID_KEYBOARD_INTERFACE_NUMBER 0
#define HID_MOUSE_INTERFACE_NUMBER    1

#define HID_KEYBOARD_ENDPOINT         1
#define HID_KEYBOARD_FIFO             USBF1   // This must match HID_KEYBOARD_ENDPOINT!

#define HID_MOUSE_ENDPOINT            2
#define HID_MOUSE_FIFO                USBF2   // This must match HID_MOUSE_ENDPOINT!

/* HID Constants **************************************************************/

// USB Class Code from HID 1.11 Section 4.1: The HID Class
#define HID_CLASS    3

// USB Subclass Code from HID 1.11 Section 4.2: Subclass
#define HID_SUBCLASS_BOOT 1

// USB Protocol Codes from HID 1.11 Section 4.3: Protocols
#define HID_PROTOCOL_KEYBOARD 1
#define HID_PROTOCOL_MOUSE    2

// USB Descriptor types from HID 1.11 Section 7.1
#define HID_DESCRIPTOR_TYPE_HID    0x21
#define HID_DESCRIPTOR_TYPE_REPORT 0x22

// Country Codes from HID 1.11 Section 6.2.1
#define HID_COUNTRY_NOT_LOCALIZED 0

// HID Report Items from HID 1.11 Section 6.2.2
#define HID_USAGE_PAGE      0x05
#define HID_USAGE           0x09
#define HID_COLLECTION      0xA1
#define HID_END_COLLECTION  0xC0
#define HID_REPORT_COUNT    0x95
#define HID_REPORT_SIZE     0x75
#define HID_USAGE_MIN       0x19
#define HID_USAGE_MAX       0x29
#define HID_LOGICAL_MIN     0x15
#define HID_LOGICAL_MAX     0x25
#define HID_LOGICAL_MAX_2   0x26 // 2-byte data
#define HID_INPUT           0x81
#define HID_OUTPUT          0x91

// HID Report Usage Pages from HID Usage Tables 1.11 Section 3, Table 1
#define HID_USAGE_PAGE_GENERIC_DESKTOP 0x01
#define HID_USAGE_PAGE_KEY_CODES       0x07
#define HID_USAGE_PAGE_LEDS            0x08
#define HID_USAGE_PAGE_BUTTONS         0x09

// HID Report Usages from HID Usage Tables 1.11 Section 4, Table 6
#define HID_USAGE_POINTER  0x01
#define HID_USAGE_MOUSE    0x02
#define HID_USAGE_KEYBOARD 0x06
#define HID_USAGE_X        0x30
#define HID_USAGE_Y        0x31
#define HID_USAGE_WHEEL    0x38

// HID Report Collection Types from HID 1.11 6.2.2.6
#define HID_COLLECTION_PHYSICAL    0
#define HID_COLLECTION_APPLICATION 1

// HID Input/Output/Feature Item Data (attributes) from HID 1.11 6.2.2.5
#define HID_ITEM_CONSTANT 0x1
#define HID_ITEM_VARIABLE 0x2
#define HID_ITEM_RELATIVE 0x4

// Request Codes from HID 1.11 Section 7.2
#define HID_REQUEST_GET_REPORT   0x1
#define HID_REQUEST_GET_IDLE     0x2
#define HID_REQUEST_GET_PROTOCOL 0x3
#define HID_REQUEST_SET_REPORT   0x9
#define HID_REQUEST_SET_IDLE     0xA
#define HID_REQUEST_SET_PROTOCOL 0xB

// Report Types from HID 1.11 Section 7.2.1
#define HID_REPORT_TYPE_INPUT   1
#define HID_REPORT_TYPE_OUTPUT  2
#define HID_REPORT_TYPE_FEATURE 3

// Protocols from HID 1.11 Section 7.2.5
#define HID_PROTOCOL_BOOT   0
#define HID_PROTOCOL_REPORT 1

/* HID USB Descriptors ****************************************************/

USB_DESCRIPTOR_DEVICE CODE usbDeviceDescriptor =
{
    sizeof(USB_DESCRIPTOR_DEVICE),
    USB_DESCRIPTOR_TYPE_DEVICE,
    0x0200,                 // USB Spec Release Number in BCD format
    0,                      // Class Code: undefined (use class code info from Interface Descriptors)
    0,                      // Subclass code
    0,                      // Protocol
    USB_EP0_PACKET_SIZE,    // Max packet size for Endpoint 0
    USB_VENDOR_ID_POLOLU,   // Vendor ID
    0x2201,                 // Product ID FIXME tmphax
    0x0000,                 // Device release number in BCD format
    1,                      // Index of Manufacturer String Descriptor
    2,                      // Index of Product String Descriptor
    3,                      // Index of Serial Number String Descriptor
    1                       // Number of possible configurations.
};

// keyboard report descriptor
// HID 1.11 Section 6.2.2: Report Descriptor
// Uses same format as keyboard boot interface report descriptor - see HID 1.11 Appendix B.1
uint8 CODE keyboardReportDescriptor[]
=
{
    HID_USAGE_PAGE, HID_USAGE_PAGE_GENERIC_DESKTOP,
    HID_USAGE, HID_USAGE_KEYBOARD,
    HID_COLLECTION, HID_COLLECTION_APPLICATION,

        HID_REPORT_COUNT, 8,                        // 8 Modifier Keys
        HID_REPORT_SIZE, 1,
        HID_USAGE_PAGE, HID_USAGE_PAGE_KEY_CODES,
        HID_USAGE_MIN, 224,                         // Left Control
        HID_USAGE_MAX, 231,                         // Right GUI (Windows key) (highest defined usage ID)
        HID_LOGICAL_MIN, 0,
        HID_LOGICAL_MAX, 1,
        HID_INPUT, HID_ITEM_VARIABLE,

        HID_REPORT_COUNT, 1,                        // Reserved (1 byte)
        HID_REPORT_SIZE, 8,
        HID_INPUT, HID_ITEM_CONSTANT,

        HID_REPORT_COUNT, 8,                        // 8 LEDs
        HID_REPORT_SIZE, 1,
        HID_USAGE_PAGE, HID_USAGE_PAGE_LEDS,
        HID_USAGE_MIN, 1,                           // Num Lock
        HID_USAGE_MAX, 8,                           // Do Not Disturb (TODO: use this as a bootloader signal?)
        HID_OUTPUT, HID_ITEM_VARIABLE,

        HID_REPORT_COUNT, 6,                        // 6 Key Codes
        HID_REPORT_SIZE, 8,
        HID_USAGE_PAGE, HID_USAGE_PAGE_KEY_CODES,
        HID_USAGE_MIN, 0,
        HID_USAGE_MAX, 231,
        HID_LOGICAL_MIN, 0,
        HID_LOGICAL_MAX_2, 231, 0,                  // Logical Maximum is signed, so we need to use an extra byte to make sure the highest bit is 0
        HID_INPUT, 0,

    HID_END_COLLECTION,
};

// mouse report descriptor
// HID 1.11 Section 6.2.2: Report Descriptor
uint8 CODE mouseReportDescriptor[]
=
{
    HID_USAGE_PAGE, HID_USAGE_PAGE_GENERIC_DESKTOP,
    HID_USAGE, HID_USAGE_MOUSE,
    HID_COLLECTION, HID_COLLECTION_APPLICATION,

        HID_USAGE, HID_USAGE_POINTER,
        HID_COLLECTION, HID_COLLECTION_PHYSICAL,

            HID_REPORT_COUNT, 8,                            // 8 Mouse Buttons
            HID_REPORT_SIZE, 1,
            HID_USAGE_PAGE, HID_USAGE_PAGE_BUTTONS,
            HID_USAGE_MIN, 1,
            HID_USAGE_MAX, 5,
            HID_LOGICAL_MIN, 0,
            HID_LOGICAL_MAX, 1,
            HID_INPUT, HID_ITEM_VARIABLE,

            HID_REPORT_COUNT, 3,                            // 3 Axes (X, Y, wheel)
            HID_REPORT_SIZE, 8,
            HID_USAGE_PAGE, HID_USAGE_PAGE_GENERIC_DESKTOP,
            HID_USAGE, HID_USAGE_X,
            HID_USAGE, HID_USAGE_Y,
            HID_USAGE, HID_USAGE_WHEEL,
            HID_LOGICAL_MIN, -127,
            HID_LOGICAL_MAX, 127,
            HID_INPUT, (HID_ITEM_VARIABLE | HID_ITEM_RELATIVE),

        HID_END_COLLECTION,

    HID_END_COLLECTION,
};

CODE struct CONFIG1 {
    struct USB_DESCRIPTOR_CONFIGURATION configuration;

    struct USB_DESCRIPTOR_INTERFACE keyboard_interface;
    uint8 keyboard_hid[9]; // HID Descriptor
    struct USB_DESCRIPTOR_ENDPOINT keyboard_in;

    struct USB_DESCRIPTOR_INTERFACE mouse_interface;
    uint8 mouse_hid[9]; // HID Descriptor
    struct USB_DESCRIPTOR_ENDPOINT mouse_in;
} usbConfigurationDescriptor
=
{
    {                                                    // Configuration Descriptor
        sizeof(struct USB_DESCRIPTOR_CONFIGURATION),
        USB_DESCRIPTOR_TYPE_CONFIGURATION,
        sizeof(struct CONFIG1),                          // wTotalLength
        2,                                               // bNumInterfaces
        1,                                               // bConfigurationValue
        0,                                               // iConfiguration
        0xC0,                                            // bmAttributes: self powered (but may use bus power)
        50,                                              // bMaxPower
    },
    {                                                    // Keyboard Interface
        sizeof(struct USB_DESCRIPTOR_INTERFACE),
        USB_DESCRIPTOR_TYPE_INTERFACE,
        HID_KEYBOARD_INTERFACE_NUMBER,                   // bInterfaceNumber
        0,                                               // bAlternateSetting
        1,                                               // bNumEndpoints
        HID_CLASS,                                       // bInterfaceClass
        HID_SUBCLASS_BOOT,                               // bInterfaceSubClass
        HID_PROTOCOL_KEYBOARD,                           // bInterfaceProtocol
        0                                                // iInterface
    },
    {
        sizeof(usbConfigurationDescriptor.keyboard_hid), // 9-byte HID Descriptor for keyboard (HID 1.11 Section 6.2.1)
        HID_DESCRIPTOR_TYPE_HID,
        0x11, 0x01,                                      // bcdHID.  We conform to HID 1.11.
        HID_COUNTRY_NOT_LOCALIZED,                       // bCountryCode
        1,                                               // bNumDescriptors
        HID_DESCRIPTOR_TYPE_REPORT,                      // bDescriptorType
        sizeof(keyboardReportDescriptor), 0              // wDescriptorLength
    },
    {                                                    // Keyboard IN Endpoint
        sizeof(struct USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_IN | HID_KEYBOARD_ENDPOINT, // bEndpointAddress
        USB_TRANSFER_TYPE_INTERRUPT,                     // bmAttributes
        HID_IN_PACKET_SIZE,                              // wMaxPacketSize
        10,                                              // bInterval
    },
    {                                                    // Mouse Interface
        sizeof(struct USB_DESCRIPTOR_INTERFACE),
        USB_DESCRIPTOR_TYPE_INTERFACE,
        HID_MOUSE_INTERFACE_NUMBER,                      // bInterfaceNumber
        0,                                               // bAlternateSetting
        1,                                               // bNumEndpoints
        HID_CLASS,                                       // bInterfaceClass
        HID_SUBCLASS_BOOT,                               // bInterfaceSubClass
        HID_PROTOCOL_MOUSE,                              // bInterfaceProtocol
        0                                                // iInterface
    },
    {
        sizeof(usbConfigurationDescriptor.mouse_hid),    // 9-byte HID Descriptor for mouse (HID 1.11 Section 6.2.1)
        HID_DESCRIPTOR_TYPE_HID,
        0x11, 0x01,                                      // bcdHID.  We conform to HID 1.11.
        HID_COUNTRY_NOT_LOCALIZED,                       // bCountryCode
        1,                                               // bNumDescriptors
        HID_DESCRIPTOR_TYPE_REPORT,                      // bDescriptorType
        sizeof(mouseReportDescriptor), 0                 // wDescriptorLength
    },
    {                                                    // Mouse IN Endpoint
        sizeof(struct USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_IN | HID_MOUSE_ENDPOINT,    // bEndpointAddress
        USB_TRANSFER_TYPE_INTERRUPT,                     // bmAttributes
        HID_IN_PACKET_SIZE,                              // wMaxPacketSize
        10,                                              // bInterval
    },
};

uint8 CODE usbStringDescriptorCount = 4;
DEFINE_STRING_DESCRIPTOR(languages, 1, USB_LANGUAGE_EN_US)
DEFINE_STRING_DESCRIPTOR(manufacturer, 18, 'P','o','l','o','l','u',' ','C','o','r','p','o','r','a','t','i','o','n')
DEFINE_STRING_DESCRIPTOR(product, 5, 'W','i','x','e','l')
uint16 CODE * CODE usbStringDescriptors[] = { languages, manufacturer, product, serialNumberStringDescriptor };

/* HID structs and global variables *******************************************/

struct HID_KEYBOARD_OUT_REPORT XDATA usbHidKeyboardOutput = {0};
struct HID_KEYBOARD_IN_REPORT XDATA usbHidKeyboardInput = {0, 0, {0}};
struct HID_MOUSE_IN_REPORT XDATA usbHidMouseInput = {0, 0, 0, 0};

BIT usbHidKeyboardInputUpdated = 0;
BIT usbHidMouseInputUpdated    = 0;

uint16 XDATA hidKeyboardIdleDuration = 500; // 0 to 1020 ms
uint16 XDATA hidKeyboardLastReportTime = 0;

BIT hidKeyboardProtocol = HID_PROTOCOL_REPORT;
BIT hidMouseProtocol    = HID_PROTOCOL_REPORT;

/* HID USB callbacks **********************************************************/
// These functions are called by the low-level USB module (usb.c) when a USB
// event happens that requires higher-level code to make a decision.

void usbCallbackInitEndpoints(void)
{
    usbInitEndpointIn(HID_KEYBOARD_ENDPOINT, HID_IN_PACKET_SIZE);
    usbInitEndpointIn(HID_MOUSE_ENDPOINT, HID_IN_PACKET_SIZE);
}

// Implements all the control transfers that are required by Appendix G of HID 1.11.
void usbCallbackSetupHandler(void)
{
    static XDATA uint8 response;

    if ((usbSetupPacket.bmRequestType & 0x7F) != 0x21)   // Require Type==Class and Recipient==Interface.
        return;

    switch(usbSetupPacket.bRequest)
    {
    // required
    case HID_REQUEST_GET_REPORT:
        if ((usbSetupPacket.wValue >> 8) != HID_REPORT_TYPE_INPUT)
            return;

        switch (usbSetupPacket.wIndex)
        {
        case HID_KEYBOARD_INTERFACE_NUMBER:
            usbControlRead(sizeof(usbHidKeyboardInput), (uint8 XDATA *)&usbHidKeyboardInput);
            return;

        case HID_MOUSE_INTERFACE_NUMBER:
            usbControlRead(sizeof(usbHidMouseInput), (uint8 XDATA *)&usbHidMouseInput);
            return;

        default:
            // unrecognized interface - stall
            return;
        }

    // required for devices with Output reports
    case HID_REQUEST_SET_REPORT:
        if (usbSetupPacket.wIndex == HID_KEYBOARD_INTERFACE_NUMBER)
        {
            usbControlWrite(sizeof(usbHidKeyboardOutput), (uint8 XDATA *)&usbHidKeyboardOutput);
        }
        return;

    // required for keyboards
    case HID_REQUEST_GET_IDLE:
        if (usbSetupPacket.wIndex == HID_KEYBOARD_INTERFACE_NUMBER)
        {
            response = hidKeyboardIdleDuration / 4; // value in request is in units of 4 ms
            usbControlRead(1, (uint8 XDATA *)&response);
        }
        return;

    // required for keyboards
    case HID_REQUEST_SET_IDLE:
        if (usbSetupPacket.wIndex == HID_KEYBOARD_INTERFACE_NUMBER)
        {
            hidKeyboardIdleDuration = (usbSetupPacket.wValue >> 8) * 4; // value in request is in units of 4 ms
            usbControlAcknowledge();
        }
        return;


    // required for boot devices
    case HID_REQUEST_GET_PROTOCOL:
        switch (usbSetupPacket.wIndex)
        {
        case HID_KEYBOARD_INTERFACE_NUMBER:
            response = hidKeyboardProtocol;
            usbControlRead(1, (uint8 XDATA *)&response);
            return;

        case HID_MOUSE_INTERFACE_NUMBER:
            response = hidMouseProtocol;
            usbControlRead(1, (uint8 XDATA *)&response);
            return;

        default:
            // unrecognized interface - stall
            return;
        }

    // required for boot devices
    case HID_REQUEST_SET_PROTOCOL:
        switch (usbSetupPacket.wIndex)
        {
        case HID_KEYBOARD_INTERFACE_NUMBER:
            hidKeyboardProtocol = usbSetupPacket.wValue;
            break;

        case HID_MOUSE_INTERFACE_NUMBER:
            hidMouseProtocol = usbSetupPacket.wValue;
            break;

        default:
            // unrecognized interface - stall
            return;
        }
        usbControlAcknowledge();
        return;

    default:
        // unrecognized request - stall
        return;
    }
}

void usbCallbackClassDescriptorHandler(void)
{
    // Require Direction==Device-to-Host, Type==Standard, and Recipient==Interface. (HID 1.11 Section 7.1.1)
    if (usbSetupPacket.bmRequestType != 0x81)
    {
        return;
    }

    switch (usbSetupPacket.wValue >> 8)
    {
    case HID_DESCRIPTOR_TYPE_HID:
        // The host has requested the HID descriptor of a particular interface.
        switch (usbSetupPacket.wIndex)
        {
        case HID_KEYBOARD_INTERFACE_NUMBER:
            usbControlRead(sizeof(usbConfigurationDescriptor.keyboard_hid), (uint8 XDATA *)&usbConfigurationDescriptor.keyboard_hid);
            return;

        case HID_MOUSE_INTERFACE_NUMBER:
            usbControlRead(sizeof(usbConfigurationDescriptor.mouse_hid), (uint8 XDATA *)&usbConfigurationDescriptor.mouse_hid);
            return;
        }
        return;

    case HID_DESCRIPTOR_TYPE_REPORT:
        // The host has requested the Report Descriptor of a particular interface.
        switch (usbSetupPacket.wIndex)
        {
        case HID_KEYBOARD_INTERFACE_NUMBER:
            usbControlRead(sizeof(keyboardReportDescriptor), (uint8 XDATA *)&keyboardReportDescriptor);
            return;

        case HID_MOUSE_INTERFACE_NUMBER:
            usbControlRead(sizeof(mouseReportDescriptor), (uint8 XDATA *)&mouseReportDescriptor);
            return;
        }
        return;
    }
}

void usbCallbackControlWriteHandler(void)
{
}

/* Other HID Functions ********************************************************/

void usbHidService(void)
{
    usbPoll();

    if (usbDeviceState != USB_STATE_CONFIGURED)
    {
        // We have not reached the Configured state yet, so we should not be touching the non-zero endpoints.
        return;
    }

    USBINDEX = HID_KEYBOARD_ENDPOINT;
    // Check if keyboard input has been updated OR if the idle period is nonzero and has expired.
    if ((usbHidKeyboardInputUpdated || (hidKeyboardIdleDuration && ((uint16)(getMs() - hidKeyboardLastReportTime) > hidKeyboardIdleDuration))) && !(USBCSIL & USBCSIL_INPKT_RDY))
    {
        usbWriteFifo(HID_KEYBOARD_ENDPOINT, sizeof(usbHidKeyboardInput), (uint8 XDATA *)&usbHidKeyboardInput);
        USBCSIL |= USBCSIL_INPKT_RDY;
        usbHidKeyboardInputUpdated = 0;
        hidKeyboardLastReportTime = getMs();
    }

    USBINDEX = HID_MOUSE_ENDPOINT;
    // Check if mouse input has been updated.
    if (usbHidMouseInputUpdated && !(USBCSIL & USBCSIL_INPKT_RDY)) {
        usbWriteFifo(HID_MOUSE_ENDPOINT, sizeof(usbHidMouseInput), (uint8 XDATA *)&usbHidMouseInput);
        USBCSIL |= USBCSIL_INPKT_RDY;
        usbHidMouseInputUpdated = 0;
    }
}
