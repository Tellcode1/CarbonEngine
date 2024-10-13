#include "../include/defines.h"
#include "../include/stdafx.h"
#include "../include/cengine.h"
#include "../include/ctext.h"
#include "../include/cinput.h"
#include "../include/cgstring.h"

#include "../include/camera.h"
#include "../include/mesh.hpp"
#include "../include/csquare.h"

#include "../include/cimage.h"

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    cg_device_t device = {};
    TIME_FUNCTION(device = cg_initialize_device("kilometers per second (edgy)(im cool now ok?)", 800, 600));

    crenderer_config rdconf = crender_config_init();
    rdconf.vsync_enabled = 1;
    rdconf.buffer_mode = CGFX_BUFFER_MODE_SINGLE_BUFFERED;
    rdconf.window_resizable = 0;
    rdconf.multisampling_enable = 1;
    rdconf.samples = CGFX_SAMPLE_COUNT_MAX_SUPPORTED;
    crenderer_t *rd = crenderer_init(&device, &rdconf);
    cinput_init();
    ctext_init(rd);

    // * get a better name for this
    csm_compile_updated(&device);

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

    bool relmodeon = 1;
    cassert(SDL_SetRelativeMouseMode(SDL_TRUE) != -1);

    cmesh_t *light;
    cmesh_t *mesh;
    cmesh_t *block = load_mesh(rd, "../Assets/cube.obj", "../Assets/model/3DBread007_HQ-1K-JPG_Color.jpg", "../Assets/empty.png");
    block->transform.position = (vec3){0.0f,0.0f,0.0f};
    block->transform.scale = (vec3){1.0f,1.0f,1.0f};

    light = load_mesh(rd, "../Assets/barrel.obj", "../Assets/barrel.png", "../Assets/model/3DBread007_HQ-1K-JPG_NormalGL.jpg");
    mesh = load_mesh(rd, "../Assets/model/3DBread007_HQ-1K-JPG.obj", "../Assets/model/3DBread007_HQ-1K-JPG_Color.jpg", "../Assets/model/3DBread007_HQ-1K-JPG_NormalGL.jpg");

    ccamera camera = ccamera_init();

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
                cam_rotate(&camera, ((float)event.motion.xrel) / sensitivity, ((float)event.motion.yrel) / sensitivity);
            }
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

        if (cinput_is_key_pressed(SDL_SCANCODE_SPACE)) {
            printf("space press\n");
            relmodeon = !relmodeon;
            SDL_SetRelativeMouseMode((SDL_bool)relmodeon);
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

        if (cinput_is_key_held(SDL_SCANCODE_I))
            mesh->transform.position.y += speed;
        if (cinput_is_key_held(SDL_SCANCODE_K))
            mesh->transform.position.y -= speed;
        if (cinput_is_key_held(SDL_SCANCODE_J))
            mesh->transform.position.x -= speed;
        if (cinput_is_key_held(SDL_SCANCODE_L))
            mesh->transform.position.x += speed;

        // light->transform.position = vec3(0.5f, 1.0f, 0.3f);
        light->transform.position = (vec3){0.0f,0.0f,0.0f};
        mesh->transform.position = (vec3){sinf(cg_get_time() / 3.0f) / 3.0f, 0.0f, cosf(cg_get_time() / 3.0f) / 3.0f};
        mesh->transform.scale = (vec3){10.0f,10.0f,10.0f};
        light->transform.scale = (vec3){10.0f,10.0f,10.0f};

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

            block->transform.rotation.y += cmdeg2rad(360.0f * 16.0f) * dt;

            // render(rd, camera, light, light->transform.position);
            // render(rd, camera, mesh, light->transform.position);
            render(rd, &camera, block, light->transform.position);

            crd_end_render(rd);
        } else {
            LOG_INFO("Skipped a frame!");
        }
    }
    
    // Do not do cleanup because we are big bois
    return 0;
}
