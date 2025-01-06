#ifndef __LUNA_PRINTF_H__
#define __LUNA_PRINTF_H__

#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>

// The size of each buffer (on the stack) that luna_printf may allocate
// luna_printf may use at most 3 buffers.
#ifndef LUNA_PRINTF_BUFSIZ
  #define LUNA_PRINTF_BUFSIZ 1024
#endif

#ifndef LUNA_PRINTF_STREAM
  #define LUNA_PRINTF_STREAM stdout
#endif

// out should be around 12 bytes (10 for __INT32_MAX__ and 1 for negative sign and 1 for terminator)
extern char *luna_itoa(int x, char out[], unsigned base);

extern char *luna_ftoa(double x, char out[], int precision);

extern int luna_atoi(const char str[]);

extern double luna_atof(const char str[]);

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