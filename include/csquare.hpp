#ifndef __SQUARE_HPP_
#define __SQUARE_HPP_

#include "vkstdafx.h"
#include "vkhelper.h"
#include "cvk.h"
#include "math/vec3.hpp"
#include "math/vec2.hpp"
#include "math/mat.hpp"

struct cvertex {
    cm::vec3 position;
    cm::vec2 texcoord;

    bool operator==(const cvertex& other) const {
        return position == other.position && texcoord == other.texcoord;
    }
};

static const int ccube_index_offset = sizeof(cvertex) * 8;

static const cvertex ccube_vertices[8] = {
    (cvertex){ { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f } },
    (cvertex){ {  1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f } },
    (cvertex){ {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f } },
    (cvertex){ { -1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f } },

    (cvertex){ { -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f } },
    (cvertex){ {  1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f } },
    (cvertex){ {  1.0f,  1.0f, -1.0f }, { 1.0f, 1.0f } },
    (cvertex){ { -1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f } }
};

static const u32 ccube_indices[36] = {
    0, 1, 2,   2, 3, 0,
    4, 5, 6,   6, 7, 4,
    4, 0, 3,   3, 7, 4,
    1, 5, 6,   6, 2, 1,
    3, 2, 6,   6, 7, 3,
    4, 5, 1,   1, 0, 4
};

static const int ccube_total_data_size = sizeof(cvertex) * 8 + sizeof(u32) * 36;

struct texture2d_t {
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
};

struct ctransform {
    cm::vec3 position;
    cm::vec3 scale;
    cm::vec3 rotation;
};

struct cpipeline {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
};

struct csquare_t {
    ctransform transform;
    cpipeline pipeline;
    texture2d_t *texture;

    VkBuffer buf;
    VkDeviceMemory mem;
};

int ccreate_cube(crenderer_t *rd, csquare_t *dst) {
    vkh_buffer_create(
        ccube_total_data_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &dst->buf, &dst->mem, 0
    );

    void *mapped;
    vkMapMemory(device, dst->mem, 0, ccube_total_data_size, 0, &mapped);
    memcpy(mapped, ccube_vertices, sizeof(cvertex) * 8);
    memcpy((char *)mapped + ccube_index_offset, ccube_indices, sizeof(u32) * 36);
    vkUnmapMemory(device, dst->mem);

    const VkVertexInputAttributeDescription attributeDescriptions[] = {
        // location; binding; format; offset;
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // pos
        { 1, 0, VK_FORMAT_R32G32_SFLOAT,    sizeof(cm::vec3) }, // uv
    };

    const VkVertexInputBindingDescription bindingDescriptions[] = {
        // binding; stride; inputRate
        { 0, sizeof(cvertex), VK_VERTEX_INPUT_RATE_VERTEX }
    };

    const VkPushConstantRange pushConstants[] = { 
        // stageFlags, offset, size
        { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(cm::mat4) * 2 }
    };

    csm_shader_t *vertex, *fragment;
    assert(csm_load_shader("Unlit/vert", &vertex) != -1);
    assert(csm_load_shader("Unlit/frag", &fragment) != -1);
    const csm_shader_t * shaders[] = { vertex, fragment };

    cvk_pipeline_create_info pc{};
    pc.format = SwapChainImageFormat;
    pc.subpass = 0;
    pc.render_pass = crenderer_get_render_pass(rd);

    pc.nAttributeDescriptions = array_len(attributeDescriptions);
    pc.pAttributeDescriptions = attributeDescriptions;

    pc.nPushConstants = array_len(pushConstants);
    pc.pPushConstants = pushConstants;

    pc.nBindingDescriptions = array_len(bindingDescriptions);
    pc.pBindingDescriptions = bindingDescriptions;

    pc.nShaders = array_len(shaders);
    pc.pShaders = shaders;

    const cengine_extent2d RenderExtent = crenderer_get_render_extent(rd);
    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.samples = Samples;
    cvk_create_pipeline_layout(device, &pc, &dst->pipeline.pipeline_layout);
    pc.pipeline_layout = dst->pipeline.pipeline_layout;
    cvk_create_graphics_pipeline(device, &pc, &dst->pipeline.pipeline, CVK_PIPELINE_FLAGS_UNFORCE_CULLING);

    return 0;
}

void render_cube(crenderer_t *rd, ccamera camera, const csquare_t *cube) {
    struct push_constants {
        cm::mat4 view;
        cm::mat4 projection;
    } pc;

    pc.view = camera.get_view();
    pc.projection = camera.get_projection();

    const VkDeviceSize offsets[1] = {};

    const VkCommandBuffer cmd = crenderer_get_drawbuffer(rd);
    vkCmdBindVertexBuffers(cmd, 0, 1, &cube->buf, offsets);
    vkCmdPushConstants(cmd, cube->pipeline.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants), &pc);
    vkCmdBindIndexBuffer(cmd, cube->buf, ccube_index_offset, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cube->pipeline.pipeline);
    vkCmdDrawIndexed(cmd, array_len(ccube_indices), 1, 0, 0, 0);
}

#endif//__SQUARE_HPP_