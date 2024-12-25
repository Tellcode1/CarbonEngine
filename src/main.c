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

#include "../include/lunaImage.h"

#include "../include/cgfreelist.h"

#include "../include/cshadermanager.h"

#include "../include/lunaUI.h"

#include "../include/lunaObject.h"

#include "../include/player.h"
#include "../include/bee.h"
#include "../include/enemy.h"

void pressed(luna_UI_Button *self) {
    (void)self;
    puts("bye bye!! xoxo");
    cg_application_running = 0;
}

void hoover(luna_UI_Button *self) {
    self->color = (vec4){ 0.6f, 0.6f, 0.6f, 1.0f };
}

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    const lunaExtent2D window_size = (lunaExtent2D){ 800, 600 };

    cg_initialize_context("kilometers per second (edgy)(im cool now ok?)", window_size.width, window_size.height);

    luna_Renderer_Config rdconf = crender_config_init();
    rdconf.vsync_enabled = 1;
    rdconf.buffer_mode = CG_BUFFER_MODE_DOUBLE_BUFFERED;
    rdconf.window_resizable = 0;
    rdconf.initial_window_size = window_size;
    rdconf.multisampling_enable = 0;
    rdconf.samples = CG_SAMPLE_COUNT_1_SAMPLES;
    luna_Renderer_t *rd = luna_Renderer_Init(&rdconf);

    const f32 updateTime = 5.0f; // seconds. 1.5f = 1.5 seconds
    f32 totalTime = 0.0f;
    u32 numFrames = 0;

    cfont_t *amongus;
    ctext_load_font(rd, "../Assets/roboto.ttf", 256.0f, &amongus);

    int curr_showing_fps = 0;

    float accumulator = 0.0f;
    const float timeStep = 1.0f / 60.0f;

    luna_UI_Init(rd);

    luna_UI_Button *bton = luna_UI_CreateButton(sprite_load_disk("../Assets/x.png"));
    bton->size.x = 5.0f;
    bton->size.y = 5.0f;
    bton->pressed = pressed;
    bton->hover = hoover;

    luna_Object_Initialize();

    luna_Object *grnd = luna_CreateObject(
        "Ground",
        1,
        (vec2){ 0.0f, -5.0f },
        (vec2){ 100.0f, 1.0f },
        0
    );
    (void)grnd;

    plr_init();
    bees_init();
    enemies_init();

    // What in the unholy f%$ where you doing
    LOG_DEBUG("Initialized in %ld ms (%.3f s)", SDL_GetTicks64(), SDL_GetTicks64() / 1000.0f);
    while(cg_running())
    {
        cg_update();
        const double dt = cg_get_delta_time();

        accumulator += dt;
        while (accumulator >= timeStep) {
            // fixed update
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

        cinput_update(rd);
        cam_update(&camera, rd);

        luna_ObjectsUpdate();
        bees_update();
        enemies_update();

        vec3 move = (vec3){ 0.0f, 0.0f, 0.0f };
        if (cinput_is_key_held(SDL_SCANCODE_W)) {
            move.y += 20.0f;
        }
        if (cinput_is_key_held(SDL_SCANCODE_S)) {
            move.y -= 20.0f;
        }
        if (cinput_is_key_held(SDL_SCANCODE_A)) {
            move.x -= 20.0f;
        }
        if (cinput_is_key_held(SDL_SCANCODE_D)) {
            move.x += 20.0f;
        }
        cam_move(&camera, v3muls(move, dt));

        luna_UI_Update(&camera);
        if (!bton->was_hovered) { // If the mouse is NOT on the button, change it back to it's default color
            bton->color = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };
        }
        bton->position = v2add((vec2){camera.position.x, camera.position.y}, (vec2){ -9.0f, 9.0f });
        bton->size = (vec2){ 1.0f, 1.0f };

        if (luna_Renderer_BeginRender(rd)) {
            ctext_begin_render(amongus);
            ctext_text_render_info crd = ctext_init_text_render_info();
            ctext_render(amongus, &crd, "piss\nis\nstored\nin\nthe\nballs");
            ctext_end_render(rd, amongus, m4init(1.0f));

            luna_Renderer_EndRender(rd);
        }
    }

    luna_Renderer_Destroy(rd);
    LOG_INFO("done.");
    return 0;
}
