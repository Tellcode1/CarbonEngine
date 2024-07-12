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

	// static void CFInit(void);
}

constexpr u32 MaxFontCount = 8;

struct ctext
{
	struct _CFont; typedef _CFont *CFont;
	struct CFontLoadInfo;

	static void Render(ctext::CFont font, VkCommandBuffer cmd, std::u32string text, f32 x, f32 y, f32 scale);
    static void Init();
    static void CreatePipeline();

    static VkPipeline pipeline;
    static VkPipelineLayout pipelineLayout;
    static VkShaderModule vertex;
    static VkShaderModule fragment;
    static VkDescriptorPool descPool;
    static VkDescriptorSetLayout descLayout;
    static VkDescriptorSet descSet;
    static VkImage dummyImage;
    static VkImageView dummyImageView;
    static VkSampler dummyImageSampler;

	constexpr static const char *CFontRegistryPath = "./CFont/Registry.txt";

	static void CFLoad(const CFontLoadInfo* pInfo, CFont* dst);

	struct CFGlyph
	{
		f32 x0, y0, x1, y1;
		f32 s0, t0, s1, t1;
		f32 advance;
		u32 codepoint;
	};

	/* Internal CFont struct. Do not modify yourselves! */
	struct _CFont
	{
		u32 atlasWidth, atlasHeight;
		u8 *atlasData;

		u32 fontIndex;
		VkImage texture;
		VkImageView textureView;
		VkDeviceMemory textureMemory;
		VkSampler sampler;

		friend void ctext::CFLoad(const CFontLoadInfo* pInfo, CFont* dst);

		private:
		std::unordered_map<u32, CFGlyph> m_glyphGeometry;
	};

	struct CFontLoadInfo
	{
		const char *fontPath;
	};
};

#endif