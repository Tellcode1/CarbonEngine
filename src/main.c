#include <stdlib.h>
#include <math.h>

#include "../include/defines.h"
#include "../include/stdafx.h"
#include "../include/cengine.h"
#include "../include/ctext.h"
#include "../include/cinput.h"
#include "../include/cgstring.h"

#include "../include/camera.h"
// #include "../include/mesh.H"
#include "../include/cquad.h"

#include "../include/cimage.h"

#include "../include/world.h"

typedef struct player_t {
    int anim_frame;
    cmesh_t *mesh;
    b2BodyId player_id;
    bool is_on_ground;
} player_t;

player_t player_init(crenderer_t *rd, b2WorldId *world_id) {
    player_t player = {};

    body_parameters params = {
        .world = *world_id,
        .position = (vec2){0.0f, 10.0f},
        .scale = (vec2){1.0f, 1.0f},
        .type = b2_dynamicBody,
        .density = 1.0f,
        .friction = 1.0f,
        .restitution = 0.0f,
    };
    player.mesh = load_mesh(NULL, &params, rd );
    player.player_id = player.mesh->body;

    return player;
}

void player_handle_input(player_t *player, cmesh_t *ground) {
    bool jump = 0;
    const float speed = 16.0f;
    b2Vec2 body_add = (b2Vec2){0.0f,0.0f};

    const b2Vec2 body_velocity = b2Body_GetLinearVelocity(player->player_id);
    if (body_velocity.x > -10.0f && cinput_is_key_held(SDL_SCANCODE_A))
        body_add.x -= 1.0f;
    if (body_velocity.x < 10.0f && cinput_is_key_held(SDL_SCANCODE_D))
        body_add.x += 1.0f;
    if (cinput_is_key_pressed(SDL_SCANCODE_SPACE)) {
        const float rayLength = 1.0f;
        b2Vec2 rayStart = b2Body_GetPosition(player->player_id);
        b2Vec2 rayEnd = b2Add(rayStart, (b2Vec2){0.0f, -rayLength});
        const b2RayCastInput input = {
            .maxFraction = 1.0f,
            .origin = rayStart,
            .translation = (b2Vec2){ 0.0f, -1.0f, }
        };
        b2ShapeId player_shape;
        b2Body_GetShapes(ground->body, &player_shape, 1);
        b2CastOutput out = b2Shape_RayCast(player_shape, &input);
        if (out.hit) {
            jump = 1;
        }
    }
    if (body_add.x != 0.0f || body_add.y != 0.0f || jump) {
        body_add = b2MulSV(speed, b2Normalize(body_add));
        if (jump) {
            body_add.y += 30.0f;
        }
        b2Body_ApplyLinearImpulseToCenter(player->player_id, body_add, 1);
    }
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    cg_initialize_context("kilometers per second (edgy)(im cool now ok?)", 800, 600);

    crenderer_config rdconf = crender_config_init();
    rdconf.vsync_enabled = 1;
    rdconf.buffer_mode = CG_BUFFER_MODE_DOUBLE_BUFFERED;
    rdconf.window_resizable = 0;
    rdconf.multisampling_enable = 1;
    rdconf.samples = CG_SAMPLE_COUNT_4_SAMPLES;
    crenderer_t *rd = crenderer_init(&rdconf);

    // * get a better name for this
    csm_compile_updated();

    cinput_init();
    ctext_init(rd);

    const f32 updateTime = 10.0f; // seconds. 1.5f = 1.5 seconds
    f32 totalTime = 0.0f;
    u32 numFrames = 0;

    cfont_t *amongus;
    ctext_load_font(rd, "../Assets/roboto.ttf", 32.0f, &amongus);

    int curr_showing_fps = 0;

    ccamera camera = ccamera_init();

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){ 0.0f, -9.8f };
    b2WorldId worldId = b2CreateWorld(&worldDef);

    body_parameters params = {
        .world = worldId,
        .position = (vec2){0.0f, -0.0f},
        .scale = (vec2){(f32)INT16_MAX, 1.0f},
        .type = b2_staticBody,
        .density = 1.0f,
        .friction = 1.0f,
        .restitution = 0.0f,
    };
    cmesh_t *ground = load_mesh(NULL, &params, rd);
    player_t player = player_init(rd, &worldId);

    float accumulator = 0.0f;
    const float timeStep = 1.0f / 60.0f;
    const int subStepCount = 4;

    block_t world[ WORLD_W * WORLD_H ];
    for (int j = 0; j < WORLD_H; j++) {
        for (int i = 0; i < WORLD_W; i++) {
            block_t *block = &world[j * WORLD_W + i];
            block->x = 0;
            block->y = 0;
            body_parameters params = {
                .world = worldId,
                .position = (vec2){(f32)i, (f32)j},
                .scale = (vec2){1.0f, 1.0f},
                .type = b2_staticBody,
                .density = 1.0f,
                .friction = 1.0f,
                .restitution = 0.0f,
            };
            block->mesh = load_mesh(sprite_empty, &params, rd);
        }
    }

    // What in the unholy f%$ where you doing
    LOG_DEBUG("Initialized in %ld ms (%.3f s)", SDL_GetTicks64(), SDL_GetTicks64() / 1000.0f);
    while(cg_running())
    {
        cg_update();
        const double dt = cg_get_delta_time();

        player_handle_input(&player, ground);

        accumulator += dt;
        while (accumulator >= timeStep) {
            b2World_Step(worldId, timeStep, subStepCount);
            b2Vec2 player_position = b2Body_GetPosition(player.player_id);
            player.mesh->transform.position = (vec3){ player_position.x, player_position.y, 0.0f };
            camera.position = (vec3){ player_position.x, player_position.y, 10.0f };
            accumulator -= timeStep;
        }

        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            cg_consume_event(&event);
        }

        // Profiling code
        totalTime += dt;
        numFrames++;
        if (totalTime >= updateTime) {
            curr_showing_fps = ceilf(numFrames / totalTime);
            LOG_INFO("%dfps : %d frames / %.2fs = ~%fms/frame", curr_showing_fps, numFrames, updateTime, (totalTime / (f32)(numFrames)));
            numFrames = 0;
            totalTime = 0.0;
        }

        cam_update(&camera, rd);

        if (crd_begin_render(rd)) {
            ctext_begin_render(rd, amongus);

            ctext_text_render_info info = {};
            info.horizontal = CTEXT_HORI_ALIGN_LEFT;
            info.vertical = CTEXT_VERT_ALIGN_TOP;
            info.scale = 1.0f;
            info.position = (vec3){-1.0f, -1.0f, 0.0f};
            info.color = (vec4){1.0f,1.0f,1.0f,1.0f};
            ctext_render(amongus, &info, "%u %s frames", curr_showing_fps, "joosy");
            
            info.horizontal = CTEXT_HORI_ALIGN_CENTER;
            info.vertical = CTEXT_VERT_ALIGN_CENTER;
            info.position = (vec3){0.0f, 0.0f, sinf(cg_get_time()) * 5.0f};
            ctext_render(amongus, &info, "POOTIS\nSPENCER\nHERE");

            ctext_end_render(rd, &camera, amongus, m4init(1.0f));

            render(rd, &camera, player.mesh);
            render(rd, &camera, ground);

            for (int j = 0; j < WORLD_H; j++) {
            for (int i = 0; i < WORLD_W; i++) {
                block_t *block = &world[j * WORLD_W + i];
                render(rd, &camera, block->mesh);
            }
        }
            crd_end_render(rd);
        }
    }

    b2DestroyWorld(worldId);
    crenderer_destroy(rd);
    LOG_INFO("done.");
    return 0;
}
