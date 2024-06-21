#ifndef __C_FONT_HPP__
#define __C_FONT_HPP__

#include "stdafx.hpp"
#include "external/msdf-atlas-gen/msdf-atlas-gen/msdf-atlas-gen.h"

namespace cf
{
	constexpr const char *CFontRegistryPath = "./CFont/Registry.txt";

	/* Internal CFont struct. Do not modify yourselves! */
	struct _CFont
	{
		u32 atlasWidth, atlasHeight;
		u8 *atlasData;

		// Font registry data
		u32 fontIndex;
		VkImage texture;
		VkImageView textureView;
		VkDeviceMemory textureMemory;
		VkSampler sampler;
	};

	typedef _CFont *CFont;

	struct CFontLoadInfo
	{
		const char *fontPath;
	};

	enum CFResult
	{
		CF_RESULT_SUCCESS = 0,
		CF_INVALID_PATH,
		CF_INVALID_DST,
		CF_INVALID_INFO,
	};

	static void CFInit(void);
	extern void CFLoad(const CFontLoadInfo* pInfo, CFont* dst);
}

#endif