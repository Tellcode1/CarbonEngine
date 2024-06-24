#ifndef __C_TEXT_RENDERER_HPP__
#define __C_TEXT_RENDERER_HPP__

#include "stdafx.hpp"
#include "CFont.hpp"

// struct CFGlyph
// {
//     msdf_atlas::GlyphGeometry geometry;
//     vec4 vertices[4];
//     u16 indices[6];
// };

struct ctext
{
	static void Render(cf::CFont font, VkCommandBuffer cmd, std::u32string text, f32 x, f32 y, f32 scale);
    static void Init();

    static VkPipeline pipeline;
    static VkPipelineLayout pipelineLayout;
    static VkShaderModule vertex;
    static VkShaderModule fragment;
};

#endif