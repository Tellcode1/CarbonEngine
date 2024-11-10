#ifndef __DEFINES_H__
#define __DEFINES_H__

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdint.h>

#define CB_CONCAT(x, y) x##y

#define array_len(arr) ((int)(sizeof(arr) / sizeof(arr[0])))

#include "stdafx.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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
    #define cassert_and_ret(expr) if (!((bool)(expr))) { LOG_ERROR("[%s(%u)] Assertion failed -> %s", __basename(__FILE__), __LINE__, #expr); return; }
    #define cassert(expr) if (!((bool)(expr))) { LOG_AND_ABORT("[%s(%u)] Assertion failed -> %s", __basename(__FILE__), __LINE__, #expr); }
#else
    static inline const char *__basename(const char *path) { return NULL; }
    // These are typecasted to void because they give warnings because result (its like expr != NULL) is not used
    #define cassert_and_ret(expr) (void)(expr)
    #define cassert(expr) (void)(expr)
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

#ifdef __cplusplus
    }
#endif

#endif