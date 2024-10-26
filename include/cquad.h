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

    VkBuffer ub;
    VkDeviceMemory ubm;
    void *ubmapped;

    cgfx_gpu_sampler_t sampler;
    cgfx_gpu_image_t texture;
    cgfx_gpu_memory_t texture_memory;

    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    VkDescriptorPool descpool;
    VkDescriptorSet set;
    VkDescriptorSetLayout setlayout;

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

cmesh_t *load_mesh(const char *texpath, body_parameters *body, crenderer_t *rd) {
    if (texpath == NULL) {
        // texpath = "../Assets/empty.png";
        texpath = "../Assets/barrel.png";
    }
    // implement what??? I forgot lmao
    cmesh_t *mesh = (cmesh_t *)malloc(sizeof(cmesh_t));
    mesh->index_count = array_len(ccube_indices);

    mesh->transform.position = (vec3){ body->position.x, body->position.y, 0.0f };
    mesh->transform.rotation = (vec3){};
    mesh->transform.scale = (vec3){ body->scale.x, body->scale.y, 0.0f };

    const int cvertexsize = sizeof(ccube_vertices);
    const int indexsize = sizeof(ccube_indices);

    vkh_buffer_create(
        cvertexsize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &mesh->vb, &mesh->vbm, 0
    );

    vkh_buffer_create(
        indexsize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &mesh->ib, &mesh->ibm, 0
    );

    vkh_buffer_create(
        sizeof(ubdata),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &mesh->ub, &mesh->ubm, 0
    );

    vkMapMemory(device, mesh->ubm, 0, sizeof(ubdata), 0, &mesh->ubmapped);

    void *mapped;
    vkMapMemory(device, mesh->vbm, 0, cvertexsize, 0, &mapped);
    memcpy(mapped, ccube_vertices, cvertexsize);
    vkUnmapMemory(device, mesh->vbm);

    vkMapMemory(device, mesh->ibm, 0, indexsize, 0, &mapped);
    memcpy(mapped, ccube_indices, indexsize);
    vkUnmapMemory(device, mesh->ibm);

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

    u32 w, h;
    VkFormat channels;

    // ! FIXME: what.
    // ! maybe write a wrapper that supports cgfx_gpu_image or whatever.
    vkh_image_from_disk(texpath, &w, &h, &channels, &mesh->texture.image, &mesh->texture_memory.memory);

    cgfx_gpu_image_view_create_info view_info = {
        .format = channels,
        .view_type = VK_IMAGE_VIEW_TYPE_2D,
        .subresourceRange = (VkImageSubresourceRange) {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    cgfx_gpu_create_image_view(&view_info, &mesh->texture);

    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &mesh->sampler.vksampler },
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

    poolInfo.pPoolSizes = (VkDescriptorPoolSize[]) {
        // type; descriptorCount;
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
    };
    poolInfo.poolSizeCount = 2;

    poolInfo.maxSets = 1;
    if (vkCreateDescriptorPool(device, &poolInfo, NULL, &mesh->descpool) != VK_SUCCESS)
        LOG_ERROR("Failed to create descriptor pool");

    VkDescriptorSetLayoutCreateInfo layoutinfo = {};
    layoutinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutinfo.pBindings = bindings;
    layoutinfo.bindingCount = array_len(bindings);
    vkCreateDescriptorSetLayout(device, &layoutinfo, NULL, &mesh->setlayout);
    
    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = mesh->descpool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &mesh->setlayout;
    if (vkAllocateDescriptorSets(device, &setAllocInfo, &mesh->set) != VK_SUCCESS)
        LOG_ERROR("Failed to allocate descriptor sets");

    VkDescriptorBufferInfo bufferinfo = {
        .buffer = mesh->ub,
        .offset = 0,
        .range = sizeof(ubdata)
    };

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mesh->set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferinfo,
    };
    vkUpdateDescriptorSets(device, 1, &write, 0, NULL);

    const VkDescriptorImageInfo image_desc_info = {
        .sampler = mesh->sampler.vksampler,
        .imageView = mesh->texture.view,
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
    const VkDescriptorSetLayout layouts[] = { mesh->setlayout };

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
    ub.projection = cam_get_projection(camera);
    ub.view = cam_get_view(camera);
    memcpy(mesh->ubmapped, &ub, sizeof(ubdata));

    const VkDeviceSize offsets[1] = {};

    const VkCommandBuffer cmd = crd_get_drawbuffer(rd);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh->pipeline_layout, 0, 1, &mesh->set, 0, NULL);
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh->vb, offsets);
    vkCmdBindIndexBuffer(cmd, mesh->ib, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh->pipeline);
    vkCmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, 0);
}

#endif//__C_MESH_H__