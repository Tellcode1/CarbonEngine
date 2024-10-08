#include "../include/cengine.h"
#include "../include/ctext.hpp"
#include "../include/cinput.h"
#include "../include/cengineinit.hpp"

#include <SDL2/SDL.h>

void cg_initialize_context(const char *title, u32 width, u32 height)
{
    ctx::Initialize(title, width, height);
}

u8 cg_current_frame = 0;
f64 cg_delta_time = 0.0;
u64 cg_last_frame_time = 0;
f64 cg_time = 0.0;
u64 cg_frame_start = 0;
u64 cg_fixed_frame_start = 0;
u64 cg_frame_time = 0;
bool cg_framebuffer_resized = false;
bool cg_application_running = true;

void cg_initialize() {
    ctext::init();
    cinput_initialize();
}

void cg_consume_event(const SDL_Event *event) {
    if ((event->type == SDL_QUIT) || ((event->type == SDL_WINDOWEVENT) && (event->window.event == SDL_WINDOWEVENT_CLOSE)) || (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_ESCAPE))
        cg_application_running = false;

    if (event->type == SDL_WINDOWEVENT && (event->window.event == SDL_WINDOWEVENT_RESIZED || event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
        cg_framebuffer_resized = true;
}

void cg_update() {
    cg_time = (f64)SDL_GetTicks64() * (1.0 / 1000.0);

    // This fixes really large values of delta time for the first frame.
    static u64 currtime = SDL_GetPerformanceCounter();
    cg_last_frame_time = currtime;
    currtime = SDL_GetPerformanceCounter();
    cg_delta_time = fdiv(currtime - cg_last_frame_time, SDL_GetPerformanceFrequency());

    if (static_cast<double>(SDL_GetTicks64() - cg_fixed_frame_start) >= FIXED_TICK_RATE) {
        // fixed update
        cg_fixed_frame_start = SDL_GetTicks64();
    }

    cinput_update();
}

void cg_begin_frame() {
    
}

void cg_end_frame() {
    
}