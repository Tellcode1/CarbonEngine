#ifndef __CGFX_MEMORY_H
#define __CGFX_MEMORY_H

// ! doesn't take into account alignment, etc.
// * This is so when we eventually have to switch to an allocator it's much easier

#include "cgfxstdafx.h"

typedef enum cgfx_memory_usage_bits {
    CGFX_MEMORY_USAGE_GPU_LOCAL = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    CGFX_MEMORY_USAGE_CPU_VISIBLE = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    CGFX_MEMORY_USAGE_CPU_READABLE = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // ? what are these two supposed to actually be??
    CGFX_MEMORY_USAGE_CPU_WRITEABLE = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    CGFX_MEMORY_USAGE_LAZILY_ALLOCATED = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
    // ext i think
    CGFX_MEMORY_USAGE_PROTECTED = VK_MEMORY_PROPERTY_PROTECTED_BIT,
    CGFX_MEMORY_USAGE_DEVICE_COHERENT = VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD,
    CGFX_MEMORY_USAGE_DEVICE_UNCACHED = VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD
} cgfx_memory_usage_bits;
typedef unsigned cgfx_memory_usage_t;

typedef struct cgfx_gpu_memory_t {
    VkDeviceMemory memory;
    cgfx_memory_usage_t usage;
    int size;
} cgfx_gpu_memory_t;

typedef struct cgfx_gpu_allocated_object_t {
    int size, memory_offset; // , alignment?
    cgfx_gpu_memory_t *memory;
} cgfx_gpu_allocated_object_t;

typedef struct cgfx_gpu_buffer_t {
    cgfx_gpu_allocated_object_t allocation;
    VkBuffer vkbuffer;
} cgfx_gpu_buffer_t;

typedef struct cgfx_gpu_image_create_info {
    cg_format format;
    cg_sample_count samples;
    VkImageType type;
    VkImageUsageFlags usage;
    VkExtent3D extent;
    int arraylayers;
    int miplevels;
} cgfx_gpu_image_create_info;

typedef struct cgfx_gpu_image_view_create_info {
    cg_format format; // May be CFMT_UNKNOWN for automatic fetching of format from image
    VkImageViewType view_type;
    VkImageSubresourceRange subresourceRange;
} cgfx_gpu_image_view_create_info;

typedef struct cgfx_gpu_image_t {
    cgfx_gpu_allocated_object_t allocation;
    VkImageLayout layout;
    VkImageAspectFlags aspect;
    VkImage image;
    VkImageView view;
    VkExtent3D extent;
    int miplevels, arraylayers;
    cg_format format;
    cg_sample_count samples;
    bool_t is_cubemap, is_render_texture;
} cgfx_gpu_image_t;

typedef struct cgfx_gpu_sampler_create_info {
    VkFilter filter;
    VkSamplerMipmapMode mipmap_mode;
    VkSamplerAddressMode address_mode;
    f32 max_anisotropy;
    f32 mip_lod_bias, min_lod, max_lod;
} cgfx_gpu_sampler_create_info;

typedef struct cgfx_gpu_sampler_t {
    VkFilter filter;
    VkSamplerMipmapMode mipmap_mode;
    VkSamplerAddressMode address_mode;
    f32 max_anisotropy;
    f32 mip_lod_bias, min_lod, max_lod;
    VkSampler vksampler;
} cgfx_gpu_sampler_t;

// I think we should make like a cgfx_err_t enum

// these parameters should be replaced
// properties should be replaced by usage. Like CGFX_MEMORY_USAGE_GPU, CPU_TO_GPU, GPU_TO_CPU, etc.
static void inline cgfx_gpu_memory_allocate(VkDeviceSize size, cgfx_memory_usage_t usage, cgfx_gpu_memory_t *dst) {
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

static inline void cgfx_gpu_create_buffer(int size, VkBufferUsageFlags usage, cgfx_gpu_buffer_t *dst) {
    dst->allocation.size = size;
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    vkCreateBuffer(device, &buffer_info, NULL, &dst->vkbuffer);
}

static inline void cgfx_gpu_memory_free(cgfx_gpu_memory_t *mem) {
    vkFreeMemory(device, mem->memory, NULL);
}

// I think these given an error when, say offset is too big so maybe we can check their return values?
static inline void cgfx_gpu_memory_bind_buffer(cgfx_gpu_memory_t *mem, int offset, cgfx_gpu_buffer_t *buffer) {
    buffer->allocation.memory = mem;
    buffer->allocation.memory_offset = offset;
    vkBindBufferMemory(device, buffer->vkbuffer, mem->memory, offset);
}
static inline void cgfx_gpu_memory_bind_image(cgfx_gpu_memory_t *mem, int offset, cgfx_gpu_image_t *image) {
    image->allocation.memory = mem;
    image->allocation.memory_offset = offset;
    vkBindImageMemory(device, image->image, mem->memory, offset);
}

// I think they should be renamed to like buffer_destroy() because they aren't freeing the memory, are they?
static inline void cgfx_gpu_buffer_free(cgfx_gpu_buffer_t *buffer) {
    if (buffer->vkbuffer) {
        vkDestroyBuffer(device, buffer->vkbuffer, NULL);
    }
}

static inline void cgfx_gpu_image_free(cgfx_gpu_image_t *image) {
    if (image->image) {
        vkDestroyImage(device, image->image, NULL);
        vkDestroyImageView(device, image->view, NULL);
    }
}

static inline void cgfx_gpu_create_sampler(const cgfx_gpu_sampler_create_info *pInfo, cgfx_gpu_sampler_t *sampler) {
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

static inline void cgfx_gpu_create_image(const cgfx_gpu_image_create_info *pInfo, cgfx_gpu_image_t *image) {
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = pInfo->type;
    imageCreateInfo.extent = pInfo->extent;
    imageCreateInfo.mipLevels = pInfo->miplevels;
    imageCreateInfo.arrayLayers = pInfo->arraylayers;
    imageCreateInfo.format = cfmt_conv_cfmt_to_vkfmt(pInfo->format);
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = pInfo->usage;
    imageCreateInfo.samples = (VkSampleCountFlagBits)pInfo->samples;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageCreateInfo, NULL, &image->image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create image");
    }
}

// vkimage is fetched from image!
static inline void cgfx_gpu_create_image_view(const cgfx_gpu_image_view_create_info *pInfo, cgfx_gpu_image_t *image) {
    const cg_format dst_format = (pInfo->format != CFMT_UNKNOWN) ? pInfo->format : image->format;
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.viewType = pInfo->view_type;
    view_info.format = cfmt_conv_cfmt_to_vkfmt(dst_format);
    view_info.image = image->image;
    view_info.subresourceRange = pInfo->subresourceRange;
    vkCreateImageView(device, &view_info, NULL, &image->view);
}

static inline void cgfx_gpu_image_transition_layout(VkCommandBuffer cmd, cgfx_gpu_image_t *image, VkImageLayout new_layout) {
    // vkh_transition_image_layout(cmd, image->image, image->miplevels, image->aspect, image->layout, new_layout, );
}

#endif//__CGFX_MEMORY_H