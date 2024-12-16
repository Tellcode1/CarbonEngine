#ifndef __ENEMY_H__
#define __ENEMY_H__

#include "player.h"
#include "bee.h"
#include "lunaObject.h"

typedef struct enemy_t {
    luna_Object *obj;
} enemy_t;

static enemy_t enemies[ 8 ];

void enemies_init() {
    for (int i = 0; i < 8; i++) {
        enemy_t *em = &enemies[i];
        em->obj = luna_CreateObject(
            "enemy!!",
            0,
            (vec2) {
                (rand() / (float)RAND_MAX) * 10.0f,
                (rand() / (float)RAND_MAX) * 10.0f,
            },
            (vec2){ 1.0f, 1.0f }, 0
        );
    }
}

void enemies_update() {
    for (int i = 0; i < 8; i++) {
        const enemy_t *em = &enemies[i];
        const float enemy_player_dist = fabsf( v2mag(v2sub(luna_ObjectGetPosition(em->obj), luna_ObjectGetPosition(plr.obj))) );

        if (enemy_player_dist <= 5.0f) {
            luna_ObjectSetPosition(em->obj, V2LERP(luna_ObjectGetPosition(em->obj), luna_ObjectGetPosition(plr.obj), 3.0f * cg_get_delta_time()));
        }
    }
}

#endif