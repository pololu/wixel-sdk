#ifndef _TYPES_H
#define _TYPES_H

typedef unsigned char  uint8;
typedef signed   char  int8;
typedef unsigned short uint16;
typedef signed   short int16;
typedef unsigned long  uint32;
typedef signed   long  int32;

#ifdef SDCC
#define BIT __bit

#define CODE  __code
#define XDATA __xdata
#define DATA  __data
#define PDATA __pdata

#else
#error "Unknown compiler."
#endif

// Avoid syntax errors in eclipse.
#ifdef __CDT_PARSER__
#define __xdata
#define __pdata
#define __data
#define xdata
#define pdata
#define data
#define __interrupt(x)
#define __at(x)
#define __using(x)
#define CODE
#define XDATA
#define DATA
#define PDATA
#endif


#endif
