#include <stdlib.h>
#include <math.h>

#include "../include/defines.h"
#include "../include/stdafx.h"
#include "../include/cengine.h"
#include "../include/ctext.h"
#include "../include/cinput.h"
#include "../include/cshadermanager.h"
#include "../include/containers/cgstring.h"
#include "../include/lunaCamera.h"
#include "../include/lunaImage.h"
#include "../include/lunaUI.h"
#include "../include/lunaScene.h"
#include "../include/lunaObject.h"
#include "../include/timer.h"

static u32 cookie_count = 0;

void pressed(luna_UI_Button *self) {
    (void)self;
    ++cookie_count;
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

    const float updateTime = 5.0f; // seconds. 1.5f = 1.5 seconds
    float totalTime = 0.0f;
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

    lunaScene *scn = lunaScene_Init();

    luna_UI_Button *cookie = luna_UI_CreateButton(sprite_empty);
    cookie->pressed = pressed;
    cookie->hover = hoover;
    cookie->size = (vec2){5.0f,5.0f};
    cookie->position = (vec2){-6.0f, 0.0f};
    (void)cookie;

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
            LOG_INFO("%dfps : %d frames / %.2fs = ~%fms/frame", curr_showing_fps, numFrames, updateTime, (totalTime / (float)(numFrames)));
            numFrames = 0;
            totalTime = 0.0;
        }

        cinput_update(rd);
        lunaCamera_Update(&camera, rd);

        lunaScene_Update(scn);

        luna_UI_Update(&camera);
        if (!bton->was_hovered) { // If the mouse is NOT on the button, change it back to it's default color
            bton->color = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };
        }
        bton->position = v2add((vec2){camera.position.x, camera.position.y}, (vec2){ -9.0f, 9.0f });
        bton->size = (vec2){ 1.0f, 1.0f };

        if (cookie->was_hovered) {
            cookie->color = (vec4){0.7f,0.7f,0.7f, 1.0f};
        } else {
            cookie->color = (vec4){1.0f,1.0f,1.0f, 1.0f};
        }

        if (luna_Renderer_BeginRender(rd)) {
            ctext_begin_render(amongus);
            ctext_text_render_info crd = ctext_init_text_render_info();
            crd.position.y = 90.0f;
            ctext_render(amongus, &crd, "%u cookies", cookie_count);

            lunaScene_Render(scn, rd);

            luna_Renderer_DrawLine(
                rd,
                (vec2){camera.position.x, camera.position.y},
                lunaCamera_GetMouseGlobalPosition(&camera),
                (vec4){ 1.0f, 0.0f, 0.0f, 1.0f },
                0
            );

            ctext_end_render(rd, amongus, m4init(1.0f));
            luna_Renderer_EndRender(rd);
        }
    }

    luna_Renderer_Destroy(rd);
    LOG_INFO("done.");
    return 0;
}
