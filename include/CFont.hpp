#ifndef __C_FONT_HPP__
#define __C_FONT_HPP__

#include "stdafx.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

#define MSDFGEN_PUBLIC
#include "external/msdf-atlas-gen/msdf-atlas-gen/msdf-atlas-gen.h"

#pragma GCC diagnostic pop

static std::u32string to_u32string(const std::string& str) {
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
	std::cout << "Input " << str << '\n' << "Output ";
	for(const auto &c : convert.from_bytes(str)) {
		std::cout << std::hex << "0x" << c << " ";
	}
	std::cout << std::endl;
    return convert.from_bytes(str);
}

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
			LOG_ERROR("No Glyph for codepoint %u\n", codepoint);

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