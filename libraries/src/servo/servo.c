#include <servo.h>

#define SERVO_TICKS_PER_MICROSECOND  24

/**ISR(T1, 1)
{
    P1_4 ^= 1;
    
    //T1IF = 0;  // T1IF gets cleared automatically when the CPU vectors to serve this interrupt.
    if (T1CTL & (1<<4)) // Check T1CTL.OVFIF
    {
        P1_4 ^= 1;
        T1CTL &= ~(1<<4);  // T1CTL.OVFIF = 0
    }
    
    if (T1CTL & (1<<5)) // Check T1CTL.CH1IF
    {
        P1_4 ^= 1;
        T1CTL &= ~(1<<5);  // T1CTL.CH1IF = 0
    }
}**/

void servosStart(uint8 XDATA * pins, uint8 num_pins)
{
    pins, num_pins;
    
    // Avoid generating a hardware PWM signal:
    P2SEL |= (1<<4);   // P2SEL.PRI1P1 = 1: Give Timer 4 priority over Timer 1.
    PERCFG |= (1<<6);  // PERCFG.T1CFG = 1:  Move Timer 1 to Alt. 2 location (P1_0, P1_1, P1_2)

    T1CC0 = 60000;         // Period = 60000/24 = 2500 microseconds.
    T1CCTL1 = 0b01000100;  // Timer 1 Channel 1: Compare mode with interrupt enabled.
    T1CTL = 0b00000010;    // Timer 1: Module mode, repeatedly counts from 0 to T1CC0.
    
    T1CC1 = 500; //(uint16)1500 * SERVO_TICKS_PER_MICROSECOND;

    // Set the Timer 1 interrupt to the highest possible priority because the timing
    // needs to be predictable.  This also means that the ADC and P2INT/USB interrupts
    // are the highest possible priority, so they can't be used.
    IP0 |= (1<<1);
    //IP1 |= (1<<1);
   
    // tmphax
    IP0 = IP1 = 2;
    
    T1IE = 1; // Enable the Timer 1 interrupt.
    EA = 1;   // Enable interrupts in general.
    
    // tmphax:
    P1DIR |= (1<<4);
}


void servoSetTarget(uint8 servo_num, uint16 position);
uint16 servoGetTarget(uint8 servo_num);

uint16 servoGetPosition(uint8 servo_num);

void servoSetSpeed(uint8 servo_num, uint16 speed);
uint16 servoGetSpeed(uint8 servo_num);