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

extern NVImage NV_img_load(const char *path);
extern NVImage NV_img_load_png(const char *path);

// jpg and jpeg (they're the same thing by the way)
extern NVImage NV_img_load_jpeg(const char *path);

extern void NV_img_write_(const NVImage *tex, const char *path);
extern void NV_img_write_png(const NVImage *tex, const char *path);
extern void NV_img_write_jpeg(const NVImage *tex, const char *path);

// dst_channels must be greater than src channels!
unsigned char *NV_img_pad_channels(const NVImage *src, int dst_channels);

NOVA_HEADER_END;

#endif //__NOVA_IMAGE_H__