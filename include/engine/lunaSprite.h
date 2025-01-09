#ifndef __LUNA_SPRITE_H__
#define __LUNA_SPRITE_H__

// Renderable sprite

#include "../GPU/vkstdafx.h"
#include "../GPU/format.h"

typedef struct lunaSprite_t lunaSprite_t;

extern lunaSprite_t *lunaSprite_Empty;

extern lunaSprite_t *lunaSprite_LoadFromMemory(const unsigned char *data, int w, int h, lunaFormat fmt);
extern lunaSprite_t *lunaSprite_LoadFromDisk(const char *path);

// force destroy
extern void lunaSprite_Destroy(lunaSprite_t *spr);

// references
extern void lunaSprite_Lock(lunaSprite_t *spr);
extern void lunaSprite_Release(lunaSprite_t *spr);

extern void lunaSprite_GetDimensions(const lunaSprite_t *spr, int *w, int *h);
extern VkImage lunaSprite_GetVkImage(const lunaSprite_t *spr);
extern VkImageView lunaSprite_GetVkImageView(const lunaSprite_t *spr);
extern VkDescriptorSet lunaSprite_GetDescriptorSet(const lunaSprite_t *spr);
extern VkSampler lunaSprite_GetSampler(const lunaSprite_t *spr);
extern lunaFormat lunaSprite_GetFormat(const lunaSprite_t *spr);

#endif//__LUNA_SPRITE_H__