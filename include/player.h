#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "lunaObject.h"

void plr_init();
void plr_on_render(luna_Renderer_t *rd);
void plr_update(float dt);

static const float plr_speed = 300.0f;

typedef struct player_t {
    luna_Object *obj;
} player_t;

player_t plr;

void plr_init() {
    plr = (player_t){};
    plr.obj = luna_CreateObject(
        "Player",
        0,
        (vec2){ 0.0f, 0.0f },
        (vec2){ 1.0f, 1.0f }
    );
    luna_ObjectAssignOnUpdateFn(plr.obj, plr_update);
    luna_ObjectAssignOnRenderFn(plr.obj, plr_on_render);
}

void plr_on_render(luna_Renderer_t *rd) {
    (void)rd;
}

void plr_update(float dt) {
    luna_SpriteRenderer *spr_rd = luna_ObjectGetSpriteRenderer(plr.obj);

    vec2 move = (vec2){};
    if (cinput_is_key_held(SDL_SCANCODE_LEFT)) {
        move.x -= plr_speed;
        spr_rd->flip_horizontal = 0;
    }
    else if (cinput_is_key_held(SDL_SCANCODE_RIGHT)) {
        move.x += plr_speed;
        spr_rd->flip_horizontal = 1;
    }
    if (cinput_is_key_held(SDL_SCANCODE_DOWN)) {
        move.y -= plr_speed;
        spr_rd->flip_vertical = 0;
    }
    else if (cinput_is_key_held(SDL_SCANCODE_UP)) {
        move.y += plr_speed;
        spr_rd->flip_vertical = 1;
    }
    luna_ObjectSetVelocity(plr.obj, v2muls(move, dt));
}

#endif