#ifndef __LUNA_PRINTF_H__
#define __LUNA_PRINTF_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// The size of each buffer (on the stack) that luna_printf may allocate
// luna_printf may use at most 2 buffers.
// one is used for fast writing access and one is used when writing to a stream
// as an intermediary

// we could copy luna_vsnprintf into a vnfprintf that would stream the
// write_buffer to the stream we'll see

#ifndef LUNA_PRINTF_BUFSIZ
#define LUNA_PRINTF_BUFSIZ 1024
#endif

#ifndef LUNA_PRINTF_STREAM
#define LUNA_PRINTF_STREAM stdout
#endif

extern bool luna_is_format_specifier(char c);

// @return The number of characters written excluding the NULL terminator
extern size_t luna_itoa2(long long x, char out[], int base, size_t max);

// @return The number of characters written excluding the NULL terminator
extern size_t luna_ftoa2(double x, char out[], int precision, size_t max);

extern size_t luna_ptoa2(void *p, char *buf, size_t max);

// Bytes to ASCII
// if upgrade is enabled, 1000 bytes will be converted to 1 MB or 1000MB will be converted to 1GB
// Supports up to 1 Petabyte, you can EASILY add more levels by just adding them to the stages array
extern size_t luna_btoa2(size_t x, bool upgrade, char *buf, size_t max);

extern int luna_atoi(const char s[]);

extern double luna_atof(const char s[]);

static inline char *luna_itoa(long long x, char out[], int base, size_t max) {
  luna_itoa2(x, out, base, max);
  return out;
}

static inline char *luna_ftoa(double x, char out[], int precision, size_t max) {
  luna_ftoa2(x, out, precision, max);
  return out;
}

static inline char *luna_ptoa(void *p, char *buf, size_t max) {
  luna_ptoa2(p, buf, max);
  return buf;
}

static inline char *luna_btoa(size_t x, bool upgrade, char *buf, size_t max) {
  luna_btoa2(x, upgrade, buf, max);
  return buf;
}

// returns the numbers of characters written
extern size_t luna_printf(const char *fmt, ...);
extern size_t luna_fprintf(FILE *f, const char *fmt, ...);
extern size_t luna_nprintf(size_t max_chars, const char *fmt, ...);
extern size_t luna_sprintf(char *dest, const char *fmt, ...);
extern size_t luna_vprintf(const char *fmt, va_list args);
extern size_t luna_vfprintf(FILE *f, const char *fmt, va_list args);
extern size_t luna_snprintf(char *dest, size_t max_chars, const char *fmt, ...);
extern size_t luna_vnprintf(size_t max_chars, va_list args, const char *fmt);

// The core print function. All luna_printf* functions eventually end up calling
// this function. It is guranteed that dest will not be written to if it is NULL
// Stops formatting when it reaches max_chars
extern size_t luna_vsnprintf(char *dest, size_t max_chars, const char *fmt, va_list args);

#endif //__LUNA_PRINTF_H__