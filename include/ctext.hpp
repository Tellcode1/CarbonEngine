#ifndef __C_FONT_HPP__
#define __C_FONT_HPP__

struct ctext;

#include "stdafx.hpp"
#include "vkcb/vkcbstdafx.hpp"

#define MSDFGEN_PUBLIC // ?
#include "external/msdf-atlas-gen/msdf-atlas-gen/msdf-atlas-gen.h"

struct ctext
{
	constexpr static u32 MAX_FONT_COUNT = 8;

	struct CFont_T;
	typedef CFont_T *CFont;
	struct CFontLoadInfo;

	static void Render(ctext::CFont font, std::u32string text, f32 x, f32 y, f32 scale);
	static void render_line(const ctext::CFont font, const std::u32string &str, f32 *xbegin, f32 y);
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
	static bool font_is_valid(const ctext::CFont fnt);

	static void begin_render(ctext::CFont fnt);
	static void end_render(ctext::CFont fnt);

	struct CFGlyph
	{
		f32 x0, y0, x1, y1;
		f32 l, b, r, t;
		f32 advance;
		u32 codepoint;

		CARBON_FORCE_INLINE f32 get_width() const {
			return x1 - x0;
		}

		CARBON_FORCE_INLINE f32 get_height() const {
			return y0 - y1;
		}
	};

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

	enum BitmapChannels
	{
		CHANNELS_1 = 0,
		CHANNELS_3 = 1,
		CHANNELS_4 = 2,

		CHANNELS_SDF = CHANNELS_1,
		CHANNELS_MSDF = CHANNELS_3,
		CHANNELS_MTSDF = CHANNELS_4,
	};

	struct text_drawcall_t {
		u32 vertex_count;
		u32 index_count;
		vec4 *vertices;
    	u32 *indices;

		CARBON_FORCE_INLINE u32 get_index_data_offset() const {
			return vertex_count * sizeof(vec4);
		}
	};
	static text_drawcall_t *create_text_drawcall(u32 num_verts, u32 num_inds);

	/* Internal CFont struct. Do not modify yourselves! */
	struct CFont_T
	{
		u32 atlasWidth, atlasHeight;
		u8 *atlasData;

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

		VkBuffer buffer;
		VkDeviceMemory buffer_memory;

		u32 chars_drawn;
		cvector<text_drawcall_t *> drawcalls;

		friend void ctext::load_font(const CFontLoadInfo* pInfo, CFont* dst);
		friend bool ctext::font_is_valid(const ctext::CFont fnt);

		const CFGlyph& get_glyph(u32 codepoint) {
			if (m_glyph_geometry.find(codepoint) != m_glyph_geometry.end())
				return m_glyph_geometry[codepoint];

			LOG_ERROR("Request for codepoint %u (nonexistent)", codepoint);
			return m_glyph_geometry[U' '];
		}

		private:
		std::unordered_map<u32, CFGlyph> m_glyph_geometry;
	};

	struct CFontLoadInfo
	{
		const char *fontPath = nullptr;
		f32 scale = 24.0f;
		msdf_atlas::Charset chset = msdf_atlas::Charset::ASCII;
		BitmapChannels channel_count = CHANNELS_SDF;
	};
};

#endif