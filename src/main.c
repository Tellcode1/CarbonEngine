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
#include "../include/lunaWorld.h"

#include "../include/lunaImage.h"
#include "../include/world.h"

#include "../include/cgfreelist.h"

#include "../include/cshadermanager.h"

#include "../include/lunaUI.h"

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

    cg_initialize_context("kilometers per second (edgy)(im cool now ok?)", 800, 600);

    luna_Renderer_Config rdconf = crender_config_init();
    rdconf.vsync_enabled = 1;
    rdconf.buffer_mode = CG_BUFFER_MODE_DOUBLE_BUFFERED;
    rdconf.window_resizable = 0;
    rdconf.multisampling_enable = 0;
    rdconf.samples = CG_SAMPLE_COUNT_1_SAMPLES;
    luna_Renderer_t *rd = luna_Renderer_Init(&rdconf);

    // * get a better name for this
    csm_compile_updated();

    cinput_init();
    ctext_init(rd);

    const f32 updateTime = 10.0f; // seconds. 1.5f = 1.5 seconds
    f32 totalTime = 0.0f;
    u32 numFrames = 0;

    cfont_t *amongus;
    ctext_load_font(rd, "../Assets/roboto.ttf", 256.0f, &amongus);

    int curr_showing_fps = 0;

    ccamera camera = ccamera_init();

    float accumulator = 0.0f;
    const float timeStep = 1.0f / 60.0f;

    world_t *world;
    world_init(rd, sprite_empty, &world);

    luna_UI_Init(rd);

    luna_UI_Button *bton = luna_UI_CreateButton(sprite_load_disk("../Assets/barrel.png"));
    bton->size.x = 50.0f;
    bton->size.y = 50.0f;
    bton->pressed = pressed;
    bton->hover = hoover;

    luna_UI_Button *bton2 = luna_UI_CreateButton(sprite_load_disk("../Assets/barrel.png"));
    bton2->size.x = 10.0f;
    bton2->size.y = 10.0f;
    bton2->pressed = pressed;
    bton2->hover = hoover;
    bton2->position.x = 10.0f;
    bton2->position.y = 10.0f;

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
        world_update(&camera, world);

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
        if (!bton2->was_hovered) {
            bton2->color = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };
        }

        static float piss = 1.0f;
        if (cinput_is_key_held(SDL_SCANCODE_UP)) {
            piss += 1.0f;
        }
        if (cinput_is_key_held(SDL_SCANCODE_DOWN)) {
            piss -= 1.0f;
        }

        if (luna_Renderer_BeginRender(rd)) {
            ctext_begin_render(amongus);

            luna_UI_Render(&camera, rd);

            bton->size.x = piss;
            bton->size.y = piss;

            ctext_text_render_info info = {};
            info.horizontal = CTEXT_HORI_ALIGN_CENTER;
            info.vertical = CTEXT_VERT_ALIGN_CENTER;
            info.scale = 1.0f;
            info.position = (vec3){bton->position.x, bton->position.y, 1.0f};
            info.color = (vec4){1.0f,0.0f,0.0f,1.0f};
            info.bbox = bton->size;
            info.scale_for_fit = 1;
            ctext_render(amongus, &info,
R"(hey there buddy chum pal friend buddy pal chum bud friend fella bruther amigo pal friend chumy chum pal
i dont mean to be rude my friend home slice bread slice dawg
but i gotta warn ya if u take one more diddly darn step right there
im gonna have to diddly darn snap ur neck and wowza wouldnt that be a crummy juncture huh?
do you want that?
do u wish upon yourself to come into physical experience with a crummy juncture?
because friend buddy chum friend chum pally pal chum friend if u keep this up
then well gosh diddly darn
i just might have to get not so friendly with u
my friendly friend friend pal friend buddy chum pally friend chum buddy)");

            info.scale = 0.1f;
            info.scale_for_fit = 0;
            info.horizontal = CTEXT_HORI_ALIGN_CENTER;
            info.vertical = CTEXT_VERT_ALIGN_CENTER;
            info.position = (vec3){0.0f, 0.0f, sinf(cg_get_time()) * 5.0f};
            ctext_render(amongus, &info, "POOTIS\nSPENCER\nHERE");

            ctext_end_render(rd, &camera, amongus, m4init(1.0f));

            // world_render(&camera, world);

            luna_Renderer_EndRender(rd);
        }
    }

    luna_Renderer_Destroy(rd);
    LOG_INFO("done.");
    return 0;
}
