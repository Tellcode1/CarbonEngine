#ifndef __C_TEXT_HPP__
#define __C_TEXT_HPP__

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

typedef struct crenderer_t crenderer_t;

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

typedef enum ccharset {
    CHARSET_ASCII = 0,
    CHARSET_LATIN1_SUPPLEMENT,
    CHARSET_LATIN_EXTENDED_A,
    CHARSET_LATIN_EXTENDED_B,
    CHARSET_GREEK_COPTIC,
    CHARSET_CYRILLIC,
    CHARSET_HEBREW,
    CHARSET_ARABIC,
    CHARSET_DEVANAGARI,
    CHARSET_CJK,
    CHARSET_HANGUL,
    CHARSET_BASIC_EMOJI,
    CHARSET_MATHEMATICAL_SYMBOLS,
    CHARSET_PRIVATE_USE_AREA_A,
    CHARSET_PRIVATE_USE_AREA_B
} ccharset;

static inline unsigned ctext_hash(const void *key, int nbytes) {
    return *(char *)key;
}

static const u32 CTEXT_MAX_FONT_COUNT = 8;
typedef struct cfont_t cfont_t;

typedef struct ctext_font_load_info ctext_font_load_info;

typedef struct ctext_text_render_info {
    HoriAlignment horizontal;
    VertAlignment vertical;
    vec3 position;
    f32 scale;
} ctext_text_render_info;

// Hmm.. sprintf doesn't work with unicode I guess.
// I'll figure out what to do later.
extern void ctext_render(cfont_t *fnt, const ctext_text_render_info *pInfo, const char *fmt, ...);
extern void ctext_init(struct crenderer_t *rd);

extern void ctext_load_font(crenderer_t *rd, const ctext_font_load_info* pInfo, cfont_t **dst);

extern void ctext_begin_render(crenderer_t *rd, cfont_t *fnt);
extern void ctext_end_render(crenderer_t *rd, ccamera *camera, cfont_t *fnt, mat4 model);

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
    catlas_t atlas;
    f32 pixel_range;

    f32 line_height;
    f32 space_width;

    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    u32 font_index;
    VkImage texture;
    VkImageView texture_view;
    VkDeviceMemory texture_memory;
    VkSampler sampler;

    u32 allocated_size;
    VkBuffer buffer;
    VkDeviceMemory buffer_memory;
    void *mapped;

    int index_buffer_offset;
    int index_count;
    bool to_render;

    u32 chars_drawn;
    cg_vector_t /* ctext_text_drawcall_t */  drawcalls;
    cg_hashmap_t * /* unicode, ctext_glyph ctext_hasher<unicode>> */ glyph_map;
} cfont_t;

void ctext__font_resize_buffer(cfont_t *fnt,  u32 new_buffer_size, u32 index_buffer_offset);

typedef struct ctext_font_load_info
{
    const char *fontPath;
    f32 scale;
    ccharset chset;
} ctext_font_load_info;
inline ctext_font_load_info ctext_font_load_info_init() {
    return (ctext_font_load_info) {
        .fontPath = 0,
        .scale = 24.0f,
        .chset = CHARSET_ASCII
    };
}

#ifdef __cplusplus
    }
#endif

#endif