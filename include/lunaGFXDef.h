#ifndef __CGFX_DEF_H
#define __CGFX_DEF_H

// for use by things that need low level access to renderer.
// Don't even think of modifying anything unless you absolutely know what you're doing!!!

#ifdef __cplusplus
    extern "C" {
#endif

#include "lunaGFX.h"
#include "lunaPipeline.h"
#include "cgvector.h"
#include "lunaGPUObjects.h"
#include "lunaDescriptors.h"
#include "sprite.h"

typedef struct cg_ctext_module {
    luna_DescriptorSet *desc_set;
    unsigned flags;
} cg_ctext_module;

typedef struct luna_QuadDrawCall_t {
    sprite_t *spr;
    vec3 siz, pos;
    vec2 tex_multiplier;
    vec4 col;
} luna_QuadDrawCall_t;

typedef struct luna_LineDrawCall_t {
    vec2 begin, end;
    vec4 col;
} luna_LineDrawCall_t;

typedef enum luna_DrawCallType {
    LUNA_DRAWCALL_QUAD = 0,
    LUNA_DRAWCALL_LINE = 1,
} luna_DrawCallType;

typedef struct luna_DrawCall_t {
    luna_DrawCallType type;
    int layer;
    union luna_DrawCallData {
        luna_LineDrawCall_t line;
        luna_QuadDrawCall_t quad;
    } drawcall;
} luna_DrawCall_t;

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

    luna_GPU_Texture *color_image;
    luna_GPU_Memory color_image_memory;

    VkFormat depth_buffer_format;

    cg_vector_t renderData;
    cg_vector_t drawBuffers;

    cg_vector_t drawcalls;

    cg_ctext_module *ctext;

    // These are used to render all the sprites in the game (quad based sprites that is)
    luna_GPU_Buffer quad_vb;
    luna_GPU_Memory quad_memory;

    void *mapped;
} luna_Renderer_t;

#ifdef __cplusplus
    }
#endif

#endif//__CGFX_DEF_H