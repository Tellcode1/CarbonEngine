#define STB_IMAGE_IMPLEMENTATION

#include "stdafx.hpp"
#include "Bootstrap.hpp"
#include "pro.hpp"
#include "Renderer.hpp"
#include "CFont.hpp"
#include "CTextRenderer.hpp"

int main(void) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);

    using RD = Renderer;

    TIME_FUNCTION(ctx::Initialize("epic", 800, 600));
    TIME_FUNCTION(Renderer::Initialize());
    ctext::Init();

    constexpr f32 updateTime = 3.0f; // seconds. 1.5f = 1.5 seconds
    f32 totalTime = 0.0f;
    u16 numFrames = 0;

    u64 currentTime = SDL_GetTicksNS();
    u64 lastFrameTime = currentTime;
    f64 dt = 0.0;

    // What in the unholy f%$ where you doing
    LOG_DEBUG("Initialized in %ld ms || %.3f s", SDL_GetTicks(), SDL_GetTicks() / 1000.0f);

    cf::CFont amongus;
    cf::CFontLoadInfo infoo{};
    infoo.fontPath = "../Assets/roboto.ttf";
    cf::CFLoad(&infoo, &amongus);

    SDL_Event event;

    while(RD::running) {
        SDL_PumpEvents();
        while(SDL_PollEvent(&event)) {
            RD::ProcessEvent(&event);
        }

        currentTime = SDL_GetTicksNS();
        dt = (currentTime - lastFrameTime) / 1000000000.0;
        lastFrameTime = currentTime;

        // Profiling code
        totalTime += dt;
        numFrames++;
        if (totalTime > updateTime) {
            printf("%ldfps : %d frames / %.2fs = ~%fs/frame\n", static_cast<u64>(floorf(numFrames / totalTime)), numFrames, updateTime, totalTime / static_cast<f32>(numFrames));
            numFrames = 0;
            totalTime = 0.0;
        }

        if (RD::BeginRender()) {

            ctext::Render(amongus, Renderer::GetDrawBuffer(), U"gamer", 0.0f, 1.0f, 1.0f);
            
            RD::EndRender();
        }

        // Input::ProcessEvents();
        
    }
    
    // Do not do cleanup because we are big bois
    return 0;
}
