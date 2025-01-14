#ifndef __LUNA_TEXTURE_H__
#define __LUNA_TEXTURE_H__

#include "../engine/lunaGFX.h"
#include "format.h"

typedef struct luna_GPU_Sampler luna_GPU_Sampler;
typedef struct lunaImage lunaImage;
typedef struct luna_GPU_Memory luna_GPU_Memory;

typedef enum luna_GPU_TextureUsage {
  LUNA_GPU_TEXTURE_USAGE_SAMPLED_TEXTURE      = 0,
  LUNA_GPU_TEXTURE_USAGE_COLOR_TEXTURE        = 1, // color render texture
  LUNA_GPU_TEXTURE_USAGE_DEPTH_TEXTURE        = 2,
  LUNA_GPU_TEXTURE_USAGE_STENCIL_TEXTURE      = 3,
  LUNA_GPU_TEXTURE_USAGE_STORAGE_TEXTURE      = 4,
  LUNA_GPU_TEXTURE_USAGE_INPUT_ATTACHMENT     = 5,
  LUNA_GPU_TEXTURE_USAGE_RESOLVE_TEXTURE      = 6,
  LUNA_GPU_TEXTURE_USAGE_TRANSIENT_ATTACHMENT = 7,
  LUNA_GPU_TEXTURE_USAGE_PRESENTATION         = 8 // swapchain image
} luna_GPU_TextureUsage;

typedef struct luna_GPU_TextureCreateInfo {
  lunaFormat format;
  lunaSampleCount samples;
  VkImageType type;
  luna_GPU_TextureUsage usage;
  VkExtent3D extent;
  int arraylayers;
  int miplevels;
} luna_GPU_TextureCreateInfo;

typedef struct luna_GPU_SamplerCreateInfo {
  VkFilter filter;
  VkSamplerMipmapMode mipmap_mode;
  VkSamplerAddressMode address_mode;
  float max_anisotropy;
  float mip_lod_bias, min_lod, max_lod;
} luna_GPU_SamplerCreateInfo;

extern void luna_GPU_GetTextureSize(const luna_GPU_Texture *tex, int *w, int *h);

extern void luna_GPU_CreateTexture(const luna_GPU_TextureCreateInfo *pInfo, luna_GPU_Texture **tex);

extern void luna_GPU_TextureAttachView(luna_GPU_Texture *tex, VkImageView view);

extern void luna_GPU_BindTextureToMemory(luna_GPU_Memory *mem, int offset, luna_GPU_Texture *tex);
extern void luna_GPU_DestroyTexture(luna_GPU_Texture *tex);

extern void luna_GPU_CreateSampler(const luna_GPU_SamplerCreateInfo *pInfo, luna_GPU_Sampler **sampler);

extern void luna_GPU_WriteToTexture(luna_GPU_Texture *tex, const lunaImage *src);

extern VkImage luna_GPU_TextureGet(const luna_GPU_Texture *tex);
extern VkImageView luna_GPU_TextureGetView(const luna_GPU_Texture *tex);
extern VkSampler luna_GPU_SamplerGet(const luna_GPU_Sampler *sampler);

#endif //__LUNA_TEXTURE_H__