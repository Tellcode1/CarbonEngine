#include "TextRenderer.hpp"
#include "pro.hpp"
#include <omp.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

// TODO: Convert the render function to a "submit" function. Then add a stream data to GPU function and finally a draw function.
struct DestroyBufferData
{
    VkDevice device;
    VkBuffer buff;
    VkDeviceMemory mem;
};
int DestroyBuffers(void *ptr)
{
    auto data = reinterpret_cast<DestroyBufferData *>(ptr);

    if (vkDeviceWaitIdle(data->device) != VK_SUCCESS)
    {
        return -1;
    }
    vkDestroyBuffer(data->device, data->buff, nullptr);
    vkFreeMemory(data->device, data->mem, nullptr);

    return 0;
}

TextRenderer::TextRenderer()
{
}
/*
#version 450 core

layout(location=0) in
vec4 verticesAndTexCoord; // <vec2 vertices, vec2 texCoord>

layout(location=0) out
vec2 FragTexCoord;

layout(push_constant) uniform PushConstantData {
    mat4 model;
};

void main() {
    gl_Position = model * vec4(verticesAndTexCoord.xy, 0.0, 1.0);
    FragTexCoord = verticesAndTexCoord.zw;
}
*/
constexpr static u32 VertexShaderBinary[] =
{
    // 1112.0.0
	0x07230203,0x00010000,0x0008000b,0x00000028,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0008000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000d,0x00000018,0x00000025,
	0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,0x00000000,0x00060005,
	0x0000000b,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x0000000b,0x00000000,
	0x505f6c67,0x7469736f,0x006e6f69,0x00070006,0x0000000b,0x00000001,0x505f6c67,0x746e696f,
	0x657a6953,0x00000000,0x00070006,0x0000000b,0x00000002,0x435f6c67,0x4470696c,0x61747369,
	0x0065636e,0x00070006,0x0000000b,0x00000003,0x435f6c67,0x446c6c75,0x61747369,0x0065636e,
	0x00030005,0x0000000d,0x00000000,0x00070005,0x00000011,0x68737550,0x736e6f43,0x746e6174,
	0x61746144,0x00000000,0x00050006,0x00000011,0x00000000,0x65646f6d,0x0000006c,0x00030005,
	0x00000013,0x00000000,0x00070005,0x00000018,0x74726576,0x73656369,0x54646e41,0x6f437865,
	0x0064726f,0x00060005,0x00000025,0x67617246,0x43786554,0x64726f6f,0x00000000,0x00050048,
	0x0000000b,0x00000000,0x0000000b,0x00000000,0x00050048,0x0000000b,0x00000001,0x0000000b,
	0x00000001,0x00050048,0x0000000b,0x00000002,0x0000000b,0x00000003,0x00050048,0x0000000b,
	0x00000003,0x0000000b,0x00000004,0x00030047,0x0000000b,0x00000002,0x00040048,0x00000011,
	0x00000000,0x00000005,0x00050048,0x00000011,0x00000000,0x00000023,0x00000000,0x00050048,
	0x00000011,0x00000000,0x00000007,0x00000010,0x00030047,0x00000011,0x00000002,0x00040047,
	0x00000018,0x0000001e,0x00000000,0x00040047,0x00000025,0x0000001e,0x00000000,0x00020013,
	0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,
	0x00000007,0x00000006,0x00000004,0x00040015,0x00000008,0x00000020,0x00000000,0x0004002b,
	0x00000008,0x00000009,0x00000001,0x0004001c,0x0000000a,0x00000006,0x00000009,0x0006001e,
	0x0000000b,0x00000007,0x00000006,0x0000000a,0x0000000a,0x00040020,0x0000000c,0x00000003,
	0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000003,0x00040015,0x0000000e,0x00000020,
	0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040018,0x00000010,0x00000007,
	0x00000004,0x0003001e,0x00000011,0x00000010,0x00040020,0x00000012,0x00000009,0x00000011,
	0x0004003b,0x00000012,0x00000013,0x00000009,0x00040020,0x00000014,0x00000009,0x00000010,
	0x00040020,0x00000017,0x00000001,0x00000007,0x0004003b,0x00000017,0x00000018,0x00000001,
	0x00040017,0x00000019,0x00000006,0x00000002,0x0004002b,0x00000006,0x0000001c,0x00000000,
	0x0004002b,0x00000006,0x0000001d,0x3f800000,0x00040020,0x00000022,0x00000003,0x00000007,
	0x00040020,0x00000024,0x00000003,0x00000019,0x0004003b,0x00000024,0x00000025,0x00000003,
	0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,
	0x00000014,0x00000015,0x00000013,0x0000000f,0x0004003d,0x00000010,0x00000016,0x00000015,
	0x0004003d,0x00000007,0x0000001a,0x00000018,0x0007004f,0x00000019,0x0000001b,0x0000001a,
	0x0000001a,0x00000000,0x00000001,0x00050051,0x00000006,0x0000001e,0x0000001b,0x00000000,
	0x00050051,0x00000006,0x0000001f,0x0000001b,0x00000001,0x00070050,0x00000007,0x00000020,
	0x0000001e,0x0000001f,0x0000001c,0x0000001d,0x00050091,0x00000007,0x00000021,0x00000016,
	0x00000020,0x00050041,0x00000022,0x00000023,0x0000000d,0x0000000f,0x0003003e,0x00000023,
	0x00000021,0x0004003d,0x00000007,0x00000026,0x00000018,0x0007004f,0x00000019,0x00000027,
	0x00000026,0x00000026,0x00000002,0x00000003,0x0003003e,0x00000025,0x00000027,0x000100fd,
	0x00010038
};

/*
#version 450 core

layout(location = 0) out vec4 FragColor;

layout(location=0)
in vec2 FragTexCoord;

layout(binding=0)
uniform sampler2D samp;

layout(binding = 1)
uniform sampler2D fontTexture;

void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(fontTexture, FragTexCoord).r);
    sampled.rgb *= sampled.a;
    FragColor = sampled;
}
*/
constexpr static u32 FragmentShaderBinary[] =
{
    // 1112.0.0
	0x07230203,0x00010000,0x0008000b,0x0000001d,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000012,0x0000001a,0x00030010,
	0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00040005,0x00000009,0x706d6173,0x0064656c,0x00050005,0x0000000e,0x746e6f66,
	0x74786554,0x00657275,0x00060005,0x00000012,0x67617246,0x43786554,0x64726f6f,0x00000000,
	0x00050005,0x0000001a,0x67617246,0x6f6c6f43,0x00000072,0x00040005,0x0000001c,0x706d6173,
	0x00000000,0x00040047,0x0000000e,0x00000022,0x00000000,0x00040047,0x0000000e,0x00000021,
	0x00000001,0x00040047,0x00000012,0x0000001e,0x00000000,0x00040047,0x0000001a,0x0000001e,
	0x00000000,0x00040047,0x0000001c,0x00000022,0x00000000,0x00040047,0x0000001c,0x00000021,
	0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
	0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000007,
	0x00000007,0x0004002b,0x00000006,0x0000000a,0x3f800000,0x00090019,0x0000000b,0x00000006,
	0x00000001,0x00000000,0x00000000,0x00000000,0x00000001,0x00000000,0x0003001b,0x0000000c,
	0x0000000b,0x00040020,0x0000000d,0x00000000,0x0000000c,0x0004003b,0x0000000d,0x0000000e,
	0x00000000,0x00040017,0x00000010,0x00000006,0x00000002,0x00040020,0x00000011,0x00000001,
	0x00000010,0x0004003b,0x00000011,0x00000012,0x00000001,0x00040015,0x00000015,0x00000020,
	0x00000000,0x0004002b,0x00000015,0x00000016,0x00000000,0x00040020,0x00000019,0x00000003,
	0x00000007,0x0004003b,0x00000019,0x0000001a,0x00000003,0x0004003b,0x0000000d,0x0000001c,
	0x00000000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
	0x0004003b,0x00000008,0x00000009,0x00000007,0x0004003d,0x0000000c,0x0000000f,0x0000000e,
	0x0004003d,0x00000010,0x00000013,0x00000012,0x00050057,0x00000007,0x00000014,0x0000000f,
	0x00000013,0x00050051,0x00000006,0x00000017,0x00000014,0x00000000,0x00070050,0x00000007,
	0x00000018,0x0000000a,0x0000000a,0x0000000a,0x00000017,0x0003003e,0x00000009,0x00000018,
	0x0004003d,0x00000007,0x0000001b,0x00000009,0x0003003e,0x0000001a,0x0000001b,0x000100fd,
	0x00010038
};

void TextRenderer::GetTextSize(const char *pText, u32 *width, u32 *height, u32* maxWidth)
{
    const u32 len = strlen(pText);
    u32 x = 0;
    u32 y = 0;
    u32 maxX = 0;

    for (u32 i = 0; i < len; i++)
    {
        const char c = pText[i];
        const Character &ch = Characters[c];

        const u32 h = ch.size.y;
        const u32 w = ch.size.x;

        x += ch.advance;
        y = std::max(y, h);
        maxX = std::max(maxX, w);
    }

    *width = x;
    *height = y;
    *maxWidth = maxX;
}

void TextRenderer::Init(VkDevice device, VkPhysicalDevice physDevice, VkQueue queue, VkCommandPool commandPool, VkSemaphore imgSemaphore, const char *fontPath)
{
    m_device = device;
    m_physDevice = physDevice;
    m_queue = queue;

    // Partial descriptor set setup to satisfy pipeline creation
    VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, VK_NULL_HANDLE},
    };

    constexpr VkDescriptorPoolSize poolSizes[] = {
        // type; descriptorCount;
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
    };

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.pBindings = bindings;
    setLayoutInfo.bindingCount = std::size(bindings);
    if (vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.poolSizeCount = std::size(poolSizes);
    poolInfo.maxSets = 1;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool!");
    }

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = m_descriptorPool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &m_descriptorSetLayout;
    if (vkAllocateDescriptorSets(device, &setAllocInfo, &m_descriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    std::thread pipelineCreator([&]()
                                { TIME_FUNCTION(CreatePipeline()); });

    constexpr u32 fontSize = 64;

    FT_Library lib;
    FT_Face face;

    if (FT_Init_FreeType(&lib))
    {
        std::cerr << "Failed to initialize FreeType library" << std::endl;
    }

    if (FT_New_Face(lib, fontPath, 0, &face))
    {
        std::cerr << "Failed to load font file: " << fontPath << std::endl;
        FT_Done_FreeType(lib);
    }

    FT_Set_Pixel_Sizes(face, 0, fontSize);

    bmpWidth = 0, bmpHeight = 0;
    for (uchar c = 0; c < 128; c++)
    {
        auto res = FT_Load_Char(face, c, FT_LOAD_BITMAP_METRICS_ONLY);
        assert(res == 0);

        Character character = {
            uvec2(face->glyph->metrics.width >> 6, face->glyph->metrics.height >> 6),
            ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            bmpWidth,
            static_cast<f32>(face->glyph->advance.x / 64.0f)};

        Characters[c] = character;

        bmpHeight = std::max(bmpHeight, face->glyph->bitmap.rows);
        bmpWidth += face->glyph->bitmap.width;
    };

    assert(bmpWidth != 0 && bmpHeight != 0);

    u8 buffer[bmpWidth * bmpHeight];

    u32 xpos = 0;
    for (uchar c = 0; c < 128; c++)
    {
        Character &ch = Characters[c];

        const u32 width = ch.size.x;
        const u32 height = ch.size.y;

        if (width > 0)
        {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cerr << "Failed to load glyph: " << c << std::endl;
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
                memcpy(buffer + row * bmpWidth + xpos,
                       charDataBuffer + row * width,
                       width
                );
            }
        }

        xpos += width;
    };

    lineHeight = face->height / 64.0f;

    const i32 ftres = FT_Done_Face(face) && FT_Done_FreeType(lib);
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

    VkResult res = vkCreateImage(device, &imageCreateInfo, nullptr, &FontTexture);
    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image" + std::to_string(res));
    }

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, FontTexture, &imageMemoryRequirements);

    const u32 localDeviceMemoryIndex = pro::GetMemoryType(physDevice, imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = imageMemoryRequirements.size;
    allocInfo.memoryTypeIndex = localDeviceMemoryIndex;

    res = vkAllocateMemory(device, &allocInfo, nullptr, &FontTextureMemory);
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

    res = vkAllocateMemory(device, &stagingBufferAllocInfo, nullptr, &stagingBufferMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    assert(res == VK_SUCCESS);

    void* stagingBufferMapped;
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

    res = vkCreateImageView(device, &viewInfo, nullptr, &FontTextureView);
    assert(res == VK_SUCCESS);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    res = vkCreateSampler(device, &samplerInfo, nullptr, &FontTextureSampler);
    assert(res == VK_SUCCESS);

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
     *   DESCRIPTOR SET CREATION (actual creation)
     */

    // constexpr u32 MaxFramesInFlight = 2;

    VkDescriptorImageInfo fontImageInfo{};
    fontImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    fontImageInfo.imageView = FontTextureView;
    fontImageInfo.sampler = FontTextureSampler;

    VkWriteDescriptorSet writeSet{};
    writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSet.dstSet = m_descriptorSet;
    writeSet.dstBinding = 1;
    writeSet.dstArrayElement = 0;
    writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeSet.descriptorCount = 1;
    writeSet.pImageInfo = &fontImageInfo;
    vkUpdateDescriptorSets(m_device, 1, &writeSet, 0, nullptr);


    VkCommandPoolCreateInfo cmdPoolCreateInfo{};
    cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCreateInfo.queueFamilyIndex = bootstrap::GetQueueIndex(m_physDevice, bootstrap::QueueType::GRAPHICS);
    cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if (vkCreateCommandPool(m_device, &cmdPoolCreateInfo, nullptr, &m_cmdPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool");
    }

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.commandPool = m_cmdPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    if (vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_cmdBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffer");
    }

    // VkSemaphoreCreateInfo semaphoreInfo{};
    // semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    // if(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &renderingFinishedSemaphore) != VK_SUCCESS) {
    //     throw std::runtime_error("Failed to create semaphore");
    // }

    // VkFenceCreateInfo fenceInfo{};
    // fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    // if(vkCreateFence(m_device, &fenceInfo, nullptr, &InFlightFence) != VK_SUCCESS) {
    //     throw std::runtime_error("Failed to create fence");
    // }

    // this->imageAvailableSemaphore = imgSemaphore;
    // if(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS) {
    //     throw std::runtime_error("Failed to create semaphore");
    // }

    pipelineCreator.join();
}

// ! Inputting characters like i make the whole text go a little down. Find out why.
void TextRenderer::Render(const char *pText, VkCommandBuffer cmd, float scale, float x, float y, TextRendererAlignmentHorizontal horizontal, TextRendererAlignmentVertical vertical)
{
    // Required to prevent vulkan errors (bufferSize will be equal to 0)
    const u32 len = strlen(pText);

    if (len == 0)
        return;

    u32 iterator = 0;
    SDL_Thread *thread = nullptr;

    size_t thisFrameHash;
    thisFrameHash = boost::hash_value(pText) + boost::hash_value(len) + boost::hash_value(scale) + boost::hash_value(horizontal) + boost::hash_value(vertical);

    u32 width, height, maxWidth;
    GetTextSize(pText, &width, &height, &maxWidth);

    const u32 indexCount = len * 6;

    // Reallocate buffers, stream data to GPU
    if (thisFrameHash != prevFrameHash)
    {
        const float fbmpHeight = (float)bmpHeight;
        const float invBmpWidth = 1.0f / (float)bmpWidth;

        vec4 vertices[len * 4];
        u16 indices[len * 6];

        // * Mesh generating for all the text
        for (u32 i = 0; i < len; i++)
        {
            const char c = pText[i];
            const Character &ch = Characters[c];

            switch (c)
            {
            case '\n':
                y += bmpHeight * scale;
                x = 0.0f;
                continue;
                break;
            // case '\t':
            //     x += ch.advance * scale * 4.0f;
            //     continue;
            //     break;
            // case '\r':
            //     y += bmpHeight * scale;
            //     x = 0.0f;
            //     continue;
            //     break;
            default:
                break;
            }

            float xpos = x + ch.bearing.x * scale;
            float ypos = y + ((bmpHeight / 2.0f) - ch.bearing.y) * scale; // (character.Size.y - character.Bearing.y)* scale;
            // float xpos = 0.0f;
            // float ypos = 0.0f;

            // switch(vertical)
            // {
            // case TEXT_VERTICAL_ALIGN_TOP:
            //     ypos = y + ch.bearing.y * scale;
            //     break;
            // case TEXT_VERTICAL_ALIGN_CENTER:
            //     ypos = y + ((bmpHeight / 2.0f) - ch.bearing.y) * scale;
            //     break;
            // case TEXT_VERTICAL_ALIGN_BOTTOM:
            //     ypos = y + (bmpHeight - ch.bearing.y) * scale;
            //     break;
            // default:
            //     throw std::runtime_error("Invalid vertical alignment. (Implement?)");
            //     break;
            // }

            // switch(horizontal)
            // {
            // case TEXT_HORIZONTAL_ALIGN_LEFT:
            //     xpos = x + ch.bearing.x * scale;
            //     break;
            // case TEXT_HORIZONTAL_ALIGN_CENTER:
            //     xpos = x + (maxWidth / 2 - ch.bearing.x) * scale;
            //     break;
            // case TEXT_HORIZONTAL_ALIGN_RIGHT:
            //     xpos = x + (maxWidth - ch.bearing.x) * scale;
            //     break;
            // default:
            //     throw std::runtime_error("Invalid horizontal alignment. (Implement?)");
            //     break;
            // }

            float scaledWidth = static_cast<float>(ch.size.x * scale);
            float rawHeight = static_cast<float>(ch.size.y);
            float scaledHeight = rawHeight * scale;
            float u0 = static_cast<float>(ch.offset * invBmpWidth);
            float u1 = static_cast<float>((ch.offset + ch.size.x) * invBmpWidth);
            float v = rawHeight / fbmpHeight;

            const vec4 chvertices[4] = {
                vec4(xpos,               ypos,                u0, 0.0f),
                vec4(xpos + scaledWidth, ypos,                u1, 0.0f),
                vec4(xpos + scaledWidth, ypos + scaledHeight, u1, v),
                vec4(xpos,               ypos + scaledHeight, u0, v),
            };

            // This makes more sense to me than copying over the elements one by one
            // I know it won't be beneficial to performance. I do not care.
            memcpy(vertices + iterator * 4, chvertices, sizeof(chvertices));

            const u32 indexOffset = iterator * 4;
            const u16 setIndices[6] = {
                // compiler hates me
                // If you don't do this, you get warnings from compiler that wah wah wah
                static_cast<u16>(indexOffset + 0), static_cast<u16>(indexOffset + 1), static_cast<u16>(indexOffset + 2),
                static_cast<u16>(indexOffset + 2), static_cast<u16>(indexOffset + 3), static_cast<u16>(indexOffset + 0)};

            memcpy(indices + iterator * 6, setIndices, sizeof(setIndices));

            iterator++;
            x += ch.advance * scale;
        }

        // const bool unifiedBufferNotNull = unifiedBuffer != VK_NULL_HANDLE;
        // const bool memNotNull = mem != VK_NULL_HANDLE;
        // if (unifiedBuffer || memNotNull)
        // {
        //     // Why do I always forget this...
        //     vkDeviceWaitIdle(m_device);

        //     if (unifiedBufferNotNull)
        //         vkDestroyBuffer(m_device, unifiedBuffer, nullptr);
        //     if (memNotNull)
        //         vkFreeMemory(m_device, mem, nullptr);
        // }
        DestroyBufferData data = {
            m_device, unifiedBuffer, mem};
        // Does not matter when this ends
        // thread = SDL_CreateThread(DestroyBuffers, ("DestroyBuffers" + std::to_string(SDL_GetTicks())).data(), &data);
        thread = SDL_CreateThread(DestroyBuffers, ("BufferDestroyer" + std::to_string(SDL_GetTicks())).data(), &data);

        const u32 vertexCount = iterator * 4;

        const u32 vertexByteSize = vertexCount * sizeof(vec4);
        const u32 indexByteSize = len * 6 * sizeof(u16);

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

        // Use uchar for pointer operations instead of void ptr
        uchar *vbMap;
        if (vkMapMemory(m_device, mem, 0, vertexByteSize + indexByteSize, 0, reinterpret_cast<void **>(&vbMap)) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to map vertex buffer memory");
        }

        memcpy(vbMap, vertices, vertexByteSize);
        memcpy(vbMap + vertexByteSize, indices, indexByteSize);

        // Non coherent mapped memory ranges must be flushed
        // This is also one of the things I just keep forgetting
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

    // constexpr VkClearValue clearValues[] = {
    //     {{{0.0f, 0.0f, 0.0f, 1.0f}}},
    // };

    // const auto& cmd = m_cmdBuffer;
    // const auto& pass = m_renderPass;

    // VkCommandBufferBeginInfo beginInfo{};
    // beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    // vkBeginCommandBuffer(cmd, &beginInfo);

    // VkRenderPassBeginInfo renderPassInfo{};
    // renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    // renderPassInfo.renderPass = pass;
    // renderPassInfo.renderArea.offset = {0, 0};
    // renderPassInfo.framebuffer = fb;
    // renderPassInfo.renderArea.extent = extent;
    // renderPassInfo.clearValueCount = 1;
    // renderPassInfo.pClearValues = clearValues;

    // vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    constexpr VkDeviceSize offsets = 0;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);
    vkCmdBindVertexBuffers(cmd, 0, 1, &unifiedBuffer, &offsets);
    vkCmdBindIndexBuffer(cmd, unifiedBuffer, ibOffset, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);

    // * thread == nullptr is valid
    SDL_WaitThread(thread, NULL);

    // // Submit
    // {
    //     vkCmdEndRenderPass(m_cmdBuffer);
    //     vkEndCommandBuffer(m_cmdBuffer);

    //     VkSubmitInfo submitInfo{};
    //     submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    //     const VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    //     const VkSemaphore signalSemaphores[] = {renderingFinishedSemaphore};

    //     VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    //     submitInfo.pWaitDstStageMask = waitStages;

    //     submitInfo.waitSemaphoreCount = 1;
    //     submitInfo.pWaitSemaphores = waitSemaphores;

    //     submitInfo.commandBufferCount = 1;
    //     submitInfo.pCommandBuffers = &m_cmdBuffer;

    //     submitInfo.signalSemaphoreCount = 1;
    //     submitInfo.pSignalSemaphores = signalSemaphores;

    //     // Submit command buffer
    //     vkQueueSubmit(m_queue, 1, &submitInfo, InFlightFence);

    //     // Present the image
    //     VkPresentInfoKHR presentInfo{};
    //     presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    //     presentInfo.waitSemaphoreCount = 1;
    //     presentInfo.pWaitSemaphores = signalSemaphores; // This is signalSemaphores so that this starts as soon as the signaled semaphores are signaled.
    //     presentInfo.pImageIndices = imgIndices;
    //     presentInfo.swapchainCount = 1;
    //     presentInfo.pSwapchains = swapchain;

    //     vkQueuePresentKHR(m_queue, &presentInfo);
    // }

    prevFrameHash = thisFrameHash;
}

void TextRenderer::CreatePipeline()
{
    // // Acquire the number of MSAA samples allowed. (Even if we'll only be using floor, some may not support multisampling so..)
    // VkPhysicalDeviceProperties properties;
    // vkGetPhysicalDeviceProperties(m_physDevice, &properties);

    // // const VkSampleCountFlags count = properties.limits.framebufferColorSampleCounts;
    // // Reads out to get the number of samples (ceiled to 4)
    // m_samples = static_cast<VkSampleCountFlagBits>(properties.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_4_BIT);
    // const bool multisamplingAvailable = m_samples != VK_SAMPLE_COUNT_1_BIT;

    VkShaderModule vertexShaderModule;
    VkShaderModule fragmentShaderModule;

    VkShaderModuleCreateInfo vertexShaderModuleInfo{};
    vertexShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderModuleInfo.codeSize = sizeof(VertexShaderBinary);
    vertexShaderModuleInfo.pCode = VertexShaderBinary;

    if (vkCreateShaderModule(m_device, &vertexShaderModuleInfo, nullptr, &vertexShaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create vertex shader module!");
    }

    VkShaderModuleCreateInfo fragmentShaderModuleInfo{};
    fragmentShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentShaderModuleInfo.codeSize = sizeof(FragmentShaderBinary);
    fragmentShaderModuleInfo.pCode = FragmentShaderBinary;

    if (vkCreateShaderModule(m_device, &fragmentShaderModuleInfo, nullptr, &fragmentShaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create fragment shader module!");
    }

    const SafeArray<VkPushConstantRange> pushConstants = {
        // stageFlags; offset; size;
        {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) + sizeof(glm::vec2)},
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
    pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    // VkAttachmentDescription colorAttachmentDescription{};
    // colorAttachmentDescription.format = VK_FORMAT_B8G8R8A8_SRGB;
    // colorAttachmentDescription.samples = m_samples;
    // colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // VkAttachmentReference colorAttachmentReference{};
    // colorAttachmentReference.attachment = 0;
    // colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // VkSubpassDependency dependency{};
    // dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    // dependency.dstSubpass = 0;
    // dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // dependency.srcAccessMask = 0;
    // dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // VkSubpassDescription subpass{};
    // subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // subpass.colorAttachmentCount = 1;

    // subpass.pColorAttachments = &colorAttachmentReference;

    // const VkAttachmentDescription attachments[] = {colorAttachmentDescription};

    // VkRenderPassCreateInfo renderPassInfo{};
    // renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    // renderPassInfo.attachmentCount = 1;
    // renderPassInfo.pAttachments = attachments;
    // renderPassInfo.subpassCount = 1;
    // renderPassInfo.pSubpasses = &subpass;
    // renderPassInfo.dependencyCount = 1;
    // renderPassInfo.pDependencies = &dependency;

    // pro::ResultCheck(vkCreateRenderPass(m_device, &renderPassInfo, VK_NULL_HANDLE, &m_renderPass));

    const SafeArray<VkVertexInputAttributeDescription> attributeDescriptions = {
        // location; binding; format; offset;
        {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
    };

    const SafeArray<VkVertexInputBindingDescription> bindingDescriptions = {
        // binding; stride; inputRate
        {0, sizeof(glm::vec4), VK_VERTEX_INPUT_RATE_VERTEX},
    };

    const SafeArray<VkDescriptorSetLayout> descriptorSetLayouts = {
        m_descriptorSetLayout};

    VkPipelineShaderStageCreateInfo vertexShaderInfo{};
    vertexShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderInfo.module = vertexShaderModule;
    vertexShaderInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderInfo{};
    fragmentShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderInfo.module = fragmentShaderModule;
    fragmentShaderInfo.pName = "main";

    const SafeArray<VkPipelineShaderStageCreateInfo> shaders = {vertexShaderInfo, fragmentShaderInfo};

    /*
     *   PIPELINE CREATION
     */

    pro::RenderPassCreateInfo rc{};
    rc.subpass = 0;
    rc.format = VK_FORMAT_B8G8R8A8_SRGB;
    rc.samples = m_samples;
    pro::CreateRenderPass(m_device, &rc, &m_renderPass, PIPELINE_CREATE_FLAGS_ENABLE_BLEND);

    pro::PipelineCreateInfo pc{};
    pc.format = VK_FORMAT_B8G8R8A8_SRGB;
    pc.subpass = 0;
    pc.renderPass = m_renderPass;
    pc.pipelineLayout = m_pipelineLayout;
    pc.pAttributeDescriptions = &attributeDescriptions;
    pc.pBindingDescriptions = &bindingDescriptions;
    pc.pDescriptorLayouts = &descriptorSetLayouts;
    pc.pPushConstants = &pushConstants;
    pc.pShaderCreateInfos = &shaders;
    pc.extent = {800, 600};
    pc.pShaderCreateInfos = &shaders;
    pc.samples = m_samples;
    pro::CreateGraphicsPipeline(m_device, &pc, &m_pipeline, PIPELINE_CREATE_FLAGS_ENABLE_BLEND);

    vkDestroyShaderModule(m_device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(m_device, fragmentShaderModule, nullptr);
}

#pragma GCC diagnostic pop