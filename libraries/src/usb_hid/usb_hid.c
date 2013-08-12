// TODO: Add a function for converting chars to keycodes.  The test_hid app
//   contains the start of such a function.
// TODO: Don't use double-buffered USB endpoints.  It just increases latency.

#include <usb_hid.h>
#include <usb.h>
#include <board.h>
#include <time.h>

/* HID Library Configuration **************************************************/

#define HID_IN_KEYBOARD_PACKET_SIZE   8
#define HID_IN_MOUSE_PACKET_SIZE      4
#define HID_IN_JOYSTICK_PACKET_SIZE   20

#define HID_KEYBOARD_INTERFACE_NUMBER 0
#define HID_MOUSE_INTERFACE_NUMBER    1
#define HID_JOYSTICK_INTERFACE_NUMBER 2

#define HID_KEYBOARD_ENDPOINT         1
#define HID_KEYBOARD_FIFO             USBF1   // This must match HID_KEYBOARD_ENDPOINT!

#define HID_MOUSE_ENDPOINT            2
#define HID_MOUSE_FIFO                USBF2   // This must match HID_MOUSE_ENDPOINT!

#define HID_JOYSTICK_ENDPOINT         3
#define HID_JOYSTICK_FIFO             USBF3   // This must match HID_JOYSTICK_ENDPOINT!

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
#define HID_LOGICAL_MIN_2   0x16 // 2-byte data
#define HID_LOGICAL_MAX     0x25
#define HID_LOGICAL_MAX_2   0x26 // 2-byte data
#define HID_INPUT           0x81
#define HID_OUTPUT          0x91

// HID Report Usage Pages from HID Usage Tables 1.12 Section 3, Table 1
#define HID_USAGE_PAGE_GENERIC_DESKTOP 0x01
#define HID_USAGE_PAGE_KEY_CODES       0x07
#define HID_USAGE_PAGE_LEDS            0x08
#define HID_USAGE_PAGE_BUTTONS         0x09

// HID Report Usages from HID Usage Tables 1.12 Section 4, Table 6
#define HID_USAGE_POINTER  0x01
#define HID_USAGE_MOUSE    0x02
#define HID_USAGE_JOYSTICK 0x04
#define HID_USAGE_KEYBOARD 0x06
#define HID_USAGE_X        0x30
#define HID_USAGE_Y        0x31
#define HID_USAGE_Z        0x32
#define HID_USAGE_RX       0x33
#define HID_USAGE_RY       0x34
#define HID_USAGE_RZ       0x35
#define HID_USAGE_SLIDER   0x36
#define HID_USAGE_DIAL     0x37
#define HID_USAGE_WHEEL    0x38

// HID Report Collection Types from HID 1.12 6.2.2.6
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
    0x2201,                 // Product ID
    0x0000,                 // Device release number in BCD format
    1,                      // Index of Manufacturer String Descriptor
    2,                      // Index of Product String Descriptor
    3,                      // Index of Serial Number String Descriptor
    1                       // Number of possible configurations.
};

// keyboard report descriptor
// HID 1.11 Section 6.2.2: Report Descriptor
// Uses format compatible with keyboard boot interface report descriptor - see HID 1.11 Appendix B.1
uint8 CODE keyboardReportDescriptor[]
=
{
    HID_USAGE_PAGE, HID_USAGE_PAGE_GENERIC_DESKTOP,
    HID_USAGE, HID_USAGE_KEYBOARD,
    HID_COLLECTION, HID_COLLECTION_APPLICATION,

        HID_REPORT_COUNT, 8,                        // 8 Modifier Keys
        HID_REPORT_SIZE, 1,
        HID_USAGE_PAGE, HID_USAGE_PAGE_KEY_CODES,
        HID_USAGE_MIN, 0xE0,                        // Left Control
        HID_USAGE_MAX, 0xE7,                        // Right GUI (Windows key) (highest defined usage ID)
        HID_LOGICAL_MIN, 0,
        HID_LOGICAL_MAX, 1,
        HID_INPUT, HID_ITEM_VARIABLE,

        HID_REPORT_COUNT, 1,                        // Reserved (1 byte)
        HID_REPORT_SIZE, 8,
        HID_INPUT, HID_ITEM_CONSTANT,

        HID_REPORT_COUNT, 8,                        // 8 LEDs
        HID_REPORT_SIZE, 1,
        HID_USAGE_PAGE, HID_USAGE_PAGE_LEDS,
        HID_USAGE_MIN, 0x1,                         // Num Lock
        HID_USAGE_MAX, 0x8,                         // Do Not Disturb (TODO: use this as a bootloader signal?)
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
// Uses format compatible with mouse boot interface report descriptor - see HID 1.11 Appendix B.2
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
            HID_USAGE_MAX, 8,
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

// joystick report descriptor
// HID 1.11 Section 6.2.2: Report Descriptor
uint8 CODE joystickReportDescriptor[]
=
{
    HID_USAGE_PAGE, HID_USAGE_PAGE_GENERIC_DESKTOP,
    HID_USAGE, HID_USAGE_JOYSTICK,
    HID_COLLECTION, HID_COLLECTION_APPLICATION,

        HID_USAGE, HID_USAGE_POINTER,
        HID_COLLECTION, HID_COLLECTION_PHYSICAL,

            HID_REPORT_COUNT, 8, // 8 Axes (X, Y, Z, Rx, Ry, Rz, Slider, Dial)
            HID_REPORT_SIZE, 16,
            HID_USAGE, HID_USAGE_X,
            HID_USAGE, HID_USAGE_Y,
            HID_USAGE, HID_USAGE_Z,
            HID_USAGE, HID_USAGE_RX,
            HID_USAGE, HID_USAGE_RY,
            HID_USAGE, HID_USAGE_RZ,
            HID_USAGE, HID_USAGE_SLIDER,
            HID_USAGE, HID_USAGE_DIAL,
            HID_LOGICAL_MIN_2, 0x01, 0x80, // -32767
            HID_LOGICAL_MAX_2, 0xFF, 0x7F, //  32767
            HID_INPUT, HID_ITEM_VARIABLE,

        HID_END_COLLECTION,

        HID_REPORT_COUNT, 32, // 32 Joystick Buttons
        HID_REPORT_SIZE, 1,
        HID_USAGE_PAGE, HID_USAGE_PAGE_BUTTONS,
        HID_USAGE_MIN, 1,
        HID_USAGE_MAX, 32,
        HID_LOGICAL_MIN, 0,
        HID_LOGICAL_MAX, 1,
        HID_INPUT, HID_ITEM_VARIABLE,

    HID_END_COLLECTION,
};

CODE struct CONFIG1 {
    USB_DESCRIPTOR_CONFIGURATION configuration;

    USB_DESCRIPTOR_INTERFACE keyboard_interface;
    uint8 keyboard_hid[9]; // HID Descriptor
    USB_DESCRIPTOR_ENDPOINT keyboard_in;

    USB_DESCRIPTOR_INTERFACE mouse_interface;
    uint8 mouse_hid[9]; // HID Descriptor
    USB_DESCRIPTOR_ENDPOINT mouse_in;

    USB_DESCRIPTOR_INTERFACE joystick_interface;
    uint8 joystick_hid[9]; // HID Descriptor
    USB_DESCRIPTOR_ENDPOINT joystick_in;
} usbConfigurationDescriptor
=
{
    {                                                    // Configuration Descriptor
        sizeof(USB_DESCRIPTOR_CONFIGURATION),
        USB_DESCRIPTOR_TYPE_CONFIGURATION,
        sizeof(struct CONFIG1),                          // wTotalLength
        3,                                               // bNumInterfaces
        1,                                               // bConfigurationValue
        0,                                               // iConfiguration
        0xC0,                                            // bmAttributes: self powered (but may use bus power)
        50,                                              // bMaxPower
    },
    {                                                    // Keyboard Interface
        sizeof(USB_DESCRIPTOR_INTERFACE),
        USB_DESCRIPTOR_TYPE_INTERFACE,
        HID_KEYBOARD_INTERFACE_NUMBER,                   // bInterfaceNumber
        0,                                               // bAlternateSetting
        1,                                               // bNumEndpoints
        HID_CLASS,                                       // bInterfaceClass
        HID_SUBCLASS_BOOT,                               // bInterfaceSubClass
        HID_PROTOCOL_KEYBOARD,                           // bInterfaceProtocol
        4                                                // iInterface
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
        sizeof(USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_IN | HID_KEYBOARD_ENDPOINT, // bEndpointAddress
        USB_TRANSFER_TYPE_INTERRUPT,                     // bmAttributes
        HID_IN_KEYBOARD_PACKET_SIZE,                     // wMaxPacketSize
        10,                                              // bInterval
    },
    {                                                    // Mouse Interface
        sizeof(USB_DESCRIPTOR_INTERFACE),
        USB_DESCRIPTOR_TYPE_INTERFACE,
        HID_MOUSE_INTERFACE_NUMBER,                      // bInterfaceNumber
        0,                                               // bAlternateSetting
        1,                                               // bNumEndpoints
        HID_CLASS,                                       // bInterfaceClass
        HID_SUBCLASS_BOOT,                               // bInterfaceSubClass
        HID_PROTOCOL_MOUSE,                              // bInterfaceProtocol
        5                                                // iInterface
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
        sizeof(USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_IN | HID_MOUSE_ENDPOINT,    // bEndpointAddress
        USB_TRANSFER_TYPE_INTERRUPT,                     // bmAttributes
        HID_IN_MOUSE_PACKET_SIZE,                        // wMaxPacketSize
        10,                                              // bInterval
    },
    {                                                    // Joystick Interface
        sizeof(USB_DESCRIPTOR_INTERFACE),
        USB_DESCRIPTOR_TYPE_INTERFACE,
        HID_JOYSTICK_INTERFACE_NUMBER,                   // bInterfaceNumber
        0,                                               // bAlternateSetting
        1,                                               // bNumEndpoints
        HID_CLASS,                                       // bInterfaceClass
        0,                                               // bInterfaceSubClass
        0,                                               // bInterfaceProtocol
        6                                                // iInterface
    },
    {
        sizeof(usbConfigurationDescriptor.joystick_hid), // 9-byte HID Descriptor for joystick (HID 1.11 Section 6.2.1)
        HID_DESCRIPTOR_TYPE_HID,
        0x11, 0x01,                                      // bcdHID.  We conform to HID 1.11.
        HID_COUNTRY_NOT_LOCALIZED,                       // bCountryCode
        1,                                               // bNumDescriptors
        HID_DESCRIPTOR_TYPE_REPORT,                      // bDescriptorType
        sizeof(joystickReportDescriptor), 0              // wDescriptorLength
    },
    {                                                    // Joystick IN Endpoint
        sizeof(USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_IN | HID_JOYSTICK_ENDPOINT, // bEndpointAddress
        USB_TRANSFER_TYPE_INTERRUPT,                     // bmAttributes
        HID_IN_JOYSTICK_PACKET_SIZE,                     // wMaxPacketSize
        10,                                              // bInterval
    },
};

uint8 CODE usbStringDescriptorCount = 7;
DEFINE_STRING_DESCRIPTOR(languages, 1, USB_LANGUAGE_EN_US)
DEFINE_STRING_DESCRIPTOR(manufacturer, 18, 'P','o','l','o','l','u',' ','C','o','r','p','o','r','a','t','i','o','n')
DEFINE_STRING_DESCRIPTOR(product, 5, 'W','i','x','e','l')
DEFINE_STRING_DESCRIPTOR(keyboardName, 14, 'W','i','x','e','l',' ','K','e','y','b','o','a','r','d')
DEFINE_STRING_DESCRIPTOR(mouseName, 11, 'W','i','x','e','l',' ','M','o','u','s','e')
DEFINE_STRING_DESCRIPTOR(joystickName, 14, 'W','i','x','e','l',' ','J','o','y','s','t','i','c','k')
uint16 CODE * CODE usbStringDescriptors[] = { languages, manufacturer, product, serialNumberStringDescriptor, keyboardName, mouseName, joystickName };

/* HID structs and global variables *******************************************/

HID_KEYBOARD_OUT_REPORT XDATA usbHidKeyboardOutput = {0};
HID_KEYBOARD_IN_REPORT XDATA usbHidKeyboardInput = {0, 0, {0}};
HID_MOUSE_IN_REPORT XDATA usbHidMouseInput = {0, 0, 0, 0};
HID_JOYSTICK_IN_REPORT XDATA usbHidJoystickInput = {0, 0, 0, 0, 0, 0, 0, 0, 0};

BIT usbHidKeyboardInputUpdated = 0;
BIT usbHidMouseInputUpdated    = 0;
BIT usbHidJoystickInputUpdated    = 0;

uint16 XDATA hidKeyboardIdleDuration = 500; // 0 to 1020 ms

BIT hidKeyboardProtocol = HID_PROTOCOL_REPORT;
BIT hidMouseProtocol    = HID_PROTOCOL_REPORT;

/* HID USB callbacks **********************************************************/
// These functions are called by the low-level USB module (usb.c) when a USB
// event happens that requires higher-level code to make a decision.

void usbCallbackInitEndpoints(void)
{
    usbInitEndpointIn(HID_KEYBOARD_ENDPOINT, HID_IN_KEYBOARD_PACKET_SIZE);
    usbInitEndpointIn(HID_MOUSE_ENDPOINT, HID_IN_MOUSE_PACKET_SIZE);
    usbInitEndpointIn(HID_JOYSTICK_ENDPOINT, HID_IN_JOYSTICK_PACKET_SIZE);
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

        case HID_JOYSTICK_INTERFACE_NUMBER:
            usbControlRead(sizeof(usbHidJoystickInput), (uint8 XDATA *)&usbHidJoystickInput);
            return;
        }
        // unrecognized interface - stall
        return;

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
        }
        // unrecognized interface - stall
        return;

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

        case HID_JOYSTICK_INTERFACE_NUMBER:
            usbControlRead(sizeof(usbConfigurationDescriptor.joystick_hid), (uint8 XDATA *)&usbConfigurationDescriptor.joystick_hid);
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

        case HID_JOYSTICK_INTERFACE_NUMBER:
            usbControlRead(sizeof(joystickReportDescriptor), (uint8 XDATA *)&joystickReportDescriptor);
            return;
        }
        return;
    }
}

void usbCallbackControlWriteHandler(void)
{
    // not used by usb_hid
}

/* Other HID Functions ********************************************************/

void usbHidService(void)
{
    static uint16 XDATA hidKeyboardLastReportTime = 0;

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
        usbHidKeyboardInputUpdated = 0; // reset updated flag
        hidKeyboardLastReportTime = getMs();
    }

    USBINDEX = HID_MOUSE_ENDPOINT;
    // Check if mouse input has been updated.
    if (usbHidMouseInputUpdated && !(USBCSIL & USBCSIL_INPKT_RDY)) {
        usbWriteFifo(HID_MOUSE_ENDPOINT, sizeof(usbHidMouseInput), (uint8 XDATA *)&usbHidMouseInput);
        USBCSIL |= USBCSIL_INPKT_RDY;
        usbHidMouseInputUpdated = 0; // reset updated flag
    }

    USBINDEX = HID_JOYSTICK_ENDPOINT;
    // Check if joystick input has been updated.
    if (usbHidJoystickInputUpdated && !(USBCSIL & USBCSIL_INPKT_RDY)) {
        usbWriteFifo(HID_JOYSTICK_ENDPOINT, sizeof(usbHidJoystickInput), (uint8 XDATA *)&usbHidJoystickInput);
        USBCSIL |= USBCSIL_INPKT_RDY;
        usbHidJoystickInputUpdated = 0; // reset updated flag
    }
}

// Look-up table stored in code memory that we use to convert from ASCII
// characters to HID key codes.
uint8 CODE hidKeyCode[128] =
{
    0,                 // 0x00 Null
    0,                 // 0x01
    0,                 // 0x02
    0,                 // 0x03
    0,                 // 0x04
    0,                 // 0x05
    0,                 // 0x06
    0,                 // 0x07
    KEY_BACKSPACE,     // 0x08 Backspace
    KEY_TAB,           // 0x09 Horizontal Tab
    KEY_RETURN,        // 0x0A Line Feed
    0,                 // 0x0B
    0,                 // 0x0C
    KEY_RETURN,        // 0x0D Carriage return
    0,                 // 0x0E
    0,                 // 0x0F
    0,                 // 0x10
    0,                 // 0x11
    0,                 // 0x12
    0,                 // 0x13
    0,                 // 0x14
    0,                 // 0x15
    0,                 // 0x16
    0,                 // 0x17
    0,                 // 0x18
    0,                 // 0x19
    0,                 // 0x1A
    KEY_ESCAPE,        // 0x1B Escape
    0,                 // 0x1C
    0,                 // 0x1D
    0,                 // 0x1E
    0,                 // 0x1F

    KEY_SPACE,         // 0x20
    KEY_1,             // 0x21 !
    KEY_APOSTROPHE,    // 0x22 "
    KEY_3,             // 0x23 #
    KEY_4,             // 0x24 $
    KEY_5,             // 0x25 %
    KEY_7,             // 0x26 &
    KEY_APOSTROPHE,    // 0x27 '
    KEY_9,             // 0x28 (
    KEY_0,             // 0x29 )
    KEY_8,             // 0x2A *
    KEY_EQUAL,         // 0x2B +
    KEY_COMMA,         // 0x2C ,
    KEY_MINUS,         // 0x2D -
    KEY_PERIOD,        // 0x2E .
    KEY_SLASH,         // 0x2F /
    KEY_0,             // 0x30 0
    KEY_1,             // 0x31 1
    KEY_2,             // 0x32 2
    KEY_3,             // 0x33 3
    KEY_4,             // 0x34 4
    KEY_5,             // 0x35 5
    KEY_6,             // 0x36 6
    KEY_7,             // 0x37 7
    KEY_8,             // 0x38 8
    KEY_9,             // 0x39 9
    KEY_SEMICOLON,     // 0x3A :
    KEY_SEMICOLON,     // 0x3B ;
    KEY_COMMA,         // 0x3C <
    KEY_EQUAL,         // 0x3D =
    KEY_PERIOD,        // 0x3E >
    KEY_SLASH,         // 0x3F ?

    KEY_2,             // 0x40 @
    KEY_A,             // 0x41 A
    KEY_B,             // 0x42 B
    KEY_C,             // 0x43 C
    KEY_D,             // 0x44 D
    KEY_E,             // 0x45 E
    KEY_F,             // 0x46 F
    KEY_G,             // 0x47 G
    KEY_H,             // 0x48 H
    KEY_I,             // 0x49 I
    KEY_J,             // 0x4A J
    KEY_K,             // 0x4B K
    KEY_L,             // 0x4C L
    KEY_M,             // 0x4D M
    KEY_N,             // 0x4E N
    KEY_O,             // 0x4F O
    KEY_P,             // 0x50 P
    KEY_Q,             // 0x51 Q
    KEY_R,             // 0x52 R
    KEY_S,             // 0x53 S
    KEY_T,             // 0x55 T
    KEY_U,             // 0x55 U
    KEY_V,             // 0x56 V
    KEY_W,             // 0x57 W
    KEY_X,             // 0x58 X
    KEY_Y,             // 0x59 Y
    KEY_Z,             // 0x5A Z
    KEY_BRACKET_LEFT,  // 0x5B [
    KEY_BACKSLASH,     // 0x5C '\'
    KEY_BRACKET_RIGHT, // 0x5D ]
    KEY_6,             // 0x5E ^
    KEY_MINUS,         // 0x5F _

    KEY_GRAVE,         // 0x60 `
    KEY_A,             // 0x61 a
    KEY_B,             // 0x62 b
    KEY_C,             // 0x63 c
    KEY_D,             // 0x66 d
    KEY_E,             // 0x65 e
    KEY_F,             // 0x66 f
    KEY_G,             // 0x67 g
    KEY_H,             // 0x68 h
    KEY_I,             // 0x69 i
    KEY_J,             // 0x6A j
    KEY_K,             // 0x6B k
    KEY_L,             // 0x6C l
    KEY_M,             // 0x6D m
    KEY_N,             // 0x6E n
    KEY_O,             // 0x6F o
    KEY_P,             // 0x70 p
    KEY_Q,             // 0x71 q
    KEY_R,             // 0x72 r
    KEY_S,             // 0x73 s
    KEY_T,             // 0x75 t
    KEY_U,             // 0x75 u
    KEY_V,             // 0x76 v
    KEY_W,             // 0x77 w
    KEY_X,             // 0x78 x
    KEY_Y,             // 0x79 y
    KEY_Z,             // 0x7A z
    KEY_BRACKET_LEFT,  // 0x7B {
    KEY_BACKSLASH,     // 0x7C |
    KEY_BRACKET_RIGHT, // 0x7D }
    KEY_GRAVE,         // 0x7E ~
    KEY_DELETE         // 0x7F Delete
};

uint8 usbHidKeyCodeFromAsciiChar(char asciiChar)
{
    if (asciiChar & 0x80){ return 0; }
    return hidKeyCode[asciiChar];
}
