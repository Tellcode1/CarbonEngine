#ifndef __C_MESH_HPP__
#define __C_MESH_HPP__

#include <stdio.h>
#include "defines.h"

#include "cvk.h"
#include "vkhelper.h"
#include "cshadermanager.h"

#include "math/vec3.h"
#include "math/vec2.h"

#include "cimage.h"

struct Transform {
    vec3 position = (vec3){0.0f,0.0f,0.0f};
    vec3 scale = (vec3){20.0f,20.0f,20.0f};
    vec3 rotation = (vec3){0.0f,0.0f,0.0f};
};

struct ubdata {
    alignas(sizeof(vec4)) mat4 model;
    alignas(sizeof(vec4)) mat4 view;
    alignas(sizeof(vec4)) mat4 projection;
};

struct cmesh_t {
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
};

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

struct cmesh_t *load_mesh(crenderer_t *rd) {
    // ! FIXME: implement yadayada
    cmesh_t *mesh = (cmesh_t *)malloc(sizeof(cmesh_t));
    mesh->index_count = array_len(ccube_indices);

    mesh->transform.position = (vec3){};
    mesh->transform.rotation = (vec3){};
    mesh->transform.scale = (vec3){ 1.0f, 1.0f, 0.0f };

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
    cgfx_gpu_image_create_info image_info = {};

    // view_info.image = mesh->normalmap;
    // view_info.format = cfmt_conv_cfmt_to_vkfmt(normal.fmt);
    // if (vkCreateImageView(device, &view_info, nullptr, &mesh->normalmapview) != VK_SUCCESS)
        // LOG_ERROR("Failed to create image view");

    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
    };

    const VkDescriptorPoolSize poolSizes[] = {
        // type; descriptorCount;
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.poolSizeCount = array_len(poolSizes);
    poolInfo.maxSets = 1;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &mesh->descpool) != VK_SUCCESS)
        LOG_ERROR("Failed to create descriptor pool");

    VkDescriptorSetLayoutCreateInfo layoutinfo{};
    layoutinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutinfo.pBindings = bindings;
    layoutinfo.bindingCount = array_len(bindings);
    vkCreateDescriptorSetLayout(device, &layoutinfo, nullptr, &mesh->setlayout);
    
    VkDescriptorSetAllocateInfo setAllocInfo{};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = mesh->descpool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &mesh->setlayout;
    if (vkAllocateDescriptorSets(device, &setAllocInfo, &mesh->set) != VK_SUCCESS)
        LOG_ERROR("Failed to allocate descriptor sets");

    VkDescriptorBufferInfo bufferinfo{};
    bufferinfo.buffer = mesh->ub;
    bufferinfo.offset = 0;
    bufferinfo.range = sizeof(ubdata);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &bufferinfo;
    write.dstSet = mesh->set;
    write.descriptorCount = 1;
    write.dstBinding = 0;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

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

    return mesh;
}

static void render(crenderer_t *rd, ccamera *camera, cmesh_t *mesh) {
    const vec3 rotationradians = v3muls(mesh->transform.rotation, (float)cmDEG2RAD_CONSTANT);
    mat4 scaling = m4scale(m4init(1.0f), mesh->transform.scale);
    mat4 rotation = m4rotatev(m4init(1.0f), rotationradians);
    mat4 translation = m4translate(m4init(1.0f), mesh->transform.position);
    ubdata ub{};
    ub.model = m4mul(scaling,  m4mul(rotation, translation));
    ub.projection = cam_get_projection(camera);
    ub.view = cam_get_view(camera);
    memcpy(mesh->ubmapped, &ub, sizeof(ubdata));

    const VkDeviceSize offsets[1] = {};

    const VkCommandBuffer cmd = crd_get_drawbuffer(rd);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh->pipeline_layout, 0, 1, &mesh->set, 0, nullptr);
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh->vb, offsets);
    vkCmdBindIndexBuffer(cmd, mesh->ib, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh->pipeline);
    vkCmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, 0);
}

#endif//__C_MESH_HPP__