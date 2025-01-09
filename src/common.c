#include "../include/common/lunaImage.h"
#include "../include/common/mem.h"
#include "../include/common/printf.h"
#include "../include/common/stdafx.h"

#include <SDL2/SDL.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void __CG_LOG(va_list args, const char* fn, const char* succeeder, const char* preceder,
              const char* str, unsigned char err)
{
  FILE* out = (err) ? stderr : stdout;

  struct tm* time = __CG_GET_TIME();

  luna_fprintf(out, "[%d:%d:%d] ", time->tm_hour % 12, time->tm_min, time->tm_sec);

  luna_fprintf(out, "[%s] ", fn);

  luna_vfprintf(out, preceder, args);
  luna_vfprintf(out, str, args);
  luna_vfprintf(out, succeeder, args);
}

// printf

bool luna_is_format_specifier(char c)
{
  // c == 'l' isn't technically a format specifier
  return (c == 'f') || (c == 'i') || (c == 'd') || (c == 'u') || (c == 'l') || (c == 'p') ||
         (c == 's') || (c == 'L');
}

char* luna_itoa(long long x, char out[], int base)
{
  cassert(base >= 2 && base <= 36);
  cassert(out != NULL);

  long long pwr = 1, i = 0;

  if (x == 0)
  {
    out[i++] = '0';
    return out;
  }

  if (x < 0 && base == 10)
  {
    out[i++] = '-';
    x        = -x;
  }

  // get the highest power of 10 in the number
  while ((x / pwr) >= base)
  {
    pwr *= base;
  }

  while (pwr > 0)
  {
    int dig  = x / pwr;
    out[i++] = dig + '0';
    x %= pwr;
    pwr /= base;
  }
  out[i] = 0;
  return out;
}

char* luna_ftoa(double n, char s[], int precision)
{
  // handle special cases
  if (isnan(n))
  {
    strcpy(s, "nan");
    return s;
  }
  else if (isinf(n))
  {
    strcpy(s, "inf");
    return s;
  }
  else if (n == 0.0)
  {
    strcpy(s, "0");
    return s;
  }
  int   digit, m, m1;
  char* c   = s;
  int   neg = (n < 0);
  if (neg)
    n = -n;
  // calculate magnitude
  m          = log10(n);
  int useExp = (m >= 14 || (neg && m >= 9) || m <= -9);
  if (neg)
    *(c++) = '-';
  // set up for scientific notation
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
  // convert the number
  const double proc = 1.0 / pow(10.0, precision);
  while (n > proc || m >= 0)
  {
    double weight = pow(10.0, m);
    if (weight > 0 && !isinf(weight))
    {
      digit = floor(n / weight);
      n -= (digit * weight);
      *(c++) = '0' + digit;
    }
    if (m == 0 && n > 0)
      *(c++) = '.';
    m--;
  }
  if (useExp)
  {
    // convert the exponent
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
  *(c) = '\0';
  return s;
}

// Returns __INT32_MAX__ on error.
int luna_atoi(const char str[])
{
  const char *begin, *end;

  begin = str;
  end   = str + strlen(str) - 1;

  int ret   = 0;
  int power = 1;

  bool neg = 0;
  if (*begin == '-')
  {
    neg = 1;
    begin++;
  }
  else if (*begin == '+')
  {
    begin++;
  }

  while (end >= begin)
  {
    int digit = *end - '0';
    if (digit < 0 || digit > 9)
    {
      return __INT32_MAX__;
    }

    ret += digit * power;
    power *= 10;
    end--;
  }

  if (neg)
  {
    ret *= -1;
  }

  return ret;
}

double luna_atof(const char str[])
{
  double      result = 0.0, fraction = 0.0;
  int         divisor = 1;
  bool        neg     = 0;
  const char* i       = str;

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

  if (*str == 'e' || *i == 'E')
  {
    i++;
    int exp_sign = 1;
    int exponent = 0;

    if (*i == '-')
    {
      exp_sign = -1;
      i++;
    }
    else if (*str == '+')
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

size_t luna_printf(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = luna_vnprintf(SIZE_MAX, args, fmt);

  va_end(args);

  return chars_written;
}

size_t luna_fprintf(FILE* f, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  char   buf[LUNA_PRINTF_BUFSIZ];
  size_t chars_written = luna_vsnprintf(buf, SIZE_MAX, fmt, args);

  fputs(buf, f);

  va_end(args);

  return chars_written;
}

size_t luna_sprintf(char* dest, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = luna_vsnprintf(dest, SIZE_MAX, fmt, args);

  va_end(args);

  return chars_written;
}

size_t luna_vprintf(const char* fmt, va_list args)
{
  char buf[LUNA_PRINTF_BUFSIZ];

  size_t chars_written = luna_vsnprintf(buf, SIZE_MAX, fmt, args);

  fputs(buf, LUNA_PRINTF_STREAM);

  return chars_written;
}

size_t luna_vfprintf(FILE* f, const char* fmt, va_list args)
{
  char   buf[LUNA_PRINTF_BUFSIZ];
  size_t chars_written = luna_vsnprintf(buf, SIZE_MAX, fmt, args);

  fputs(buf, f);
  return chars_written;
}

size_t luna_snprintf(char* dest, size_t max_chars, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = luna_vsnprintf(dest, max_chars, fmt, args);

  va_end(args);

  return chars_written;
}

size_t luna_nprintf(size_t max_chars, const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  size_t chars_written = luna_vnprintf(max_chars, args, fmt);

  va_end(args);

  return chars_written;
}

size_t luna_vnprintf(size_t max_chars, va_list args, const char* fmt)
{
  char   buf[LUNA_PRINTF_BUFSIZ];
  size_t chars_written = luna_vsnprintf(buf, max_chars, fmt, args);

  fputs(buf, LUNA_PRINTF_STREAM);

  return chars_written;
}

size_t __luna_write_string(char* buf, const char* src, size_t max)
{
  size_t dest_len  = strnlen(buf, max);
  size_t remaining = max - dest_len - 1;

  if (remaining == 0)
    return 0;

  size_t to_copy = strnlen(src, remaining);
  memcpy(buf + dest_len, src, to_copy);
  buf[dest_len + to_copy] = 0;

  return to_copy;
}

size_t luna_vsnprintf(char* dest, size_t max_chars, const char* fmt, va_list args)
{
  size_t chars_written                    = 0;
  char   out_buffer[LUNA_PRINTF_BUFSIZ]   = {0};
  char   write_buffer[LUNA_PRINTF_BUFSIZ] = {0};

  int         n;
  const char* s;
  double      f;
  const char* iter = fmt;

  for (; *iter && chars_written < max_chars; iter++)
  {
    if (*iter == '%')
    {
      iter++;

      switch (*iter)
      {
        // we don't currently allow any digit before . because yes
        // i think it's like padding or somtehing but write it yourself lmao it's so simple!!
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

          f = va_arg(args, double);
          luna_ftoa(f, write_buffer, precision);
          break;
        }
        case 'f':
        {
          f = va_arg(args, double);
          luna_ftoa(f, write_buffer, 6);
          break;
        }
        case 'l':
          if (*(iter + 1) == 'd' || *(iter + 1) == 'i' || *(iter + 1) == 'c')
          {
            iter++;
          }
          __attribute__((__fallthrough__));
        case 'd':
        case 'i':
        case 'u':
          n = va_arg(args, int64_t);
          luna_itoa(n, write_buffer, 10);
          break;
        case 's':
          s = va_arg(args, const char*);
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
      if (chars_written + len < max_chars)
      {
        __luna_write_string(out_buffer, write_buffer, max_chars);
        chars_written += len;
      }
      else
      {
        __luna_write_string(out_buffer, write_buffer, max_chars);
        chars_written = max_chars;
      }
    }
    else
    {
      if (chars_written + 1 < max_chars)
      {
        size_t len          = strlen(out_buffer);
        out_buffer[len]     = *iter;
        out_buffer[len + 1] = '\0';
        chars_written++;
      }
    }
  }

  if (dest)
  {
    size_t written = (chars_written < max_chars) ? chars_written : max_chars - 1;
    strncpy(dest, out_buffer, written);
    dest[written] = '\0';
  }

  return chars_written;
}
// printf

void __LOG_ERROR(const char* func, const char* fmt, ...)
{
  // this is my project I can do whatever the F@#!@# I want
  const char* preceder  = "oh baby an error! ";
  const char* succeeder = "\n";
  va_list     args;
  va_start(args, fmt);
  __CG_LOG(args, func, succeeder, preceder, fmt, 1);
  va_end(args);
}

void __LOG_AND_ABORT(const char* func, const char* fmt, ...)
{
  const char* preceder  = "fatal error: ";
  const char* succeeder = "\nabort.\n";
  va_list     args;
  va_start(args, fmt);
  __CG_LOG(args, func, succeeder, preceder, fmt, 1);
  va_end(args);
  abort();
}

void __LOG_WARNING(const char* func, const char* fmt, ...)
{
  const char* preceder  = "warning: ";
  const char* succeeder = "\n";
  va_list     args;
  va_start(args, fmt);
  __CG_LOG(args, func, succeeder, preceder, fmt, 0);
  va_end(args);
}

void __LOG_INFO(const char* func, const char* fmt, ...)
{
  const char* preceder  = "info: ";
  const char* succeeder = "\n";
  va_list     args;
  va_start(args, fmt);
  __CG_LOG(args, func, succeeder, preceder, fmt, 0);
  va_end(args);
}

void __LOG_DEBUG(const char* func, const char* fmt, ...)
{
  const char* preceder  = "debug: ";
  const char* succeeder = "\n";
  va_list     args;
  va_start(args, fmt);
  __CG_LOG(args, func, succeeder, preceder, fmt, 0);
  va_end(args);
}

void __LOG_CUSTOM(const char* func, const char* preceder, const char* fmt, ...)
{
  const char* succeeder = "\n";
  va_list     args;
  va_start(args, fmt);
  __CG_LOG(args, func, succeeder, preceder, fmt, 0);
  va_end(args);
}

// lunaImage
#include "printf.h"

#include <jpeglib.h>
#include <png.h>
#include <zlib.h>

const char* get_file_extension(const char* path)
{
  const char* dot = strrchr(path, '.');
  // Imagine someone actually uses this project.
  // And then they see this.
  if (!dot || dot == path)
    return "piss";
  return dot + 1;
}

int luna_compress_data(const void* input, size_t input_size, void* output, size_t* output_size)
{
  z_stream stream;
  memset(&stream, 0, sizeof(stream));

  if (deflateInit(&stream, Z_BEST_COMPRESSION) != Z_OK)
  {
    return -1;
  }

  stream.next_in  = (unsigned char*) input;
  stream.avail_in = input_size;

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

int luna_decompress_data(const void* compressed_data, size_t compressed_size, void* o_buf,
                         size_t o_buf_sz)
{
  z_stream strm  = {0};
  strm.next_in   = (unsigned char*) compressed_data;
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

lunaImage lunaImage_Load(const char* path)
{
  const char* ext = get_file_extension(path);
  if (strcmp(ext, "jpeg") == 0 || strcmp(ext, "jpg") == 0)
  {
    return lunaImage_LoadJPEG(path);
  }
  else if (strcmp(ext, "png") == 0)
  {
    return lunaImage_LoadPNG(path);
  }
  cassert(0);
  return (lunaImage){};
}

unsigned char* lunaImage_PadChannels(const lunaImage* src, int dst_channels)
{
  const int src_channels = luna_FormatGetNumChannels(src->fmt);
  cassert(src_channels < dst_channels);

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

lunaImage lunaImage_LoadPNG(const char* path)
{
  lunaImage texture = {};

  FILE* f = fopen(path, "rb");
  cassert(f != NULL);

  png_struct* png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  cassert(png != NULL);

  png_info* info = png_create_info_struct(png);
  cassert(info != NULL);

  png_init_io(png, f);
  png_read_info(png, info);

  if (setjmp(png_jmpbuf(png)))
  {
    cassert(0);
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
      texture.fmt = LUNA_FORMAT_R8;
      break;
    case 2:
      texture.fmt = LUNA_FORMAT_RG8;
      break;
    case 3:
      texture.fmt = LUNA_FORMAT_RGB8;
      break;
    case 4:
      texture.fmt = LUNA_FORMAT_RGBA8;
      break;
    default:
      LOG_ERROR("unsupported file(png) format: channels = %d", channels);
      cassert(0);
      break;
      break;
  }

  int rowbytes = png_get_rowbytes(png, info);
  texture.data = (unsigned char*) luna_malloc(rowbytes * texture.h * channels);
  cassert(texture.data != NULL);

  u8** row_pointers = luna_malloc(sizeof(u8*) * texture.h);
  for (int y = 0; y < texture.h; y++)
  {
    row_pointers[y] = texture.data + y * texture.w * luna_FormatGetBytesPerPixel(texture.fmt);
  }

  png_read_image(png, row_pointers);

  png_destroy_read_struct(&png, &info, NULL);
  fclose(f);
  free(row_pointers);

  return texture;
}

lunaImage lunaImage_LoadJPEG(const char* path)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr         jerr;
  FILE*                         f;
  lunaImage                     img = {};

  if ((f = fopen(path, "rb")) == NULL)
  {
    LOG_ERROR("cimageload :: couldn't open file \"%s\" Are you sure that it exists?", path);
    return img;
  }

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpeg_stdio_src(&cinfo, f);
  jpeg_read_header(&cinfo, 1);

  jpeg_start_decompress(&cinfo);

  img.w = cinfo.output_width;
  img.h = cinfo.output_height;

  int channels = cinfo.output_components;

  switch (channels)
  {
    case 1:
      img.fmt = LUNA_FORMAT_R8;
      break;
    case 3:
      img.fmt  = LUNA_FORMAT_RGB8;
      channels = 4;
      break;
    default:
      LOG_ERROR("invalid num channels: %d", channels);
      cassert(0);
      break;
      break;
  }

  img.data = (unsigned char*) luna_malloc(img.w * img.h * luna_FormatGetBytesPerPixel(img.fmt));

  unsigned char* bufarr[1];
  for (int i = 0; i < (int) cinfo.output_height; i++)
  {
    bufarr[0] = img.data + i * img.w * luna_FormatGetBytesPerPixel(img.fmt);
    jpeg_read_scanlines(&cinfo, bufarr, 1);
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  fclose(f);

  return img;
}

void lunaImage_WritePNG(const lunaImage* tex, const char* path)
{
  FILE* f = fopen(path, "wb");
  cassert(f != NULL);

  png_struct* png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  cassert(png != NULL);

  png_infop info = png_create_info_struct(png);
  cassert(info != NULL);

  if (setjmp(png_jmpbuf(png)))
  {
    cassert(0);
  }

  png_init_io(png, f);

  int       coltype = -1;
  const int numc    = luna_FormatGetNumChannels(tex->fmt);
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
  cassert(coltype != -1);

  const int bytesperpixel = luna_FormatGetBytesPerPixel(tex->fmt);
  png_set_IHDR(png, info, tex->w, tex->h, bytesperpixel * 8, coltype, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png, info);

  png_bytep* row_pointers = luna_malloc(sizeof(png_byte*) * tex->h);
  for (int y = 0; y < tex->h; y++)
  {
    row_pointers[y] = tex->data + y * tex->w * bytesperpixel;
  }
  png_write_image(png, row_pointers);
  free(row_pointers);

  png_write_end(png, NULL);

  fclose(f);
  png_destroy_write_struct(&png, &info);
}
// lunaImage

void luna_FormatToString(lunaFormat format, const char** dst)
{
  switch (format)
  {
    case LUNA_FORMAT_UNDEFINED:
      *dst = "LUNA_FORMAT_UNDEFINED";
      return;
    case LUNA_FORMAT_R8:
      *dst = "LUNA_FORMAT_R8";
      return;
    case LUNA_FORMAT_RG8:
      *dst = "LUNA_FORMAT_RG8";
      return;
    case LUNA_FORMAT_RGB8:
      *dst = "LUNA_FORMAT_RGB8";
      return;
    case LUNA_FORMAT_RGBA8:
      *dst = "LUNA_FORMAT_RGBA8";
      return;
    case LUNA_FORMAT_BGR8:
      *dst = "LUNA_FORMAT_BGR8";
      return;
    case LUNA_FORMAT_BGRA8:
      *dst = "LUNA_FORMAT_BGRA8";
      return;
    case LUNA_FORMAT_RGB16:
      *dst = "LUNA_FORMAT_RGB16";
      return;
    case LUNA_FORMAT_RGBA16:
      *dst = "LUNA_FORMAT_RGBA16";
      return;
    case LUNA_FORMAT_RG32:
      *dst = "LUNA_FORMAT_RG32";
      return;
    case LUNA_FORMAT_RGB32:
      *dst = "LUNA_FORMAT_RGB32";
      return;
    case LUNA_FORMAT_RGBA32:
      *dst = "LUNA_FORMAT_RGBA32";
      return;
    case LUNA_FORMAT_R8_SINT:
      *dst = "LUNA_FORMAT_R8_SINT";
      return;
    case LUNA_FORMAT_RG8_SINT:
      *dst = "LUNA_FORMAT_RG8_SINT";
      return;
    case LUNA_FORMAT_RGB8_SINT:
      *dst = "LUNA_FORMAT_RGB8_SINT";
      return;
    case LUNA_FORMAT_RGBA8_SINT:
      *dst = "LUNA_FORMAT_RGBA8_SINT";
      return;
    case LUNA_FORMAT_R8_UINT:
      *dst = "LUNA_FORMAT_R8_UINT";
      return;
    case LUNA_FORMAT_RG8_UINT:
      *dst = "LUNA_FORMAT_RG8_UINT";
      return;
    case LUNA_FORMAT_RGB8_UINT:
      *dst = "LUNA_FORMAT_RGB8_UINT";
      return;
    case LUNA_FORMAT_RGBA8_UINT:
      *dst = "LUNA_FORMAT_RGBA8_UINT";
      return;
    case LUNA_FORMAT_R8_SRGB:
      *dst = "LUNA_FORMAT_R8_SRGB";
      return;
    case LUNA_FORMAT_RG8_SRGB:
      *dst = "LUNA_FORMAT_RG8_SRGB";
      return;
    case LUNA_FORMAT_RGB8_SRGB:
      *dst = "LUNA_FORMAT_RGB8_SRGB";
      return;
    case LUNA_FORMAT_RGBA8_SRGB:
      *dst = "LUNA_FORMAT_RGBA8_SRGB";
      return;
    case LUNA_FORMAT_BGR8_SRGB:
      *dst = "LUNA_FORMAT_BGR8_SRGB";
      return;
    case LUNA_FORMAT_BGRA8_SRGB:
      *dst = "LUNA_FORMAT_BGRA8_SRGB";
      return;
    case LUNA_FORMAT_D16:
      *dst = "LUNA_FORMAT_D16";
      return;
    case LUNA_FORMAT_D24:
      *dst = "LUNA_FORMAT_D24";
      return;
    case LUNA_FORMAT_D32:
      *dst = "LUNA_FORMAT_D32";
      return;
    case LUNA_FORMAT_D24_S8:
      *dst = "LUNA_FORMAT_D24_S8";
      return;
    case LUNA_FORMAT_D32_S8:
      *dst = "LUNA_FORMAT_D32_S8";
      return;
    case LUNA_FORMAT_BC1:
      *dst = "LUNA_FORMAT_BC1";
      return;
    case LUNA_FORMAT_BC3:
      *dst = "LUNA_FORMAT_BC3";
      return;
    case LUNA_FORMAT_BC7:
      *dst = "LUNA_FORMAT_BC7";
      return;
  }
}

bool luna_FormatHasColorChannel(lunaFormat fmt)
{
  switch (fmt)
  {
    case LUNA_FORMAT_D16:
    case LUNA_FORMAT_D24:
    case LUNA_FORMAT_D24_S8:
    case LUNA_FORMAT_D32:
    case LUNA_FORMAT_D32_S8:
    case LUNA_FORMAT_BC1:
    case LUNA_FORMAT_BC3:
    case LUNA_FORMAT_BC7:
    case LUNA_FORMAT_UNDEFINED:
      return 0;
    default:
      return 1;
  }
}

// Returns false even for stencil/depth and undefined format
bool luna_FormatHasAlphaChannel(lunaFormat fmt)
{
  switch (fmt)
  {
    case LUNA_FORMAT_RGBA8:
    case LUNA_FORMAT_BGRA8:
    case LUNA_FORMAT_RGBA16:
    case LUNA_FORMAT_RGBA32:
    case LUNA_FORMAT_RGBA8_SINT:
    case LUNA_FORMAT_RGBA8_UINT:
    case LUNA_FORMAT_RGBA8_SRGB:
    case LUNA_FORMAT_BGRA8_SRGB:
      return 1;
    default:
      return 0;
  }
}

bool luna_FormatHasDepthChannel(lunaFormat fmt)
{
  switch (fmt)
  {
    case LUNA_FORMAT_D16:
    case LUNA_FORMAT_D24:
    case LUNA_FORMAT_D24_S8:
    case LUNA_FORMAT_D32:
    case LUNA_FORMAT_D32_S8:
      return 1;

    default:
      return 0;
  }
}

bool luna_FormatHasStencilChannel(lunaFormat fmt)
{
  switch (fmt)
  {
    case LUNA_FORMAT_D24_S8:
    case LUNA_FORMAT_D32_S8:
      return 1;

    default:
      return 0;
  }
}

int luna_FormatGetBytesPerChannel(lunaFormat fmt)
{
  switch (fmt)
  {
    case LUNA_FORMAT_R8:
    case LUNA_FORMAT_RG8:
    case LUNA_FORMAT_RGB8:
    case LUNA_FORMAT_RGBA8:
    case LUNA_FORMAT_BGR8:
    case LUNA_FORMAT_BGRA8:
    case LUNA_FORMAT_R8_SINT:
    case LUNA_FORMAT_RG8_SINT:
    case LUNA_FORMAT_RGB8_SINT:
    case LUNA_FORMAT_RGBA8_SINT:
    case LUNA_FORMAT_R8_UINT:
    case LUNA_FORMAT_RG8_UINT:
    case LUNA_FORMAT_RGB8_UINT:
    case LUNA_FORMAT_RGBA8_UINT:
    case LUNA_FORMAT_R8_SRGB:
    case LUNA_FORMAT_RG8_SRGB:
    case LUNA_FORMAT_RGB8_SRGB:
    case LUNA_FORMAT_RGBA8_SRGB:
    case LUNA_FORMAT_BGR8_SRGB:
    case LUNA_FORMAT_BGRA8_SRGB:
      return 1;

    case LUNA_FORMAT_RGB16:
    case LUNA_FORMAT_RGBA16:
    case LUNA_FORMAT_D16:
      return 2;

    case LUNA_FORMAT_D24:
    case LUNA_FORMAT_D24_S8:
      return 3;

    case LUNA_FORMAT_RG32:
    case LUNA_FORMAT_RGB32:
    case LUNA_FORMAT_RGBA32:
    case LUNA_FORMAT_D32:
    case LUNA_FORMAT_D32_S8:
      return 4;

    default:
    case LUNA_FORMAT_BC1:
    case LUNA_FORMAT_BC3:
    case LUNA_FORMAT_BC7:
    case LUNA_FORMAT_UNDEFINED:
      return -1;
  }
}

int luna_FormatGetBytesPerPixel(lunaFormat fmt)
{
  return luna_FormatGetBytesPerChannel(fmt) * luna_FormatGetNumChannels(fmt);
}

int luna_FormatGetNumChannels(lunaFormat fmt)
{
  switch (fmt)
  {
    case LUNA_FORMAT_R8:
    case LUNA_FORMAT_R8_SINT:
    case LUNA_FORMAT_R8_UINT:
    case LUNA_FORMAT_R8_SRGB:
    case LUNA_FORMAT_D16:
    case LUNA_FORMAT_D24:
    case LUNA_FORMAT_D32:
      return 1;

    case LUNA_FORMAT_RG8:
    case LUNA_FORMAT_RG32:
    case LUNA_FORMAT_RG8_SINT:
    case LUNA_FORMAT_RG8_UINT:
    case LUNA_FORMAT_RG8_SRGB:
    case LUNA_FORMAT_D24_S8:
    case LUNA_FORMAT_D32_S8:
      return 2;

    case LUNA_FORMAT_RGB8:
    case LUNA_FORMAT_BGR8:
    case LUNA_FORMAT_RGB16:
    case LUNA_FORMAT_RGB32:
    case LUNA_FORMAT_RGB8_SINT:
    case LUNA_FORMAT_RGB8_UINT:
    case LUNA_FORMAT_RGB8_SRGB:
    case LUNA_FORMAT_BGR8_SRGB:
      return 3;

    case LUNA_FORMAT_RGBA8:
    case LUNA_FORMAT_BGRA8:
    case LUNA_FORMAT_RGBA16:
    case LUNA_FORMAT_RGBA32:
    case LUNA_FORMAT_RGBA8_SINT:
    case LUNA_FORMAT_RGBA8_UINT:
    case LUNA_FORMAT_RGBA8_SRGB:
    case LUNA_FORMAT_BGRA8_SRGB:
      return 4;

    case LUNA_FORMAT_BC1:
    case LUNA_FORMAT_BC3:
    case LUNA_FORMAT_BC7:
      // FIXME: Implement
      return 0;

    case LUNA_FORMAT_UNDEFINED:
    default:
      return 0;
  }
}

void* luna_memcpy(void* dst, size_t dstsz, const void* src, size_t sz)
{
  cassert(dst != NULL);
  cassert(src != NULL);
  cassert(dstsz != 0);
  cassert(sz != 0);

  // I saw this optimization trick a long time ago in some big codebase
  // and it has sticken to me
  // do you know how much I just get an ITCH to write memcpy myself?
  if (((uintptr_t) src & 0x3) == 0 && ((uintptr_t) dst & 0x3) == 0)
  {
    const int* read  = (const int*) src;
    int*       write = (int*) dst;

    const size_t int_count = sz / sizeof(int);
    for (size_t i = 0; i < int_count; i++)
    {
      write[i] = read[i];
    }

    const uchar* byte_read  = (uchar*) (read + int_count);
    uchar*       byte_write = (uchar*) (write + int_count);

    sz %= sizeof(int);
    for (size_t i = 0; i < sz; i++)
    {
      byte_write[i] = byte_read[i];
    }
  }
  else
  {
    const uchar* read  = (const uchar*) src;
    uchar*       write = (uchar*) dst;
    for (size_t i = 0; i < sz; i++)
    {
      write[i] = read[i];
    }
  }

  return dst;
}

void* luna_memset(void* dst, ubyte to, size_t sz)
{
  cassert(dst != NULL);
  cassert(sz != 0);

  if (((uintptr_t) dst & 0x3) == 0)
  {
    int* write = (int*) dst;

    int i_to = (to << 24) | (to << 16) | (to << 8) | to;

    const size_t int_count = sz / sizeof(int);
    for (size_t i = 0; i < int_count; i++)
    {
      write[i] = i_to;
    }

    uchar* byte_write = (uchar*) (write + int_count);

    sz %= sizeof(int);
    for (size_t i = 0; i < sz; i++)
    {
      byte_write[i] = i_to;
    }
  }
  else
  {
    uchar* write = (uchar*) dst;
    for (size_t i = 0; i < sz; i++)
    {
      write[i] = to;
    }
  }

  return dst;
}

void* luna_memmove(void* dst, size_t dstsz, const void* src, size_t sz)
{
  void* ret = luna_memcpy(dst, dstsz, src, sz);
  if (!ret)
  {
    return NULL;
  }
  return luna_memset(dst, 0, sz);
}

void* luna_malloc(size_t sz)
{
  void* ptr = malloc(sz);
  cassert(ptr != NULL);
  return ptr;
}
