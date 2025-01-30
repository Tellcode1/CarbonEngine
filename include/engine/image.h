#ifndef __NOVA_IMAGE_H__
#define __NOVA_IMAGE_H__

#include "../../common/stdafx.h"
#include "../GPU/format.h"

NOVA_HEADER_START;

// CPU Image
typedef struct NVImage {
  int w, h;
  NVFormat fmt;
  unsigned char *data;
} NVImage;

extern NVImage NVImage_Load(const char *path);
extern NVImage NVImage_LoadPNG(const char *path);
extern NVImage NVImage_LoadJPEG(const char *path);

extern void NVImage_Write(const NVImage *tex, const char *path);
extern void NVImage_WritePNG(const NVImage *tex, const char *path);
extern void NVImage_WriteJPEG(const NVImage *tex, const char *path);

// dst_channels must be greater than src channels!
unsigned char *NVImage_PadChannels(const NVImage *src, int dst_channels);

NOVA_HEADER_END;

#endif //__NOVA_IMAGE_H__