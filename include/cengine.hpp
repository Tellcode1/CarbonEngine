#ifndef __C_ENGINE_HPP__
#define __C_ENGINE_HPP__

#include "defines.h"
#include <SDL2/SDL.h>

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

    constexpr static u32 FIXED_FRAME_RATE = 30;
    constexpr static f64 fixed_frame_delay = 1000.0 / FIXED_FRAME_RATE;

    static void initialize();
    static void consume_event(SDL_Event *event);
    static void update();
    static void begin_frame();
    static void end_frame();

private:
    static u8 current_frame;
    static f64 delta_time;
    static f64 last_frame_time;
    static f64 time;

    static u64 frame_start;
    static u64 fixed_frame_start;
    static u64 frame_time;

    static bool framebuffer_resized;
    static bool application_running;
};

#endif // __C_ENGINE_HPP__