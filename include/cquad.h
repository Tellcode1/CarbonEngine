#ifndef __SQUARE_HPP_
#define __SQUARE_HPP_

#include "cgfx.h"
#include "camera.h"
#include "cshadermanager.h"
#include "vkhelper.h"
#include "cvk.h"
#include "math/vec3.h"
#include "math/vec2.h"
#include "math/mat.h"

typedef struct cvertex {
    vec3 position;
    vec2 texcoord;
} cvertex;

static const cvertex ccube_vertices[4] = {
    (cvertex){ { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f } },
    (cvertex){ {  1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f } },
    (cvertex){ {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f } },
    (cvertex){ { -1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f } },
};
static const int ccube_index_offset = sizeof(ccube_vertices);

static const u32 ccube_indices[6] = {
    0, 1, 2,   2, 3, 0,
};

static const int ccube_total_data_size = sizeof(ccube_vertices) + sizeof(ccube_indices);

typedef struct texture2d_t {
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
} texture2d_t;

typedef struct ctransform {
    vec3 position;
    vec3 scale;
    vec3 rotation;
} ctransform;

typedef struct cpipeline {
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
} cpipeline;

typedef struct csquare_t {
    ctransform transform;
    cpipeline pipeline;
    texture2d_t *texture;

    VkBuffer buf;
    VkDeviceMemory mem;
} csquare_t;

int ccreate_cube(crenderer_t *rd, csquare_t *dst) {
    vkh_buffer_create(
        ccube_total_data_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &dst->buf, &dst->mem, 0
    );

    void *mapped;
    vkMapMemory(device, dst->mem, 0, ccube_total_data_size, 0, &mapped);
    memcpy(mapped, ccube_vertices, sizeof(ccube_vertices));
    memcpy((char *)mapped + ccube_index_offset, ccube_indices, sizeof(ccube_indices));
    vkUnmapMemory(device, dst->mem);

    const VkVertexInputAttributeDescription attributeDescriptions[] = {
        // location; binding; format; offset;
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // pos
        { 1, 0, VK_FORMAT_R32G32_SFLOAT,    sizeof(vec3) }, // uv
    };

    const VkVertexInputBindingDescription bindingDescriptions[] = {
        // binding; stride; inputRate
        { 0, sizeof(cvertex), VK_VERTEX_INPUT_RATE_VERTEX }
    };

    const VkPushConstantRange pushConstants[] = { 
        // stageFlags, offset, size
        { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4) * 2 }
    };

    csm_shader_t *vertex, *fragment;
    assert(csm_load_shader("Unlit/vert", &vertex) != -1);
    assert(csm_load_shader("Unlit/frag", &fragment) != -1);
    const csm_shader_t * shaders[] = { vertex, fragment };

    cvk_pipeline_create_info pc = cvk_init_pipeline_create_info();
    pc.format = SwapChainImageFormat;
    pc.subpass = 0;
    pc.render_pass = crd_get_render_pass(rd);

    pc.nAttributeDescriptions = array_len(attributeDescriptions);
    pc.pAttributeDescriptions = attributeDescriptions;

    pc.nPushConstants = array_len(pushConstants);
    pc.pPushConstants = pushConstants;

    pc.nBindingDescriptions = array_len(bindingDescriptions);
    pc.pBindingDescriptions = bindingDescriptions;

    pc.nShaders = array_len(shaders);
    pc.pShaders = shaders;

    const cg_extent2d RenderExtent = crd_get_render_extent(rd);
    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.samples = Samples;
    cvk_create_pipeline_layout(&pc, &dst->pipeline.pipeline_layout);
    pc.pipeline_layout = dst->pipeline.pipeline_layout;
    cvk_create_graphics_pipeline(&pc, &dst->pipeline.pipeline, CVK_PIPELINE_FLAGS_UNFORCE_CULLING);

    return 0;
}

void render_cube(crenderer_t *rd, ccamera *camera, const csquare_t *cube) {
    struct push_constants {
        mat4 view;
        mat4 projection;
    } pc;

    pc.view = cam_get_view(camera);
    pc.projection = cam_get_projection(camera);

    const VkDeviceSize offsets[1] = {};

    const VkCommandBuffer cmd = crd_get_drawbuffer(rd);
    vkCmdBindVertexBuffers(cmd, 0, 1, &cube->buf, offsets);
    vkCmdPushConstants(cmd, cube->pipeline.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(struct push_constants), &pc);
    vkCmdBindIndexBuffer(cmd, cube->buf, ccube_index_offset, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, cube->pipeline.pipeline);
    vkCmdDrawIndexed(cmd, array_len(ccube_indices), 1, 0, 0, 0);
}

#endif//__SQUARE_HPP_