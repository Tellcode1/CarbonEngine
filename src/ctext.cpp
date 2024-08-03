#include "ctext.hpp"
#include "epichelperlib.hpp"
#include "Bootstrap.hpp"

#include "stb/stb_image_write.h"
#include "stb/stb_image.h"
#include "cinput.hpp"

VkDescriptorPool ctext::desc_pool = VK_NULL_HANDLE;
VkDescriptorSetLayout ctext::desc_Layout = VK_NULL_HANDLE;
VkDescriptorSet ctext::desc_set = VK_NULL_HANDLE;
VkImage ctext::error_image = VK_NULL_HANDLE;
VkImageView ctext::error_image_view = VK_NULL_HANDLE;
VkSampler ctext::error_image_sampler = VK_NULL_HANDLE;

/* I have no idea what any of this is */

typedef char32_t unicode;

template <typename T, u32 channels>
u8 *i_hate_my_life(u32 width, u32 height, T &generator, std::vector<msdf_atlas::GlyphGeometry> &glyphs) {
    generator.setThreadCount(4);
    generator.generate(glyphs.data(), glyphs.size());
    msdfgen::BitmapConstRef<f32, channels> bitmap = generator.atlasStorage();

    u8* pixelBuffer = reinterpret_cast<u8*>(malloc(width * height * channels));
    memset(pixelBuffer, 0, width * height * channels);

    // what the f(*(#$)*@#) is this
    for (u32 y = 0; y < height; y++)
        for (u32 x = 0; x < width; x++)
            for (u32 c = 0; c < channels; c++)
                pixelBuffer[(width * (height - y - 1) + x) * channels + c] = msdfgen::pixelFloatToByte(bitmap.pixels[(width * y + x) * channels + c]);

    msdfgen::savePng(bitmap, "debug.png");

    return pixelBuffer;
}

void ctext::load_font(const CFontLoadInfo *__restrict pInfo, CFont* dst)
{
	if (!pInfo || !dst)
	{
		LOG_ERROR("pInfo or dst is nullptr!");
		return;
	}

    cassert_and_ret(pInfo->scale > 0.0f);
    cassert_and_ret(pInfo->chset.size() > 0);

    msdfgen::FreetypeHandle *ft = msdfgen::initializeFreetype();
    if (!ft) {
        LOG_ERROR("msdfgen<Freetype> failed to initialize");
        return;
    }

    msdfgen::FontHandle *fnt = msdfgen::loadFont(ft, pInfo->fontPath);
    if (!fnt) {
        LOG_ERROR("Failed to load fnt. Is path (\"%s\") valid?", pInfo->fontPath);
        return;
    }

    *dst = new CFont_T();

    std::vector<msdf_atlas::GlyphGeometry> glyphs;
    msdf_atlas::FontGeometry fontGeometry(&glyphs);
    fontGeometry.loadCharset(fnt, 1.0, pInfo->chset);

    constexpr double maxCornerAngle = 3.0;
    for (msdf_atlas::GlyphGeometry& glyph : glyphs)
        glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);

    const f32 scale = pInfo->scale;
    msdf_atlas::TightAtlasPacker packer;
    packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::POWER_OF_TWO_SQUARE);
    packer.setMinimumScale(scale);
    packer.setPixelRange(scale);
    packer.setMiterLimit(scale / 12.0f);
    packer.setSpacing(2);
    packer.pack(glyphs.data(), glyphs.size());

    i32 width_ = 0, height_ = 0;
    packer.getDimensions(width_, height_);
    u32 width = width_, height = height_; // I hate people who use integers for no reason like bro an unsigned is obviously better here

    const VkFormat bitmap_fmt =
        (pInfo->channel_count == 4) ? VK_FORMAT_R8G8B8A8_UNORM :
        (pInfo->channel_count == 3) ? VK_FORMAT_R8G8B8_UNORM :
        (pInfo->channel_count == 1) ? VK_FORMAT_R8_UNORM :
        VK_FORMAT_UNDEFINED;

    using sdf_gen =  msdf_atlas::ImmediateAtlasGenerator<
        f32,
        1,
        msdf_atlas::sdfGenerator,
        msdf_atlas::BitmapAtlasStorage<f32, 1>
    >;

    using msdf_gen =  msdf_atlas::ImmediateAtlasGenerator<
        f32,
        3,
        msdf_atlas::msdfGenerator,
        msdf_atlas::BitmapAtlasStorage<f32, 3>
    >;

    using mtsdf_gen =  msdf_atlas::ImmediateAtlasGenerator<
        f32,
        4,
        msdf_atlas::mtsdfGenerator,
        msdf_atlas::BitmapAtlasStorage<f32, 4>
    >;

    u8 *pixelBuffer;
    if (pInfo->channel_count == CHANNELS_1) {
        sdf_gen gen(width, height);
        pixelBuffer = i_hate_my_life<sdf_gen, 1>(width, height, gen, glyphs);
    }
    else if (pInfo->channel_count == CHANNELS_3) {
        msdf_gen gen(width, height);
        pixelBuffer = i_hate_my_life<msdf_gen, 3>(width, height, gen, glyphs);
    }
    else if (pInfo->channel_count == CHANNELS_4) {
        mtsdf_gen gen(width, height);
        pixelBuffer = i_hate_my_life<mtsdf_gen, 4>(width, height, gen, glyphs);
    }
    else {
        __builtin_unreachable();
        LOG_AND_ABORT("Invalid channel count");
        pixelBuffer = nullptr; // Satisfy the compiler gods
    }

    for (const msdf_atlas::GlyphGeometry& glyph : glyphs) {
        if (glyph.isWhitespace()) {
            (*dst)->space_width = glyph.getAdvance();
            continue;
        }

        f64 x0_, y0_, x1_, y1_;
        glyph.getQuadPlaneBounds(x0_, y0_, x1_, y1_);

        f64 l_, b_, r_, t_;
        glyph.getQuadAtlasBounds(l_, b_, r_, t_);

        f32 l = (f32(l_)) / f32(width);
        f32 r = (f32(r_)) / f32(height);
        f32 b  =( 1.0f - (f32)b_) / f32(width);
        f32 t  =( 1.0f - (f32)t_) / f32(height);
        
        f32 x0 = f32(x0_), x1 = f32(x1_);
        f32 y0 = 1.0f - (f32)y0_;
        f32 y1 = 1.0f - (f32)y1_;

        // glyph.l, glyph.t
        // glyph.r, glyph.t
        // glyph.r, glyph.b
        // glyph.l, glyph.b
        CFGlyph mogus;

        alignas(16) f32 positions[4] = {
            x0, x1,
            y0, y1,
        };

        alignas(16) f32 uv_positions[4] = {
            l, r,
            b, t
        };

        memcpy(mogus.positions, positions, sizeof(float) * 4);
        memcpy(mogus.uv, uv_positions, sizeof(float) * 4);

        // mogus.positions = _mm_load_ps(positions);
        // mogus.uv = _mm_load_ps(uv_positions);

        msdfgen::FontMetrics metrics;
        msdfgen::getFontMetrics(metrics, fnt);

        (*dst)->line_height = f32(metrics.lineHeight) / f32(metrics.emSize);

        mogus.advance = static_cast<f32>(glyph.getAdvance());
        (*dst)->glyph_map.insert({glyph.getCodepoint(), mogus});
    }

    (*dst)->atlas_width = width;
    (*dst)->atlas_height = height;
    (*dst)->atlas_data = pixelBuffer;

    help::Images::killme(pixelBuffer, width, height, bitmap_fmt, pInfo->channel_count, &(*dst)->texture, &(*dst)->texture_memory);

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if (vkCreateSampler(device, &sampler_info, nullptr, &(*dst)->sampler) != VK_SUCCESS)
        LOG_ERROR("Failed to create sampler");

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = bitmap_fmt;
    view_info.image = (*dst)->texture;
    view_info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &view_info, nullptr, &(*dst)->texture_view) != VK_SUCCESS)
        LOG_ERROR("Failed to create image view");

    VkDescriptorImageInfo error_image_info{};
    error_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    error_image_info.sampler = (*dst)->sampler;
    error_image_info.imageView = (*dst)->texture_view;

    for (u32 i = 0; i < MAX_FONT_COUNT; i++)
    {
        VkWriteDescriptorSet writeSet{};
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = desc_set;
        writeSet.dstBinding = 0;
        writeSet.descriptorCount = 1;
        writeSet.pImageInfo = &error_image_info;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.dstArrayElement = i;
        vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
    }

    msdfgen::deinitializeFreetype(ft);

    const char *fshader_path;

    if (pInfo->channel_count == CHANNELS_SDF)
        fshader_path = "./Shaders/ctext/sdf.frag.spv";
    else if (pInfo->channel_count == CHANNELS_MSDF)
        fshader_path = "./Shaders/ctext/msdf.frag.spv";
    else if (pInfo->channel_count == CHANNELS_MTSDF)
        fshader_path = "./Shaders/ctext/mtsdf.frag.spv";

    cvector<u8> vshader_binary;
    cvector<u8> fshader_binary;
    cvector<u8> cshader_binary;

    help::Files::LoadBinary("./Shaders/vert.text.spv", &vshader_binary);
    help::Files::LoadBinary("./Shaders/ctext/test.comp.spv", &cshader_binary);
    help::Files::LoadBinary(fshader_path, &fshader_binary);

    std::cout << "Loaded font shader from " << get_filename(fshader_path) << '\n';

    VkShaderModuleCreateInfo vertexShaderModuleInfo{};
    vertexShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderModuleInfo.codeSize = vshader_binary.size();
    vertexShaderModuleInfo.pCode = reinterpret_cast<const u32*>(vshader_binary.data());
    if (vkCreateShaderModule(device, &vertexShaderModuleInfo, nullptr, &(*dst)->vshader) != VK_SUCCESS) {
        LOG_ERROR("ctext failed to create vertex shader module");
    }

    VkShaderModuleCreateInfo fragmentShaderModuleInfo{};
    fragmentShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentShaderModuleInfo.codeSize = fshader_binary.size();
    fragmentShaderModuleInfo.pCode = reinterpret_cast<const u32*>(fshader_binary.data());
    if (vkCreateShaderModule(device, &fragmentShaderModuleInfo, nullptr, &(*dst)->fshader) != VK_SUCCESS) {
        LOG_ERROR("ctext failed to create fragment shader module");
    }

    const std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
        // location; binding; format; offset;
        { 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 },
    };

    const std::vector<VkVertexInputBindingDescription> bindingDescriptions = {
        // binding; stride; inputRate
        { 0, sizeof(vec4), VK_VERTEX_INPUT_RATE_VERTEX }
    };

    const std::vector<VkPushConstantRange> pushConstants = { 
        // stageFlags, offset, size
        { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4) },
    };

    VkPipelineShaderStageCreateInfo vertexShaderInfo{};
    vertexShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderInfo.module = (*dst)->vshader;
    vertexShaderInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderInfo{};
    fragmentShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderInfo.module = (*dst)->fshader;
    fragmentShaderInfo.pName = "main";

    const std::vector<VkPipelineShaderStageCreateInfo> shaders = { vertexShaderInfo, fragmentShaderInfo };
    const std::vector<VkDescriptorSetLayout> layouts = { desc_Layout };

    const pro::PipelineBlendState blend = pro::PipelineBlendState(pro::BlendPreset::PRO_BLEND_PRESET_ALPHA);

    pro::PipelineCreateInfo pc{};
    pc.format = vctx::SwapChainImageFormat;
    pc.subpass = 0;
    pc.renderPass = vctx::GlobalRenderPass;
    pc.pipelineLayout = (*dst)->pipeline_layout;
    pc.pAttributeDescriptions = &attributeDescriptions;
    pc.pBindingDescriptions = &bindingDescriptions;
    pc.pPushConstants = &pushConstants;
    pc.pDescriptorLayouts = &layouts;
    pc.pShaderCreateInfos = &shaders;
    pc.extent.width = vctx::RenderExtent.width;
    pc.extent.height = vctx::RenderExtent.height;
    pc.pShaderCreateInfos = &shaders;
    pc.pBlendState = &blend;
    pro::CreatePipelineLayout(device, &pc, &(*dst)->pipeline_layout);
    pc.pipelineLayout = (*dst)->pipeline_layout;
    pro::CreateGraphicsPipeline(device, &pc, &(*dst)->pipeline, PIPELINE_CREATE_FLAGS_ENABLE_BLEND);

    help::Buffers::CreateBuffer(
        1, // alloc 1 byte of memory to store all program data (don't you dare allocate more)
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &(*dst)->buffer,
        &(*dst)->buffer_memory
    );
}

void ctext::end_render(ctext::CFont fnt, mat4 model)
{
    if (fnt->drawcalls.empty())
        return;

    u32 total_vertex_byte_size = 0;
    u32 total_index_count = 0;

    for (const text_drawcall_t &drawcall : fnt->drawcalls) {
        total_vertex_byte_size += drawcall.vertex_count * sizeof(vec4);
        total_index_count += drawcall.index_count;
    }

    const u32 total_index_byte_size = total_index_count * sizeof(u32);
    const u32 total_buffer_size = total_index_byte_size + total_vertex_byte_size;

    fnt->resize_buffer(total_buffer_size, total_vertex_byte_size);

    void *mapped;
    vkMapMemory(device, fnt->buffer_memory, 0, fnt->allocated_size, 0, &mapped);

    u32 vertex_copy_iterator = 0;
    u32 index_copy_iterator = 0;
    for (const text_drawcall_t &drawcall : fnt->drawcalls) {
        memcpy(static_cast<u8 *>(mapped) + vertex_copy_iterator, drawcall.vertices, drawcall.vertex_count * sizeof(vec4));
        memcpy(static_cast<u8 *>(mapped) + total_vertex_byte_size + index_copy_iterator, drawcall.indices, drawcall.index_count * sizeof(u32));
        vertex_copy_iterator += drawcall.vertex_count * sizeof(vec4);
        index_copy_iterator += drawcall.index_count * sizeof(u32);
    }

    vkUnmapMemory(device, fnt->buffer_memory);

    const VkCommandBuffer cmd = Renderer::GetDrawBuffer();
    constexpr VkDeviceSize offsets[] = { 0 };

    vkCmdPushConstants(
        cmd,
        fnt->pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0, sizeof(mat4),
        glm::value_ptr(mat4(1.0f))
    );

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fnt->pipeline_layout, 0, 1, &desc_set, 0, nullptr);
    vkCmdBindVertexBuffers(cmd, 0, 1, &fnt->buffer, offsets);
    vkCmdBindIndexBuffer(cmd, fnt->buffer, total_vertex_byte_size, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fnt->pipeline);
    vkCmdDrawIndexed(cmd, total_index_count, 1, 0, 0, 0);
}

static std::vector<std::u32string_view> split_string(const std::u32string_view& str) {
    std::vector<std::u32string_view> result;
    result.reserve(16);

    const unicode* str_data = str.data();
    size_t prev = 0;
    size_t pos = 0;

    while ((pos = str.find(U'\n', prev)) != std::u32string::npos) {
        result.emplace_back(str_data + prev, pos - prev);
        prev = pos + 1;
    }

    if (prev < str.size())
        result.emplace_back(str_data + prev, str.size() - prev);

    return result;
}

void ctext::begin_render(ctext::CFont fnt)
{
    fnt->chars_drawn = 0;
    for (text_drawcall_t &drawcall : fnt->drawcalls)
        free(drawcall.vertices);
    fnt->drawcalls.clear();
}

static void render_line(
    const ctext::CFont fnt,
    const std::u32string_view &str,
    const f32 scale,
    const ctext::text_drawcall_t* drawcall,
    f32 x,
    const f32 y,
    const u32 i,
    const u32 j,
    const ctext::CFGlyph **glyphs
) {

    const f32 scaled_space_width = fnt->space_width * scale;
    const f32 scaled_tab_width = scaled_space_width * 4.0f;

    vec4* vertex_output_data = drawcall->vertices + i * 4;
    u32* index_output_data = drawcall->indices + i * 6;
    u32 index_offset = fnt->chars_drawn * 4;

    for (int k = 0; k < (int)str.size(); k++) {
        const unicode c = str[k];
        switch (c) {
            case U' ':
                x += scaled_space_width;
                continue;
            case U'\t':
                x += scaled_tab_width;
                continue;
            default:
                const ctext::CFGlyph *glyph = glyphs[j + k];

                const f32 glyph_x0 = glyph->positions[0] * scale + x;
                const f32 glyph_x1 = glyph->positions[1] * scale + x;
                const f32 glyph_y0 = glyph->positions[2] * scale + y;
                const f32 glyph_y1 = glyph->positions[3] * scale + y;

                vertex_output_data[0] = { glyph_x0, glyph_y0, glyph->uv[0], glyph->uv[2] };
                vertex_output_data[1] = { glyph_x1, glyph_y0, glyph->uv[1], glyph->uv[2] };
                vertex_output_data[2] = { glyph_x1, glyph_y1, glyph->uv[1], glyph->uv[3] };
                vertex_output_data[3] = { glyph_x0, glyph_y1, glyph->uv[0], glyph->uv[3] };

                index_output_data[0] = index_offset;
                index_output_data[1] = index_offset + 1;
                index_output_data[2] = index_offset + 2;
                index_output_data[3] = index_offset + 2;
                index_output_data[4] = index_offset + 3;
                index_output_data[5] = index_offset;

                vertex_output_data += 4;
                index_output_data += 6;
                index_offset += 4;

                x += glyph->advance * scale;
                continue;
        }
    }
}

constexpr inline u32 get_effective_length(const std::u32string_view &str) {
    i32 len = 0;
    for (const unicode &c : str)
        switch (c) {
            case U' ':
            case U'\t':
            case U'\n':
                continue;
            default:
                len++;
                continue;
            break;
        }
    return len;
}

#include <atomic>

void gen_vertices(const ctext::CFont fnt, const HoriAlignment horizontal, const VertAlignment vertical, ctext::text_drawcall_t* drawcall, const std::u32string_view& str, f32 xbegin, f32 y, const f32 scale) {
    if (str.empty())
        return;

    const std::vector<std::u32string_view> splitted_strings = split_string(str);

    const f32 scaled_line_height = fnt->line_height * scale;
    const f32 scaled_space_width = fnt->space_width * scale;
    const f32 scaled_tab_width = (fnt->space_width * 4.0f) * scale;

    switch(vertical)
    {
        case CTEXT_VERT_ALIGN_CENTER:
            y = y - ((fnt->line_height * scale) * splitted_strings.size()) / 2.0f;
            break;

        case CTEXT_VERT_ALIGN_BOTTOM:
            y = y - (fnt->line_height * scale) * splitted_strings.size();
            break;

        case CTEXT_VERT_ALIGN_TOP:
            break;

        default:
            __builtin_unreachable();
            LOG_ERROR("Invalid vertical alignment. Specified (int)%u. (Implement?)\n", vertical);
            break;
        break;
    }

    i32 naive_length = 0;
    for (const auto &line : splitted_strings)
        naive_length += line.size();

    const ctext::CFGlyph **glyphs = (const ctext::CFGlyph **)malloc(naive_length * sizeof(void *));
    f32 widths[ splitted_strings.size() ];
    i32 line_lengths[ splitted_strings.size() ];

    #pragma omp parallel
    {
        #pragma omp for nowait
        for (int i = 0; i < (int)splitted_strings.size(); i++) {
            const std::u32string_view& line = splitted_strings[i];
            std::atomic_int accum_len = 0;
            std::atomic<float> accum_width = 0.0f;
            // Try to remove atomics somehow. They decrease the fps by like 30%!!!!
            #pragma omp parallel
            {
                #pragma omp for nowait
                for (int j = 0; j < (int)line.length(); j++)
                {
                    const unicode c = line[j];
                    switch (c) {
                        case U' ':
                            accum_width.store(accum_width.load() + scaled_space_width);
                            continue;
                        case U'\t':
                            accum_width.store(accum_width.load() + scaled_tab_width);
                            continue;
                        case U'\n':
                            continue;
                        default:
                            const u32 index = i * splitted_strings.size() + j;
                            glyphs[index] = &fnt->glyph_map.at(c);
                            accum_len++;
                            accum_width.store(accum_width.load() + glyphs[index]->advance * scale);
                            continue;
                    }
                }

                widths[i] = accum_width;
                line_lengths[i] = accum_len;
            }
        }

    }

    i32 iter = 0;
    for (int i = 0; i < (int)splitted_strings.size(); i++) {
        const f32 offset =
            (horizontal == CTEXT_HORI_ALIGN_CENTER) ?
                xbegin - widths[i] / 2.0f
               :
               (horizontal == CTEXT_HORI_ALIGN_RIGHT) ?
                   xbegin - widths[i]
                   :
                   xbegin;
        render_line(fnt, splitted_strings[i], scale, drawcall, offset, y + i * scaled_line_height, iter, i * splitted_strings.size(), glyphs);

        const u32 &effective_len = line_lengths[i];
        iter += effective_len;
        fnt->chars_drawn += effective_len;
    }

    free(glyphs);

    drawcall->vertex_count = iter * 4;
    drawcall->index_count = iter * 6;
}

void ctext::render(ctext::CFont fnt, const std::u32string_view text, const HoriAlignment horizontal, const VertAlignment vertical, const f32 x, const f32 y, const f32 scale)
{
    const u32 effective_length = get_effective_length(text);

    if (effective_length == 0)
        return;

    const u32 vertex_count = effective_length * 4;
    const u32 index_count = effective_length * 6;

    const u32 allocation_size = (vertex_count * sizeof(vec4)) + (index_count * sizeof(u32));

    void *allocation = malloc(allocation_size);
    memset(allocation, 0, allocation_size);

    text_drawcall_t drawcall;
    drawcall.vertices = (vec4 *)((uchar *)allocation);
    drawcall.indices = (u32 *)((uchar *)allocation + (vertex_count * sizeof(vec4)));
    drawcall.index_offset = (vertex_count * sizeof(vec4));
    gen_vertices(fnt, horizontal, vertical, &drawcall, text, x, y, scale);

    drawcall.vertex_count = effective_length * 4;
    drawcall.index_count = effective_length * 6;

    fnt->drawcalls.push_back(drawcall);
}

void ctext::initialize()
{
    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FONT_COUNT, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    constexpr VkDescriptorPoolSize poolSizes[] = {
        // type; descriptorCount;
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FONT_COUNT },
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.poolSizeCount = array_len(poolSizes);
    poolInfo.maxSets = 1;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &desc_pool) != VK_SUCCESS)
        LOG_ERROR("Failed to create descriptor pool");

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.pBindings = bindings;
    setLayoutInfo.bindingCount = array_len(bindings);
    if (vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &desc_Layout) != VK_SUCCESS)
        LOG_ERROR("%s Failto create descriptor set layout");

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = desc_pool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &desc_Layout;
    if (vkAllocateDescriptorSets(device, &setAllocInfo, &desc_set) != VK_SUCCESS)
        LOG_ERROR("Failed to allocate descriptor sets");

    u32 width, height;
    VkFormat dummyImageFmt;

    // No need to store image memory because it won't ever be deleted (probably)
    VkDeviceMemory dummyImageMemory;;
    stbi_image_free(help::Images::killme("../Assets/error.png", &width, &height, &dummyImageFmt, &error_image, &dummyImageMemory));

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &error_image_sampler) != VK_SUCCESS)
        LOG_ERROR("Failed to create sampler");

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = dummyImageFmt;
    viewInfo.image = error_image;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, nullptr, &error_image_view) != VK_SUCCESS)
        LOG_ERROR("Failed to create image view");

    VkDescriptorImageInfo dummyImageInfo{};
    dummyImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    dummyImageInfo.sampler = error_image_sampler;
    dummyImageInfo.imageView = error_image_view;

    for (u32 i = 0; i < MAX_FONT_COUNT; i++)
    {
        VkWriteDescriptorSet write_set{};
        write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_set.dstSet = desc_set;
        write_set.dstBinding = 0;
        write_set.descriptorCount = 1;
        write_set.pImageInfo = &dummyImageInfo;
        write_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_set.dstArrayElement = i;
        vkUpdateDescriptorSets(device, 1, &write_set, 0, nullptr);
    }
}

void ctext::CFont_T::resize_buffer(u32 new_buffer_size, u32 index_buffer_offset)
{
    u32 new_allocation_size;

    if (allocated_size < new_buffer_size) {
        new_allocation_size = glm::max(allocated_size * 2, new_buffer_size);
    }
    else if (new_buffer_size < (allocated_size / 3)) {
        new_allocation_size = glm::max(allocated_size / 3, new_buffer_size);
    }
    else
        return;

    vkDeviceWaitIdle(device);

    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, buffer_memory, nullptr);

    help::Buffers::CreateBuffer(
        new_allocation_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &buffer,
        &buffer_memory
    );

    allocated_size = new_allocation_size;
}
