#include <cc2511_map.h>
#include <cc2511_types.h>
#include <usb.h>
#include <usb_com.h>
#include <board.h>           // just for boardStartBootloader() and serialNumberString

/* CDC ACM Library Configuration **********************************************/
// Note: USB 2.0 says that the maximum packet size for full-speed bulk endpoints
// can only be 8, 16, 32, or 64 bytes.
// We picked endpoint 4 for the data because it has a 256-byte FIFO memory area,
// which is exactly enough for us to have two 64-byte IN buffers and two 64-byte
// OUT buffers.

#define CDC_OUT_PACKET_SIZE          64
#define CDC_IN_PACKET_SIZE           64
#define CDC_CONTROL_INTERFACE_NUMBER 0
#define CDC_DATA_INTERFACE_NUMBER    1

#define CDC_NOTIFICATION_ENDPOINT    1

#define CDC_DATA_ENDPOINT            4
#define CDC_DATA_FIFO                USBF4   // This must match CDC_DATA_ENDPOINT!

/* CDC and ACM Constants ******************************************************/

// USB Class Codes
#define CDC_CLASS 2                  // (CDC 1.20 Section 4.1: Communications Device Class Code).
#define CDC_DATA_INTERFACE_CLASS 0xA // (CDC 1.20 Section 4.5: Data Class Interface Codes).

// USB Subclass Codes
#define CDC_SUBCLASS_ACM  2           // (CDC 1.20 Section 4.3: Communications Class Subclass Codes).  Refer to USBPSTN1.2.

// USB Protocol Codes
#define CDC_PROTOCOL_V250 1          // (CDC 1.20 Section 4.4: Communications Class Protocol Codes).

// USB Descriptor types from CDC 1.20 Section 5.2.3, Table 12
#define CDC_DESCRIPTOR_TYPE_CS_INTERFACE 0x24
#define CDC_DESCRIPTOR_TYPE_CS_ENDPOINT  0x25

// USB Descriptor sub-types from CDC 1.20 Table 13: bDescriptor SubType in Communications Class Functional Descriptors
#define CDC_DESCRIPTOR_SUBTYPE_HEADER                       0
#define CDC_DESCRIPTOR_SUBTYPE_CALL_MANAGEMENT              1
#define CDC_DESCRIPTOR_SUBTYPE_ABSTRACT_CONTROL_MANAGEMENT  2
#define CDC_DESCRIPTOR_SUBTYPE_UNION                        6

// Request Codes from CDC 1.20 Section 6.2: Management Element Requests.
#define ACM_GET_ENCAPSULATED_RESPONSE 0
#define ACM_SEND_ENCAPSULATED_COMMAND 1

// Request Codes from PSTN 1.20 Table 13.
#define ACM_REQUEST_SET_LINE_CODING 0x20
#define ACM_REQUEST_GET_LINE_CODING 0x21
#define ACM_REQUEST_SET_CONTROL_LINE_STATE 0x22

// Notification Codes from PSTN 1.20 Table 30.
#define ACM_NOTIFICATION_RESPONSE_AVAILABLE 0x01
#define ACM_NOTIFICATION_SERIAL_STATE 0x20

/* USB COM Variables **********************************************************/

uint8 usbComControlLineState = 0xFF;

ACM_LINE_CODING XDATA usbComLineCoding =
{
    9600,     // dwDTERate (baud rate)
    0,        // bCharFormat = 0: 1 stop bit
    0,        // bParityType = 0: no parity
    8,        // bDataBits = 8
};

// This bit is true if we need to send an empty (zero-length) packet of data to
// the computer soon.  Every data transfer needs to be ended with a packet that
// is less than full length, so sometimes we need to send empty packets.
static BIT sendEmptyPacketSoon = 0;

// The number of bytes that we have loaded in to the IN FIFO that are NOT yet
// queued up to be sent.  This will always be less than CDC_IN_PACKET_SIZE because
// once we've loaded up a full packet we should always send it immediately.
static uint8 DATA inFifoBytesLoaded = 0;

/* CDC ACM USB Descriptors ****************************************************/

USB_DESCRIPTOR_DEVICE CODE usbDeviceDescriptor =
{
    sizeof(USB_DESCRIPTOR_DEVICE),
    USB_DESCRIPTOR_TYPE_DEVICE,
    0x0200,                 // USB Spec Release Number in BCD format
    CDC_CLASS,              // Class Code: Communications Device Class
    CDC_SUBCLASS_ACM,       // Subclass code: ACM
    CDC_PROTOCOL_V250,
    USB_EP0_PACKET_SIZE,    //  Max packet size for Endpoint 0
    USB_VENDOR_ID_POLOLU,   //  Vendor ID
    0x2200,                 //  Product ID (Generic Wixel with one CDC ACM port)
    0x0100,                 //  Device release number in BCD format
    1,                      //  Index of Manufacturer String Descriptor
    2,                      //  Index of Product String Descriptor
    3,                      //  Index of Serial Number String Descriptor
    1                       //  Number of possible configurations.
};

CODE struct CONFIG1 {
    struct USB_DESCRIPTOR_CONFIGURATION configuration;

    struct USB_DESCRIPTOR_INTERFACE communication_interface;
    unsigned char class_specific[19];  // CDC-Specific Descriptors
    struct USB_DESCRIPTOR_ENDPOINT notification_element;

    struct USB_DESCRIPTOR_INTERFACE data_interface;
    struct USB_DESCRIPTOR_ENDPOINT data_out;
    struct USB_DESCRIPTOR_ENDPOINT data_in;
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
    {                                                    // Communications Interface: Used for device management.
        sizeof(struct USB_DESCRIPTOR_INTERFACE),
        USB_DESCRIPTOR_TYPE_INTERFACE,
        CDC_CONTROL_INTERFACE_NUMBER,                    // bInterfaceNumber
        0,                                               // bAlternateSetting
        1,                                               // bNumEndpoints
        CDC_CLASS,                                       // bInterfaceClass
        CDC_SUBCLASS_ACM,                                // bInterfaceSubClass
        CDC_PROTOCOL_V250,                               // bInterfaceProtocol
        0                                                // iInterface
    },
    {                                                    // Functional Descriptors.

        5,                                               // 5-byte General Descriptor: Header Functional Descriptor
        CDC_DESCRIPTOR_TYPE_CS_INTERFACE,
        CDC_DESCRIPTOR_SUBTYPE_HEADER,
        0x20,0x01,                                       // bcdCDC.  We conform to CDC 1.20.


        4,                                               // 4-byte PTSN-Specific Descriptor: Abstract Control Management Functional Descriptor.
        CDC_DESCRIPTOR_TYPE_CS_INTERFACE,
        CDC_DESCRIPTOR_SUBTYPE_ABSTRACT_CONTROL_MANAGEMENT,
        2,                                               // bmCapabilities.  See USBPSTN1.2 Table 4.  We support SetLineCoding,
                                                         //SetControlLineState, GetLineCoding, and SerialState notifications.

        5,                                               // 5-byte General Descriptor: Union Interface Functional Descriptor (CDC 1.20 Table 16).
        CDC_DESCRIPTOR_TYPE_CS_INTERFACE,
        CDC_DESCRIPTOR_SUBTYPE_UNION,
        CDC_CONTROL_INTERFACE_NUMBER,                    // index of the control interface
        CDC_DATA_INTERFACE_NUMBER,                       // index of the subordinate interface

        5,                                               // 5-byte PTSN-Specific Descriptor
        CDC_DESCRIPTOR_TYPE_CS_INTERFACE,
        CDC_DESCRIPTOR_SUBTYPE_CALL_MANAGEMENT,
        0x00,                                            // bmCapabilities.  USBPSTN1.2 Table 3.  Device does not handle call management.
        CDC_DATA_INTERFACE_NUMBER                        // index of the data interface
    },
    {
        sizeof(struct USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_IN | CDC_NOTIFICATION_ENDPOINT,  // bEndpointAddress
        USB_TRANSFER_TYPE_INTERRUPT,                     // bmAttributes
        8,                                               // wMaxPacketSize
        1,                                               // bInterval
    },
    {
        sizeof(struct USB_DESCRIPTOR_INTERFACE),         // Data Interface: used for RX and TX data.
        USB_DESCRIPTOR_TYPE_INTERFACE,
        CDC_DATA_INTERFACE_NUMBER,                       // bInterfaceNumber
        0,                                               // bAlternateSetting
        2,                                               // bNumEndpoints
        CDC_DATA_INTERFACE_CLASS,                        // bInterfaceClass
        0,                                               // bInterfaceSubClass
        0,                                               // bInterfaceProtocol
        0                                                // iInterface
    },
    {                                                    // OUT Endpoint: Sends data out to Wixel.
        sizeof(struct USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_OUT | CDC_DATA_ENDPOINT,    // bEndpointAddress
        USB_TRANSFER_TYPE_BULK,                          // bmAttributes
        CDC_OUT_PACKET_SIZE,                             // wMaxPacketSize
        0,                                               // bInterval
    },
    {
        sizeof(struct USB_DESCRIPTOR_ENDPOINT),
        USB_DESCRIPTOR_TYPE_ENDPOINT,
        USB_ENDPOINT_ADDRESS_IN | CDC_DATA_ENDPOINT,     // bEndpointAddress
        USB_TRANSFER_TYPE_BULK,                          // bmAttributes
        CDC_IN_PACKET_SIZE,                              // wMaxPacketSize
        0,                                               // bInterval
    },
};

uint8 CODE usbStringDescriptorCount = 4;
DEFINE_STRING_DESCRIPTOR(languages, 1, USB_LANGUAGE_EN_US)
DEFINE_STRING_DESCRIPTOR(manufacturer, 18, 'P','o','l','o','l','u',' ','C','o','r','p','o','r','a','t','i','o','n')
DEFINE_STRING_DESCRIPTOR(product, 5, 'W','i','x','e','l')
uint16 CODE * CODE usbStringDescriptors[] = { languages, manufacturer, product, serialNumberStringDescriptor };

/* CDC ACM USB callbacks ******************************************************/
// These functions are called by the low-level USB module (usb.c) when a USB
// event happens that requires higher-level code to make a decision.

void usbCallbackInitEndpoints()
{
    usbInitEndpointIn(CDC_NOTIFICATION_ENDPOINT, 8);
    usbInitEndpointOut(CDC_DATA_ENDPOINT, CDC_OUT_PACKET_SIZE);
    usbInitEndpointIn(CDC_DATA_ENDPOINT, CDC_IN_PACKET_SIZE);
}

// Implements all the control transfers that are required by D1 of the
// ACM descriptor bmCapabilities, (USBPSTN1.20 Table 4).
void usbCallbackSetupHandler()
{
    if ((usbSetupPacket.bmRequestType & 0x7F) != 0x21)   // Require Type==Class and Recipient==Interface.
        return;

    if (!(usbSetupPacket.wIndex == CDC_CONTROL_INTERFACE_NUMBER || usbSetupPacket.wIndex == CDC_DATA_INTERFACE_NUMBER))
        return;

    switch(usbSetupPacket.bRequest)
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
    }
}

void usbCallbackControlWriteHandler()
{

}

/* CDC ACM RX Functions *******************************************************/
// These functions can be called by the higher-level user of the CDC ACM library
// to receive bytes from the computer.

uint8 usbComRxAvailable()
{
    USBINDEX = CDC_DATA_ENDPOINT;      // Select the data endpoint.
    if (USBCSOL & USBCSOL_OUTPKT_RDY)  // Check the OUTPKT_RDY flag because USBCNTL is only valid when it is 1.
    {
        // Assumption: We don't need to read USBCNTH because we can't receive packets
        // larger than 255 bytes.
        return USBCNTL;
    }
    else
    {
        return 0;
    }
}


// Assumption: We don't need to read USBCNTH because we can't receive packets
// larger than 255 bytes.
// Assumption: The user has previously called usbComRxAvailable and its return value
// was non-zero.
uint8 usbComRxReceiveByte()
{
    uint8 tmp;

    USBINDEX = CDC_DATA_ENDPOINT;         // Select the CDC data endpoint.
    tmp = CDC_DATA_FIFO;                  // Read one byte from the FIFO.

    if (USBCNTL == 0)                     // If there are no bytes left in this packet...
    {
        USBCSOL &= ~USBCSOL_OUTPKT_RDY;   // Tell the USB module we are done reading this packet, so it can receive more.
    }
    return tmp;
}

// Assumption: The user has previously called usbComRxAvailable and its return value
// was greater than or equal to size.
uint8 usbComRxReceiveNonBlocking(uint8 XDATA* buffer, uint8 size)
{
    uint8 bytesToGet;
    bytesToGet = usbComRxAvailable();
    if (bytesToGet >= size)
    {
        bytesToGet = size;
    }
    if (bytesToGet == 0)
    {
        return 0;
    }

    usbReadFifo(CDC_DATA_ENDPOINT, bytesToGet, buffer);

    if (USBCNTL == 0)
    {
        USBCSOL &= ~USBCSOL_OUTPKT_RDY;   // Tell the USB module we are done reading this packet, so it can receive more.
    }

    return bytesToGet;
}

void usbComRxReceive(const uint8 XDATA * buffer, uint8 size);


/* CDC ACM TX Functions *******************************************************/
// These functions can be called by the higher-level user of the CDC ACM library
// to send bytes to the computer.

static void sendPacketNow()
{
    USBINDEX = CDC_DATA_ENDPOINT;
    USBCSIL |= USBCSIL_INPKT_RDY;                      // Send the packet.

    // If the last packet transmitted was a full packet, we should send an empty packet later.
    sendEmptyPacketSoon = (inFifoBytesLoaded == CDC_IN_PACKET_SIZE);
    inFifoBytesLoaded = 0;
}

void usbComService(void)
{
    USBINDEX = CDC_DATA_ENDPOINT;

    // Send a packet now if there is data loaded in the FIFO waiting to be sent OR
    //
    // Typical USB systems wait for a short or empty packet before forwarding the data
    // up to the software that requested it, so this is necessary.  However, we only transmit
    // an empty packet if there are no packets currently loaded in the FIFO.
    if (inFifoBytesLoaded || ( sendEmptyPacketSoon && !(USBCSIL & USBCSIL_PKT_PRESENT) ) )
    {
        sendPacketNow();
    }

    usbPoll();

    if (usbComLineCoding.dwDTERate == 333)
    {
        // TODO: we should wait for 100-500 ms before actually going in to bootloader mode,
        // so that the computer can do a successful GetLineCoding request and SetCommState
        // does not return an error code.

        // The baud rate has been set to 333.  That is the special signal
        // sent by the USB host telling us to enter bootloader mode.
        boardStartBootloader();
    }
}

// Assumption: We are using double buffering, so we can load either 0, 1, or 2
// packets in to the FIFO at this time.
uint8 usbComTxAvailable()
{
    uint8 tmp;
    USBINDEX = CDC_DATA_ENDPOINT;
    tmp = USBCSIL;
    if (tmp & USBCSIL_PKT_PRESENT)
    {
        if (tmp & USBCSIL_INPKT_RDY)
        {
            return 0;                                       // 2 packets are in the FIFO, so no room
        }
        return CDC_IN_PACKET_SIZE - inFifoBytesLoaded;      // 1 packet is in the FIFO, so there is room for 1 more
    }
    else
    {
        return (CDC_IN_PACKET_SIZE<<1) - inFifoBytesLoaded; // 0 packets are in the FIFO, so there is room for 2 more
    }
}

// Assumption: The user called usbComTxAvailable() before calling this function,
// and it returned a number greater than or equal to size.
// TIMING: David did an experiment on 2011-1-3.  This function took 25us to run when size==8 and 250us when size
void usbComTxSendNonBlocking(const uint8 XDATA * buffer, uint8 size)
{
    uint8 packetSize;
    while(size)
    {
        packetSize = CDC_IN_PACKET_SIZE - inFifoBytesLoaded;   // Decide how many bytes to send in this packet (packetSize).
        if (packetSize > size){ packetSize = size; }

        usbWriteFifo(CDC_DATA_ENDPOINT, packetSize, buffer);    // Write those bytes to the USB FIFO.

        buffer += packetSize;                                   // Update pointers.
        size -= packetSize;
        inFifoBytesLoaded += packetSize;

        if (inFifoBytesLoaded == CDC_IN_PACKET_SIZE)
        {
            sendPacketNow();
        }
    }
}

// For use with printf.
void usbComTxSendByte(uint8 byte)
{
    while(usbComTxAvailable() == 0)
    {
        // TODO: call any background tasks that need to be called?
    }

    CDC_DATA_FIFO = byte;                          // Give the byte to the USB module's FIFO.
    inFifoBytesLoaded++;

    if (inFifoBytesLoaded == CDC_IN_PACKET_SIZE)
    {
        sendPacketNow();
    }
}

void usbComTxSend(const uint8 XDATA * buffer, uint8 size)
{
    uint8 bytesToSend;
    while(size)
    {
        bytesToSend = usbComTxAvailable();                 // Determine how many bytes we can send in the next call.
        if (bytesToSend == 0){ continue; }                 // Skip the rest of the loop if we can't send any bytes right now.
        if (bytesToSend > size){ bytesToSend = size; }     // Don't send more bytes than we need to.

        usbComTxSendNonBlocking(buffer, size);
        buffer += bytesToSend;
        size -= bytesToSend;
    }
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
