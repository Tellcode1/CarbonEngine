#ifndef __CGFX_DEF_H
#define __CGFX_DEF_H

// for use by things that need low level access to renderer.
// Don't even think of modifying anything unless you absolutely know what you're doing!!!

#ifdef __cplusplus
    extern "C" {
#endif

#include "../include/cgfx.h"

// * it shold be like cg_init_ctext(rd); and then you can just do crd_render_text(&info);
typedef struct cg_ctext_module {
    // TODO Implement
    VkDescriptorPool desc_pool;
    VkDescriptorSetLayout desc_Layout;
    VkDescriptorSet desc_set;
    VkImage error_image;
    VkImageView error_image_view;
    VkSampler error_image_sampler;
    float model_matrix[4][4];
} cg_ctext_module;

typedef struct crenderer_t
{
    struct cg_device_t *device;
    crenderer_config config;

    VkRenderPass render_pass;
    cg_extent2d render_extent;

    VkSwapchainKHR swapchain;
    VkCommandPool commandPool;

    u32 attachment_count;
    u32 renderer_frame;
    u32 imageIndex;

    VkImage color_image;
    VkDeviceMemory color_image_memory;
    VkImageView color_image_view;

    VkFormat depth_buffer_format;

    struct cg_vector_t *renderData;
    struct cg_vector_t *drawBuffers;

    cg_ctext_module *ctext;
} crenderer_t;

#ifdef __cplusplus
    }
#endif

#endif//__CGFX_DEF_H