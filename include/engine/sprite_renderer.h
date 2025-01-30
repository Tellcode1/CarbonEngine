#ifndef __NV_SPRITE_RENDERER_H__
#define __NV_SPRITE_RENDERER_H__

#include "../../common/math/vec2.h"
#include "../../common/math/vec4.h"
#include "sprite.h"

NOVA_HEADER_START;

typedef struct NVRenderer_t NVRenderer_t;

typedef struct NV_SpriteRenderer {
  NVSprite *spr;
  bool flip_horizontal;
  bool flip_vertical;
  vec2 tex_coord_multiplier; // This is multiplied with the texture coordinates while rendering.
  vec4 color;
} NV_SpriteRenderer;

static inline NV_SpriteRenderer NV_SpriteRendererInit() {
  return (NV_SpriteRenderer){NVSprite_Empty, 0, 0, (vec2){1.0f, 1.0f}, (vec4){1.0f, 1.0f, 1.0f, 1.0f}};
}

NOVA_HEADER_END;

#endif //__NOVA_SPRITE_RENDERER_H__