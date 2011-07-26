; This file contains the addresses of certain pieces of information that are
; stored the Wixel's bootloader.

    .module delay
    .area CSEG (CODE)

    .globl _serialNumber
    .globl _serialNumberStringDescriptor
    .globl _bootloaderDeviceDescriptor

;; The USB device descriptor of the bootloader is stored at this address:
_bootloaderDeviceDescriptor = 0x03CC

;; The four bytes of the serial number are stored in the bootloader at this address:
_serialNumber = 0x03E0

;; The Serial Number String Descriptor is stored in the bootloader at this address:
_serialNumberStringDescriptor = 0x03E6
