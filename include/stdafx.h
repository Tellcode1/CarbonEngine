#ifndef __PCH__
#define __PCH__

#include "defines.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define __WRAPPER1(x, y) CB_CONCAT(x, y)

// May god never have a look at this define. I will not be spared.
#define TIME_FUNCTION_REAL(func, LINE) const auto __WRAPPER1(__COUNTER_BEGIN__, __LINE__) = SDL_GetTicks64();\
							func; /* Call the function*/ \
                            LOG_DEBUG("[Line %d] Function %s took %ldms", LINE, #func, SDL_GetTicks64() - __WRAPPER1(__COUNTER_BEGIN__, __LINE__), LINE);
#define TIME_FUNCTION(func) TIME_FUNCTION_REAL(func, __LINE__)

#define DEBUG

#define __LOG()     va_list args; \
                    va_start(args, fmt); \
                    vfprintf(stderr, preceder, args); \
                    vfprintf(stderr, fmt, args); \
                    vfprintf(stderr, succeeder, args); \
                    va_end(args)

inline void LOG_ERROR(const char * fmt, ...) {
    const char * preceder = "error: ";
    const char * succeeder = "\n";
    __LOG();
}

inline void LOG_AND_ABORT(const char * fmt, ...) {
    const char * preceder = "fatal error: ";
    const char * succeeder = "\nabort.\n";
    __LOG();
    abort();
}

inline void LOG_WARNING(const char * fmt, ...) {
    const char * preceder =  "warning: ";
    const char * succeeder = "\n";
    __LOG();
}

inline void LOG_INFO(const char * fmt, ...) {
    const char * preceder =  "info: ";
    const char * succeeder = "\n";
    __LOG();
}

inline void LOG_DEBUG(const char * fmt, ...) {
    const char * preceder =  "debug: ";
    const char * succeeder = "\n";
    __LOG();
}

#undef __LOG

inline u32 align(u32 size, u32 alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

#endif