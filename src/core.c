#include <SDL2/SDL.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "../common/containers/atlas.h"
#include "../common/containers/bitset.h"
#include "../common/containers/hashmap.h"
#include "../common/containers/string.h"
#include "../common/containers/vector.h"
#include "../common/mem.h"
#include "../common/printf.h"
#include "../common/stdafx.h"
#include "../common/string.h"
#include "../include/engine/image.h"

#define WRITING_MAX_FOR_THE_23RD_TIME(a, b) ((a) > (b) ? (a) : (b))
#define _min(a, b) ((a) < (b) ? (a) : (b))

#define ALIGN_UP(ptr, alignment) ((void*)(((uintptr_t)(ptr) + ((alignment) - 1)) & ~((alignment) - 1)))
#define ALIGN_UP_SIZE(size, align) (((size) + (align) - 1) & ~((align) - 1))

static FILE*  g_stdstream   = NULL;
static char*  g_writebuf    = NULL;
static size_t g_writebufsiz = NOVA_PRINTF_BUFSIZ;

void
_NV_LOG(va_list args, const char* fn, const char* succeeder, const char* preceder, const char* s, unsigned char err)
{
  FILE*      out  = (err) ? stderr : stdout;

  struct tm* time = _NV_GET_TIME();

  NV_fprintf(out, "[%d:%d:%d] %s(): %s", time->tm_hour % 12, time->tm_min, time->tm_sec, fn, preceder);

  NV_vfprintf(out, s, args);
  NV_fprintf(out, "%s", succeeder);
}

// printf

void
NV_setprintbuf(char* buf, size_t size)
{
  NV_assert(size > 0);
  if (g_writebuf)
    NV_free(g_writebuf);

  if (!buf)
  {
    g_writebufsiz = NOVA_PRINTF_BUFSIZ;
    g_writebuf    = NV_malloc(NOVA_PRINTF_BUFSIZ);
  }
  else
  {
    g_writebufsiz = size;
    g_writebuf    = buf;
  }
}

void
NV_setstdout(FILE* stream)
{
  g_stdstream = stream;
}

bool
NV_is_format_specifier(char c)
{
  // c == 'l' isn't technically a format specifier, it's generally followd by
  // one.
  return (c == 'f') || (c == 'i') || (c == 'd') || (c == 'u') || (c == 'l') || (c == 'p') || (c == 's') || (c == 'L');
}

// This works for non base 10 integers, unlike the original
static inline long long
get_highest_pwr(long long x, int base)
{
  long long pwr = 1;
  while (pwr * base <= x)
  {
    pwr *= base;
  }
  return pwr;
}

size_t
NV_itoa2(long long x, char out[], int base, size_t max)
{
  NV_assert(base >= 2 && base <= 36);
  NV_assert(out != NULL);

  if (max == 0)
  {
    return 0; // this shouldn't be an error
  }
  else if (max == 1)
  {
    out[0] = 0;
    return 0;
  }

  // now, max should atleast be 1

  if (x == 0)
  {
    return NV_strncpy2(out, "0", max);
  }

  long long pwr = get_highest_pwr(x, base), i = 0;

  if (x < 0 && base == 10)
  {
    out[i++] = '-';
    x        = -x;
  }

  // we need 1 space for null terminator!!
  const long long imax = max - 1;
  while (pwr > 0)
  {
    if (i >= imax)
    {
      out[i] = 0;
      return i;
    }
    int dig = x / pwr;
    // this is for non base 10 digits
    // it'll still work for base 10 though
    out[i++] = (dig < 10) ? '0' + dig : 'A' + (dig - 10);
    // lmao I accidentally removed the code because it wasn't working...
    x %= pwr;
    pwr /= base;
  }
  out[i++] = 0;
  return i - 1;
}

#define NOVA_FTOA_HANDLE_CASE(fn, n, str)                                                                                                                                     \
  if (fn(n))                                                                                                                                                                  \
  {                                                                                                                                                                           \
    if (signbit(n) == 0)                                                                                                                                                      \
      return NV_strncpy2(s, str, max);                                                                                                                                      \
    else                                                                                                                                                                      \
      return NV_strncpy2(s, "-" str, max);                                                                                                                                  \
  }

// WARNING::: I didn't write most of this, stole it from stack overflow.
// if it explodes your computer its your fault!!!
size_t
NV_ftoa2(double n, char s[], int precision, size_t max, bool remove_zeros)
{
  if (max == 0)
  {
    return 0;
  }
  else if (max == 1)
  {
    s[0] = 0;
    return 0;
  }

  NOVA_FTOA_HANDLE_CASE(isnan, n, "nan");
  NOVA_FTOA_HANDLE_CASE(isinf, n, "inf");
  NOVA_FTOA_HANDLE_CASE(0.0 ==, n, "0.0");

  int   digit, m, m1;
  char* c   = s;
  int   neg = (n < 0);
  if (neg)
  {
    n = -n;
  }

  m          = log10(n);
  int useExp = (m >= 14 || (neg && m >= 9) || m <= -9);
  if (neg)
  {
    *(c++) = '-';
  }

  if (useExp)
  {
    if (m < 0)
      m -= 1.0;
    n  = n / pow(10.0, m);
    m1 = m;
    m  = 0;
  }

  if (m < 1.0)
  {
    m = 0;
  }

  const double proc               = 1.0 / pow(10.0, precision);
  bool         decimal_point_seen = false;

  while (n > proc || m >= 0)
  {
    double weight = pow(10.0, m);
    if (weight > 0 && !isinf(weight))
    {
      digit = floor(n / weight);
      n -= (digit * weight);
      *(c++) = '0' + digit;
    }

    if (m == 0 && n > 0 && !decimal_point_seen)
    {
      *(c++)             = '.';
      decimal_point_seen = true;
    }

    m--;
  }

  // remove useless zeros if user asked for it.
  if (remove_zeros && decimal_point_seen)
  {
    while (*(c - 1) == '0')
    {
      c--;
    }
    // if theres no digit after the decimal, remove the decimal as well
    if (*(c - 1) == '.')
    {
      c--;
    }
  }

  // scientific notation
  if (useExp)
  {
    int i, j;
    *(c++) = 'e';
    if (m1 > 0)
    {
      *(c++) = '+';
    }
    else
    {
      *(c++) = '-';
      m1     = -m1;
    }

    m = 0;
    while (m1 > 0)
    {
      *(c++) = '0' + m1 % 10;
      m1 /= 10;
      m++;
    }
    c -= m;

    for (i = 0, j = m - 1; i < j; i++, j--)
    {
      // swap without temporary
      c[i] ^= c[j];
      c[j] ^= c[i];
      c[i] ^= c[j];
    }

    c += m;
  }

  *c = 0;
  return c - s;
}

int
NV_atoi(const char s[])
{
  const char* i   = s;
  int         ret = 0;

  while (isspace(*i))
  {
    i++;
  }

  bool neg = 0;
  if (*i == '-')
  {
    neg = 1;
    i++;
  }
  else if (*i == '+')
  {
    i++;
  }

  while (*i)
  {
    if (!isdigit(*i))
    {
      break;
    }

    int digit = *i - '0';
    ret       = ret * 10 + digit;

    i++;
  }

  if (neg)
  {
    ret *= -1;
  }

  return ret;
}

double
NV_atof(const char s[])
{
  double      result = 0.0, fraction = 0.0;
  int         divisor = 1;
  bool        neg     = 0;
  const char* i       = s;

  while (isspace(*i))
    i++;

  if (*i == '-')
  {
    neg = 1;
    i++;
  }
  else if (*i == '+')
  {
    i++;
  }

  while (isdigit(*i))
  {
    result = result * 10 + (*i - '0');
    i++;
  }

  if (*i == '.')
  {
    i++;
    while (isdigit(*i))
    {
      fraction = fraction * 10 + (*i - '0');
      divisor *= 10;
      i++;
    }
    result += fraction / divisor;
  }

  if (*s == 'e' || *i == 'E')
  {
    i++;
    int exp_sign = 1;
    int exponent = 0;

    if (*i == '-')
    {
      exp_sign = -1;
      i++;
    }
    else if (*s == '+')
    {
      i++;
    }

    while (isdigit(*i))
    {
      exponent = exponent * 10 + (*i - '0');
      i++;
    }

    result = ldexp(result, exp_sign * exponent);
  }

  if (neg)
  {
    result *= -1.0;
  }

  return result;
}

size_t
NV_ptoa2(void* p, char* buf, size_t max)
{
  if (p == NULL)
  {
    NV_strncpy(buf, "null", max);
  }

  unsigned long addr   = (unsigned long)p;
  char          digs[] = "0123456789abcdef";

  size_t        w      = 0;

  w += NV_strncpy2(buf, "0x", max);

  for (int i = (sizeof(addr) * 2) - 1; i >= 0 && w < max - 1; i--)
  {
    int dig = (addr >> (i * 4)) & 0xF;
    buf[w]  = digs[dig];
    w++;
  }
  buf[w] = 0;
  return w;
};

size_t
NV_btoa2(size_t x, bool upgrade, char* buf, size_t max)
{
  size_t written = 0;
  if (upgrade)
  {
    const char* stages[] = { " B", " KB", " MB", " GB", " TB", " PB" };
    double      b        = (double)x;
    int         stagei   = 0; // we could use log here but that'd be overkill
    while (b >= 1000.0)
    {
      stagei++;
      b /= 1000.0;
    }
    if (stagei >= 5)
    {
      NV_LOG_ERROR("btoa: x is too big. x cannot be greater than 1000 petabytes. No "
                "bytes have been written");
      return 0;
    }
    written = NV_ftoa2(b, buf, 3, max, 1);
    buf += written;
    max -= written;
    written += NV_strncpy2(buf, stages[stagei], max);
  }
  else
  {
    written = NV_itoa2(x, buf, 10, max);
  }
  return written;
}

// Moral of the story? FU@# SIZE_MAX
// I spent an HOUR trying to figure out what's going wrong
// and I didn't even bat an eye towards it

size_t
NV_printf(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = NV_vnprintf(g_writebufsiz, args, fmt);

  va_end(args);

  return chars_written;
}

size_t
NV_fprintf(FILE* f, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  char   buf[g_writebufsiz];
  size_t chars_written = NV_vsnprintf(buf, g_writebufsiz, fmt, args);

  fputs(buf, f);

  va_end(args);

  return chars_written;
}

size_t
NV_sprintf(char* dest, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = NV_vsnprintf(dest, g_writebufsiz, fmt, args);

  va_end(args);

  return chars_written;
}

size_t
NV_vprintf(const char* fmt, va_list args)
{
  char   buf[g_writebufsiz];
  size_t chars_written = NV_vsnprintf(buf, g_writebufsiz, fmt, args);
  fputs(buf, g_stdstream);
  return chars_written;
}

size_t
NV_vfprintf(FILE* f, const char* fmt, va_list args)
{
  char   buf[g_writebufsiz];
  size_t chars_written = NV_vsnprintf(buf, g_writebufsiz, fmt, args);

  fputs(buf, f);
  return chars_written;
}

size_t
NV_snprintf(char* dest, size_t max_chars, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  if (max_chars >= g_writebufsiz)
  {
    max_chars = g_writebufsiz;
  }

  size_t chars_written = NV_vsnprintf(dest, max_chars, fmt, args);

  va_end(args);

  return chars_written;
}

size_t
NV_nprintf(size_t max_chars, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = NV_vnprintf(max_chars, args, fmt);

  va_end(args);

  return chars_written;
}

size_t
NV_vnprintf(size_t max_chars, va_list args, const char* fmt)
{
  if (max_chars > g_writebufsiz)
  {
    max_chars = g_writebufsiz;
  }

  char   buf[g_writebufsiz];
  size_t chars_written = NV_vsnprintf(buf, max_chars, fmt, args);

  fputs(buf, g_stdstream);

  return chars_written;
}

void
NV_printf_write_to_buf(char** write, size_t* chars_written, size_t max_chars, const char* write_buffer, size_t written)
{
  if (*write && written > 0)
  {
    NV_strncpy(*write, write_buffer, max_chars - (*chars_written));
    (*write) += written;
  }
  (*chars_written) += written;
}

size_t
NV_vsnprintf(char* dest, size_t max_chars, const char* fmt, va_list src)
{
  NV_assert(max_chars <= g_writebufsiz && "Can't write with smaller buffer size than max_chars");

  if (!g_writebuf)
  {
    g_writebuf = NV_malloc(g_writebufsiz);
  }
  if (!g_stdstream)
  {
    g_stdstream = stdout;
  }

  size_t            chars_written = 0;
  size_t            written       = 0;

  char*             writep        = dest;

  const char* const dest_end      = dest + max_chars;

  int64_t           n;
  const char*       s;
  double            f;
  const char*       iter = fmt;
  void*             p;

  va_list           args;
  va_copy(args, src);

  for (; *iter && chars_written < max_chars; iter++)
  {
    if (*iter == '%')
    {
      iter++;

      switch (*iter)
      {
        case '.':
        {
          iter++;
          int precision = 6;

          if (*iter == '*')
          {
            precision = va_arg(args, int);
            iter++;
          }
          else
          {
            if (isdigit(*iter))
            {
              precision = 0;
              while (isdigit(*iter))
              {
                precision = precision * 10 + (*iter - '0');
                iter++;
              }
            }
          }

          f       = va_arg(args, double);
          written = NV_ftoa2(f, g_writebuf, precision, max_chars - chars_written, 0);
          NV_printf_write_to_buf(&writep, &chars_written, max_chars, g_writebuf, written);
          break;
        }
        case 'f':
        {
          f       = va_arg(args, double);
          written = NV_ftoa2(f, g_writebuf, 6, max_chars - chars_written, 0);
          NV_printf_write_to_buf(&writep, &chars_written, max_chars, g_writebuf, written);
          break;
        }
        case 'l':
          if ((iter + 1) != dest_end && (*(iter + 1) == 'd' || *(iter + 1) == 'i'))
          {
            iter++;
          }
          else
          {
            NV_LOG_ERROR("Expected D\\d\\i after l");
            break;
          }
          __attribute__((__fallthrough__));
        case 'd':
        case 'i':
        case 'u':
          n       = va_arg(args, int64_t);
          written = NV_itoa2(n, g_writebuf, 10, max_chars - chars_written);
          NV_printf_write_to_buf(&writep, &chars_written, max_chars, g_writebuf, written);
          break;
        case 'D':
          NV_assert(0 && "%%D is not corrently accepted");
          break;
        case 'p':
          p       = va_arg(args, void*);
          written = NV_ptoa2(p, g_writebuf, max_chars - chars_written);
          NV_printf_write_to_buf(&writep, &chars_written, max_chars, g_writebuf, written);
          break;
        case 's':
          s = va_arg(args, const char*);
          if (s == NULL)
          {
            s = "(null string)";
          }
          written = NV_strncpy2(writep, s, max_chars - chars_written);
          chars_written += written;
          if (writep)
          {
            writep += written;
          }
          break;
        case '%':
          if (chars_written < max_chars - 1)
          {
            if (writep)
            {
              *writep = '%';
              writep++;
            }
            chars_written++;
          }
          break;
        default:
          // we're copying unrecognized format specifiers as regular chars
          // could be bad though...
          // Oh okay I thought about thsi and this should be how chars should be
          // parsed
          // \n would be passed as \n
          // dumbass %n is not \n
          // I keep talking to myself through comments, this is great
          // who needs friends.
          if (writep)
          {
            *writep = *iter;
            writep++;
          }
          chars_written++;
          break;
      }
    }
    else
    {
      if (chars_written < max_chars - 1)
      {
        if (writep)
        {
          *writep = *iter;
          writep++;
        }
        chars_written++;
      }
    }
  }

  if (dest && max_chars > 0)
  {
    size_t w = (chars_written < max_chars) ? chars_written : max_chars - 1;
    dest[w]  = 0;
  }

  va_end(args);

  return chars_written;
}
// printf

void
__NV_LOG_ERROR(const char* func, const char* fmt, ...)
{
  // this is my project I can do whatever the F@#!@# I want
  const char* preceder  = "oh baby an error! ";
  const char* succeeder = "\n";
  va_list     args;
  va_start(args, fmt);
  _NV_LOG(args, func, succeeder, preceder, fmt, 1);
  va_end(args);
}

void
__NV_LOG_AND_ABORT(const char* func, const char* fmt, ...)
{
  const char* preceder  = "fatal error: ";
  const char* succeeder = "\nabort.\n";
  va_list     args;
  va_start(args, fmt);
  _NV_LOG(args, func, succeeder, preceder, fmt, 1);
  va_end(args);
  abort();
}

void
__NV_LOG_WARNING(const char* func, const char* fmt, ...)
{
  const char* preceder  = "warning: ";
  const char* succeeder = "\n";
  va_list     args;
  va_start(args, fmt);
  _NV_LOG(args, func, succeeder, preceder, fmt, 0);
  va_end(args);
}

void
__NV_LOG_INFO(const char* func, const char* fmt, ...)
{
  const char* preceder  = "info: ";
  const char* succeeder = "\n";
  va_list     args;
  va_start(args, fmt);
  _NV_LOG(args, func, succeeder, preceder, fmt, 0);
  va_end(args);
}

void
__NV_LOG_DEBUG(const char* func, const char* fmt, ...)
{
  const char* preceder  = "debug: ";
  const char* succeeder = "\n";
  va_list     args;
  va_start(args, fmt);
  _NV_LOG(args, func, succeeder, preceder, fmt, 0);
  va_end(args);
}

void
__NV_LOG_CUSTOM(const char* func, const char* preceder, const char* fmt, ...)
{
  const char* succeeder = "\n";
  va_list     args;
  va_start(args, fmt);
  _NV_LOG(args, func, succeeder, preceder, fmt, 0);
  va_end(args);
}

// NVImage
#include <jpeglib.h>
#include <png.h>
#include <zlib.h>

const char*
get_file_extension(const char* path)
{
  const char* dot = strrchr(path, '.');
  // Imagine someone actually uses this project.
  // And then they see this.
  if (!dot || dot == path)
    return "piss";
  return dot + 1;
}

int
NV_bufcompress(const void* input, size_t input_size, void* output, size_t* output_size)
{
  z_stream stream = (z_stream){};

  if (deflateInit(&stream, Z_BEST_COMPRESSION) != Z_OK)
  {
    return -1;
  }

  stream.next_in   = (unsigned char*)input;
  stream.avail_in  = input_size;

  stream.next_out  = output;
  stream.avail_out = *output_size;

  if (deflate(&stream, Z_FINISH) != Z_STREAM_END)
  {
    deflateEnd(&stream);
    return -1;
  }

  *output_size = stream.total_out;

  deflateEnd(&stream);
  return 0;
}

int
NV_bufdecompress(const void* compressed_data, size_t compressed_size, void* o_buf, size_t o_buf_sz)
{
  z_stream strm  = { 0 };
  strm.next_in   = (unsigned char*)compressed_data;
  strm.avail_in  = compressed_size;
  strm.next_out  = o_buf;
  strm.avail_out = o_buf_sz;

  if (inflateInit(&strm) != Z_OK)
  {
    return -1;
  }

  int ret = inflate(&strm, Z_FINISH);
  if (ret != Z_STREAM_END)
  {
    inflateEnd(&strm);
    return -1;
  }

  inflateEnd(&strm);
  return strm.total_out;
}

NVImage
NVImage_Load(const char* path)
{
  const char* ext = get_file_extension(path);
  if (NV_strcmp(ext, "jpeg") == 0 || NV_strcmp(ext, "jpg") == 0)
  {
    return NVImage_LoadJPEG(path);
  }
  else if (NV_strcmp(ext, "png") == 0)
  {
    return NVImage_LoadPNG(path);
  }
  NV_assert(0);
  return (NVImage){};
}

unsigned char*
NVImage_PadChannels(const NVImage* src, int dst_channels)
{
  const int src_channels = NV_FormatGetNumChannels(src->fmt);
  NV_assert(src_channels < dst_channels);

  uint8_t* dst = calloc(src->w * src->h * dst_channels, sizeof(uint8_t));

  for (int y = 0; y < src->h; y++)
  {
    for (int x = 0; x < src->w; x++)
    {
      for (int c = 0; c < dst_channels; c++)
      {
        if (c < src_channels)
        {
          dst[(y * src->w + x) * dst_channels + c] = src->data[(y * src->w + x) * src_channels + c];
        }
        else
        {
          if (c == 3)
          { // alpha channel
            dst[(y * src->w + x) * dst_channels + c] = 255;
          }
          else
          {
            dst[(y * src->w + x) * dst_channels + c] = 0;
          }
        }
      }
    }
  }

  return dst;
}

NVImage
NVImage_LoadPNG(const char* path)
{
  NVImage texture = {};

  FILE*     f       = fopen(path, "rb");
  NV_assert(f != NULL);

  png_struct* png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  NV_assert(png != NULL);

  png_info* info = png_create_info_struct(png);
  NV_assert(info != NULL);

  png_init_io(png, f);
  png_read_info(png, info);

  if (setjmp(png_jmpbuf(png)))
  {
    NV_assert(0);
  }

  texture.w           = png_get_image_width(png, info);
  texture.h           = png_get_image_height(png, info);
  png_byte color_type = png_get_color_type(png, info);
  png_byte bit_depth  = png_get_bit_depth(png, info);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
  {
    png_set_palette_to_rgb(png);
  }

  // if image has less than 8 bits per pixel, increase it to 8 bpp
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
  {
    png_set_expand_gray_1_2_4_to_8(png);
  }

  if (png_get_valid(png, info, PNG_INFO_tRNS))
  {
    png_set_tRNS_to_alpha(png);
  }

  png_read_update_info(png, info);

  int channels = png_get_channels(png, info);

  switch (channels)
  {
    case 1:
      texture.fmt = NOVA_FORMAT_R8;
      break;
    case 2:
      texture.fmt = NOVA_FORMAT_RG8;
      break;
    case 3:
      texture.fmt = NOVA_FORMAT_RGB8;
      break;
    case 4:
      texture.fmt = NOVA_FORMAT_RGBA8;
      break;
    default:
      NV_LOG_ERROR("unsupported file(png) format: channels = %d", channels);
      NV_assert(0);
      break;
      break;
  }

  int rowbytes = png_get_rowbytes(png, info);
  texture.data = (unsigned char*)NV_malloc(rowbytes * texture.h * channels);
  NV_assert(texture.data != NULL);

  u8** row_pointers = NV_malloc(sizeof(u8*) * texture.h);
  for (int y = 0; y < texture.h; y++)
  {
    row_pointers[y] = texture.data + y * texture.w * NV_FormatGetBytesPerPixel(texture.fmt);
  }

  png_read_image(png, row_pointers);

  png_destroy_read_struct(&png, &info, NULL);
  fclose(f);
  NV_free(row_pointers);

  return texture;
}

NVImage
NVImage_LoadJPEG(const char* path)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr         jerr;
  FILE*                         f;
  NVImage                     img = {};

  if ((f = fopen(path, "rb")) == NULL)
  {
    NV_LOG_ERROR("cimageload :: couldn't open file \"%s\" Are you sure that it exists?", path);
    return img;
  }

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpeg_stdio_src(&cinfo, f);
  jpeg_read_header(&cinfo, 1);

  jpeg_start_decompress(&cinfo);

  img.w        = cinfo.output_width;
  img.h        = cinfo.output_height;

  int channels = cinfo.output_components;

  switch (channels)
  {
    case 1:
      img.fmt = NOVA_FORMAT_R8;
      break;
    case 3:
      img.fmt  = NOVA_FORMAT_RGB8;
      channels = 4;
      break;
    default:
      NV_LOG_ERROR("invalid num channels: %d", channels);
      NV_assert(0);
      break;
      break;
  }

  img.data = (unsigned char*)NV_malloc(img.w * img.h * NV_FormatGetBytesPerPixel(img.fmt));

  unsigned char* bufarr[1];
  for (int i = 0; i < (int)cinfo.output_height; i++)
  {
    bufarr[0] = img.data + i * img.w * NV_FormatGetBytesPerPixel(img.fmt);
    jpeg_read_scanlines(&cinfo, bufarr, 1);
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  fclose(f);

  return img;
}

void
NVImage_WritePNG(const NVImage* tex, const char* path)
{
  FILE* f = fopen(path, "wb");
  NV_assert(f != NULL);

  png_struct* png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  NV_assert(png != NULL);

  png_infop info = png_create_info_struct(png);
  NV_assert(info != NULL);

  if (setjmp(png_jmpbuf(png)))
  {
    NV_assert(0);
  }

  png_init_io(png, f);

  int       coltype = -1;
  const int numc    = NV_FormatGetNumChannels(tex->fmt);
  switch (numc)
  {
    case 1:
      coltype = PNG_COLOR_TYPE_GRAY;
      break;
    case 2:
      coltype = PNG_COLOR_TYPE_RGB;
      break;
    case 4:
      coltype = PNG_COLOR_TYPE_RGBA;
      break;
  }
  NV_assert(coltype != -1);

  const int bytesperpixel = NV_FormatGetBytesPerPixel(tex->fmt);
  png_set_IHDR(png, info, tex->w, tex->h, bytesperpixel * 8, coltype, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png, info);

  png_bytep* row_pointers = NV_malloc(sizeof(png_byte*) * tex->h);
  for (int y = 0; y < tex->h; y++)
  {
    row_pointers[y] = tex->data + y * tex->w * bytesperpixel;
  }
  png_write_image(png, row_pointers);
  NV_free(row_pointers);

  png_write_end(png, NULL);

  fclose(f);
  png_destroy_write_struct(&png, &info);
}
// NVImage

void
NV_FormatToString(NVFormat format, const char** dst)
{
  switch (format)
  {
    case NOVA_FORMAT_UNDEFINED:
      *dst = "NOVA_FORMAT_UNDEFINED";
      return;
    case NOVA_FORMAT_R8:
      *dst = "NOVA_FORMAT_R8";
      return;
    case NOVA_FORMAT_RG8:
      *dst = "NOVA_FORMAT_RG8";
      return;
    case NOVA_FORMAT_RGB8:
      *dst = "NOVA_FORMAT_RGB8";
      return;
    case NOVA_FORMAT_RGBA8:
      *dst = "NOVA_FORMAT_RGBA8";
      return;
    case NOVA_FORMAT_BGR8:
      *dst = "NOVA_FORMAT_BGR8";
      return;
    case NOVA_FORMAT_BGRA8:
      *dst = "NOVA_FORMAT_BGRA8";
      return;
    case NOVA_FORMAT_RGB16:
      *dst = "NOVA_FORMAT_RGB16";
      return;
    case NOVA_FORMAT_RGBA16:
      *dst = "NOVA_FORMAT_RGBA16";
      return;
    case NOVA_FORMAT_RG32:
      *dst = "NOVA_FORMAT_RG32";
      return;
    case NOVA_FORMAT_RGB32:
      *dst = "NOVA_FORMAT_RGB32";
      return;
    case NOVA_FORMAT_RGBA32:
      *dst = "NOVA_FORMAT_RGBA32";
      return;
    case NOVA_FORMAT_R8_SINT:
      *dst = "NOVA_FORMAT_R8_SINT";
      return;
    case NOVA_FORMAT_RG8_SINT:
      *dst = "NOVA_FORMAT_RG8_SINT";
      return;
    case NOVA_FORMAT_RGB8_SINT:
      *dst = "NOVA_FORMAT_RGB8_SINT";
      return;
    case NOVA_FORMAT_RGBA8_SINT:
      *dst = "NOVA_FORMAT_RGBA8_SINT";
      return;
    case NOVA_FORMAT_R8_UINT:
      *dst = "NOVA_FORMAT_R8_UINT";
      return;
    case NOVA_FORMAT_RG8_UINT:
      *dst = "NOVA_FORMAT_RG8_UINT";
      return;
    case NOVA_FORMAT_RGB8_UINT:
      *dst = "NOVA_FORMAT_RGB8_UINT";
      return;
    case NOVA_FORMAT_RGBA8_UINT:
      *dst = "NOVA_FORMAT_RGBA8_UINT";
      return;
    case NOVA_FORMAT_R8_SRGB:
      *dst = "NOVA_FORMAT_R8_SRGB";
      return;
    case NOVA_FORMAT_RG8_SRGB:
      *dst = "NOVA_FORMAT_RG8_SRGB";
      return;
    case NOVA_FORMAT_RGB8_SRGB:
      *dst = "NOVA_FORMAT_RGB8_SRGB";
      return;
    case NOVA_FORMAT_RGBA8_SRGB:
      *dst = "NOVA_FORMAT_RGBA8_SRGB";
      return;
    case NOVA_FORMAT_BGR8_SRGB:
      *dst = "NOVA_FORMAT_BGR8_SRGB";
      return;
    case NOVA_FORMAT_BGRA8_SRGB:
      *dst = "NOVA_FORMAT_BGRA8_SRGB";
      return;
    case NOVA_FORMAT_D16:
      *dst = "NOVA_FORMAT_D16";
      return;
    case NOVA_FORMAT_D24:
      *dst = "NOVA_FORMAT_D24";
      return;
    case NOVA_FORMAT_D32:
      *dst = "NOVA_FORMAT_D32";
      return;
    case NOVA_FORMAT_D24_S8:
      *dst = "NOVA_FORMAT_D24_S8";
      return;
    case NOVA_FORMAT_D32_S8:
      *dst = "NOVA_FORMAT_D32_S8";
      return;
    case NOVA_FORMAT_BC1:
      *dst = "NOVA_FORMAT_BC1";
      return;
    case NOVA_FORMAT_BC3:
      *dst = "NOVA_FORMAT_BC3";
      return;
    case NOVA_FORMAT_BC7:
      *dst = "NOVA_FORMAT_BC7";
      return;
  }
}

bool
NV_FormatHasColorChannel(NVFormat fmt)
{
  switch (fmt)
  {
    case NOVA_FORMAT_D16:
    case NOVA_FORMAT_D24:
    case NOVA_FORMAT_D24_S8:
    case NOVA_FORMAT_D32:
    case NOVA_FORMAT_D32_S8:
    case NOVA_FORMAT_BC1:
    case NOVA_FORMAT_BC3:
    case NOVA_FORMAT_BC7:
    case NOVA_FORMAT_UNDEFINED:
      return 0;
    default:
      return 1;
  }
}

// Returns false even for stencil/depth and undefined format
bool
NV_FormatHasAlphaChannel(NVFormat fmt)
{
  switch (fmt)
  {
    case NOVA_FORMAT_RGBA8:
    case NOVA_FORMAT_BGRA8:
    case NOVA_FORMAT_RGBA16:
    case NOVA_FORMAT_RGBA32:
    case NOVA_FORMAT_RGBA8_SINT:
    case NOVA_FORMAT_RGBA8_UINT:
    case NOVA_FORMAT_RGBA8_SRGB:
    case NOVA_FORMAT_BGRA8_SRGB:
      return 1;
    default:
      return 0;
  }
}

bool
NV_FormatHasDepthChannel(NVFormat fmt)
{
  switch (fmt)
  {
    case NOVA_FORMAT_D16:
    case NOVA_FORMAT_D24:
    case NOVA_FORMAT_D24_S8:
    case NOVA_FORMAT_D32:
    case NOVA_FORMAT_D32_S8:
      return 1;

    default:
      return 0;
  }
}

bool
NV_FormatHasStencilChannel(NVFormat fmt)
{
  switch (fmt)
  {
    case NOVA_FORMAT_D24_S8:
    case NOVA_FORMAT_D32_S8:
      return 1;

    default:
      return 0;
  }
}

int
NV_FormatGetBytesPerChannel(NVFormat fmt)
{
  switch (fmt)
  {
    case NOVA_FORMAT_R8:
    case NOVA_FORMAT_RG8:
    case NOVA_FORMAT_RGB8:
    case NOVA_FORMAT_RGBA8:
    case NOVA_FORMAT_BGR8:
    case NOVA_FORMAT_BGRA8:
    case NOVA_FORMAT_R8_SINT:
    case NOVA_FORMAT_RG8_SINT:
    case NOVA_FORMAT_RGB8_SINT:
    case NOVA_FORMAT_RGBA8_SINT:
    case NOVA_FORMAT_R8_UINT:
    case NOVA_FORMAT_RG8_UINT:
    case NOVA_FORMAT_RGB8_UINT:
    case NOVA_FORMAT_RGBA8_UINT:
    case NOVA_FORMAT_R8_SRGB:
    case NOVA_FORMAT_RG8_SRGB:
    case NOVA_FORMAT_RGB8_SRGB:
    case NOVA_FORMAT_RGBA8_SRGB:
    case NOVA_FORMAT_BGR8_SRGB:
    case NOVA_FORMAT_BGRA8_SRGB:
      return 1;

    case NOVA_FORMAT_RGB16:
    case NOVA_FORMAT_RGBA16:
    case NOVA_FORMAT_D16:
      return 2;

    case NOVA_FORMAT_D24:
    case NOVA_FORMAT_D24_S8:
      return 3;

    case NOVA_FORMAT_RG32:
    case NOVA_FORMAT_RGB32:
    case NOVA_FORMAT_RGBA32:
    case NOVA_FORMAT_D32:
    case NOVA_FORMAT_D32_S8:
      return 4;

    default:
    case NOVA_FORMAT_BC1:
    case NOVA_FORMAT_BC3:
    case NOVA_FORMAT_BC7:
    case NOVA_FORMAT_UNDEFINED:
      return -1;
  }
}

int
NV_FormatGetBytesPerPixel(NVFormat fmt)
{
  return NV_FormatGetBytesPerChannel(fmt) * NV_FormatGetNumChannels(fmt);
}

int
NV_FormatGetNumChannels(NVFormat fmt)
{
  switch (fmt)
  {
    case NOVA_FORMAT_R8:
    case NOVA_FORMAT_R8_SINT:
    case NOVA_FORMAT_R8_UINT:
    case NOVA_FORMAT_R8_SRGB:
    case NOVA_FORMAT_D16:
    case NOVA_FORMAT_D24:
    case NOVA_FORMAT_D32:
      return 1;

    case NOVA_FORMAT_RG8:
    case NOVA_FORMAT_RG32:
    case NOVA_FORMAT_RG8_SINT:
    case NOVA_FORMAT_RG8_UINT:
    case NOVA_FORMAT_RG8_SRGB:
    case NOVA_FORMAT_D24_S8:
    case NOVA_FORMAT_D32_S8:
      return 2;

    case NOVA_FORMAT_RGB8:
    case NOVA_FORMAT_BGR8:
    case NOVA_FORMAT_RGB16:
    case NOVA_FORMAT_RGB32:
    case NOVA_FORMAT_RGB8_SINT:
    case NOVA_FORMAT_RGB8_UINT:
    case NOVA_FORMAT_RGB8_SRGB:
    case NOVA_FORMAT_BGR8_SRGB:
      return 3;

    case NOVA_FORMAT_RGBA8:
    case NOVA_FORMAT_BGRA8:
    case NOVA_FORMAT_RGBA16:
    case NOVA_FORMAT_RGBA32:
    case NOVA_FORMAT_RGBA8_SINT:
    case NOVA_FORMAT_RGBA8_UINT:
    case NOVA_FORMAT_RGBA8_SRGB:
    case NOVA_FORMAT_BGRA8_SRGB:
      return 4;

    case NOVA_FORMAT_BC1:
    case NOVA_FORMAT_BC3:
    case NOVA_FORMAT_BC7:
      // FIXME: Implement
      return 0;

    case NOVA_FORMAT_UNDEFINED:
    default:
      return 0;
  }
}

void*
NV_memcpy(void* dst, const void* src, size_t sz)
{
  NV_assert(dst != NULL);
  NV_assert(src != NULL);
  NV_assert(sz != 0);

  #if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
    return __builtin_memcpy(dst, src, sz);
  #endif

  // I saw this optimization trick a long time ago in some big codebase
  // and it has sticken to me
  // do you know how much I just get an ITCH to write memcpy myself?
  if (((uintptr_t)src & 0x3) == 0 && ((uintptr_t)dst & 0x3) == 0)
  {
    const int*   read      = (const int*)src;
    int*         writep    = (int*)dst;

    const size_t int_count = sz / sizeof(int);
    for (size_t i = 0; i < int_count; i++)
    {
      writep[i] = read[i];
    }

    const uchar* byte_read  = (uchar*)(read + int_count);
    uchar*       byte_write = (uchar*)(writep + int_count);

    sz %= sizeof(int);
    for (size_t i = 0; i < sz; i++)
    {
      byte_write[i] = byte_read[i];
    }
  }
  else
  {
    const uchar* read   = (const uchar*)src;
    uchar*       writep = (uchar*)dst;
    for (size_t i = 0; i < sz; i++)
    {
      writep[i] = read[i];
    }
  }

  return dst;
}

// rewritten memcpy
void*
NV_memset(void* dst, char to, size_t sz)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_memset(dst, to, sz);
#endif

  NV_assert(dst != NULL);
  NV_assert(sz != 0);

  if (((uintptr_t)dst & 0x3) == 0)
  {
    int*         write     = (int*)dst;

    int          i_to      = (to << 24) | (to << 16) | (to << 8) | to;

    const size_t int_count = sz / sizeof(int);
    for (size_t i = 0; i < int_count; i++)
    {
      write[i] = i_to;
    }

    uchar* byte_write = (uchar*)(write + int_count);

    sz %= sizeof(int);
    for (size_t i = 0; i < sz; i++)
    {
      byte_write[i] = i_to;
    }
  }
  else
  {
    uchar* write = (uchar*)dst;
    for (size_t i = 0; i < sz; i++)
    {
      write[i] = to;
    }
  }

  return dst;
}

void*
NV_memmove(void* dst, const void* src, size_t sz)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_memmove(dst, src, sz);
#endif

  if (!dst || !src || sz == 0)
  {
    return NULL;
  }
  void* ret = NV_memcpy(dst, src, sz);
  if (!ret)
  {
    return NULL;
  }
  return NV_memset(dst, 0, sz);
}

void*
NV_malloc(size_t sz)
{
  void* ptr = malloc(sz);
  NV_assert(ptr != NULL);
  return ptr;
}

void
NV_free(void* block)
{
  NV_assert(block != NULL);
  if (block == NULL)
  {
    abort();
  }
  free(block);
}

void*
NV_memchr(const void* p, int chr, size_t psize)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_memchr(p, chr, psize);
#endif

  if (!p || !psize)
  {
    return NULL;
  }
  const unsigned char* read = (const unsigned char*)p;
  const unsigned char  chk  = chr;
  for (size_t i = 0; i < psize; i++)
  {
    if (read[i] == chk)
      return (void*)(read + i);
  }
  return NULL;
}

int
__NV_memcmp_aligned(const u32* p1, const u32* p2, size_t max)
{
  size_t i = 0;
  while (i < max && *p1 == *p2)
  {
    p1++;
    p2++;
    i++;
  }
  return (i == max) ? 0 : (*p1 - *p2);
}

int
NV_memcmp(const void* _p1, const void* _p2, size_t max)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_memcmp(_p1, _p2, max);
#endif

  if (!_p1 || !_p2 || max == 0)
  {
    return -1;
  }

  const uchar* p1 = _p1;
  const uchar* p2 = _p2;

  if (((uintptr_t)p1 & 0x3) == 0 && ((uintptr_t)p2 & 0x3) == 0)
  {
    size_t word_count = max / 4;
    int    result     = __NV_memcmp_aligned((u32*)p1, (u32*)p2, word_count);

    if (result != 0)
    {
      return result;
    }

    size_t remaining_bytes = max % 4;
    if (remaining_bytes > 0)
    {
      p1 += word_count * 4;
      p2 += word_count * 4;
      for (size_t i = 0; i < remaining_bytes; i++)
      {
        if (*p1 != *p2)
        {
          return *p1 - *p2;
        }
        p1++;
        p2++;
      }
    }
    return 0;
  }

  size_t i = 0;
  while (i < max && *p1 == *p2)
  {
    p1++;
    p2++;
    i++;
  }
  return (i == max) ? 0 : (*p1 - *p2);
}

size_t
NV_strncpy2(char* dest, const char* src, size_t max)
{
  if (!dest) {
    return _min(NV_strlen(src), max);
  }

  if (max == 0 || !src)
  {
    return -1;
  }

  #if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
    return __builtin_strlen(__builtin_strncpy(dest, src, max));
  #endif

  max--; // -1 so we can fit the NULL terminator
  size_t i = 0;
  // clang-format off
  while (i < max && src[i])
  {
    dest[i] = src[i]; i++;
  }
  // clang-format on
  dest[i] = 0;

  return i;
}

char*
NV_strcpy(char* dest, const char* src)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_strcpy(dest, src);
#endif

  if (!dest || !src)
  {
    return NULL;
  }
  size_t i = 0;
  // clang-format off
  while (src[i])
  {
    dest[i] = src[i]; i++;
  }
  // clang-format on
  dest[i] = 0;
}

char*
NV_strncpy(char* dest, const char* src, size_t max)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_strncpy(dest, src, max);
#endif

  if (max == 0 || !dest || !src)
  {
    return NULL;
  }
  max--; // -1 so we can fit the NULL terminator
  size_t i = 0;
  // clang-format off
  while (i < max && src[i])
  {
    dest[i] = src[i]; i++;
  }
  // clang-format on
  dest[i] = 0;
}

int
NV_strcmp(const char* s1, const char* s2)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_strcmp(s1, s2);
#endif

  while (*s1 && *s2 && (*s1 == *s2))
  {
    s1++;
    s2++;
  }
  return *s2 - *s1;
}

char*
NV_strchr(const char* s, int chr)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_strchr(s, chr);
#endif

  if (!s)
  {
    return NULL;
  }
  while (*s)
  {
    if (*s == chr)
      return (char*)s;
    s++;
  }
  return NULL;
}

char*
NV_strrchr(const char* s, int chr)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_strrchr(s, chr);
#endif

  if (!s)
    return NULL;

  const char* beg = s;
  s += NV_strlen(s) - 1;
  while (s >= beg)
  {
    if (*s == chr)
    {
      return (char*)s;
    }
    s--;
  }
  return NULL;
}

int
NV_strncmp(const char* s1, const char* s2, size_t max)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_strncmp(s1, s2, max);
#endif

  if (!s1 || !s2 || max == 0)
  {
    return -1;
  }
  size_t i = 0;
  while (*s1 && *s2 && (*s1 == *s2) && i < max)
  {
    s1++;
    s2++;
    i++;
  }
  return (i == max) ? 0 : (*(const unsigned char*)s1 - *(const unsigned char*)s2);
}

size_t
NV_strlen(const char* s)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_strlen(s);
#endif

  if (!s)
    return 0;

  const char* b = s;
  while (*s)
  {
    s++;
  }
  return s - b;
}

char*
NV_strstr(const char* s, const char* sub)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_strstr(s, sub);
#endif

  if (!s || !sub)
    return NULL;

  for (; *s; s++)
  {
    const char* s = s;
    const char* p = sub;

    while (*s && *p && *s == *p)
    {
      s++;
      p++;
    }

    if (!*p)
    {
      return (char*)s;
    }
  }

  return NULL;
}

size_t
NV_strspn(const char* s, const char* accept)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_strspn(s, accept);
#endif

  if (!s || !accept)
  {
    return 0;
  }
  size_t i = 0;
  while (*s && *accept && *s == *accept)
  {
    i++;
  }
  return i;
}

size_t
NV_strcspn(const char* s, const char* reject)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_strcspn(s, reject);
#endif

  if (!s || !reject)
  {
    return 0;
  }

  const char* base = reject;
  size_t      i    = 0;

  while (*s)
  {
    const char* j = base;
    while (*j && *j != *s)
    {
      j++;
    }
    if (*j)
    {
      break;
    }
    i++;
    s++;
  }
  return i;
}

char*
NV_strpbrk(const char* s1, const char* s2)
{
#if defined(__GNUC__) && (__NOVA_STR_USE_BUILTIN)
  return __builtin_strpbrk(s1, s2);
#endif

  if (!s1 || !s2)
    return NULL;

  while (*s1)
  {
    const char* j = s2;
    while (*j)
    {
      if (*j == *s1)
      {
        return (char*)s1;
      }
      j++;
    }
    s1++;
  }
  return NULL;
}

char* strtoks = NULL;
char*
NV_strtok(char* s, const char* delim)
{
  if (!s)
    s = strtoks;
  char* p;

  s += NV_strspn(s, delim);
  if (!s || *s == 0)
  {
    strtoks = s;
    return NULL;
  }

  p = s;
  s = NV_strpbrk(s, delim);

  if (!s)
  {
    strtoks = NV_strchr(s, 0); // get pointer to last char
    return p;
  }
  *s      = 0;
  strtoks = s + 1;
  return p;
}

char*
NV_basename(const char* path)
{
  char* p         = (char*)path; // shut up C compiler
  char* backslash = NV_strrchr(path, '\\');
  if (backslash != NULL)
  {
    return backslash + 1;
  }
  return p;
}

// header of memory block
typedef struct sablock
{
  size_t   size;
  unsigned canary;
} sablock;

void
NVAllocatorStackInit(NVAllocatorStack* allocator, unsigned char* buf, size_t available)
{
  allocator->buf       = buf;
  allocator->bufsiz    = available;
  allocator->bufoffset = 0;
}

void*
sarealloc(NVAllocator* parent, void* prevblock, size_t alignment, size_t size)
{
  if (!prevblock)
  {
    NV_LOG_AND_ABORT("invalid pointer");
  }
  sablock* prevblockp = (sablock*)prevblock - 1;
  if (prevblockp->size >= size)
  {
    return prevblock;
  }
  if (prevblockp->canary != NOVA_ALLOCATION_CANARY)
  {
    NV_LOG_AND_ABORT("corrupt memory");
  }

  void* new_data = saalloc(parent, alignment, size);
  NV_assert(new_data != NULL);
  NV_memset(new_data, 0, size);
  NV_memcpy(new_data, prevblock, prevblockp->size);
  safree(parent, prevblock);

  return new_data;
}

void*
saalloc(NVAllocator* parent, size_t alignment, size_t size)
{
  NVAllocatorStack* allocator = (NVAllocatorStack*)parent->context;
  size                          = ALIGN_UP_SIZE(size, alignment);
  if ((allocator->bufoffset + size + sizeof(sablock)) > allocator->bufsiz)
  {
    NV_LOG_ERROR("oom"); // OUT OF MEMORY
    return NULL;
  }

  sablock* block = ALIGN_UP(allocator->buf + allocator->bufoffset, alignment);
  block->size    = size;
  block->canary  = NOVA_ALLOCATION_CANARY;
  block++; // move past the header, so return is the memory after header
  allocator->bufoffset += size + sizeof(sablock);
  return (void*)block;
}

void*
sacalloc(NVAllocator* parent, size_t alignment, size_t size)
{
  void* allocation = saalloc(parent, alignment, size);
  NV_memset(allocation, 0, size);
  return allocation;
}

void
safree(NVAllocator* parent, void* block)
{
  NVAllocatorStack* allocator = (NVAllocatorStack*)parent->context;
  sablock*            p         = (sablock*)block;
  p--;
  NV_assert(p->canary == NOVA_ALLOCATION_CANARY);
  void* allocator_last_block = (allocator->buf + allocator->bufoffset - p->size - sizeof(sablock));
  if (block != allocator_last_block)
  {
    return;
  }
  allocator->bufoffset -= p->size + sizeof(sablock);
  return;
}

NVAllocator NVAllocatorDefault = (NVAllocator){ .alloc = heapalloc, .calloc = heapcalloc, .realloc = heaprealloc, .free = heapfree, .context = NULL };

void*
heapalloc(NVAllocator* parent, size_t alignment, size_t size)
{
  (void)parent;
  size    = ALIGN_UP_SIZE(size, alignment);
  void* p = malloc(size);
  p       = ALIGN_UP(p, alignment);
  return p;
}

void*
heapcalloc(NVAllocator* parent, size_t alignment, size_t size)
{
  (void)parent;
  size    = ALIGN_UP_SIZE(size, alignment);
  void* p = calloc(1, size);
  p       = ALIGN_UP(p, alignment);
  return p;
}

void*
heaprealloc(NVAllocator* parent, void* prevblock, size_t alignment, size_t size)
{
  (void)parent;
  size    = ALIGN_UP_SIZE(size, alignment);
  void* p = realloc(prevblock, size);
  p       = ALIGN_UP(p, alignment);
  return p;
}

void
heapfree(NVAllocator* parent, void* block)
{
  (void)parent;
  free(block);
}

static pthread_mutex_t malloc_mutex = PTHREAD_MUTEX_INITIALIZER;

// A pool is moade of many chunks
NV_node_t*
pool_alloc_internal(size_t alignment, size_t size)
{
  NV_assert((alignment & (alignment - 1)) == 0 && alignment > 0);
  NV_assert(size < SIZE_MAX - alignment - sizeof(NV_node_t) - sizeof(unsigned));

  size += sizeof(NV_node_t);

  size_t total_size = ALIGN_UP_SIZE(size, alignment);

  if (total_size <= 0)
  {
    NV_LOG_ERROR("zero size malloc\n");
    return NULL;
  }

  // void *mapping = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  void* mapping = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (mapping == MAP_FAILED || !mapping)
  {
    NV_LOG_ERROR("mmap failed: %s\n", strerror(*(__errno_location())));
    return NULL;
  }

  NV_node_t* p = (NV_node_t*)mapping;
  *p           = (NV_node_t){
              .payload      = ALIGN_UP(p + 1, alignment),
              .mapping      = mapping,
              .mapping_size = total_size,
              .canary       = NOVA_ALLOCATION_CANARY,
              .in_use       = 1,
  };
  NV_assert(((uintptr_t)p->payload % alignment) == 0);
  return p;
}

void
pool_free_node_internal(NV_node_t* node)
{
  if (!node)
  {
    NV_LOG_AND_ABORT("invalid ptr\n");
    return;
  }

  if (node->canary != NOVA_ALLOCATION_CANARY)
  {
    NV_LOG_AND_ABORT("memory is corrupt\n");
    return;
  }

  void*  mapping = node->mapping;
  size_t size    = node->mapping_size;
  *node          = (NV_node_t){};

  if (munmap(mapping, size) == -1)
  {
    NV_LOG_ERROR("munmap failed: %s\n", strerror(*(__errno_location())));
    return;
  }
}

void
NVAllocatorHeapInit(NVAllocatorHeap* pool)
{
  pthread_mutex_lock(&malloc_mutex);
  NV_freelist_init(0, pool_alloc_internal, pool_free_node_internal, &pool->freelist);
  pthread_mutex_unlock(&malloc_mutex);
}

void*
poolmalloc(NVAllocator* allocator, size_t alignment, size_t size)
{
  pthread_mutex_lock(&malloc_mutex);
  NVAllocatorHeap* pool = (NVAllocatorHeap*)allocator->context;
  NV_freelist_check_circle(&pool->freelist);
  void* p = NV_freelist_alloc(&pool->freelist, alignment, size);
  if (!p)
  {
    NV_LOG_ERROR("alloc (size=%i) failed", (int)size);
  }
  pthread_mutex_unlock(&malloc_mutex);
  return p;
}

// This does not need a mutex as memset is not thread unsafe
void*
poolcalloc(NVAllocator* allocator, size_t alignment, size_t size)
{
  void* p = poolmalloc(allocator, alignment, size);
  if (p)
  {
    NV_memset(p, 0, size);
  }
  return p;
}

void*
poolrealloc(NVAllocator* allocator, void* prevblock, size_t alignment, size_t size)
{
  NVAllocatorHeap* pool = (NVAllocatorHeap*)allocator->context;
  NV_freelist_check_circle(&pool->freelist);

  if (prevblock == NULL)
  {
    return poolmalloc(allocator, alignment, size);
  }

  pthread_mutex_lock(&malloc_mutex);

  NV_node_t* prev_node = pool->freelist.m_root;
  bool       found     = false;

  while (prev_node)
  {
    if (prev_node->payload == prevblock)
    {
      found = true;
      break;
    }
    prev_node = prev_node->next;
  }

  if (!found)
  {
    NV_LOG_ERROR("Invalid prevblock");
    pthread_mutex_unlock(&malloc_mutex);
    return poolmalloc(allocator, alignment, size);
  }

  if (prev_node->canary != NOVA_ALLOCATION_CANARY)
  {
    NV_LOG_AND_ABORT("Double linked list corrupted");
  }

  if (prev_node->size == 0)
  {
    NV_freelist_free(&pool->freelist, prev_node->payload);
    pthread_mutex_unlock(&malloc_mutex);
    return poolmalloc(allocator, alignment, size);
  }

  if (prev_node->size >= size)
  {
  }

  void* new_mem = NV_freelist_alloc(&pool->freelist, alignment, size);

  NV_memcpy(new_mem, prevblock, prev_node->size < size ? prev_node->size : size);

  NV_freelist_free(&pool->freelist, prev_node->payload);

  pthread_mutex_unlock(&malloc_mutex);

  return new_mem;
}

void
poolfree(NVAllocator* allocator, void* block)
{
  pthread_mutex_lock(&malloc_mutex);
  NVAllocatorHeap* pool = (NVAllocatorHeap*)allocator->context;
  NV_freelist_free(&pool->freelist, block);
  pthread_mutex_unlock(&malloc_mutex);
}

// I was having an existential crisis
// should we use calloc or malloc?
// the user *can* initialize it to zero right??

#if defined(__GNUC__)
#define RESTRICT __restrict__
#else
#define RESTRICT restrict
#endif

// deadbeef is for losers
#define CONT_CANARY 0xFEEF

#define CONT_IS_VALID(cont) ((cont) && ((cont)->m_canary == CONT_CANARY))

// ==============================
// VECTOR
// ==============================

NV_vector_t
NV_vector_init(int typesize, size_t init_size)
{
  NV_assert(typesize > 0);

  NV_vector_t vec = {};
  vec.m_size      = 0;
  vec.m_typesize  = typesize;
  vec.m_capacity  = init_size;
  vec.m_canary    = CONT_CANARY;
  vec.m_rwlock    = (pthread_rwlock_t)PTHREAD_RWLOCK_INITIALIZER;

  if (init == 0)
  {
    NVAllocatorHeapInit(&pool);
    init = 1;
  }

  NVAllocatorBindHeapAllocator(&vec.allocator, &pool);

  if (init_size > 0)
  {
    pthread_rwlock_wrlock(&vec.m_rwlock);
    NV_vector_resize(&vec, init_size);
    pthread_rwlock_unlock(&vec.m_rwlock);
  }
  else
  {
    vec.m_data = NULL;
  }

  return vec;
}

void
NV_vector_destroy(NV_vector_t* vec)
{
  if (vec)
  {
    pthread_rwlock_wrlock(&vec->m_rwlock);
    NV_assert(CONT_IS_VALID(vec));
    if (vec->m_data)
    {
      vec->allocator.free(&vec->allocator, vec->m_data);
      pthread_rwlock_unlock(&vec->m_rwlock);
      pthread_rwlock_destroy(&vec->m_rwlock);
    }
  }
}

void
NV_vector_clear(NV_vector_t* vec)
{
  pthread_rwlock_wrlock(&vec->m_rwlock);
  NV_assert(CONT_IS_VALID(vec));
  vec->m_size = 0;
  pthread_rwlock_unlock(&vec->m_rwlock);
}

size_t
NV_vector_size(const NV_vector_t* vec)
{
  pthread_rwlock_rdlock((pthread_rwlock_t*)&vec->m_rwlock);
  NV_assert(CONT_IS_VALID(vec));
  size_t sz = vec->m_size;
  pthread_rwlock_unlock((pthread_rwlock_t*)&vec->m_rwlock);
  return sz;
}

size_t
NV_vector_capacity(const NV_vector_t* vec)
{
  pthread_rwlock_rdlock((pthread_rwlock_t*)&vec->m_rwlock);
  NV_assert(CONT_IS_VALID(vec));
  size_t cap = vec->m_capacity;
  pthread_rwlock_unlock((pthread_rwlock_t*)&vec->m_rwlock);
  return cap;
}

int
NV_vector_typesize(const NV_vector_t* vec)
{
  pthread_rwlock_rdlock((pthread_rwlock_t*)&vec->m_rwlock);
  NV_assert(CONT_IS_VALID(vec));
  size_t tsize = vec->m_typesize;
  pthread_rwlock_unlock((pthread_rwlock_t*)&vec->m_rwlock);
  return tsize;
}

void*
NV_vector_data(const NV_vector_t* vec)
{
  pthread_rwlock_rdlock((pthread_rwlock_t*)&vec->m_rwlock);
  NV_assert(CONT_IS_VALID(vec));
  void* p = vec->m_data;
  pthread_rwlock_unlock((pthread_rwlock_t*)&vec->m_rwlock);
  return p;
}

void*
NV_vector_back(NV_vector_t* vec)
{
  pthread_rwlock_wrlock((pthread_rwlock_t*)&vec->m_rwlock);
  NV_assert(CONT_IS_VALID(vec));
  void* p = NV_vector_get(vec, WRITING_MAX_FOR_THE_23RD_TIME(1ULL, vec->m_size) - 1); // stupid but works
  pthread_rwlock_unlock((pthread_rwlock_t*)&vec->m_rwlock);
  return p;
}

void*
NV_vector_get(const NV_vector_t* vec, size_t i)
{
  pthread_rwlock_wrlock((pthread_rwlock_t*)&vec->m_rwlock);
  NV_assert(CONT_IS_VALID(vec));
  uchar *data = vec->m_data;
  size_t typesize = vec->m_typesize;
  pthread_rwlock_unlock((pthread_rwlock_t*)&vec->m_rwlock);
  return data + (typesize * i);
}

void
NV_vector_set(NV_vector_t* vec, size_t i, void* elem)
{
  pthread_rwlock_wrlock((pthread_rwlock_t*)&vec->m_rwlock);
  NV_assert(CONT_IS_VALID(vec));
  NV_memcpy((char*)vec + (vec->m_typesize * i), elem, vec->m_typesize);
  pthread_rwlock_unlock((pthread_rwlock_t*)&vec->m_rwlock);
}

void
NV_vector_copy_from(const NV_vector_t* RESTRICT src, NV_vector_t* RESTRICT dst)
{
  pthread_rwlock_rdlock((pthread_rwlock_t*)&src->m_rwlock);
  pthread_rwlock_wrlock((pthread_rwlock_t*)&dst->m_rwlock);

  NV_assert(CONT_IS_VALID(src));
  NV_assert(CONT_IS_VALID(dst));

  NV_assert(src->m_typesize == dst->m_typesize);
  if (src->m_size >= dst->m_capacity)
  {
    NV_vector_resize(dst, src->m_size);
  }
  dst->m_size = src->m_size;
  NV_memcpy(dst->m_data, src->m_data, src->m_size * src->m_typesize);

  pthread_rwlock_unlock((pthread_rwlock_t*)&src->m_rwlock);
  pthread_rwlock_unlock((pthread_rwlock_t*)&dst->m_rwlock);
}

void
NV_vector_move_from(NV_vector_t* RESTRICT src, NV_vector_t* RESTRICT dst)
{
  pthread_rwlock_wrlock((pthread_rwlock_t*)&src->m_rwlock);
  pthread_rwlock_wrlock((pthread_rwlock_t*)&dst->m_rwlock);

  NV_assert(CONT_IS_VALID(src));
  NV_assert(CONT_IS_VALID(dst));

  dst->m_size     = src->m_size;
  dst->m_capacity = src->m_capacity;
  dst->m_data     = src->m_data;

  src->m_size     = 0;
  src->m_capacity = 0;
  src->m_data     = NULL;

  pthread_rwlock_unlock((pthread_rwlock_t*)&src->m_rwlock);
  pthread_rwlock_unlock((pthread_rwlock_t*)&dst->m_rwlock);
}

bool
NV_vector_empty(const NV_vector_t* vec)
{
  NV_assert(CONT_IS_VALID(vec));
  return (vec->m_size == 0);
}

bool
NV_vector_equal(const NV_vector_t* vec1, const NV_vector_t* vec2)
{
  NV_assert(CONT_IS_VALID(vec1));
  NV_assert(CONT_IS_VALID(vec2));

  pthread_rwlock_rdlock((pthread_rwlock_t*)&vec1->m_rwlock);
  pthread_rwlock_rdlock((pthread_rwlock_t*)&vec2->m_rwlock);

  bool eq = 1;
  if (vec1->m_size != vec2->m_size || vec1->m_typesize != vec2->m_typesize)
  {
    eq = 0;
  }

  else if (NV_memcmp(vec1->m_data, vec2->m_data, vec1->m_size * vec1->m_typesize) != 0)
  {
    eq = 0;
  }

  pthread_rwlock_unlock((pthread_rwlock_t*)&vec1->m_rwlock);
  pthread_rwlock_unlock((pthread_rwlock_t*)&vec2->m_rwlock);

  return eq;
}

void
NV_vector_resize(NV_vector_t* vec, size_t new_size)
{
  NV_assert(CONT_IS_VALID(vec));

  if (vec->m_data)
  {
    vec->m_data = vec->allocator.realloc(&vec->allocator, vec->m_data, 1, vec->m_typesize * new_size);
  }
  else
  {
    vec->m_data = vec->allocator.calloc(&vec->allocator, 1, vec->m_typesize * new_size);
  }
  NV_assert(vec->m_data != NULL);

  vec->m_capacity = new_size;
}

void
NV_vector_push_back(NV_vector_t* RESTRICT vec, const void* RESTRICT elem)
{
  NV_assert(CONT_IS_VALID(vec));

  pthread_rwlock_wrlock(&vec->m_rwlock);

  if (vec->m_size >= vec->m_capacity)
  {
    NV_vector_resize(vec, WRITING_MAX_FOR_THE_23RD_TIME(1, vec->m_capacity * 2));
  }

  NV_assert(vec->m_data != NULL);
  NV_assert(!(elem >= vec->m_data && elem <= (vec->m_data + vec->m_size)));
  NV_memcpy((uchar*)vec->m_data + (vec->m_size * vec->m_typesize), elem, vec->m_typesize);
  vec->m_size++;

  pthread_rwlock_unlock(&vec->m_rwlock);
}

void
NV_vector_push_set(NV_vector_t* RESTRICT vec, const void* RESTRICT arr, size_t count)
{
  NV_assert(CONT_IS_VALID(vec));

  pthread_rwlock_wrlock(&vec->m_rwlock);

  size_t required_capacity = vec->m_size + count;
  if (required_capacity >= vec->m_capacity)
    NV_vector_resize(vec, required_capacity);
  NV_memcpy((uchar*)vec->m_data + (vec->m_size * vec->m_typesize), arr, count * vec->m_typesize);
  vec->m_size += count;

  pthread_rwlock_unlock(&vec->m_rwlock);
}

void
NV_vector_pop_back(NV_vector_t* vec)
{
  NV_assert(CONT_IS_VALID(vec));

  pthread_rwlock_wrlock(&vec->m_rwlock);

  if (vec->m_size > 0)
  {
    vec->m_size--;
  }

  pthread_rwlock_unlock(&vec->m_rwlock);
}

void
NV_vector_pop_front(NV_vector_t* vec)
{
  NV_assert(CONT_IS_VALID(vec));

  pthread_rwlock_wrlock(&vec->m_rwlock);

  if (vec->m_size > 0)
  {
    vec->m_size--;
    NV_memcpy(vec->m_data, (uchar*)vec->m_data + vec->m_typesize, vec->m_size * vec->m_typesize);
  }

  pthread_rwlock_unlock(&vec->m_rwlock);
}

void
NV_vector_insert(NV_vector_t* RESTRICT vec, size_t index, const void* RESTRICT elem)
{
  NV_assert(CONT_IS_VALID(vec));

  pthread_rwlock_wrlock(&vec->m_rwlock);

  if (index >= vec->m_capacity)
  {
    NV_vector_resize(vec, WRITING_MAX_FOR_THE_23RD_TIME(1, index * 2));
  }
  if (index >= vec->m_size)
  {
    vec->m_size = index + 1;
  }
  NV_memcpy((uchar*)vec->m_data + (vec->m_typesize * index), elem, vec->m_typesize);

  pthread_rwlock_unlock(&vec->m_rwlock);
}

void
NV_vector_remove(NV_vector_t* vec, size_t index)
{
  NV_assert(CONT_IS_VALID(vec));

  pthread_rwlock_wrlock(&vec->m_rwlock);

  if (index >= vec->m_size)
    return;
  else if (vec->m_size - index - 1)
  {
    // please don't ask me what this is
    NV_memcpy((uchar*)vec->m_data + (index * vec->m_typesize), (uchar*)vec->m_data + ((index + 1) * vec->m_typesize), (vec->m_size - index - 1) * vec->m_typesize);
  }
  vec->m_size--;

  pthread_rwlock_unlock(&vec->m_rwlock);
}

int
NV_vector_find(const NV_vector_t* RESTRICT vec, const void* RESTRICT elem)
{
  NV_assert(CONT_IS_VALID(vec));

  pthread_rwlock_rdlock((pthread_rwlock_t*)&vec->m_rwlock);

  for (int i = 0; i < (int)vec->m_size; i++)
  {
    if (NV_memcmp(vec->m_data + (i * vec->m_typesize), elem, vec->m_typesize))
    {
      pthread_rwlock_unlock((pthread_rwlock_t*)&vec->m_rwlock);
      return i;
    }
  }

  pthread_rwlock_unlock((pthread_rwlock_t*)&vec->m_rwlock);
  return -1;
}

void
NV_vector_sort(NV_vector_t* vec, NV_vector_compare_fn compare)
{
  NV_assert(CONT_IS_VALID(vec));

  pthread_rwlock_wrlock(&vec->m_rwlock);

  qsort(vec->m_data, vec->m_size, vec->m_typesize, compare);

  pthread_rwlock_unlock(&vec->m_rwlock);
}

// ==============================
// STRING
// ==============================

#define __NV_string_alloc(size) str->allocator->alloc(str->allocator, 1, size)
#define __NV_string_calloc(size) str->allocator->calloc(str->allocator, 1, size)
#define __NV_string_realloc(prevblock, size) str->allocator->realloc(str->allocator, prevblock, 1, size)
#define __NV_string_free(size) str->allocator->free(str->allocator, size)

static void
NV_string_resize(NV_string_t* str, int new_capacity)
{
  char* new_data = __NV_string_realloc(str->m_data, new_capacity);
  NV_assert(new_data != NULL);
  str->m_data     = new_data;
  str->m_capacity = new_capacity;
}

NV_string_t
NV_string_init(size_t initial_size)
{
  NV_string_t str;
  str.allocator  = &NVAllocatorDefault;
  str.m_capacity = (initial_size > 0) ? initial_size : 1;
  str.m_data     = str.allocator->alloc(str.allocator, 1, str.m_capacity);
  str.m_canary   = CONT_CANARY;
  NV_assert(str.m_data != NULL);

  str.m_data[0] = '\0';
  str.m_size    = 0;
  return str;
}

NV_string_t
NV_string_init_str(const char* init)
{
  NV_assert(init != NULL && NV_strlen(init) > 0);
  NV_string_t str = (NV_string_t){};
  str.allocator   = &NVAllocatorDefault;
  size_t len      = NV_strlen(init);
  str.m_capacity  = len + 1;
  str.m_data      = str.allocator->alloc(str.allocator, 1, str.m_capacity);
  str.m_canary    = CONT_CANARY;
  NV_assert(str.m_data != NULL);

  NV_strcpy(str.m_data, init);
  str.m_size = len;

  return str;
}

NV_string_t
NV_string_substring(const NV_string_t* str, size_t start, size_t length)
{
  NV_assert(CONT_IS_VALID(str));
  NV_assert(start + length <= str->m_size);

  NV_string_t substr = NV_string_init(length + 1);

  NV_strncpy(substr.m_data, str->m_data + start, length);
  substr.m_data[length] = '\0';
  substr.m_size         = length;
  return substr;
}

void
NV_string_destroy(NV_string_t* str)
{
  NV_assert(CONT_IS_VALID(str));
  if (str)
  {
    __NV_string_free(str->m_data);
  }
}

void
NV_string_clear(NV_string_t* str)
{
  NV_assert(CONT_IS_VALID(str));
  if (str)
  {
    str->m_size    = 0;
    str->m_data[0] = '\0';
  }
}

size_t
NV_string_length(const NV_string_t* str)
{
  NV_assert(CONT_IS_VALID(str));
  return str->m_size;
}

size_t
NV_string_capacity(const NV_string_t* str)
{
  NV_assert(CONT_IS_VALID(str));
  return str->m_capacity;
}

const char*
NV_string_data(const NV_string_t* str)
{
  NV_assert(CONT_IS_VALID(str));
  return str->m_data;
}

void
NV_string_append(NV_string_t* str, const char* suffix)
{
  NV_assert(CONT_IS_VALID(str));
  NV_assert(suffix != NULL);

  size_t suffix_length = NV_strlen(suffix);
  if (str->m_size + suffix_length + 1 > str->m_capacity)
  {
    NV_string_resize(str, str->m_size + suffix_length + 1);
  }

  NV_strcpy(str->m_data + str->m_size, suffix);
  str->m_size += suffix_length;
}

void
NV_string_append_char(NV_string_t* str, char suffix)
{
  NV_assert(CONT_IS_VALID(str));

  if (str->m_size + 2 > str->m_capacity)
  {
    NV_string_resize(str, str->m_size + 2);
  }

  str->m_data[str->m_size] = suffix;
  str->m_size++;
  str->m_data[str->m_size] = '\0';
}

void
NV_string_prepend(NV_string_t* str, const char* prefix)
{
  NV_assert(CONT_IS_VALID(str));
  NV_assert(prefix != NULL);

  size_t prefix_length = NV_strlen(prefix);
  if (str->m_size + prefix_length + 1 > str->m_capacity)
  {
    NV_string_resize(str, str->m_size + prefix_length + 1);
  }

  NV_memmove(str->m_data + prefix_length, str->m_data, str->m_size + 1);
  NV_memcpy(str->m_data, prefix, prefix_length);
  str->m_size += prefix_length;
}

void
NV_string_set(NV_string_t* str, const char* new_str)
{
  NV_assert(CONT_IS_VALID(str));
  NV_assert(new_str != NULL);

  size_t new_length = NV_strlen(new_str);
  if (new_length + 1 > str->m_capacity)
  {
    NV_string_resize(str, new_length + 1);
  }

  NV_strcpy(str->m_data, new_str);
  str->m_size = new_length;
}

size_t
NV_string_find(const NV_string_t* str, const char* substr)
{
  NV_assert(CONT_IS_VALID(str));
  NV_assert(substr != NULL);

  char* pos = NV_strstr(str->m_data, substr);
  return pos ? (size_t)(pos - str->m_data) : (size_t)-1;
}

void
NV_string_remove(NV_string_t* str, size_t index, size_t length)
{
  NV_assert(CONT_IS_VALID(str));
  NV_assert(index < str->m_size);

  if (index + length > str->m_size)
  {
    length = str->m_size - index;
  }

  NV_memmove(str->m_data + index, str->m_data + index + length, str->m_size - index - length + 1);
  str->m_size -= length;
}

void
NV_string_copy_from(const NV_string_t* src, NV_string_t* dst)
{
  NV_assert(CONT_IS_VALID(src));
  NV_assert(CONT_IS_VALID(dst));

  NV_assert(src != NULL);
  NV_assert(dst != NULL);
  NV_string_set(dst, src->m_data);
}

void
NV_string_move_from(NV_string_t* src, NV_string_t* dst)
{
  NV_assert(CONT_IS_VALID(src));
  NV_assert(CONT_IS_VALID(dst));

  NV_assert(src != NULL);
  NV_assert(dst != NULL);
  NV_string_copy_from(src, dst);
  NV_string_destroy(src);
}

// ==============================
// HASHMAP
// ==============================

unsigned
closest_power_of_two(unsigned i)
{
  if (i == 0)
  {
    return 1;
  }
  i--;
  i |= i >> 1;
  i |= i >> 2;
  i |= i >> 4;
  i |= i >> 8;
  i |= i >> 16;
  i++;
  return i;
}

unsigned int
power_of_two_mod(unsigned int x, unsigned int n)
{
  return x & (n - 1);
}

unsigned
NV_hashmap_std_hash(const void* bytes, int nbytes)
{
  const unsigned FNV_PRIME    = 16777619;
  const unsigned OFFSET_BASIS = 2166136261;

  unsigned       hash         = OFFSET_BASIS;
  for (unsigned char byte = 0; byte < nbytes; byte++)
  {
    hash ^= ((unsigned char*)bytes)[byte]; // xor
    hash *= FNV_PRIME;
  }
  return hash;
};

bool
NV_hashmap_std_key_eq(const void* key1, const void* key2, unsigned long nbytes)
{
  if (key1 == key2)
  {
    return 1;
  }
  else
  {
    return NV_memcmp(key1, key2, nbytes) == 0;
  }
}

#define __NV_hashmap_alloc(size) map->allocator->alloc(map->allocator, 1, size)
#define __NV_hashmap_calloc(size) map->allocator->calloc(map->allocator, 1, size)
#define __NV_hashmap_free(block) map->allocator->free(map->allocator, block)

NV_hashmap_t*
NV_hashmap_init(int init_size, int keysize, int valuesize, NV_hashmap_hash_fn hash_fn, NV_hashmap_key_equal_fn equal_fn)
{
  NV_assert(keysize > 0 && valuesize > 0);

  NV_hashmap_t* map = calloc(1, sizeof(struct NV_hashmap_t));
  NV_assert(map != NULL);

  if (init_size < 0)
  {
    init_size = 1;
  }
  if (hash_fn == NULL)
  {
    hash_fn = NV_hashmap_std_hash;
  }
  if (equal_fn == NULL)
  {
    equal_fn = NV_hashmap_std_key_eq;
  }

  map->allocator = &NVAllocatorDefault;
  map->m_nodes   = (ch_node_t**)__NV_hashmap_calloc(init_size * sizeof(ch_node_t));
  NV_assert(map->m_nodes != NULL);

  map->m_hash_fn    = hash_fn;
  map->m_equal_fn   = equal_fn;
  map->m_key_size   = keysize;
  map->m_value_size = valuesize;
  map->m_entries    = closest_power_of_two(init_size);
  map->m_size       = 0;
  map->m_canary     = CONT_CANARY;
  return map;
}

void
NV_hashmap_destroy(NV_hashmap_t* map)
{
  NV_assert(CONT_IS_VALID(map));
  if (!map->m_nodes)
  {
    return;
  }
  for (int i = 0; i < map->m_entries; i++)
  {
    if (map->m_nodes[i])
    {
      __NV_hashmap_free(map->m_nodes[i]);
    }
  }
  __NV_hashmap_free(map->m_nodes);
  NV_free(map);
}

void
NV_hashmap_resize(NV_hashmap_t* map, int new_size)
{
  NV_assert(CONT_IS_VALID(map));
  ch_node_t** old_nodes     = map->m_nodes;
  const int   old_m_entries = map->m_entries;

  if (new_size <= 0)
  {
    new_size = 1;
  }

  map->m_entries = closest_power_of_two(new_size);
  map->m_size    = 0;

  map->m_nodes   = __NV_hashmap_calloc(new_size * sizeof(ch_node_t));
  NV_assert(map->m_nodes != NULL);

  if (old_nodes)
  {
    for (int i = 0; i < old_m_entries; i++)
    {
      ch_node_t* node = old_nodes[i];
      if (node && node->is_occupied)
      {
        NV_hashmap_insert(map, node->key, node->value);
        __NV_hashmap_free(node);
      }
    }
    __NV_hashmap_free(old_nodes);
  }
}

void
NV_hashmap_clear(NV_hashmap_t* map)
{
  NV_assert(CONT_IS_VALID(map));
  if (!map->m_nodes)
  {
    return;
  }
  for (int i = 0; i < map->m_entries; i++)
  {
    if (map->m_nodes[i])
    {
      __NV_hashmap_free(map->m_nodes[i]);
    }
  }
  __NV_hashmap_free(map->m_nodes);
  map->m_nodes   = NULL;
  map->m_size    = 0;
  map->m_entries = 0;
}

int
NV_hashmap_size(const NV_hashmap_t* map)
{
  NV_assert(CONT_IS_VALID(map));
  return map->m_size;
}

int
NV_hashmap_capacity(const NV_hashmap_t* map)
{
  NV_assert(CONT_IS_VALID(map));
  return map->m_entries;
}

int
NV_hashmap_keysize(const NV_hashmap_t* map)
{
  NV_assert(CONT_IS_VALID(map));
  return map->m_key_size;
}

int
NV_hashmap_valuesize(const NV_hashmap_t* map)
{
  NV_assert(CONT_IS_VALID(map));
  return map->m_value_size;
}

ch_node_t*
NV_hashmap_iterate(const NV_hashmap_t* map, int* __i)
{
  NV_assert(CONT_IS_VALID(map));
  for (; (*__i) < map->m_entries; (*__i)++)
  {
    int i = *__i;
    if (map->m_nodes[i] && map->m_nodes[i]->is_occupied)
    {
      (*__i)++;
      return map->m_nodes[i];
    }
  }
  return NULL;
}

ch_node_t**
NV_hashmap_root_node(const NV_hashmap_t* map)
{
  NV_assert(CONT_IS_VALID(map));
  return map->m_nodes;
}

void*
NV_hashmap_find(const NV_hashmap_t* map, const void* key)
{
  NV_assert(CONT_IS_VALID(map));
  if (!map->m_nodes)
  {
    return NULL;
  }

  const unsigned begin = (map->m_hash_fn(key, map->m_key_size) % map->m_entries);
  unsigned       i     = begin;
  while (map->m_nodes[i] != NULL && map->m_nodes[i]->is_occupied)
  {
    if (map->m_equal_fn(map->m_nodes[i]->key, key, map->m_key_size))
    {
      return map->m_nodes[i]->value;
    }
    i = power_of_two_mod((i + 1), map->m_entries);
    if (i == begin)
    {
      break;
    }
  }
  return NULL;
}

void
NV_hashmap_insert(NV_hashmap_t* map, const void* key, const void* value)
{
  NV_assert(CONT_IS_VALID(map));
  // the second check
  if (!map->m_nodes || map->m_size >= (map->m_entries * 3) / 4)
  {
    // The check to whether map->m_entries is greater than 0 is already done in
    // resize();
    NV_hashmap_resize(map, map->m_entries * 2);
  }

  const unsigned begin = power_of_two_mod(map->m_hash_fn(key, map->m_key_size), map->m_entries);
  unsigned       i     = begin;
  while (map->m_nodes[i] && map->m_nodes[i]->is_occupied)
  {
    i = power_of_two_mod((i + 1), map->m_entries);
    if (i == begin)
    {
      break;
    }
  }

  if (!map->m_nodes[i])
  {
    // Batch allocation for the entire node at once.
    void* alloc            = __NV_hashmap_alloc(sizeof(ch_node_t) + map->m_key_size + map->m_value_size);
    map->m_nodes[i]        = alloc;
    map->m_nodes[i]->key   = alloc + sizeof(ch_node_t);
    map->m_nodes[i]->value = alloc + sizeof(ch_node_t) + map->m_key_size;
  }

  NV_memcpy(map->m_nodes[i]->key, key, map->m_key_size);
  NV_memcpy(map->m_nodes[i]->value, value, map->m_value_size);
  map->m_nodes[i]->is_occupied = 1;
  map->m_size++;
}

void
NV_hashmap_insert_or_replace(NV_hashmap_t* map, const void* key, void* value)
{
  NV_assert(CONT_IS_VALID(map));
  if (!map->m_nodes || map->m_size >= (map->m_entries * 3) / 4)
  {
    // The check to whether map->m_entries is greater than 0 is already done in
    // resize();
    NV_hashmap_resize(map, map->m_entries * 2);
  }

  const unsigned begin = power_of_two_mod(map->m_hash_fn(key, map->m_key_size), map->m_entries);
  unsigned       i     = begin;
  while (map->m_nodes[i] && map->m_nodes[i]->is_occupied)
  {
    i = power_of_two_mod((i + 1), map->m_entries);
    if (map->m_equal_fn(map->m_nodes[i]->key, key, map->m_key_size))
    {
      NV_memcpy(map->m_nodes[i]->value, value, map->m_value_size);
    }
    else if (i == begin)
    {
      break;
    }
  }

  if (!map->m_nodes[i])
  {
    // Batch allocation for the entire node at once.
    void* alloc            = __NV_hashmap_alloc(sizeof(ch_node_t) + map->m_key_size + map->m_value_size);
    map->m_nodes[i]        = alloc;
    map->m_nodes[i]->key   = alloc + sizeof(ch_node_t);
    map->m_nodes[i]->value = alloc + sizeof(ch_node_t) + map->m_key_size;
  }

  NV_memcpy(map->m_nodes[i]->key, key, map->m_key_size);
  NV_memcpy(map->m_nodes[i]->value, value, map->m_value_size);
  map->m_nodes[i]->is_occupied = 1;
  map->m_size++;
}

void
NV_hashmap_serialize(NV_hashmap_t* map, FILE* f)
{
  NV_assert(CONT_IS_VALID(map));
  const int key_size = map->m_key_size;
  const int val_size = map->m_value_size;

  for (int i = 0; i < map->m_entries; i++)
  {
    if (map->m_nodes[i] && map->m_nodes[i]->is_occupied)
    {
      void* node_key   = map->m_nodes[i]->key;
      void* node_value = map->m_nodes[i]->value;

      fwrite(node_value, val_size, 1, f);
      fwrite(node_key, key_size, 1, f);
    }
  }
}

void
NV_hashmap_deserialize(NV_hashmap_t* map, FILE* f)
{
  NV_assert(CONT_IS_VALID(map));
  void* key   = NV_malloc(map->m_key_size);
  void* value = NV_malloc(map->m_value_size);

  while (fread(value, map->m_value_size, 1, f) == 1 && fread(key, map->m_key_size, 1, f) == 1)
  {
    NV_hashmap_insert(map, key, value);
  }

  NV_free(key);
  NV_free(value);
}

// ==============================
// ATLAS
// ==============================

NV_atlas_t
NV_atlas_init(int init_w, int init_h)
{
  NV_atlas_t atlas           = {};

  atlas.width              = init_w;
  atlas.height             = init_h;
  atlas.next_x             = 0;
  atlas.next_y             = 0;
  atlas.current_row_height = 0;
  atlas.allocator          = &NVAllocatorDefault;
  atlas.data               = atlas.allocator->calloc(atlas.allocator, 1, init_w * init_h);

  return atlas;
}

bool
NV_atlas_add_image(NV_atlas_t* RESTRICT atlas, int w, int h, const unsigned char* RESTRICT data, int* RESTRICT x, int* RESTRICT y)
{
  const int padding = 4;
  const int prev_h = atlas->height, prev_w = atlas->width;
  bool      NV_cont_realloc_needed = 0;
  if (w > atlas->width)
  {
    // ! This doesn't work because the old image is not correctly copied by
    // NV_cont_realloc ! It'll be fixed by copying over the data row by row
    // probably doesn't need fixing right now, will delay it for another eon
    // TODO: FIXME
    atlas->width           = w;
    NV_cont_realloc_needed = 1;
  }

  if (atlas->next_x + w + padding > atlas->width)
  {
    atlas->next_x = 0;
    atlas->next_y += atlas->current_row_height + padding;
    atlas->current_row_height = 0;
  }

  if (atlas->next_y + h + padding > atlas->height)
  {
    atlas->height          = WRITING_MAX_FOR_THE_23RD_TIME(atlas->height * 2, atlas->next_y + h + padding);
    NV_cont_realloc_needed = 1;
  }

  if (NV_cont_realloc_needed)
  {
    atlas->data = atlas->allocator->realloc(atlas->allocator, atlas->data, 1, atlas->width * atlas->height);
    NV_memset(atlas->data + prev_w * prev_h, 0, (atlas->width - prev_w) * (atlas->height - prev_h));
  }

  for (int y = 0; y < h; y++)
  {
    NV_memcpy(atlas->data + (atlas->next_x + (atlas->next_y + y) * atlas->width), data + (y * w), w);
  }

  *x = atlas->next_x;
  *y = atlas->next_y;

  atlas->next_x += w + padding;
  atlas->current_row_height = WRITING_MAX_FOR_THE_23RD_TIME(atlas->current_row_height, h + padding);

  return 0;
}

// ==============================
// BITSET
// ==============================

NV_bitset_t
NV_bitset_init(int init_capacity)
{
  NV_bitset_t ret = {};
  if (init_capacity > 0)
  {
    init_capacity = (init_capacity + 7) / 8;
    ret.size      = init_capacity;
    ret.allocator = &NVAllocatorDefault;
    ret.data      = ret.allocator->calloc(ret.allocator, 1, init_capacity * sizeof(uint8_t));
  }
  else
  {
    ret.size = 0;
  }
  return ret;
}

void
NV_bitset_set_bit(NV_bitset_t* set, int bitindex)
{
  set->data[bitindex / 8] |= (1 << (bitindex % 8));
}

void
NV_bitset_set_bit_to(NV_bitset_t* set, int bitindex, NV_bitset_bit to)
{
  to ? NV_bitset_set_bit(set, bitindex) : NV_bitset_clear_bit(set, bitindex);
}

void
NV_bitset_clear_bit(NV_bitset_t* set, int bitindex)
{
  set->data[bitindex / 8] &= ~(1 << (bitindex % 8));
}

void
NV_bitset_toggle_bit(NV_bitset_t* set, int bitindex)
{
  set->data[bitindex / 8] ^= (1 << (bitindex % 8));
}

NV_bitset_bit
NV_bitset_access_bit(NV_bitset_t* set, int bitindex)
{
  return (set->data[bitindex / 8] & (1 << (bitindex % 8))) != 0;
}

void
NV_bitset_copy_from(NV_bitset_t* dst, const NV_bitset_t* src)
{
  if (src->size != dst->size && dst->data)
  {
    dst->allocator->free(dst->allocator, dst->data);
    dst->data = src->allocator->alloc(src->allocator, 1, src->size);
    dst->size = src->size;
  }
  NV_memcpy(dst->data, src->data, src->size);
}

void
NV_bitset_destroy(NV_bitset_t* set)
{
  set->allocator->free(set->allocator, set->data);
}

NVAllocatorHeap pool;
bool              init = 0;

void
NV_freelist_check_circle(const NV_freelist_t* list)
{
#ifndef NDEBUG
  NV_node_t* node = list->m_root;
  NV_node_t *slow = node, *fast = node;

  while (fast && fast->next)
  {
    slow = slow->next;
    fast = fast->next->next;
    if (fast)
      NV_assert(fast->canary == NOVA_ALLOCATION_CANARY);
    if (slow)
      NV_assert(slow->canary == NOVA_ALLOCATION_CANARY);

    if (slow == fast)
    {
      NV_LOG_AND_ABORT("circular freelist");
      return;
    }
  }
#endif
}

NV_node_t*
NV_freelist_mknode(const NV_freelist_t* list, size_t alignment, size_t size, NV_node_t* next, NV_node_t* prev)
{
  NV_assert(CONT_IS_VALID(list));
  NV_freelist_check_circle(list);

  NV_node_t* node = list->m_alloc_fn(alignment, size);
  node->next      = next;
  node->prev      = prev;
  return node;
}

void
NV_freelist_init(size_t init_size, NV_freelist_alloc_fn alloc_fn, NV_freelist_free_fn free_fn, NV_freelist_t* list)
{
  list->m_alloc_fn = alloc_fn;
  list->m_free_fn  = free_fn;
  if (init_size > 0)
  {
    list->m_root       = NV_freelist_mknode(list, 1, init_size, NULL, NULL);
    list->m_root->size = init_size;
  }
  else
  {
    list->m_root = NULL;
  }

  list->m_canary = CONT_CANARY;
  NV_freelist_check_circle(list);
}

void
NV_freelist_destroy(NV_freelist_t* list)
{
  NV_assert(CONT_IS_VALID(list));
  NV_freelist_check_circle(list);
  if (!list || !list->m_root)
    return;

  NV_node_t* node = list->m_root;

  while (node->next)
  {
    node = node->next;
  }

  while (node)
  {
    NV_node_t* prev_node = node->prev;
    if (node->in_use)
    {
      NV_freelist_free(list, node->payload);
    }
    node = prev_node;
  }
  NV_freelist_check_circle(list);
}

void*
NV_freelist_alloc(NV_freelist_t* list, size_t alignment, size_t size)
{
  NV_assert(CONT_IS_VALID(list));
  NV_freelist_check_circle(list);

  NV_node_t* node = list->m_root;
  while (node)
  {
    size_t aligned_node_size = ALIGN_UP_SIZE(node->mapping_size, alignment);
    if (!node->in_use && aligned_node_size >= size)
    {
      node->in_use  = 1;
      node->payload = ALIGN_UP(node->payload, alignment);
      NV_assert(((uintptr_t)node->payload % alignment) == 0);
      NV_freelist_check_circle(list);
      return node->payload;
    }
    node = node->next;
  }

  node         = NV_freelist_expand(list, alignment, size);
  node->in_use = 1;
  NV_freelist_check_circle(list);
  NV_assert(((uintptr_t)node->payload % alignment) == 0);
  NV_assert(node->payload != NULL);
  return node->payload;
}

NV_node_t*
NV_freelist_expand(NV_freelist_t* list, size_t alignment, size_t expand_by)
{
  NV_assert(CONT_IS_VALID(list));
  NV_freelist_check_circle(list);

  if (!list->m_root)
  {
    list->m_root       = NV_freelist_mknode(list, alignment, expand_by, NULL, NULL);
    list->m_root->size = expand_by;
    return list->m_root;
  }

  NV_node_t* last_node = list->m_root;
  while (last_node->next)
  {
    last_node = last_node->next;
  }
  // now we have last_node

  NV_node_t* new_node = NV_freelist_mknode(list, alignment, expand_by, NULL, NULL);
  last_node->next     = new_node;
  new_node->prev      = last_node;
  new_node->size      = expand_by;
  NV_freelist_check_circle(list);
  return new_node;
}
void
NV_freelist_free(NV_freelist_t* list, void* block)
{
  NV_assert(CONT_IS_VALID(list));
  NV_freelist_check_circle(list);

  if (!block)
  {
    NV_LOG_INFO("invalid block");
    return;
  }

  bool       found = 0;
  NV_node_t* node  = list->m_root;

  while (node)
  {
    if (block == node->payload)
    {
      found = 1;
      break;
    }
    node = node->next;
  }

  if (!found)
  {
    NV_LOG_ERROR("no block found");
    return;
  }

  if (!node->in_use)
  {
    NV_LOG_ERROR("double free");
    return;
  }

  node->in_use    = 0;

  NV_node_t* prev = node->prev;
  NV_node_t* next = node->next;

  if (prev)
  {
    prev->next = next;
  }
  else
  {
    list->m_root = next;
  }

  if (next)
  {
    next->prev = prev;
  }

  list->m_free_fn(node);

  NV_freelist_check_circle(list);
}

NV_node_t*
NV_freelist_find(NV_freelist_t* list, void* alloc)
{
  NV_assert(CONT_IS_VALID(list));
  NV_freelist_check_circle(list);

  NV_node_t* node = list->m_root;
  while (node)
  {
    if (node->payload == alloc)
    {
      return node;
    }
    node = node->next;
  }
  NV_freelist_check_circle(list);
  return NULL;
}
