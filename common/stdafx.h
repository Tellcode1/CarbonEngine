#ifndef __NOVA_STDAFX_H__
#define __NOVA_STDAFX_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef __cplusplus
#define NOVA_HEADER_START                                                                                                                                                     \
  extern "C"                                                                                                                                                                  \
  {
#define NOVA_HEADER_END }
#else
#define NOVA_HEADER_START
#define NOVA_HEADER_END
#endif

NOVA_HEADER_START;

#define DEBUG

#define NV_MAX(a, b) ((a) > (b) ? (a) : (b))
#define NV_MIN(a, b) ((a) < (b) ? (a) : (b))

// puts but with formatting and with the preceder "error". does not stop
// execution of program if you want that, use NV_LOG_AND_ABORT instead.
#define NV_LOG_ERROR(err, ...) _NV_LOG_ERROR(__PRETTY_FUNCTION__, err, ##__VA_ARGS__)

// formats the string, puts() it with the preceder "fatal error" and then aborts
// the program
#define NV_LOG_AND_ABORT(err, ...) _NV_LOG_AND_ABORT(__PRETTY_FUNCTION__, err, ##__VA_ARGS__)

// puts but with formatting and with the preceder "warning"
#define NV_LOG_WARNING(err, ...) _NV_LOG_WARNING(__PRETTY_FUNCTION__, err, ##__VA_ARGS__)

// puts but with formatting and with the preceder "info"
#define NV_LOG_INFO(err, ...) _NV_LOG_INFO(__PRETTY_FUNCTION__, err, ##__VA_ARGS__)

// puts but with formatting and with the preceder "debug"
#define NV_LOG_DEBUG(err, ...) _NV_LOG_DEBUG(__PRETTY_FUNCTION__, err, ##__VA_ARGS__)

// puts but with formatting and with a custom preceder
#define NV_LOG_CUSTOM(preceder, err, ...) _NV_LOG_CUSTOM(__PRETTY_FUNCTION__, preceder, err, ##__VA_ARGS__)

extern void _NV_LOG_ERROR(const char* func, const char* fmt, ...);
extern void _NV_LOG_AND_ABORT(const char* func, const char* fmt, ...);
extern void _NV_LOG_WARNING(const char* func, const char* fmt, ...);
extern void _NV_LOG_INFO(const char* func, const char* fmt, ...);
extern void _NV_LOG_DEBUG(const char* func, const char* fmt, ...);
extern void _NV_LOG_CUSTOM(const char* func, const char* preceder, const char* fmt, ...);

#define __WRAPPER1(x, y) NV_CONCAT(x, y)

// May god never have a look at this define. I will not be spared.
#define _NV_TIME_FUNCTION(func, LINE)                                                                                                                                         \
  const auto __WRAPPER1(__COUNTER_BEGIN__, __LINE__) = SDL_GetTicks64();                                                                                                      \
  func; /* Call the function*/                                                                                                                                                \
  NV_LOG_DEBUG("[Line %d] Function %s took %ldms", LINE, #func, SDL_GetTicks64() - __WRAPPER1(__COUNTER_BEGIN__, __LINE__), LINE);
#define NV_TIME_FUNCTION(func) _NV_TIME_FUNCTION(func, __LINE__)

static inline struct tm*
_NV_GET_TIME()
{
  time_t     now;
  struct tm* tm;

  now = time(0);
  if ((tm = localtime(&now)) == NULL)
  {
    NV_LOG_ERROR("Error extracting time stuff");
    return NULL;
  }

  return tm;
}

extern void _NV_LOG(va_list args, const char* fn, const char* succeeder, const char* preceder, const char* str, unsigned char err);

#define NV_CONCAT(x, y) x##y
#define NV_arrlen(arr) ((size_t)(sizeof(arr) / sizeof(arr[0])))

#ifndef NDEBUG
#define NV_assert_and_ret(expr, retval)                                                                                                                                       \
  if (!((bool)(expr)))                                                                                                                                                        \
  {                                                                                                                                                                           \
    NV_LOG_AND_ABORT("Assertion failed -> %s", #expr);                                                                                                                        \
    return retval;                                                                                                                                                            \
  }
#define NV_assert(expr)                                                                                                                                                       \
  if (!((bool)(expr)))                                                                                                                                                        \
  {                                                                                                                                                                           \
    NV_LOG_AND_ABORT("Assertion failed -> %s", #expr);                                                                                                                        \
  }
#else
// These are typecasted to void because they give warnings because result (its
// like expr != NULL) is not used
#define NV_assert_and_ret(expr, retval) (void)(expr)
#define NV_assert(expr) (void)(expr)
#pragma message "Assertions disabled"
#endif

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef uint8_t  uchar;
typedef int8_t   sbyte;
typedef uint8_t  ubyte;

typedef int64_t  i64;
typedef int32_t  i32;
typedef int16_t  i16;
typedef int8_t   i8;

// They ARE 32 and 64 bits by IEEE-754 but aren't set by the standard
// But there is a 99.9% chance that they will be
typedef float  f32;
typedef double f64;

NOVA_HEADER_END;

#endif
