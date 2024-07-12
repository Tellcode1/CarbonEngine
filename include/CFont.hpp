#ifndef __C_FONT_HPP__
#define __C_FONT_HPP__

#include "stdafx.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#define MSDFGEN_PUBLIC // ?
#include "external/msdf-atlas-gen/msdf-atlas-gen/msdf-atlas-gen.h"

#pragma GCC diagnostic pop

namespace cf
{
	constexpr const char *CFontRegistryPath = "./CFont/Registry.txt";

	struct _CFont; typedef _CFont *CFont;
	struct CFontLoadInfo;

	extern void CFLoad(const CFontLoadInfo* pInfo, CFont* dst);

	// Cannot be incomplete because the stupd unordered_map needs it
	struct CFGlyph
	{
		inline CFGlyph() : initialized(false) {};
		inline CFGlyph(msdf_atlas::GlyphGeometry g) : geometry(g), initialized(true) {}

		inline u32 getCodepoint() const { return geometry.getCodepoint(); }

		msdf_atlas::GlyphGeometry geometry;
		bool initialized = false;
		private:
	};

	/* Internal CFont struct. Do not modify yourselves! */
	struct _CFont
	{
		u32 atlasWidth, atlasHeight;
		u8 *atlasData;

		inline CFGlyph GetGlyph(u32 codepoint) {
			if (m_glyphGeometry.find(codepoint) != m_glyphGeometry.end()) return m_glyphGeometry[codepoint];
			LOG_ERROR("No Glyph for codepoint %u", codepoint);

			return CFGlyph();
		}

		u32 fontIndex;
		VkImage texture;
		VkImageView textureView;
		VkDeviceMemory textureMemory;
		VkSampler sampler;

		friend void cf::CFLoad(const CFontLoadInfo* pInfo, CFont* dst);

		private:
		std::unordered_map<u32, CFGlyph> m_glyphGeometry;
	};

	struct CFontLoadInfo
	{
		const char *fontPath;
	};

	// static void CFInit(void);
}

#endif