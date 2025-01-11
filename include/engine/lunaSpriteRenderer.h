#ifndef __LUNA_SPRITE_RENDERER_H__
#define __LUNA_SPRITE_RENDERER_H__

#include "lunaSprite.h"
#include "../math/vec2.h"
#include "../math/vec4.h"

typedef struct lunaRenderer_t lunaRenderer_t;

typedef struct luna_SpriteRenderer {
    lunaSprite *spr;
    bool flip_horizontal;
    bool flip_vertical;
    vec2 tex_coord_multiplier; // This is multiplied with the texture coordinates while rendering.
    vec4 color;
} luna_SpriteRenderer;

static inline luna_SpriteRenderer luna_SpriteRendererInit() {
    return (luna_SpriteRenderer) {
        lunaSprite_Empty,
        0, 0,
        (vec2){ 1.0f, 1.0f },
        (vec4){ 1.0f, 1.0f, 1.0f, 1.0f }
    };
}

#endif//__LUNA_SPRITE_RENDERER_H__