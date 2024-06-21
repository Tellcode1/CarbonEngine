#include "CFont.hpp"

#define STB_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

constexpr u32 channels = 1;

void saveImagePNG(const msdfgen::BitmapConstRef<f32, channels>& bitmap, u8* dstData, const char* filename) {
    int width = bitmap.width;
    int height = bitmap.height;

    // for (int y = 0; y < height; y++) {
    //     for (int x = 0; x < width; x++) {
	// 		// stb or something has reverse pixel data idk
    //         const glm::vec<channels, f32> pixel = *reinterpret_cast<const vec3*>(bitmap(x, height - 1 - y));
	// 		for(u32 channel = 0; channel < channels; channel++) {
	// 			uchar _pixel = static_cast<uchar>(std::max(0.0f, std::min(255.0f, (pixel[channel] + 0.5f) * 255.0f)));
	// 			dstData[channels * (y * width + x) + channel] = _pixel;
	// 		}
    //     }
    // }

	for (i32 y = 0; y < height; y++) {
		for (i32 x = 0; x < width; x++) {
			dstData[channels * (y * width + x)] = static_cast<uchar>(*bitmap(x, height - 1 - y));
		}
	}

    stbi_write_png(filename, width, height, channels, dstData, width * channels);
}

static void amongus(const cf::CFontLoadInfo *pInfo, cf::CFont *dst)
{
	bool success = false;

	if (msdfgen::FreetypeHandle *ft = msdfgen::initializeFreetype())
	{
		if (msdfgen::FontHandle *font = msdfgen::loadFont(ft, pInfo->fontPath)) 
		{
			std::vector<msdf_atlas::GlyphGeometry> glyphs;
			msdf_atlas::FontGeometry fontGeometry(&glyphs);
			fontGeometry.loadCharset(font, 1.0, msdf_atlas::Charset::ASCII);

			constexpr double maxCornerAngle = 3.0;
			for (msdf_atlas::GlyphGeometry& glyph : glyphs) {
				glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);
			}

			msdf_atlas::TightAtlasPacker packer;
			packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
			packer.setMinimumScale(24.0);
			packer.setPixelRange(2.0);
			packer.setSpacing(1);
			packer.setMiterLimit(1.0);
			packer.pack(glyphs.data(), glyphs.size());

			int width = 0, height = 0;
			packer.getDimensions(width, height);

			// change sdfgenerator when chaning num of channels
			// Won't put in top of file because it won't compile without it :>
			msdf_atlas::ImmediateAtlasGenerator<
				f32,
				channels,
				&msdf_atlas::sdfGenerator,
				msdf_atlas::BitmapAtlasStorage<f32, channels>
			> generator(width, height);

			msdf_atlas::GeneratorAttributes attributes;
			generator.setAttributes(attributes);
			generator.setThreadCount(std::thread::hardware_concurrency() + 2);
			// generator.setThreadCount(1);
			generator.generate(glyphs.data(), glyphs.size());

			*dst = new cf::_CFont();
			u8* pixelBuffer = new u8[ width * height * channels ];

			const msdfgen::BitmapConstRef<f32, channels> ref = generator.atlasStorage();
			saveImagePNG(ref, pixelBuffer, "amongus.png");

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
			memset(*dst, 0, sizeof(cf::_CFont));
			dstRef.atlasWidth = width;
			dstRef.atlasHeight = height;
			dstRef.atlasData = pixelBuffer;
		}
	}
}

void cf::CFLoad(const CFontLoadInfo* pInfo, CFont* dst) {
	TIME_FUNCTION(amongus(pInfo, dst));
}