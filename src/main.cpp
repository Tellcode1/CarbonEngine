#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "../include/Renderer.hpp"
#include "../../external/stb/stb_image.h"
#include "../../external/stb/stb_image_write.h"

#include "../include/defines.h"
#include "../include/math/vec3.hpp"
#include "../include/math/vec2.hpp"
#include "../include/stdafx.h"
#include "../include/cengine.hpp"
#include "../include/ctext.hpp"
#include "../include/cinput.hpp"
#include "../include/containers/cvector.hpp"
#include "../include/containers/cstring.hpp"

#include "../include/engine/object/cgameobject.hpp"
#include "../include/engine/object/mesh.hpp"
#include "../include/csquare.hpp"

#include "../include/cshadermanager.h"

#include "../include/containers/cvector.h"
#include "../include/containers/chashmap.h"

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    using RD = Renderer;

    TIME_FUNCTION(cengine::initialize_context("kilometers per second (edgy)(im cool now ok?)", 800, 600));

    renderer_config rdconf{};
    rdconf.present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    rdconf.buffer_mode = BUFFER_MODE_NONE;
    rdconf.window_resizable = false;
    rdconf.multisampling_enable = false;
    rdconf.samples = VK_SAMPLE_COUNT_1_BIT;
    cengine::initialize(&rdconf);

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
    ctext::load_font(&infoo, &amongus);

    int curr_showing_fps = 0;

    cassert(SDL_SetRelativeMouseMode(SDL_TRUE) == 0);

    cmesh_t *light;
    cmesh_t *mesh;
    cmesh_t *block = load_mesh("../Assets/cube.obj", "../Assets/model/3DBread007_HQ-1K-JPG_Color.jpg", "../Assets/empty.png");
    block->transform.position = cm::vec3(0.0f);
    block->transform.scale = cm::vec3(1.0f);

    TIME_FUNCTION(
        light = load_mesh("../Assets/model/3DBread007_HQ-1K-JPG.obj", "../Assets/model/3DBread007_HQ-1K-JPG_Color.jpg", "../Assets/model/3DBread007_HQ-1K-JPG_NormalGL.jpg");
        mesh = load_mesh("../Assets/model/3DBread007_HQ-1K-JPG.obj", "../Assets/model/3DBread007_HQ-1K-JPG_Color.jpg", "../Assets/model/3DBread007_HQ-1K-JPG_NormalGL.jpg");
    );

    // What in the unholy f%$ where you doing
    LOG_DEBUG("Initialized in %ld ms (%.3f s)", SDL_GetTicks64(), SDL_GetTicks64() / 1000.0f);
    while(cengine::running())
    {
        cengine::update();
        const double dt = cengine::get_delta_time();

        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            cengine::consume_event(&event);
            if (event.type == SDL_MOUSEMOTION) {
                const float sensitivity = 10.0f;
                camera.rotate(((float)event.motion.xrel) / sensitivity, ((float)event.motion.yrel) / sensitivity);
            }
        }

        const float speed = 16.6f * dt;
        cm::vec3 cam_pos_add = cm::vec3(0.0f);
        if (cinput::is_key_held(SDL_SCANCODE_W))
            cam_pos_add.z += speed;
        if (cinput::is_key_held(SDL_SCANCODE_S))
            cam_pos_add.z -= speed;
        if (cinput::is_key_held(SDL_SCANCODE_A))
            cam_pos_add.x -= speed;
        if (cinput::is_key_held(SDL_SCANCODE_D))
            cam_pos_add.x += speed;
        if (cinput::is_key_held(SDL_SCANCODE_SPACE))
            cam_pos_add.y += speed;
        if (cinput::is_key_held(SDL_SCANCODE_LALT))
            cam_pos_add.y -= speed;
        if (cam_pos_add.x != 0.0f || cam_pos_add.y != 0.0f || cam_pos_add.z != 0.0f) {
            camera.move(cam_pos_add);
        }

        // Profiling code
        totalTime += cengine::get_delta_time();
        numFrames++;
        if (totalTime >= updateTime) {
            curr_showing_fps = ceilf(numFrames / totalTime);
            LOG_INFO("%dfps : %d frames / %.2fs = ~%fms/frame", curr_showing_fps, numFrames, updateTime, (totalTime / static_cast<f32>(numFrames)));
            numFrames = 0;
            totalTime = 0.0;
        }

        if (cinput::is_key_held(SDL_SCANCODE_I))
            mesh->transform.position.y += speed;
        if (cinput::is_key_held(SDL_SCANCODE_K))
            mesh->transform.position.y -= speed;
        if (cinput::is_key_held(SDL_SCANCODE_J))
            mesh->transform.position.x -= speed;
        if (cinput::is_key_held(SDL_SCANCODE_L))
            mesh->transform.position.x += speed;

        // light->transform.position = cm::vec3(0.5f, 1.0f, 0.3f);
        light->transform.position = cm::vec3(0.0f);
        mesh->transform.position = cm::vec3(sinf(cengine::get_time() / 3.0f) / 3.0f, 0.0f, cosf(cengine::get_time() / 3.0f) / 3.0f);
        mesh->transform.scale = cm::vec3(10.0f);
        light->transform.scale = cm::vec3(10.0f);

        if (RD::BeginRender()) {
            const VkCommandBuffer cmd = Renderer::GetDrawBuffer();

            ctext::begin_render(amongus);

            ctext::text_render_info info;
            info.horizontal = CTEXT_HORI_ALIGN_LEFT;
            info.vertical = CTEXT_VERT_ALIGN_TOP;
            info.scale = 1.0f;
            info.position = cm::vec3(-1.0f, -1.0f, -1.0f);
            ctext::render(amongus, &info, U"%u %s frames", curr_showing_fps, U"joosy");
            
            info.horizontal = CTEXT_HORI_ALIGN_CENTER;
            info.vertical = CTEXT_VERT_ALIGN_CENTER;
            info.position = cm::vec3(0.0f, 0.0f, sinf(cengine::get_time()) * 5.0f);
            ctext::render(amongus, &info, U"pootis");

            ctext::end_render(amongus, cm::mat4(1.0f));

            block->transform.rotation.y += cmdeg2rad(360.0f * 16.0f) * dt;

            render(light, light->transform.position);
            render(mesh, light->transform.position);
            render(block, light->transform.position);

            RD::EndRender();
        } else {
            LOG_INFO("Skipped a frame!");
        }
    }
    
    // Do not do cleanup because we are big bois
    return 0;
}
