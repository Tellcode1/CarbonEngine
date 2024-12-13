#ifndef __LUNA_INTERNAL_OBJECTS_DEF_H__
#define __LUNA_INTERNAL_OBJECTS_DEF_H__

#include "lunaGFX.h"
#include "cgvector.h"

extern cg_vector_t g_Samplers;

typedef struct luna_GPU_Sampler {
    VkFilter filter;
    VkSamplerMipmapMode mipmap_mode;
    VkSamplerAddressMode address_mode;
    f32 max_anisotropy;
    f32 mip_lod_bias, min_lod, max_lod;
    VkSampler vksampler;
} luna_GPU_Sampler;

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

#endif//__LUNA_INTERNAL_OBJECTS_DEF_H__