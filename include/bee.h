#ifndef __BEE_H__
#define __BEE_H__

#include "math/vec2.h"
#include "cengine.h"
#include "player.h"
#include <time.h>

#define BEE_COUNT (5)
#define BEE_ACTION_INTERVAL (2.5f)

typedef enum bee_state {
    BEE_IDLE = 0,
    BEE_MOVING = 1,
    BEE_ATTACKING = 2,
} bee_state;

typedef struct bee_t {
    luna_Object *obj;
    vec2 moving_to, randir;
    bee_state state;
    f64 last_action_time;
} bee_t;

static bee_t bees[ BEE_COUNT ];

void bees_init() {
    for (int i = 0; i < BEE_COUNT; i++) {
        bees[i].obj = luna_CreateObject(
            "bee",
            0,
            v2add(luna_ObjectGetPosition(plr.obj), (vec2) {
                (rand() / (float)RAND_MAX) * 5.0f,
                (rand() / (float)RAND_MAX) * 5.0f
            }),
            (vec2){ 0.5f, 0.5f },
            1
        );
        bees[i].last_action_time = cg_get_time() + 0.5f * (rand() / (float)RAND_MAX);
        bees[i].moving_to = luna_ObjectGetPosition(bees[i].obj);
        bees[i].randir = luna_ObjectGetPosition(bees[i].obj);
    }
}

#define V2LERP(a, b, t) (v2add(a, v2muls(v2sub(b, a), t)))

void bee_move_to_random_pos(bee_t *bee) {
    vec2 randir = (vec2) {
        (rand() / (float)RAND_MAX) * 2.0f - 1.0f,
        (rand() / (float)RAND_MAX) * 2.0f - 1.0f
    };
    randir = v2normalize(randir);
    float MIN_BEE_DISTANCE = v2mag(luna_ObjectGetSize(plr.obj));
    float MAX_BEE_DISTANCE = MIN_BEE_DISTANCE * 2.5f;
    float d = MIN_BEE_DISTANCE + (rand() / (float)RAND_MAX) * (MAX_BEE_DISTANCE - MIN_BEE_DISTANCE);
    bee->randir = v2muls(randir, d);
    bee->moving_to = v2add( luna_ObjectGetPosition(plr.obj), v2muls(randir, d) );
    bee->last_action_time = cg_get_time();
}

void bees_update() {
    for (int i = 0; i < BEE_COUNT; i++) {
        bee_t *bee = &bees[i];
        float elapsed_time = (float)(cg_get_time() - bee->last_action_time);
        float bee_player_distance = v2mag(v2sub(luna_ObjectGetPosition(bee->obj), luna_ObjectGetPosition(plr.obj)));
        if (fabsf(bee_player_distance) >= 10.0f && bee->state != BEE_MOVING) {
            bee->moving_to = v2add(luna_ObjectGetPosition(plr.obj), bee->randir);
            bee->state = BEE_MOVING;
        }
        else if (elapsed_time >= BEE_ACTION_INTERVAL && bee->state != BEE_MOVING) {
            bee_move_to_random_pos(bee);
            bee->state = BEE_MOVING;
        } else {
            bee->state = BEE_IDLE;
        }

        vec2 pos = luna_ObjectGetPosition(bee->obj);
        pos = V2LERP(pos, bee->moving_to, 2.0f * cg_get_delta_time());
        luna_ObjectSetPosition(bee->obj, pos);
    }
}

#endif//__BEE_H__