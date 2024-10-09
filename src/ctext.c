#include "../include/ctext.h"
#include "../include/defines.h"
#include "../include/math/math.h"

#include "../include/cgfx.h"

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

typedef struct {
    unsigned char *data;
    int width, height;
    int next_x, next_y, current_row_height;
} cdynamic_atlas;

typedef struct cfont_t {
    cdynamic_atlas *atlas;
    FT_Face face;
    int pixel_sizes;
    cglyph glyphs[ 128 ];
} cfont_t;

FT_Library lib = NULL;

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

int atlas_add_glyph(cdynamic_atlas *atlas, const unsigned char *glyph_data, int glyph_w, int glyph_h, cglyph_info *glyph_info, int padding) {
    cg_bool_t need_realloc = 0;
    const int prev_h = atlas->height;
    const int effective_glyph_w = glyph_w + 2 * padding;
    const int effective_glyph_h = glyph_h + 2 * padding;

    if (atlas->next_x + effective_glyph_w > atlas->width) {
        atlas->next_x = 0;
        atlas->next_y += atlas->current_row_height;
        atlas->current_row_height = 0;
    }

    if (atlas->next_y + effective_glyph_h > atlas->height) {
        int new_height = cmmax(atlas->next_y + effective_glyph_h, atlas->height + effective_glyph_h);
        need_realloc = 1;
        atlas->height = new_height;
    }

    if (need_realloc) {
        unsigned char *new_data = realloc(atlas->data, atlas->width * atlas->height);
        if (!new_data) {
            return -1;
        }
        atlas->data = new_data;
        memset(atlas->data + prev_h * atlas->width, 0, (atlas->height - prev_h) * atlas->width);
    }

    for (int y = 0; y < glyph_h; y++) {
        memcpy(
            atlas->data + (atlas->next_y + y + padding) * atlas->width + (atlas->next_x + padding),
            glyph_data + y * glyph_w,
            glyph_w
        );
    }

    glyph_info->x = atlas->next_x + padding;
    glyph_info->y = atlas->next_y + padding;
    glyph_info->w = glyph_w;
    glyph_info->h = glyph_h;

    atlas->next_x += effective_glyph_w;
    if (effective_glyph_h > atlas->current_row_height) {
        atlas->current_row_height = effective_glyph_h;
    }

    return 0;
}

#include "../include/cimage.h"

/* info will then contain the position of the glyph. */
bool ctext_load_glyph(cfont_t *fnt, cglyph_info *info, cglyph *retglyph, unicode code) {
    FT_Set_Pixel_Sizes(fnt->face, 0, fnt->pixel_sizes);

    const unicode codepoint = FT_Get_Char_Index(fnt->face, code);
    if (codepoint != 0) {
        cassert(FT_Load_Glyph(fnt->face, codepoint, FT_LOAD_DEFAULT) == FT_Err_Ok);
        cassert(FT_Render_Glyph(fnt->face->glyph, FT_RENDER_MODE_NORMAL) == FT_Err_Ok);

        info->w = fnt->face->glyph->bitmap.width;
        info->h = fnt->face->glyph->bitmap.rows;
        unsigned char *buffer = fnt->face->glyph->bitmap.buffer;

        const FT_GlyphSlot glyph = fnt->face->glyph;
        retglyph->x0 = glyph->metrics.horiBearingX / 64.0f;
        retglyph->y0 = -glyph->metrics.horiBearingY / 64.0f;
        retglyph->x1 = (glyph->metrics.horiBearingX + glyph->metrics.width) / 64.0f;
        retglyph->y1 = (glyph->metrics.height - glyph->metrics.horiBearingY) / 64.0f;

        retglyph->l = (info->x / (f32)fnt->atlas->width);
        retglyph->r = ((info->x + info->w) / (f32)fnt->atlas->width);
        retglyph->b = (info->y / (f32)fnt->atlas->height);
        retglyph->t = ((info->y + info->h) / (f32)fnt->atlas->height);

        retglyph->advance = glyph->metrics.horiAdvance / 64.0f;

        cassert(atlas_add_glyph(fnt->atlas, buffer, info->w, info->h, info, 2) == 0);
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

    for (int i = U'A'; i < U'Z'+1; i++) {
        cglyph_info glyph_info = {};
        ctext_load_glyph(fnt, &glyph_info, &fnt->glyphs[i], i);
    }

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