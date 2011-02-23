/*  radio_registers.c:  This file contains the recommended configuration of the CC2511
 *  radio registers.
 *
 *  NOTE: The PKTLEN, MCSM0, MCSM1, MCSM2, CHANNR, and ADDR registers and the DMA do not get configured here.
 */

#include "radio_registers.h"
#include <cc2511_map.h>

void radioRegistersInit()
{
    // TODO: Look at the signal on the spectrum analyzer to choose a good RX filter bandwidth (MDMCFG4).
    // TODO: try increasing RX filter BW to 1200 kHz because the spectrum analyzer makes it look like the band is
    // that wide

    // Transmit power: one of the highest settings, but no the highest.
    PA_TABLE0 = 0xFE;

    // Set the frequency to 2400.159 MHz.
    // FREQ[23:0] = 2^16*(fCarrier/fRef) = 2^16*(2400.156/24) = 0x6401AA
    FREQ2 = 0x64;
    FREQ1 = 0x01;
    FREQ0 = 0xAA;

    // Controls the FREQ_IF used for RX.
    // This is affected by MDMCFG2.DEM_DCFILT_OFF according to p.212 of datasheet.
    // TODO: Learn what the correct value should be!
    FSCTRL1 = 0x0A;  // Frequency Synthesizer Control
    FSCTRL0 = 0x00;  // Frequency Synthesizer Control

    // Sets the data rate (symbol rate) used in TX and RX.  See Sec 13.5 of the datasheet.
    // Also sets the channel bandwidth.
    // We tried different data rates: 375 kbps was pretty good, but 400 kbps and above caused lots of packet errors.
    // NOTE: If you change this, you must change RSSI_OFFSET in radio_registers.h
    MDMCFG4 = 0x1D;  MDMCFG3 = 0xDE; // Modem configuration (data rate = 350 kbps, bandwidth = 600 kHz).

    // MDMCFG2.DEM_DCFILT_OFF = 0, enable digital DC blocking filter before
    //   demodulator.  This affects the FREQ_IF according to p.212 of datasheet.
    // MDMCFC2.MANCHESTER_EN = 0 is required because we are using MSK (see Sec 13.9.2)
    // MDMCFG2.MOD_FORMAT = 111: MSK modulation
    // MDMCFG2.SYNC_MODE = 111: Strictest requirements for receiving a packet.
    MDMCFG2 = 0x73;  // Modem Configuration

    // Note: I had to modify MDMCFG1 from the settings given by
    // SmartRF Studio to be compatible with the per_test and datasheet
    // (NUM_PREAMBLE should be 8 at 500 kbps and having it be high is a good idea in general).
    // MDMCFG1.FEC_EN = 0,1 : 0=Disable,1=Enable Forward Error Correction
    // MDMCFG1.NUM_PREAMBLE = 100 : Minimum number of preamble bytes is 8.
    // MDMCFG1.CHANSPC_E = 11 : Channel spacing exponent.
    // MDMCFG1.CHANSPC_M = 0xFF : Channel spacing mantissa.
    // Channel spacing = (256 + CHANSPC_M)*2^(CHANSPC_E) * f_ref / 2^18
    //   = (256 + 170)*2^(3) * 24000kHz / 2^18 = 312 kHz
    // NOTE: The radio's Forward Error Correction feature requires CLKSPD=000.
    MDMCFG1 = 0x43;
    MDMCFG0 = 0xAA;  // Modem Configuration

    //DEVIATN = 0x00;  // Modem Deviation Setting.  No effect because we are using MSK.
    // See Sec 13.9.2.

    FREND1 = 0xB6;   // Front End RX Configuration (adjusts various things, not well documented)
    FREND0 = 0x10;   // Front End TX Configuration (adjusts current TX LO buffer, not well documented)

    // F0CFG and BSCFG configure details of the PID loop used to correct the
    // bit rate and frequency of the signal (RX only I believe).
    FOCCFG = 0x1D;  // Frequency Offset Compensation Configuration
    BSCFG = 0x1C;   // Bit Synchronization Configuration

    // AGC Control: Complicated stuff that David doesn't understand yet.
    // This affects many things, including:
    //    Carrier Sense Absolute Threshold (Sec 13.10.5).
    //    Carrier Sense Relative Threshold (Sec 13.10.6).
    AGCCTRL2 = 0xC7;
    AGCCTRL1 = 0x00;
    AGCCTRL0 = 0xB2;

    // Frequency Synthesizer registers that are not fully documented.
    // TODO: Implement fast channel hopping by storing the results of calibrations
    // in memory.  See p. 221 of the datasheet. Also change MCSM.FS_AUTOCAL.
    FSCAL3 = 0xEA;
    FSCAL2 = 0x0A;
    FSCAL1 = 0x00;
    FSCAL0 = 0x11;

    // Mostly-undocumented test settings.
    // NOTE: The datasheet says TEST1 must be 0x31, but SmartRF Studio recommends 0x11.
    TEST2 = 0x88;
    TEST1 = 0x31;//0x31;//0x11;
    TEST0 = 0x09;//0x09;//0x0B;

    // Packet control settings.
    PKTCTRL1 = 0x04;
    PKTCTRL0 = 0x45; // Enable data whitening, CRC, and variable length packets.
}

BIT radioCrcPassed()
{
    return (LQI & 0x80) ? 1 : 0;
    //return radioPacketRx[radioPacketRx[0] + 2] & 0x80;
}

uint8 radioLqi()
{
    return LQI & 0x7F;
    //return radioPacketRx[radioPacketRx[0] + 2] & 0x7F;
}

int8 radioRssi()
{
    return ((int8)RSSI)/2 - RSSI_OFFSET;
    //return ((int8)radioPacketRx[radioPacketRx[0] + 1])/2 - RSSI_OFFSET;
}

// Local Variables: **
// mode: C **
// c-basic-offset: 4 **
// tab-width: 4 **
// indent-tabs-mode: nil **
// end: **
