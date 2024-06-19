#ifndef __C_FONT_HPP__
#define __C_FONT_HPP__

#include "stdafx.hpp"


namespace cf
{
	constexpr const char *CFontRegistryPath = "./CFont/Registry.txt";

	/* Internal CFont struct. Do not modify yourselves! */
	struct _CFont
	{
		// Fill
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
	static void CFLoad(const CFontLoadInfo* pInfo, CFont* dst);
}

#endif