#include "cengine.hpp"
#include "Renderer.hpp"
#include "ctext.hpp"
#include "cinput.hpp"

u8 cengine::current_frame = 0;
f64 cengine::delta_time = 0.0;
f64 cengine::last_frame_time = 0.0;
f64 cengine::time = 0.0;
u64 cengine::frame_start = 0;
u64 cengine::fixed_frame_start = 0;
u64 cengine::frame_time = 0;
bool cengine::framebuffer_resized = false;
bool cengine::application_running = true;

void cengine::initialize(const renderer_config *conf) {
    Renderer::initialize(conf);
    ctext::initialize();
    cinput::initialize();
}

void cengine::consume_event(const SDL_Event *event) {
    if ((event->type == SDL_QUIT) || (event->type == SDL_KEYDOWN && event->key.keysym.scancode == SDL_SCANCODE_ESCAPE))
        application_running = false;

    if (event->type == SDL_WINDOWEVENT && (event->window.event == SDL_WINDOWEVENT_RESIZED || event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
        framebuffer_resized = true;
}

void cengine::update() {
    time = SDL_GetTicks64() / 1000.0;
    delta_time = (time - last_frame_time) / 1000.0;
    last_frame_time = time;

    if (static_cast<double>(SDL_GetTicks64() - fixed_frame_start) >= fixed_frame_delay) {
        fixed_frame_start = SDL_GetTicks64();
    }

    cinput::update();
}

void cengine::begin_frame() {
    
}

void cengine::end_frame() {
    
}