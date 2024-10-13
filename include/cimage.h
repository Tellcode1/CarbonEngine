#ifndef __CGFXTEXTURE_H
#define __CGFXTEXTURE_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "cgfxstdafx.h"

typedef struct cg_tex2D {
    int w, h;
    cg_format fmt;
    unsigned char *data;
} cg_tex2D;

extern cg_tex2D cimg_load(const char *path);
extern cg_tex2D cimg_load_png(const char *path);
extern cg_tex2D cimg_load_jpg(const char *path);

extern void cimg_write(const cg_tex2D *tex, const char *path);
extern void cimg_write_png(const cg_tex2D *tex, const char *path);
extern void cimg_write_jpg(const cg_tex2D *tex, const char *path);

#ifdef __cplusplus
    }
#endif

#endif//__CGFXTEXTURE_H