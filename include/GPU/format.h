#ifndef __NOVA_FORMAT_H__
#define __NOVA_FORMAT_H__

#include "../../common/stdafx.h"

NOVA_HEADER_START;

typedef uint32_t VkFormat_;

typedef enum NVFormat {
  NOVA_FORMAT_UNDEFINED = 0,

  // These are UNORM's by the way.
  // Because that's how images are normally handled in computers.
  NOVA_FORMAT_R8,
  NOVA_FORMAT_RG8,
  NOVA_FORMAT_RGB8,
  NOVA_FORMAT_RGBA8,

  NOVA_FORMAT_BGR8,
  NOVA_FORMAT_BGRA8,

  NOVA_FORMAT_RGB16,
  NOVA_FORMAT_RGBA16,
  NOVA_FORMAT_RG32,
  NOVA_FORMAT_RGB32,
  NOVA_FORMAT_RGBA32,

  NOVA_FORMAT_R8_SINT,
  NOVA_FORMAT_RG8_SINT,
  NOVA_FORMAT_RGB8_SINT,
  NOVA_FORMAT_RGBA8_SINT,

  NOVA_FORMAT_R8_UINT,
  NOVA_FORMAT_RG8_UINT,
  NOVA_FORMAT_RGB8_UINT,
  NOVA_FORMAT_RGBA8_UINT,

  NOVA_FORMAT_R8_SRGB,
  NOVA_FORMAT_RG8_SRGB,
  NOVA_FORMAT_RGB8_SRGB,
  NOVA_FORMAT_RGBA8_SRGB,

  NOVA_FORMAT_BGR8_SRGB,
  NOVA_FORMAT_BGRA8_SRGB,

  NOVA_FORMAT_D16,
  NOVA_FORMAT_D24,
  NOVA_FORMAT_D32,
  NOVA_FORMAT_D24_S8,
  NOVA_FORMAT_D32_S8,

  NOVA_FORMAT_BC1,
  NOVA_FORMAT_BC3,
  NOVA_FORMAT_BC7,
} NVFormat;

extern VkFormat_ NV_NVFormatToVKFormat(NVFormat format);

extern NVFormat NV_VKFormatToNVFormat(VkFormat_ format);

// dst is a pointer to a const char *
// like:
// const char *str; NV_FormatToString(NOVA_FORMAT_R8, &str);
extern void NV_FormatToString(NVFormat format, const char **dst);

extern bool NV_FormatHasColorChannel(NVFormat fmt);

// Returns false even for stencil/depth and undefined format
extern bool NV_FormatHasAlphaChannel(NVFormat fmt);

extern bool NV_FormatHasDepthChannel(NVFormat fmt);

extern bool NV_FormatHasStencilChannel(NVFormat fmt);

// returns -1 on invalid format (or compressed formats)
// returns the size of depth component (in bytes) even in combined depth stencil formats
extern int NV_FormatGetBytesPerChannel(NVFormat fmt);

extern int NV_FormatGetBytesPerPixel(NVFormat fmt);

// 0 on error or undefined format.
// oh and I haven't implemented compressed formats yet..
// also, stencil channels are also counted
extern int NV_FormatGetNumChannels(NVFormat fmt);

NOVA_HEADER_END;

#endif //__NOVA_FORMAT_H__