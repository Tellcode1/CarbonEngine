#include "../include/cengine.hpp"
#include "../include/vkcb/Renderer.hpp"
#include "../include/engine/ctext/ctext.hpp"
#include "../include/cinput.hpp"

u8 cengine::current_frame = 0;
f64 cengine::delta_time = 0.0;
u64 cengine::last_frame_time = 0;
f64 cengine::time = 0.0;
u64 cengine::frame_start = 0;
u64 cengine::fixed_frame_start = 0;
u64 cengine::frame_time = 0;
bool cengine::framebuffer_resized = false;
bool cengine::application_running = true;

void cengine::initialize(const renderer_config *conf) {
    Renderer::initialize(conf);
    ctext::init();
    cinput::initialize();
}

void cengine::consume_event(const SDL_Event *event) {
    if ((event->type == SDL_QUIT) || (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_ESCAPE))
        application_running = false;

    if (event->type == SDL_WINDOWEVENT && (event->window.event == SDL_WINDOWEVENT_RESIZED || event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
        framebuffer_resized = true;
}

void cengine::update() {
    time = (f64)SDL_GetTicks64() * (1.0 / 1000.0);

    u64 curr_time = SDL_GetPerformanceCounter();
    delta_time = fdiv(curr_time - last_frame_time, SDL_GetPerformanceFrequency());
    last_frame_time = curr_time;

    if (static_cast<double>(SDL_GetTicks64() - fixed_frame_start) >= fixed_frame_delay) {
        // fixed update
        fixed_frame_start = SDL_GetTicks64();
    }

    cinput::update();
}

void cengine::begin_frame() {
    
}

void cengine::end_frame() {
    
}