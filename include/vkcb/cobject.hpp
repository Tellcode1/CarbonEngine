#ifndef __C_OBJECT_HPP__
#define __C_OBJECT_HPP__

#include "vkcbstdafx.hpp"
#include <functional>

constexpr inline u32 bit(u32 idx) {
    return 1 << idx;
};


enum cobject_shape {
    COBJECT_SHAPE_QUAD      = bit(0),
    COBJECT_SHAPE_CIRCLE    = bit(2),
    COBJECT_SHAPE_DYNAMIC   = bit(3),
};
enum cobject_indexing {
    COBJECT_INDEXING_DISABLED = 0,
    COBJECT_INDEXING_ENABLED  = 1
};

typedef u32 cobject_flags;

enum cvkpipeline_flag_bits {
    CVK_PIPELINE_DESCRIPTORS_ENABLED   = bit(0),
    CVK_PIPELINE_PUSH_CONSTANTS_ENABLE = bit(1),
    CVK_PIPELINE_INDEXING_ENABLE       = bit(2),
    CVK_PIPELINE_RENDER_DISABLE        = bit(3),
};
typedef u32 cvkpipeline_flags;

struct ctransform;
struct cobject_base;
struct cvkpipeline;

typedef std::function<void(VkCommandBuffer cmd)> cvk_render_fn;

void cvk_bind_pipeline(const VkCommandBuffer cmd, const cvkpipeline *pipeline);
cobject_base create_sprite(cobject_shape shape, cobject_indexing indexing);
void render(cobject_base *spr);

struct ctransform {
    f32 position[2];
    f32 size[2]; // for circle, size[0] == size[1]
    f32 rotation;
};

struct cvkbuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    i32 offset;
};

struct cvkpipeline {
    friend struct cobject_base;
    friend cobject_base create_sprite(cobject_shape shape, cobject_indexing indexing);

    cvkpipeline_flags flags;

    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    cvector<VkDescriptorSet> desc_sets;
    VkDescriptorSetLayout desc_set_layout;

    /* vertex count || index count */
    i32 draw_count;

    // for "compatibility" (just an excuse so I dont have to figure out how to not make dynamic allocs)
    // ++ my pc only supports 128 bytes so you have to suffer or buy me a new pc ;D
    u8 push_constant_data[ 128 ];
    VkShaderStageFlagBits push_constant_shader_stages;
    i32 push_constant_size;

    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    cvector<VkBuffer> vertex_buffers;
    cvector<VkDeviceSize> vertex_buffer_offsets;

    VkBuffer index_buffer;
    i32 index_buffer_offset;

    CARBON_FORCE_INLINE void bind(const VkCommandBuffer cmd) const {
        cvk_bind_pipeline(cmd, this);
    }

    CARBON_FORCE_INLINE i32 get_id() const {
        return id;
    }

    CARBON_FORCE_INLINE void set_render_fn(cvk_render_fn new_render_fn) {
        render_fn = new_render_fn;
    }

    private:
    cvk_render_fn render_fn;
    i32 id;
};

struct cobject_base {
    ctransform transform;
    cvkpipeline pipeline;
    cobject_flags flags;

    CARBON_FORCE_INLINE void set_render_fn(cvk_render_fn new_render_fn) {
        pipeline.set_render_fn(new_render_fn);
    }

    virtual void render(const VkCommandBuffer cmd) = 0;
};

#endif//__C_OBJECT_HPP__