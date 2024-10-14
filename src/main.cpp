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

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    TIME_FUNCTION(cg_initialize_context("kilometers per second (edgy)(im cool now ok?)", 800, 600));

    crenderer_config rdconf = crender_config_init();
    rdconf.vsync_enabled = 1;
    rdconf.buffer_mode = CG_BUFFER_MODE_SINGLE_BUFFERED;
    rdconf.window_resizable = 0;
    rdconf.multisampling_enable = 1;
    rdconf.samples = CG_SAMPLE_COUNT_MAX_SUPPORTED;
    crenderer_t *rd = crenderer_init(&rdconf);
    cinput_init();
    ctext_init(rd);

    // * get a better name for this
    csm_compile_updated();

    const f32 updateTime = 3.0f; // seconds. 1.5f = 1.5 seconds
    f32 totalTime = 0.0f;
    u32 numFrames = 0;

    cfont_t *amongus;
    ctext_font_load_info infoo = ctext_font_load_info_init();
    infoo.fontPath = "../Assets/roboto.ttf";
    infoo.chset = CHARSET_ASCII;
    infoo.scale = 32.0f;
    ctext_load_font(rd, &infoo, &amongus);

    int curr_showing_fps = 0;

    cmesh_t *mesh = load_mesh(rd);
    cmesh_t *ground = load_mesh(rd);

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

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = (b2Vec2){0.0f, 4.0f};
    b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);

    b2Polygon dynamicBox = b2MakeBox(1.0f, 1.0f);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.friction = 0.3f;
    b2CreatePolygonShape(bodyId, &shapeDef, &dynamicBox);

    const float timeStep = 1.0f / 60.0f;
    const int subStepCount = 4;

    // What in the unholy f%$ where you doing
    LOG_DEBUG("Initialized in %ld ms (%.3f s)", SDL_GetTicks64(), SDL_GetTicks64() / 1000.0f);
    while(cg_running())
    {
        cg_update();
        const double dt = cg_get_delta_time();

        // for (int i = 0; i < 90; ++i)
        {
            b2World_Step(worldId, timeStep, subStepCount);
            b2Vec2 player_position = b2Body_GetPosition(bodyId);
            b2Rot player_rotation = b2Body_GetRotation(bodyId);
            mesh->transform.position = (vec3){ player_position.x, player_position.y, 0.0f };
            // mesh->transform.rotation = (vec3){ player_rotation.s, player_rotation.c, 0.0f };
        }

        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            cg_consume_event(&event);
        }

        const float speed = 16.6f * dt;
        vec3 cam_pos_add = (vec3){0.0f,0.0f,0.0f};
        if (cinput_is_key_held(SDL_SCANCODE_W))
            cam_pos_add.z += speed;
        if (cinput_is_key_held(SDL_SCANCODE_S))
            cam_pos_add.z -= speed;
        if (cinput_is_key_held(SDL_SCANCODE_A))
            cam_pos_add.x -= speed;
        if (cinput_is_key_held(SDL_SCANCODE_D))
            cam_pos_add.x += speed;
        if (cinput_is_key_held(SDL_SCANCODE_SPACE))
            cam_pos_add.y += speed;
        if (cinput_is_key_held(SDL_SCANCODE_LALT))
            cam_pos_add.y -= speed;
        if (cam_pos_add.x != 0.0f || cam_pos_add.y != 0.0f || cam_pos_add.z != 0.0f) {
            cam_move(&camera, cam_pos_add);
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
            info.position = (vec3){-1.0f, -1.0f, -1.0f};
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
    return 0;
}
