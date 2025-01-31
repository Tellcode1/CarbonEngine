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

typedef struct NV_sprite NV_sprite;

extern NV_sprite *NV_sprite_empty;

extern NV_sprite *NV_sprite_load_from_memory(const unsigned char *data, int w, int h, NVFormat fmt);
extern NV_sprite *NV_sprite_load_from_disk(const char *path);

// force destroy
extern void NV_sprite_destroy(NV_sprite *spr);

// references
extern void NV_sprite_lock(NV_sprite *spr);
extern void NV_sprite_release(NV_sprite *spr);

extern void NV_sprite_get_dimensions(const NV_sprite *spr, int *w, int *h);
extern VkImage NV_sprite_get_vk_image(const NV_sprite *spr);
extern VkImageView NV_sprite_get_vk_image_view(const NV_sprite *spr);
extern VkDescriptorSet NV_sprite_get_descriptor_set(const NV_sprite *spr);
extern VkSampler NV_sprite_get_sampler(const NV_sprite *spr);
extern NVFormat NV_sprite_get_format(const NV_sprite *spr);

NOVA_HEADER_END;

#endif //__NOVA_SPRITE_H__