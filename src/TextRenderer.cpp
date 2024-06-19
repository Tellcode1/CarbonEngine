#include "TextRenderer.hpp"
#include "pro.hpp"
#include "Renderer.hpp"
#include "epichelperlib.hpp"
#include <boost/filesystem.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"
#endif

static std::vector<std::string> SplitString(const std::string& text) {
    std::vector<std::string> result;
    std::istringstream iss(text);
    
    std::string line; 
    for (;std::getline(iss, line);)
        result.push_back(line);

    return result;
}

constexpr static inline u32 align(u32 num, u32 align) {
    return (num + align - 1) & ~(align - 1);
}

void TextRendererSingleton::GetTextSize(const NanoFont* font, const std::string_view& pText, f32 *width, f32 *height, f32 scale)
{
    const u32 len = pText.size();
    f32 x = 0;
    f32 y = 0;

    for (u32 i = 0; i < len; i++)
    {
        const char c = pText[i];
        const Character &ch = font->characters[c];

        const vec2 chSize = glm::unpackHalf2x16(ch.size);

        const f32 h = chSize.y * scale;
        const f32 w = ch.advance * scale; // Use advance instead of size for width

        x += w; // Accumulate the advance (width) of each character
        y = std::max(y, h);
    }

    *width = x;
    *height = y;
}

void TextRendererSingleton::Initialize()
{
    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxFontCount, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    constexpr VkDescriptorPoolSize poolSizes[] = {
        // type; descriptorCount;
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxFontCount},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MaxFontCount + 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MaxFontCount},
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.poolSizeCount = array_len(poolSizes);
    poolInfo.maxSets = 2;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create descriptor pool!");
    }

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.pBindings = bindings;
    setLayoutInfo.bindingCount = array_len(bindings);
    if (vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create descriptor set layout!");
    }

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = m_descriptorPool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &m_descriptorSetLayout;
    if (vkAllocateDescriptorSets(device, &setAllocInfo, &m_descriptorSet) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to allocate descriptor sets!");
    }

    const VkDescriptorSetLayoutBinding computeBindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MaxFontCount, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MaxFontCount, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
        { 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
    };

    VkDescriptorSetLayoutCreateInfo computeSetLayoutInfo{};
    computeSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    computeSetLayoutInfo.pBindings = computeBindings;
    computeSetLayoutInfo.bindingCount = array_len(computeBindings);
    if (vkCreateDescriptorSetLayout(device, &computeSetLayoutInfo, nullptr, &m_computeDescriptorSetLayout) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create compute descriptor set layout!");
    }

    VkDescriptorSetAllocateInfo computeSetAllocInfo{};
    computeSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    computeSetAllocInfo.descriptorPool = m_descriptorPool;
    computeSetAllocInfo.descriptorSetCount = 1;
    computeSetAllocInfo.pSetLayouts = &m_computeDescriptorSetLayout;
    if (vkAllocateDescriptorSets(device, &computeSetAllocInfo, &m_computeDescriptorSet) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to allocate compute descriptor sets!");
    }

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &dummySampler) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create sampler!");
    }

    u32 imageWidth, imageHeight;
    u8 *dummyImageData;
    help::Images::LoadFromDisk("../Assets/error.jpeg", 4, &dummyImageData, &imageWidth, &imageHeight);

    help::Images::LoadVulkanImage(
        dummyImageData, imageWidth, imageHeight,
        VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SAMPLE_COUNT_1_BIT, 1,
        &dummyImage, &dummyImageMemory
    );

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = dummyImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &viewInfo, nullptr, &dummyImageView) != VK_SUCCESS)
    {
        LOG_ERROR("Failed to create image view!");
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = dummyImageView;
    imageInfo.sampler = dummySampler;

    VkWriteDescriptorSet writeSets[MaxFontCount] = {};
    for(u32 i = 0; i < MaxFontCount; i++)
    {
        writeSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSets[i].dstSet = m_descriptorSet;
        writeSets[i].dstBinding = 0;
        writeSets[i].dstArrayElement = i;
        writeSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSets[i].descriptorCount = 1;
        writeSets[i].pImageInfo = &imageInfo;
    }
    vkUpdateDescriptorSets(device, MaxFontCount, writeSets, 0, nullptr);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0;
    vkCreateFence(device, &fenceInfo, nullptr, &fence);

    // VkCommandPoolCreateInfo commandPoolInfo{};
    // commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // commandPoolInfo.queueFamilyIndex = Graphics->GraphicsFamilyIndex;
    // commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    // vkCreateCommandPool(device, &commandPoolInfo, nullptr, &m_cmdPool);

    // VkCommandBufferAllocateInfo commandBufferInfo{};
    // commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    // commandBufferInfo.commandPool = m_cmdPool;
    // commandBufferInfo.commandBufferCount = MaxFramesInFlight;
    // vkAllocateCommandBuffers(device, &commandBufferInfo, renderCmd);

    // VkCommandBufferAllocateInfo stagingCopyInfo{};
    // stagingCopyInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // stagingCopyInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    // stagingCopyInfo.commandPool = m_cmdPool;
    // stagingCopyInfo.commandBufferCount = 1;
    // vkAllocateCommandBuffers(device, &stagingCopyInfo, &m_copyBuffer);
    
    TIME_FUNCTION(CreatePipeline());

    Graphics->OnWindowResized.connect([&]() {
        std::cout << "resized pipeline\n";
        RecreatePipeline();
    });
}

void TextRendererSingleton::BeginRender()
{
    // const f32 halfWidth = (f32)Graphics->RenderArea.x / 2.0f;
    // const f32 halfHeight = (f32)Graphics->RenderArea.y / 2.0f;
    
    // projection = glm::ortho(-halfWidth,halfWidth, -halfHeight, halfHeight, 0.0f, 1.0f);
}

void TextRendererSingleton::Render(TextRenderInfo& info, const NanoFont *font)
{
    const auto& draw = info;
    if(draw.line.size() == 0)
        return;

    // textList.emplace_back({
    //     font,
    //     info.scale,
    //     info.x,
    //     info.y,
    //     info.line,
    //     info.horizontal,
    //     info.vertical
    // });
    // return;

    const std::string& text = draw.line;
    const f32 scale = draw.scale;
    const NanoTextAlignmentHorizontal horizontal = draw.horizontal;
    const f32 x = draw.x;
    f32 y = draw.y;

    const std::vector<std::string> splitLines = SplitString(text);

    // switch(draw.vertical)
    // {
    //     case TEXT_VERTICAL_ALIGN_CENTER:
    //         y = y - ((font->bmpHeight * scale) * splitLines.size()) / 2.0f;
    //         break;

    //     case TEXT_VERTICAL_ALIGN_BOTTOM:
    //         y = y - (font->bmpHeight * scale) * splitLines.size();
    //         break;

    //     case TEXT_VERTICAL_ALIGN_TOP:
    //         break;

    //     case TEXT_VERTICAL_ALIGN_BASELINE:
    //         y = y - (font->bmpHeight * scale) * splitLines.size();
    //         break;

    //     default:
    //         __builtin_unreachable();
    //         LOG_ERROR("Invalid vertical alignment. Specified (int)%u. (Implement?)\n", draw.vertical);
    //         break;
    // }

    f32 currY = y;
    u32 currYIndex = 0;

    u32 renderableCharCount = 0;
    for(const auto& c : draw.line)
        if(c != '\n')
            renderableCharCount++;

    f32 yPositions[renderableCharCount];

    for(const auto& line : splitLines) {
        for(const auto& c : line)
            if(c == '\n')
                currY += font->bmpHeight * scale;
        yPositions[currYIndex++] = currY;
        currY += font->bmpHeight * scale;
    }

    // vertices.resize(renderableCharCount);

    for (u32 i = 0; i < splitLines.size(); i++) {
        const std::string line = splitLines[i];
        RenderLine(x, yPositions[i], line, horizontal, scale, font);
    }

    // Coalesce draw commands.
    if(!drawList.empty() && drawList.back().font->fontIndex == font->fontIndex)
        drawList.back().indexCount += renderableCharCount * 6;
    else
    {
        Draw drawCmd{};
        drawCmd.font = font;
        drawCmd.indexCount = renderableCharCount * 6;
        drawCmd.vertexOffset = std::max((int)(vertices.size() - renderableCharCount * 4), 0);
        drawCmd.scale = scale;
        drawList.push_back(drawCmd);
    }
}

// void TextRendererSingleton::RenderString(const amongus &amog)
// {
//     const std::vector<std::string> split = SplitString(amog.line);
//     for(const auto& line : split) {
//         RenderLine();
//     }
// }

void TextRendererSingleton::RenderLine(const f32 x, f32 y, const std::string_view line, const NanoTextAlignmentHorizontal horizontal, const f32 scale, const NanoFont* font)
{
    f32 lineX = 0.0f;
    f32 totalWidth, height;
    GetTextSize(font, std::string(line), &totalWidth, &height, scale);

    switch (horizontal)
    {
    case TEXT_HORIZONTAL_ALIGN_LEFT:
        // x = x;
        lineX = x;
        break;
    case TEXT_HORIZONTAL_ALIGN_CENTER:
        lineX = x - totalWidth / 2.0f;
        break;
    case TEXT_HORIZONTAL_ALIGN_RIGHT:
        lineX = x - totalWidth;
        break;
    default:
        __builtin_unreachable();
        LOG_ERROR("Invalid horizontal alignment. Specified (int)%u. (Implement?)\n", horizontal);
        break;
    }
    __builtin_nanf32("sussy");
    vec4 vv[line.size() * 4];
    u16 ii[line.size() * 6];

    for (u32 i = 0; i < line.size(); i++)
    {
        const char& c = line[i];
        const Character &ch = font->characters[c];

        const vec2 chBearing = glm::unpackHalf2x16(ch.bearing);
        const vec2 chSize = glm::unpackHalf2x16(ch.size);

        const f32 xpos = lineX + chBearing.x * scale;
        const f32 ypos = y + (font->bmpHeight - chBearing.y - font->lineHeight) * scale;

        const f32 scaledWidth = chSize.x * scale;
        const f32 scaledHeight = chSize.y * scale;

        const f32 halfWidth = (f32)Graphics->RenderExtent.width / 2.0f;
        const f32 halfHeight = (f32)Graphics->RenderExtent.height / 2.0f;
        const glm::mat4 projection = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.0f, 1.0f);

        u32 vertexIndex = i * 4;
        vv[vertexIndex + 0] = vec4(xpos              , ypos               , font->texturePositions[(c * 4) + 0]);
        vv[vertexIndex + 1] = vec4(xpos + scaledWidth, ypos               , font->texturePositions[(c * 4) + 1]);
        vv[vertexIndex + 2] = vec4(xpos + scaledWidth, ypos + scaledHeight, font->texturePositions[(c * 4) + 2]);
        vv[vertexIndex + 3] = vec4(xpos              , ypos + scaledHeight, font->texturePositions[(c * 4) + 3]);

        const u32 indexOffset = charsDrawn * 4;
        const u32 indexIndex = i * 6;

        // Calculate indices for the current character
        ii[indexIndex + 0] = static_cast<u16>(indexOffset + 0);
        ii[indexIndex + 1] = static_cast<u16>(indexOffset + 1);
        ii[indexIndex + 2] = static_cast<u16>(indexOffset + 2);
        ii[indexIndex + 3] = static_cast<u16>(indexOffset + 2);
        ii[indexIndex + 4] = static_cast<u16>(indexOffset + 3);
        ii[indexIndex + 5] = static_cast<u16>(indexOffset + 0);
        
        charsDrawn++;
        lineX += ch.advance * scale;
    }
    vertices.insert(vertices.end(), vv, vv + (line.size() * 4));
    indices.insert(indices.end(), ii, ii + (line.size() * 6));
}

// void TextRendererSingleton::RenderLine(const f32 x, f32 *y, const std::string_view line, const NanoTextAlignmentHorizontal horizontal, const f32 scale, const NanoSDFFont *font)
// {
//         f32 lineX = 0.0f;
//     f32 totalWidth, height;
//     GetTextSize(font, std::string(line), &totalWidth, &height, scale);

//     switch (horizontal)
//     {
//     case TEXT_HORIZONTAL_ALIGN_LEFT:
//         // x = x;
//         lineX = x;
//         break;
//     case TEXT_HORIZONTAL_ALIGN_CENTER:
//         lineX = x - totalWidth / 2.0f;
//         break;
//     case TEXT_HORIZONTAL_ALIGN_RIGHT:
//         lineX = x - totalWidth;
//         break;
//     default:
//         LOG_ERROR("Invalid horizontal alignment. Specified (int)%u. (Implement?)\n", horizontal);
//         break;
//     }
    
//     vec4 vv[line.size() * 4];
//     u16 ii[line.size() * 6];

//     for (u32 i = 0; i < line.size(); i++)
//     {
//         const char& c = line[i];
//         const bmchar &ch = font->characters[c];

//         if(c == '\n') {
//             // Newline, pushes the character to the next line
//             *y += font->bmpHeight * scale;
//             lineX = 0.0f;
//             continue;
//         }

//         const vec2 chBearing = glm::unpackHalf2x16(ch.bearing);
//         const vec2 chSize = glm::unpackHalf2x16(ch.size);

//         const f32 xpos = lineX + chBearing.x * scale;
//         const f32 ypos = *y + (font->bmpHeight - chBearing.y - font->lineHeight) * scale;

//         const f32 scaledWidth = chSize.x * scale;
//         const f32 scaledHeight = chSize.y * scale;

//         const u32 vertexIndex = i * 4;
//         vv[vertexIndex + 0] = vec4(xpos              , ypos               , font->texturePositions[(c * 4) + 0]);
//         vv[vertexIndex + 1] = vec4(xpos + scaledWidth, ypos               , font->texturePositions[(c * 4) + 1]);
//         vv[vertexIndex + 2] = vec4(xpos + scaledWidth, ypos + scaledHeight, font->texturePositions[(c * 4) + 2]);
//         vv[vertexIndex + 3] = vec4(xpos              , ypos + scaledHeight, font->texturePositions[(c * 4) + 3]);

//         const u32 indexOffset = charsDrawn * 4;
//         const u32 indexIndex = i * 6;

//         // Calculate indices for the current character
//         ii[indexIndex + 0] = static_cast<u16>(indexOffset + 0);
//         ii[indexIndex + 1] = static_cast<u16>(indexOffset + 1);
//         ii[indexIndex + 2] = static_cast<u16>(indexOffset + 2);
//         ii[indexIndex + 3] = static_cast<u16>(indexOffset + 2);
//         ii[indexIndex + 4] = static_cast<u16>(indexOffset + 3);
//         ii[indexIndex + 5] = static_cast<u16>(indexOffset + 0);
        
//         charsDrawn++;
//         lineX += ch.advance * scale;
//     }
//     vertices.insert(vertices.end(), vv, vv + (line.size() * 4));
//     indices.insert(indices.end(), ii, ii + (line.size() * 6));

//     *y += font->bmpHeight * scale;
// }

void TextRendererSingleton::EndRender(VkCommandBuffer cmd, const glm::mat4 &projection, const glm::mat4 &matrix)
{
    if(vertices.size() == 0)
        return;

    const u32 vertexByteSize = vertices.size() * sizeof(vec4);
    const u32 indexByteSize = indices.size() * sizeof(u16);
    
    ReallocateBuffers(false);

    ibOffset = align(vertexByteSize, alignment);

    uchar *vbMap;
    if (vkMapMemory(device, stagingBufferMemory, 0, allocatedMemorySize, 0, reinterpret_cast<void **>(&vbMap)) != VK_SUCCESS)
    {
        LOG_ERROR("TextRenderer::Failed to map staging buffer memory");
    }

    memcpy(vbMap, vertices.data(), vertexByteSize);
    memcpy(vbMap + ibOffset, indices.data(), indexByteSize);

    vkUnmapMemory(device, stagingBufferMemory);

    const VkCommandBuffer copyCmd = help::Commands::BeginSingleTimeCommands();

    VkBufferCopy copy{};
    copy.dstOffset = 0;
    copy.srcOffset = 0;
    copy.size = allocatedMemorySize;
    vkCmdCopyBuffer(copyCmd, stagingBuffer, unifiedBuffer, 1, &copy);

    help::Commands::EndSingleTimeCommands(copyCmd);

    constexpr VkDeviceSize VBOffset = 0;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);
    vkCmdBindVertexBuffers(cmd, 0, 1, &unifiedBuffer, &VBOffset);
    vkCmdBindIndexBuffer(cmd, unifiedBuffer, ibOffset, VK_INDEX_TYPE_UINT16);

    const glm::mat4 mvp = projection * matrix;
    vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &mvp);

    for(const auto& draw : drawList) {
        const u32 textureIndex = draw.font->fontIndex;

        vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), sizeof(u32), (void*)&textureIndex);
        vkCmdDrawIndexed(cmd, draw.indexCount, 1, 0, draw.vertexOffset, 0);
    }

    vertices.clear();
    indices.clear();
    drawList.clear();
    charsDrawn = 0;
}

void TextRendererSingleton::DispatchCompute()
{
    // const auto& cmd = this->computeCmdBuffers[Renderer->currentFrame];
    // dispatchedCompute = true;
    
    // VkCommandBufferBeginInfo beginInfo{};
    // beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    // vkBeginCommandBuffer(cmd, &beginInfo);
    // vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1, &m_computeDescriptorSet, 0, nullptr);
    // vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);
    // vkCmdDispatch(cmd, 128, 1, 1);
    // vkEndCommandBuffer(cmd);

    // VkPipelineStageFlags waitStages[] = {};

    // VkSubmitInfo submitInfo{};
    // submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // submitInfo.pWaitDstStageMask = waitStages;
    // submitInfo.commandBufferCount = 1;
    // submitInfo.pCommandBuffers = &cmd;

    // // Submit command buffer
    // vkQueueSubmit(Graphics->ComputeQueue, 1, &submitInfo, fence);
}

void TextRendererSingleton::CreatePipeline() {
    VkShaderModule vertexShaderModule;
    VkShaderModule fragmentShaderModule;
    VkShaderModule computeShaderModule;

    u32 VertexShaderBinarySize, FragmentShaderBinarySize, ComputeShaderBinarySize;
    help::Files::LoadBinary("./Shaders/vert.text.spv", nullptr, &VertexShaderBinarySize);
    u8 *VertexShaderBinary = new u8[VertexShaderBinarySize];
    help::Files::LoadBinary("./Shaders/vert.text.spv", VertexShaderBinary, &VertexShaderBinarySize);

    help::Files::LoadBinary("./Shaders/frag.text.spv", nullptr, &FragmentShaderBinarySize);
    u8 *FragmentShaderBinary = new u8[FragmentShaderBinarySize];
    help::Files::LoadBinary("./Shaders/frag.text.spv", FragmentShaderBinary, &FragmentShaderBinarySize);

    help::Files::LoadBinary("./Shaders/comp.text.spv", nullptr, &ComputeShaderBinarySize);
    u8 *ComputeShaderBinary = new u8[ComputeShaderBinarySize];
    help::Files::LoadBinary("./Shaders/comp.text.spv", ComputeShaderBinary, &ComputeShaderBinarySize);

    VkShaderModuleCreateInfo vertexShaderModuleInfo{};
    vertexShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderModuleInfo.codeSize = VertexShaderBinarySize;
    vertexShaderModuleInfo.pCode = reinterpret_cast<const u32*>(VertexShaderBinary);
    if (vkCreateShaderModule(device, &vertexShaderModuleInfo, nullptr, &vertexShaderModule) != VK_SUCCESS) {
        LOG_ERROR("TextRenderer::Failed to create vertex shader module!");
    }

    VkShaderModuleCreateInfo fragmentShaderModuleInfo{};
    fragmentShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentShaderModuleInfo.codeSize = FragmentShaderBinarySize;
    fragmentShaderModuleInfo.pCode = reinterpret_cast<const u32*>(FragmentShaderBinary);
    if (vkCreateShaderModule(device, &fragmentShaderModuleInfo, nullptr, &fragmentShaderModule) != VK_SUCCESS) {
        LOG_ERROR("TextRenderer::Failed to create fragment shader module!");
    }

    delete[] VertexShaderBinary;
    delete[] FragmentShaderBinary;

    const std::vector<VkPushConstantRange> pushConstantRanges = {
        // stageFlags; offset; size;
        {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) + sizeof(u32)},
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        LOG_ERROR("TextRenderer::Failed to create pipeline layout!");
    }

    const std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {
        // location; binding; format; offset;
        {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
    };

    const std::vector<VkVertexInputBindingDescription> bindingDescriptions = {
        // binding; stride; inputRate
        {0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX},
    };

    const std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {m_descriptorSetLayout};

    VkPipelineShaderStageCreateInfo vertexShaderInfo{};
    vertexShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderInfo.module = vertexShaderModule;
    vertexShaderInfo.pName = "main";

    VkSpecializationMapEntry mapEntry{};
    mapEntry.constantID = 0;
    mapEntry.offset = 0;
    mapEntry.size = sizeof(u32);

    const u32 specData = MaxFontCount;

    VkSpecializationInfo specInfo{};
    specInfo.dataSize = sizeof(u32);
    specInfo.mapEntryCount = 1;
    specInfo.pMapEntries = &mapEntry;
    specInfo.pData = &specData;

    VkPipelineShaderStageCreateInfo fragmentShaderInfo{};
    fragmentShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderInfo.module = fragmentShaderModule;
    fragmentShaderInfo.pName = "main";
    fragmentShaderInfo.pSpecializationInfo = &specInfo;

    const std::vector<VkPipelineShaderStageCreateInfo> shaders = {
        vertexShaderInfo, fragmentShaderInfo
    };

    /*
    *   PIPELINE CREATION
    */

    pro::PipelineBlendState blendState(pro::BlendPreset::PRO_BLEND_PRESET_ALPHA);

    pro::PipelineCreateInfo pc{};
    pc.format = Graphics->SwapChainImageFormat;
    pc.subpass = 0;
    pc.renderPass = Graphics->GlobalRenderPass;
    pc.pipelineLayout = m_pipelineLayout;
    pc.pAttributeDescriptions = &attributeDescriptions;
    pc.pBindingDescriptions = &bindingDescriptions;
    pc.pDescriptorLayouts = &descriptorSetLayouts;
    pc.pShaderCreateInfos = &shaders;
    pc.pPushConstants = &pushConstantRanges;
    pc.pBlendState = &blendState;
    pc.extent.width = Graphics->RenderExtent.width;
    pc.extent.height = Graphics->RenderExtent.height;
    pc.pShaderCreateInfos = &shaders;
    pro::CreateGraphicsPipeline(device, &pc, &m_pipeline, PIPELINE_CREATE_FLAGS_ENABLE_BLEND);

    vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(device, fragmentShaderModule, nullptr);

    delete[] ComputeShaderBinary;

    /* Compute shader create Info */
    
    // VkShaderModuleCreateInfo computeShderModuleInfo{};
    // computeShderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    // computeShderModuleInfo.codeSize = sizeof(ComputeShaderBinary);
    // computeShderModuleInfo.pCode = reinterpret_cast<const u32*>(ComputeShaderBinary);
    // if (vkCreateShaderModule(device, &computeShderModuleInfo, nullptr, &computeShaderModule) != VK_SUCCESS) {
    //     LOG_ERROR("TextRenderer::Failed to create fragment shader module!");
    // }

    // VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    // computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    // computeShaderStageInfo.module = computeShaderModule;
    // computeShaderStageInfo.pName = "main";
    
    // VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    // pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // pipelineLayoutCreateInfo.setLayoutCount = 1;
    // pipelineLayoutCreateInfo.pSetLayouts = &m_computeDescriptorSetLayout;
    // if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &m_computePipelineLayout) != VK_SUCCESS) {
    //     LOG_ERROR("TextRenderer::Failed to create pipeline layout!\n");
    // }

    // Create compute pipeline
    // VkComputePipelineCreateInfo computePipelineCreateInfo{};
    // computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    // computePipelineCreateInfo.stage = computeShaderStageInfo;
    // computePipelineCreateInfo.layout = m_computePipelineLayout;
    // computePipelineCreateInfo.basePipelineHandle = m_pipeline;
    
    // if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_computePipeline) != VK_SUCCESS) {
    //     LOG_ERROR("TextRenderer::Failed to create compute pipeline!\n");
    // }

    // vkDestroyShaderModule(device, computeShaderModule, nullptr);
}

void TextRendererSingleton::RecreatePipeline()
{
    vkDeviceWaitIdle(device);

    vkDestroyPipeline(device, m_pipeline, nullptr);
    vkDestroyPipeline(device, m_computePipeline, nullptr);

   
    vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
    vkDestroyPipelineLayout(device, m_computePipelineLayout, nullptr);

    CreatePipeline();
}

void TextRendererSingleton::ReallocateBuffers(bool shrinkToFit)
{

    const u32 vertexByteSize = vertices.size() * sizeof(vec4);
    const u32 indexByteSize = indices.size() * sizeof(u16);
    
    const u32 bufferSize = vertexByteSize + indexByteSize;

    // If the buffer is this much larger than needed, make it smaller.
    constexpr f32 BufferSizeDownscale = 2;
    // Allocate this much times more than we need to avoid further allocations. 2 is good enough.
    constexpr f32 BufferSizeUpscale = 1.5;

    static_assert(BufferSizeUpscale < BufferSizeDownscale && "Can cause undefined behaviour due to upscaling "
                                                             "the buffer one frame and immediately downscaling it in the other");

    const bool needLargerBuffer = bufferSize > allocatedMemorySize;
    const bool needSmallerBuffer = bufferSize < allocatedMemorySize / BufferSizeDownscale;

    if (bufferSize == 0)
        return;

    if (needLargerBuffer || needSmallerBuffer || (shrinkToFit && allocatedMemorySize != bufferSize))
    {
        if (shrinkToFit)
            allocatedMemorySize = bufferSize;

        else if (needLargerBuffer)
            allocatedMemorySize = bufferSize * BufferSizeUpscale;

        else if (needSmallerBuffer)
            allocatedMemorySize = std::max(static_cast<f32>(bufferSize), allocatedMemorySize / BufferSizeDownscale);

        vkDeviceWaitIdle(device);

        vkDestroyBuffer(device, unifiedBuffer, nullptr);
        vkFreeMemory(device, unifiedBufferMemory, nullptr);

        help::Buffers::CreateBuffer(
            allocatedMemorySize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &unifiedBuffer, &unifiedBufferMemory
        );

        help::Buffers::CreateBuffer(
            allocatedMemorySize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &stagingBuffer, &stagingBufferMemory
        );

        VkMemoryRequirements UBmemoryRequirements;
        vkGetBufferMemoryRequirements(device, unifiedBuffer, &UBmemoryRequirements);
        alignment = UBmemoryRequirements.alignment;
    }
}

void TextRendererSingleton::LoadFont(const char* path, u32 pixelSizes, NanoFont* dstFont)
{
    constexpr u32 Padding = 2;

    FT_Library lib;
    FT_Face face;

    if (FT_Init_FreeType(&lib))
    {
        LOG_ERROR("TextRenderer::Failed to initialize FreeType library\n");
    }

    if (FT_New_Face(lib, path, 0, &face))
    {
        LOG_ERROR("TextRenderer::Failed to load font file: %s\n", path);
        FT_Done_FreeType(lib);
    }

    FT_Set_Pixel_Sizes(face, 0, pixelSizes);

    auto& bmpWidth = dstFont->bmpWidth;
    auto& bmpHeight = dstFont->bmpHeight;
    bmpWidth = 0, bmpHeight = 0;

    vec2 chSizes[ 128 ];
    f32 chOffsets[ 128 ];

    for (uchar c = 0; c < 128; ++c) {
        auto res = FT_Load_Char(face, c, FT_LOAD_BITMAP_METRICS_ONLY);
        assert(res == 0);

        chSizes[c] = glm::vec2(
            face->glyph->metrics.width / 64.0f + 2 * Padding,
            face->glyph->metrics.height / 64.0f + 2 * Padding
        );

        Character character = {
            .size = glm::packHalf2x16(
                glm::vec2(
                    face->glyph->metrics.width / 64.0f + 2 * Padding,
                    face->glyph->metrics.height / 64.0f + 2 * Padding
                )
            ),
            .bearing = glm::packHalf2x16(glm::vec2(face->glyph->bitmap_left, face->glyph->bitmap_top)),
            .advance = face->glyph->advance.x / 64.0f,
        };
        dstFont->characters[c] = character;

        bmpHeight = std::max(bmpHeight, face->glyph->bitmap.rows + 2 * Padding);
        bmpWidth += face->glyph->bitmap.width + 2 * Padding;
    }

    u8* buffer = new u8[bmpWidth * bmpHeight];
    memset(buffer, 0, bmpWidth * bmpHeight);

    u32 xpos = 0;
    for (uchar c = 0; c < 128; c++)
    {
        const vec2 chSize = chSizes[c];

        chOffsets[c] = xpos;

        const u32 width = chSize.x - 2 * Padding;
        const u32 height = chSize.y - 2 * Padding;

        if (width > 0)
        {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                LOG_ERROR("TextRenderer::Failed to load glyph: %c\n", c);
                continue;
            }

            uchar const *charDataBuffer = face->glyph->bitmap.buffer;
            assert(charDataBuffer != nullptr);

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
                memcpy(
                    buffer + xpos + Padding + (row + Padding) * bmpWidth,
                    charDataBuffer + row * width,
                    width
                );
            }
        }

        xpos += width + 2 * Padding;
    };

    const f32 invBmpWidth = 1.0f / static_cast<f32>(bmpWidth);
    const f32 invBmpHeight = 1.0f / static_cast<f32>(bmpHeight);
    
    #pragma omp parallel for
    for(uchar i = 0; i < 128; i++)
    {
        const f32 chOffset = chOffsets[i];
        const vec2 chSize = chSizes[i];

        if(chSize.x == 0 || chSize.y == 0)
            continue;

        const f32 u0 = chOffset * invBmpWidth;
        const f32 u1 = (chOffset + chSize.x) * invBmpWidth;
        const f32 v0 = 0.0f;
        const f32 v1 = chSize.y * invBmpHeight;

        const u32 index = i * 4;
        dstFont->texturePositions[index + 0] = glm::vec2(u0, v0);
        dstFont->texturePositions[index + 1] = glm::vec2(u1, v0);
        dstFont->texturePositions[index + 2] = glm::vec2(u1, v1);
        dstFont->texturePositions[index + 3] = glm::vec2(u0, v1);
    }

    assert(bmpWidth != 0 && bmpHeight != 0);

    dstFont->lineHeight = face->height / 64.0f;

    const i32 ftres = FT_Done_Face(face) && FT_Done_FreeType(lib);
    assert(ftres == 0);

    dstFont->mipLevels = static_cast<u8>(floorf(log2f(bmpWidth) / 2.0f) + 1);

    help::Images::LoadVulkanImage(
        buffer, bmpWidth, bmpHeight,
        VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_SAMPLE_COUNT_1_BIT, dstFont->mipLevels,
        &dstFont->fontTexture, &dstFont->fontTextureMemory
    );

    delete[] buffer;

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = dstFont->fontTexture;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = dstFont->mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewInfo, nullptr, &dstFont->fontTextureView);

    // VkFormatProperties formatProperties;
    // vkGetPhysicalDeviceFormatProperties(physDevice, VK_FORMAT_R8_UNORM, &formatProperties);

    // assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

    const VkCommandBuffer cmd = help::Commands::BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = dstFont->fontTexture;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    i32 mipWidth = bmpWidth;
    i32 mipHeight = bmpHeight;

    for(u32 i = 1; i < dstFont->mipLevels; i++)
    {
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.subresourceRange.baseMipLevel = i - 1;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(
            cmd,
            dstFont->fontTexture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstFont->fontTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR
        );

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = dstFont->mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    help::Commands::EndSingleTimeCommands(cmd);

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<f32>(dstFont->mipLevels);
    samplerInfo.mipLodBias = 0.0f;

    if(vkCreateSampler(device, &samplerInfo, nullptr, &dstFont->sampler) != VK_SUCCESS) {
        LOG_ERROR("Failed to create font texture sampler!\n");
    }

    if(currFontIndex >= MaxFontCount) {
        LOG_ERROR("TextRenderer::unsupported. can't handle more than %u fonts.\n", MaxFontCount);
        abort();
    }
    dstFont->fontIndex = currFontIndex;

    VkDescriptorImageInfo fontImageInfo{};
    fontImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    fontImageInfo.imageView = dstFont->fontTextureView;
    fontImageInfo.sampler = dstFont->sampler;

    VkWriteDescriptorSet writeSet{};
    writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSet.dstSet = m_descriptorSet;
    writeSet.dstBinding = 0;
    writeSet.dstArrayElement = dstFont->fontIndex;
    writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSet.descriptorCount = 1;
    writeSet.pImageInfo = &fontImageInfo;
    vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);

    currFontIndex++;
}

// std::future<NanoFont> TextRendererSingleton::LoadFont(const char *path, u32 pixelSizes)
// {
//     std::promise<NanoFont> promise;
//     std::future<NanoFont> future = promise.get_future();
//     std::thread(
//         [&]() {
//             NanoFont font{};
//             LoadFont(path, pixelSizes, &font);
//             promise.set_value(font);
//         }, path, pixelSizes
//     ).detach();
//     return future;
// }

void TextRendererSingleton::Destroy(NanoFont *obj)
{
    vkDeviceWaitIdle(device);
    
    vkDestroyImage(device, obj->fontTexture, nullptr);
    vkDestroyImageView(device, obj->fontTextureView, nullptr);
    vkFreeMemory(device, obj->fontTextureMemory, nullptr);

    VkDescriptorImageInfo fontImageInfo{};
    fontImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    fontImageInfo.sampler = VK_NULL_HANDLE;
    fontImageInfo.imageView = VK_NULL_HANDLE;

    VkWriteDescriptorSet writeSet{};
    writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSet.dstSet = m_descriptorSet;
    writeSet.dstBinding = 0;
    writeSet.dstArrayElement = obj->fontIndex;
    writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSet.descriptorCount = 1;
    writeSet.pImageInfo = &fontImageInfo;
    vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);

    currFontIndex--;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
