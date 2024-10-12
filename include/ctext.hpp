#ifndef __C_TEXT_HPP__
#define __C_TEXT_HPP__

#include "camera.hpp"

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

enum HoriAlignment
{
    CTEXT_HORI_ALIGN_LEFT = 0,
    CTEXT_HORI_ALIGN_CENTER = 1,
    CTEXT_HORI_ALIGN_RIGHT = 2,
};

enum VertAlignment
{
    CTEXT_VERT_ALIGN_TOP = 0,
    CTEXT_VERT_ALIGN_CENTER = 1,
    CTEXT_VERT_ALIGN_BOTTOM = 2,
};

enum ccharset {
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
};

static inline unsigned ctext_hash(const void *key, int nbytes) {
    return *(char *)key;
}

struct ctext
{
    constexpr static u32 MAX_FONT_COUNT = 8;

    struct cfont_t;

    struct CFontLoadInfo;

    struct text_render_info {
        HoriAlignment horizontal;
        VertAlignment vertical;
        vec3 position;
        f32 scale;

        text_render_info() = default;
        ~text_render_info() = default;
    };

    // Hmm.. sprintf doesn't work with unicode I guess.
    // I'll figure out what to do later.
    static void render(cfont_t *fnt, const text_render_info *pInfo, const char *fmt, ...);
    static void init();

    static VkDescriptorPool desc_pool;
    static VkDescriptorSetLayout desc_Layout;
    static VkDescriptorSet desc_set;
    static VkImage error_image;
    static VkImageView error_image_view;
    static VkSampler error_image_sampler;

    static void load_font(struct crenderer_t *rd, const CFontLoadInfo* pInfo, cfont_t **dst);

    static void begin_render(cfont_t *fnt);
    static void end_render(crenderer_t *rd, ccamera camera, cfont_t *fnt, mat4 model);

    struct CFGlyph
    {
        float x0,x1,y0,y1;
        float l,r,b,t;
        f32 advance;
    };

    enum BitmapChannels
    {
        CHANNELS_1 = 1,
        CHANNELS_3 = 3,
        CHANNELS_4 = 4,

        CHANNELS_SDF = CHANNELS_1,
        CHANNELS_MSDF = CHANNELS_3,
        CHANNELS_MTSDF = CHANNELS_4,
    };

    struct text_drawcall_t;

    /* Internal CFont struct. Do not modify yourselves! */
    struct cfont_t
    {
        catlas_t atlas;
        f32 pixel_range;

        f32 line_height;
        f32 space_width;

        VkPipeline pipeline;
        VkPipelineLayout pipeline_layout;
        VkShaderModule vshader;
        VkShaderModule fshader;

        u32 font_index;
        VkImage texture;
        VkImageView texture_view;
        VkDeviceMemory texture_memory;
        VkSampler sampler;

        u32 allocated_size;
        VkBuffer buffer;
        VkDeviceMemory buffer_memory;
        void *mapped;
        void resize_buffer(u32 new_buffer_size, u32 index_buffer_offset);

        int index_buffer_offset;
        int index_count;
        bool to_render;

        u32 chars_drawn;
        cg_vector_t * /* text_drawcall_t */  drawcalls;
        cg_hashmap_t * /* unicode, CFGlyph ctext_hasher<unicode>> */ glyph_map;

        friend void ctext::load_font(struct crenderer_t *rd, const CFontLoadInfo* pInfo, cfont_t **dst);
    };

    struct CFontLoadInfo
    {
        const char *fontPath = nullptr;
        f32 scale = 24.0f;
        ccharset chset = CHARSET_ASCII;
        BitmapChannels channel_count = CHANNELS_SDF;
    };
};

#endif
