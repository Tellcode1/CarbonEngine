#ifndef __CGFX_H
#define __CGFX_H

#ifdef __cplusplus
    extern "C" {
#endif

// just keeping this here for reference
// god tier article btw
// https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/

// I strive for a world where I do not have to call vulkan functions myself again

#include "../GPU/descriptors.h"

#include "../math/vec2.h"
#include "../math/vec3.h"
#include "../math/vec4.h"

typedef struct luna_GPU_Texture luna_GPU_Texture;

extern luna_DescriptorPool g_pool;
extern struct lunaCamera camera;
typedef struct lunaSprite lunaSprite;

typedef enum luna_Window_OptionBits {
    LUNA_WINDOW_OPTION_VSYNC           = 1 << 0,
    LUNA_WINDOW_OPTION_RESIZABLE       = 1 << 1,
    LUNA_WINDOW_OPTION_BORDERLESS      = 1 << 2,
    LUNA_WINDOW_OPTION_FULLSCREEN      = 1 << 3,
    LUNA_WINDOW_OPTION_MAXIMIZED       = 1 << 4,
    LUNA_WINDOW_OPTION_MINIMIZED       = 1 << 5,
    LUNA_WINDOW_OPTION_HIDDEN          = 1 << 6,
    LUNA_WINDOW_OPTION_HIGH_DPI        = 1 << 7,
    LUNA_WINDOW_OPTION_ALWAYS_ON_TOP   = 1 << 8,
} luna_Window_OptionBits;

typedef enum luna_Window_VSyncBits {
    LUNA_WINDOW_VSYNC_DISABLED = 0,
    LUNA_WINDOW_VSYNC_ENABLED = 1
} luna_Window_VSyncBits;
typedef bool luna_Window_VSync;

typedef enum luna_Window_BufferModeBits {
    CG_BUFFER_MODE_SINGLE_BUFFERED = 0,
    CG_BUFFER_MODE_DOUBLE_BUFFERED = 1,
    CG_BUFFER_MODE_TRIPLE_BUFFERED = 2,
} luna_Window_BufferModeBits;
typedef unsigned lunaBufferMode;

typedef enum cg_sample_count_bits {
    CG_SAMPLE_COUNT_MAX_SUPPORTED = 0xFFFFFFFF,
    CG_SAMPLE_COUNT_NO_EXTRA_SAMPLES = 1,
    CG_SAMPLE_COUNT_1_SAMPLES = 1,
    CG_SAMPLE_COUNT_2_SAMPLES = 2,
    CG_SAMPLE_COUNT_4_SAMPLES = 4,
    CG_SAMPLE_COUNT_8_SAMPLES = 8,
    CG_SAMPLE_COUNT_16_SAMPLES = 16,
    CG_SAMPLE_COUNT_32_SAMPLES = 32,
} cg_sample_count_bits;
typedef unsigned lunaSampleCount;

typedef struct lunaExtent2D {
    int width, height;
} lunaExtent2D;

// Move ownership to camera VV
typedef struct lunaFrameRenderData
{
    luna_GPU_Texture *sc_image; // sc -> swapchain owned
    luna_GPU_Texture *depth_image;
    VkFramebuffer    color_framebuffer;
    VkSemaphore      image_available_semaphore;
    VkSemaphore      render_finish_semaphore;
    VkFence          in_flight_fence;
} lunaFrameRenderData;

typedef enum cg_renderer_flag_bits {
    CG_RENDERER_MULTISAMPLING_ENABLE = 1 << 0,
    CG_RENDERER_VSYNC_ENABLE = 1 << 1,
    CG_RENDERER_WINDOW_RESIZABLE = 1 << 2,
} cg_renderer_flag_bits;

typedef struct lunaRenderer_Config {
    lunaSampleCount        samples;
    lunaBufferMode         buffer_mode;
    lunaExtent2D           initial_window_size;
    int                    exit_key;
    bool                   multisampling_enable;
    bool                   window_resizable;
    luna_Window_VSync      vsync_enabled;
} lunaRenderer_Config;

static inline lunaRenderer_Config crender_config_init() {
    return (lunaRenderer_Config) {
        .samples = CG_SAMPLE_COUNT_NO_EXTRA_SAMPLES,
        .buffer_mode = CG_BUFFER_MODE_DOUBLE_BUFFERED,
        .initial_window_size = { 800, 600 },
        .exit_key = 41, /* SDL_SCANCODE_ESCAPE */
        .multisampling_enable = 0,
        .window_resizable = 0,
        .vsync_enabled = 1,
    };
}

typedef struct luna_SpriteRenderer luna_SpriteRenderer;
typedef struct lunaRenderer_t lunaRenderer_t;

extern lunaRenderer_t *lunaRenderer_Init(const lunaRenderer_Config *conf);
extern void lunaRenderer_Destroy(struct lunaRenderer_t *rd);

extern bool lunaRenderer_BeginRender(struct lunaRenderer_t *rd);
extern void lunaRenderer_EndRender(struct lunaRenderer_t *rd);

extern int lunaRenderer_GetFrame(const struct lunaRenderer_t *rd);
extern struct VkCommandBuffer_T *lunaRenderer_GetDrawBuffer(const lunaRenderer_t *rd);
extern struct VkRenderPass_T *lunaRenderer_GetRenderPass(const lunaRenderer_t *rd);
extern struct lunaExtent2D lunaRenderer_GetRenderExtent(const lunaRenderer_t *rd);
extern int lunaRenderer_GetMaxFramesInFlight(const struct lunaRenderer_t *rd);

extern void lunaRenderer_DrawTexturedQuad(lunaRenderer_t *rd, luna_SpriteRenderer *sprite_renderer, vec3 position, vec3 size, int layer);
extern void lunaRenderer_DrawQuad(
    lunaRenderer_t *rd,
    lunaSprite *spr,
    vec2 tex_coord_multiplier,
    vec3 position,
    vec3 size,
    vec4 color,
    int layer
);
extern void lunaRenderer_DrawLine(lunaRenderer_t *rd, vec2 start, vec2 end, vec4 color, int layer);

extern lunaExtent2D luna_GetWindowSize();

#ifdef __cplusplus
    }
#endif

#endif//__CGFX_H