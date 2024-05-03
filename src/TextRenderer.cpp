#include "TextRenderer.hpp"
#include "pro.hpp"
#include "DMalloc.hpp"
#include <omp.h>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"
#endif

// TODO: Convert the render function to a "submit" function. Then add a stream
// data to GPU function and finally a draw function.
struct DestroyBufferData
{
    VkDevice device;
    VkBuffer buff;
    VkDeviceMemory mem;
};

/*
    You can learn more about the FNV Hashing functions from : http://isthe.com/chongo/tech/comp/fnv/index.html#FNV-1a
    (that's where I got them from).
*/
static u32 FNV1AHash(const char* str) {
    constexpr u32 FNV1APrime = 16777619;
    constexpr u32 OffsetBasis = 2166136261;
    
    u32 hash = OffsetBasis;
    while(*str)
        hash ^= *str++ * FNV1APrime;

    return hash;
}
static u32 FNV1AHash(u32 n) {
    constexpr u32 FNV1APrime = 16777619;
    constexpr u32 OffsetBasis = 2166136261;
    
    u32 hash = OffsetBasis;
    for(u8 i = 0; i < 4; i++)
        hash ^= reinterpret_cast<u8*>(&n)[i] * FNV1APrime;

    return hash;
}

static u64 FNV1AHash(u64 n) {
    constexpr u64 FNV1APrime = 1099511628211U;
    constexpr u64 OffsetBasis = 14695981039346656037U;
    
    u64 hash = OffsetBasis;
    for(u8 i = 0; i < 8; i++)
        hash ^= reinterpret_cast<u8*>(&n)[i] * FNV1APrime;

    return hash;
}

static inline u32 FNV1AHash(f32 f) {
    return FNV1AHash(static_cast<u32>(f * 100)); // Convert to u32
}

void* vkalloc(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) {
    std::cout << "alloc'd: " << size << ", " << alignment << '\n';
    return pUserData;
}

void* vkrealloc(void *pUserData, void *pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) {
    return pUserData;
}

void vkfree(void *pUserData, void *pMemory) {}

static std::vector<std::string> SplitString(const std::string& text)
{
    std::vector<std::string> result;
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line))
        result.push_back(line);
    return result;
}

constexpr static inline u32 align(u32 num, u32 align) {
    return (num + align - 1) & ~(align - 1);
}

TextRenderer::TextRenderer() {}
/*
#version 450 core

layout(location=0) in
vec4 verticesAndUV; // <vec2 vertices, vec2 uv>

layout(location=0) out
vec2 FragTexCoord;

layout(push_constant) uniform PushConstantData {
    mat4 model;
};

void main() {
    gl_Position = model * vec4(verticesAndUV.xy, 0.0, 1.0);
    FragTexCoord = verticesAndUV.zw;
}
*/
constexpr static u32 VertexShaderBinary[] = {
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
	0x00000013,0x00000000,0x00060005,0x00000018,0x74726576,0x73656369,0x55646e41,0x00000056,
	0x00060005,0x00000025,0x67617246,0x43786554,0x64726f6f,0x00000000,0x00050048,0x0000000b,
	0x00000000,0x0000000b,0x00000000,0x00050048,0x0000000b,0x00000001,0x0000000b,0x00000001,
	0x00050048,0x0000000b,0x00000002,0x0000000b,0x00000003,0x00050048,0x0000000b,0x00000003,
	0x0000000b,0x00000004,0x00030047,0x0000000b,0x00000002,0x00040048,0x00000011,0x00000000,
	0x00000005,0x00050048,0x00000011,0x00000000,0x00000023,0x00000000,0x00050048,0x00000011,
	0x00000000,0x00000007,0x00000010,0x00030047,0x00000011,0x00000002,0x00040047,0x00000018,
	0x0000001e,0x00000000,0x00040047,0x00000025,0x0000001e,0x00000000,0x00020013,0x00000002,
	0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,
	0x00000006,0x00000004,0x00040015,0x00000008,0x00000020,0x00000000,0x0004002b,0x00000008,
	0x00000009,0x00000001,0x0004001c,0x0000000a,0x00000006,0x00000009,0x0006001e,0x0000000b,
	0x00000007,0x00000006,0x0000000a,0x0000000a,0x00040020,0x0000000c,0x00000003,0x0000000b,
	0x0004003b,0x0000000c,0x0000000d,0x00000003,0x00040015,0x0000000e,0x00000020,0x00000001,
	0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040018,0x00000010,0x00000007,0x00000004,
	0x0003001e,0x00000011,0x00000010,0x00040020,0x00000012,0x00000009,0x00000011,0x0004003b,
	0x00000012,0x00000013,0x00000009,0x00040020,0x00000014,0x00000009,0x00000010,0x00040020,
	0x00000017,0x00000001,0x00000007,0x0004003b,0x00000017,0x00000018,0x00000001,0x00040017,
	0x00000019,0x00000006,0x00000002,0x0004002b,0x00000006,0x0000001c,0x00000000,0x0004002b,
	0x00000006,0x0000001d,0x3f800000,0x00040020,0x00000022,0x00000003,0x00000007,0x00040020,
	0x00000024,0x00000003,0x00000019,0x0004003b,0x00000024,0x00000025,0x00000003,0x00050036,
	0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000014,
	0x00000015,0x00000013,0x0000000f,0x0004003d,0x00000010,0x00000016,0x00000015,0x0004003d,
	0x00000007,0x0000001a,0x00000018,0x0007004f,0x00000019,0x0000001b,0x0000001a,0x0000001a,
	0x00000000,0x00000001,0x00050051,0x00000006,0x0000001e,0x0000001b,0x00000000,0x00050051,
	0x00000006,0x0000001f,0x0000001b,0x00000001,0x00070050,0x00000007,0x00000020,0x0000001e,
	0x0000001f,0x0000001c,0x0000001d,0x00050091,0x00000007,0x00000021,0x00000016,0x00000020,
	0x00050041,0x00000022,0x00000023,0x0000000d,0x0000000f,0x0003003e,0x00000023,0x00000021,
	0x0004003d,0x00000007,0x00000026,0x00000018,0x0007004f,0x00000019,0x00000027,0x00000026,
	0x00000026,0x00000002,0x00000003,0x0003003e,0x00000025,0x00000027,0x000100fd,0x00010038
};

/*
#version 450 core

layout(location = 0) out vec4 FragColor;

layout(location=0) 
in vec2 FragTexCoord;

layout(binding = 1)
uniform sampler2D fontTexture;

void main() {
    const vec4 sampled = vec4(1.0, 1.0, 1.0, texture(fontTexture, FragTexCoord.xy).r);
    FragColor = sampled;
}
*/
constexpr static u32 FragmentShaderBinary[] = {
	// 1112.0.0
	0x07230203,0x00010000,0x0008000b,0x0000001c,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000012,0x0000001a,0x00030010,
	0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00040005,0x00000009,0x706d6173,0x0064656c,0x00050005,0x0000000e,0x746e6f66,
	0x74786554,0x00657275,0x00060005,0x00000012,0x67617246,0x43786554,0x64726f6f,0x00000000,
	0x00050005,0x0000001a,0x67617246,0x6f6c6f43,0x00000072,0x00040047,0x0000000e,0x00000022,
	0x00000000,0x00040047,0x0000000e,0x00000021,0x00000001,0x00040047,0x00000012,0x0000001e,
	0x00000000,0x00040047,0x0000001a,0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,
	0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,
	0x00000004,0x00040020,0x00000008,0x00000007,0x00000007,0x0004002b,0x00000006,0x0000000a,
	0x3f800000,0x00090019,0x0000000b,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
	0x00000001,0x00000000,0x0003001b,0x0000000c,0x0000000b,0x00040020,0x0000000d,0x00000000,
	0x0000000c,0x0004003b,0x0000000d,0x0000000e,0x00000000,0x00040017,0x00000010,0x00000006,
	0x00000002,0x00040020,0x00000011,0x00000001,0x00000010,0x0004003b,0x00000011,0x00000012,
	0x00000001,0x00040015,0x00000015,0x00000020,0x00000000,0x0004002b,0x00000015,0x00000016,
	0x00000000,0x00040020,0x00000019,0x00000003,0x00000007,0x0004003b,0x00000019,0x0000001a,
	0x00000003,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
	0x0004003b,0x00000008,0x00000009,0x00000007,0x0004003d,0x0000000c,0x0000000f,0x0000000e,
	0x0004003d,0x00000010,0x00000013,0x00000012,0x00050057,0x00000007,0x00000014,0x0000000f,
	0x00000013,0x00050051,0x00000006,0x00000017,0x00000014,0x00000000,0x00070050,0x00000007,
	0x00000018,0x0000000a,0x0000000a,0x0000000a,0x00000017,0x0003003e,0x00000009,0x00000018,
	0x0004003d,0x00000007,0x0000001b,0x00000009,0x0003003e,0x0000001a,0x0000001b,0x000100fd,
	0x00010038
};

void TextRenderer::GetTextSize(const std::string& pText, f32 *width, f32 *height, f32 scale)
{
    const u32 len = pText.size();
    f32 x = 0;
    f32 y = 0;

    for (u32 i = 0; i < len; i++)
    {
        const char c = pText[i];
        const Character &ch = Characters[c];

        const f32 h = ch.size.y * scale;
        const f32 w = ch.advance * scale; // Use advance instead of size for width

        x += w; // Accumulate the advance (width) of each character
        y = std::max(y, h);
    }

    *width = x;
    *height = y;
}

void TextRenderer::Init(VkDevice device, VkPhysicalDevice physDevice, VkQueue queue, VkCommandPool commandPool, VkRenderPass renderPass, const char *fontPath)
{
    memset(this, 0, sizeof(TextRenderer));

    m_device = device;
    m_physDevice = physDevice;
    m_queue = queue;
    // Partial descriptor set setup to satisfy pipeline creation

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
    if(vkCreateSampler(device, &samplerInfo, nullptr, &FontTextureSampler) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create font texture sampler!");
    }

    VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &FontTextureSampler },
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

    TIME_FUNCTION(CreatePipeline(renderPass));

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
            uvec2(face->glyph->metrics.width >> 6,
                  face->glyph->metrics.height >> 6),
            ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            bmpWidth,
            static_cast<f32>(face->glyph->advance.x / 64.0f)
        };

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

                    To fetch all the data from row 0, you would do buffer offset by row
                   * the width of each row (bmpWidth).
                */
                memcpy(buffer + row * bmpWidth + xpos, charDataBuffer + row * width,
                       width);
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
    if (vkCreateImage(device, &imageCreateInfo, nullptr, &FontTexture) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image");
    }

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, FontTexture, &imageMemoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = imageMemoryRequirements.size;
    allocInfo.memoryTypeIndex = pro::GetMemoryType(physDevice, imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if(vkAllocateMemory(device, &allocInfo, nullptr, &FontTextureMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate memory");
    }
    vkBindImageMemory(device, FontTexture, FontTextureMemory, 0);

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    uchar stagingBufferBacking[256];

    VkAllocationCallbacks test{};
    test.pfnAllocation = vkalloc;
    test.pfnFree = vkfree;
    test.pfnReallocation = vkrealloc;
    test.pUserData = reinterpret_cast<void*>(stagingBufferBacking);

    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = bmpWidth * bmpHeight;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(device, &stagingBufferInfo, &test, &stagingBuffer);

    VkMemoryRequirements stagingBufferRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferRequirements);

    VkMemoryAllocateInfo stagingBufferAllocInfo{};
    stagingBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingBufferAllocInfo.allocationSize = stagingBufferRequirements.size;
    stagingBufferAllocInfo.memoryTypeIndex = pro::GetMemoryType(physDevice, stagingBufferRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if(vkAllocateMemory(device, &stagingBufferAllocInfo, &test, &stagingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate memory");
    }

    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

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
    viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.image = FontTexture;
    if(vkCreateImageView(device, &viewInfo, nullptr, &FontTextureView)) {
        throw std::runtime_error("Failed to create image view");
    }

    // Image layout transition : UNDEFINED -> TRANSFER_DST_OPTIMAL
    const VkCommandBuffer cmd = bootstrap::BeginSingleTimeCommands(device, commandPool);

    VkImageMemoryBarrier preCopyBarrier{};
    preCopyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    preCopyBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    preCopyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    preCopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preCopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preCopyBarrier.image = FontTexture;
    preCopyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    preCopyBarrier.subresourceRange.baseMipLevel = 0;
    preCopyBarrier.subresourceRange.levelCount = 1;
    preCopyBarrier.subresourceRange.baseArrayLayer = 0;
    preCopyBarrier.subresourceRange.layerCount = 1;
    preCopyBarrier.srcAccessMask = 0;
    preCopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &preCopyBarrier);

    VkBufferImageCopy region{};
    region.imageExtent = { bmpWidth, bmpHeight, 1 };
    region.imageOffset = { 0, 0, 0 };
    region.bufferOffset = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.mipLevel = 0;
    vkCmdCopyBufferToImage(cmd, stagingBuffer, FontTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Image layout transition : TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    VkImageMemoryBarrier postCopyBarrier{};
    postCopyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    postCopyBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    postCopyBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    postCopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postCopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postCopyBarrier.image = FontTexture;
    postCopyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    postCopyBarrier.subresourceRange.baseMipLevel = 0;
    postCopyBarrier.subresourceRange.levelCount = 1;
    postCopyBarrier.subresourceRange.baseArrayLayer = 0;
    postCopyBarrier.subresourceRange.layerCount = 1;
    postCopyBarrier.srcAccessMask = 0;
    postCopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &postCopyBarrier);

    bootstrap::EndSingleTimeCommands(device, cmd, queue, commandPool);

    vkDestroyBuffer(device, stagingBuffer, &test);
    vkFreeMemory(device, stagingBufferMemory, &test);

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
}

void TextRenderer::AddToTextRenderQueue(std::string& line, const float scale, const float x, float y, TextRendererAlignmentHorizontal horizontal, TextRendererAlignmentVertical vertical)
{
    TextRenderInfo info;
    info.scale = scale;
    info.x = x;
    info.y = y;
    info.horizontal = horizontal;
    info.vertical = vertical;
    info.line = line;
    drawList.push_back(info);
}

void TextRenderer::Render(VkCommandBuffer cmd)
{
    u32 len = 0;
    const auto begin = std::chrono::high_resolution_clock::now();
    
    u64 thisFrameHash = 0;
    for(const auto& draw : drawList) {
        len += draw.line.size();
        thisFrameHash += FNV1AHash(draw.line.c_str()) + FNV1AHash(draw.scale) + FNV1AHash(draw.x) + FNV1AHash(draw.y) + FNV1AHash(draw.horizontal) + FNV1AHash(draw.vertical);
    }
    const auto end = std::chrono::high_resolution_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);

    if(len == 0)
        return;
    
    thisFrameHash += FNV1AHash(len);

    if(thisFrameHash != lastFrameHash)
    {
        u32 iterator = 0;
        alignas(16) vec4 vertices[len * 4];
        alignas(2) u16 indices[len * 6];
        for(u32 drawIndex = 0; drawIndex < drawList.size(); drawIndex++)
        {
            const auto& draw = drawList[drawIndex];

            const std::string& pText = draw.line;
            const f32 scale = draw.scale;
            const TextRendererAlignmentHorizontal horizontal = draw.horizontal;
            const TextRendererAlignmentVertical vertical = draw.vertical;
            const f32 x = draw.x;
            f32 y = draw.y;

            const std::vector<std::string> splitLines = SplitString(pText);
            const f32 invBmpWidth = 1.0f / static_cast<f32>(bmpWidth);

            if(vertical == TEXT_VERTICAL_ALIGN_CENTER || vertical == TEXT_VERTICAL_ALIGN_BOTTOM) {
                // Calculate total height of text
                f32 totalHeight = 0.0f;
                for (const auto& line : splitLines)
                {
                    // f32 _, lineHeight, __;
                    // GetTextSize(line.c_str(), &_, &lineHeight, &__, scale);

                    f32 lineHeight = 0.0f;
                    for (const auto& c : pText)
                    {
                        const Character &ch = Characters[c];
                        const f32 h = ch.size.y * scale;

                        lineHeight = std::max(lineHeight, h);
                    }

                    totalHeight += lineHeight;
                }

                // Calculate starting y position for centered vertical alignment
                (vertical == TEXT_VERTICAL_ALIGN_CENTER) ? y = y - (totalHeight / 2.0f) : y = y - totalHeight;
            }

            // Reallocate buffers, stream data to GPU
            for (u32 l = 0; l < splitLines.size(); l++)
            {
                const auto& line = splitLines[l];
                f32 lineX = 0.0f;
                f32 totalWidth, height;
                GetTextSize(line, &totalWidth, &height, scale);

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
                    throw std::runtime_error("Invalid horizontal alignment. (Implement?)");
                    break;
                }

                // Mesh generating for all the text
                for (u32 i = 0; i < line.size(); i++)
                {
                    const char& c = line[i];
                    const Character &ch = Characters[c];

                    if(c == '\n') {
                        y += bmpHeight * scale;
                        lineX = 0.0f;
                        continue;
                    }

                    const f32 xpos = lineX + ch.bearing.x * scale;
                    f32 ypos;

                    switch (vertical)
                    {
                    case TEXT_VERTICAL_ALIGN_TOP:
                        ypos = y + (bmpHeight - ch.bearing.y) * scale; 
                        break;
                    case TEXT_VERTICAL_ALIGN_CENTER:
                        ypos = y + (lineHeight - ch.bearing.y) * scale;
                        break;
                    case TEXT_VERTICAL_ALIGN_BOTTOM:
                        ypos = y + (bmpHeight - ch.bearing.y - lineHeight) * scale; 
                        break;
                    default:
                        throw std::runtime_error("Invalid vertical alignment. (Implement?)");
                        break;
                    }

                    const f32 scaledWidth = ch.size.x * scale;
                    const f32 rawHeight = static_cast<f32>(ch.size.y);
                    const f32 scaledHeight = rawHeight * scale;
                    const f32 u0 = ch.offset * invBmpWidth;
                    const f32 u1 = (ch.offset + ch.size.x) * invBmpWidth;
                    const f32 v = rawHeight / bmpHeight;

                    const vec4 newVertices[4] = {
                        vec4(xpos,               ypos,                u0, 0.0f),
                        vec4(xpos + scaledWidth, ypos,                u1, 0.0f),
                        vec4(xpos + scaledWidth, ypos + scaledHeight, u1, v   ),
                        vec4(xpos,               ypos + scaledHeight, u0, v   ),
                    };
                    
                    const u32 indexOffset = iterator << 2;

                    // Calculate indices for the current character
                    const u16 newIndices[6] = {
                        static_cast<u16>(indexOffset + 0), static_cast<u16>(indexOffset + 1), static_cast<u16>(indexOffset + 2),
                        static_cast<u16>(indexOffset + 2), static_cast<u16>(indexOffset + 3), static_cast<u16>(indexOffset + 0),
                    };

                    const u32 vertexIndex = iterator * 4;
                    const u32 indexIndex = iterator * 6;

                    memcpy(vertices + vertexIndex, newVertices, sizeof(newVertices));
                    memcpy(indices + indexIndex, newIndices, sizeof(newIndices));

                    iterator++;
                    lineX += ch.advance * scale;
                }

                y += bmpHeight * scale;
            }
        }
    
        indexCount = iterator * 6;

        const u32 vertexByteSize = (iterator * 4) * sizeof(vec4);
        const u32 indexByteSize = indexCount * sizeof(u16);

        const u32 bufferSize = ( iterator * 4 * sizeof(vec4) ) + ( iterator * 6 * sizeof(u16) );

        // If the buffer is this much larger than needed, make it smaller.
        constexpr u8 BufferSizeDownscale = 3;
        // Allocate this much times more than we need to avoid further allocations. 2 is good enough.
        constexpr u8 BufferSizeUpscale = 2;

        static_assert(BufferSizeUpscale < BufferSizeDownscale && "Can cause undefined behaviour due to upscaling "
                                                                 "the buffer one frame and immediately downscaling it in the other");

        const bool needLargerBuffer = bufferSize > allocatedMemorySize;
        const bool needSmallerBuffer = bufferSize * BufferSizeDownscale < allocatedMemorySize;

        // Check if we need to resize the buffer
        if (needLargerBuffer || needSmallerBuffer)
        {
            const VkBuffer back_unifiedBuffer = unifiedBuffer;
            const VkDeviceMemory back_mem = mem;

            (needLargerBuffer) ?
                allocatedMemorySize = bufferSize * BufferSizeUpscale
                :
                allocatedMemorySize = std::max(bufferSize, allocatedMemorySize / BufferSizeDownscale);

            vkDeviceWaitIdle(m_device);
            vkDestroyBuffer(m_device, back_unifiedBuffer, nullptr);
            vkFreeMemory(m_device, back_mem, nullptr);

            VkBufferCreateInfo VBbufferInfo = {};
            VBbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            VBbufferInfo.size = allocatedMemorySize;
            VBbufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            VBbufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            if (vkCreateBuffer(m_device, &VBbufferInfo, nullptr, &unifiedBuffer) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create vertex buffer!");
            }

            VkMemoryRequirements VBmemoryRequirements;
            vkGetBufferMemoryRequirements(m_device, unifiedBuffer, &VBmemoryRequirements);

            std::cout << "but vulkan said " << VBmemoryRequirements.size << '\n';

            if (alignment == 0) alignment = VBmemoryRequirements.alignment;

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = VBmemoryRequirements.size;
            allocInfo.memoryTypeIndex = bootstrap::GetMemoryType(m_physDevice, VBmemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (vkAllocateMemory(m_device, &allocInfo, nullptr, &mem) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate vertex buffer memory!");
            }
    
            vkBindBufferMemory(m_device, unifiedBuffer, mem, 0);
        }

        ibOffset = align(vertexByteSize, alignment);
        
        // Use uchar for pointer operations instead of void ptr
        uchar *vbMap;
        if (vkMapMemory(m_device, mem, 0, allocatedMemorySize, 0, reinterpret_cast<void **>(&vbMap)) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to map vertex buffer memory");
        }
        memcpy(vbMap, vertices, vertexByteSize);
        memcpy(vbMap + ibOffset, indices, indexByteSize);

        // Non coherent mapped memory ranges must be flushed
        // This is also one of the things I just keep forgetting
        VkMappedMemoryRange range{};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = mem;
        range.size = allocatedMemorySize;
        range.offset = 0;
        vkFlushMappedMemoryRanges(m_device, 1, &range);

        vkUnmapMemory(m_device, mem);
    }

    drawList.clear();

    constexpr VkDeviceSize offsets = 0;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);
    vkCmdBindVertexBuffers(cmd, 0, 1, &unifiedBuffer, &offsets);
    vkCmdBindIndexBuffer(cmd, unifiedBuffer, ibOffset, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);

    lastFrameHash = thisFrameHash;
}

void TextRenderer::CreatePipeline(VkRenderPass renderPass) {
    VkShaderModule vertexShaderModule;
    VkShaderModule fragmentShaderModule;

    VkShaderModuleCreateInfo vertexShaderModuleInfo{};
    vertexShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderModuleInfo.codeSize = sizeof(VertexShaderBinary);
    vertexShaderModuleInfo.pCode = VertexShaderBinary;
    if (vkCreateShaderModule(m_device, &vertexShaderModuleInfo, nullptr, &vertexShaderModule) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create vertex shader module!");
    }

    VkShaderModuleCreateInfo fragmentShaderModuleInfo{};
    fragmentShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentShaderModuleInfo.codeSize = sizeof(FragmentShaderBinary);
    fragmentShaderModuleInfo.pCode = FragmentShaderBinary;
    if (vkCreateShaderModule(m_device, &fragmentShaderModuleInfo, nullptr, &fragmentShaderModule) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create fragment shader module!");
    }

    const std::vector<VkPushConstantRange> pushConstants = {
        // stageFlags; offset; size;
        {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) + sizeof(glm::vec2) },
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
    pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create pipeline layout!");
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

    VkPipelineShaderStageCreateInfo fragmentShaderInfo{};
    fragmentShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderInfo.module = fragmentShaderModule;
    fragmentShaderInfo.pName = "main";

    const std::vector<VkPipelineShaderStageCreateInfo> shaders = {
        vertexShaderInfo, fragmentShaderInfo
    };

    /*
    *   PIPELINE CREATION
    */

    pro::PipelineCreateInfo pc{};
    pc.format = VK_FORMAT_B8G8R8A8_SRGB;
    pc.subpass = 0;

    pc.renderPass = renderPass;
    pc.pipelineLayout = m_pipelineLayout;
    pc.pAttributeDescriptions = &attributeDescriptions;
    pc.pBindingDescriptions = &bindingDescriptions;
    pc.pDescriptorLayouts = &descriptorSetLayouts;
    pc.pPushConstants = &pushConstants;
    pc.pShaderCreateInfos = &shaders;
    pc.extent = VkExtent2D{800, 600};
    pc.pShaderCreateInfos = &shaders;
    pro::CreateGraphicsPipeline(m_device, &pc, &m_pipeline, PIPELINE_CREATE_FLAGS_ENABLE_BLEND);

    vkDestroyShaderModule(m_device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(m_device, fragmentShaderModule, nullptr);
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif