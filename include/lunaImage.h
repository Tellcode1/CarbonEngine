#ifndef __CGFXTEXTURE_H
#define __CGFXTEXTURE_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "lunaGFXstdafx.h"
#include "GPU/format.h"

// CPU Image
typedef struct lunaImage {
    int w, h;
    lunaFormat fmt;
    unsigned char *data;
} lunaImage;

extern lunaImage lunaImage_Load(const char *path);
extern lunaImage lunaImage_LoadPNG(const char *path);
extern lunaImage lunaImage_LoadJPEG(const char *path);

extern void lunaImage_Write(const lunaImage *tex, const char *path);
extern void lunaImage_WritePNG(const lunaImage *tex, const char *path);
extern void lunaImage_WriteJPEG(const lunaImage *tex, const char *path);

// dst_channels must be greater than src channels!
uint8_t *lunaImage_PadChannels(const lunaImage *src, int dst_channels);

#ifdef __cplusplus
    }
#endif

#endif//__CGFXTEXTURE_H