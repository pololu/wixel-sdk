/*! \file cc2511_types.h
 * This file provides the definitions of useful data types.
 */

#ifndef _TYPES_H
#define _TYPES_H

/** An unsigned 8-bit integer.  The range of this data type is 0 to 255. **/
typedef unsigned char  uint8;

/** A signed 8-bit integer.  The range of this data type is -128 to 127. **/
typedef signed   char  int8;

/** An unsigned 16-bit integer.  The range of this data type is 0 to 65,535. **/
typedef unsigned short uint16;

/** A signed 16-bit integer.  The range of this data type is -32,768 to 32,767. **/
typedef signed   short int16;

/** An unsigned 32-bit integer.  The range of this data type is 0 to 4,294,967,295. **/
typedef unsigned long  uint32;

/** A signed 32-bit integer.  The range of this data type is -2,147,483,648 to 2,147,483,647. **/
typedef signed   long  int32;

#ifdef SDCC

/** A 1-bit value that is stored in the processor's bit-addressable memory region.
 *  The CC2511 has 16 bytes of user-defined bit-addressable memory, so you can have at
 *  most 128 BIT variables per program.
 */
typedef __bit BIT;

/** Specifies that the variable is stored in code space (flash memory).
 * This is a good choice for variables and data structures that never need to be
 * changed.
 * See the <a href="http://sdcc.sourceforge.net/doc/sdccman.html/node59.html">SDCC docs</a> for more information.
 */
#define CODE __code

/** Specifies that the variable is stored in the Fast Access Ram section of the chip,
 * also known as "Internal Ram".
 * This is a good choice for small variables that need to be accessed in many places.
 * Reading and writing DATA variables is faster than reading and writing XDATA variables.
 * There are only 256 bytes of Fast Access Ram, so use this qualifier sparingly.
 *
 * Besides being faster, another advantage of DATA over XDATA is the ability to do atomic
 * operations.  The 8051 instruction set supports several instructions
 * (INC, DEC, ORL, ANL, XRL) that let you read, modify, and write a single byte of internal
 * RAM with one instruction.
 */
#define DATA __data

/** Specifies that the variable is stored in the paged data area (the first 256 bytes
 * of XDATA).  This the default memory space for variables.  Accessing PDATA variables
 * takes less code space than accessing XDATA variables.
 */
#define PDATA __pdata

/** Specifies that the variable is stored in the Slow Access Ram section of the chip,
 * also known as "External Ram".
 * This is a good choice for large buffers or variables that don't need to be accessed
 * quickly.  The CC2511 has 3840 bytes of Slow Access Ram.
 */
#define XDATA __xdata

#elif defined(__CDT_PARSER__)

// Avoid syntax and semantic errors in eclipse.
#define CODE
#define XDATA
#define DATA
#define PDATA
typedef unsigned char BIT;
#define ISR(source, bank) void ISR_##source()
#define __reentrant

#else
#error "Unknown compiler."
#endif

#endif
