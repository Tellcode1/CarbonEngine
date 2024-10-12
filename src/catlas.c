#include "../../include/catlas.h"
#include "../../include/math/math.h"
#include <stdlib.h>

catlas_t catlas_init(int init_w, int init_h)
{
    catlas_t atlas = {};

    atlas.width = init_w;
    atlas.height = init_h;
    atlas.next_x = 0;
    atlas.next_y = 0;
    atlas.current_row_height = 0;
    atlas.data = calloc(init_w, init_h);

    return atlas;
}

bool_t catlas_add_image(catlas_t *__restrict__ atlas, int w, int h, const unsigned char *__restrict__ data, int *x, int *y)
{
    const int padding = 4;
    const int prev_h = atlas->height, prev_w = atlas->width;
    bool_t realloc_needed = 0;
    if (w > atlas->width) {
        // ! This doesn't work because the old image is not correctly copied by realloc
        // ! It'll be fixed by copying over the data row by row
        // TODO: FIXME
        atlas->width = w;
        realloc_needed = 1;
    }

    if (atlas->next_x + w + padding > atlas->width) {
        atlas->next_x = 0;
        atlas->next_y += atlas->current_row_height + padding;
        atlas->current_row_height = 0;
    }

    if (atlas->next_y + h + padding > atlas->height) {
        atlas->height = cmmax(atlas->height * 2, atlas->next_y + h + padding);
        realloc_needed = 1;
    }

    if (realloc_needed) {
        atlas->data = realloc(atlas->data, atlas->width * atlas->height);
        memset(atlas->data + prev_w * prev_h, 0, (atlas->width - prev_w) * (atlas->height - prev_h));
    }

    for (int y = 0; y < h; y++) {
        memcpy(atlas->data + (atlas->next_x + (atlas->next_y + y) * atlas->width), data + (y * w), w);
    }

    *x = atlas->next_x;
    *y = atlas->next_y;

    atlas->next_x += w + padding;
    atlas->current_row_height = cmmax(atlas->current_row_height, h + padding);

    return 0;
}