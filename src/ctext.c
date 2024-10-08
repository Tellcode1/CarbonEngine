#include "../include/ctext.h"
#include "../include/defines.h"
#include "../include/math/math.h"

#include "../include/cgfx.h"

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

FT_Library lib = NULL;

typedef struct {
    unsigned char *data;
    int width, height;
    int next_x, next_y, current_row_height;
} cdynamic_atlas;

cdynamic_atlas* atlas_create(int width, int height) {
    cdynamic_atlas *atlas = calloc(1, sizeof(cdynamic_atlas));
    atlas->data = calloc(width, height);
    atlas->width = width;
    atlas->height = height;
    atlas->next_x = 0;
    atlas->next_y = 0;
    atlas->current_row_height = 0;
    return atlas;
}

void atlas_destroy(cdynamic_atlas *atlas) {
    free(atlas->data);
    free(atlas);
}

int atlas_add_glyph(cdynamic_atlas *atlas, const unsigned char *glyph_data, int glyph_w, int glyph_h, cglyph_info *glyph_info) {
    bool_t need_realloc = 0;
    const int padding = 16;

    glyph_w += 2 * padding;
    glyph_h += 2 * padding;

    if (atlas->next_x + glyph_w > atlas->width) {
        atlas->next_x = 0;
        atlas->next_y += atlas->current_row_height;
        atlas->current_row_height = 0;
    }

    if (atlas->next_y + glyph_h > atlas->height) {
        atlas->height = cmmax(atlas->next_y + glyph_h, atlas->height + glyph_h);
        need_realloc = 1;
    }

    if (need_realloc) {
        atlas->data = realloc(atlas->data, atlas->width * atlas->height);
        cassert(atlas->data != NULL);
    }

    for (int y = 0; y < (glyph_h - 2 * padding); y++) {
        memcpy(
            atlas->data + (atlas->next_y + y) * atlas->width + atlas->next_x,
            glyph_data + y * (glyph_w - 2 * padding),
            (glyph_w - 2 * padding)
        );
    }

    glyph_info->x = atlas->next_x;
    glyph_info->y = atlas->next_y;
    glyph_info->w = glyph_w;
    glyph_info->h = glyph_h;

    atlas->next_x += glyph_w;
    if (glyph_h > atlas->current_row_height) {
        atlas->current_row_height = glyph_h;
    }

    return 0;
}

typedef struct cfont_t {
    cdynamic_atlas *atlas;
    FT_Face face;
    int pixel_sizes;
} cfont_t;

#include "../include/cimage.h"

/* info will then contain the position of the glyph. */
bool ctext_load_glyph(cfont_t *fnt, cglyph_info *info, unicode code) {
    FT_Set_Pixel_Sizes(fnt->face, 0, fnt->pixel_sizes);

    const unicode codepoint = FT_Get_Char_Index(fnt->face, code);
    if (codepoint != 0) {
        cassert(FT_Load_Glyph(fnt->face, codepoint, FT_LOAD_DEFAULT) == FT_Err_Ok);
        cassert(FT_Render_Glyph(fnt->face->glyph, FT_RENDER_MODE_NORMAL) == FT_Err_Ok);

        info->w = fnt->face->glyph->bitmap.width;
        info->h = fnt->face->glyph->bitmap.rows;
        unsigned char *buffer = fnt->face->glyph->bitmap.buffer;

        cassert(atlas_add_glyph(fnt->atlas, buffer, info->w, info->h, info) == 0);
    }
    return 0;
}

int ctext_initialize(crenderer_t *rd)
{
    
    return 0;
}

cfont_t *ctext_load_font(const cfont_load_info *info) 
{
    cfont_t *fnt = malloc(sizeof(cfont_t));
    fnt->atlas = atlas_create(1024, 1024);

    if (!lib) {
        if (FT_Init_FreeType(&lib))
        {
            LOG_ERROR("failed to initialize ft\n");
        }
    }

    if (FT_New_Face(lib, info->path, 0, &fnt->face))
    {
        LOG_ERROR("failed to load font file: %s\n", info->path);
        FT_Done_FreeType(lib);
    }

    fnt->pixel_sizes = info->pixel_sizes;

    return fnt;
}

void balls(const cfont_t *fnt)
{
    ctex2D tex = {};
    tex.w = fnt->atlas->width;
    tex.h = fnt->atlas->height;
    tex.fmt = CFMT_R8_UINT;
    tex.data = fnt->atlas->data;
    printf("atlas width -> %i, atlas height -> %i", tex.w, tex.h);
    cimg_write_png(&tex, "skdlfj.png");
}