#include "ctext.hpp"
#include "epichelperlib.hpp"
#include "Bootstrap.hpp"

#include "stb/stb_image_write.h"
#include "stb/stb_image.h"
#include <cinput.hpp>

VkDescriptorPool ctext::desc_pool = VK_NULL_HANDLE;
VkDescriptorSetLayout ctext::desc_Layout = VK_NULL_HANDLE;
VkDescriptorSet ctext::desc_set = VK_NULL_HANDLE;
VkImage ctext::error_image = VK_NULL_HANDLE;
VkImageView ctext::error_image_view = VK_NULL_HANDLE;
VkSampler ctext::error_image_sampler = VK_NULL_HANDLE;

/* I have no idea what any of this is */

typedef char32_t unicode;

constexpr u32 channels = 1;
constexpr auto DistanceFieldGenerator = &msdf_atlas::sdfGenerator; // Change this when changing channels too
static_assert(channels >= 1 && channels <= 4 && channels != 2);

constexpr VkFormat imageFormat = 
    (channels == 4) ? VK_FORMAT_R8G8B8A8_UNORM :
    (channels == 3) ? VK_FORMAT_R8G8B8_UNORM :
    (channels == 1) ? VK_FORMAT_R8_UNORM :
    VK_FORMAT_UNDEFINED;

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

    msdf_atlas::ImmediateAtlasGenerator<
        f32,
        channels,
        DistanceFieldGenerator,
        msdf_atlas::BitmapAtlasStorage<f32, channels>
    > generator(width, height);

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

    std::ofstream among("amongus.md");
    for (const msdf_atlas::GlyphGeometry& glyph : glyphs) {
        if (glyph.isWhitespace()) {
            (*dst)->space_width = glyph.getAdvance();
            continue;
        }

        f64 x0_, y0_, x1_, y1_;
        glyph.getQuadPlaneBounds(x0_, y0_, x1_, y1_);

        f64 l_, b_, r_, t_;
        glyph.getQuadAtlasBounds(l_, b_, r_, t_);

        f32 l = f32(l_), r = f32(r_);
        f32 b  = 1.0f - (f32)b_;
        f32 t  = 1.0f - (f32)t_;
        
        f32 x0 = f32(x0_), x1 = f32(x1_);
        f32 y0 = 1.0f - (f32)y0_;
        f32 y1 = 1.0f - (f32)y1_;

        CFGlyph mogus;
        mogus.l = l / f32(width);
        mogus.b = t / f32(height);
        mogus.r = r / f32(width);
        mogus.t = b / f32(height);

        mogus.x0 = x0;
        mogus.y0 = y0;
        mogus.x1 = x1;
        mogus.y1 = y1;

        msdfgen::FontMetrics metrics;
        msdfgen::getFontMetrics(metrics, fnt);

        (*dst)->line_height = f32(metrics.lineHeight) / f32(metrics.emSize);

        mogus.advance = static_cast<f32>(glyph.getAdvance());
        if (glyph.getCodepoint() !=0)
            (*dst)->m_glyph_geometry[glyph.getCodepoint()] = mogus;
    }
    among.close();

    (*dst)->atlasWidth = width;
    (*dst)->atlasHeight = height;
    (*dst)->atlasData = pixelBuffer;

    help::Images::killme(pixelBuffer, width, height, imageFormat, channels, &(*dst)->texture, &(*dst)->texture_memory);

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
    view_info.format = imageFormat;
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
        { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4) }
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
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &(*dst)->buffer,
        &(*dst)->buffer_memory
    );
}

bool ctext::font_is_valid(const ctext::CFont fnt)
{
    bool valid = true;
    if (fnt == nullptr) valid = false;
    if (fnt->atlasWidth == 0 && fnt->atlasHeight == 0) valid = false;
    if (fnt->m_glyph_geometry.size() == 0) valid = false;
    return valid;
}

void ctext::end_render(ctext::CFont fnt)
{
    if (fnt->drawcalls.empty())
        return;

    u32 total_vertex_byte_size = 0;
    u32 total_index_count = 0;

    for (const text_drawcall_t *drawcall : fnt->drawcalls) {
        total_vertex_byte_size += drawcall->vertex_count * sizeof(vec4);
        total_index_count += drawcall->index_count;
    }

    const u32 total_index_byte_size = total_index_count * sizeof(u32);

    vkDeviceWaitIdle(device);
    vkDestroyBuffer(device, fnt->buffer, nullptr);
    vkFreeMemory(device, fnt->buffer_memory, nullptr);

    const u32 buff_size = total_vertex_byte_size + total_index_byte_size;
    help::Buffers::CreateBuffer(
        buff_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &fnt->buffer,
        &fnt->buffer_memory
    );

    void *mapped;
    vkMapMemory(device, fnt->buffer_memory, 0, buff_size, 0, &mapped);

    u32 vertex_copy_iterator = 0;
    u32 index_copy_iterator = 0;
    for (const text_drawcall_t *drawcall : fnt->drawcalls) {
        memcpy(static_cast<u8 *>(mapped) + vertex_copy_iterator, drawcall->vertices, drawcall->vertex_count * sizeof(vec4));
        memcpy(static_cast<u8 *>(mapped) + total_vertex_byte_size + index_copy_iterator, drawcall->indices, drawcall->index_count * sizeof(u32));
        vertex_copy_iterator += drawcall->vertex_count * sizeof(vec4);
        index_copy_iterator += drawcall->index_count * sizeof(u32);
    }

    vkUnmapMemory(device, fnt->buffer_memory);

    const VkCommandBuffer cmd = Renderer::GetDrawBuffer();
    constexpr VkDeviceSize offsets[] = { 0 };

    vkCmdPushConstants(
        cmd,
        fnt->pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0, sizeof(mat4),
        glm::value_ptr(glm::translate(glm::mat4(1.0f), vec3(cinput::get_mouse_position(), 0.0f)))
    );

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fnt->pipeline_layout, 0, 1, &desc_set, 0, nullptr);
    vkCmdBindVertexBuffers(cmd, 0, 1, &fnt->buffer, offsets);
    vkCmdBindIndexBuffer(cmd, fnt->buffer, total_vertex_byte_size, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fnt->pipeline);
    vkCmdDrawIndexed(cmd, total_index_count, 1, 0, 0, 0);
}

ctext::text_drawcall_t *ctext::create_text_drawcall(u32 num_verts, u32 num_inds)
{
    void *allocation = malloc(sizeof(text_drawcall_t) + (num_verts * sizeof(vec4)) + (num_inds * sizeof(u32)));
    text_drawcall_t  *new_drawcall = (text_drawcall_t *)allocation;
    new_drawcall->vertex_count = num_verts;
    new_drawcall->index_count = num_inds;
    new_drawcall->vertices = (vec4 *)((uchar *)allocation + sizeof(text_drawcall_t));
    new_drawcall->indices = (u32 *)((uchar *)allocation + (num_verts * sizeof(vec4)) + sizeof(text_drawcall_t));

    return new_drawcall;
}

void ctext::begin_render(ctext::CFont fnt)
{
    fnt->chars_drawn = 0;
    for (text_drawcall_t *drawcall : fnt->drawcalls) {
        free(drawcall);
    }
    fnt->drawcalls.clear();
}

/* Takes a string with either \0 or \n at the end */
void ctext::render_line(const ctext::CFont fnt, const std::u32string &str, f32 *xbegin, f32 y)
{

}

/*
*   rets the count of indices. pass to vkCmdDrawIndexed
*/
u32 gen_vertices(const ctext::CFont fnt, ctext::text_drawcall_t *drawcall, const std::u32string& str, f32 xbegin, f32 ybegin, f32 scale) {
    if (str.length() == 0)
        return 0;

    if (!ctext::font_is_valid(fnt))
        return UINT32_MAX;

    f32 x = xbegin;
    f32 y = ybegin;
    u32 i = 0;

    drawcall->index_count = 0;
    drawcall->vertex_count = 0;

    for (const auto& c : str)
    {
        if (c == U'\n') {
            x = xbegin;
            y += fnt->line_height;
            continue;
        } else if (c == U' ') {
            x += fnt->space_width;
            continue;
        }

        const ctext::CFGlyph& glyph = fnt->get_glyph(c);

        const f32 x0 = (x + glyph.x0) * scale;
        const f32 x1 = (x + glyph.x1) * scale;
        const f32 y0 = (y + glyph.y0) * scale;
        const f32 y1 = (y + glyph.y1) * scale;

        vec4 *vertex_output_data = (drawcall->vertices) + i * 4;
        vertex_output_data[0] = vec4( x0, y0, glyph.l, glyph.t );
        vertex_output_data[1] = vec4( x1, y0, glyph.r, glyph.t );
        vertex_output_data[2] = vec4( x1, y1, glyph.r, glyph.b );
        vertex_output_data[3] = vec4( x0, y1, glyph.l, glyph.b );
        
        const u32 index_offset = fnt->chars_drawn * 4;
        u32 *index_output_data = (drawcall->indices) + i * 6;
        index_output_data[0] = index_offset + 0;
        index_output_data[1] = index_offset + 1;
        index_output_data[2] = index_offset + 2;
        index_output_data[3] = index_offset + 2;
        index_output_data[4] = index_offset + 3;
        index_output_data[5] = index_offset + 0;

        x += glyph.advance;
        fnt->chars_drawn++;
        drawcall->vertex_count += 4;
        drawcall->index_count += 6;
        i++;
    }

    return i * 6;
}

void ctext::Render(ctext::CFont fnt, std::u32string text, float x, float y, float scale)
{
    cassert_and_ret(font_is_valid(fnt));

    u32 effective_length = 0;
    for (const auto& c : text)
        if (c != U'\n' && c != U'\t' && c != U' ')
            effective_length++;

    if (effective_length == 0) {
        fnt->chars_drawn = 0;
        return;
    }

    const u32 vertex_count = effective_length * 4;
    const u32 index_count = effective_length * 6;

    text_drawcall_t *drawcall = create_text_drawcall(vertex_count, index_count);
    
    gen_vertices(fnt, drawcall, text, 0.0f, 0.0f, scale);
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
