#ifndef __DEFINES_H__
#define __DEFINES_H__

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdint.h>

#define CB_CONCAT(x, y) x##y

#define array_len(arr) (sizeof(arr) / sizeof(arr[0]))

#include "stdafx.h"
#include <libgen.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifndef NDEBUG
    //  ISO C++ doesn't allow conversion of __FILE__ to a char * (its a const char *).
    static inline const char *__basename(const char *path) {
        char *cpath = strdup(path);
        const char *out = basename(cpath);
        free(cpath);
        return out;
    }
    #define cassert_and_ret(expr) if (!((bool)(expr))) { LOG_ERROR("[%s(%u)] Assertion failed -> %s", __basename(__FILE__), __LINE__, #expr); return; }
    #define cassert(expr) if (!((bool)(expr))) { LOG_AND_ABORT("[%s(%u)] Assertion failed -> %s", __basename(__FILE__), __LINE__, #expr); }
#else
    static inline const char *__basename(const char *path) { return NULL; }
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

// These aren't guranteed to be 32/64 bits. (I think so, I may be wrong)
typedef float f32;
typedef double f64;

// I think bool_t is defined by some libraries so just to be safe.
#ifndef bool_t
    #define bool_t bool
#endif

#ifdef __cplusplus
    }
#endif

#endif