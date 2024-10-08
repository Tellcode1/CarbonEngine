#ifndef __C_TEXT_HPP__
#define __C_TEXT_HPP__

#include "../../../include/camera.hpp"

#include "../include/math/vec3.hpp"
#include "../include/math/vec2.hpp"
#include "../include/math/mat.hpp"

#include "stdafx.h"
#include "vkstdafx.h"

#include "containers/cvector.h"
#include "containers/cstring.h"
#include "containers/chashmap.h"

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
        cm::vec3 position;
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
    static cm::mat4 model_matrix;

    /*
    *	I'll just detail what I want to do with the registry.
    *	The registry is supposed to contain all valid font 'caches' (atlas + vertex && texture positions compressed).
    *	Need a way to check if the font's been changed since the last time the 'regchecker' ran. You can probably guess what 'regchecker' is.
    *	This will be a very volatile component of the engine so I ought to make it as secure as possible but we'll see.
    *	This block of text should not be removed until the registry has achieved production status.
    *	The registry should also contain a count of the fonts loaded since the last time the program was ran, pass it through
    *	as a spec constant. This is because the shader will need to be recompiled everytime the max font count is to increase.
    *	+ Descriptor count will be equal to that and we really do not want multiple descriptor pool recreations or multiple descriptor pool allocations
    *	This won't be very heavy though and instead can probably be replaced by a simple pass from the engine to collect all fonts, cache them
    *	into the binary and instead just use the accumulated count.
    */
    constexpr static const char *CFontRegistryPath = "./CFont/Registry.txt";

    static void load_font(struct crenderer_t *rd, const CFontLoadInfo* pInfo, cfont_t **dst);

    static void begin_render(cfont_t *fnt);
    static void end_render(crenderer_t *rd, ccamera camera, cfont_t *fnt, cm::mat4 model);

    static inline void set_model_matrix(const cm::mat4 &new_matrix) {
        model_matrix = new_matrix;
    }

    struct CFGlyph
    {
        f32 positions[4];
        f32 uv[4];
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
        u32 atlas_width, atlas_height;
        u8 *atlas_data;
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
        cvector_t * /* text_drawcall_t */  drawcalls;
        chashmap_t * /* unicode, CFGlyph ctext_hasher<unicode>> */ glyph_map;

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
