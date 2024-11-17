#ifndef __LUNA_UI_H
#define __LUNA_UI_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"
#include "lunaGFX.h"
#include "lunaPipeline.h"
#include "sprite.h"
#include "undescriptorset.h"
#include "lunaVK.h"
#include "cshadermanager.h"
#include "cgvector.h"
#include "world.h"
#include "cdevicememory.h"
#include "camera.h"

typedef struct luna_UI_Button luna_UI_Button;

typedef void (*luna_UI_button_pressed_fn)(luna_UI_Button *bton);
typedef void (*luna_UI_button_hover_fn)(luna_UI_Button *bton);

typedef struct luna_UI_Context {
    luna_VK_Pipeline pipeline;
    undescriptorset set;
    cg_vector_t btons;
    void *ubmapped;
    luna_GPU_Sampler sampler;
    VkBuffer vb;
    VkBuffer ib;
    VkDeviceMemory vbm;
    VkDeviceMemory ibm;
} luna_UI_Context;

static luna_UI_Context luna_ui_ctx;

typedef struct luna_UI_Button {
    vec2 position;
    vec2 size;
    vec4 color;
    luna_UI_button_hover_fn hover;
    luna_UI_button_pressed_fn pressed;
    bool was_hovered; // was it being hovered in this frame?
    bool was_pressed; // was the button pressed?
} luna_UI_Button;

void luna_UI_Init(luna_Renderer_t *rd, sprite_t *spr) {
    luna_ui_ctx.btons = cg_vector_init(sizeof(luna_UI_Button), 4);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physDevice, &properties);

    const int ub_alignment = properties.limits.minUniformBufferOffsetAlignment;

    const int vertex_size = align_up(sizeof(cvertex) * 4 + sizeof(ubdata), ub_alignment);
    const int indices_size = sizeof(u32) * 6;

    luna_VK_CreateBuffer(
        align_up(vertex_size, ub_alignment) + sizeof(ubdata),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &luna_ui_ctx.vb, &luna_ui_ctx.vbm, 0
    );

    luna_VK_CreateBuffer(
        indices_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &luna_ui_ctx.ib, &luna_ui_ctx.ibm, 0
    );

    cvertex *vertex_mapped;
    u32 *index_mapped;
    vkMapMemory(device, luna_ui_ctx.vbm, 0, vertex_size, 0, (void **)&vertex_mapped);
    vkMapMemory(device, luna_ui_ctx.ibm, 0, indices_size, 0, (void **)&index_mapped);

    vertex_mapped[0] = (cvertex){ (vec3){+HALF_WIDTH, +HALF_WIDTH, 0.0f}, { 0.0f, 0.0f } };
    vertex_mapped[1] = (cvertex){ (vec3){-HALF_WIDTH, +HALF_WIDTH, 0.0f}, { 1.0f, 0.0f } };
    vertex_mapped[2] = (cvertex){ (vec3){-HALF_WIDTH, -HALF_WIDTH, 0.0f}, { 1.0f, 1.0f } };
    vertex_mapped[3] = (cvertex){ (vec3){+HALF_WIDTH, -HALF_WIDTH, 0.0f}, { 0.0f, 1.0f } };

    const int vertex_offset = 0;
    index_mapped[0] = vertex_offset;
    index_mapped[1] = vertex_offset + 1;
    index_mapped[2] = vertex_offset + 2;
    index_mapped[3] = vertex_offset + 2;
    index_mapped[4] = vertex_offset + 3;
    index_mapped[5] = vertex_offset;

    vkUnmapMemory(device, luna_ui_ctx.vbm);
    vkUnmapMemory(device, luna_ui_ctx.ibm);

    vkMapMemory(device, luna_ui_ctx.vbm, align_up(vertex_size, ub_alignment), sizeof(ubdata), 0, &luna_ui_ctx.ubmapped);

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
        {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(mat4) + sizeof(vec4)}
    };
    luna_GPU_SamplerCreateInfo sampler_info = {
       .filter = VK_FILTER_LINEAR,
       .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
       .address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT 
    };
    luna_GPU_CreateSampler(&sampler_info, &luna_ui_ctx.sampler);

    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &luna_ui_ctx.sampler.vksampler },
    };
    create_undescriptor_set(&luna_ui_ctx.set, bindings, array_len(bindings));

    VkDescriptorBufferInfo bufferinfo = {
        .buffer = luna_ui_ctx.vb,
        .offset = align_up(vertex_size, ub_alignment),
        .range = sizeof(ubdata)
    };

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = luna_ui_ctx.set.set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferinfo,
    };
    vkUpdateDescriptorSets(device, 1, &write, 0, NULL);

    const VkDescriptorImageInfo image_desc_info = {
        .sampler = luna_ui_ctx.sampler.vksampler,
        .imageView = sprite_get_internal_view(spr),
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
    const VkDescriptorSetLayout layouts[] = { luna_ui_ctx.set.layout };

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
    luna_GPU_CreatePipelineLayout(&pc, &luna_ui_ctx.pipeline.layout);
    pc.pipeline_layout = luna_ui_ctx.pipeline.layout;
    luna_GPU_CreateGraphicsPipeline(&pc, &luna_ui_ctx.pipeline.pipeline, 0);
}

luna_UI_Button *luna_UI_CreateButton() {
    luna_UI_Button bton = (luna_UI_Button){};
    bton.position = (vec2){};
    bton.size = (vec2){1.0f,1.0f};
    bton.color = (vec4){ 1.0f, 1.0f, 1.0f, 1.0f };
    cg_vector_push_back(&luna_ui_ctx.btons, &bton);
    return &((luna_UI_Button *)cg_vector_data(&luna_ui_ctx.btons))[ cg_vector_size(&luna_ui_ctx.btons) - 1 ];
}

void luna_UI_Render(ccamera *camera, luna_Renderer_t *rd) {

    VkDeviceSize offsets = 0;

    const VkCommandBuffer cmd = luna_Renderer_GetDrawBuffer(rd);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, luna_ui_ctx.pipeline.layout, 0, 1, &luna_ui_ctx.set.set, 0, NULL);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, luna_ui_ctx.pipeline.pipeline);
    vkCmdBindVertexBuffers(cmd, 0, 1, &luna_ui_ctx.vb, &offsets);
    vkCmdBindIndexBuffer(cmd, luna_ui_ctx.ib, 0, VK_INDEX_TYPE_UINT32);

    ubdata ub = {};
    ub.view = cam_get_view(camera);
    ub.projection = camera->ortho;
    *((ubdata *)luna_ui_ctx.ubmapped) = ub;

    struct push_constants {
        mat4 model;
        vec4 color;
    } pc;

    for (int i = 0; i < cg_vector_size(&luna_ui_ctx.btons); i++) {
        const luna_UI_Button *bton = (luna_UI_Button *)cg_vector_get(&luna_ui_ctx.btons, i);

        mat4 scale = m4scale(m4init(1.0f), (vec3){ bton->size.x, bton->size.y, 1.0f });
        mat4 rotate = m4init(1.0f);
        mat4 translate = m4translate(m4init(1.0f), (vec3){ bton->position.x, bton->position.y, 0.0f });
        pc.model = m4mul(translate, m4mul(rotate, scale));
        pc.color = bton->color;
        vkCmdPushConstants(cmd, luna_ui_ctx.pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(struct push_constants), &pc);

        VkDeviceSize vertex_offset = 0;
        vkCmdDrawIndexed(cmd, 6, 1, 0, vertex_offset, 0);
    }
}

void luna_UI_Update(ccamera *camera) {
    const bool mouse_pressed = cinput_is_mouse_pressed(SDL_BUTTON_LEFT);
    const vec2 translated_mouse_position = v2add(
        v2mulv(cinput_get_mouse_position(), (vec2){ CAMERA_ORTHO_W, CAMERA_ORTHO_H }),
        (vec2){ camera->position.x, camera->position.y }
    );
    const vec2 ortho = (vec2){
        (1.0f / CAMERA_ORTHO_W),
        (1.0f / CAMERA_ORTHO_H)
    };
    for (int i = 0; i < cg_vector_size(&luna_ui_ctx.btons); i++) {
        luna_UI_Button *bton = (luna_UI_Button *)cg_vector_get(&luna_ui_ctx.btons, i);
        const vec2 orthosize = (vec2){bton->size.x * ortho.x, bton->size.y * ortho.y};
        const bool horizontal = translated_mouse_position.x > (bton->position.x - orthosize.x) && translated_mouse_position.x < (bton->position.x + orthosize.x);
        const bool vertical = translated_mouse_position.y > (bton->position.y - orthosize.y) && translated_mouse_position.y < (bton->position.y + orthosize.y);
        if (horizontal && vertical) {
            bton->was_hovered = 1;
            if (mouse_pressed && bton->pressed) {
                bton->was_pressed = 1;
                bton->pressed(bton);
            } else if (bton->hover) {
                bton->was_pressed = 0;
                bton->hover(bton);
            }
        } else {
            bton->was_hovered = 0;
            bton->was_pressed = 0;
        }
    }
}

#ifdef __cplusplus
    }
#endif

#endif//__LUNA_UI_H