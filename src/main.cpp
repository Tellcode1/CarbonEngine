#define STB_IMAGE_IMPLEMENTATION

#include "stdafx.hpp"
#include "Bootstrap.hpp"
#include "pro.hpp"
#include "Renderer.hpp"
#include "CFont.hpp"
#include "CArrays/CVector.hpp"

int main(void) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

    using RD = Renderer;

    TIME_FUNCTION(ctx::Initialize("epic", 800, 600));
    TIME_FUNCTION(Renderer::Initialize());
    ctext::Init();

    constexpr f32 updateTime = 5.0f; // seconds. 1.5f = 1.5 seconds
    f32 totalTime = 0.0f;
    u16 numFrames = 0;

    u64 currentTime = SDL_GetTicks64();
    u64 lastFrameTime = currentTime;
    f64 dt = 0.0;

    ctext::CFont amongus;
    ctext::CFontLoadInfo infoo{};
    infoo.fontPath = "../Assets/roboto.ttf";
    ctext::CFLoad(&infoo, &amongus);

    f32 scale = 1.0f;

    SDL_Event event;

    // What in the unholy f%$ where you doing
    LOG_DEBUG("Initialized in %ld ms (%.3f s)", SDL_GetTicks(), SDL_GetTicks() / 1000.0f);
    while(RD::running) {
        while(SDL_PollEvent(&event)) {
            RD::ProcessEvent(&event);
            if (event.type == SDL_MOUSEWHEEL) {
                scale += event.wheel.y / 20.0f;
            }
        }

        currentTime = SDL_GetTicks();
        dt = (currentTime - lastFrameTime) / 1000.0;
        lastFrameTime = currentTime;

        // Profiling code
        totalTime += dt;
        numFrames++;
        if (totalTime > updateTime) {
            printf("%ldfps : %d frames / %.2fs = ~%fms/frame\n", static_cast<u64>(floorf(numFrames / totalTime)), numFrames, updateTime, (totalTime / static_cast<f32>(numFrames)) * 1000.0);
            numFrames = 0;
            totalTime = 0.0;
        }

        if (RD::BeginRender()) {

            ctext::Render(amongus, Renderer::GetDrawBuffer(), U"gamer", 0.0f, 0.0f, scale);
            
            RD::EndRender();
        }
    }
    
    // Do not do cleanup because we are big bois
    return 0;
}
