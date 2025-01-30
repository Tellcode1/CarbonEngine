#ifndef __NV_SPRITE_H__
#define __NV_SPRITE_H__

#include "../GPU/vkstdafx.h"
#include "../GPU/format.h"

NOVA_HEADER_START;

// Renderable sprite

NVVK_FORWARD_DECLARE(VkImage);
NVVK_FORWARD_DECLARE(VkImageView);
NVVK_FORWARD_DECLARE(VkDescriptorSet);
NVVK_FORWARD_DECLARE(VkSampler);

typedef struct NVSprite NVSprite;

extern NVSprite *NVSprite_Empty;

extern NVSprite *NVSprite_LoadFromMemory(const unsigned char *data, int w, int h, NVFormat fmt);
extern NVSprite *NVSprite_LoadFromDisk(const char *path);

// force destroy
extern void NVSprite_Destroy(NVSprite *spr);

// references
extern void NVSprite_Lock(NVSprite *spr);
extern void NVSprite_Release(NVSprite *spr);

extern void NVSprite_GetDimensions(const NVSprite *spr, int *w, int *h);
extern VkImage NVSprite_GetVkImage(const NVSprite *spr);
extern VkImageView NVSprite_GetVkImageView(const NVSprite *spr);
extern VkDescriptorSet NVSprite_GetDescriptorSet(const NVSprite *spr);
extern VkSampler NVSprite_GetSampler(const NVSprite *spr);
extern NVFormat NVSprite_GetFormat(const NVSprite *spr);

NOVA_HEADER_END;

#endif //__NOVA_SPRITE_H__