#ifndef __CATLAS_H
#define __CATLAS_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "defines.h"

typedef struct catlas_t {
    int width, height, next_x, next_y, current_row_height;
    unsigned char *data;
} catlas_t;

extern catlas_t catlas_init(int init_w, int init_h);
extern bool_t catlas_add_image(catlas_t *__restrict__ atlas, int w, int h, const unsigned char *__restrict__ data, int *x, int *y);

#ifdef __cplusplus
    }
#endif

#endif//__CATLAS_H