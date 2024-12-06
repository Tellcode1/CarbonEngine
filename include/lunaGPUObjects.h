#ifndef __LUNA_GPU_MEMORY_H
#define __LUNA_GPU_MEMORY_H

// ! doesn't take into account alignment, etc.
// * This is so when we eventually have to switch to an allocator it's much easier

#include "lunaGFXstdafx.h"

typedef enum luna_GPU_MemoryUsageBits {
    LUNA_GPU_MEMORY_USAGE_GPU_LOCAL = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    LUNA_GPU_MEMORY_USAGE_CPU_VISIBLE = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    LUNA_GPU_MEMORY_USAGE_CPU_WRITEABLE = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    LUNA_GPU_MEMORY_USAGE_LAZILY_ALLOCATED = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
} luna_GPU_MemoryUsageBits;
typedef unsigned luna_GPU_MemoryUsage;

typedef struct luna_GPU_Memory {
    VkDeviceMemory memory;
    luna_GPU_MemoryUsage usage;
    int size;
} luna_GPU_Memory;

typedef struct luna_GPU_AllocatedObject {
    int size, memory_offset; // , alignment?
    luna_GPU_Memory *memory;
} luna_GPU_AllocatedObject;

typedef struct luna_GPU_Buffer {
    VkBuffer vkbuffer;
    luna_GPU_AllocatedObject allocation;
} luna_GPU_Buffer;

typedef struct luna_GPU_TextureCreateInfo {
    VkFormat format;
    lunaSampleCount samples;
    VkImageType type;
    VkImageUsageFlags usage;
    VkExtent3D extent;
    int arraylayers;
    int miplevels;
} luna_GPU_TextureCreateInfo;

typedef struct luna_GPU_TextureViewCreateInfo {
    VkFormat format; // May be VK_FORMAT_UNDEFINED for automatic fetching of format from image
    VkImageViewType view_type;
    VkImageSubresourceRange subresourceRange;
} luna_GPU_TextureViewCreateInfo;

typedef struct luna_GPU_Texture {
    luna_GPU_AllocatedObject allocation;
    VkImageLayout layout;
    VkImageAspectFlags aspect;
    VkImage image;
    VkImageView view;
    VkExtent3D extent;
    int miplevels, arraylayers;
    VkFormat format;
    lunaSampleCount samples;
    bool is_cubemap, is_render_texture;
} luna_GPU_Texture;

typedef struct luna_GPU_SamplerCreateInfo {
    VkFilter filter;
    VkSamplerMipmapMode mipmap_mode;
    VkSamplerAddressMode address_mode;
    f32 max_anisotropy;
    f32 mip_lod_bias, min_lod, max_lod;
} luna_GPU_SamplerCreateInfo;

typedef struct luna_GPU_Sampler {
    VkFilter filter;
    VkSamplerMipmapMode mipmap_mode;
    VkSamplerAddressMode address_mode;
    f32 max_anisotropy;
    f32 mip_lod_bias, min_lod, max_lod;
    VkSampler vksampler;
} luna_GPU_Sampler;

// I think we should make like a cgfx_err_t enum

// these parameters should be replaced
// properties should be replaced by usage. Like LUNA_GPU_MEMORY_USAGE_GPU, CPU_TO_GPU, GPU_TO_CPU, etc.
inline static void luna_GPU_AllocateMemory(VkDeviceSize size, luna_GPU_MemoryUsage usage, luna_GPU_Memory *dst) {
    dst->size = size;
    dst->usage = usage;
    const VkMemoryPropertyFlags properties = (VkMemoryPropertyFlags)usage;

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = size;

    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &mem_properties);

    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((properties & mem_properties.memoryTypes[i].propertyFlags) == properties) {
            alloc_info.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(device, &alloc_info, NULL, &dst->memory) != VK_SUCCESS || !dst->memory) {
        printf("An allocation has failed! What are we to do???\n");
    }
}

inline static void luna_GPU_CreateBuffer(int size, VkBufferUsageFlags usage, luna_GPU_Buffer *dst) {
    dst->allocation.size = size;
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    vkCreateBuffer(device, &buffer_info, NULL, &dst->vkbuffer);
}

inline static void luna_GPU_FreeMemory(luna_GPU_Memory *mem) {
    vkFreeMemory(device, mem->memory, NULL);
}

// I think these given an error when, say offset is too big so maybe we can check their return values?
inline static void luna_GPU_BindBufferToMemory(luna_GPU_Memory *mem, int offset, luna_GPU_Buffer *buffer) {
    buffer->allocation.memory = mem;
    buffer->allocation.memory_offset = offset;
    vkBindBufferMemory(device, buffer->vkbuffer, mem->memory, offset);
}
inline static void luna_GPU_BindImageToMemory(luna_GPU_Memory *mem, int offset, luna_GPU_Texture *tex) {
    tex->allocation.memory = mem;
    tex->allocation.memory_offset = offset;
    vkBindImageMemory(device, tex->image, mem->memory, offset);
}

inline static void luna_GPU_DestroyBuffer(luna_GPU_Buffer *buffer) {
    if (buffer->vkbuffer) {
        vkDestroyBuffer(device, buffer->vkbuffer, NULL);
    } else {
        LOG_INFO("Attempt to destroy a buffer which is NULL");
    }
}

inline static void luna_GPU_DestroyImage(luna_GPU_Texture *tex) {
    if (tex->image) {
        vkDestroyImage(device, tex->image, NULL);
        vkDestroyImageView(device, tex->view, NULL);
    } else {
        LOG_INFO("Attempt to destroy an image which is NULL");
    }
}

inline static void luna_GPU_CreateSampler(const luna_GPU_SamplerCreateInfo *pInfo, luna_GPU_Sampler *sampler) {
    sampler->filter = pInfo->filter;
    sampler->mipmap_mode = pInfo->mipmap_mode;
    sampler->address_mode = pInfo->address_mode;
    sampler->max_anisotropy = pInfo->max_anisotropy;
    sampler->mip_lod_bias = pInfo->mip_lod_bias;
    sampler->min_lod = pInfo->min_lod;
    sampler->max_lod = pInfo->max_lod;

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = pInfo->filter;
    samplerInfo.minFilter = pInfo->filter;
    samplerInfo.mipmapMode = pInfo->mipmap_mode;
    samplerInfo.addressModeU = pInfo->address_mode;
    samplerInfo.addressModeV = pInfo->address_mode;
    samplerInfo.addressModeW = pInfo->address_mode;
    samplerInfo.anisotropyEnable = pInfo->max_anisotropy > 1.0f;
    samplerInfo.maxLod = pInfo->max_lod;
    samplerInfo.minLod = pInfo->min_lod;
    cassert(vkCreateSampler(device, &samplerInfo, NULL, &sampler->vksampler) == VK_SUCCESS);
}

inline static void luna_GPU_CreateTexture(const luna_GPU_TextureCreateInfo *pInfo, luna_GPU_Texture *tex) {
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = pInfo->type;
    imageCreateInfo.extent = pInfo->extent;
    imageCreateInfo.mipLevels = pInfo->miplevels;
    imageCreateInfo.arrayLayers = pInfo->arraylayers;
    imageCreateInfo.format = pInfo->format;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = pInfo->usage;
    imageCreateInfo.samples = (VkSampleCountFlagBits)pInfo->samples;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageCreateInfo, NULL, &tex->image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create image");
    }
}

// vkimage is fetched from image!
inline static void luna_GPU_CreateTextureView(const luna_GPU_TextureViewCreateInfo *pInfo, luna_GPU_Texture *tex) {
    const VkFormat dst_format = (pInfo->format != VK_FORMAT_UNDEFINED) ? pInfo->format : tex->format;
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = tex->image,
        .viewType = pInfo->view_type,
        .format = dst_format,
        .subresourceRange = pInfo->subresourceRange,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };
    cassert(vkCreateImageView(device, &view_info, NULL, &tex->view) == VK_SUCCESS);
}

#endif//__LUNA_GPU_MEMORY_H