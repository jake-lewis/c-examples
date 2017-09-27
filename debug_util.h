#ifndef C_EXAMPLES_DEBUG_UTIL_H
#define C_EXAMPLES_DEBUG_UTIL_H

#ifndef _STDIO_H_
#include <stdio.h>
#endif

#ifndef _STDARG_H
#include <stdarg.h>
#endif

#ifndef _STDBOOL_H
#include <stdbool.h>
#endif

//int printDebug(const char*, ...);
#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif

#define printDebug(fmt, ...) do { if (DEBUG_TEST) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)

#endif //C_EXAMPLES_DEBUG_UTIL_H
