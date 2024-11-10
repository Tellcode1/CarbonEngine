#include "../include/cengine.h"
#include "../include/ctext.h"
#include "../include/cinput.h"
#include "../include/cengineinit.h"

#include <SDL2/SDL.h>

u8 cg_current_frame = 0;
u64 cg_last_frame_time = 0.0; // div by SDL_GetPerofrmanceCounterFrequency to get actual time.
f64 cg_time = 0.0;

f64 cg_delta_time = 0.0;
u64 cg_delta_time_last_frame_time = 0;

u64 cg_frame_start = 0;
u64 cg_fixed_frame_start = 0;
u64 cg_frame_time = 0;

bool cg_framebuffer_resized = 0;
bool cg_application_running = 1;

static u64 currtime;

void cg_initialize_context(const char *window_title, int window_width, int window_height)
{
    ctx_initialize(window_title, window_width, window_height);

    // This fixes really large values of delta time for the first frame.
    currtime = SDL_GetPerformanceCounter();
}

void cg_consume_event(const SDL_Event *event)
{
    if ((event->type == SDL_QUIT) || ((event->type == SDL_WINDOWEVENT) && (event->window.event == SDL_WINDOWEVENT_CLOSE)) || (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_ESCAPE))
        cg_application_running = false;

    if (event->type == SDL_WINDOWEVENT && (event->window.event == SDL_WINDOWEVENT_RESIZED || event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
        cg_framebuffer_resized = true;
}

void cg_update() {
    cg_time = (f64)SDL_GetTicks64() * (1.0 / 1000.0);

    cg_last_frame_time = currtime;
    currtime = SDL_GetPerformanceCounter();
    cg_delta_time = (currtime - cg_last_frame_time) / (double)SDL_GetPerformanceFrequency();
}