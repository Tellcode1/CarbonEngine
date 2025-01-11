#ifndef __LUNA_PRINTF_H__
#define __LUNA_PRINTF_H__

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>

// The size of each buffer (on the stack) that luna_printf may allocate
// luna_printf may use at most 2 buffers.
// one is used for fast writing access and one is used when writing to a stream as an intermediary

// we could copy luna_vsnprintf into a vnfprintf that would stream the write_buffer to the stream
// we'll see

#ifndef LUNA_PRINTF_BUFSIZ
  #define LUNA_PRINTF_BUFSIZ 1024
#endif

#ifndef LUNA_PRINTF_STREAM
  #define LUNA_PRINTF_STREAM stdout
#endif

extern bool luna_is_format_specifier(char c);

// out should be around 12 bytes (10 for __INT32_MAX__ and 1 for negative sign and 1 for terminator)
extern char *luna_itoa(long long x, char out[], int base, size_t max);

// @return The number of characters written excluding the NULL terminator
extern size_t luna_itoa2(long long x, char out[], int base, size_t max);

extern char *luna_ftoa(double x, char out[], int precision, size_t max);

// @return The number of characters written excluding the NULL terminator
extern size_t luna_ftoa2(double x, char out[], int precision, size_t max);

extern int luna_atoi(const char str[]);

extern double luna_atof(const char str[]);

extern size_t luna_ptoa(void* p, char *buf, size_t max);

// returns the numbers of characters written
extern size_t luna_printf(const char *fmt, ...);
extern size_t luna_fprintf(FILE *f, const char *fmt, ...);
extern size_t luna_nprintf(size_t max_chars, const char *fmt, ...);
extern size_t luna_sprintf(char *dest, const char *fmt, ...);
extern size_t luna_vprintf(const char *fmt, va_list args);
extern size_t luna_vfprintf(FILE *f, const char *fmt, va_list args);
extern size_t luna_snprintf(char *dest, size_t max_chars, const char *fmt, ...);
extern size_t luna_vnprintf(size_t max_chars, va_list args, const char *fmt);

// The core print function. All luna_printf* functions eventually end up calling this function.
// It is guranteed that dest will not be written to if it is NULL
// Stops formatting when it reaches max_chars
extern size_t luna_vsnprintf(char *dest, size_t max_chars, const char *fmt, va_list args);

#endif//__LUNA_PRINTF_H__