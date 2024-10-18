#ifndef __CGFX_DEF_H
#define __CGFX_DEF_H

// for use by things that need low level access to renderer.
// Don't even think of modifying anything unless you absolutely know what you're doing!!!

#ifdef __cplusplus
    extern "C" {
#endif

#include "cgfx.h"
#include "cgvector.h"
#include "cdevicememory.h"

// * it shold be like cg_init_ctext(rd); and then you can just do crd_render_text(&info);
typedef struct cg_ctext_module {
    // TODO Implement
    // The pipeline should most likely be static
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
    VkDescriptorPool desc_pool;
    VkDescriptorSetLayout desc_Layout;
    VkDescriptorSet desc_set;
    cgfx_gpu_image_t error_image;
    cgfx_gpu_sampler_t error_sampler;
    float model_matrix[4][4]; // what the hell?
} cg_ctext_module;

typedef struct crenderer_t
{
    crenderer_config config;

    VkRenderPass render_pass;
    cg_extent2d render_extent;

    VkSwapchainKHR swapchain;
    VkCommandPool commandPool;

    u32 attachment_count;
    u32 renderer_frame;
    u32 imageIndex;

    int shadow_image_size; // the size of ONE depth texture. Multiply by SwapchainImageCount to get total size
    cgfx_gpu_memory_t depth_image_memory;

    cgfx_gpu_image_t color_image;
    cgfx_gpu_memory_t color_image_memory;

    VkFormat depth_buffer_format;

    cg_vector_t renderData;
    cg_vector_t drawBuffers;

    cg_ctext_module *ctext;
} crenderer_t;

#ifdef __cplusplus
    }
#endif

#endif//__CGFX_DEF_H