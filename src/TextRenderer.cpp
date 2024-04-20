#include "TextRenderer.hpp"

TextRenderer::TextRenderer()
{
}

// To help parallelize
// static void loop() {

// }

void TextRenderer::Init(VkDevice device, VkPhysicalDevice physDevice, VkQueue queue, VkCommandPool commandPool, const char *fontPath)
{
    m_device = device;
    m_physDevice = physDevice;
    m_queue = queue;

    constexpr const char *fontFile = "/usr/share/fonts/truetype/liberation2/LiberationMono-Italic.ttf";
    constexpr u32 fontSize = 32;

    FT_Library lib;
    FT_Face face;

    if (FT_Init_FreeType(&lib))
    {
        std::cerr << "Failed to initialize FreeType library" << std::endl;
    }

    if (FT_New_Face(lib, fontFile, 0, &face))
    {
        std::cerr << "Failed to load font file: " << fontFile << std::endl;
        FT_Done_FreeType(lib);
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    bmpWidth = 0, bmpHeight = 0;

    for (uchar c = 0; c < 128; c++)
    {
        auto res = FT_Load_Char(face, c, FT_LOAD_BITMAP_METRICS_ONLY);
        assert(res == 0);

        Character character = {
            uvec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            bmpWidth,
            static_cast<u32>(face->glyph->advance.x)};

        Characters.insert(std::pair<char, Character>(c, character));

        bmpHeight = std::max(bmpHeight, face->glyph->bitmap.rows);
        bmpWidth += face->glyph->bitmap.width;
    };
        
    // }

    assert(bmpWidth != 0 && bmpHeight != 0);

    u8 buffer[bmpWidth * bmpHeight];

    u32 xpos = 0;
    for (uchar c = 0; c < 128; c++)
    {
        Character& ch = Characters[c];

        const u32 width = ch.size.x;
        const u32 height = ch.size.y;

        if (width > 0)
        {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                std::cerr << "Failed to load glyph: " << c << std::endl;
                continue;
            }

            uchar const* charDataBuffer = face->glyph->bitmap.buffer;

            for (u32 row = 0; row < height; row++)
            {
                /*
                    Copies over each row of pixels

                    <-----bmpWidth----->
                    X X X X X X X X X X ; row = 0 (rowSize = bmpWidth)
                    X X X X X X X X X X ; row = 1
                    X X X X X X X X X X ; row = 2

                    To fetch all the data from row 0, you would do buffer offset by row * the width of each row (bmpWidth).
                */
                memcpy(buffer + row * bmpWidth + xpos,
                       charDataBuffer + row * width,
                       width
                );
            }
        }

        xpos += width;
    };

    i32 ftres = FT_Done_Face(face) && FT_Done_FreeType(lib);
    assert(ftres == 0);

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = bmpWidth;
    imageCreateInfo.extent.height = bmpHeight;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = VK_FORMAT_R8_UNORM;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult vkres = vkCreateImage(device, &imageCreateInfo, nullptr, &FontTexture);
    if (vkres != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image" + std::to_string(vkres));
    }

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, FontTexture, &imageMemoryRequirements);

    const u32 localDeviceMemoryIndex = pro::GetMemoryType(physDevice, imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = imageMemoryRequirements.size;
    allocInfo.memoryTypeIndex = localDeviceMemoryIndex;

    vkres = vkAllocateMemory(device, &allocInfo, nullptr, &FontTextureMemory);
    vkBindImageMemory(device, FontTexture, FontTextureMemory, 0);

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
    
    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = bmpWidth * bmpHeight;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(device, &stagingBufferInfo, nullptr, &stagingBuffer);

    VkMemoryRequirements stagingBufferRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferRequirements);

    VkMemoryAllocateInfo stagingBufferAllocInfo{};
    stagingBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingBufferAllocInfo.allocationSize = stagingBufferRequirements.size;
    stagingBufferAllocInfo.memoryTypeIndex = pro::GetMemoryType(physDevice, stagingBufferRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkres = vkAllocateMemory(device, &stagingBufferAllocInfo, nullptr, &stagingBufferMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    assert(vkres == VK_SUCCESS);

    void *stagingBufferMapped;
    if (vkMapMemory(device, stagingBufferMemory, 0, bmpWidth * bmpHeight, 0, &stagingBufferMapped) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to map staging buffer memory!");
    }
    memcpy(stagingBufferMapped, buffer, bmpWidth * bmpHeight);
    vkUnmapMemory(device, stagingBufferMemory);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.image = FontTexture;

    vkres = vkCreateImageView(device, &viewInfo, nullptr, &FontTextureView);
    if (vkres != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image view" + std::to_string(vkres));
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    vkres = vkCreateSampler(device, &samplerInfo, nullptr, &FontTextureSampler);
    assert(vkres == VK_SUCCESS);

    // stager.join();
    // Image layout transition : UNDEFINED -> TRANSFER_DST_OPTIMAL
    {
        const VkCommandBuffer cmd = bootstrap::BeginSingleTimeCommands(device, commandPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = FontTexture;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        bootstrap::EndSingleTimeCommands(device, cmd, queue, commandPool);
    }

    const VkCommandBuffer cmd = bootstrap::BeginSingleTimeCommands(device, commandPool);

    VkBufferImageCopy region{};
    region.imageExtent = {bmpWidth, bmpHeight, 1};
    region.imageOffset = {0, 0, 0};
    region.bufferOffset = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.mipLevel = 0;

    vkCmdCopyBufferToImage(cmd, stagingBuffer, FontTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    bootstrap::EndSingleTimeCommands(device, cmd, queue, commandPool);

    // Image layout transition : TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    {
        const VkCommandBuffer cmd = bootstrap::BeginSingleTimeCommands(device, commandPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = FontTexture;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        bootstrap::EndSingleTimeCommands(device, cmd, queue, commandPool);
    }

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    unifiedBuffer = 0;
    mem = 0;

    /*
    *   DESCRIPTOR SET CREATION
    */
    // VkDescriptorSetLayoutBinding bindings[] = {
    //     // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
    //     { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_NULL_HANDLE },
    // };

    // constexpr VkDescriptorPoolSize poolSizes[] = {
    //     // type; descriptorCount;
    //     { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
    // };

    // VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    // setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    // setLayoutInfo.pBindings = bindings;
    // setLayoutInfo.bindingCount = std::size(bindings);
    // if (vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
    //     throw std::runtime_error("Failed to create descriptor set layout!");
    // }

    // VkDescriptorPoolCreateInfo poolInfo{};
    // poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    // poolInfo.pPoolSizes = poolSizes;
    // poolInfo.poolSizeCount = std::size(poolSizes);
    // poolInfo.maxSets = 1;
    // if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
    //     throw std::runtime_error("Failed to create descriptor pool!");
    // }

    // VkDescriptorSetAllocateInfo setAllocInfo{};
    // setAllocInfo.descriptorPool = m_descriptorPool;
    // setAllocInfo.descriptorSetCount = 1;
    // setAllocInfo.pSetLayouts = &m_descriptorSetLayout;
    // if (vkAllocateDescriptorSets(device, &setAllocInfo, &m_descriptorSet) != VK_SUCCESS) {
    //     throw std::runtime_error("Failed to allocate descriptor sets!");
    // }
}

void TextRenderer::Render(const char *pText, VkCommandBuffer cmd, VkPipelineLayout test, vec2 &pos)
{
    u16 indexOffset = 0;
    float x = 0.0f;
    float y = 0.0f;
    float fbmpHeight = static_cast<float>(bmpHeight);
    float invBmpWidth = 1 / static_cast<float>(bmpWidth);
    float scale = 1.0f;

    size_t thisFrameHash;
    boost::hash_combine(thisFrameHash, pText);
    
    u32 len = strlen(pText);

    std::vector<u16> indices(len * 6);

    if (thisFrameHash != prevFrameHash)
    {
        // Reallocate buffers, stream data to GPU

        std::vector<vec4> vertices(len * 4);

        float width, height;
        GetTextSize(pText, &width, &height);

        for (u32 i = 0; i < len; i++)
        {
            char c = pText[i];
            Character &ch = Characters[c];

            float xpos = x + ch.bearing.x * scale;
            float ypos = y + (height - ch.bearing.y) * scale; // (character.Size.y - character.Bearing.y)* scale;

            float scaledWidth =  static_cast<float>(ch.size.x * scale);
            float rawHeight =    static_cast<float>(ch.size.y);
            float scaledHeight = rawHeight * scale;
            float u0 =           ch.offset * invBmpWidth;
            float u1 =           (ch.offset + ch.size.x) * invBmpWidth;
            float v =            rawHeight / fbmpHeight;

            float __TEXTURE_POSITION_EPSILON__ = 0.000f;
            vec4 chvertices[4] = {
                vec4(xpos              , ypos               , u0 + __TEXTURE_POSITION_EPSILON__, 0.0f),
                vec4(xpos + scaledWidth, ypos               , u1 - __TEXTURE_POSITION_EPSILON__, 0.0f),
                vec4(xpos + scaledWidth, ypos + scaledHeight, u1 - __TEXTURE_POSITION_EPSILON__, v - __TEXTURE_POSITION_EPSILON__),
                vec4(xpos              , ypos + scaledHeight, u0 + __TEXTURE_POSITION_EPSILON__, v - __TEXTURE_POSITION_EPSILON__),
            };

            // This makes more sense to me than copying over the elements one by one
            // I know it won't be beneficial to performance. I do not care.
            memcpy(vertices.data() + i * 4, chvertices, sizeof(chvertices));

            const u16 setIndices[] = {
                // compiler hates me
                (u16)(indexOffset + 0), (u16)(indexOffset + 1), (u16)(indexOffset + 2),
                (u16)(indexOffset + 2), (u16)(indexOffset + 3), (u16)(indexOffset + 0)};

            memcpy(indices.data() + i * 6, setIndices, sizeof(setIndices));

            indexOffset += 4;
            x += (ch.advance >> 6) * scale;
        }

        if (vertices.size() == 0)
            return;

        const bool unifiedBufferNotNull = unifiedBuffer != VK_NULL_HANDLE;
        const bool memNotNull = mem != VK_NULL_HANDLE;
        if (unifiedBuffer || memNotNull)
        {
            vkDeviceWaitIdle(m_device);

            if (unifiedBufferNotNull)
                vkDestroyBuffer(m_device, unifiedBuffer, nullptr);
            if (memNotNull)
                vkFreeMemory(m_device, mem, nullptr);
        }

        const u32 vertexByteSize = vertices.size() * sizeof(vec4);
        const u32 indexByteSize = indices.size() * sizeof(u16);

        VkBufferCreateInfo VBbufferInfo = {};
        VBbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        VBbufferInfo.size = vertexByteSize + indexByteSize;
        VBbufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        VBbufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_device, &VBbufferInfo, nullptr, &unifiedBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create vertex buffer!");
        }

        VkMemoryRequirements VBmemoryRequirements;
        vkGetBufferMemoryRequirements(m_device, unifiedBuffer, &VBmemoryRequirements);

        // This just rounds vertexByteSize to the nearest aligned size
        ibOffset = (vertexByteSize + VBmemoryRequirements.alignment - 1) & ~(VBmemoryRequirements.alignment - 1);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = VBmemoryRequirements.size;
        allocInfo.memoryTypeIndex = bootstrap::GetMemoryType(m_physDevice, VBmemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(m_device, &allocInfo, nullptr, &mem) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(m_device, unifiedBuffer, mem, 0);

        // Use uchar for arithmetic operations instead of void ptr
        uchar* vbMap;
        if (vkMapMemory(m_device, mem, 0, vertexByteSize + indexByteSize, 0, reinterpret_cast<void **>(&vbMap)) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to map vertex buffer memory");
        }

        memcpy(vbMap, vertices.data(), vertexByteSize);
        memcpy(vbMap + vertexByteSize, indices.data(), indexByteSize);

        // Non coherent mapped memory ranges must be flushed
        VkMappedMemoryRange ranges[2]{};
        ranges[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        ranges[0].memory = mem;
        ranges[0].offset = 0;
        ranges[0].size = vertexByteSize;

        ranges[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        ranges[1].memory = mem;
        ranges[1].offset = ibOffset;
        ranges[1].size = indexByteSize;
        if (vkFlushMappedMemoryRanges(m_device, 2, ranges) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to flush mapped memory ranges");
        }

        vkUnmapMemory(m_device, mem);
    }

    VkDeviceSize offsets = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &unifiedBuffer, &offsets);
    vkCmdBindIndexBuffer(cmd, unifiedBuffer, ibOffset, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(cmd, indices.size(), 1, 0, 0, 0);
    prevFrameHash = thisFrameHash;
}

void TextRenderer::GetTextSize(const char *pText, float* width, float* height)
{
    const u32 len = strlen(pText);
    float x = 0.0f;
    float y = 0.0f;

    for (u32 i = 0; i < len; i++)
    {
        char c = pText[i];
        const Character &ch = Characters[c];

        float h = static_cast<float>(ch.size.y);

        x += (ch.advance >> 6);
        y = std::max(y, h);
    }

    *width = x;
    *height = y;
}

// void TextRenderer::CreatePipeline()
// {

//     VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//     pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//     pipelineLayoutInfo.setLayoutCount = 1;
//     pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
//     pipelineLayoutInfo.
//     // pipelineLayoutInfo.
//     if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_layout) != VK_SUCCESS) {
//         throw std::runtime_error("Failed to create pipeline layout!");
//     }

//     VkAttachmentDescription colorAttachmentDescription{};
// 	colorAttachmentDescription.format = VK_FORMAT_B8G8R8A8_SRGB;
// 	colorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
// 	colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
// 	colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
// 	colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
// 	colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
// 	colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
// 	colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

// 	VkAttachmentReference colorAttachmentReference{};
// 	colorAttachmentReference.attachment = 0;
// 	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

// 	VkSubpassDependency dependency{};
// 	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
// 	dependency.dstSubpass = 0;
// 	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
// 	dependency.srcAccessMask = 0;
// 	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
// 	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

// 	VkSubpassDescription subpass{};
// 	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
// 	subpass.colorAttachmentCount = 1;

// 	subpass.pColorAttachments = &colorAttachmentReference;

// 	const VkAttachmentDescription attachments[] =  { colorAttachmentDescription };

//     VkRenderPassCreateInfo renderPassInfo{};
// 	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
// 	renderPassInfo.attachmentCount = 1;
// 	renderPassInfo.pAttachments = attachments;
// 	renderPassInfo.subpassCount = 1;
// 	renderPassInfo.pSubpasses = &subpass;
// 	renderPassInfo.dependencyCount = 1;
// 	renderPassInfo.pDependencies = &dependency;

// 	pro::ResultCheck(vkCreateRenderPass(m_device, &renderPassInfo, VK_NULL_HANDLE, &m_renderPass));

//     constexpr VkVertexInputAttributeDescription attributeDescriptions[] = {
//         // location; binding; format; offset;
//         { 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 },
//     };

//     constexpr VkVertexInputBindingDescription bindingDescriptions[] = {
//         // binding; stride; inputRate
//         { 0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX },
//     };

//     /*
//     *   PIPELINE CREATION
//     */
//     pro::PipelineCreateInfo pcio{};
//     pcio.
// }
