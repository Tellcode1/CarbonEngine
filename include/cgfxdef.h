#ifndef __CGFX_DEF_H
#define __CGFX_DEF_H

// for use by things that need low level access to renderer.
// Don't even think of modifying anything unless you absolutely know what you're doing!!!

#ifdef __cplusplus
    extern "C" {
#endif

#include "../include/cgfx.h"

typedef struct crenderer_t
{
    crenderer_config config;

    VkRenderPass render_pass;
    cengine_extent2d render_extent;

    VkSwapchainKHR swapchain;
    VkCommandPool commandPool;

    u32 attachment_count;
    u32 renderer_frame;
    u32 imageIndex;

    VkImage color_image;
    VkDeviceMemory color_image_memory;
    VkImageView color_image_view;

    VkFormat depth_buffer_format;

    cvector_t *renderData;
    cvector_t *drawBuffers;
} crenderer_t;

#ifdef __cplusplus
    }
#endif

#endif//__CGFX_DEF_H