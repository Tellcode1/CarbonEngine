#ifndef __NOVA_ATLAS_H__
#define __NOVA_ATLAS_H__

#include "../stdafx.h"
#include "../mem.h"

NOVA_HEADER_START;

typedef struct NV_atlas_t {
    int width, height, next_x, next_y, current_row_height;
    unsigned char *data;
    NVAllocator *allocator;
} NV_atlas_t;

extern NV_atlas_t NV_atlas_init(int init_w, int init_h);
extern bool NV_atlas_add_image(NV_atlas_t *__restrict__ atlas, int w, int h, const unsigned char *__restrict__ data, int *__restrict__ x, int *__restrict__ y);

NOVA_HEADER_END;

#endif//__NOVA_ATLAS_H__