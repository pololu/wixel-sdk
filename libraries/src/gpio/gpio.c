#include <cc2511_types.h>
#include <cc2511_map.h>
#include <gpio.h>

void setDigitalOutput(uint8 pin, BIT value) __reentrant
{
	switch(pin)
	{
	case 0:  P0_0 = value; P0DIR |= (1<<0); return;
	case 1:  P0_1 = value; P0DIR |= (1<<1); return;
	case 2:  P0_2 = value; P0DIR |= (1<<2); return;
	case 3:  P0_3 = value; P0DIR |= (1<<3); return;
	case 4:  P0_4 = value; P0DIR |= (1<<4); return;
	case 5:  P0_5 = value; P0DIR |= (1<<5); return;
	case 10: P1_0 = value; P1DIR |= (1<<0); return;
	case 11: P1_1 = value; P1DIR |= (1<<1); return;
	case 12: P1_2 = value; P1DIR |= (1<<2); return;
	case 13: P1_3 = value; P1DIR |= (1<<3); return;
	case 14: P1_4 = value; P1DIR |= (1<<4); return;
	case 15: P1_5 = value; P1DIR |= (1<<5); return;
	case 16: P1_6 = value; P1DIR |= (1<<6); return;
	case 17: P1_7 = value; P1DIR |= (1<<7); return;
	case 20: P2_0 = value; P2DIR |= (1<<0); return;
	case 21: P2_1 = value; P2DIR |= (1<<1); return;
	case 22: P2_2 = value; P2DIR |= (1<<2); return;
	case 23: P2_3 = value; P2DIR |= (1<<3); return;
	case 24: P2_4 = value; P2DIR |= (1<<4); return;
	}
}
