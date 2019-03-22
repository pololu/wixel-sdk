#ifndef _OLD_PUTCHAR_H
#define _OLD_PUTCHAR_H

// In SDCC versions older than 3.7.0, putchar had a prototype of
// 'void putchar(char)'.
#if !defined(__SDCC_VERSION_MAJOR) || (__SDCC_VERSION_MAJOR == 3 && __SDCC_VERSION_MINOR < 7)
#define OLD_PUTCHAR
#endif

#endif
