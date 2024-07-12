#include "CFont.hpp"

#define STB_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

/* I have no idea what any of this is */

constexpr u32 channels = 3;

void SaveImage(const msdfgen::BitmapConstRef<f32, channels> &bitmap, u8* dstData, const char* filename) {
    const u32 width = bitmap.width;
    const u32 height = bitmap.height;

	stbi_flip_vertically_on_write(true);

    for (u32 y = 0; y < height; y++) {
        for (u32 x = 0; x < width; x++) {
            const f32 *pixel = bitmap(x, y);
			for(u32 channel = 0; channel < channels; channel++) {
				
				uchar _pixel = static_cast<uchar>(std::max(0.0f, std::min(255.0f, (pixel[channel] + 0.5f) * 255.0f)));
				dstData[channels * (y * width + x) + channel] = _pixel;
			}
        }
    }

    stbi_write_png(filename, width, height, channels, dstData, width * channels);
}

void cf::CFLoad(const CFontLoadInfo* pInfo, CFont* dst)
{
	if (!pInfo || !dst)
	{
		LOG_ERROR("pInfo or dst is nullptr!");
		return;
	}

	if (msdfgen::FreetypeHandle *ft = msdfgen::initializeFreetype())
	{
		if (msdfgen::FontHandle *font = msdfgen::loadFont(ft, pInfo->fontPath)) 
		{
			*dst = new cf::_CFont();

			std::vector<msdf_atlas::GlyphGeometry> glyphs;
			msdf_atlas::FontGeometry fontGeometry(&glyphs);
			fontGeometry.loadCharset(font, 1.0, msdf_atlas::Charset::ASCII);

			constexpr double maxCornerAngle = 3.0;
			for (msdf_atlas::GlyphGeometry& glyph : glyphs) {
				glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);
			}

			msdf_atlas::TightAtlasPacker packer;
			packer.setPixelRange(2.0);
			packer.pack(glyphs.data(), glyphs.size());

			int width = 0, height = 0;
			packer.getDimensions(width, height);

			u8* pixelBuffer = new u8[ width * height * channels ];

			// change sdfgenerator when changing num of channels
			// Won't put in top of file because it won't compile without it :>
			msdf_atlas::ImmediateAtlasGenerator<
				f32,
				channels,
				&msdf_atlas::msdfGenerator,
				msdf_atlas::BitmapAtlasStorage<f32, channels>
			> generator(width, height);

			msdf_atlas::GeneratorAttributes attributes;
			generator.setAttributes(attributes);
			generator.setThreadCount(std::thread::hardware_concurrency());
			generator.generate(glyphs.data(), glyphs.size());


			msdfgen::savePng(generator.atlasStorage(), "amongus.png");

			for (msdf_atlas::GlyphGeometry& glyph : glyphs)
				(*dst)->m_glyphGeometry[glyph.getCodepoint()] = CFGlyph(glyph);

			std::ofstream among("amongus.md");
			for (const msdf_atlas::GlyphGeometry& glyph : glyphs) {
                i32 x0, y0, x1, y1;
                glyph.getBoxRect(x0, y0, x1, y1);

                f32 u0, v0, u1, v1;
                u0 = static_cast<f32>(x0) / static_cast<f32>(width);
                v0 = static_cast<f32>(y0) / static_cast<f32>(height);
                u1 = static_cast<f32>(x1) / static_cast<f32>(width);
                v1 = static_cast<f32>(y1) / static_cast<f32>(height);
				
                among <<
					glyph.getCodepoint() << " : " <<
					u0 << ", " <<
					v0 << ", " <<
					u1 << ", " <<
					v1 << std::endl;
            }
			among.close();

			cf::_CFont &dstRef = **dst; // Dereference CFont* -> _CFont* -> _CFont
			// memset(*dst, 0, sizeof(cf::_CFont));
			dstRef.atlasWidth = width;
			dstRef.atlasHeight = height;
			dstRef.atlasData = pixelBuffer;
		} else {
			LOG_ERROR("Failed to open font at path \"%s\". Is the path correct?", pInfo->fontPath);
		}
	}
}