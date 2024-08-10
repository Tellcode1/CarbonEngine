#ifndef __C_FONT_HPP__
#define __C_FONT_HPP__

struct ctext;

static_assert(__STDC_UTF_32__ == 1);

#include "stdafx.hpp"
#include "vkcb/vkcbstdafx.hpp"

#define MSDFGEN_PUBLIC // ?
#include "external/msdf-atlas-gen/msdf-atlas-gen/msdf-atlas-gen.h"

#include "containers/cstring.hpp"
#include "containers/chashmap.hpp"

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

struct ctext
{
    constexpr static u32 MAX_FONT_COUNT = 8;

    struct CFont_T;
    typedef CFont_T *CFont;
    struct CFontLoadInfo;
    struct text_drawcall_t;

    struct text_render_info {
        HoriAlignment horizontal;
        VertAlignment vertical;
        f32 x;
        f32 y;
        f32 scale;

        text_render_info() = default;
        ~text_render_info() = default;
    };

    // Hmm.. sprintf doesn't work with unicode I guess.
    // I'll figure out what to do later.
    static void render(ctext::CFont fnt, const text_render_info *pInfo, const unicode *fmt, ...);
    static void initialize();

    static VkDescriptorPool desc_pool;
    static VkDescriptorSetLayout desc_Layout;
    static VkDescriptorSet desc_set;
    static VkImage error_image;
    static VkImageView error_image_view;
    static VkSampler error_image_sampler;

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

    static void load_font(const CFontLoadInfo* pInfo, CFont* dst);

    static void begin_render(ctext::CFont fnt);
    static void end_render(ctext::CFont fnt, mat4 model);

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

    struct text_drawcall_t {
        u32 vertex_count;
        u32 index_count;
        u32 index_offset;
        vec4 *vertices;
        u32 *indices;
    };

    /* Internal CFont struct. Do not modify yourselves! */
    struct CFont_T
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

        template <typename T>
        struct ctext_hasher {
            constexpr static inline u32 hash(const T& key) {
                return key;
            }
        };

        u32 chars_drawn;
        cvector<text_drawcall_t> drawcalls;
        chashmap<unicode, CFGlyph, ctext_hasher<unicode>> glyph_map;

        friend void ctext::load_font(const CFontLoadInfo* pInfo, CFont* dst);
    };

    struct CFontLoadInfo
    {
        const char *fontPath = nullptr;
        f32 scale = 24.0f;
        msdf_atlas::Charset chset = msdf_atlas::Charset::ASCII;
        BitmapChannels channel_count = CHANNELS_SDF;
    };

    // reddit saves the day
    static cvector<int> unicode_to_utf8(unicode charcode)
    {
        cvector<int> d;
        if (charcode < 128)
        {
            d.push_back(charcode);
        }
        else
        {
            int first_bits = 6; 
            const int other_bits = 6;
            int first_val = 0xC0;
            int t = 0;
            while (charcode >= (1 << first_bits))
            {
                {
                    t = 128 | (charcode & ((1 << other_bits)-1));
                    charcode >>= other_bits;
                    first_val |= 1 << (first_bits);
                    first_bits--;
                }
                d.push_back(t);
            }
            t = first_val | charcode;
            d.push_back(t);
            std::reverse(d.begin(), d.end());
        }
        return d;
    };

    static cvector<unicode> utf32_to_utf8_str(const char *str) {
        cvector<unicode> ret;
        cvector<char> unicode_points;
        unicode_points.push_set(str, strlen(str));
        for (auto node : unicode_points) {
            const cvector<int> utf8 = unicode_to_utf8(node);
            for (const unicode &uc : utf8) {
                ret.push_back(uc);
            }
        }
        return ret;
    }

    static unicode utf8_to_unicode(const cvector<unicode> &coded)
    {
        int i = 0;
        unicode charcode = 0;
        int t = coded[0]; i++;
        if (t < 128)
        {
            return t;
        }
        int high_bit_mask = (1 << 6) -1;
        int high_bit_shift = 0;
        int total_bits = 0;
        constexpr int other_bits = 6;
        while((t & 0xC0) == 0xC0)
        {
            t <<= 1;
            t &= 0xff;
            total_bits += 6;
            high_bit_mask >>= 1; 
            high_bit_shift++;
            charcode <<= other_bits;
            charcode |= coded[i] & ((1 << other_bits)-1);
            i++;
        } 
        charcode |= ((t >> high_bit_shift) & high_bit_mask) << total_bits;
        return charcode;
    }
};

#endif