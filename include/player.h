#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "../external/box2d/include/box2d/box2d.h"
#include "../include/math/vec2.h"
#include <SDL2/SDL.h>

typedef struct player_t {
    vec2 pos;
    b2BodyId body;
} player_t;

player_t player_init(b2WorldId world) {
    player_t player = {};

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = (b2Vec2){0.0f, 4.0f};
    player.body = b2CreateBody(world, &bodyDef);

    b2Polygon dynamicBox = b2MakeBox(1.0f, 1.0f);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.friction = 0.3f;
    b2CreatePolygonShape(player.body, &shapeDef, &dynamicBox);

    return player;
}

void player_update(player_t *pl) {
    vec2 move = (vec2){ 0.0f, 0.0f, 0.0f };
    if (cinput_is_key_held(SDL_SCANCODE_UP)) {
        move.y += 20.0f;
    }
    if (cinput_is_key_held(SDL_SCANCODE_DOWN)) {
        move.y -= 20.0f;
    }
    if (cinput_is_key_held(SDL_SCANCODE_LEFT)) {
        move.x -= 20.0f;
    }
    if (cinput_is_key_held(SDL_SCANCODE_RIGHT)) {
        move.x += 20.0f;
    }
    pl->pos = v2add(pl->pos, move);
}

#endif