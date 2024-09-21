#ifndef __C_ENGINE_HPP__
#define __C_ENGINE_HPP__

#include "vkcb/Context.hpp"
#include "defines.h"

struct renderer_config;
union SDL_Event;

struct cengine
{
public:
    static bool CARBON_FORCE_INLINE running() {
        return application_running;
    }

    static bool CARBON_FORCE_INLINE _reset_frame_buffer_resized() {
        framebuffer_resized = false;
        return framebuffer_resized;
    }

    static bool CARBON_FORCE_INLINE get_frame_buffer_resized() {
        return framebuffer_resized;
    }

    static u8 CARBON_FORCE_INLINE get_current_frame() {
        return current_frame;
    }

    static f64 CARBON_FORCE_INLINE get_delta_time() {
        return delta_time;
    }

    static f64 CARBON_FORCE_INLINE get_last_frame_time() {
        return last_frame_time;
    }

    static f64 CARBON_FORCE_INLINE get_time() {
        return time;
    }

    static void CARBON_FORCE_INLINE initialize_context(const char *title, u32 width, u32 height) {
        ctx::Initialize(title, width, height);
    }

    constexpr static u32 FIXED_FRAME_RATE = 30;
    constexpr static f64 FIXED_TICK_RATE = 1000.0 / (f64)FIXED_FRAME_RATE; // 1000 milliseconds

    static void initialize(const renderer_config *conf);
    static void consume_event(const SDL_Event *event);
    static void update();
    static void begin_frame();
    static void end_frame();

private:
    static u8 current_frame;
    static u64 last_frame_time; // div by SDL_GetPerofrmanceCounterFrequency to get actual time.
    static f64 time;

    static f64 delta_time;
    static u64 delta_time_last_frame_time;

    static u64 frame_start;
    static u64 fixed_frame_start;
    static u64 frame_time;

    static bool framebuffer_resized;
    static bool application_running;
};

#endif // __C_ENGINE_HPP__