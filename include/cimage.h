#ifndef __CGFXTEXTURE_H
#define __CGFXTEXTURE_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "cgfxstdafx.h"

typedef struct ctex2D {
    int w, h;
    cformat fmt;
    unsigned char *data;
} ctex2D;

extern ctex2D cimg_load(const char *path);
extern ctex2D cimg_load_png(const char *path);
extern ctex2D cimg_load_jpg(const char *path);

extern void cimg_write(const ctex2D *tex, const char *path);
extern void cimg_write_png(const ctex2D *tex, const char *path);
extern void cimg_write_jpg(const ctex2D *tex, const char *path);

#ifdef __cplusplus
    }
#endif

#endif//__CGFXTEXTURE_H