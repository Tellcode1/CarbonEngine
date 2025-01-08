#ifndef __CGFXTEXTURE_H
#define __CGFXTEXTURE_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "../engine/lunaGFXstdafx.h"
#include "../GPU/format.h"

// CPU Image
typedef struct lunaImage {
    int w, h;
    lunaFormat fmt;
    unsigned char *data;
} lunaImage;

extern int lunaImage_Compress(const unsigned char *input, size_t input_size, unsigned char *output, size_t *output_size);
extern int lunaImage_Decompress(const unsigned char *compressed_data, size_t compressed_size, unsigned char *o_buf, size_t o_buf_sz);

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