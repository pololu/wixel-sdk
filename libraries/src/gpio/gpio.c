#include <cc2511_types.h>
#include <cc2511_map.h>
#include <gpio.h>

#define PIN_SWITCH(operation) switch(pinNumber) { \
        case 0:  operation(0,0); break; \
        case 1:  operation(0,1); break; \
        case 2:  operation(0,2); break; \
        case 3:  operation(0,3); break; \
        case 4:  operation(0,4); break; \
        case 5:  operation(0,5); break; \
        case 10: operation(1,0); break; \
        case 11: operation(1,1); break; \
        case 12: operation(1,2); break; \
        case 13: operation(1,3); break; \
        case 14: operation(1,4); break; \
        case 15: operation(1,5); break; \
        case 16: operation(1,6); break; \
        case 17: operation(1,7); break; \
        case 20: operation(2,0); break; \
        case 21: operation(2,1); break; \
        case 22: operation(2,2); break; \
        case 23: operation(2,3); break; \
        case 24: operation(2,4); break; }

#define SET_DIGITAL_OUTPUT(port, pin) { \
    P##port##_##pin = value; \
    P##port##DIR |= (1<<pin); }

#define SET_DIGITAL_INPUT(port, pin) { \
    if (pulled){ P##port##INP &= ~(1<<pin); } else { P##port##INP |= (1<<pin); } \
    P##port##DIR &= ~(1<<pin); }

#define IS_DIGITAL_INPUT_HIGH(port, pin) { return P##port##_##pin; }

void setDigitalOutput(uint8 pinNumber, BIT value) __reentrant
{
    PIN_SWITCH(SET_DIGITAL_OUTPUT);
}

void setDigitalInput(uint8 pinNumber, BIT pulled) __reentrant
{
    PIN_SWITCH(SET_DIGITAL_INPUT);
}

BIT isPinHigh(uint8 pinNumber) __reentrant
{
    PIN_SWITCH(IS_DIGITAL_INPUT_HIGH);
    return 0;
}

void setPort0PullType(BIT pullType) __reentrant
{
    if (pullType){ P2INP &= ~(1<<5); }
    else { P2INP |= (1<<5); }
}

void setPort1PullType(BIT pullType) __reentrant
{
    if (pullType){ P2INP &= ~(1<<6); }
    else { P2INP |= (1<<6); }
}

void setPort2PullType(BIT pullType) __reentrant
{
    if (pullType){ P2INP &= ~(1<<7); }
    else { P2INP |= (1<<7); }
}
