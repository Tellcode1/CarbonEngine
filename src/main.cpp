#define STB_IMAGE_IMPLEMENTATION

#include "defines.h"
#include "stdafx.hpp"
#include "cengine.hpp"
#include "ctext.hpp"
#include "cinput.hpp"
#include "carrays/cvector.hpp"

int main(void) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

    using RD = Renderer;

    TIME_FUNCTION(cengine::initialize_context("kilometers per second (edgy)(im cool now ok?)", 800, 600));

    renderer_config rdconf{};
    rdconf.present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    rdconf.max_frames_in_flight = 2;
    rdconf.window_resizable = true;
    rdconf.multisampling_enable = false;
    cengine::initialize(&rdconf);

    constexpr f32 updateTime = 1.5f; // seconds. 1.5f = 1.5 seconds
    f32 totalTime = 0.0f;
    u32 numFrames = 0;

    ctext::CFont amongus;
    ctext::CFontLoadInfo infoo{};
    infoo.fontPath = "../Assets/beiruti.ttf";
    infoo.chset = msdf_atlas::Charset::ASCII;
    infoo.scale = 32.0f;
    infoo.channel_count = ctext::CHANNELS_MSDF;
    ctext::load_font(&infoo, &amongus);

    std::u32string str = U"Aq\nJQWzz";

    f32 scale = 1.0f;

    // What in the unholy f%$ where you doing
    LOG_DEBUG("Initialized in %ld ms (%.3f s)", SDL_GetTicks(), SDL_GetTicks() / 1000.0f);
    while(cengine::running())
    {    
        SDL_Event event;

        SDL_PumpEvents();
        while(SDL_PollEvent(&event)) {
            cengine::consume_event(&event);
            switch (event.type)
            {
                case SDL_MOUSEWHEEL:
                    scale += event.wheel.y / 50.0f;
                    break;
                case SDL_TEXTINPUT:
                    for (u32 i = 0; i < strlen(event.text.text); i++)
                        str += event.text.text[i];
                    break;
                break;
            }
        }
        cengine::update();

        if (cinput::is_key_pressed(SDL_SCANCODE_BACKSPACE) && cinput::is_key_held(SDL_SCANCODE_LCTRL)) {
            size_t backpos;
            if ((backpos = str.rfind(U' ')) != std::string::npos || (backpos = str.rfind(U'\n')) != std::string::npos)
                str = str.substr(0, backpos + 1);
            else
                str.clear();
        }
        if (cinput::is_key_pressed(SDL_SCANCODE_BACKSPACE) && str.length() > 0) {
            str.pop_back();
        }
        if (cinput::is_key_pressed(SDL_SCANCODE_RETURN)) {
            str += '\n';
        }
        if (cinput::is_key_pressed(SDL_SCANCODE_TAB)) {
            str += '\t';
        }
        if (cinput::is_key_pressed(SDL_SCANCODE_DELETE))
            str.clear();
        if (cinput::is_key_held(SDL_SCANCODE_K))
            scale -= 0.0005f;

        // Profiling code
        totalTime += cengine::get_delta_time()*1000.0f;
        numFrames++;
        if (totalTime > updateTime) {
            printf("%ldfps : %d frames / %.2fs = ~%fms/frame\n", static_cast<u64>(floorf(numFrames / totalTime)), numFrames, updateTime, (totalTime / static_cast<f32>(numFrames)) * 1000.0);
            numFrames = 0;
            totalTime = 0.0;
        }

        if (RD::BeginRender()) {
            vec2 mouse_position = cinput::get_mouse_position();
            
            ctext::begin_render(amongus);

            ctext::render(amongus, str, ctext::CTEXT_HORI_ALIGN_CENTER, ctext::CTEXT_VERT_ALIGN_CENTER, mouse_position.x, mouse_position.y, scale);
            ctext::render(amongus, U"volvo unban me pls", ctext::CTEXT_HORI_ALIGN_CENTER, ctext::CTEXT_VERT_ALIGN_CENTER, 0.0f, 0.0f, scale);

            ctext::end_render(amongus, mat4(1.0f));
            
            RD::EndRender();
        }

    }
    
    // Do not do cleanup because we are big bois
    return 0;
}
