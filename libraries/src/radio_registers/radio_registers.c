#include <radio_registers.h>
#include <cc2511_map.h>

void radioRegistersInit()
{
    // Transmit power: one of the highest settings, but not the highest.
    PA_TABLE0 = 0xFE;

    // Set the center frequency of channel 0 to 2403.47 MHz.
    // Freq = 24/2^16*(FREQ[23:0]) = 24/2^16*(0x642500) = 2403.47 MHz
    FREQ2 = 0x64;
    FREQ1 = 0x25;
    FREQ0 = 0x00;

    // Note: We had to modify MDMCFG1 from the settings given by
    // SmartRF Studio to be compatible with the datasheet.
    // (NUM_PREAMBLE should be 8 at 500 kbps and having it be high is a good idea in general).
    // MDMCFG1.FEC_EN = 0 : Disable Forward Error Correction
    // MDMCFG1.NUM_PREAMBLE = 100 : Minimum number of preamble bytes is 8.
    // MDMCFG1.CHANSPC_E = 11 : Channel spacing exponent.
    // MDMCFG0.CHANSPC_M = 0x87 : Channel spacing mantissa.
    // Channel spacing = (256 + CHANSPC_M)*2^(CHANSPC_E) * f_ref / 2^18
    // So the center of channel 255 is
    //   2403.47 + 255 * ((256 + 0x87)*2^(3) * 24/2^18) = 2476.50 MHz
    // NOTE: The radio's Forward Error Correction feature requires CLKSPD=000.
    MDMCFG1 = 0x43;
    MDMCFG0 = 0x87;  // Modem Configuration

    // Controls the FREQ_IF used for RX.
    // This is affected by MDMCFG2.DEM_DCFILT_OFF according to p.212 of datasheet.
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

    //DEVIATN = 0x00;  // Modem Deviation Setting.  No effect because we are using MSK.
    // See Sec 13.9.2.

    FREND1 = 0xB6;   // Front End RX Configuration (adjusts various things, not well documented)
    FREND0 = 0x10;   // Front End TX Configuration (adjusts current TX LO buffer, not well documented)

    // F0CFG and BSCFG configure details of the PID loop used to correct the
    // bit rate and frequency of the signal (RX only I believe).
    FOCCFG = 0x1D;  // Frequency Offset Compensation Configuration
    BSCFG = 0x1C;   // Bit Synchronization Configuration

    // AGC Control:
    // This affects many things, including:
    //    Carrier Sense Absolute Threshold (Sec 13.10.5).
    //    Carrier Sense Relative Threshold (Sec 13.10.6).
    AGCCTRL2 = 0xC7;
    AGCCTRL1 = 0x00;
    AGCCTRL0 = 0xB2;

    // Frequency Synthesizer registers that are not fully documented.
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
}

uint8 radioLqi()
{
    return LQI & 0x7F;
}

int8 radioRssi()
{
    return ((int8)RSSI)/2 - RSSI_OFFSET;
}
