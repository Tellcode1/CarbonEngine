#ifndef __LUNA_STDAFX_H__
#define __LUNA_STDAFX_H__

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// puts but with formatting and with the preceder "error". does not stop execution of program
// if you want that, use LOG_AND_ABORT instead.
void LOG_ERROR(const char * fmt, ...);
// formats the string, puts() it with the preceder "fatal error" and then aborts the program
void LOG_AND_ABORT(const char * fmt, ...);
// puts but with formatting and with the preceder "warning"
void LOG_WARNING(const char * fmt, ...);
// puts but with formatting and with the preceder "info"
void LOG_INFO(const char * fmt, ...);
// puts but with formatting and with the preceder "debug"
void LOG_DEBUG(const char * fmt, ...);

void LOG_CUSTOM(const char *preceder, const char *fmt, ...);

#define __WRAPPER1(x, y) CB_CONCAT(x, y)

// May god never have a look at this define. I will not be spared.
#define TIME_FUNCTION_REAL(func, LINE) const auto __WRAPPER1(__COUNTER_BEGIN__, __LINE__) = SDL_GetTicks64();\
							func; /* Call the function*/ \
                            LOG_DEBUG("[Line %d] Function %s took %ldms", LINE, #func, SDL_GetTicks64() - __WRAPPER1(__COUNTER_BEGIN__, __LINE__), LINE);
#define TIME_FUNCTION(func) TIME_FUNCTION_REAL(func, __LINE__)

#define DEBUG

static inline struct tm *__CG_GET_TIME() {
    time_t now;
    struct tm *tm;

    now = time(0);
    if ((tm = localtime (&now)) == NULL) {
        LOG_ERROR ("Error extracting time stuff");
        return NULL;
    }

    return tm;
}

extern void __CG_LOG(va_list args, const char *succeeder, const char *preceder, const char *str, unsigned char err);

#define CB_CONCAT(x, y) x##y
#define array_len(arr) ((int)(sizeof(arr) / sizeof(arr[0])))

#ifndef NDEBUG
    // we don't use libgen's basename because ISO C++ doesn't allow conversion of __FILE__ to a char * (its a const char *).
    static inline const char *__basename(const char *path) {
        char *ret = strrchr(path, '/');
        if (ret) {
            return ret + 1;
        } else {
            return path;
        }
    }
    #define cassert_and_ret(expr) if (!((bool)(expr))) { LOG_ERROR("[%s:%u:%s] Assertion failed -> %s", __basename(__FILE__), __LINE__, __PRETTY_FUNCTION__, #expr); return; }
    #define cassert(expr) if (!((bool)(expr))) { LOG_ERROR("[%s:%u:%s] Assertion failed -> %s", __basename(__FILE__), __LINE__, __PRETTY_FUNCTION__, #expr); }
#else
    static inline const char *__basename(const char *path) { return NULL; }
    // These are typecasted to void because they give warnings because result (its like expr != NULL) is not used
    #define cassert_and_ret(expr) (void)(expr)
    #define cassert(expr) (void)(expr)
    #pragma message "Assertions disabled"
#endif

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef uint8_t uchar;
typedef int8_t sbyte;
typedef uint8_t ubyte;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

// They ARE 32 and 64 bits by IEEE-754 but aren't set by the standard
// But there is a 99.9% chance that they will be
typedef float f32;
typedef double f64;


#endif