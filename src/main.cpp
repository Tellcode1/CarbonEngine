#define STB_IMAGE_IMPLEMENTATION

#include "defines.h"
#include "stdafx.hpp"
#include "cengine.hpp"
#include "ctext.hpp"
#include "cinput.hpp"
#include "containers/cvector.hpp"
#include "containers/cstring.hpp"

int main(int argc, char *argv[]) {
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
    infoo.fontPath = "../Assets/roboto.ttf";
    infoo.chset = msdf_atlas::Charset::ASCII;
    infoo.scale = 32.0f;
    infoo.channel_count = ctext::CHANNELS_MSDF;
    ctext::load_font(&infoo, &amongus);

    cstring str = U"The quick brown fox jumped over the lazy dog";
    cstring alt_str;

    f32 scale = 0.5f;

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
            const u32 space_pos = str.rfind(U' ');
            const u32 newline_pos = str.rfind(U'\n');
            u32 pos = cstring::npos;

            if (space_pos != cstring::npos && (newline_pos == cstring::npos || space_pos > newline_pos))
                pos = space_pos + 1;
            else if (newline_pos != cstring::npos && (space_pos == cstring::npos || newline_pos > space_pos))
                pos = newline_pos + 1;

            if (pos != cstring::npos)
                str = str.substr(0, pos);
            else
                str.clear();
        }
        else if (cinput::is_key_pressed(SDL_SCANCODE_BACKSPACE) && str.length() > 0) {
            str.pop_back();
        }
        else if (cinput::is_key_pressed(SDL_SCANCODE_RETURN))
            str += U'\n';
        else if (cinput::is_key_pressed(SDL_SCANCODE_TAB))
            str += U'\t';
        else if (cinput::is_key_pressed(SDL_SCANCODE_DELETE))
            str.clear();
        else if (cinput::is_key_pressed(SDL_SCANCODE_KP_0)) {
            // pc abuse
            for (u32 j = 0; j < 100; j++) {
                for (u32 i = 0; i < 50; i++) {
                    alt_str += (const unicode *)U"100k chars at 70fps ";
                }
                alt_str += U'\n';
            }
            std::cout << alt_str.size() << '\n';
        }


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

            ctext::text_render_info info;
            info.x = mouse_position.x;
            info.y = mouse_position.y;
            info.horizontal = CTEXT_HORI_ALIGN_CENTER;
            info.vertical = CTEXT_VERT_ALIGN_CENTER;
            info.scale = scale;
            ctext::render(amongus, &info, str.data());

            ctext::render(amongus, &info, alt_str.data());

            info.horizontal = CTEXT_HORI_ALIGN_LEFT;
            info.vertical = CTEXT_VERT_ALIGN_TOP;
            info.x = -1.0f;
            info.y = -1.0f;
            ctext::render(amongus, &info, U"%u fps", int(1.0f / cengine::get_delta_time()));

            ctext::end_render(amongus, mat4(1.0f));
            
            RD::EndRender();
        }

    }
    
    // Do not do cleanup because we are big bois
    return 0;
}
