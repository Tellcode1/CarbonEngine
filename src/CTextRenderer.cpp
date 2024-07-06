#include "CTextRenderer.hpp"
#include "Renderer.hpp"
#include "epichelperlib.hpp"
#include "stb/stb_image.h"

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

typedef char32_t unicode;

void GenerateGlyphVertices(const cf::CFont font, const cf::CFGlyph& glyph, std::vector<vec4>& vertices, float x, float y, float scale) {
    for (const msdfgen::Contour& contour : glyph.geometry.getShape().contours) {
        for (const msdfgen::EdgeHolder& edge : contour.edges) {
            const msdfgen::Point2 start = edge->point(0.0);
            const msdfgen::Point2 end = edge->point(1.0);

            /*
                Why the FJSDKLFJKL does msdfgen use doubles instead of floats :<<<<<<
                kill me
            */
            double l_, b_, r_, t_;
            glyph.geometry.getQuadAtlasBounds(l_, b_, r_, t_);
            
            // doubles are for losers. L bozo ratio
            f32 l = l_, b = b_, r = r_, t = t_;

            vec2 texturePositionsA = vec2(
                l / font->atlasWidth, b / font->atlasHeight
            );

            vec2 texturePositionsB = vec2(
                r / font->atlasWidth, t / font->atlasHeight
            );

            vec2 verticesA = vec2(
                (x + start.x * scale),
                (y + start.y * scale)
            );

            vec2 verticesB = vec2(
                (x + end.x * scale),
                (y + end.y * scale)
            );

            vertices.push_back(vec4(verticesA, texturePositionsA));
            vertices.push_back(vec4(verticesB, texturePositionsB));
        }
    }
}

void ctext::Render(cf::CFont font, VkCommandBuffer cmd, std::u32string text, float x, float y, float scale)
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

    // const u32 indexOffset = vertices.size() * sizeof(vec4);

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

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descSet, 0, nullptr);
    vkCmdBindVertexBuffers(cmd, 0, 1, &buff, &offsets);
    VkDeviceSize indexOffset = vertices.size() * sizeof(vec4);
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
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxFontCount},
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

    i32 width, height, channels;
    u8 *buffer = stbi_load("../Assets/error.png", &width, &height, &channels, 3);

    VkFormat dummyImageFmt = VK_FORMAT_R8G8B8_SRGB;

    // No need to store image memory because it won't ever be deleted (probably)
    VkDeviceMemory dummyImageMemory;
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = dummyImageFmt;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageCreateInfo, nullptr, &dummyImage) != VK_SUCCESS)
        LOG_ERROR("Failed to create dummy image for invalid glyphs");

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, dummyImage, &imageMemoryRequirements);

    const u32 localDeviceMemoryIndex = pro::GetMemoryType(physDevice, imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = imageMemoryRequirements.size;
    allocInfo.memoryTypeIndex = localDeviceMemoryIndex;

    vkAllocateMemory(device, &allocInfo, nullptr, &dummyImageMemory);
    vkBindImageMemory(device, dummyImage, dummyImageMemory, 0);

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = width * height * channels;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(device, &stagingBufferInfo, nullptr, &stagingBuffer);

    VkMemoryRequirements stagingBufferRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferRequirements);

    VkMemoryAllocateInfo stagingBufferAllocInfo{};
    stagingBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingBufferAllocInfo.allocationSize = stagingBufferRequirements.size;
    stagingBufferAllocInfo.memoryTypeIndex = pro::GetMemoryType(physDevice, stagingBufferRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(device, &stagingBufferAllocInfo, nullptr, &stagingBufferMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void *stagingBufferMapped;
    if (vkMapMemory(device, stagingBufferMemory, 0, width * height * channels, 0, &stagingBufferMapped) != VK_SUCCESS)
        LOG_ERROR("Failed to map staging buffer memory!");
    memcpy(stagingBufferMapped, buffer, width * height * channels);
    vkUnmapMemory(device, stagingBufferMemory);

    const VkCommandBuffer cmd = help::Commands::BeginSingleTimeCommands();

    help::Images::TransitionImageLayout(
        cmd, dummyImage, 1,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        0, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );

    VkBufferImageCopy region{};
    region.imageExtent = { (u32)width, (u32)height, 1 };
    region.imageOffset = { 0, 0, 0 };
    region.bufferOffset = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.mipLevel = 0;
    vkCmdCopyBufferToImage(cmd, stagingBuffer, dummyImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    help::Commands::EndSingleTimeCommands(cmd);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

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
    viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B };
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
        [&]() {
            CreatePipeline();
        }
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

    // ! ENABLE BLENDING FOR TEXT RENDERING (I'm debugging this so im leaving it disabled ok)
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
    pro::CreatePipelineLayout(device, &pc, &pipelineLayout);
    pc.pipelineLayout = pipelineLayout;
    pro::CreateGraphicsPipeline(device, &pc, &pipeline, 0);
}
