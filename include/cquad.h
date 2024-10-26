#ifndef __C_MESH_H__
#define __C_MESH_H__

#include <stdio.h>
#include "defines.h"

#include "cvk.h"
#include "vkhelper.h"
#include "cshadermanager.h"

#include "math/vec3.h"
#include "math/vec2.h"
#include "math/mat.h"

#include "cimage.h"
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

    void *ubmapped;

    cgfx_gpu_sampler_t sampler;
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

typedef struct body_parameters {
    b2WorldId world;
    vec2 position;
    vec2 scale;
    b2BodyType type;
    f32 density;
    f32 friction;
    f32 restitution;
} body_parameters;

int align_up(int size, int alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

cmesh_t *load_mesh(sprite_t *spr, body_parameters *body, crenderer_t *rd) {
    cmesh_t *mesh = (cmesh_t *)malloc(sizeof(cmesh_t));
    if (spr == NULL) {
        mesh->spr = sprite_empty;
    } else {
        mesh->spr = spr;
    }
    mesh->index_count = array_len(ccube_indices);

    mesh->transform.position = (vec3){ body->position.x, body->position.y, 0.0f };
    mesh->transform.rotation = (vec3){};
    mesh->transform.scale = (vec3){ body->scale.x, body->scale.y, 0.0f };

    const int cvertexsize = sizeof(ccube_vertices);
    const int indexsize = sizeof(ccube_indices);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physDevice, &properties);

    const int ub_alignment = properties.limits.minUniformBufferOffsetAlignment;

    vkh_buffer_create(
        align_up(cvertexsize + indexsize, ub_alignment) + sizeof(ubdata),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &mesh->vb, &mesh->vbm, 0
    );

    void *mapped;
    vkMapMemory(device, mesh->vbm, 0, cvertexsize + indexsize, 0, &mapped);
    memcpy(mapped, ccube_vertices, cvertexsize);
    memcpy(mapped + cvertexsize, ccube_indices, indexsize);
    vkUnmapMemory(device, mesh->vbm);

    vkMapMemory(device, mesh->vbm, align_up(cvertexsize + indexsize, ub_alignment), sizeof(ubdata), 0, &mesh->ubmapped);

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
    cgfx_gpu_sampler_create_info sampler_info = {
       .filter = VK_FILTER_LINEAR,
       .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
       .address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT 
    };
    cgfx_gpu_create_sampler(&sampler_info, &mesh->sampler);

    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &mesh->sampler.vksampler },
    };
    create_undescriptor_set(&mesh->set, bindings, array_len(bindings));

    VkDescriptorBufferInfo bufferinfo = {
        .buffer = mesh->vb,
        .offset = align_up(cvertexsize + indexsize, ub_alignment),
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

    const cg_extent2d RenderExtent = crd_get_render_extent(rd);
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

    pc.nDescriptorLayouts = array_len(layouts);
    pc.pDescriptorLayouts = layouts;

    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.samples = Samples;
    cvk_create_pipeline_layout(&pc, &mesh->pipeline_layout);
    pc.pipeline_layout = mesh->pipeline_layout;
    cvk_create_graphics_pipeline(&pc, &mesh->pipeline, 0);

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = body->type;
    bodyDef.position = (b2Vec2){body->position.x, body->position.y};
    bodyDef.fixedRotation = 1;
    b2BodyId body_id = b2CreateBody(body->world, &bodyDef);

    b2Polygon dynamicBox = b2MakeBox(body->scale.x, body->scale.y);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = body->density;
    shapeDef.friction = body->friction;
    shapeDef.restitution = body->restitution;
    b2CreatePolygonShape(body_id, &shapeDef, &dynamicBox);

    mesh->body = body_id;

    b2Vec2 temp = b2Body_GetPosition(body_id);
    mesh->transform.position = (vec3){temp.x, temp.y, 0.0f};

    return mesh;
}

static void render(crenderer_t *rd, ccamera *camera, cmesh_t *mesh) {
    const vec3 rotationradians = v3muls(mesh->transform.rotation, (float)cmDEG2RAD_CONSTANT);
    mat4 scaling = m4scale(m4init(1.0f), mesh->transform.scale);
    mat4 rotation = m4rotatev(m4init(1.0f), rotationradians);
    mat4 translation = m4translate(m4init(1.0f), mesh->transform.position);
    ubdata ub = {};
    ub.model = m4mul(scaling,  m4mul(rotation, translation));
    ub.view = cam_get_view(camera);
    ub.projection = cam_get_projection(camera);
    memcpy(mesh->ubmapped, &ub, sizeof(ubdata));

    const VkDeviceSize offsets[1] = {};

    const VkCommandBuffer cmd = crd_get_drawbuffer(rd);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh->pipeline_layout, 0, 1, &mesh->set.set, 0, NULL);
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh->vb, offsets);
    vkCmdBindIndexBuffer(cmd, mesh->vb, sizeof(ccube_vertices), VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh->pipeline);
    vkCmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, 0);
}

#endif//__C_MESH_H__