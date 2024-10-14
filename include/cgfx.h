#ifndef __CGFX_H
#define __CGFX_H

#ifdef __cplusplus
    extern "C" {
#endif

// just keeping this here for reference
// god tier article btw
// https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/

#include "defines.h"
#include "stdafx.h"
#include "vkstdafx.h"
#include "cgfxstdafx.h"
#include "cdevicememory.h"

// Move ownership to camera VV
typedef struct cg_framerender_data
{
    cgfx_gpu_image_t sc_image; // sc -> swapchain owned
    cgfx_gpu_image_t depth_image;
    VkFramebuffer    color_framebuffer;
    VkSemaphore      image_available_semaphore;
    VkSemaphore      render_finish_semaphore;
    VkFence          in_flight_fence;
} cg_framerender_data;

typedef struct crenderer_config {
    cg_sample_count   samples;
    cg_buffering_mode buffer_mode;
    cg_extent2d            initial_window_size;
    int                    exit_key;
    cg_bool_t              multisampling_enable;
    cg_bool_t              window_resizable;
    cg_vsync               vsync_enabled;
} crenderer_config;

static inline crenderer_config crender_config_init() {
    return (crenderer_config) {
        .samples = CG_SAMPLE_COUNT_NO_EXTRA_SAMPLES,
        .buffer_mode = CG_BUFFER_MODE_DOUBLE_BUFFERED,
        .initial_window_size = { 800, 600 },
        .exit_key = 41, /* SDL_SCANCODE_ESCAPE */
        .multisampling_enable = 0,
        .window_resizable = 0,
        .vsync_enabled = 1,
    };
}

typedef struct crenderer_t crenderer_t;

extern crenderer_t *crenderer_init(const crenderer_config *conf);

extern cg_bool_t crd_begin_render(struct crenderer_t *rd);
extern void crd_end_render(struct crenderer_t *rd);

extern int crd_get_renderer_frame(const struct crenderer_t *rd);
extern struct VkCommandBuffer_T *crd_get_drawbuffer(const crenderer_t *rd);
extern struct VkRenderPass_T *crd_get_render_pass(const crenderer_t *rd);
extern struct cg_extent2d crd_get_render_extent(const crenderer_t *rd);

extern void crenderer_destroy(struct crenderer_t *rd);

#ifdef __cplusplus
    }
#endif

#endif//__CGFX_H