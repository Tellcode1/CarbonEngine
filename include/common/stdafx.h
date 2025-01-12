#ifndef __LUNA_STDAFX_H__
#define __LUNA_STDAFX_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOG_ERROR(err, ...)            __LOG_ERROR (__PRETTY_FUNCTION__, err, ##__VA_ARGS__)
#define LOG_AND_ABORT(err, ...)        __LOG_AND_ABORT (__PRETTY_FUNCTION__, err, ##__VA_ARGS__)
#define LOG_WARNING(err, ...)          __LOG_WARNING (__PRETTY_FUNCTION__, err, ##__VA_ARGS__)
#define LOG_INFO(err, ...)             __LOG_INFO (__PRETTY_FUNCTION__, err, ##__VA_ARGS__)
#define LOG_DEBUG(err, ...)            __LOG_DEBUG (__PRETTY_FUNCTION__, err, ##__VA_ARGS__)
#define LOG_CUSTOM(preceder, err, ...) __LOG_CUSTOM (__PRETTY_FUNCTION__, preceder, err, ##__VA_ARGS__)

// puts but with formatting and with the preceder "error". does not stop
// execution of program if you want that, use LOG_AND_ABORT instead.
void __LOG_ERROR (const char *func, const char *fmt, ...);
// formats the string, puts() it with the preceder "fatal error" and then aborts
// the program
void __LOG_AND_ABORT (const char *func, const char *fmt, ...);
// puts but with formatting and with the preceder "warning"
void __LOG_WARNING (const char *func, const char *fmt, ...);
// puts but with formatting and with the preceder "info"
void __LOG_INFO (const char *func, const char *fmt, ...);
// puts but with formatting and with the preceder "debug"
void __LOG_DEBUG (const char *func, const char *fmt, ...);

void __LOG_CUSTOM (const char *func, const char *preceder, const char *fmt, ...);

#define __WRAPPER1(x, y) CB_CONCAT (x, y)

// May god never have a look at this define. I will not be spared.
#define TIME_FUNCTION_REAL(func, LINE)                                                                                                                                                                 \
  const auto __WRAPPER1 (__COUNTER_BEGIN__, __LINE__) = SDL_GetTicks64 ();                                                                                                                             \
  func; /* Call the function*/                                                                                                                                                                         \
  LOG_DEBUG ("[Line %d] Function %s took %ldms", LINE, #func, SDL_GetTicks64 () - __WRAPPER1 (__COUNTER_BEGIN__, __LINE__), LINE);
#define TIME_FUNCTION(func) TIME_FUNCTION_REAL (func, __LINE__)

#define DEBUG

static inline struct tm *
__CG_GET_TIME ()
{
  time_t now;
  struct tm *tm;

  now = time (0);
  if ((tm = localtime (&now)) == NULL)
    {
      LOG_ERROR ("Error extracting time stuff");
      return NULL;
    }

  return tm;
}

extern void __CG_LOG (va_list args, const char *fn, const char *succeeder, const char *preceder, const char *str, unsigned char err);

#define CB_CONCAT(x, y) x##y
#define array_len(arr)  ((int)(sizeof (arr) / sizeof (arr[0])))

#ifndef NDEBUG
// we don't use libgen's basename because ISO C/C++ doesn't allow conversion of
// const char * to a char *.
static inline const char *
__basename (const char *path)
{
  const char *ret = strrchr (path, '/');
  if (ret)
    {
      return ret + 1;
    }
  else
    {
      return path;
    }
}
#define cassert_and_ret(expr)                                                                                                                                                                          \
  if (!((bool)(expr)))                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
      LOG_ERROR ("[%s:%u:%s] Assertion failed -> %s", __basename (__FILE__), __LINE__, __PRETTY_FUNCTION__, #expr);                                                                                    \
      return;                                                                                                                                                                                          \
    }
#define cassert(expr)                                                                                                                                                                                  \
  if (!((bool)(expr)))                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
      LOG_ERROR ("[%s:%u:%s] Assertion failed -> %s", __basename (__FILE__), __LINE__, __PRETTY_FUNCTION__, #expr);                                                                                    \
    }
#else
static inline const char *
__basename (const char *path)
{
  return NULL;
}
// These are typecasted to void because they give warnings because result (its
// like expr != NULL) is not used
#define cassert_and_ret(expr) (void)(expr)
#define cassert(expr)         (void)(expr)
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