#ifndef __CGFX_DEF_H
#define __CGFX_DEF_H

// for use by things that need low level access to renderer.
// Don't even think of modifying anything unless you absolutely know what you're doing!!!

#ifdef __cplusplus
    extern "C" {
#endif

#include "lunaGFX.h"
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
    luna_GPU_Texture error_image;
    luna_GPU_Sampler error_sampler;
    float model_matrix[4][4]; // what the hell?
} cg_ctext_module;

typedef enum cg_renderer_flag_bits {
    CG_RENDERER_MULTISAMPLING_ENABLE = 1 << 0,
    CG_RENDERER_VSYNC_ENABLE = 1 << 1,
    CG_RENDERER_WINDOW_RESIZABLE = 1 << 2,
} cg_renderer_flag_bits;

typedef struct luna_Renderer_t
{
    unsigned flags;
    lunaBufferMode buffer_mode;

    VkRenderPass render_pass;
    lunaExtent2D render_extent;

    VkSwapchainKHR swapchain;
    VkCommandPool commandPool;

    u32 attachment_count;
    u32 renderer_frame;
    u32 imageIndex;

    int shadow_image_size; // the size of ONE depth texture. Multiply by SwapchainImageCount to get total size
    luna_GPU_Memory depth_image_memory;

    luna_GPU_Texture color_image;
    luna_GPU_Memory color_image_memory;

    VkFormat depth_buffer_format;

    cg_vector_t renderData;
    cg_vector_t drawBuffers;

    cg_ctext_module *ctext;
} luna_Renderer_t;

#ifdef __cplusplus
    }
#endif

#endif//__CGFX_DEF_H