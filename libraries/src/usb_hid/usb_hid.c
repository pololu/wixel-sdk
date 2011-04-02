#include <usb.h>
#include <board.h>           // just for boardStartBootloader() and serialNumberString

#define HID_DATA_ENDPOINT             1
#define HID_IN_PACKET_SIZE            8
#define HID_KEYBOARD_INTERFACE_NUMBER 0
#define HID_MOUSE_INTERFACE_NUMBER    1

#define HID_KEYBOARD_ENDPOINT         1
#define HID_KEYBOARD_FIFO             USBF1   // This must match HID_KEYBOARD_ENDPOINT!

#define HID_MOUSE_ENDPOINT            2
#define HID_MOUSE_FIFO                USBF2   // This must match HID_MOUSE_ENDPOINT!

/* HID Constants ******************************************************/

// USB Class Codes
#define DEVICE_CLASS 0
#define HID_CLASS    3          // (HID 1.11 Section 4.1: The HID Class).

// USB Subclass Codes
#define HID_SUBCLASS_BOOT 1     // (HID 1.11 Section 4.2: Subclass).

// USB Protocol Codes
#define HID_PROTOCOL_KEYBOARD 1 // (HID 1.11 Section 4.3: Protocols).
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
#define HID_COUNT           0x95
#define HID_SIZE            0x75
#define HID_USAGE_MIN       0x19
#define HID_USAGE_MAX       0x29
#define HID_LOGICAL_MIN     0x15
#define HID_LOGICAL_MAX     0x25
#define HID_INPUT           0x81
#define HID_OUTPUT          0x91

// HID Report Usage Pages from HID Usage Tables 1.11 Section 3
#define HID_USAGE_PAGE_GENERIC_DESKTOP 1
#define HID_USAGE_PAGE_KEY_CODES       7
#define HID_USAGE_PAGE_LEDS            8
#define HID_USAGE_PAGE_BUTTONS         9

// HID Report Usages from HID Usage Tables 1.11 Section 4
#define HID_USAGE_POINTER  1
#define HID_USAGE_MOUSE    2
#define HID_USAGE_KEYBOARD 6
#define HID_USAGE_X        30
#define HID_USAGE_Y        31
#define HID_USAGE_WHEEL    38

// HID Report Collection Types from HID 1.11 6.2.2.6
#define HID_COLLECTION_PHYSICAL    0
#define HID_COLLECTION_APPLICATION 1

// HID Input/Output/Feature Item Data (attributes) from HID 1.11 6.2.2.5
#define HID_ITEM_CONSTANT 0x1
#define HID_ITEM_VARIABLE 0x2
#define HID_ITEM_RELATIVE 0x4

/* HID USB Descriptors ****************************************************/

USB_DESCRIPTOR_DEVICE CODE usbDeviceDescriptor =
{
    sizeof(USB_DESCRIPTOR_DEVICE),
    USB_DESCRIPTOR_TYPE_DEVICE,
    0x0200,                 // USB Spec Release Number in BCD format
    DEVICE_CLASS,           // Class Code: undefined (use class code info from Interface Descriptors)
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
CODE uint8 keyboardReportDescriptor[]
=
{
    HID_USAGE_PAGE, HID_USAGE_PAGE_GENERIC_DESKTOP,
    HID_USAGE, HID_USAGE_KEYBOARD,
    HID_COLLECTION, HID_COLLECTION_APPLICATION,

        HID_COUNT, 8,                               // 8 Modifier Keys
        HID_SIZE, 1,
        HID_USAGE_PAGE, HID_USAGE_PAGE_KEY_CODES,
        HID_USAGE_MIN, 224,                         // Left Control
        HID_USAGE_MAX, 231,                         // Right GUI (Windows key)
        HID_LOGICAL_MIN, 0,
        HID_LOGICAL_MAX, 1,
        HID_INPUT, HID_ITEM_VARIABLE,

        HID_COUNT, 1,                               // Reserved (1 byte)
        HID_SIZE, 8,
        HID_INPUT, HID_ITEM_CONSTANT,

        HID_COUNT, 5,                               // 5 LEDs
        HID_SIZE, 1,
        HID_USAGE_PAGE, HID_USAGE_PAGE_LEDS,
        HID_USAGE_MIN, 1,                           // Num Lock
        HID_USAGE_MAX, 5,                           // Kana
        HID_OUTPUT, HID_ITEM_VARIABLE,

        HID_COUNT, 1,                               // Padding (3 bits)
        HID_SIZE, 3,
        HID_OUTPUT, HID_ITEM_CONSTANT,

        HID_COUNT, 6,                               // 6 Key Codes
        HID_SIZE, 8,
        HID_USAGE_PAGE, HID_USAGE_PAGE_KEY_CODES,
        HID_USAGE_MIN, 0,
        HID_USAGE_MAX, 255,
        HID_LOGICAL_MIN, 0,
        HID_LOGICAL_MAX, 255,
        HID_INPUT, 0,

    HID_END_COLLECTION,
};

// mouse report descriptor
// HID 1.11 Section 6.2.2: Report Descriptor
CODE uint8 mouseReportDescriptor[]
=
{
    HID_USAGE_PAGE, HID_USAGE_PAGE_GENERIC_DESKTOP,
    HID_USAGE, HID_USAGE_MOUSE,
    HID_COLLECTION, HID_COLLECTION_APPLICATION,

        HID_USAGE, HID_USAGE_POINTER,
        HID_COLLECTION, HID_COLLECTION_PHYSICAL,

            HID_COUNT, 5,                                   // 5 Mouse Buttons
            HID_SIZE, 1,
            HID_USAGE_PAGE, HID_USAGE_PAGE_BUTTONS,
            HID_USAGE_MIN, 1,
            HID_USAGE_MAX, 5,
            HID_LOGICAL_MIN, 0,
            HID_LOGICAL_MAX, 1,
            HID_INPUT, HID_ITEM_VARIABLE,

            HID_COUNT, 1,                                   // Padding (3 bits)
            HID_SIZE, 3,
            HID_OUTPUT, HID_ITEM_CONSTANT,

            HID_COUNT, 3,                                   // 3 Axes (X, Y, wheel)
            HID_SIZE, 8,
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
    uint8 keyboard_class_specific[9];  // HID-Specific Descriptors
    struct USB_DESCRIPTOR_ENDPOINT keyboard_in;

    struct USB_DESCRIPTOR_INTERFACE mouse_interface;
    uint8 mouse_class_specific[9];  // HID-Specific Descriptors
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
    {                                                    // Functional Descriptors.
        9,                                               // 9-byte HID Descriptor
        HID_DESCRIPTOR_TYPE_HID,
        0x11,0x01,                                       // bcdHID.  We conform to HID 1.11.
        HID_COUNTRY_NOT_LOCALIZED,                       // bCountryCode
        1,                                               // bNumDescriptors
        HID_DESCRIPTOR_TYPE_REPORT,                      // bDescriptorType
        sizeof(keyboardReportDescriptor)                 // wDescriptorLength
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
    {                                                    // Functional Descriptors.
        9,                                               // 9-byte HID Descriptor
        HID_DESCRIPTOR_TYPE_HID,
        0x11,0x01,                                       // bcdHID.  We conform to HID 1.11.
        HID_COUNTRY_NOT_LOCALIZED,                       // bCountryCode
        1,                                               // bNumDescriptors
        HID_DESCRIPTOR_TYPE_REPORT,                      // bDescriptorType
        sizeof(mouseReportDescriptor)                    // wDescriptorLength
    },
    {                                                    // Mouse IN Endpoint
        sizeof(struct USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_OUT | HID_MOUSE_ENDPOINT,   // bEndpointAddress
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

/* HID USB callbacks ******************************************************/
// These functions are called by the low-level USB module (usb.c) when a USB
// event happens that requires higher-level code to make a decision.

void usbCallbackInitEndpoints(void)
{
    usbInitEndpointIn(HID_KEYBOARD_ENDPOINT, HID_IN_PACKET_SIZE);
    usbInitEndpointIn(HID_MOUSE_ENDPOINT, HID_IN_PACKET_SIZE);
}

// Implements all the control transfers that are required by D1 of the
// ACM descriptor bmCapabilities, (USBPSTN1.20 Table 4).
void usbCallbackSetupHandler(void)
{
    if ((usbSetupPacket.bmRequestType & 0x7F) != 0x21)   // Require Type==Class and Recipient==Interface.
        return;

    if (!(usbSetupPacket.wIndex == HID_KEYBOARD_INTERFACE_NUMBER || usbSetupPacket.wIndex == HID_MOUSE_INTERFACE_NUMBER))
        return;

    /*switch(usbSetupPacket.bRequest)
    {
        case ACM_REQUEST_SET_LINE_CODING:                          // SetLineCoding (USBPSTN1.20 Section 6.3.10 SetLineCoding)
            usbControlWrite(sizeof(usbComLineCoding), (uint8 XDATA *)&usbComLineCoding);
            break;

        case ACM_REQUEST_GET_LINE_CODING:                          // GetLineCoding (USBPSTN1.20 Section 6.3.11 GetLineCoding)
            usbControlRead(sizeof(usbComLineCoding), (uint8 XDATA *)&usbComLineCoding);
            break;

        case ACM_REQUEST_SET_CONTROL_LINE_STATE:                   // SetControlLineState (USBPSTN1.20 Section 6.3.12 SetControlLineState)
            usbComControlLineState = usbSetupPacket.wValue;
            usbControlAcknowledge();
            break;
    }*/
}

void usbCallbackClassDescriptorHandler(void)
{
    if (usbSetupPacket.bmRequestType != 0x81)   // Require Direction==Device-to-Host, Type==Standard, and Recipient==Interface. (HID 1.11 Section 7.1.1)
        return;

    if (!(usbSetupPacket.wIndex == HID_KEYBOARD_INTERFACE_NUMBER || usbSetupPacket.wIndex == HID_MOUSE_INTERFACE_NUMBER))
        return;


}

void usbCallbackControlWriteHandler(void)
{
}
