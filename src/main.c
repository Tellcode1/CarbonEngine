#include "../include/defines.h"
#include "../include/stdafx.h"
#include "../include/cengine.h"
#include "../include/ctext.h"
#include "../include/cinput.h"
#include "../include/cgstring.h"

#include "../include/camera.h"
// #include "../include/mesh.hpp"
#include "../include/cquad.h"

#include "../include/cimage.h"

#include "../external/box2d/include/box2d/box2d.h"

typedef struct player_t {
    int anim_frame;
    cmesh_t *mesh;
    b2BodyId player_id;
} player_t;

player_t player_init(crenderer_t *rd, b2WorldId *world_id) {
    player_t player = {};
    player.mesh = load_mesh(NULL, rd);

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = (b2Vec2){0.0f, 4.0f};
    bodyDef.fixedRotation = 1;
    b2BodyId player_id = b2CreateBody(*world_id, &bodyDef);

    b2Polygon dynamicBox = b2MakeBox(1.0f, 1.0f);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.friction = 0.7f;
    shapeDef.restitution = 0.0f;
    b2CreatePolygonShape(player_id, &shapeDef, &dynamicBox);

    player.player_id = player_id;

    return player;
}

void player_move(player_t *player, vec2 v) {
    b2Body_ApplyLinearImpulseToCenter(player->player_id, (b2Vec2){ v.x, v.y }, 1);
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    cg_initialize_context("kilometers per second (edgy)(im cool now ok?)", 800, 600);

    crenderer_config rdconf = crender_config_init();
    rdconf.vsync_enabled = 1;
    rdconf.buffer_mode = CG_BUFFER_MODE_SINGLE_BUFFERED;
    rdconf.window_resizable = 0;
    rdconf.multisampling_enable = 0;
    rdconf.samples = CG_SAMPLE_COUNT_1_SAMPLES;
    crenderer_t *rd = crenderer_init(&rdconf);

    // * get a better name for this
    csm_compile_updated();

    cinput_init();
    ctext_init(rd);

    const f32 updateTime = 10.0f; // seconds. 1.5f = 1.5 seconds
    f32 totalTime = 0.0f;
    u32 numFrames = 0;

    cfont_t *amongus;
    ctext_font_load_info infoo = ctext_font_load_info_init();
    infoo.fontPath = "../Assets/roboto.ttf";
    infoo.chset = CHARSET_ASCII;
    infoo.scale = 32.0f;
    ctext_load_font(rd, &infoo, &amongus);

    int curr_showing_fps = 0;

    cmesh_t *mesh = load_mesh(NULL, rd);
    cmesh_t *ground = load_mesh(NULL, rd);

    ground->transform.scale.x = 100.0f;
    ground->transform.position.y = -5.0f;

    ccamera camera = ccamera_init();

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = (b2Vec2){ 0.0f, -9.8f };
    b2WorldId worldId = b2CreateWorld(&worldDef);

    b2BodyDef groundBodyDef = b2DefaultBodyDef();
    groundBodyDef.position = (b2Vec2){ground->transform.position.x, ground->transform.position.y};
    b2BodyId groundId = b2CreateBody(worldId, &groundBodyDef);

    b2Polygon groundBox = b2MakeBox(ground->transform.scale.x / 2.0f, ground->transform.scale.y / 2.0f);

    b2ShapeDef groundShapeDef = b2DefaultShapeDef();
    b2CreatePolygonShape(groundId, &groundShapeDef, &groundBox);

    player_t player = player_init(rd, &worldId);

    const float timeStep = 1.0f / 60.0f;
    const int subStepCount = 4;

    // What in the unholy f%$ where you doing
    LOG_DEBUG("Initialized in %ld ms (%.3f s)", SDL_GetTicks64(), SDL_GetTicks64() / 1000.0f);
    while(cg_running())
    {
        cg_update();
        const double dt = cg_get_delta_time();

        const float speed = 16.0f;
        b2Vec2 body_add = (b2Vec2){0.0f,0.0f};

        bool_t moved = 0;
        if (cinput_is_key_held(SDL_SCANCODE_A))
            body_add.x -= 1.0f;
        if (cinput_is_key_held(SDL_SCANCODE_D))
            body_add.x += 1.0f;
        if (cinput_is_key_held(SDL_SCANCODE_W))
            body_add.y += 1.0f;
        if (cinput_is_key_held(SDL_SCANCODE_S))
            body_add.y -= 1.0f;
        if (body_add.x != 0.0f || body_add.y != 0.0f) {
            vec2 *body_add_v = ((vec2 *)&body_add);
            *body_add_v = v2muls(v2normalize(*body_add_v), speed);
            b2Body_SetLinearVelocity(player.player_id, *((b2Vec2 *)body_add_v));
            moved = 1;
        }

        {
            b2World_Step(worldId, timeStep, subStepCount);
            b2Vec2 player_position = b2Body_GetPosition(player.player_id);
            mesh->transform.position = (vec3){ player_position.x, player_position.y, 0.0f };
        }

        if (moved) {
            b2Body_SetLinearVelocity(player.player_id, (b2Vec2){ 0.0f, b2Body_GetLinearVelocity(player.player_id).y });
        }

        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            cg_consume_event(&event);
        }

        // Profiling code
        totalTime += cg_get_delta_time();
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
            ctext_render(amongus, &info, "%u %s frames", curr_showing_fps, "joosy");
            
            info.horizontal = CTEXT_HORI_ALIGN_CENTER;
            info.vertical = CTEXT_VERT_ALIGN_CENTER;
            info.position = (vec3){0.0f, 0.0f, sinf(cg_get_time()) * 5.0f};
            ctext_render(amongus, &info, "POOTIS\nSPENCER\nHERE");

            ctext_end_render(rd, &camera, amongus, m4init(1.0f));

            render(rd, &camera, mesh);
            render(rd, &camera, ground);

            crd_end_render(rd);
        } else {
            LOG_INFO("Skipped a frame!");
        }
    }

    b2DestroyWorld(worldId);
    crenderer_destroy(rd);
    LOG_INFO("done.");
    return 0;
}
