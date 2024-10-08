#ifndef __CTEXT_H
#define __CTEXT_H

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdlib.h>
#include <stdbool.h>

typedef struct cglyph_info {
    int x, y, w, h;
} cglyph_info;

typedef struct cfont_load_info {
    const char *path;
    int pixel_sizes;
} cfont_load_info;

typedef struct cfont_t cfont_t;
typedef unsigned unicode;

// extern int ctext_initialize(struct crenderer_t *rd);
extern cfont_t *ctext_load_font(const cfont_load_info *info);
extern bool ctext_load_glyph(cfont_t *fnt, cglyph_info *info, unicode code);
extern cfont_t *ctext_load_font(const cfont_load_info *info);

extern void balls(const cfont_t *fnt);

#ifdef __cplusplus
    }
#endif

#endif//__CTEXT_H