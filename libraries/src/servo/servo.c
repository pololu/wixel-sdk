#include <servo.h>

// TODO: do something disable the PWM when going into bootloader mode

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
static uint8 PDATA servoAssignment[MAX_SERVOS];

struct SERVO_DATA
{
    uint16 target;
    uint16 position;
    uint16 positionReg;
    uint16 speed;
};

static volatile struct SERVO_DATA XDATA servoData[MAX_SERVOS];

ISR(T1,0)
{
    volatile struct SERVO_DATA XDATA * d;
    uint8 i;

    P1_3 = 1;  // tmphax

    switch(servoCounter++)
    {
    case 0:
        P1SEL &= ~0b111;
        PERCFG &= ~(1<<6);  // PERCFG.T1CFG = 0:  Move Timer 1 to Alt. 1 location (P0_2, P0_3, P0_4)
        P0SEL |= 0b11100;
        T1CC0 = servoData[0].positionReg;
        T1CC1 = servoData[1].positionReg;
        T1CC2 = servoData[2].positionReg;
        break;

    case 1:
        T1CC0 = T1CC1 = T1CC2 = 0xFFFF;
        break;

    case 3:
        P0SEL &= ~0b11100;
        PERCFG |= (1<<6);  // PERCFG.T1CFG = 1:  Move Timer 1 to Alt. 2 location (P1_2, P1_1, P1_0)
        P1SEL |= 0b111;
        T1CC0 = servoData[3].positionReg;
        T1CC1 = servoData[4].positionReg;
        T1CC2 = servoData[5].positionReg;
        break;

    case 4:
        T1CC0 = T1CC1 = T1CC2 = 0xFFFF;
        break;

    case 6:
        // David measured how long these updates take, and it is only about 70us even if there is
        // speed enabled for all channels, so it seems OK to do it in the interrupt.

        servoCounter = 0;

        for(i = 0; i < MAX_SERVOS; i++)
        {
            uint16 pos;

            d = servoData + i;
            pos = d->position;

            if (d->speed && pos && d->target)
            {
                if (d->target > pos)
                {
                    if (d->target - pos < d->speed)
                    {
                        pos = d->target;
                    }
                    else
                    {
                        pos += d->speed;
                    }
                }
                else
                {
                    if (pos - d->target < d->speed)
                    {
                        pos = d->target;
                    }
                    else
                    {
                        pos -= d->speed;
                    }
                }
            }
            else
            {
                pos = d->target;
            }
            d->position = pos;
            d->positionReg = ~pos + 1;
        }

        break;
    }

    P1_3 = 0; // tmphax
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
        servoData[i].target = 0;
        servoData[i].position = 0;
        servoData[i].positionReg = 0;
        servoData[i].speed = 0;

        if (i < num_pins)
        {
            servoAssignment[i] = pinToInternalChannelNumber(pins[i]);
        }
    }

    P1_0 = P1_1 = P1_2 = 0;
    P1SEL |= 0b111;
    P1DIR |= 0b111;

    P0_4 = P0_3 = P0_2 = 0;
    P0SEL |= 0b11100;
    P0DIR |= 0b11100;

    // Set PRIP0[1:0] to 11 (Timer 1 channel 2 - USART0).  Otherwise,
    // Timer 1 can not control P0_4.
    P2DIR = (P2DIR & ~0b11000000) | 0b11000000;

    // Configure Timer 1 Channels 0-2 to be in compare mode.  Set output on compare-up, clear on 0.
    // With this configuration, we can set T1CC0, T1CC1, or T1CC2 to -N to get a pulse of with N,
    // as long as N > 1.
    // We can set the register to -1 or 0 to disable the pulse.
    T1CCTL0 = T1CCTL1 = T1CCTL2 = 0b00011100;

    // Turn off all the pulses.
    T1CC0 = T1CC1 = T1CC2 = 0;

    T1CTL = 0b00000001;    // Timer 1: Start free-running mode, counting from 0x0000 to 0xFFFF.

    // Set the Timer 1 interrupt priority to 2, the second highest.
    IP0 &= ~(1<<1);
    IP1 |= (1<<1);
    T1IE = 1; // Enable the Timer 1 interrupt.
    EA = 1;   // Enable interrupts in general.

    P1_3 = 0; P1DIR |= (1<<3); // tmphax

}

void servoSetTarget(uint8 servo_num, uint16 target)
{
    volatile struct SERVO_DATA XDATA * d = servoData + servoAssignment[servo_num];

    // If the user specified the target in microseconds, convert it to timer
    // ticks (24ths of a microsecond).
    if (target < 3000){ target *= 24; }

    T1IE = 0; // Make sure we don't get interrupted in the middle of an update.
    d->target = target;
    //d->position = target; //tmphax
    //d->positionReg = ~target + 1; //tmphax
    T1IE = 1;
}

uint16 servoGetTarget(uint8 servo_num)
{
    return servoData[servoAssignment[servo_num]].target;
}

uint16 servoGetPosition(uint8 servo_num)
{
    // TODO: remove the T1IE precautions here if it turns out we don't update position in the interrupt
    uint16 position;
    T1IE = 0; // Make sure we don't get interrupted in the middle of reading the position.
    position = servoData[servoAssignment[servo_num]].position;
    T1IE = 1;
    return position;
}

void servoSetSpeed(uint8 servo_num, uint16 speed)
{
    T1IE = 0; // Make sure we don't get interrupted in the middle of an update.
    servoData[servoAssignment[servo_num]].speed = speed;
    T1IE = 1;
}

uint16 servoGetSpeed(uint8 servo_num)
{
    return servoData[servoAssignment[servo_num]].speed;
}
