#ifndef __LUNA_OBJECT_DEF_H__
#define __LUNA_OBJECT_DEF_H__

#include "lunaGFX.h"
#include "../GPU/format.h"
#include "../containers/cgvector.h"

extern cg_vector_t g_Samplers;

typedef struct luna_GPU_Sampler {
    VkFilter filter;
    VkSamplerMipmapMode mipmap_mode;
    VkSamplerAddressMode address_mode;
    float max_anisotropy;
    float mip_lod_bias, min_lod, max_lod;
    VkSampler vksampler;
} luna_GPU_Sampler;

typedef struct luna_GPU_Texture {
    luna_GPU_Memory *memory;
    int size,offset;

    VkImageLayout layout;
    VkImageAspectFlags aspect;
    VkImageType type;
    VkImageUsageFlags usage;

    VkImage image;
    VkImageView view;
    VkExtent3D extent;
    int miplevels, arraylayers;
    lunaFormat format;
    lunaSampleCount samples;
} luna_GPU_Texture;

#endif//__LUNA_OBJECT_DEF_H__