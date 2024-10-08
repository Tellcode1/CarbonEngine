#ifndef __CGFX_H
#define __CGFX_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "defines.h"
#include "stdafx.h"
#include "vkstdafx.h"

#include "../external/volk/volk.h"
#include "containers/cvector.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#ifdef bool_t
    typedef bool_t cengine_bool_t;
#else
    typedef unsigned char cengine_bool_t;
#endif

// Move ownership to camera VV
typedef struct cg_framerender_data
{
    VkImage         swapchainImage;
    VkImageView     swapchainImageView;
    VkFramebuffer   color_framebuffer;
    VkImage         depth_image;
    VkImageView     depth_image_view;
    VkDeviceMemory  shadow_image_memory; // FIXME: move to Renderer struct to allocate all at once
    VkSemaphore     imageAvailableSemaphore;
    VkSemaphore     renderingFinishedSemaphore;
    VkFence         inFlightFence;
} cg_framerender_data;

typedef enum cengine_vsync_bits {
    CENGINE_VSYNC_DISABLED = 0,
    CENGINE_VSYNC_ENABLED = 1
} cengine_vsync_bits;
typedef cengine_bool_t cengine_vsync;

typedef enum cengine_buffering_mode_bits {
    CGFX_BUFFER_MODE_SINGLE_BUFFERED = 0,
    CGFX_BUFFER_MODE_DOUBLE_BUFFERED = 1,
    CGFX_BUFFER_MODE_TRIPLE_BUFFERED = 2,
} cengine_buffering_mode_bits;
typedef unsigned cengine_buffering_mode;

typedef enum cengine_sample_count_bits {
    CGFX_SAMPLE_COUNT_MAX_SUPPORTED = 0xFFFFFFFF,
    CGFX_SAMPLE_COUNT_NO_EXTRA_SAMPLES = 0,
    CGFX_SAMPLE_COUNT_1_SAMPLES = 0,
    CGFX_SAMPLE_COUNT_2_SAMPLES = 2,
    CGFX_SAMPLE_COUNT_4_SAMPLES = 4,
    CGFX_SAMPLE_COUNT_8_SAMPLES = 8,
    CGFX_SAMPLE_COUNT_16_SAMPLES = 16,
    CGFX_SAMPLE_COUNT_32_SAMPLES = 32,
} cengine_sample_count_bits;
typedef unsigned cengine_sample_count;

typedef struct cengine_extent2d {
    int width, height;
} cengine_extent2d;

typedef struct crenderer_config {
    cengine_sample_count   samples;
    cengine_buffering_mode buffer_mode;
    cengine_extent2d       initial_window_size;
    SDL_Scancode           exit_key;
    cengine_bool_t         multisampling_enable;
    cengine_bool_t         window_resizable;
    cengine_vsync          vsync_enabled;
} crenderer_config;

static inline crenderer_config crender_config_init() {
    return (crenderer_config) {
        .samples = CGFX_SAMPLE_COUNT_NO_EXTRA_SAMPLES,
        .buffer_mode = CGFX_BUFFER_MODE_DOUBLE_BUFFERED,
        .initial_window_size = { 800, 600 },
        .exit_key = SDL_SCANCODE_ESCAPE,
        .multisampling_enable = 0,
        .window_resizable = 0,
        .vsync_enabled = 1,
    };
}

typedef struct crenderer_t crenderer_t;
extern crenderer_t *crenderer_init(const crenderer_config *conf);
extern cengine_bool_t crenderer_begin_render(struct crenderer_t *rd);
extern void crenderer_end_render(struct crenderer_t *rd);

extern int crenderer_get_renderer_frame(const struct crenderer_t *rd);
extern struct VkCommandBuffer_T *crenderer_get_drawbuffer(const struct crenderer_t *rd);
extern struct VkRenderPass_T *crenderer_get_render_pass(const struct crenderer_t *rd);
extern struct cengine_extent2d crenderer_get_render_extent(const struct crenderer_t *rd);

extern void crenderer_destroy(struct crenderer_t *rd);

#ifdef __cplusplus
    }
#endif

#endif//__CGFX_H