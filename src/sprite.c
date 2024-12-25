#include "../include/sprite.h"
#include "../include/lunaVK.h"
#include "../include/lunaImage.h"
#include "../include/lunaGFX.h"
#include "../include/lunaDescriptors.h"

typedef struct sprite_t {
    int w,h;
    VkImage image;
    VkImageView view;
    VkDeviceMemory mem;
    luna_GPU_Sampler *sampler;
    luna_DescriptorSet *set;
} sprite_t;

sprite_t *sprite_empty = NULL;

sprite_t *sprite_load_mem(const unsigned char *data, int w, int h, lunaFormat fmt)
{
    sprite_t *spr = calloc(1, sizeof(struct sprite_t));

    fmt = luna_VK_GetSupportedFormatForDraw(fmt);

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent = (VkExtent3D){ w, h, 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = luna_lunaFormatToVKFormat(fmt);
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    cassert(vkCreateImage(device, &imageCreateInfo, NULL, &spr->image) == VK_SUCCESS);

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, spr->image, &imageMemoryRequirements);

    const u32 localDeviceMemoryIndex = luna_VK_GetMemType(imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    const VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = imageMemoryRequirements.size,
        .memoryTypeIndex = localDeviceMemoryIndex,
    };

    cassert(vkAllocateMemory(device, &allocInfo, NULL, &spr->mem) == VK_SUCCESS);
    vkBindImageMemory(device, spr->image, spr->mem, 0);

    luna_VK_StageImageTransfer(spr->image, data, w, h, w * h * vk_fmt_get_bpp(luna_lunaFormatToVKFormat(fmt)));

    const VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = spr->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = luna_lunaFormatToVKFormat(fmt),
        .subresourceRange = (VkImageSubresourceRange){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };
    cassert(vkCreateImageView(device, &view_info, NULL, &spr->view) == VK_SUCCESS);

    luna_GPU_SamplerCreateInfo sampler_info = {
        .filter = VK_FILTER_NEAREST,
        .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .max_anisotropy = 1.0f,
        .mip_lod_bias = 0.0f,
        .min_lod = 0.0f,
        .max_lod = VK_LOD_CLAMP_NONE,
    };
    luna_GPU_CreateSampler(&sampler_info, &spr->sampler);

    VkDescriptorSetLayoutBinding binding = (VkDescriptorSetLayoutBinding) {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    luna_AllocateDescriptorSet(&g_pool, &binding, 1, &spr->set);

    VkDescriptorImageInfo desc_img = {
        .sampler = luna_GPU_SamplerGet(spr->sampler),
        .imageView = spr->view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = spr->set->set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &desc_img,
    };
    luna_DescriptorSetSubmitWrite(spr->set, &write);
    
    return spr;
}

sprite_t *sprite_load_disk(const char *path)
{
    luna_Image tex = luna_ImageLoad(path);
    sprite_t *spr = sprite_load_mem(tex.data, tex.w, tex.h, tex.fmt);
    free(tex.data);
    return spr;
}

void sprite_get_dimensions(const sprite_t *spr, int *w, int *h)
{
    if (w) { *w = spr->w; }
    if (h) { *h = spr->h; }
}

VkImage sprite_get_internal(const sprite_t *spr)
{
    return spr->image;
}

VkImageView sprite_get_internal_view(const sprite_t *spr)
{
    return spr->view;
}

VkDescriptorSet sprite_get_descriptor(const sprite_t *spr)
{
    return spr->set->set;
}

VkSampler sprite_get_sampler(const sprite_t *spr)
{
    return luna_GPU_SamplerGet(spr->sampler);
}