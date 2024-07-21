#include "CFont.hpp"
#include "epichelperlib.hpp"

#include "stb/stb_image_write.h"
#include "stb/stb_image.h"

VkPipeline ctext::pipeline = VK_NULL_HANDLE;
VkPipelineLayout ctext::pipeline_layout = VK_NULL_HANDLE;
VkShaderModule ctext::vertex = VK_NULL_HANDLE;
VkShaderModule ctext::fragment = VK_NULL_HANDLE;
VkDescriptorPool ctext::desc_pool = VK_NULL_HANDLE;
VkDescriptorSetLayout ctext::desc_Layout = VK_NULL_HANDLE;
VkDescriptorSet ctext::desc_set = VK_NULL_HANDLE;
VkImage ctext::error_image = VK_NULL_HANDLE;
VkImageView ctext::error_image_view = VK_NULL_HANDLE;
VkSampler ctext::error_image_sampler = VK_NULL_HANDLE;

/* I have no idea what any of this is */

typedef char32_t unicode;

constexpr u32 channels = 3;
constexpr auto DistanceFieldGenerator = &msdf_atlas::msdfGenerator; // Change this when changing channels too
static_assert(channels >= 1 && channels <= 4 && channels != 2);

constexpr VkFormat imageFormat = 
    (channels == 4) ? VK_FORMAT_R8G8B8A8_UNORM :
    (channels == 3) ? VK_FORMAT_R8G8B8_UNORM :
    (channels == 1) ? VK_FORMAT_R8_UNORM :
    VK_FORMAT_UNDEFINED;

void ctext::CFLoad(const CFontLoadInfo* pInfo, CFont* dst)
{
	if (!pInfo || !dst)
	{
		LOG_ERROR("pInfo or dst is nullptr!");
		return;
	}

    msdfgen::FreetypeHandle *ft = msdfgen::initializeFreetype();
    if (!ft) {
        LOG_ERROR("msdfgen<Freetype> failed to initialize");
        return;
    }

    msdfgen::FontHandle *font = msdfgen::loadFont(ft, pInfo->fontPath);
    if (!font) {
        LOG_ERROR("Failed to load font. Is path (\"%s\") valid?", pInfo->fontPath);
        return;
    }

    *dst = new ctext::_CFont();

    std::vector<msdf_atlas::GlyphGeometry> glyphs;
    msdf_atlas::FontGeometry fontGeometry(&glyphs);
    msdf_atlas::Charset charset;
    fontGeometry.loadCharset(font, 1.0, charset.ASCII);

    constexpr double maxCornerAngle = 3.0;
    for (msdf_atlas::GlyphGeometry& glyph : glyphs)
        glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);

    msdf_atlas::TightAtlasPacker packer;
    packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::POWER_OF_TWO_SQUARE);
    packer.setMinimumScale(24.0);
    packer.setPixelRange(2.0);
    packer.setMiterLimit(1.0);
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
        msdfgen::getFontMetrics(metrics, font);

        (**dst).line_height = metrics.lineHeight;
        (**dst).ascender = metrics.ascenderY / metrics.emSize;
        (**dst).descender  = metrics.descenderY / metrics.emSize;

        mogus.advance = static_cast<f32>(glyph.getAdvance());
        (**dst).m_glyph_geometry[glyph.getCodepoint()] = mogus;
    }
    among.close();

    ctext::_CFont &dstRef = **dst; // Dereference CFont* -> _CFont* -> _CFont
    dstRef.atlasWidth = width;
    dstRef.atlasHeight = height;
    dstRef.atlasData = pixelBuffer;

    help::Images::killme(pixelBuffer, width, height, imageFormat, channels, &dstRef.texture, &dstRef.texture_memory);

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if (vkCreateSampler(device, &sampler_info, nullptr, &dstRef.sampler) != VK_SUCCESS)
        LOG_ERROR("Failed to create sampler");

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = imageFormat;
    view_info.image = dstRef.texture;
    view_info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &view_info, nullptr, &dstRef.texture_view) != VK_SUCCESS)
        LOG_ERROR("Failed to create image view");

    VkDescriptorImageInfo error_image_info{};
    error_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    error_image_info.sampler = dstRef.sampler;
    error_image_info.imageView = dstRef.texture_view;

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
}

void GenerateGlyphVertices(const ctext::CFont font, std::vector<vec4>& vertices, std::vector<u32>& indices, const std::u32string& str, f32 xbegin, f32 ybegin) {
    f32 x = xbegin;
    f32 y = 0.0f;
    u32 indexOffset = 0;
    for (const auto& c : str)
    {
        if (c == U'\n') {
            x = 0.0f;
            y += font->line_height;
            continue;
        }

        const ctext::CFGlyph& glyph = font->get_glyph(c);

        f32 x0 = x + glyph.x0;
        f32 x1 = x + glyph.x1;
        f32 y0 = y + glyph.y0;
        f32 y1 = y + glyph.y1;

        vertices.push_back(vec4( x0, y0, glyph.l, glyph.t ));
        vertices.push_back(vec4( x1, y0, glyph.r, glyph.t ));
        vertices.push_back(vec4( x1, y1, glyph.r, glyph.b ));
        vertices.push_back(vec4( x0, y1, glyph.l, glyph.b ));

        indices.push_back(indexOffset + 0);
        indices.push_back(indexOffset + 1);
        indices.push_back(indexOffset + 2);
        indices.push_back(indexOffset + 2);
        indices.push_back(indexOffset + 3);
        indices.push_back(indexOffset + 0);

        x += glyph.advance;
        indexOffset += 4;
    }
}

void ctext::Render(ctext::CFont font, VkCommandBuffer cmd, std::u32string text, float x, float y, float scale)
{
    cmd = Renderer::GetDrawBuffer();
    std::vector<vec4> vertices;
    std::vector<u32> indices;

    static VkBuffer buff;
    static VkDeviceMemory mem;
    std::u32string sus = U"AjBqG";
    GenerateGlyphVertices(font, vertices, indices, sus, 0.0f, 0.0f);

    if (!buff) {
        help::Buffers::CreateBuffer(
            vertices.size() * sizeof(vec4) + indices.size() * sizeof(u32),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &buff,
            &mem
        );
    }

    void* data;
    vkMapMemory(device, mem, 0, vertices.size() * sizeof(vec4), 0, &data);
    memcpy(data, vertices.data(), vertices.size() * sizeof(vec4));
    memcpy((uchar *)data + vertices.size() * sizeof(vec4), indices.data(), indices.size() * sizeof(u32));
    vkUnmapMemory(device, mem);

    i32 mx_, my_;
    SDL_GetMouseState(&mx_, &my_);
    f32 mx = mx_, my = my_;
    mx = f32(mx)/vctx::RenderExtent.width;
    my = f32(my)/vctx::RenderExtent.height;
    mx = mx * 2.0f - 1.0f;
    my = my * 2.0f - 1.0f;

    mat4 model(1.0f);
    model = glm::translate(model, vec3(mx, my, 0.0f));
    model = glm::scale(model, vec3(scale, scale, 1.0f));

    constexpr VkDeviceSize offsets = 0;
    VkDeviceSize indexOffset = vertices.size() * sizeof(vec4);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &desc_set, 0, nullptr);
    vkCmdBindVertexBuffers(cmd, 0, 1, &buff, &offsets);
    vkCmdBindIndexBuffer(cmd, buff, indexOffset, VK_INDEX_TYPE_UINT32);
    vkCmdPushConstants(cmd, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &model);
    vkCmdDrawIndexed(cmd, indices.size(), 1, 0, 0, 0);
}

void ctext::Init()
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

    VkWriteDescriptorSet *writeSets = new VkWriteDescriptorSet[MAX_FONT_COUNT];
    memset(writeSets, 0, sizeof(VkWriteDescriptorSet) * MAX_FONT_COUNT);
    for (u32 i = 0; i < MAX_FONT_COUNT; i++)
    {
        writeSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSets[i].dstSet = desc_set;
        writeSets[i].dstBinding = 0;
        writeSets[i].descriptorCount = 1;
        writeSets[i].pImageInfo = &dummyImageInfo;
        writeSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSets[i].dstArrayElement = i;
    }
    vkUpdateDescriptorSets(device, MAX_FONT_COUNT, writeSets, 0, nullptr);
    delete[] writeSets;

    u32 VertexShaderBinarySize, FragmentShaderBinarySize;
    help::Files::LoadBinary("./Shaders/vert.text.spv", nullptr, &VertexShaderBinarySize);
    std::vector<u8> VertexShaderBinary(VertexShaderBinarySize);
    help::Files::LoadBinary("./Shaders/vert.text.spv", VertexShaderBinary.data(), &VertexShaderBinarySize);

    help::Files::LoadBinary("./Shaders/frag.text.spv", nullptr, &FragmentShaderBinarySize);
    std::vector<u8> FragmentShaderBinary(FragmentShaderBinarySize);
    help::Files::LoadBinary("./Shaders/frag.text.spv", FragmentShaderBinary.data(), &FragmentShaderBinarySize);

    VkShaderModuleCreateInfo vertexShaderModuleInfo{};
    vertexShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderModuleInfo.codeSize = VertexShaderBinarySize;
    vertexShaderModuleInfo.pCode = reinterpret_cast<const u32*>(VertexShaderBinary.data());
    if (vkCreateShaderModule(device, &vertexShaderModuleInfo, nullptr, &vertex) != VK_SUCCESS) {
        LOG_ERROR("Failed to create vertex shader module!");
    }

    VkShaderModuleCreateInfo fragmentShaderModuleInfo{};
    fragmentShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentShaderModuleInfo.codeSize = FragmentShaderBinarySize;
    fragmentShaderModuleInfo.pCode = reinterpret_cast<const u32*>(FragmentShaderBinary.data());
    if (vkCreateShaderModule(device, &fragmentShaderModuleInfo, nullptr, &fragment) != VK_SUCCESS) {
        LOG_ERROR("Failed to create fragment shader module!");
    }

    CreatePipeline();

    vctx::OnWindowResized.connect(
        CreatePipeline
    );
}

void ctext::CreatePipeline()
{
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
    vertexShaderInfo.module = vertex;
    vertexShaderInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderInfo{};
    fragmentShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderInfo.module = fragment;
    fragmentShaderInfo.pName = "main";

    const std::vector<VkPipelineShaderStageCreateInfo> shaders = { vertexShaderInfo, fragmentShaderInfo };
    const std::vector<VkDescriptorSetLayout> layouts = { desc_Layout };

    const pro::PipelineBlendState blend = pro::PipelineBlendState(pro::BlendPreset::PRO_BLEND_PRESET_ALPHA);

    pro::PipelineCreateInfo pc{};
    pc.format = vctx::SwapChainImageFormat;
    pc.subpass = 0;
    pc.renderPass = vctx::GlobalRenderPass;
    pc.pipelineLayout = pipeline_layout;
    pc.pAttributeDescriptions = &attributeDescriptions;
    pc.pBindingDescriptions = &bindingDescriptions;
    pc.pPushConstants = &pushConstants;
    pc.pDescriptorLayouts = &layouts;
    pc.pShaderCreateInfos = &shaders;
    pc.extent.width = vctx::RenderExtent.width;
    pc.extent.height = vctx::RenderExtent.height;
    pc.pShaderCreateInfos = &shaders;
    pc.pBlendState = &blend;
    pro::CreatePipelineLayout(device, &pc, &pipeline_layout);
    pc.pipelineLayout = pipeline_layout;
    pro::CreateGraphicsPipeline(device, &pc, &pipeline, 0);
}
