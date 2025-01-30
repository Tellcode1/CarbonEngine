#ifndef __LUNA_RENDERER_H__
#define __LUNA_RENDERER_H__

// just keeping this here for reference
// god tier article btw
// https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/

// I strive for a world where I do not have to call vulkan functions myself again

#include "../GPU/vkstdafx.h"
#include "../GPU/descriptors.h"

#include "../../common/math/vec2.h"
#include "../../common/math/vec3.h"
#include "../../common/math/vec4.h"

NOVA_HEADER_START;

NVVK_FORWARD_DECLARE(VkFramebuffer);
NVVK_FORWARD_DECLARE(VkSemaphore);
NVVK_FORWARD_DECLARE(VkFence);

typedef struct NV_GPU_Texture NV_GPU_Texture;

extern NV_DescriptorPool g_pool;
extern struct NVCamera camera;
typedef struct NVSprite NVSprite;

typedef enum NVWindow_OptionBits {
  NOVA_WINDOW_OPTION_VSYNC         = 1 << 0,
  NOVA_WINDOW_OPTION_RESIZABLE     = 1 << 1,
  NOVA_WINDOW_OPTION_BORDERLESS    = 1 << 2,
  NOVA_WINDOW_OPTION_FULLSCREEN    = 1 << 3,
  NOVA_WINDOW_OPTION_MAXIMIZED     = 1 << 4,
  NOVA_WINDOW_OPTION_MINIMIZED     = 1 << 5,
  NOVA_WINDOW_OPTION_HIDDEN        = 1 << 6,
  NOVA_WINDOW_OPTION_HIGH_DPI      = 1 << 7,
  NOVA_WINDOW_OPTION_ALWAYS_ON_TOP = 1 << 8,
} NVWindow_OptionBits;

typedef enum NVWindow_VSyncBits { NOVA_WINDOW_VSYNC_DISABLED = 0, NOVA_WINDOW_VSYNC_ENABLED = 1 } NVWindow_VSyncBits;
typedef bool NV_Window_VSync;

typedef enum NVWindow_BufferModeBits {
  NOVA_BUFFER_MODE_SINGLE_BUFFERED = 0,
  NOVA_BUFFER_MODE_DOUBLE_BUFFERED = 1,
  NOVA_BUFFER_MODE_TRIPLE_BUFFERED = 2,
} NVWindow_BufferModeBits;
typedef unsigned NVBufferMode;

typedef enum NVRenderer_SampleCountBits {
  NOVA_SAMPLE_COUNT_MAX_SUPPORTED    = 0xFFFFFFFF,
  NOVA_SAMPLE_COUNT_NO_EXTRA_SAMPLES = 1,
  NOVA_SAMPLE_COUNT_1_SAMPLES        = 1,
  NOVA_SAMPLE_COUNT_2_SAMPLES        = 2,
  NOVA_SAMPLE_COUNT_4_SAMPLES        = 4,
  NOVA_SAMPLE_COUNT_8_SAMPLES        = 8,
  NOVA_SAMPLE_COUNT_16_SAMPLES       = 16,
  NOVA_SAMPLE_COUNT_32_SAMPLES       = 32,
} NVRenderer_SampleCountBits;
typedef unsigned NVSampleCount;

typedef struct NVExtent2D {
  int width, height;
} NVExtent2D;

typedef struct NVExtent3D {
  int width, height, depth;
} NVExtent3D;

// Move ownership to camera VV
typedef struct NVFrameRenderData {
  NV_GPU_Texture *sc_image; // sc -> swapchain owned
  NV_GPU_Texture *depth_image;
  VkFramebuffer color_framebuffer;
  VkSemaphore image_available_semaphore;
  VkSemaphore render_finish_semaphore;
  VkFence in_flight_fence;
} NVFrameRenderData;

typedef enum NV_renderer_flag_bits {
  NOVA_RENDERER_MULTISAMPLING_ENABLE = 1 << 0,
  NOVA_RENDERER_VSYNC_ENABLE         = 1 << 1,
  NOVA_RENDERER_WINDOW_RESIZABLE     = 1 << 2,
} NVRenderer_FlagsBits;

typedef struct NVRenderer_Config {
  NVSampleCount samples;
  NVBufferMode buffer_mode;
  NVExtent2D initial_window_size;
  int exit_key;
  bool multisampling_enable;
  bool window_resizable;
  NV_Window_VSync vsync_enabled;
} NVRenderer_Config;

static inline NVRenderer_Config NVRenderer_ConfigInit() {
  return (NVRenderer_Config){
      .samples              = NOVA_SAMPLE_COUNT_NO_EXTRA_SAMPLES,
      .buffer_mode          = NOVA_BUFFER_MODE_DOUBLE_BUFFERED,
      .initial_window_size  = {800, 600},
      .exit_key             = 41, /* SDL_SCANCODE_ESCAPE */
      .multisampling_enable = 0,
      .window_resizable     = 0,
      .vsync_enabled        = 1,
  };
}

typedef struct NV_SpriteRenderer NV_SpriteRenderer;
typedef struct NVRenderer_t NVRenderer_t;

extern NVRenderer_t *NVRenderer_Init(const NVRenderer_Config *conf);
extern void NVRenderer_Destroy(struct NVRenderer_t *rd);

extern bool NVRenderer_BeginRender(struct NVRenderer_t *rd);
extern void NVRenderer_EndRender(struct NVRenderer_t *rd);

extern void NVRenderer_SetClearColor(struct NVRenderer_t *rd, vec4 col);

extern int NVRenderer_GetFrame(const struct NVRenderer_t *rd);
extern struct VkCommandBuffer_T *NVRenderer_GetDrawBuffer(const NVRenderer_t *rd);
extern struct VkRenderPass_T *NVRenderer_GetRenderPass(const NVRenderer_t *rd);
extern struct NVExtent2D NVRenderer_GetRenderExtent(const NVRenderer_t *rd);
extern int NVRenderer_GetMaxFramesInFlight(const struct NVRenderer_t *rd);

extern void NVRenderer_DrawTexturedQuad(NVRenderer_t *rd, const NV_SpriteRenderer *sprite_renderer, vec3 position, vec3 size, int layer);
extern void NVRenderer_DrawQuad(NVRenderer_t *rd, NVSprite *spr, vec2 tex_coord_multiplier, vec3 position, vec3 size, vec4 color, int layer);
extern void NVRenderer_DrawLine(NVRenderer_t *rd, vec2 start, vec2 end, vec4 color, int layer);

extern NVExtent2D NV_GetWindowSize();

NOVA_HEADER_END;

#endif //__LUNA_RENDERER_H__