#include "TextRenderer.hpp"
#include "pro.hpp"
#include "Renderer.hpp"
#include "Image.hpp"
#include "epichelperlib.hpp"
#include <boost/filesystem.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"
#endif

// struct ComputeCharacter
// {
//     vec2 offsetAndAdvance; // < x = offset, y = advance >
//     vec4 bearingAndSize;// < xy = bearing, zw = size >
// };

// struct UBOData
// {
//     ComputeCharacter characters[ 128 ];
// };

// struct OutBufferData
// {
//     vec2 texturePositions[ 128 * 4 ];
// };

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

constexpr static u32 ComputeShaderBinary[] =
{
	// 1112.0.0
	0x07230203,0x00010000,0x0008000b,0x00000079,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0006000f,0x00000005,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x00060010,0x00000004,
	0x00000011,0x00000004,0x00000001,0x00000001,0x00030003,0x00000002,0x000001c2,0x00080004,
	0x455f4c47,0x735f5458,0x65646168,0x36315f72,0x5f746962,0x726f7473,0x00656761,0x00040005,
	0x00000004,0x6e69616d,0x00000000,0x00040005,0x00000008,0x65646e69,0x00000078,0x00080005,
	0x0000000b,0x475f6c67,0x61626f6c,0x766e496c,0x7461636f,0x496e6f69,0x00000044,0x00050005,
	0x00000018,0x664f6863,0x74657366,0x00000000,0x00050005,0x0000001b,0x72616843,0x65746361,
	0x00000072,0x00080006,0x0000001b,0x00000000,0x7366666f,0x6e417465,0x76644164,0x65636e61,
	0x00000000,0x00070006,0x0000001b,0x00000001,0x72616562,0x41676e69,0x6953646e,0x0000657a,
	0x00050005,0x0000001d,0x646e6572,0x61447265,0x00006174,0x00060006,0x0000001d,0x00000000,
	0x57706d62,0x68746469,0x00000000,0x00060006,0x0000001d,0x00000001,0x48706d62,0x68676965,
	0x00000074,0x00060006,0x0000001d,0x00000002,0x72616863,0x65746361,0x00007372,0x00030005,
	0x0000001f,0x00000000,0x00040005,0x00000028,0x69536863,0x0000657a,0x00050005,0x0000002f,
	0x42766e69,0x6957706d,0x00687464,0x00030005,0x00000036,0x00003075,0x00030005,0x0000003a,
	0x00003175,0x00030005,0x00000041,0x00000076,0x00050005,0x0000004a,0x4274754f,0x65666675,
	0x00000072,0x00060006,0x0000004a,0x00000000,0x74726576,0x73656369,0x00000000,0x00030005,
	0x0000004c,0x00000000,0x00040047,0x0000000b,0x0000000b,0x0000001c,0x00050048,0x0000001b,
	0x00000000,0x00000023,0x00000000,0x00050048,0x0000001b,0x00000001,0x00000023,0x00000010,
	0x00040047,0x0000001c,0x00000006,0x00000020,0x00050048,0x0000001d,0x00000000,0x00000023,
	0x00000000,0x00050048,0x0000001d,0x00000001,0x00000023,0x00000004,0x00050048,0x0000001d,
	0x00000002,0x00000023,0x00000010,0x00030047,0x0000001d,0x00000002,0x00040047,0x0000001f,
	0x00000022,0x00000000,0x00040047,0x0000001f,0x00000021,0x00000000,0x00040047,0x00000049,
	0x00000006,0x00000010,0x00050048,0x0000004a,0x00000000,0x00000023,0x00000000,0x00030047,
	0x0000004a,0x00000003,0x00040047,0x0000004c,0x00000022,0x00000000,0x00040047,0x0000004c,
	0x00000021,0x00000001,0x00040047,0x00000077,0x0000000b,0x00000019,0x00020013,0x00000002,
	0x00030021,0x00000003,0x00000002,0x00040015,0x00000006,0x00000020,0x00000000,0x00040020,
	0x00000007,0x00000007,0x00000006,0x00040017,0x00000009,0x00000006,0x00000003,0x00040020,
	0x0000000a,0x00000001,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000001,0x0004002b,
	0x00000006,0x0000000c,0x00000000,0x00040020,0x0000000d,0x00000001,0x00000006,0x0004002b,
	0x00000006,0x00000011,0x00000080,0x00020014,0x00000012,0x00030016,0x00000016,0x00000020,
	0x00040020,0x00000017,0x00000007,0x00000016,0x00040017,0x00000019,0x00000016,0x00000002,
	0x00040017,0x0000001a,0x00000016,0x00000004,0x0004001e,0x0000001b,0x00000019,0x0000001a,
	0x0004001c,0x0000001c,0x0000001b,0x00000011,0x0005001e,0x0000001d,0x00000006,0x00000006,
	0x0000001c,0x00040020,0x0000001e,0x00000002,0x0000001d,0x0004003b,0x0000001e,0x0000001f,
	0x00000002,0x00040015,0x00000020,0x00000020,0x00000001,0x0004002b,0x00000020,0x00000021,
	0x00000002,0x0004002b,0x00000020,0x00000023,0x00000000,0x00040020,0x00000024,0x00000002,
	0x00000016,0x00040020,0x00000027,0x00000007,0x00000019,0x0004002b,0x00000020,0x0000002a,
	0x00000001,0x00040020,0x0000002b,0x00000002,0x0000001a,0x0004002b,0x00000016,0x00000030,
	0x3f800000,0x00040020,0x00000031,0x00000002,0x00000006,0x0004002b,0x00000006,0x00000042,
	0x00000001,0x0003001d,0x00000049,0x0000001a,0x0003001e,0x0000004a,0x00000049,0x00040020,
	0x0000004b,0x00000002,0x0000004a,0x0004003b,0x0000004b,0x0000004c,0x00000002,0x0004002b,
	0x00000006,0x0000004e,0x00000004,0x0004002b,0x00000016,0x00000052,0x00000000,0x0004002b,
	0x00000006,0x00000054,0x00000002,0x0004002b,0x00000006,0x00000057,0x00000003,0x0006002c,
	0x00000009,0x00000077,0x0000004e,0x00000042,0x00000042,0x0004002b,0x00000006,0x00000078,
	0x00000005,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
	0x0004003b,0x00000007,0x00000008,0x00000007,0x0004003b,0x00000017,0x00000018,0x00000007,
	0x0004003b,0x00000027,0x00000028,0x00000007,0x0004003b,0x00000017,0x0000002f,0x00000007,
	0x0004003b,0x00000017,0x00000036,0x00000007,0x0004003b,0x00000017,0x0000003a,0x00000007,
	0x0004003b,0x00000017,0x00000041,0x00000007,0x00050041,0x0000000d,0x0000000e,0x0000000b,
	0x0000000c,0x0004003d,0x00000006,0x0000000f,0x0000000e,0x0003003e,0x00000008,0x0000000f,
	0x0004003d,0x00000006,0x00000010,0x00000008,0x000500b0,0x00000012,0x00000013,0x00000010,
	0x00000011,0x000300f7,0x00000015,0x00000000,0x000400fa,0x00000013,0x00000014,0x00000015,
	0x000200f8,0x00000014,0x0004003d,0x00000006,0x00000022,0x00000008,0x00080041,0x00000024,
	0x00000025,0x0000001f,0x00000021,0x00000022,0x00000023,0x0000000c,0x0004003d,0x00000016,
	0x00000026,0x00000025,0x0003003e,0x00000018,0x00000026,0x0004003d,0x00000006,0x00000029,
	0x00000008,0x00070041,0x0000002b,0x0000002c,0x0000001f,0x00000021,0x00000029,0x0000002a,
	0x0004003d,0x0000001a,0x0000002d,0x0000002c,0x0007004f,0x00000019,0x0000002e,0x0000002d,
	0x0000002d,0x00000002,0x00000003,0x0003003e,0x00000028,0x0000002e,0x00050041,0x00000031,
	0x00000032,0x0000001f,0x00000023,0x0004003d,0x00000006,0x00000033,0x00000032,0x00040070,
	0x00000016,0x00000034,0x00000033,0x00050088,0x00000016,0x00000035,0x00000030,0x00000034,
	0x0003003e,0x0000002f,0x00000035,0x0004003d,0x00000016,0x00000037,0x00000018,0x0004003d,
	0x00000016,0x00000038,0x0000002f,0x00050085,0x00000016,0x00000039,0x00000037,0x00000038,
	0x0003003e,0x00000036,0x00000039,0x0004003d,0x00000016,0x0000003b,0x00000018,0x00050041,
	0x00000017,0x0000003c,0x00000028,0x0000000c,0x0004003d,0x00000016,0x0000003d,0x0000003c,
	0x00050081,0x00000016,0x0000003e,0x0000003b,0x0000003d,0x0004003d,0x00000016,0x0000003f,
	0x0000002f,0x00050085,0x00000016,0x00000040,0x0000003e,0x0000003f,0x0003003e,0x0000003a,
	0x00000040,0x00050041,0x00000017,0x00000043,0x00000028,0x00000042,0x0004003d,0x00000016,
	0x00000044,0x00000043,0x00050041,0x00000031,0x00000045,0x0000001f,0x0000002a,0x0004003d,
	0x00000006,0x00000046,0x00000045,0x00040070,0x00000016,0x00000047,0x00000046,0x00050088,
	0x00000016,0x00000048,0x00000044,0x00000047,0x0003003e,0x00000041,0x00000048,0x0004003d,
	0x00000006,0x0000004d,0x00000008,0x00050084,0x00000006,0x0000004f,0x0000004d,0x0000004e,
	0x00050080,0x00000006,0x00000050,0x0000004f,0x0000000c,0x0004003d,0x00000016,0x00000051,
	0x00000036,0x00050050,0x00000019,0x00000053,0x00000051,0x00000052,0x00070041,0x00000024,
	0x00000055,0x0000004c,0x00000023,0x00000050,0x00000054,0x00050051,0x00000016,0x00000056,
	0x00000053,0x00000000,0x0003003e,0x00000055,0x00000056,0x00070041,0x00000024,0x00000058,
	0x0000004c,0x00000023,0x00000050,0x00000057,0x00050051,0x00000016,0x00000059,0x00000053,
	0x00000001,0x0003003e,0x00000058,0x00000059,0x0004003d,0x00000006,0x0000005a,0x00000008,
	0x00050084,0x00000006,0x0000005b,0x0000005a,0x0000004e,0x00050080,0x00000006,0x0000005c,
	0x0000005b,0x00000042,0x0004003d,0x00000016,0x0000005d,0x0000003a,0x00050050,0x00000019,
	0x0000005e,0x0000005d,0x00000052,0x00070041,0x00000024,0x0000005f,0x0000004c,0x00000023,
	0x0000005c,0x00000054,0x00050051,0x00000016,0x00000060,0x0000005e,0x00000000,0x0003003e,
	0x0000005f,0x00000060,0x00070041,0x00000024,0x00000061,0x0000004c,0x00000023,0x0000005c,
	0x00000057,0x00050051,0x00000016,0x00000062,0x0000005e,0x00000001,0x0003003e,0x00000061,
	0x00000062,0x0004003d,0x00000006,0x00000063,0x00000008,0x00050084,0x00000006,0x00000064,
	0x00000063,0x0000004e,0x00050080,0x00000006,0x00000065,0x00000064,0x00000054,0x0004003d,
	0x00000016,0x00000066,0x0000003a,0x0004003d,0x00000016,0x00000067,0x00000041,0x00050050,
	0x00000019,0x00000068,0x00000066,0x00000067,0x00070041,0x00000024,0x00000069,0x0000004c,
	0x00000023,0x00000065,0x00000054,0x00050051,0x00000016,0x0000006a,0x00000068,0x00000000,
	0x0003003e,0x00000069,0x0000006a,0x00070041,0x00000024,0x0000006b,0x0000004c,0x00000023,
	0x00000065,0x00000057,0x00050051,0x00000016,0x0000006c,0x00000068,0x00000001,0x0003003e,
	0x0000006b,0x0000006c,0x0004003d,0x00000006,0x0000006d,0x00000008,0x00050084,0x00000006,
	0x0000006e,0x0000006d,0x0000004e,0x00050080,0x00000006,0x0000006f,0x0000006e,0x00000057,
	0x0004003d,0x00000016,0x00000070,0x00000036,0x0004003d,0x00000016,0x00000071,0x00000041,
	0x00050050,0x00000019,0x00000072,0x00000070,0x00000071,0x00070041,0x00000024,0x00000073,
	0x0000004c,0x00000023,0x0000006f,0x00000054,0x00050051,0x00000016,0x00000074,0x00000072,
	0x00000000,0x0003003e,0x00000073,0x00000074,0x00070041,0x00000024,0x00000075,0x0000004c,
	0x00000023,0x0000006f,0x00000057,0x00050051,0x00000016,0x00000076,0x00000072,0x00000001,
	0x0003003e,0x00000075,0x00000076,0x000200f9,0x00000015,0x000200f8,0x00000015,0x000100fd,
	0x00010038
};

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

    VkSampler fontTextureSampler;
    if(vkCreateSampler(device, &samplerInfo, nullptr, &fontTextureSampler) != VK_SUCCESS) {
        printf("Failed to create font texture sampler!");
    }

    // Partial descriptor set setup to satisfy pipeline creation
    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &fontTextureSampler },
        { 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MaxFontCount, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    constexpr VkDescriptorPoolSize poolSizes[] = {
        // type; descriptorCount;
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MaxFontCount},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MaxFontCount + 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MaxFontCount},
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.poolSizeCount = std::size(poolSizes);
    poolInfo.maxSets = 2;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
    {
        printf("Failed to create descriptor pool!");
    }

    VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
    setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutInfo.pBindings = bindings;
    setLayoutInfo.bindingCount = std::size(bindings);
    if (vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
    {
        printf("Failed to create descriptor set layout!");
    }

    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = m_descriptorPool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &m_descriptorSetLayout;
    if (vkAllocateDescriptorSets(device, &setAllocInfo, &m_descriptorSet) != VK_SUCCESS)
    {
        printf("Failed to allocate descriptor sets!");
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
    computeSetLayoutInfo.bindingCount = std::size(computeBindings);
    if (vkCreateDescriptorSetLayout(device, &computeSetLayoutInfo, nullptr, &m_computeDescriptorSetLayout) != VK_SUCCESS)
    {
        printf("Failed to create compute descriptor set layout!");
    }

    VkDescriptorSetAllocateInfo computeSetAllocInfo{};
    computeSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    computeSetAllocInfo.descriptorPool = m_descriptorPool;
    computeSetAllocInfo.descriptorSetCount = 1;
    computeSetAllocInfo.pSetLayouts = &m_computeDescriptorSetLayout;
    if (vkAllocateDescriptorSets(device, &computeSetAllocInfo, &m_computeDescriptorSet) != VK_SUCCESS)
    {
        printf("Failed to allocate compute descriptor sets!");
    }

    VkDescriptorImageInfo imageInfo{};

    if(!EnabledFeatures::NullDescriptor)
    {
        /* Binding 0 == Sampler, Binding 1-MaxFontTextures == Font Texture */
        ImageLoadInfo loadInfo{};
        loadInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        loadInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        loadInfo.pPath = "../Assets/error.jpeg";
        Image fallbackImage;
        Images->LoadImage(&loadInfo, &fallbackImage, IMAGE_LOAD_TYPE_FROM_DISK);

        errorImage = fallbackImage.image;
        errorImageView = fallbackImage.view;
        errorImageMemory = fallbackImage.memory;
        
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = errorImageView;
        imageInfo.sampler = VK_NULL_HANDLE;
    } else {
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.imageView = VK_NULL_HANDLE;
        imageInfo.sampler = VK_NULL_HANDLE;
    }

    VkWriteDescriptorSet writeSets[MaxFontCount] = {};
    for(u32 i = 0; i < MaxFontCount; i++)
    {
        writeSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSets[i].dstSet = m_descriptorSet;
        writeSets[i].dstBinding = 1;
        writeSets[i].dstArrayElement = i;
        writeSets[i].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
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
    // commandPoolInfo.queueFamilyIndex = Graphics->ComputeFamilyIndex;
    // commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    // vkCreateCommandPool(device, &commandPoolInfo, nullptr, &m_cmdPool);
    // VkCommandBufferAllocateInfo commandBufferInfo{};

    // commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // commandBufferInfo.commandPool = m_cmdPool;
    // commandBufferInfo.commandBufferCount = MaxFramesInFlight;
    // vkAllocateCommandBuffers(device, &commandBufferInfo, computeCmdBuffers);

    // VkCommandBufferAllocateInfo stagingCopyInfo{};
    // stagingCopyInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // stagingCopyInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // stagingCopyInfo.commandPool = m_cmdPool;
    // stagingCopyInfo.commandBufferCount = 1;
    // vkAllocateCommandBuffers(device, &stagingCopyInfo, &stagingCmdBuffer);

    namespace files = boost::filesystem;
    std::thread([&]() {
        time_t vertexShaderLastWriteTime = files::last_write_time("Shaders/vert.text.spv");
        time_t fragmentShaderLastWriteTime = files::last_write_time("Shaders/frag.text.spv");

        while(true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            time_t vertexShaderWriteTime = files::last_write_time("Shaders/vert.text.spv");
            time_t fragmentShaderWriteTime = files::last_write_time("Shaders/frag.text.spv");
            if(vertexShaderWriteTime != vertexShaderLastWriteTime || fragmentShaderWriteTime != fragmentShaderLastWriteTime)
            {
                vertexShaderLastWriteTime = vertexShaderWriteTime;
                fragmentShaderLastWriteTime = fragmentShaderWriteTime;
                CreatePipeline(Graphics->GlobalRenderPass);
            }
        }
    }).detach();
    
    TIME_FUNCTION(CreatePipeline(Graphics->GlobalRenderPass));
}

void TextRendererSingleton::BeginRender()
{
    const f32 windowWidth = (f32)Graphics->RenderArea.x;
    const f32 windowHeight = (f32)Graphics->RenderArea.y;
    const f32 halfWidth = windowWidth / 2.0f;
    const f32 halfHeight = windowHeight / 2.0f;

    projection = glm::ortho(-halfWidth,halfWidth, -halfHeight, halfHeight, 0.0f, 1.0f);
}

void TextRendererSingleton::Render(TextRenderInfo& info, const NanoFont *font)
{
    const auto& draw = info;
    if(draw.line.size() == 0)
        return;

    u32 renderableCharCount = 0;
    for(const auto& c : draw.line)
        if(c != '\n')
            renderableCharCount++;

    const u32 prevCharsDrawn = charsDrawn;

    const std::string& text = draw.line;
    const f32 scale = draw.scale;
    const NanoTextAlignmentHorizontal horizontal = draw.horizontal;
    const NanoTextAlignmentVertical vertical = draw.vertical;
    const f32 x = draw.x;
    f32 y = draw.y;

    const std::vector<std::string> splitLines = SplitString(text);

    switch(vertical)
    {
    case TEXT_VERTICAL_ALIGN_CENTER:
        y = y - ((font->bmpHeight * scale) * splitLines.size()) / 2.0f;
        break;
    case TEXT_VERTICAL_ALIGN_BOTTOM:
        y = y - (font->bmpHeight * splitLines.size()) * scale;
        break;
    case TEXT_VERTICAL_ALIGN_TOP:
        y = y - font->lineHeight * scale;
        break;
    default:
        printf("Invalid vertical alignment. Specified (int)%u. (Implement?)\n", vertical);
        break;
    }

    // Reallocate buffers, stream data to GPU
    for (const auto& line : splitLines)
        RenderLine(x, &y, line, horizontal, scale, font);

    if(!drawList.empty() && drawList.back().font->fontIndex == font->fontIndex)
    {
        drawList.back().indexCount += renderableCharCount * 6;
    }
    else
    {
        Draw drawCmd{};
        drawCmd.font = font;
        drawCmd.indexCount = renderableCharCount * 6;
        drawCmd.vertexOffset = prevCharsDrawn * 4;
        drawCmd.scale = scale;
        drawList.push_back(drawCmd);
    }
}

void TextRendererSingleton::RenderLine(const f32 x, f32 *y, const std::string_view line, const NanoTextAlignmentHorizontal horizontal, const f32 scale, const NanoFont* font)
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
        printf("Invalid horizontal alignment. Specified (int)%u. (Implement?)\n", horizontal);
        break;
    }
    
    vec4 vv[line.size() * 4];
    u16 ii[line.size() * 6];

    // #pragma omp parallel for
    for (u32 i = 0; i < line.size(); i++)
    {
        const char& c = line[i];
        const Character &ch = font->characters[c];

        if(c == '\n') {
            // Newline, pushes the character to the next line
            *y += font->bmpHeight * scale;
            lineX = 0.0f;
            continue;
        }

        const vec2 chBearing = glm::unpackHalf2x16(ch.bearing);
        const vec2 chSize = glm::unpackHalf2x16(ch.size);

        const f32 xpos = lineX + chBearing.x * scale;
        const f32 ypos = *y + (font->bmpHeight - chBearing.y - font->lineHeight) * scale;

        const f32 scaledWidth = chSize.x * scale;
        const f32 scaledHeight = chSize.y * scale;

        const u32 vertexIndex = i * 4;
        vv[vertexIndex + 0] = vec4(xpos,               ypos               , font->texturePositions[(c * 4) + 0]);
        vv[vertexIndex + 1] = vec4(xpos + scaledWidth, ypos               , font->texturePositions[(c * 4) + 1]);
        vv[vertexIndex + 2] = vec4(xpos + scaledWidth, ypos + scaledHeight, font->texturePositions[(c * 4) + 2]);
        vv[vertexIndex + 3] = vec4(xpos,               ypos + scaledHeight, font->texturePositions[(c * 4) + 3]);
        
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

    *y += font->bmpHeight * scale;
}

void TextRendererSingleton::EndRender(VkCommandBuffer cmd, const glm::mat2 &matrix)
{
    const u32 vertexByteSize = vertices.size() * sizeof(vec4);
    const u32 indexByteSize = indices.size() * sizeof(u16);
    const u32 bufferSize = vertexByteSize + indexByteSize;

    // If the buffer is this much larger than needed, make it smaller.
    constexpr u8 BufferSizeDownscale = 3;
    // Allocate this much times more than we need to avoid further allocations. 2 is good enough.
    constexpr u8 BufferSizeUpscale = 2;

    static_assert(BufferSizeUpscale < BufferSizeDownscale && "Can cause undefined behaviour due to upscaling "
                                                             "the buffer one frame and immediately downscaling it in the other");

    const bool needLargerBuffer = bufferSize > allocatedMemorySize;
    const bool needSmallerBuffer = bufferSize * BufferSizeDownscale < allocatedMemorySize;

    if (bufferSize == 0)
        return;

    if (needLargerBuffer || needSmallerBuffer)
    {
        const VkBuffer back_unifiedBuffer = unifiedBuffer;
        const VkDeviceMemory back_mem = mem;

        (needLargerBuffer) ?
            allocatedMemorySize = bufferSize * BufferSizeUpscale
            :
            allocatedMemorySize = std::max(bufferSize, allocatedMemorySize / BufferSizeDownscale);

        vkDeviceWaitIdle(device);

        vkDestroyBuffer(device, back_unifiedBuffer, nullptr);
        vkFreeMemory(device, back_mem, nullptr);

        help::Buffers::CreateBuffer(
            allocatedMemorySize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &unifiedBuffer, &mem,
            false
        );

        VkMemoryRequirements UBmemoryRequirements;
        vkGetBufferMemoryRequirements(device, unifiedBuffer, &UBmemoryRequirements);
        alignment = UBmemoryRequirements.alignment;
    }

    ibOffset = align(vertexByteSize, alignment);

    uchar *vbMap;
    if (vkMapMemory(device, mem, 0, allocatedMemorySize, 0, reinterpret_cast<void **>(&vbMap)) != VK_SUCCESS)
    {
        printf("TextRenderer::Failed to map vertex buffer memory");
    }

    memcpy(vbMap, vertices.data(), vertexByteSize);
    memcpy(vbMap + ibOffset, indices.data(), indexByteSize);

    vkUnmapMemory(device, mem);

    // if(bufferCopyThread.joinable())
    //     bufferCopyThread.join();


    constexpr VkDeviceSize VBOffset = 0;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);
    vkCmdBindVertexBuffers(cmd, 0, 1, &unifiedBuffer, &VBOffset);
    vkCmdBindIndexBuffer(cmd, unifiedBuffer, ibOffset, VK_INDEX_TYPE_UINT16);

    const glm::mat2 mvp = projection * matrix;
    vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat2), &mvp);

    for(const auto& draw : drawList) {
        struct {
            uint textureIndex;
            float scale;
        } pc;
        pc.textureIndex = draw.font->fontIndex;
        pc.scale = draw.scale;

        vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(mat2), sizeof(pc), &pc);
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

void TextRendererSingleton::CreatePipeline(VkRenderPass renderPass) {
    VkShaderModule vertexShaderModule;
    VkShaderModule fragmentShaderModule;
    VkShaderModule computeShaderModule;

    u32 VertexShaderBinarySize, FragmentShaderBinarySize;
    help::Files::LoadBinary("./Shaders/vert.text.spv", nullptr, &VertexShaderBinarySize);
    u8 VertexShaderBinary[VertexShaderBinarySize];
    help::Files::LoadBinary("./Shaders/vert.text.spv", VertexShaderBinary, &VertexShaderBinarySize);

    help::Files::LoadBinary("./Shaders/frag.text.spv", nullptr, &FragmentShaderBinarySize);
    u8 FragmentShaderBinary[FragmentShaderBinarySize];
    help::Files::LoadBinary("./Shaders/frag.text.spv", FragmentShaderBinary, &FragmentShaderBinarySize);

    VkShaderModuleCreateInfo vertexShaderModuleInfo{};
    vertexShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderModuleInfo.codeSize = VertexShaderBinarySize;
    vertexShaderModuleInfo.pCode = reinterpret_cast<const u32*>(VertexShaderBinary);
    if (vkCreateShaderModule(device, &vertexShaderModuleInfo, nullptr, &vertexShaderModule) != VK_SUCCESS) {
        printf("TextRenderer::Failed to create vertex shader module!");
    }

    VkShaderModuleCreateInfo fragmentShaderModuleInfo{};
    fragmentShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentShaderModuleInfo.codeSize = FragmentShaderBinarySize;
    fragmentShaderModuleInfo.pCode = reinterpret_cast<const u32*>(FragmentShaderBinary);
    if (vkCreateShaderModule(device, &fragmentShaderModuleInfo, nullptr, &fragmentShaderModule) != VK_SUCCESS) {
        printf("TextRenderer::Failed to create fragment shader module!");
    }

    const std::vector<VkPushConstantRange> pushConstantRanges = {
        // stageFlags; offset; size;
        {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat2) + sizeof(u32) + sizeof(float)},
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        printf("TextRenderer::Failed to create pipeline layout!");
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
    pc.format = Graphics->SwapChainImageFormat;
    pc.subpass = 0;

    pc.renderPass = renderPass;
    pc.pipelineLayout = m_pipelineLayout;
    pc.pAttributeDescriptions = &attributeDescriptions;
    pc.pBindingDescriptions = &bindingDescriptions;
    pc.pDescriptorLayouts = &descriptorSetLayouts;
    pc.pShaderCreateInfos = &shaders;
    pc.pPushConstants = &pushConstantRanges;
    pc.extent = VkExtent2D({Graphics->RenderArea.x, Graphics->RenderArea.y});
    pc.pShaderCreateInfos = &shaders;
    pro::CreateGraphicsPipeline(device, &pc, &m_pipeline, PIPELINE_CREATE_FLAGS_ENABLE_BLEND);

    vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(device, fragmentShaderModule, nullptr);

    /* Compute shader create Info */
    
    VkShaderModuleCreateInfo computeShderModuleInfo{};
    computeShderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    computeShderModuleInfo.codeSize = sizeof(ComputeShaderBinary);
    computeShderModuleInfo.pCode = ComputeShaderBinary;
    if (vkCreateShaderModule(device, &computeShderModuleInfo, nullptr, &computeShaderModule) != VK_SUCCESS) {
        printf("TextRenderer::Failed to create fragment shader module!");
    }

    VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = computeShaderModule;
    computeShaderStageInfo.pName = "main";
    
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &m_computeDescriptorSetLayout;
    if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &m_computePipelineLayout) != VK_SUCCESS) {
        printf("TextRenderer::Failed to create pipeline layout!\n");
    }

    // Create compute pipeline
    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.stage = computeShaderStageInfo;
    computePipelineCreateInfo.layout = m_computePipelineLayout;
    
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_computePipeline) != VK_SUCCESS) {
        printf("TextRenderer::Failed to create compute pipeline!\n");
    }

    vkDestroyShaderModule(device, computeShaderModule, nullptr);
}

void TextRendererSingleton::RecreatePipeline()
{
    vkDestroyPipeline(device, m_pipeline, nullptr);
    vkDestroyPipeline(device, m_computePipeline, nullptr);

    vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
    vkDestroyPipelineLayout(device, m_computePipelineLayout, nullptr);
    
    CreatePipeline(Graphics->GlobalRenderPass);
}

void TextRendererSingleton::LoadFont(const char* path, u32 pixelSizes, NanoFont* dstFont)
{
    FT_Library lib;
    FT_Face face;

    if (FT_Init_FreeType(&lib))
    {
        printf("TextRenderer::Failed to initialize FreeType library\n");
    }

    if (FT_New_Face(lib, path, 0, &face))
    {
        printf("TextRenderer::Failed to load font file: %s\n", path);
        FT_Done_FreeType(lib);
    }

    FT_Set_Pixel_Sizes(face, 0, pixelSizes);

    auto& bmpWidth = dstFont->bmpWidth;
    auto& bmpHeight = dstFont->bmpHeight;
    bmpWidth = 0, bmpHeight = 0;

    uvec2 chSizes[ 128 ];
    u32 chOffsets[ 128 ];

    for (uchar c = 0; c < 128; c++)
    {
        auto res = FT_Load_Char(face, c, FT_LOAD_BITMAP_METRICS_ONLY);
        assert(res == 0);

        chSizes[c] = 
            glm::uvec2(
                face->glyph->metrics.width >> 6,
                face->glyph->metrics.height >> 6
            );
        chOffsets[c] = bmpWidth;

        Character character = {
            .size = glm::packHalf2x16(
                glm::uvec2(
                    face->glyph->metrics.width >> 6,
                    face->glyph->metrics.height >> 6
                )
            ),
            .bearing = glm::packHalf2x16(glm::vec2(face->glyph->bitmap_left, face->glyph->bitmap_top)),
            // .offsetAndAdvance = glm::packHalf2x16(
                // glm::vec2(
                    // static_cast<f32>(bmpWidth),
                    // static_cast<f32>(face->glyph->advance.x >> 6)
                // )
            // )
            .advance = static_cast<f32>(face->glyph->advance.x >> 6)
        };

        dstFont->characters[c] = character;

        bmpHeight = std::max(bmpHeight, face->glyph->bitmap.rows);
        bmpWidth += face->glyph->bitmap.width;
    };

    const f32 invBmpWidth = 1.0f / static_cast<f32>(bmpWidth);
    
    #pragma omp parallel for
    for(uchar i = 0; i < 128; i++)
    {
        const f32 chOffset = chOffsets[i];
        const vec2 chSize = chSizes[i];
        
        if(chSize.x == 0 || chSize.y == 0)
            continue;

        const f32 u0 = chOffset * invBmpWidth;
        const f32 u1 = (chOffset + chSize.x) * invBmpWidth;
        const f32 v = chSize.y / bmpHeight;

        dstFont->texturePositions[(i * 4) + 0] = vec2(u0, 0.0f);
        dstFont->texturePositions[(i * 4) + 1] = vec2(u1, 0.0f);
        dstFont->texturePositions[(i * 4) + 2] = vec2(u1, v   );
        dstFont->texturePositions[(i * 4) + 3] = vec2(u0, v   );
    }

    assert(bmpWidth != 0 && bmpHeight != 0);

    u8 buffer[bmpWidth * bmpHeight];

    u32 xpos = 0;
    for (uchar c = 0; c < 128; c++)
    {
        Character &ch = dstFont->characters[c];
        const vec2 chSize = glm::unpackHalf2x16(ch.size);

        const u32 width = chSize.x;
        const u32 height = chSize.y;

        if (width > 0)
        {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                printf("TextRenderer::Failed to load glyph:\n");
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
                    + the width of each row (bmpWidth).
                */
                memcpy(
                    buffer + xpos + row * bmpWidth,
                    charDataBuffer + row * width,
                    width
                );
            }
        }

        xpos += width;
    };

    dstFont->lineHeight = face->height / 64.0f;

    const i32 ftres = FT_Done_Face(face) && FT_Done_FreeType(lib);
    assert(ftres == 0);

    ImageLoadInfo fontImgLoadInfo{};
    fontImgLoadInfo.width = bmpWidth;
    fontImgLoadInfo.height = bmpHeight;
    fontImgLoadInfo.data = buffer;
    fontImgLoadInfo.format = VK_FORMAT_R8_UNORM;
    fontImgLoadInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    Image fontImg{};
    Images->LoadImage(&fontImgLoadInfo, &fontImg, IMAGE_LOAD_TYPE_FROM_MEMORY);

    dstFont->fontTextureView = fontImg.view;
    dstFont->fontTexture = fontImg.image;
    dstFont->fontTextureMemory = fontImg.memory;

    if(currFontIndex >= MaxFontCount)
        printf("TextRenderer::unsupported. can't handle more than %u fonts.", MaxFontCount);
    dstFont->fontIndex = currFontIndex;

    VkDescriptorImageInfo fontImageInfo{};
    fontImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    fontImageInfo.imageView = dstFont->fontTextureView;
    fontImageInfo.sampler = VK_NULL_HANDLE;

    VkWriteDescriptorSet writeSet{};
    writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeSet.dstSet = m_descriptorSet;
    writeSet.dstBinding = 1;
    writeSet.dstArrayElement = dstFont->fontIndex;
    writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    writeSet.descriptorCount = 1;
    writeSet.pImageInfo = &fontImageInfo;
    vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
}

void TextRendererSingleton::Destroy(NanoFont *obj)
{
    vkDeviceWaitIdle(device);
    vkDestroyImage(device, obj->fontTexture, nullptr);
    vkDestroyImageView(device, obj->fontTextureView, nullptr);
    vkFreeMemory(device, obj->fontTextureMemory, nullptr);

    if(!EnabledFeatures::NullDescriptor)
    {
        VkDescriptorImageInfo fontImageInfo{};
        fontImageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        fontImageInfo.sampler = VK_NULL_HANDLE;
        fontImageInfo.imageView = VK_NULL_HANDLE;

        VkWriteDescriptorSet writeSet{};
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = m_descriptorSet;
        writeSet.dstBinding = 1;
        writeSet.dstArrayElement = obj->fontIndex;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        writeSet.descriptorCount = 1;
        writeSet.pImageInfo = &fontImageInfo;
        vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
    }

    currFontIndex--;
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
