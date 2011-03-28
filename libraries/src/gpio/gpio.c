#include <cc2511_types.h>
#include <cc2511_map.h>
#include <gpio.h>

#define PIN_SWITCH(operation) switch(pin) { \
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

#define SET_DIGITAL_OUTPUT(port, pin) { P##port##_##pin = value; P##port##DIR |= (1<<pin); }

void setDigitalOutput(uint8 pin, BIT value) __reentrant
{
	PIN_SWITCH(SET_DIGITAL_OUTPUT);
}
