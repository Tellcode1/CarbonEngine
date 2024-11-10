#ifndef __CGFXTEXTURE_H
#define __CGFXTEXTURE_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "lunaGFXstdafx.h"

// CPU Image
typedef struct luna_Image {
    int w, h;
    VkFormat fmt;
    unsigned char *data;
} luna_Image;

extern luna_Image luna_ImageLoad(const char *path);
extern luna_Image luna_ImageLoadPNG(const char *path);
extern luna_Image luna_ImageLoadJPEG(const char *path);

extern void luna_ImageWrite(const luna_Image *tex, const char *path);
extern void luna_ImageWritePNG(const luna_Image *tex, const char *path);
extern void luna_ImageWriteJPEG(const luna_Image *tex, const char *path);

#ifdef __cplusplus
    }
#endif

#endif//__CGFXTEXTURE_H