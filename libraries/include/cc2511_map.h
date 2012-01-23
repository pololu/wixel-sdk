/*! \file cc2511_map.h
 * This header file provides access to the special registers on the CC2511F32, which
 * allow direct manipulation of the chip's hardware.
 *
 * This file also provides macros for defining Interrupt Service Routines (ISRs).
 *
 * For documentation, see the
 * <a href="http://focus.ti.com/docs/prod/folders/print/cc2511f32.html">CC2511F32 datasheet</a>.
 */

#ifndef _CC2511_MAP_H
#define _CC2511_MAP_H

// Avoid false-alarm syntax errors in Eclipse.
#ifdef __CDT_PARSER__
#define __sfr
#define __at(x)
#define __xdata
#define __interrupt(x)
#define __using(x)
#endif

#define SFRBIT(address, name, bit7, bit6, bit5, bit4, bit3, bit2, bit1, bit0) \
  SFR(address, name)    \
  SBIT(address+0, bit0) \
  SBIT(address+1, bit1) \
  SBIT(address+2, bit2) \
  SBIT(address+3, bit3) \
  SBIT(address+4, bit4) \
  SBIT(address+5, bit5) \
  SBIT(address+6, bit6) \
  SBIT(address+7, bit7)

#if defined SDCC || defined __CDT_PARSER__
// Syntax for the SDCC (Small Device C Compiler).
#define SFR(address, name) static __sfr __at (address) name;
#define SBIT(address, name) static __sbit __at (address) name;
#define SFR16(addressH, addressL, name) static __sfr16 __at (((addressH) << 8) + (addressL)) name;
#define SFRX(address, name)       static volatile unsigned char __xdata __at(address) name;

/*! Defines or declares an interrupt service routine (ISR).
 * <b>For the interrupt to work, SDCC requires that the declaration must
 * be present in the file that defines main().</b>
 *
 * \param source
 *    The source of the interrupt.  Must be either the first word of one of
 *    the *_VECTOR macros defined in this file (e.g. "P1INT").
 *
 * \param bank
 *    The register bank to use.  Must be a number from 0 to 3, inclusive.
 *    If you choose a non-zero bank, then the compiler will assume that the
 *    ISR can modify the registers in that bank, and not bother to restore
 *    them to their original value.
 *    Therefore, we recommend choosing bank 0 unless you want to save some
 *    CPU time and you can guarantee that it is OK for the interrupt to
 *    modify those registers.
 *
 * Example ISR declaration (in a .h file):
\code
ISR(UTX1, 0);
\endcode
 *
 * Example ISR definition (in a .c file):
\code
ISR(UTX1, 0)
{
    // code for handling event and clearing interrupt flag
}
\endcode
 */
#define ISR(source, bank) void ISR_##source() __interrupt(source##_VECTOR) __using(bank)

#else
#error "Unknown compiler."
#endif

// Interrupt vectors (SWRS055F Table 39)
#define RFTXRX_VECTOR 0
#define ADC_VECTOR    1
#define URX0_VECTOR   2
#define URX1_VECTOR   3
#define ENC_VECTOR    4
#define ST_VECTOR     5
#define P2INT_VECTOR  6
#define UTX0_VECTOR   7
#define DMA_VECTOR    8
#define T1_VECTOR     9
#define T2_VECTOR     10
#define T3_VECTOR     11
#define T4_VECTOR     12
#define P0INT_VECTOR  13
#define UTX1_VECTOR   14
#define P1INT_VECTOR  15
#define RF_VECTOR     16
#define WDT_VECTOR    17

// Special Function Registers (SWRS055F Table 30)

SFRBIT(0x80, P0, P0_7, P0_6, P0_5, P0_4, P0_3, P0_2, P0_1, P0_0)
SFR(0x81, SP)
SFR(0x82, DPL0)
SFR(0x83, DPH0)
SFR(0x84, DPL1)
SFR(0x85, DPH1)
SFR(0x86, U0CSR)
SFR(0x87, PCON)

SFRBIT(0x88, TCON, URX1IF, _TCON_6, ADCIF, _TCON_4, URX0IF, _TCON_2, RFTXRXIF, _TCON_0)
SFR(0x89, P0IFG)
SFR(0x8A, P1IFG)
SFR(0x8B, P2IFG)
SFR(0x8C, PICTL)
SFR(0x8D, P1IEN)
//  0x8E
SFR(0x8F, P0INP)

SFRBIT(0x90, P1, P1_7, P1_6, P1_5, P1_4, P1_3, P1_2, P1_1, P1_0)
SFR(0x91, RFIM)
SFR(0x92, DPS)
SFR(0x93, MPAGE)
//  0x94
SFR(0x95, ENDIAN)
//  0x96
//  0x97

SFRBIT(0x98, S0CON, _SOCON7, _SOCON6, _SOCON5, _SOCON4, _SOCON3, _SOCON2, ENCIF_1, ENCIF_0)
//  0x99
SFR(0x9A, IEN2)
SFR(0x9B, S1CON)
SFR(0x9C, T2CT)
SFR(0x9D, T2PR)
SFR(0x9E, T2CTL)
//  0x9F

SFRBIT(0xA0, P2, P2_7, P2_6, P2_5, P2_4, P2_3, P2_2, P2_1, P2_0)
SFR(0xA1, WORIRQ)
SFR(0xA2, WORCTRL)
SFR(0xA3, WOREVT0)
SFR(0xA4, WOREVT1)
SFR(0xA5, WORTIME0)
SFR(0xA6, WORTIME1)
//  0xA7

SFRBIT(0xA8, IEN0, EA, _IEN06, STIE, ENCIE, URX1IE, URX0IE, ADCIE, RFTXRXIE)
SFR(0xA9, IP0)
//  0xAA
SFR(0xAB, FWT)
SFR(0xAC, FADDRL)
SFR(0xAD, FADDRH)
SFR(0xAE, FCTL)
SFR(0xAF, FWDATA)

//  0xB0
SFR(0xB1, ENCDI)
SFR(0xB2, ENCDO)
SFR(0xB3, ENCCS)
SFR(0xB4, ADCCON1)
SFR(0xB5, ADCCON2)
SFR(0xB6, ADCCON3)
//  0xB7

SFRBIT(0xB8, IEN1, _IEN17, _IEN16, P0IE, T4IE, T3IE, T2IE, T1IE, DMAIE)
SFR(0xB9, IP1)
SFR(0xBA, ADCL)
SFR(0xBB, ADCH)
SFR(0xBC, RNDL)
SFR(0xBD, RNDH)
SFR(0xBE, SLEEP)
//  0xBF

SFRBIT(0xC0, IRCON, STIF, _IRCON6, P0IF, T4IF, T3IF, T2IF, T1IF, DMAIF)
SFR(0xC1, U0DBUF)
SFR(0xC2, U0BAUD)
//  0xC3
SFR(0xC4, U0UCR)
SFR(0xC5, U0GCR)
SFR(0xC6, CLKCON)
SFR(0xC7, MEMCTR)

//  0xC8
SFR(0xC9, WDCTL)
SFR(0xCA, T3CNT)
SFR(0xCB, T3CTL)
SFR(0xCC, T3CCTL0)
SFR(0xCD, T3CC0)
SFR(0xCE, T3CCTL1)
SFR(0xCF, T3CC1)

SFRBIT(0xD0, PSW, CY, AC, F0, RS1, RS0, OV, F1, P)
SFR(0xD1, DMAIRQ)
SFR(0xD2, DMA1CFGL)
SFR(0xD3, DMA1CFGH)
SFR(0xD4, DMA0CFGL)
SFR(0xD5, DMA0CFGH)
SFR(0xD6, DMAARM)
SFR(0xD7, DMAREQ)

SFRBIT(0xD8, TIMIF, _TIMIF7, OVFIM, T4CH1IF, T4CH0IF, T4OVFIF, T3CH1IF, T3CH0IF, T3OVFIF)
SFR(0xD9, RFD)
SFR(0xDA, T1CC0L)
SFR(0xDB, T1CC0H)
SFR(0xDC, T1CC1L)
SFR(0xDD, T1CC1H)
SFR(0xDE, T1CC2L)
SFR(0xDF, T1CC2H)

SFRBIT(0xE0, ACC, ACC_7, ACC_6, ACC_5, ACC_4, ACC_3, ACC_2, ACC_1, ACC_0)
SFR(0xE1, RFST)
SFR(0xE2, T1CNTL)
SFR(0xE3, T1CNTH)
SFR(0xE4, T1CTL)
SFR(0xE5, T1CCTL0)
SFR(0xE6, T1CCTL1)
SFR(0xE7, T1CCTL2)

SFRBIT(0xE8, IRCON2, _IRCON27, _IRCON26, _IRCON25, WDTIF, P1IF, UTX1IF, UTX0IF, P2IF)
SFR(0xE9, RFIF)
SFR(0xEA, T4CNT)
SFR(0xEB, T4CTL)
SFR(0xEC, T4CCTL0)
SFR(0xED, T4CC0)
SFR(0xEE, T4CCTL1)
SFR(0xEF, T4CC1)

SFRBIT(0xF0, B, B_7, B_6, B_5, B_4, B_3, B_2, B_1, B_0)
SFR(0xF1, PERCFG)
SFR(0xF2, ADCCFG)
SFR(0xF3, P0SEL)
SFR(0xF4, P1SEL)
SFR(0xF5, P2SEL)
SFR(0xF6, P1INP)
SFR(0xF7, P2INP)

SFRBIT(0xF8, U1CSR, U1MODE, U1RE, U1SLAVE, U1FE, U1ERR, U1RX_BYTE, U1TX_BYTE, U1ACTIVE)
SFR(0xF9, U1DBUF)
SFR(0xFA, U1BAUD)
SFR(0xFB, U1UCR)
SFR(0xFC, U1GCR)
SFR(0xFD, P0DIR)
SFR(0xFE, P1DIR)
SFR(0xFF, P2DIR)

#define USB_VECTOR P2INT_VECTOR
#define USBIF P2IF

// 16-bit SFRs
SFR16(0xD5, 0xD4, DMA0CFG)
SFR16(0xD3, 0xD2, DMA1CFG)
SFR16(0xAD, 0xAC, FADDR)
SFR16(0xBB, 0xBA, ADC)
SFR16(0xDB, 0xDA, T1CC0)
SFR16(0xDD, 0xDC, T1CC1)
SFR16(0xDF, 0xDE, T1CC2)

// XDATA Radio Registers (SWRS055F Table 32)

SFRX(0xDF00, SYNC1)
SFRX(0xDF01, SYNC0)
SFRX(0xDF02, PKTLEN)
SFRX(0xDF03, PKTCTRL1)
SFRX(0xDF04, PKTCTRL0)
SFRX(0xDF05, ADDR)
SFRX(0xDF06, CHANNR)
SFRX(0xDF07, FSCTRL1)
SFRX(0xDF08, FSCTRL0)
SFRX(0xDF09, FREQ2)
SFRX(0xDF0A, FREQ1)
SFRX(0xDF0B, FREQ0)
SFRX(0xDF0C, MDMCFG4)
SFRX(0xDF0D, MDMCFG3)
SFRX(0xDF0E, MDMCFG2)
SFRX(0xDF0F, MDMCFG1)
SFRX(0xDF10, MDMCFG0)
SFRX(0xDF11, DEVIATN)
SFRX(0xDF12, MCSM2)
SFRX(0xDF13, MCSM1)
SFRX(0xDF14, MCSM0)
SFRX(0xDF15, FOCCFG)
SFRX(0xDF16, BSCFG)
SFRX(0xDF17, AGCCTRL2)
SFRX(0xDF18, AGCCTRL1)
SFRX(0xDF19, AGCCTRL0)
SFRX(0xDF1A, FREND1)
SFRX(0xDF1B, FREND0)
SFRX(0xDF1C, FSCAL3)
SFRX(0xDF1D, FSCAL2)
SFRX(0xDF1E, FSCAL1)
SFRX(0xDF1F, FSCAL0)

SFRX(0xDF23, TEST2)
SFRX(0xDF24, TEST1)
SFRX(0xDF25, TEST0)

SFRX(0xDF2E, PA_TABLE0)
SFRX(0xDF2F, IOCFG2)
SFRX(0xDF30, IOCFG1)
SFRX(0xDF31, IOCFG0)

SFRX(0xDF36, PARTNUM)
SFRX(0xDF37, VERSION)
SFRX(0xDF38, FREQEST)
SFRX(0xDF39, LQI)
SFRX(0xDF3A, RSSI)
SFRX(0xDF3B, MARCSTATE)
SFRX(0xDF3C, PKTSTATUS)
SFRX(0xDF3D, VCO_VC_DAC)


// I2S Registers (SWRS055F Table 33)
SFRX(0xDF40, I2SCFG0)
SFRX(0xDF41, I2SCFG1)
SFRX(0xDF42, I2SDATL)
SFRX(0xDF43, I2SDATH)
SFRX(0xDF44, I2SWCNT)
SFRX(0xDF45, I2SSTAT)
SFRX(0xDF46, I2SCLKF0)
SFRX(0xDF47, I2SCLKF1)
SFRX(0xDF48, I2SCLKF2)


// Common USB Registers (SWRS055F Table 34)
SFRX(0xDE00, USBADDR)
SFRX(0xDE01, USBPOW)
SFRX(0xDE02, USBIIF)

SFRX(0xDE04, USBOIF)

SFRX(0xDE06, USBCIF)
SFRX(0xDE07, USBIIE)

SFRX(0xDE09, USBOIE)

SFRX(0xDE0B, USBCIE)
SFRX(0xDE0C, USBFRML)
SFRX(0xDE0D, USBFRMH)
SFRX(0xDE0E, USBINDEX)

// Indexed USB Endpoint Registers (SWRS055F Table 35)
SFRX(0xDE10, USBMAXI)
SFRX(0xDE11, USBCSIL)
SFRX(0xDE12, USBCSIH)
SFRX(0xDE13, USBMAXO)
SFRX(0xDE14, USBCSOL)
SFRX(0xDE15, USBCSOH)
SFRX(0xDE16, USBCNTL)
SFRX(0xDE17, USBCNTH)

// USB Fifo addresses
SFRX(0xDE20, USBF0)
SFRX(0xDE22, USBF1)
SFRX(0xDE24, USBF2)
SFRX(0xDE26, USBF3)
SFRX(0xDE28, USBF4)
SFRX(0xDE2A, USBF5)

#define USBCS0     USBCSIL
#define USBCNT0    USBCNTL

/*! Evaluates to the XDATA address of an SFR.
 *
 * Most of the internal SFRs are also part of the XDATA memory space,
 * which means you can have pointers to them of type
 * <code>uint8 XDATA *</code> and you can read or write to them using
 * DMA transfers.
 *
 * This macro does NOT work with the SFRs that are highlighted in gray
 * in Table 30 of the CC2511F32 datasheet (the "SFR Address Overview"
 * table). */
#define XDATA_SFR_ADDRESS(sfr) (0xDF00 + ((unsigned int)&(sfr)))

/*! This struct represents the configuration of a single DMA channel.
 * See the "DMA Controller" section of the CC2511F32 datasheet
 * for information on how to use this struct and DMA in general.
 *
 * Also see dma.h.
 *
 * NOTE: You won't find DC6 or DC7 in the datasheet, the names of
 * those bytes were invented for this struct. */
typedef struct
{
    unsigned char SRCADDRH;
    unsigned char SRCADDRL;
    unsigned char DESTADDRH;
    unsigned char DESTADDRL;

    /*! Bits 7:5 are VLEN, bits 4:0 are LEN[12:8] */
    unsigned char VLEN_LENH;
    unsigned char LENL;

    /*! Bit 7 is WORDSIZE, Bits 6:5 are TMODE, Bits 4:0 are TRIG. */
    unsigned char DC6;

    /*! Bits 7:6 are SRCINC, 5:4 are DESTINC, 3 is IRQMASK, 2 is M8, 1:0 are PRIORITY. */
    unsigned char DC7;
} DMA_CONFIG;


#endif