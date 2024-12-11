#ifndef __SPRITE_H
#define __SPRITE_H

// Renderable sprite

#include "vkstdafx.h"

typedef struct sprite_t sprite_t;

extern sprite_t *sprite_empty;

extern sprite_t *sprite_load_mem(const unsigned char *data, int w, int h, VkFormat fmt);
extern sprite_t *sprite_load_disk(const char *path);

extern void sprite_get_dimensions(const sprite_t *spr, int *w, int *h);
extern VkImage sprite_get_internal(const sprite_t *spr);
extern VkImageView sprite_get_internal_view(const sprite_t *spr);
extern VkDescriptorSet sprite_get_descriptor(const sprite_t *spr);
extern VkSampler sprite_get_sampler(const sprite_t *spr);

#endif//__SPRITE_H