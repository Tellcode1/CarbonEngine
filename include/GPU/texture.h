#ifndef __NOVA_TEXTURE_H__
#define __NOVA_TEXTURE_H__

#include "../../common/stdafx.h"
#include "../engine/renderer.h"
#include "format.h"
#include "vkstdafx.h"

NOVA_HEADER_START;

NVVK_FORWARD_DECLARE(VkImage);
NVVK_FORWARD_DECLARE(VkImageView);
NVVK_FORWARD_DECLARE(VkSampler);

typedef struct NV_GPU_Sampler NV_GPU_Sampler;
typedef struct NVImage NVImage;
typedef struct NV_GPU_Memory NV_GPU_Memory;

typedef enum NV_GPU_TextureUsage {
  NOVA_GPU_TEXTURE_USAGE_SAMPLED_TEXTURE      = 0,
  NOVA_GPU_TEXTURE_USAGE_COLOR_TEXTURE        = 1, // color render texture
  NOVA_GPU_TEXTURE_USAGE_DEPTH_TEXTURE        = 2,
  NOVA_GPU_TEXTURE_USAGE_STENCIL_TEXTURE      = 3,
  NOVA_GPU_TEXTURE_USAGE_STORAGE_TEXTURE      = 4,
  NOVA_GPU_TEXTURE_USAGE_INPUT_ATTACHMENT     = 5,
  NOVA_GPU_TEXTURE_USAGE_RESOLVE_TEXTURE      = 6,
  NOVA_GPU_TEXTURE_USAGE_TRANSIENT_ATTACHMENT = 7,
  NOVA_GPU_TEXTURE_USAGE_PRESENTATION         = 8 // swapchain image
} NV_GPU_TextureUsage;

typedef struct NV_GPU_TextureCreateInfo {
  NVFormat format;
  NVSampleCount samples;
  uint32_t type;
  NV_GPU_TextureUsage usage;
  NVExtent3D extent;
  int arraylayers;
  int miplevels;
} NV_GPU_TextureCreateInfo;

typedef struct NV_GPU_SamplerCreateInfo {
  /* VkFilter */ uint32_t filter;
  /* VkSamplerMipmapMode */ uint32_t mipmap_mode;
  /* VkSamplerAddressMode */ uint32_t address_mode;
  float max_anisotropy;
  float mip_lod_bias, min_lod, max_lod;
} NV_GPU_SamplerCreateInfo;

extern void NV_GPU_GetTextureSize(const NV_GPU_Texture *tex, int *w, int *h);

extern void NV_GPU_CreateTexture(const NV_GPU_TextureCreateInfo *pInfo, NV_GPU_Texture **tex);

extern void NV_GPU_TextureAttachView(NV_GPU_Texture *tex, VkImageView view);

extern void NV_GPU_BindTextureToMemory(NV_GPU_Memory *mem, size_t offset, NV_GPU_Texture *tex);
extern void NV_GPU_DestroyTexture(NV_GPU_Texture *tex);

extern void NV_GPU_CreateSampler(const NV_GPU_SamplerCreateInfo *pInfo, NV_GPU_Sampler **sampler);

extern void NV_GPU_WriteToTexture(NV_GPU_Texture *tex, const NVImage *src);

extern VkImage NV_GPU_TextureGet(const NV_GPU_Texture *tex);
extern VkImageView NV_GPU_TextureGetView(const NV_GPU_Texture *tex);
extern VkSampler NV_GPU_SamplerGet(const NV_GPU_Sampler *sampler);

NOVA_HEADER_END;

#endif //__NOVA_TEXTURE_H__