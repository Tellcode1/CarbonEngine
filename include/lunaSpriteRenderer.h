#ifndef __LUNA_SPRITE_RENDERER_H__
#define __LUNA_SPRITE_RENDERER_H__

#include "sprite.h"
#include "math/vec2.h"
#include "math/vec4.h"

typedef struct luna_Renderer_t luna_Renderer_t;
typedef struct sprite_t sprite_t;

typedef struct luna_SpriteRenderer {
    sprite_t *spr;
    bool flip_horizontal;
    bool flip_vertical;
    vec2 tex_coord_multiplier; // This is multiplied with the texture coordinates while rendering.
    vec4 color;
} luna_SpriteRenderer;

static inline luna_SpriteRenderer luna_SpriteRendererInit() {
    return (luna_SpriteRenderer) {
        sprite_empty,
        0, 0,
        (vec2){ 1.0f, 1.0f },
        (vec4){ 1.0f, 1.0f, 1.0f, 1.0f }
    };
}

#endif//__LUNA_SPRITE_RENDERER_H__