#include "../include/sys/io/printf.h"
#include "../include/defines.h"
#include "../include/math/math.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

void __CG_LOG(va_list args, const char *succeeder, const char *preceder, const char *str, unsigned char err) {
  FILE *out = (err) ? stderr : stdout;
  
  struct tm *time = __CG_GET_TIME();

  luna_printf("[%d:%d:%d] ", time->tm_hour % 12, time->tm_min, time->tm_sec);

  luna_vfprintf(out, preceder, args);
  luna_vfprintf(out, str, args);
  luna_vfprintf(out, succeeder, args);
}

//printf
char *luna_itoa(int x, char out[], unsigned base) {
  cassert(base != 0);
  cassert(out != NULL);

  size_t pwr = 1, i = 0;

  if (x == 0) {
    out[i++] = '0';
    return out;
  }

  if (x < 0 && base == 10) {
    out[i++] = '-';
    x = -x;
  }

  // get the highest power of 10 in the number
  while ((x / pwr) >= base) {
    pwr *= base;
  }

  while (pwr > 0) {
    int dig = x / pwr;
    out[i++] = dig + '0';
    x %= pwr;
    pwr /= base;
  }
  out[i] = 0;
  return out;
}

char *luna_ftoa(double n, char s[], int precision) {
    // handle special cases
    if (isnan(n)) {
        strcpy(s, "nan");
    } else if (isinf(n)) {
        strcpy(s, "inf");
    } else if (n == 0.0) {
        strcpy(s, "0");
    } else {
        int digit, m, m1;
        char *c = s;
        int neg = (n < 0);
        if (neg)
            n = -n;
        // calculate magnitude
        m = log10(n);
        int useExp = (m >= 14 || (neg && m >= 9) || m <= -9);
        if (neg)
            *(c++) = '-';
        // set up for scientific notation
        if (useExp) {
            if (m < 0)
               m -= 1.0;
            n = n / pow(10.0, m);
            m1 = m;
            m = 0;
        }
        if (m < 1.0) {
            m = 0;
        }
        // convert the number
        const double proc = 1.0 / pow(10.0, precision);
        while (n > proc || m >= 0) {
            double weight = pow(10.0, m);
            if (weight > 0 && !isinf(weight)) {
                digit = floor(n / weight);
                n -= (digit * weight);
                *(c++) = '0' + digit;
            }
            if (m == 0 && n > 0)
                *(c++) = '.';
            m--;
        }
        if (useExp) {
            // convert the exponent
            int i, j;
            *(c++) = 'e';
            if (m1 > 0) {
                *(c++) = '+';
            } else {
                *(c++) = '-';
                m1 = -m1;
            }
            m = 0;
            while (m1 > 0) {
                *(c++) = '0' + m1 % 10;
                m1 /= 10;
                m++;
            }
            c -= m;
            for (i = 0, j = m-1; i<j; i++, j--) {
                // swap without temporary
                c[i] ^= c[j];
                c[j] ^= c[i];
                c[i] ^= c[j];
            }
            c += m;
        }
        *(c) = '\0';
    }
    return s;
}

// Returns __INT32_MAX__ on error.
int luna_atoi(const char str[]) {
  const char *begin, *end;

  begin = str;
  end = str + strlen(str) - 1;

  int ret = 0;
  int power = 1;

  bool neg = 0;
  if (*begin == '-') {
    neg = 1;
    begin++;
  } else if (*begin == '+') {
    begin++;
  }
  
  while (end >= begin) {
    int digit = *end - '0';
    if (digit < 0 || digit > 9) {
      return __INT32_MAX__;
    }

    ret += digit * power;
    power *= 10;
    end--;
  }

  if (neg) {
    ret *= -1;
  }

  return ret;
}

double luna_atof(const char str[]) {
  double result = 0.0, fraction = 0.0;
  int divisor = 1;
  bool neg = 0;
  const char *i = str;

  while (isspace(*i)) i++;

  if (*i == '-') {
    neg = 1;
    i++;
  } else if (*i == '+') {
    i++;
  }

  while (isdigit(*i)) {
    result = result * 10 + (*i - '0');
    i++;
  }

  if (*i == '.') {
    i++;
    while (isdigit(*i)) {
      fraction = fraction * 10 + (*i - '0');
      divisor *= 10;
      i++;
    }
    result += fraction / divisor;
  }

  if (*str == 'e' || *i == 'E') {
    i++;
    int exp_sign = 1;
    int exponent = 0;

    if (*i == '-') {
      exp_sign = -1;
      i++;
    } else if (*str == '+') {
      i++;
    }

    while (isdigit(*i)) {
      exponent = exponent * 10 + (*i - '0');
      i++;
    }

    result = ldexp(result, exp_sign * exponent);
  }

  if (neg) {
    result *= -1.0;
  }

  return result;
}

size_t luna_printf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t chars_written = luna_vnprintf(SIZE_MAX, args, fmt);

  va_end(args);

  return chars_written;
}

size_t luna_fprintf(FILE *f, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  char buf[ LUNA_PRINTF_BUFSIZ ];
  size_t chars_written = luna_vsnprintf(buf, SIZE_MAX, fmt, args);

  fputs(buf, f);

  va_end(args);

  return chars_written;
}

size_t luna_sprintf(char *dest, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = luna_vsnprintf(dest, SIZE_MAX, fmt, args);

  va_end(args);

  return chars_written;
}

size_t luna_vprintf(const char *fmt, va_list args)
{
  char buf[ LUNA_PRINTF_BUFSIZ ];

  size_t chars_written = luna_vsnprintf(buf, SIZE_MAX, fmt, args);

  fputs(buf, LUNA_PRINTF_STREAM);

  return chars_written;
}

size_t luna_vfprintf(FILE *f, const char *fmt, va_list args)
{
  char buf[ LUNA_PRINTF_BUFSIZ ];
  size_t chars_written = luna_vsnprintf(buf, SIZE_MAX, fmt, args);

  fputs(buf, f);
  return chars_written;
}

size_t luna_snprintf(char *dest, size_t max_chars, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = luna_vsnprintf(dest, max_chars, fmt, args);

  va_end(args);

  return chars_written;
}

size_t luna_nprintf(size_t max_chars, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  size_t chars_written = luna_vnprintf(max_chars, args, fmt);

  va_end(args);

  return chars_written;
}

size_t luna_vnprintf(size_t max_chars, va_list args, const char *fmt) {
  char buf[ LUNA_PRINTF_BUFSIZ ];
  size_t chars_written = luna_vsnprintf(buf, max_chars, fmt, args);

  fputs(buf, LUNA_PRINTF_STREAM);

  return chars_written;
}

size_t luna_vsnprintf(char *dest, size_t max_chars, const char *fmt, va_list args) {
  size_t chars_written = 0;
  char out_buffer[LUNA_PRINTF_BUFSIZ] = {0};
  char write_buffer[LUNA_PRINTF_BUFSIZ] = {0};

  va_list args_copy;
  va_copy(args_copy, args);

  int n;
  const char *s;
  double f;
  const char *iter = fmt;

  for (;*iter && chars_written < max_chars; iter++) {
    if (*iter == '%') {
      iter++;

      switch (*iter)
      {
      case '.': {
        iter++;
        int precision = 6;

        if (*iter == '*') {
          precision = va_arg(args_copy, int);
          iter++;
        } else {
          if (isdigit(*iter)) {
            precision = 0;
            while (isdigit(*iter)) {
              precision = precision * 10 + (*iter - '0');
              iter++;
            }
          }
        }

        f = va_arg(args_copy, double);
        luna_ftoa(f, write_buffer, precision);
        break;
      }
      case 'f': {
        f = va_arg(args_copy, double);
        luna_ftoa(f, write_buffer, 6);
        break;
      }
      case 'l':
        iter++;
      case 'd':
      case 'i':
        n = va_arg(args_copy, int);
        luna_itoa(n, write_buffer, 10);
        break;
      case 's':
        s = va_arg(args_copy, const char *);
        strncpy(write_buffer, s, sizeof(write_buffer) - 1);
        write_buffer[sizeof(write_buffer) - 1] = '\0';
        break;
      case '%':
        write_buffer[0] = '%';
        write_buffer[1] = '\0';
        break;
      default:
        write_buffer[0] = '\0';
        break;
      }

      size_t len = strlen(write_buffer);
      if (chars_written + len < max_chars) {
        strncat(out_buffer, write_buffer, sizeof(out_buffer) - strlen(out_buffer) - 1);
        chars_written += len;
      } else {
        strncat(out_buffer, write_buffer, max_chars - chars_written);
        chars_written = max_chars;
      }
    } else {
      if (chars_written + 1 < max_chars) {
        size_t len = strlen(out_buffer);
        out_buffer[len] = *iter;
        out_buffer[len + 1] = '\0';
        chars_written++;
      }
    }
  }

  va_end(args_copy);

  if (dest) {
    size_t written = (chars_written < max_chars) ? chars_written : max_chars - 1;
    strncpy(dest, out_buffer, written);
    dest[written] = '\0';
  }

  return chars_written;
}
// printf

void LOG_ERROR(const char * fmt, ...) {
  const char * preceder = "error: ";
  const char * succeeder = "\n";
  va_list args;
  va_start(args, fmt);
  __CG_LOG(args, succeeder, preceder, fmt, 1);
  va_end(args);
}


void LOG_AND_ABORT(const char * fmt, ...) {
  const char * preceder = "fatal error: ";
  const char * succeeder = "\nabort.\n";
  va_list args;
  va_start(args, fmt);
  __CG_LOG(args, succeeder, preceder, fmt, 1);
  va_end(args);
  abort();
}

void LOG_WARNING(const char * fmt, ...) {
  const char * preceder =  "warning: ";
  const char * succeeder = "\n";
  va_list args;
  va_start(args, fmt); 
  __CG_LOG(args, succeeder, preceder, fmt, 0);
  va_end(args);
}

void LOG_INFO(const char * fmt, ...) {
  const char * preceder =  "info: ";
  const char * succeeder = "\n";
  va_list args;
  va_start(args, fmt); 
  __CG_LOG(args, succeeder, preceder, fmt, 0);
  va_end(args);
}

void LOG_DEBUG(const char * fmt, ...) {
  const char * preceder =  "debug: ";
  const char * succeeder = "\n";
  va_list args;
  va_start(args, fmt); 
  __CG_LOG(args, succeeder, preceder, fmt, 0);
  va_end(args);
}

void LOG_CUSTOM(const char *preceder, const char *fmt, ...) {
  const char * succeeder = "\n";
  va_list args;
  va_start(args, fmt); 
  __CG_LOG(args, succeeder, preceder, fmt, 0);
  va_end(args);
}