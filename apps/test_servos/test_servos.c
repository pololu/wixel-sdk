#include <cc2511_map.h>
#include <servo.h>
#include <wixel.h>
#include <usb.h>
#include <usb_com.h>

volatile uint8 DATA servoCounter = 0;

ISR(T1,0)
{
    switch(servoCounter++)
    {
    case 0:
        P0SEL &= ~0b11100;
        PERCFG |= (1<<6);  // PERCFG.T1CFG = 1:  Move Timer 1 to Alt. 2 location (P1_2, P1_1, P1_0)
        P1SEL |= 0b111;
        T1CC0 = 1;
        T1CC1 = -25000;
        T1CC2 = -26000;
        break;
    case 1:
        T1CC0 = T1CC1 = T1CC2 = 0xFFFF;
        break;

    case 4:
        P1SEL &= ~0b111;
        PERCFG &= ~(1<<6);  // PERCFG.T1CFG = 0:  Move Timer 1 to Alt. 1 location (P0_2, P0_3, P0_4)
        P0SEL |= 0b11100;
        T1CC0 = -30000;
        T1CC1 = -32000;
        T1CC2 = 0x10000-34000;
        break;

    case 5:
        T1CC0 = T1CC1 = T1CC2 = 0xFFFF;
        break;

    case 7:
        servoCounter = 0;
        break;
    }

}

void main()
{
    systemInit();
    usbInit();

    P2DIR = (P2DIR & ~0b11000000) | 0b11000000;
    P2SEL |= (1<<3); // might not be necessary, makes Timer 1 have priority over USART1 on Port 1

    // Set up hardware PWM.
    PERCFG &= ~(1<<6);  // PERCFG.T1CFG = 0:  Move Timer 1 to Alt. 1 location (P0_2, P0_3, P0_4)

    P1_0 = P1_1 = P1_2 = 0;
    P1SEL |= 0b111;
    P1DIR |= 0b111;

    P0_4 = P0_3 = P0_2 = 0;
    P0SEL |= 0b11100;
    P0DIR |= 0b11100;

    T1CC0 = 60000;         // Period = 60000/24 = 2500 microseconds.

    // Configure Timer 1 Channels 0-2 to be in compare mode.  Set output on compare-up, clear on 0.
    // With this configuration, we can set T1CC0, T1CC1, or T1CC2 to -N to get a pulse of with N,
    // as long as N > 1.
    // We can set the register to -1 or 0 to disable the pulse.
    T1CCTL0 = T1CCTL1 = T1CCTL2 = 0b00011100;

    T1CTL = 0b00000001;    // Timer 1: Start free-running mode, counting from 0x0000 to 0xFFFF.

    T1CC0 = -1;
    T1CC1 = -9;
    T1CC2 = -15;

    IP0 |= (1<<1);
    IP1 |= (1<<1);
    T1IE = 1; // Enable the Timer 1 interrupt.
    EA = 1;   // Enable interrupts in general.

    P1DIR |= (1<<4);

    while(1)
    {
        boardService();
        usbComService();
    }
}
