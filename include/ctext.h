#ifndef __C_TEXT_H__
#define __C_TEXT_H__

#ifdef __cplusplus
    extern "C" {
#endif

#include "camera.h"

#include "../include/math/vec3.h"
#include "../include/math/vec2.h"
#include "../include/math/mat.h"

#include "stdafx.h"
#include "vkstdafx.h"

#include "cgvector.h"
#include "cgstring.h"
#include "cghashmap.h"
#include "catlas.h"
#include "cdevicememory.h"

typedef struct luna_Renderer_t luna_Renderer_t;

typedef enum HoriAlignment
{
    CTEXT_HORI_ALIGN_LEFT = 0,
    CTEXT_HORI_ALIGN_CENTER = 1,
    CTEXT_HORI_ALIGN_RIGHT = 2,
} HoriAlignment;

typedef enum VertAlignment
{
    CTEXT_VERT_ALIGN_TOP = 0,
    CTEXT_VERT_ALIGN_CENTER = 1,
    CTEXT_VERT_ALIGN_BOTTOM = 2,
} VertAlignment;

// Since the hash is to be used for individual characters, we can expect
// that there will only be one entry for each character.
static inline unsigned ctext_hash(const void *key, int nbytes) {
    (void)nbytes;
    return *(char *)key;
}

// DEPRECATE THIS YOU FOOL
// IT WAS ONLY MEANT FOR SIMPLE TESTING
static const int CTEXT_MAX_FONT_COUNT = 8;
typedef struct cfont_t cfont_t;

typedef struct ctext_text_render_info {
    HoriAlignment horizontal;
    VertAlignment vertical;
    vec4 color;
    vec3 position;
    f32 scale;
} ctext_text_render_info;

// Hmm.. sprintf doesn't work with unicode I guess.
// I'll figure out what to do later.
extern void ctext_render(cfont_t *fnt, const ctext_text_render_info *pInfo, const char *fmt, ...);
extern void ctext_init(struct luna_Renderer_t *rd);

extern void ctext_load_font(luna_Renderer_t *rd, const char *font_path, int scale, cfont_t **dst);

extern void ctext_begin_render(cfont_t *fnt);
extern void ctext_end_render(luna_Renderer_t *rd, ccamera *camera, cfont_t *fnt, mat4 model);

typedef struct ctext_glyph
{
    float x0,x1,y0,y1;
    float l,r,b,t;
    f32 advance;
} ctext_glyph;

typedef struct ctext_text_drawcall_t ctext_text_drawcall_t;

/* Internal CFont struct. Do not modify yourselves! */
typedef struct cfont_t
{
    char family_name[ 128 ];
    char style_name[ 128 ];
    catlas_t atlas;

    float line_height;
    float space_width;

    int font_index;
    luna_GPU_Texture texture;
    luna_GPU_Memory texture_mem;
    luna_GPU_Sampler sampler;

    int allocated_size;
    luna_GPU_Buffer buffer;
    luna_GPU_Memory buffer_mem;
    void *mapped;

    int index_buffer_offset;
    int index_count;
    bool to_render;

    int chars_drawn;
    cg_vector_t /* ctext_text_drawcall_t */  drawcalls;
    cg_hashmap_t * /* unicode, ctext_glyph ctext_hasher<unicode>> */ glyph_map;
} cfont_t;

#ifdef __cplusplus
    }
#endif

#endif