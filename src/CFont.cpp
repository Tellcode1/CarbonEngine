#include "CFont.hpp"
#include "epichelperlib.hpp"

#include "stb/stb_image_write.h"
#include "stb/stb_image.h"

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
    msdfgen::FontHandle *font = msdfgen::loadFont(ft, pInfo->fontPath);

    if (!ft) {
        LOG_ERROR("msdfgen<Freetype> failed to initialize");
        return;
    }

    if (!font) {
        LOG_ERROR("Failed to load font. Is path (\"%s\") valid?", pInfo->fontPath);
        return;
    }

    *dst = new ctext::_CFont();

    std::vector<msdf_atlas::GlyphGeometry> glyphs;
    msdf_atlas::FontGeometry fontGeometry(&glyphs);
    fontGeometry.loadCharset(font, 1.0, msdf_atlas::Charset::ASCII);

    constexpr double maxCornerAngle = 3.0;
    for (msdf_atlas::GlyphGeometry& glyph : glyphs) {
        glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);
    }

    msdf_atlas::TightAtlasPacker packer;
    packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::POWER_OF_TWO_SQUARE);
    packer.setMinimumScale(24.0);
    packer.setPixelRange(2.0);
    packer.setMiterLimit(1.0);
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

    msdf_atlas::GeneratorAttributes attributes;
    // generator.setAttributes(attributes);
    generator.setThreadCount(std::thread::hardware_concurrency());
    generator.generate(glyphs.data(), glyphs.size());

    msdfgen::BitmapConstRef<f32, channels> bitmap = generator.atlasStorage();

    u8* pixelBuffer = new u8[ width * height * channels ];
    memset(pixelBuffer, 0, width * height * channels);

    // what the f(*(#$)*@#) is this
    for (u32 y = 0; y < height; y++)
        for (u32 x = 0; x < width; x++)
            for (u32 c = 0; c < channels; c++)
                pixelBuffer[(width * (height - y - 1) + x) * channels + c] = msdfgen::pixelFloatToByte(bitmap.pixels[(width * y + x) * channels + c]);

    msdfgen::savePng(bitmap, "debug.png");

    std::ofstream among("amongus.md");
    for (const msdf_atlas::GlyphGeometry& glyph : glyphs) {
        i32 x0, y0, x1, y1;
        glyph.getBoxRect(x0, y0, x1, y1);

        f64 s0, t0, s1, t1;
        glyph.getQuadAtlasBounds(s0, t0, s1, t1);
        
        among <<
            glyph.getCodepoint() << " : " <<
            s0 << ", " <<
            t0 << ", " <<
            s1 << ", " <<
            t1 << '\n';

        CFGlyph mogus;
        mogus.s0 = s0 / f32(width);
        mogus.t0 = t0 / f32(height);
        mogus.s1 = s1 / f32(width);
        mogus.t1 = t1 / f32(height);

        mogus.x0 = x0;
        mogus.y0 = y0;
        mogus.x1 = x1;
        mogus.y1 = y1;

        mogus.advance = glyph.getAdvance();
        (*dst)->m_glyphGeometry[glyph.getCodepoint()] = mogus;
    }
    among.close();

    ctext::_CFont &dstRef = **dst; // Dereference CFont* -> _CFont* -> _CFont
    dstRef.atlasWidth = width;
    dstRef.atlasHeight = height;
    dstRef.atlasData = pixelBuffer;

    std::cout << "Allocated image of size " << width*height*channels << " bytes" << std::endl;

    help::Images::killme(pixelBuffer, width, height, imageFormat, channels, &dstRef.texture, &dstRef.textureMemory);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &dstRef.sampler) != VK_SUCCESS)
        LOG_ERROR("Failed to create sampler");

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageFormat;
    viewInfo.image = dstRef.texture;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, nullptr, &dstRef.textureView) != VK_SUCCESS)
        LOG_ERROR("Failed to create image view");

    VkDescriptorImageInfo dummyImageInfo{};
    dummyImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    dummyImageInfo.sampler = dstRef.sampler;
    dummyImageInfo.imageView = dstRef.textureView;

    for (u32 i = 0; i < MaxFontCount; i++)
    {
        VkWriteDescriptorSet writeSet{};
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = descSet;
        writeSet.dstBinding = 0;
        writeSet.descriptorCount = 1;
        writeSet.pImageInfo = &dummyImageInfo;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.dstArrayElement = i;
        vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
    }
}

VkPipeline ctext::pipeline = VK_NULL_HANDLE;
VkPipelineLayout ctext::pipelineLayout = VK_NULL_HANDLE;
VkShaderModule ctext::vertex = VK_NULL_HANDLE;
VkShaderModule ctext::fragment = VK_NULL_HANDLE;
VkDescriptorPool ctext::descPool = VK_NULL_HANDLE;
VkDescriptorSetLayout ctext::descLayout = VK_NULL_HANDLE;
VkDescriptorSet ctext::descSet = VK_NULL_HANDLE;
VkImage ctext::dummyImage = VK_NULL_HANDLE;
VkImageView ctext::dummyImageView = VK_NULL_HANDLE;
VkSampler ctext::dummyImageSampler = VK_NULL_HANDLE;

void GenerateGlyphVertices(const ctext::CFont font, const ctext::CFGlyph& glyph, std::vector<vec4>& vertices, std::vector<u16>& indices, float x, float y, float scale) {
    
}

void ctext::Render(ctext::CFont font, VkCommandBuffer cmd, std::u32string text, float x, float y, float scale)
{
    cmd = Renderer::GetDrawBuffer();
    std::vector<vec4> vertices;

    static VkBuffer buff;
    static VkDeviceMemory mem;
    vertices = {
        vec4(-1.0f, -1.0f, vec2(0.0f, 0.0f)),
        vec4( 1.0f, -1.0f, vec2(1.0f, 0.0f)),
        vec4( 1.0f,  1.0f, vec2(1.0f, 1.0f)),
        vec4(-1.0f,  1.0f, vec2(0.0f, 1.0f))
    };
    std::vector<u32> indices = {
        0, 1, 2,
        0, 2, 3
    };

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

    constexpr VkDeviceSize offsets = 0;
    VkDeviceSize indexOffset = vertices.size() * sizeof(vec4);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSet, 0, nullptr);
    vkCmdBindVertexBuffers(cmd, 0, 1, &buff, &offsets);
    vkCmdBindIndexBuffer(cmd, buff, indexOffset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, indices.size(), 1, 0, 0, 0);
}

void ctext::Init()
{
    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxFontCount, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    constexpr VkDescriptorPoolSize poolSizes[] = {
        // type; descriptorCount;
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxFontCount },
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.poolSizeCount = array_len(poolSizes);
    poolInfo.maxSets = 1;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descPool) != VK_SUCCESS)
        LOG_ERROR("Failed to create descriptor pool");

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.pBindings = bindings;
    setLayoutInfo.bindingCount = array_len(bindings);
    if (vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &descLayout) != VK_SUCCESS)
        LOG_ERROR("%s Failto create descriptor set layout");

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = descPool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &descLayout;
    if (vkAllocateDescriptorSets(device, &setAllocInfo, &descSet) != VK_SUCCESS)
        LOG_ERROR("Failed to allocate descriptor sets");

    u32 width, height;
    VkFormat dummyImageFmt;

    // No need to store image memory because it won't ever be deleted (probably)
    VkDeviceMemory dummyImageMemory;;
    stbi_image_free(help::Images::killme("../Assets/error.png", &width, &height, &dummyImageFmt, &dummyImage, &dummyImageMemory));

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &dummyImageSampler) != VK_SUCCESS)
        LOG_ERROR("Failed to create sampler");

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = dummyImageFmt;
    viewInfo.image = dummyImage;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, nullptr, &dummyImageView) != VK_SUCCESS)
        LOG_ERROR("Failed to create image view");

    VkDescriptorImageInfo dummyImageInfo{};
    dummyImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    dummyImageInfo.sampler = dummyImageSampler;
    dummyImageInfo.imageView = dummyImageView;

    VkWriteDescriptorSet *writeSets = new VkWriteDescriptorSet[MaxFontCount];
    memset(writeSets, 0, sizeof(VkWriteDescriptorSet) * MaxFontCount);
    for (u32 i = 0; i < MaxFontCount; i++)
    {
        writeSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSets[i].dstSet = descSet;
        writeSets[i].dstBinding = 0;
        writeSets[i].descriptorCount = 1;
        writeSets[i].pImageInfo = &dummyImageInfo;
        writeSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSets[i].dstArrayElement = i;
    }
    vkUpdateDescriptorSets(device, MaxFontCount, writeSets, 0, nullptr);
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
    const std::vector<VkDescriptorSetLayout> layouts = { descLayout };

    const pro::PipelineBlendState blend = pro::PipelineBlendState(pro::BlendPreset::PRO_BLEND_PRESET_ALPHA);

    pro::PipelineCreateInfo pc{};
    pc.format = vctx::SwapChainImageFormat;
    pc.subpass = 0;
    pc.renderPass = vctx::GlobalRenderPass;
    pc.pipelineLayout = pipelineLayout;
    pc.pAttributeDescriptions = &attributeDescriptions;
    pc.pBindingDescriptions = &bindingDescriptions;
    pc.pDescriptorLayouts = &layouts;
    pc.pShaderCreateInfos = &shaders;
    pc.extent.width = vctx::RenderExtent.width;
    pc.extent.height = vctx::RenderExtent.height;
    pc.pShaderCreateInfos = &shaders;
    pc.pBlendState = &blend;
    pro::CreatePipelineLayout(device, &pc, &pipelineLayout);
    pc.pipelineLayout = pipelineLayout;
    pro::CreateGraphicsPipeline(device, &pc, &pipeline, 0);
}
