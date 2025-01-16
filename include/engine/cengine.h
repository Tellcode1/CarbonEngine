#ifndef __C_ENGINE_H__
#define __C_ENGINE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../../common/stdafx.h"

struct lunaRenderer_Config;
union SDL_Event;

typedef struct lunaTime {
  f64 time;
  u64 last_frame_time;

} lunaTime;

extern u8 luna_CurrentFrame;
extern u64 luna_LastFrameTime; // div by SDL_GetPerofrmanceCounterFrequency to get actual time.
extern double luna_Time;

extern double luna_DeltaTime;

extern u64 luna_FrameStartTime;
extern u64 luna_FixedFrameStartTime;
extern u64 luna_FrameTime;

extern bool luna_WindowFramebufferResized;
extern bool luna_ApplicationRunning;

static inline bool cg_running() {
  return luna_ApplicationRunning;
}

static inline void cg__reset_frame_buffer_resized() {
  luna_WindowFramebufferResized = false;
}

static inline bool cg_get_frame_buffer_resized() {
  return luna_WindowFramebufferResized;
}

static inline u8 cg_get_current_frame() {
  return luna_CurrentFrame;
}

static inline double cg_get_delta_time() {
  return luna_DeltaTime;
}

static inline double cg_get_last_frame_time() {
  return luna_LastFrameTime;
}

static inline double cg_get_time() {
  return luna_Time;
}

extern void cg_initialize_context(const char *window_title, int window_width, int window_height);
extern void ctx_initialize(const char *window_title, u32 window_width, u32 window_height);

static const u32 FIXED_FRAME_RATE   = 60;
static const double FIXED_TICK_RATE = 1000.0 / (double)FIXED_FRAME_RATE; // 1000 milliseconds

extern void cg_consume_event(const union SDL_Event *event);
extern void cg_update();

#ifdef __cplusplus
}
#endif

#endif // __C_ENGINE_H__