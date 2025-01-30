#ifndef __NOVA_ENGINE_H__
#define __NOVA_ENGINE_H__

#include "../../common/stdafx.h"

NOVA_HEADER_START;

struct NVRenderer_Config;
union SDL_Event;

typedef struct NVTime {
  f64 time;
  u64 last_frame_time;

} NVTime;

extern u8 NV_CurrentFrame;
extern u64 NV_LastFrameTime; // div by SDL_GetPerofrmanceCounterFrequency to get actual time.
extern double NV_Time;

extern double NV_DeltaTime;

extern u64 NV_FrameStartTime;
extern u64 NV_FixedFrameStartTime;
extern u64 NV_FrameTime;

extern bool NV_WindowFramebufferResized;
extern bool NV_ApplicationRunning;

static inline bool NV_running() {
  return NV_ApplicationRunning;
}

static inline void _NV_reset_frame_buffer_resized() {
  NV_WindowFramebufferResized = false;
}

static inline bool NV_get_frame_buffer_resized() {
  return NV_WindowFramebufferResized;
}

static inline u8 NV_get_current_frame() {
  return NV_CurrentFrame;
}

static inline double NV_get_delta_time() {
  return NV_DeltaTime;
}

static inline double NV_get_last_frame_time() {
  return NV_LastFrameTime;
}

static inline double NV_get_time() {
  return NV_Time;
}

extern void NV_initialize_context(const char *window_title, int window_width, int window_height);
extern void _NV_initialize_context_internal(const char *window_title, u32 window_width, u32 window_height);

static const u32 NV_FIXED_FRAME_RATE   = 60;
static const double NV_FIXED_TICK_RATE = 1000.0 / (double)NV_FIXED_FRAME_RATE; // 1000 milliseconds

extern void NV_consume_event(const union SDL_Event *event);
extern void NV_update();

NOVA_HEADER_END;

#endif // __C_ENGINE_H__