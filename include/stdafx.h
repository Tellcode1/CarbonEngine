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

static inline void __CG_LOG(va_list args, const char *succeeder, const char *preceder, const char *str, unsigned char err) {
    FILE *out = (err) ? stderr : stdout;
    vfprintf(out, preceder, args);
    vfprintf(out, str, args);
    vfprintf(out, succeeder, args);
}

static inline void LOG_ERROR(const char * fmt, ...) {
    const char * preceder = "error: ";
    const char * succeeder = "\n";
    va_list args;
    va_start(args, fmt);
    __CG_LOG(args, succeeder, preceder, fmt, 1);
    va_end(args);
}

static inline void LOG_AND_ABORT(const char * fmt, ...) {
    const char * preceder = "fatal error: ";
    const char * succeeder = "\nabort.\n";
    va_list args;
    va_start(args, fmt);
    __CG_LOG(args, succeeder, preceder, fmt, 1);
    va_end(args);
    abort();
}

static inline void LOG_WARNING(const char * fmt, ...) {
    const char * preceder =  "warning: ";
    const char * succeeder = "\n";
    va_list args;
    va_start(args, fmt); 
    __CG_LOG(args, succeeder, preceder, fmt, 0);
    va_end(args);
}

static inline void LOG_INFO(const char * fmt, ...) {
    const char * preceder =  "info: ";
    const char * succeeder = "\n";
    va_list args;
    va_start(args, fmt); 
    __CG_LOG(args, succeeder, preceder, fmt, 0);
    va_end(args);
}

static inline void LOG_DEBUG(const char * fmt, ...) {
    const char * preceder =  "debug: ";
    const char * succeeder = "\n";
    va_list args;
    va_start(args, fmt); 
    __CG_LOG(args, succeeder, preceder, fmt, 0);
    va_end(args);
}

#undef __LOG

#endif