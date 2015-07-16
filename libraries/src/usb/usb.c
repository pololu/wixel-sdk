#include <usb.h>
#include <cc2511_map.h>
#include <cc2511_types.h>
#include <board.h>

// TODO: make the usb library work will with Sleep Mode 0 (an interrupt should be enabled for all the endpoints we care about so we can handle them quickly)
// TODO: SUSPEND MODE!

extern uint8 CODE usbConfigurationDescriptor[];

static void usbStandardDeviceRequestHandler();

#define CONTROL_TRANSFER_STATE_NONE  0
#define CONTROL_TRANSFER_STATE_WRITE 1
#define CONTROL_TRANSFER_STATE_READ  2

USB_SETUP_PACKET XDATA usbSetupPacket;
uint8 XDATA usbDeviceState = USB_STATE_DETACHED;

uint8 XDATA controlTransferState = CONTROL_TRANSFER_STATE_NONE;
uint16 XDATA controlTransferBytesLeft;
XDATA uint8 * controlTransferPointer;

volatile BIT usbSuspendMode = 0;

// TODO: eventually: Enable the USB interrupt and only set usbActivityFlag in the ISR
volatile BIT usbActivityFlag = 0;

void usbInit()
{
}

// TODO: try using DMA in usbReadFifo and usbWriteFifo and see how that affects the speed of usbComTxSend(x, 128).
void usbReadFifo(uint8 endpointNumber, uint8 count, uint8 XDATA * buffer)
{
    XDATA uint8 * fifo = (XDATA uint8 *)(0xDE20 + (uint8)(endpointNumber<<1));
    while(count > 0)
    {
        count--;
        *(buffer++) = *fifo;
    }

    usbActivityFlag = 1;
}

void usbWriteFifo(uint8 endpointNumber, uint8 count, const uint8 XDATA * buffer)
{
    XDATA uint8 * fifo = (XDATA uint8 *)(0xDE20 + (uint8)(endpointNumber<<1));
    while(count > 0)
    {
        count--;
        *fifo = *(buffer++);
    }

    // We don't set the usbActivityFlag here; we wait until the packet is
    // actually sent.
}

// Performs some basic tasks that should be done after USB is connected and after every
// Reset interrupt.
static void basicUsbInit()
{
    usbSuspendMode = 0;

    // Enable suspend detection and disable any other weird features.
    USBPOW = 1;

    // Enable the USB common interrupts we care about: Reset, Resume, Suspend.
    // Without this, we USBCIF.SUSPENDIF will not get set (the datasheet is incomplete).
    USBCIE = 0b0111;
}

void usbPoll()
{
    uint8 usbcif;
    uint8 usbiif;
    //uint8 usboif = USBOIF;

    if (!usbPowerPresent())
    {
        // The VBUS line is low.  This usually means that the USB cable has been
        // disconnected or the computer has been turned off.

        SLEEP &= ~(1<<7); // Disable the USB module (SLEEP.USB_EN = 0).

        disableUsbPullup();
        usbDeviceState = USB_STATE_DETACHED;
        usbSuspendMode = 0;
        return;
    }

    if (usbDeviceState == USB_STATE_DETACHED)
    {
        enableUsbPullup();
        SLEEP |= (1<<7);            // Enable the USB module (SLEEP.USB_EN = 1).
        __asm nop __endasm;         // Datasheet doesn't say so, but David suspects we need some NOPs here before writing to USB registers.
        __asm nop __endasm;
        __asm nop __endasm;
        __asm nop __endasm;
        usbDeviceState = USB_STATE_POWERED;

        basicUsbInit();
    }

    usbcif = USBCIF;
    usbiif = USBIIF;

    if (usbcif & (1<<0)) // Check SUSPENDIF
    {
        // The bus has been idle for 3 ms, so we are now in Suspend mode.
        // It is the user's responsibility to check usbSuspended() and go to sleep (PM1)
        // if necessary.
        usbSuspendMode = 1;
    }

    if (usbcif & (1<<2))  // check RSTIF, the reset flag
    {
        // A USB reset signal has been received.
        usbDeviceState = USB_STATE_DEFAULT;
        controlTransferState = CONTROL_TRANSFER_STATE_NONE;

        basicUsbInit();
    }

    if (usbcif & (1<<1)) // Check RESUMEIF
    {
        usbSuspendMode = 0;
    }

    if (usbiif & (1<<0)) // Check EP0IF
    {
        // Something happened on Endpoint 0, the endpoint for control transfers.
        uint8 usbcs0;
        USBINDEX = 0;
        usbcs0 = USBCS0;

        usbActivityFlag = 1;

        if (usbcs0 & (1<<4)) // Check SETUP_END
        {
            // A new setup packet has arrived, prematurely ending the previous control transfer.
            USBCS0 = 0x80; // Clear the SETUP_END bit
            controlTransferState = CONTROL_TRANSFER_STATE_NONE;
        }

        if (usbcs0 & (1<<2))  // Check SENT_STALL
        {
            // A STALL packet was sent
            USBCS0 = 0x00;  // Reset endpoint 0.
            controlTransferState = CONTROL_TRANSFER_STATE_NONE;
        }

        if (usbcs0 & (1<<0))  // Check OUTPKT_RDY
        {
            // Requirement: Every codepath from here must result in writing a 1 to
            // bit 6 of USBCS0 to clear the OUTPKT_RDY flag: USBCS0 = (1<<6).

            if (controlTransferState == CONTROL_TRANSFER_STATE_WRITE)
            {
                // A data packet has been received as part of a control write transfer.
                uint8 bytesReceived = USBCNT0;
                if (bytesReceived > controlTransferBytesLeft)
                {
                    bytesReceived = controlTransferBytesLeft;
                }
                usbReadFifo(0, bytesReceived, controlTransferPointer);
                controlTransferPointer += bytesReceived;
                controlTransferBytesLeft -= bytesReceived;

                if (controlTransferBytesLeft)
                {
                    // Arm the endpoint to receive more bytes
                    USBCS0 = (1<<6);  // De-asserts the OUTPKT_RDY bit (bit 0).
                }
                else
                {
                    // The host has sent all the data we were expecting.

                    if (usbSetupPacket.requestType != USB_REQUEST_TYPE_STANDARD) // OPT: remove this check
                    {
                        usbCallbackControlWriteHandler();
                    }

                    USBINDEX = 0;  // Just in case USBINDEX was changed above.

                    if (controlTransferState == CONTROL_TRANSFER_STATE_NONE)
                    {
                        // The data received was invalid.
                        USBCS0 = (1<<6) | (1<<3) | (1<<5); // clear OUTPKT_RDY, set DATA_END, SEND_STALL
                    }
                    else
                    {
                        // The data received was valid.
                        USBCS0 = (1<<6) | (1<<3); // clear OUTPKT_RDY, set DATA_END
                        controlTransferState = CONTROL_TRANSFER_STATE_NONE;
                    }
                }
            }
            else if (USBCNT0 == 8)
            {
                // A SETUP packet has been received from the computer, starting a new
                // control transfer.

                usbReadFifo(0, 8, (uint8 XDATA *)&usbSetupPacket); // Store the data in usbSetupPacket.

                // Wipe out the information about the last control transfer.
                controlTransferState = CONTROL_TRANSFER_STATE_NONE;

                if (usbSetupPacket.requestType == USB_REQUEST_TYPE_STANDARD)
                {
                    // The request_type field indicates that this is a Standard Device Request
                    // as described in USB2.0 Chapter 9.4 Standard Device Requests.
                    // These requests are handled by the library in the function below.
                    usbStandardDeviceRequestHandler();
                }
                else
                {
                    // Otherwise, we use this callback so the user can decide how to handle the
                    // setup packet.  In this callback, the user can call various helper
                    // functions that set controlTransferState.
                    usbCallbackSetupHandler();
                }

                USBINDEX = 0;  // Select EP0 again because the functions above might have changed USBINDEX.

                // Modify the count so that we don't send more data than the host requested.
                if(controlTransferBytesLeft > usbSetupPacket.wLength)
                {
                    controlTransferBytesLeft = usbSetupPacket.wLength;
                }

                // Prepare for the first transaction after the SETUP packet.
                if (controlTransferState == CONTROL_TRANSFER_STATE_NONE)
                {
                    // This is invalid/unrecognized control transfer, so send a STALL packet.
                    USBCS0 = (1<<6) | (1<<5); // Clears the OUTPKT_RDY flag because we've handled it, and sends a STALL.
                }
                else if (controlTransferState == CONTROL_TRANSFER_STATE_WRITE)
                {
                    if (controlTransferBytesLeft)
                    {
                        // Arm the endpoint to receive the first data packet of a control write transfer.
                        USBCS0 = (1<<6);  // De-asserts the OUTPKT_RDY bit.
                    }
                    else
                    {
                        // Acknowledge a control write transfer with no data phase.
                        USBCS0 = (1<<6) | (1<<3) | (1<<1);  // De-asserts OUTPKY_RDY, asserts DATA_END, asserts INPKT_RDY.
                        controlTransferState = CONTROL_TRANSFER_STATE_NONE;
                    }
                }
            }
            else
            {
                // An OUT packet was received on Endpoint 0, but we are not in the middle of a
                // control write transfer and it was not the right length to be a setup packet.
                // This situation is not expected.
                USBCS0 = (1<<6);  // De-asserts the OUTPKT_RDY.
            }
        }

        if (!(usbcs0 & (1<<1)) && (controlTransferState == CONTROL_TRANSFER_STATE_READ))
        {
            // We are doing a control read transfer, and Endpoint 0 is ready to accept another
            // IN packet to send to the computer.
            uint8 bytesToSend;
            if (controlTransferBytesLeft < USB_EP0_PACKET_SIZE)
            {
                // Send the last packet (might be an empty packet).
                usbcs0 = (1<<1)|(1<<3);  // INPKT_RDY and DATA_END
                bytesToSend = controlTransferBytesLeft;
                controlTransferState = CONTROL_TRANSFER_STATE_NONE;
            }
            else
            {
                // Send a packet.
                usbcs0 = (1<<1); // INPKT_RDY
                bytesToSend = USB_EP0_PACKET_SIZE;
            }

            // Arm endpoint 0 to send the next packet.
            usbWriteFifo(0, bytesToSend, controlTransferPointer);
            USBCS0 = usbcs0;

            // Update the control transfer state.
            controlTransferPointer += bytesToSend;
            controlTransferBytesLeft -= bytesToSend;
        }
    }
}

// usbStandardDeviceRequestHandler(): Implementation of USB2.0 Section 9.4, Standard Device Requests.
// This function gets called whenever we receive a SETUP packet on endpoint zero with the requestType
// field set to STANDARD.  This function reads the SETUP packet and uses that to set the control
// transfer state variables with all the information needed to respond to the request.
// Assumption: controlTransferState is CONTROL_TRANSFER_STATE_NONE when this function is called.
static void usbStandardDeviceRequestHandler()
{
    // Prepare a convenient two-byte buffer for sending 1 or 2 byte responses.
    static XDATA uint8 response[2];
    response[0] = 0;
    response[1] = 0;

    // Now we decide how to handle the new setup packet.  There are several possibilities:
    // * Invalid: The SETUP packet had a problem with it, or we don't support the feature,
    //     so we need to STALL the next transaction to indicate an request error to the host.
    // * Control Read:  We must send some data to the computer, so we need to decide
    //     where the data is coming from (address, plus RAM/ROM selection)
    // * Control Write with no data phase: We need to prepare for the status phase, where
    //     our device must send a zero-length EP0 IN packet to indicate success.
    // * Control Write with data phase: The computer will send data to us, and we need to
    //     decide where in RAM to put it.  (No standard device requests use this type,
    //     so ignore this case.)

    switch(usbSetupPacket.bRequest)
    {
        case USB_REQUEST_GET_DESCRIPTOR: // USB Spec 9.4.3 Get Descriptor
        {
            switch(usbSetupPacket.wValue >> 8)
            {
                case USB_DESCRIPTOR_TYPE_DEVICE:
                {
                    controlTransferPointer = (uint8 XDATA *)&usbDeviceDescriptor;
                    controlTransferBytesLeft = sizeof(USB_DESCRIPTOR_DEVICE);
                    break;
                }
                case USB_DESCRIPTOR_TYPE_CONFIGURATION:
                {
                    if ((usbSetupPacket.wValue & 0xFF) != 0)
                    {
                        // Invalid configuration index.
                        return;
                    }

                    // The configuration descriptor has an application-dependent size, which
                    // we determine by reading the 3rd and 4th byte.
                    controlTransferPointer = (uint8 XDATA *)usbConfigurationDescriptor;
                    controlTransferBytesLeft = *(uint16 *)&usbConfigurationDescriptor[2];
                    break;
                }
                case USB_DESCRIPTOR_TYPE_STRING:
                {
                    if ((usbSetupPacket.wValue & 0xFF) >= usbStringDescriptorCount)
                    {
                        // This is either an invalid string index or it is 0xEE,
                        // which is defined by Microsoft OS Descriptors 1.0.
                        // This library provides no features for handling such requests,
                        // but we call the user's callback in case they want to.
                        usbCallbackClassDescriptorHandler();
                        return;
                    }

                    controlTransferPointer = (uint8 XDATA *)usbStringDescriptors[usbSetupPacket.wValue & 0xFF];
                    controlTransferBytesLeft = controlTransferPointer[0];
                    break;
                }
                default:
                {
                    // see if the class recognizes the descriptor type; it should call usbControlRead if it does
                    usbCallbackClassDescriptorHandler();

                    if (controlTransferState == CONTROL_TRANSFER_STATE_NONE)
                    {
                        // unknown type of descriptor
                        return;
                    }
                    break;
                }
            }

            controlTransferState = CONTROL_TRANSFER_STATE_READ;
            return;
        }
        case USB_REQUEST_SET_ADDRESS: // USB Spec, 9.4.6 Set Address
        {
            // Get ready to set the address when the status phase is complete.
            // We always set the most siginificant bit, because .device_address might be 0
            // and that is a valid request, meaning we should revert to address 0.
            //pendingDeviceAddress = (usbSetupPacket.wValue & 0xFF) | 0x80;

            USBADDR = (uint8)usbSetupPacket.wValue;
            usbDeviceState = ((uint8)usbSetupPacket.wValue) ? USB_STATE_ADDRESS : USB_STATE_DEFAULT;

            // Get ready to provide a handshake.
            usbControlAcknowledge();
            return;
        }
        case USB_REQUEST_SET_CONFIGURATION: // USB Spec, 9.4.7 Set Configuration
        {
            // Assumption: there is only one configuration and its value is 1.
            switch(usbSetupPacket.wValue)
            {
                case 0:
                {
                    // We have been deconfigured.

                    // TODO: Add resetNonzeroEndpoints() and call it here.

                    if (usbDeviceState > USB_STATE_ADDRESS)
                    {
                        usbDeviceState = USB_STATE_ADDRESS;
                    }
                    break;
                }
                case 1:
                {
                    // The device has been configured.  This is normal operating
                    // state of a USB device.  We can now start using non-zero
                    // endpoints.
                    usbDeviceState = USB_STATE_CONFIGURED;
                    usbCallbackInitEndpoints();
                    break;
                }
                default:
                {
                    // Invalid configuration value, so STALL.
                    return;
                }
            }

            // Get ready to provide a handshake.
            usbControlAcknowledge();
            return;
        }
        case USB_REQUEST_GET_CONFIGURATION: // USB Spec 9.4.2 Get Configuration
        {
            // Assumption: there is only one configuration and its value is 1.
            response[0] = (usbDeviceState == USB_STATE_CONFIGURED) ? 1 : 0;
            usbControlRead(1, response);
            return;
        }
        case USB_REQUEST_GET_INTERFACE: // USB Spec 9.4.4 Get Interface
        {
            // Assumption: the "alternate setting number" of each interface
            //   is zero and there are no alternate settings.
            // Assumption: interface numbers go from 0 to
            //   config->interface_count-1, with no gaps.

            if (usbDeviceState < USB_STATE_CONFIGURED)
            {
                // Invalid request because we have not reached the configured state.
                return;
            }

            if (usbSetupPacket.wIndex >= ((USB_DESCRIPTOR_CONFIGURATION *)&usbConfigurationDescriptor)->bNumInterfaces)
            {
                // Invalid index: there is no such interface.
                return;
            }

            // Send a single-byte response of "0".
            // Assumption: response[0] == 0
            usbControlRead(1, response);
            return;
        }
        case USB_REQUEST_GET_STATUS: // USB Spec 9.4.5 Get Status
        {
            switch(usbSetupPacket.recipient)
            {
                case USB_RECIPIENT_DEVICE:
                {
                    // See USB Spec Table 9-4.
                    response[0] = vinPowerPresent() ? 1 : 0;
                    // Assumption: response[1] == 0
                    usbControlRead(2, response);
                    return;
                }
                case USB_RECIPIENT_INTERFACE:
                {
                    if (usbDeviceState < USB_STATE_CONFIGURED && usbSetupPacket.wIndex != 0)
                    {
                        // It is invalid to ask about interfaces other than 0 before the
                        // configured state.
                        return;
                    }

                    if (usbSetupPacket.wIndex >= ((USB_DESCRIPTOR_CONFIGURATION *)&usbConfigurationDescriptor)->bNumInterfaces)
                    {
                        // Invalid index: there is no such interface.
                        return;
                    }

                    // Send a 2-byte response of 0,0 (all of the bits are reserved)
                    // Assumption: response[0] == 0 and response[1] == 0
                    usbControlRead(2, response);
                    return;
                }
                case USB_RECIPIENT_ENDPOINT:
                {
                    if ((usbSetupPacket.wValue & 15) == 0)
                    {
                        // We don't support the halt feature on Endpoint 0
                        // (the USB Spec does not require or recommend it).
                        return;
                    }

                    if (usbDeviceState < USB_STATE_CONFIGURED)
                    {
                        // It is invalid to ask about non-zero endpoints before
                        // the configured state.
                        return;
                    }

                    // Assumption: We don't have a USB halt feature, i.e. we
                    // don't stall on non-zero endpoints.

                    // Send a 2-byte response of 0,0.
                    // Assumption: response[0] == 0 and response[1] == 0
                    usbControlRead(2, response);
                    return;
                }
            }
            return;
        }

        // Here are some more standard device requests we would need
        // to be USB compliant.  We didn't use them yet on any of our
        // PIC devices and it has not caused a problem as far as I
        // know.  We pay lip service to them here just in case they are
        // needed by some future driver.
        case USB_REQUEST_SET_FEATURE:
        case USB_REQUEST_CLEAR_FEATURE:
        {
            // Acknowledge the request but don't do anything.
            usbControlAcknowledge();
            return;
        }
        case USB_REQUEST_SYNCH_FRAME:
        {
            // Send a two-byte response of 0,0.
            usbControlRead(2, response);
            return;
        }
    }
}

BIT usbSuspended()
{
    return usbSuspendMode;
}

// Sleeps until we receive USB resume signaling.
// This uses PM1.  ( PM2 and PM3 are not usable because they will reset the USB module. )
// NOTE: For some reason, USB suspend does not work if you plug your device into a computer
// that is already sleeping.  If you do that, the device will remain awake with
// usbDeviceState == USB_STATE_POWERED and it will draw more power than it should from USB.
// TODO: figure out how to wake up when self power is connected.  Probably we should use the
// sleep timer to wake up regularly and check (that's going to be easier than using a P2
// interrupt I think).
// TODO: make sure suspend mode doesn't interfere with the radio libraries.  We should
// probably make a simple power-event registration thing using function pointers,
// (either an array, or a linked list where the memory is contributed by the modules using it).
// When going to sleep, we would call these functions in the order they were added;
// when waking up, we would call them in the opposite order.  Look at how we handle power
// on the jrk, maestros, and simple motor controller to see if this pattern works.
void usbSleep()
{
    uint8 savedPICTL = PICTL;
    BIT savedP0IE = P0IE;

    // The USB resume interrupt is mapped to the non-existent pin, P0_7.

    P0IE = 0;         // Disable the P0 interrupt while we are reconfiguring it (maybe not necessary).
    PICTL |= (1<<4);  // PICTL.P0IENH = 1  Enable the Port 0 interrupts for inputs 4-7 (USB_RESUME is #7).
    PICTL &= ~(1<<0); // PICTL.P0ICON = 0  Detect rising edges (this is required for waking up).

    do
    {
        // Clear the P0 interrupt flag that might prevent us from sleeping.
        P0IFG = 0;   // Clear Port 0 module interrupt flags.
        P0IF = 0;    // Clear Port 0 CPU interrupt flag (IRCON.P0IF = 0).

        P0IE = 1;    // Enable the Port 0 interrupt (IEN1.P0IE = 1) so we can wake up.

        // Put the device to sleep by following the recommended pseudo code in the datasheet section 12.1.3:
        SLEEP = (SLEEP & ~3) | 1;    // SLEEP.MODE = 1 : Selects Power Mode 1 (PM1).
        __asm nop __endasm; __asm nop __endasm; __asm nop __endasm;
        if (SLEEP & 3)
        {
            P1_0 = 1;
            PCON |= 1;    // PCON.IDLE = 1 : Actually go to sleep.
            P1_0 = 0;
        }

        // Disable the Port 0 interrupt.  If we don't do this, and there is no ISR
        // (just a reti), then the non-existent ISR could run many times while we
        // are awake, slowing down this loop.
        P0IE = 0; // (IEN1.P0IE = 1)

        // Check to see if the USB_RESUME bit is set.  If it is set, then there was
        // activity detected on the USB and it is time to wake up.
        // NOTE: The P0INT ISR might also set usbSuspendMode to 0 if the user defines
        // that ISR.  See the comment about P0INT in usb.h for more info.
        if (P0IFG & 0x80)
        {
            usbSuspendMode = 0;
        }
    }
    while(usbSuspendMode && !vinPowerPresent());

    // Wait for the high speed crystal oscillator to become stable again
    // because we can't write to USB registers until that happens.
    while(!(SLEEP & (1<<6))){};

    // Restore the interrupt registers to their original states.
    PICTL = savedPICTL;
    P0IE = savedP0IE;
}

void usbControlRead(uint16 bytesCount, uint8 XDATA * source)
{
    controlTransferState = CONTROL_TRANSFER_STATE_READ;
    controlTransferBytesLeft = bytesCount;
    controlTransferPointer = source;
}

void usbControlWrite(uint16 bytesCount, uint8 XDATA * source)
{
    controlTransferState = CONTROL_TRANSFER_STATE_WRITE;
    controlTransferBytesLeft = bytesCount;
    controlTransferPointer = source;
}

void usbControlAcknowledge()
{
    controlTransferState = CONTROL_TRANSFER_STATE_WRITE;
    controlTransferBytesLeft = 0;
}

void usbControlStall()
{
    controlTransferState = CONTROL_TRANSFER_STATE_NONE;
}

void usbInitEndpointIn(uint8 endpointNumber, uint8 maxPacketSize)
{
    USBINDEX = endpointNumber;
    USBMAXI = (maxPacketSize + 7) >> 3;
    USBCSIH = 1;                    // Enable Double buffering
}

void usbInitEndpointOut(uint8 endpointNumber, uint8 maxPacketSize)
{
    USBINDEX = endpointNumber;
    USBMAXO = (maxPacketSize + 7) >> 3;
    USBCSOH = 1;                    // Enable Double buffering
}
