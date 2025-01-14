#ifndef __LUNA_SPRITE_H__
#define __LUNA_SPRITE_H__

// Renderable sprite

#define VK_FORWARD_DECLARE(s)                                                                                                                        \
  typedef struct s##_T s##_T;                                                                                                                        \
  typedef s##_T *s;

VK_FORWARD_DECLARE(VkImage);
VK_FORWARD_DECLARE(VkImageView);
VK_FORWARD_DECLARE(VkDescriptorSet);
VK_FORWARD_DECLARE(VkSampler);

#include "../GPU/format.h"

typedef struct lunaSprite lunaSprite;

extern lunaSprite *lunaSprite_Empty;

extern lunaSprite *lunaSprite_LoadFromMemory(const unsigned char *data, int w, int h, lunaFormat fmt);
extern lunaSprite *lunaSprite_LoadFromDisk(const char *path);

// force destroy
extern void lunaSprite_Destroy(lunaSprite *spr);

// references
extern void lunaSprite_Lock(lunaSprite *spr);
extern void lunaSprite_Release(lunaSprite *spr);

extern void lunaSprite_GetDimensions(const lunaSprite *spr, int *w, int *h);
extern VkImage lunaSprite_GetVkImage(const lunaSprite *spr);
extern VkImageView lunaSprite_GetVkImageView(const lunaSprite *spr);
extern VkDescriptorSet lunaSprite_GetDescriptorSet(const lunaSprite *spr);
extern VkSampler lunaSprite_GetSampler(const lunaSprite *spr);
extern lunaFormat lunaSprite_GetFormat(const lunaSprite *spr);

#endif //__LUNA_SPRITE_H__