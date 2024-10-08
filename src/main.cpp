#include "../include/defines.h"
#include "../include/math/vec2.hpp"
#include "../include/math/vec3.hpp"
#include "../include/stdafx.h"
#include "../include/cengine.h"
#include "../include/ctext.hpp"
#include "../include/cinput.h"
#include "../include/containers/cstring.h"

#include "../include/camera.hpp"
#include "../include/mesh.hpp"
#include "../include/csquare.hpp"

#include "../include/cimage.h"
#include "../include/ctext.h"

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    TIME_FUNCTION(cg_initialize_context("kilometers per second (edgy)(im cool now ok?)", 800, 600));

    crenderer_config rdconf = crender_config_init();
    rdconf.vsync_enabled = 1;
    rdconf.buffer_mode = CGFX_BUFFER_MODE_SINGLE_BUFFERED;
    rdconf.window_resizable = 0;
    rdconf.multisampling_enable = 0;
    crenderer_t *rd = crenderer_init(&rdconf);
    cg_initialize();

    csm_compile_updated();

    constexpr f32 updateTime = 3.0f; // seconds. 1.5f = 1.5 seconds
    f32 totalTime = 0.0f;
    u32 numFrames = 0;

    ctext::cfont_t *amongus;
    ctext::CFontLoadInfo infoo{};
    infoo.fontPath = "../Assets/roboto.ttf";
    infoo.chset = CHARSET_ASCII;
    infoo.scale = 32.0f;
    infoo.channel_count = ctext::CHANNELS_MSDF;
    ctext::load_font(rd, &infoo, &amongus);

    int curr_showing_fps = 0;

    bool relmodeon = 1;
    cassert(SDL_SetRelativeMouseMode(SDL_TRUE) != -1);

    cmesh_t *light;
    cmesh_t *mesh;
    cmesh_t *block = load_mesh(rd, "../Assets/cube.obj", "../Assets/model/3DBread007_HQ-1K-JPG_Color.jpg", "../Assets/empty.png");
    block->transform.position = cm::vec3(0.0f);
    block->transform.scale = cm::vec3(1.0f);

    TIME_FUNCTION(
        light = load_mesh(rd, "../Assets/barrel.obj", "../Assets/barrel.png", "../Assets/model/3DBread007_HQ-1K-JPG_NormalGL.jpg");
        mesh = load_mesh(rd, "../Assets/model/3DBread007_HQ-1K-JPG.obj", "../Assets/model/3DBread007_HQ-1K-JPG_Color.jpg", "../Assets/model/3DBread007_HQ-1K-JPG_NormalGL.jpg");
    );

    ccamera camera{};

    // What in the unholy f%$ where you doing
    LOG_DEBUG("Initialized in %ld ms (%.3f s)", SDL_GetTicks64(), SDL_GetTicks64() / 1000.0f);
    while(cg_running())
    {
        cg_update();
        const double dt = cg_get_delta_time();

        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            cg_consume_event(&event);
            if (event.type == SDL_MOUSEMOTION) {
                const float sensitivity = 10.0f;
                camera.rotate(((float)event.motion.xrel) / sensitivity, ((float)event.motion.yrel) / sensitivity);
            }
        }

        const float speed = 16.6f * dt;
        cm::vec3 cam_pos_add = cm::vec3(0.0f);
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
            camera.move(cam_pos_add);
        }

        if (cinput_is_key_pressed(SDL_SCANCODE_TAB)) {
            relmodeon = !relmodeon;
            SDL_SetRelativeMouseMode((SDL_bool)relmodeon);
        }

        // Profiling code
        totalTime += cg_get_delta_time();
        numFrames++;
        if (totalTime >= updateTime) {
            curr_showing_fps = ceilf(numFrames / totalTime);
            LOG_INFO("%dfps : %d frames / %.2fs = ~%fms/frame", curr_showing_fps, numFrames, updateTime, (totalTime / static_cast<f32>(numFrames)));
            numFrames = 0;
            totalTime = 0.0;
        }

        if (cinput_is_key_held(SDL_SCANCODE_I))
            mesh->transform.position.y += speed;
        if (cinput_is_key_held(SDL_SCANCODE_K))
            mesh->transform.position.y -= speed;
        if (cinput_is_key_held(SDL_SCANCODE_J))
            mesh->transform.position.x -= speed;
        if (cinput_is_key_held(SDL_SCANCODE_L))
            mesh->transform.position.x += speed;

        // light->transform.position = cm::vec3(0.5f, 1.0f, 0.3f);
        light->transform.position = cm::vec3(0.0f);
        mesh->transform.position = cm::vec3(sinf(cg_get_time() / 3.0f) / 3.0f, 0.0f, cosf(cg_get_time() / 3.0f) / 3.0f);
        mesh->transform.scale = cm::vec3(10.0f);
        light->transform.scale = cm::vec3(10.0f);

        camera.update(rd);

        if (crenderer_begin_render(rd)) {
            ctext::begin_render(amongus);

            ctext::text_render_info info;
            info.horizontal = CTEXT_HORI_ALIGN_LEFT;
            info.vertical = CTEXT_VERT_ALIGN_TOP;
            info.scale = 1.0f;
            info.position = cm::vec3(-1.0f, -1.0f, -1.0f);
            ctext::render(amongus, &info, "%u %s frames", curr_showing_fps, "joosy");
            
            info.horizontal = CTEXT_HORI_ALIGN_CENTER;
            info.vertical = CTEXT_VERT_ALIGN_CENTER;
            info.position = cm::vec3(0.0f, 0.0f, sinf(cg_get_time()) * 5.0f);
            ctext::render(amongus, &info, "pootis");

            ctext::end_render(rd, camera, amongus, cm::mat4(1.0f));

            block->transform.rotation.y += cmdeg2rad(360.0f * 16.0f) * dt;

            render(rd, camera, light, light->transform.position);
            render(rd, camera, mesh, light->transform.position);
            render(rd, camera, block, light->transform.position);

            crenderer_end_render(rd);
        } else {
            LOG_INFO("Skipped a frame!");
        }
    }
    
    // Do not do cleanup because we are big bois
    return 0;
}
