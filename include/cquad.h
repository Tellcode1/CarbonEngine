#ifndef __C_MESH_H__
#define __C_MESH_H__

#include <stdio.h>
#include "defines.h"

#include "lunaPipeline.h"
#include "lunaVK.h"
#include "cshadermanager.h"

#include "math/vec3.h"
#include "math/vec2.h"
#include "math/mat.h"

#include "lunaImage.h"
#include "sprite.h"
#include "undescriptorset.h"

#include "../external/box2d/include/box2d/box2d.h"

typedef struct Transform {
    vec3 position;
    vec3 scale;
    vec3 rotation;
} Transform;

typedef struct ubdata {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubdata;

typedef struct cmesh_t {
    Transform transform;

    VkBuffer vb;
    VkDeviceMemory vbm;

    VkBuffer ib;
    VkDeviceMemory ibm;

    void *ubmapped;

    luna_GPU_Sampler sampler;
    sprite_t *spr;

    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    undescriptorset set;

    int index_count;
    b2BodyId body;
} cmesh_t;

typedef struct cvertex {
    vec3 position;
    vec2 texcoord;
} cvertex;

static const float HALF_WIDTH = 0.5f;
static const cvertex ccube_vertices[4] = {
    (cvertex){ (vec3){ HALF_WIDTH,  HALF_WIDTH, 0.0f}, { 0.0f, 0.0f } },
    (cvertex){ (vec3){-HALF_WIDTH,  HALF_WIDTH, 0.0f}, { 1.0f, 0.0f } },
    (cvertex){ (vec3){-HALF_WIDTH, -HALF_WIDTH, 0.0f}, { 1.0f, 1.0f } },
    (cvertex){ (vec3){ HALF_WIDTH, -HALF_WIDTH, 0.0f}, { 0.0f, 1.0f } },
};
static const int ccube_index_offset = sizeof(ccube_vertices);

static const u32 ccube_indices[6] = {
    0, 1, 2,
    2, 3, 0,
};

static int ub_alignment = -1;

typedef struct body_parameters {
    b2WorldId world;
    vec3 position;
    vec3 scale;
    b2BodyType type;
    f32 density;
    f32 friction;
    f32 restitution;
    uint64_t category,mask;
} body_parameters;

int align_up(int size, int alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

void cmesh_mk_b2body(body_parameters *body, cmesh_t *mesh) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = body->type;
    bodyDef.position = (b2Vec2){mesh->transform.position.x, mesh->transform.position.y};
    bodyDef.fixedRotation = 1;
    mesh->body = b2CreateBody(body->world, &bodyDef);

    b2Polygon dynamicBox = b2MakeBox(mesh->transform.scale.x, 3.0f / mesh->transform.scale.y);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.friction = 0.0f;
    shapeDef.restitution = 0.0f;
    b2CreatePolygonShape(mesh->body, &shapeDef, &dynamicBox);
}

cmesh_t *load_mesh(sprite_t *spr, body_parameters *body, luna_Renderer_t *rd) {
    cmesh_t *mesh = (cmesh_t *)calloc(1, sizeof(cmesh_t));
    if (spr != NULL) {
        mesh->spr = spr;
    } else {
        mesh->spr = sprite_empty;
    }
    mesh->index_count = array_len(ccube_indices);

    const int vertex_size = sizeof(ccube_vertices);
    const int indices_size = sizeof(ccube_indices);

    mesh->transform.position = body->position;
    mesh->transform.scale = body->scale;

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physDevice, &properties);

    ub_alignment = properties.limits.minUniformBufferOffsetAlignment;

    luna_VK_CreateBuffer(
        align_up(vertex_size, ub_alignment) + sizeof(ubdata),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &mesh->vb, &mesh->vbm, 0
    );

    luna_VK_CreateBuffer(
        indices_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &mesh->ib, &mesh->ibm, 0
    );

    cvertex *vertex_mapped;
    u32 *index_mapped;
    vkMapMemory(device, mesh->vbm, 0, vertex_size, 0, (void **)&vertex_mapped);
    vkMapMemory(device, mesh->ibm, 0, indices_size, 0, (void **)&index_mapped);

    vertex_mapped[0] = ccube_vertices[0];
    vertex_mapped[1] = ccube_vertices[1];
    vertex_mapped[2] = ccube_vertices[2];
    vertex_mapped[3] = ccube_vertices[3];

    index_mapped[0] = 0;
    index_mapped[1] = 1;
    index_mapped[2] = 2;
    index_mapped[3] = 2;
    index_mapped[4] = 3;
    index_mapped[5] = 0;

    vkUnmapMemory(device, mesh->vbm);
    vkUnmapMemory(device, mesh->ibm);

    vkMapMemory(device, mesh->vbm, 0, sizeof(ubdata), 0, &mesh->ubmapped);

    const VkVertexInputAttributeDescription attributeDescriptions[] = {
        // location; binding; format; offset;
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // pos
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(vec3) }, // texcoord
    };

    const VkVertexInputBindingDescription bindingDescriptions[] = {
        // binding; stride; inputRate
        { 0, sizeof(cvertex), VK_VERTEX_INPUT_RATE_VERTEX }
    };

    const VkPushConstantRange pushConstants[] = { 
        // stageFlags, offset, size
        {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vec3)}
    };
    luna_GPU_SamplerCreateInfo sampler_info = {
       .filter = VK_FILTER_LINEAR,
       .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
       .address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT 
    };
    luna_GPU_CreateSampler(&sampler_info, &mesh->sampler);

    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &mesh->sampler.vksampler },
    };
    create_undescriptor_set(&mesh->set, bindings, array_len(bindings));

    VkDescriptorBufferInfo bufferinfo = {
        .buffer = mesh->vb,
        .offset = align_up(vertex_size, ub_alignment),
        .range = sizeof(ubdata)
    };

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mesh->set.set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferinfo,
    };
    vkUpdateDescriptorSets(device, 1, &write, 0, NULL);

    const VkDescriptorImageInfo image_desc_info = {
        .sampler = mesh->sampler.vksampler,
        .imageView = sprite_get_internal_view(mesh->spr),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    write.pImageInfo = &image_desc_info;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.dstBinding = 1;
    vkUpdateDescriptorSets(device, 1, &write, 0, NULL);

    csm_shader_t *cvertex, *fragment;
    csm_load_shader("Unlit/vert", &cvertex);
    csm_load_shader("Unlit/frag", &fragment);

    cassert(cvertex != NULL && fragment != NULL);

    const csm_shader_t *shaders[] = { cvertex, fragment };
    const VkDescriptorSetLayout layouts[] = { mesh->set.layout };

    const lunaExtent2D RenderExtent = luna_Renderer_GetRenderExtent(rd);
    luna_GPU_PipelineCreateInfo pc = luna_GPU_InitPipelineCreateInfo();
    pc.format = SwapChainImageFormat;
    pc.subpass = 0;
    pc.render_pass = luna_Renderer_GetRenderPass(rd);

    pc.nAttributeDescriptions = array_len(attributeDescriptions);
    pc.pAttributeDescriptions = attributeDescriptions;

    pc.nPushConstants = array_len(pushConstants);
    pc.pPushConstants = pushConstants;

    pc.nBindingDescriptions = array_len(bindingDescriptions);
    pc.pBindingDescriptions = bindingDescriptions;

    pc.nShaders = array_len(shaders);
    pc.pShaders = shaders;

    pc.nDescriptorLayouts = array_len(layouts);
    pc.pDescriptorLayouts = layouts;

    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.samples = Samples;
    luna_GPU_CreatePipelineLayout(&pc, &mesh->pipeline_layout);
    pc.pipeline_layout = mesh->pipeline_layout;
    luna_GPU_CreateGraphicsPipeline(&pc, &mesh->pipeline, 0);

    return mesh;
}

static void render(luna_Renderer_t *rd, ccamera *camera, cmesh_t *mesh) {
    const vec3 rotationradians = v3muls(mesh->transform.rotation, (float)cmDEG2RAD_CONSTANT);
    mat4 scaling = m4scale(m4init(1.0f), mesh->transform.scale);
    mat4 rotation = m4rotatev(m4init(1.0f), rotationradians);
    mat4 translation = m4translate(m4init(1.0f), mesh->transform.position);
    ubdata ub = {};
    ub.model = m4mul(scaling,  m4mul(rotation, translation));
    ub.view = cam_get_view(camera);
    ub.projection = cam_get_projection(camera);
    memcpy((uchar *)mesh->ubmapped + align_up(sizeof(ccube_vertices), ub_alignment), &ub, sizeof(ubdata));

    const VkDeviceSize offsets[1] = {};

    cvertex vertex_mapped[4];
    vertex_mapped[0] = (cvertex){ (vec3){+HALF_WIDTH, +HALF_WIDTH, 0.0f}, { 0.0f, 0.0f } };
    vertex_mapped[1] = (cvertex){ (vec3){-HALF_WIDTH, +HALF_WIDTH, 0.0f}, { 1.0f, 0.0f } };
    vertex_mapped[2] = (cvertex){ (vec3){-HALF_WIDTH, -HALF_WIDTH, 0.0f}, { 1.0f, 1.0f } };
    vertex_mapped[3] = (cvertex){ (vec3){+HALF_WIDTH, -HALF_WIDTH, 0.0f}, { 0.0f, 1.0f } };
    memcpy(mesh->ubmapped, vertex_mapped, sizeof(ccube_vertices));

    const VkCommandBuffer cmd = luna_Renderer_GetDrawBuffer(rd);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh->pipeline_layout, 0, 1, &mesh->set.set, 0, NULL);
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh->vb, offsets);
    vkCmdBindIndexBuffer(cmd, mesh->ib, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh->pipeline);
    vkCmdDrawIndexed(cmd, array_len(ccube_indices), 1, 0, 0, 0);
}

#endif//__C_MESH_H__