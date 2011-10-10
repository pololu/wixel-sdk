#include <servo.h>

/** Note: This library assumes that the Wixel is running at 24 MHz. **/

/** Note: The servo pulse period used by this library is 2^16/24*7 = 19114.66 microseconds.
 *  There is no easy way to get a period of 20000 microseconds:
 *  We can't run the timer in modulo mode because then we would lose control of the
 *    duty cycle of channel 0 (T1CC0 would be used to set the timer period).
 *  We can't set the T1CNT in an interrupt, because any write to T1CNTL resets the count to 0.
 *  We could delay for 900 microseconds in this interrupt and then write to T1CNTL,
 *    but that uses up a lot of CPU time.
 *  During one of the timer periods we could try switching the timer to modulo mode,
 *    to make the period shorter.
 */


/** Internal Channel Number   Pin       Timer 1 Channel     Alt Location
 *  0                         P0_2      0                   1
 *  1                         P0_3      1                   1
 *  2                         P0_4      2                   1
 *  3                         P1_2      0                   2
 *  4                         P1_1      1                   2
 *  5                         P1_0      2                   2
 */

#define SERVO_TICKS_PER_MICROSECOND  24
#define MAX_SERVOS 6

volatile uint8 DATA servoCounter = 0;

// Associates external channel number (the number picked by the user) to the
// internal channel number.
static uint8 servoAssignment[MAX_SERVOS];

struct SERVO_DATA
{
    uint16 target;
    uint16 position;
    uint16 speed;
};

static struct SERVO_DATA PDATA servoData[MAX_SERVOS];

ISR(T1,0)
{
    switch(servoCounter++)
    {
    case 0:
        P0SEL &= ~0b11100;
        PERCFG |= (1<<6);  // PERCFG.T1CFG = 1:  Move Timer 1 to Alt. 2 location (P1_2, P1_1, P1_0)
        P1SEL |= 0b111;
        T1CC0 = servoData[3].position;
        T1CC1 = servoData[4].position;
        T1CC2 = servoData[5].position;
        break;

    case 1:
        T1CC0 = T1CC1 = T1CC2 = 0xFFFF;
        break;

    case 3:
        P1SEL &= ~0b111;
        PERCFG &= ~(1<<6);  // PERCFG.T1CFG = 0:  Move Timer 1 to Alt. 1 location (P0_2, P0_3, P0_4)
        P0SEL |= 0b11100;
        T1CC0 = servoData[0].position;
        T1CC1 = servoData[1].position;
        T1CC2 = servoData[2].position;
        break;

    case 4:
        T1CC0 = T1CC1 = T1CC2 = 0xFFFF;
        break;

    case 6:
        servoCounter = 0;
        break;
    }
}

static uint8 pinToInternalChannelNumber(uint8 pin)
{
    switch(pin)
    {
    case 2: return 0;
    case 3: return 1;
    case 4: return 2;
    case 12: return 3;
    case 11: return 4;
    case 10: return 5;
    default: return 0;
    }
}


void servosStart(uint8 XDATA * pins, uint8 num_pins)
{
    uint8 i;
    for (i = 0; i < MAX_SERVOS; i++)
    {
        if (i < num_pins)
        {
            servoAssignment[i] = pinToInternalChannelNumber(pins[i]);
        }
    }

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

}

void servoSetTarget(uint8 servo_num, uint16 target)
{
    servoData[servoAssignment[servo_num]].target = target*SERVO_TICKS_PER_MICROSECOND;
    servoData[servoAssignment[servo_num]].position = target*SERVO_TICKS_PER_MICROSECOND; //tmphax
}

uint16 servoGetTarget(uint8 servo_num);

uint16 servoGetPosition(uint8 servo_num);

void servoSetSpeed(uint8 servo_num, uint16 speed);
uint16 servoGetSpeed(uint8 servo_num);
