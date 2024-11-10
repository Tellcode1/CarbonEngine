#ifndef __CGFX_H
#define __CGFX_H

#ifdef __cplusplus
    extern "C" {
#endif

// just keeping this here for reference
// god tier article btw
// https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/

// I strive for a world where I do not have to call vulkan functions myself again

#include "defines.h"
#include "stdafx.h"
#include "vkstdafx.h"
#include "lunaGFXstdafx.h"
#include "cdevicememory.h"

// Move ownership to camera VV
typedef struct lunaFrameRenderData
{
    luna_GPU_Texture sc_image; // sc -> swapchain owned
    luna_GPU_Texture depth_image;
    VkFramebuffer    color_framebuffer;
    VkSemaphore      image_available_semaphore;
    VkSemaphore      render_finish_semaphore;
    VkFence          in_flight_fence;
} lunaFrameRenderData;

typedef struct luna_Renderer_Config {
    lunaSampleCount        samples;
    lunaBufferMode      buffer_mode;
    lunaExtent2D           initial_window_size;
    int                    exit_key;
    bool                   multisampling_enable;
    bool                   window_resizable;
    luna_Window_VSync              vsync_enabled;
} luna_Renderer_Config;

static inline luna_Renderer_Config crender_config_init() {
    return (luna_Renderer_Config) {
        .samples = CG_SAMPLE_COUNT_NO_EXTRA_SAMPLES,
        .buffer_mode = CG_BUFFER_MODE_DOUBLE_BUFFERED,
        .initial_window_size = { 800, 600 },
        .exit_key = 41, /* SDL_SCANCODE_ESCAPE */
        .multisampling_enable = 0,
        .window_resizable = 0,
        .vsync_enabled = 1,
    };
}

typedef struct luna_Renderer_t luna_Renderer_t;

extern luna_Renderer_t *luna_Renderer_Init(const luna_Renderer_Config *conf);
extern void luna_Renderer_Destroy(struct luna_Renderer_t *rd);

extern bool luna_Renderer_BeginRender(struct luna_Renderer_t *rd);
extern void luna_Renderer_EndRender(struct luna_Renderer_t *rd);

extern int luna_Renderer_GetFrame(const struct luna_Renderer_t *rd);
extern struct VkCommandBuffer_T *luna_Renderer_GetDrawBuffer(const luna_Renderer_t *rd);
extern struct VkRenderPass_T *luna_Renderer_GetRenderPass(const luna_Renderer_t *rd);
extern struct lunaExtent2D luna_Renderer_GetRenderExtent(const luna_Renderer_t *rd);

#ifdef __cplusplus
    }
#endif

#endif//__CGFX_H