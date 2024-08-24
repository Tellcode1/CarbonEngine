#define STB_IMAGE_IMPLEMENTATION

#include "../include/defines.h"
#include "../include/math/vec2.h"
#include "../include/stdafx.hpp"
#include "../include/cengine.hpp"
#include "../include/engine/ctext/ctext.hpp"
#include "../include/cinput.hpp"
#include "../include/containers/cvector.hpp"
#include "../include/containers/cstring.hpp"

#include "../include/engine/object/cgameobject.hpp"

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

    using RD = Renderer;

    TIME_FUNCTION(cengine::initialize_context("kilometers per second (edgy)(im cool now ok?)", 800, 600));

    renderer_config rdconf{};
    rdconf.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    rdconf.max_frames_in_flight = 2;
    rdconf.window_resizable = false;
    rdconf.multisampling_enable = false;
    rdconf.depth_buffer_enable = false;
    cengine::initialize(&rdconf);

    constexpr f32 updateTime = 3.0f; // seconds. 1.5f = 1.5 seconds
    f32 totalTime = 0.0f;
    u32 numFrames = 0;

    ctext::CFont amongus;
    ctext::CFontLoadInfo infoo{};
    infoo.fontPath = "../Assets/roboto.ttf";
    infoo.chset = CHARSET_ASCII;
    infoo.scale = 32.0f;
    infoo.channel_count = ctext::CHANNELS_MSDF;
    ctext::load_font(&infoo, &amongus);

    int curr_showing_fps = 0;

    cassert(SDL_SetRelativeMouseMode(SDL_TRUE) == 0);

    // What in the unholy f%$ where you doing
    LOG_DEBUG("Initialized in %ld ms (%.3f s)", SDL_GetTicks(), SDL_GetTicks() / 1000.0f);
    while(cengine::running())
    {
        const double dt = cengine::get_delta_time();

        const float speed = 16.6f * dt;
        vec3 cam_pos_add = vec3(0.0f);
        if (cinput::is_key_held(SDL_SCANCODE_W))
            cam_pos_add.z += speed;
        if (cinput::is_key_held(SDL_SCANCODE_S))
            cam_pos_add.z -= speed;
        if (cinput::is_key_held(SDL_SCANCODE_A))
            cam_pos_add.x -= speed;
        if (cinput::is_key_held(SDL_SCANCODE_D))
            cam_pos_add.x += speed;
        if (cinput::is_key_held(SDL_SCANCODE_SPACE))
            cam_pos_add.y -= speed;
        if (cinput::is_key_held(SDL_SCANCODE_LALT))
            cam_pos_add.y += speed;
        if (cam_pos_add.x != 0.0f || cam_pos_add.y != 0.0f || cam_pos_add.z != 0.0f) {
            camera.move(cam_pos_add);
        }

        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_MOUSEMOTION) {
                const float deltaX = (float)event.motion.xrel, deltaY = (float)event.motion.yrel;
                const float sensitivity = 0.1f;
                camera.rotate(deltaX * sensitivity, deltaY * sensitivity);
            }
            cengine::consume_event(&event);
        }

        camera.update_vectors(false);

        // Profiling code
        totalTime += cengine::get_delta_time();
        numFrames++;
        if (totalTime > updateTime) {
            curr_showing_fps = ceilf(numFrames / totalTime);
            printf("%dfps : %d frames / %.2fs = ~%fms/frame\n", curr_showing_fps, numFrames, updateTime, (totalTime / static_cast<f32>(numFrames)));
            numFrames = 0;
            totalTime = 0.0;
        }

        if (RD::BeginRender()) {
            ctext::begin_render(amongus);

            ctext::text_render_info info;
            info.horizontal = CTEXT_HORI_ALIGN_LEFT;
            info.vertical = CTEXT_VERT_ALIGN_TOP;
            info.scale = 1.0f;
            info.position = vec3(-1.0f, -1.0f, -1.0f);
            ctext::render(amongus, &info, U"%u %s frames", curr_showing_fps, U"joosy");
            
            info.horizontal = CTEXT_HORI_ALIGN_CENTER;
            info.vertical = CTEXT_VERT_ALIGN_CENTER;
            info.position = vec3(0.0f, 0.0f, sinf(cengine::get_time()) * 5.0f);
            ctext::render(amongus, &info, U"pootis");

            ctext::end_render(amongus, mat4(1.0f));

            RD::EndRender();

            // This seems to fix the mouse stuttering. I'll see why later.
            SDL_Delay(17);
        } else {
            printf("Skipped a frame!\n");
        }
        cengine::update();
    }
    
    // Do not do cleanup because we are big bois
    return 0;
}
