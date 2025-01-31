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

extern NV_descriptor_pool g_pool;
extern struct NV_camera_t camera;
typedef struct NV_sprite NV_sprite;

typedef enum NV_window_flag_bits {
  NOVA_WINDOW_VSYNC         = 1 << 0,
  NOVA_WINDOW_RESIZABLE     = 1 << 1,
  NOVA_WINDOW_BORDERLESS    = 1 << 2,
  NOVA_WINDOW_FULLSCREEN    = 1 << 3,
  NOVA_WINDOW_MAXIMIZED     = 1 << 4,
  NOVA_WINDOW_MINIMIZED     = 1 << 5,
  NOVA_WINDOW_HIDDEN        = 1 << 6,
  NOVA_WINDOW_HIGH_DPI      = 1 << 7,
  NOVA_WINDOW_ALWAYS_ON_TOP = 1 << 8,
} NV_window_flag_bits;

typedef enum NVWindow_VSyncBits { NOVA_WINDOW_VSYNC_DISABLED = 0, NOVA_WINDOW_VSYNC_ENABLED = 1 } NVWindow_VSyncBits;
typedef bool NV_window_vsync;

typedef enum NV_buffer_mode_bits {
  NOVA_BUFFER_MODE_SINGLE_BUFFERED = 0,
  NOVA_BUFFER_MODE_DOUBLE_BUFFERED = 1,
  NOVA_BUFFER_MODE_TRIPLE_BUFFERED = 2,
} NV_buffer_mode_bits;
typedef unsigned NV_buffer_mode;

typedef enum NV_sample_count_bits {
  NOVA_SAMPLE_COUNT_MAX_SUPPORTED    = 0xFFFFFFFF,
  NOVA_SAMPLE_COUNT_NO_EXTRA_SAMPLES = 1,
  NOVA_SAMPLE_COUNT_1_SAMPLES        = 1,
  NOVA_SAMPLE_COUNT_2_SAMPLES        = 2,
  NOVA_SAMPLE_COUNT_4_SAMPLES        = 4,
  NOVA_SAMPLE_COUNT_8_SAMPLES        = 8,
  NOVA_SAMPLE_COUNT_16_SAMPLES       = 16,
  NOVA_SAMPLE_COUNT_32_SAMPLES       = 32,
} NV_sample_count_bits;
typedef unsigned NV_sample_count;

typedef struct NV_extent2d {
  int width, height;
} NV_extent2d;

typedef struct NV_extent3D {
  int width, height, depth;
} NV_extent3D;

// Move ownership to camera VV
typedef struct NV_renderer_frame_render_info {
  NV_GPU_Texture *sc_image; // sc -> swapchain owned
  NV_GPU_Texture *depth_image;
  VkFramebuffer color_framebuffer;
  VkSemaphore image_available_semaphore;
  VkSemaphore render_finish_semaphore;
  VkFence in_flight_fence;
} NV_renderer_frame_render_info;

typedef enum NV_renderer_flag_bits {
  NOVA_RENDERER_MULTISAMPLING_ENABLE = 1 << 0,
  NOVA_RENDERER_VSYNC_ENABLE         = 1 << 1,
  NOVA_RENDERER_WINDOW_RESIZABLE     = 1 << 2,
} NV_renderer_flag_bits;

typedef struct NV_renderer_config {
  NV_sample_count samples;
  NV_buffer_mode buffer_mode;
  NV_extent2d initial_window_size;
  int exit_key;
  bool multisampling_enable;
  bool window_resizable;
  NV_window_vsync vsync_enabled;
} NV_renderer_config;

static inline NV_renderer_config NV_renderer_configInit() {
  return (NV_renderer_config){
      .samples              = NOVA_SAMPLE_COUNT_NO_EXTRA_SAMPLES,
      .buffer_mode          = NOVA_BUFFER_MODE_DOUBLE_BUFFERED,
      .initial_window_size  = {800, 600},
      .multisampling_enable = 0,
      .window_resizable     = 0,
      .vsync_enabled        = 1,
  };
}

typedef struct NV_SpriteRenderer NV_SpriteRenderer;
typedef struct NV_renderer_t NV_renderer_t;

extern NV_renderer_t *NVRenderer_Init(const NV_renderer_config *conf);
extern void NVRenderer_Destroy(struct NV_renderer_t *rd);

extern bool NV_renderer_begin(struct NV_renderer_t *rd);
extern void NV_renderer_end(struct NV_renderer_t *rd);

extern void NV_renderer_set_clear_color(struct NV_renderer_t *rd, vec4 col);

extern int NV_renderer_get_frame(const struct NV_renderer_t *rd);
extern struct VkCommandBuffer_T *NV_renderer_get_draw_buffer(const NV_renderer_t *rd);
extern struct VkRenderPass_T *NV_renderer_get_render_pass(const NV_renderer_t *rd);
extern struct NV_extent2d NV_renderer_get_render_extent(const NV_renderer_t *rd);
extern int NV_renderer_get_max_frames_in_flight(const struct NV_renderer_t *rd);

extern void NV_renderer_render_textured_quad(NV_renderer_t *rd, const NV_SpriteRenderer *sprite_renderer, vec3 position, vec3 size, int layer);
extern void NV_renderer_render_quad(NV_renderer_t *rd, NV_sprite *spr, vec2 tex_coord_multiplier, vec3 position, vec3 size, vec4 color, int layer);
extern void NV_renderer_render_line(NV_renderer_t *rd, vec2 start, vec2 end, vec4 color, int layer);

extern NV_extent2d NV_get_window_size();

NOVA_HEADER_END;

#endif //__LUNA_RENDERER_H__