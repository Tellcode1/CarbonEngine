#ifndef __C_TEXT_RENDERER_HPP__
#define __C_TEXT_RENDERER_HPP__

#include "stdafx.hpp"
#include "CFont.hpp"

struct ctext
{
	static void Render(cf::CFont font, const char *text, float x, float y, float scale);
};

#endif