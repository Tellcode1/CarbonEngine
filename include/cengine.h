#ifndef __C_ENGINE_HPP__
#define __C_ENGINE_HPP__

#include "defines.h"

struct crenderer_config;
union SDL_Event;

extern u8 cg_current_frame;
extern u64 cg_last_frame_time; // div by SDL_GetPerofrmanceCounterFrequency to get actual time.
extern f64 cg_time;

extern f64 cg_delta_time;
extern u64 cg_delta_time_last_frame_time;

extern u64 cg_frame_start;
extern u64 cg_fixed_frame_start;
extern u64 cg_frame_time;

extern bool cg_framebuffer_resized;
extern bool cg_application_running;

static inline bool cg_running() {
    return cg_application_running;
}

static inline void cg__reset_frame_buffer_resized() {
    cg_framebuffer_resized = false;
}

static inline bool cg_get_frame_buffer_resized() {
    return cg_framebuffer_resized;
}

static inline u8 cg_get_current_frame() {
    return cg_current_frame;
}

static inline f64 cg_get_delta_time() {
    return cg_delta_time;
}

static inline f64 cg_get_last_frame_time() {
    return cg_last_frame_time;
}

static inline f64 cg_get_time() {
    return cg_time;
}

extern void cg_initialize_context(const char *title, u32 width, u32 height);

static const u32 FIXED_FRAME_RATE = 30;
static const f64 FIXED_TICK_RATE = 1000.0 / (f64)FIXED_FRAME_RATE; // 1000 milliseconds

extern void cg_initialize();
extern void cg_consume_event(const SDL_Event *event);
extern void cg_update();
extern void cg_begin_frame();
extern void cg_end_frame();

#endif // __C_ENGINE_HPP__