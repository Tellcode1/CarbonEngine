#ifndef __WORLD_H
#define __WORLD_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "sprite.h"

#define WORLD_W 1
#define WORLD_H 1

typedef struct block_t {
    int x,y; // The position of the block in the grid
    b2BodyId body;
    sprite_t *spr;
    bool to_render;
} block_t;

typedef struct world_t {
    luna_Renderer_t *rd;

    VkBuffer vb;
    VkDeviceMemory vbm;
    void *ubmapped;

    VkBuffer ib;
    VkDeviceMemory ibm;

    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    luna_GPU_Sampler sampler;
    undescriptorset set;
    block_t grid[ WORLD_H ][ WORLD_W ];
} world_t;

void world_init(luna_Renderer_t *rd, sprite_t *spr, b2WorldId *b2_world_id, world_t **worldptr) {
    *worldptr = calloc(1, sizeof(world_t));
    world_t *world = *worldptr;
    world->rd = rd;

    const int vertex_size = sizeof(cvertex) * 4 * WORLD_W * WORLD_H;
    const int indices_size = sizeof(uint32_t) * 6 * WORLD_W * WORLD_H;

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physDevice, &properties);

    const int ub_alignment = properties.limits.minUniformBufferOffsetAlignment;

    luna_VK_CreateBuffer(
        align_up(vertex_size, ub_alignment) + sizeof(ubdata),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &world->vb, &world->vbm, 0
    );

    luna_VK_CreateBuffer(
        indices_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &world->ib, &world->ibm, 0
    );

    int iter = 0;
    bool gaming = 1;

    cvertex *vertex_mapped;
    u32 *index_mapped;
    vkMapMemory(device, world->vbm, 0, vertex_size, 0, (void **)&vertex_mapped);
    vkMapMemory(device, world->ibm, 0, indices_size, 0, (void **)&index_mapped);

    for (int j = 0; j < WORLD_H; j++) {
        for (int i = 0; i < WORLD_W; i++) {
            float pootis = (float)j - 2.0f;
            float dispenser = (float)i;

            static const float HALF_WIDTH = 5.0f;
            vertex_mapped[0] = (cvertex){ (vec3){pootis + HALF_WIDTH, dispenser + HALF_WIDTH, 0.0f}, { 0.0f, 0.0f } };
            vertex_mapped[1] = (cvertex){ (vec3){pootis - HALF_WIDTH, dispenser + HALF_WIDTH, 0.0f}, { 1.0f, 0.0f } };
            vertex_mapped[2] = (cvertex){ (vec3){pootis - HALF_WIDTH, dispenser - HALF_WIDTH, 0.0f}, { 1.0f, 1.0f } };
            vertex_mapped[3] = (cvertex){ (vec3){pootis + HALF_WIDTH, dispenser - HALF_WIDTH, 0.0f}, { 0.0f, 1.0f } };

            index_mapped[0] = iter;
            index_mapped[1] = iter + 1;
            index_mapped[2] = iter + 2;
            index_mapped[3] = iter + 2;
            index_mapped[4] = iter + 3;
            index_mapped[5] = iter;

            index_mapped += 6;
            vertex_mapped += 4;
            iter += 4;

            world->grid[j][i].to_render = gaming;

            block_t *block = &world->grid[j][i];
            
            b2BodyDef bodyDef = b2DefaultBodyDef();
            bodyDef.type = b2_staticBody;
            bodyDef.position = (b2Vec2){pootis, dispenser};
            bodyDef.fixedRotation = 1;
            block->body = b2CreateBody(*b2_world_id, &bodyDef);

            b2Polygon dynamicBox = b2MakeSquare(HALF_WIDTH);

            b2ShapeDef shapeDef = b2DefaultShapeDef();
            shapeDef.density = 1.0f;
            shapeDef.friction = 0.0f;
            shapeDef.restitution = 0.0f;
            b2CreatePolygonShape(block->body, &shapeDef, &dynamicBox);

            block->spr = spr;
        }
    }

    vkUnmapMemory(device, world->vbm);
    vkUnmapMemory(device, world->ibm);

    vkMapMemory(device, world->vbm, align_up(vertex_size, ub_alignment), sizeof(ubdata), 0, &world->ubmapped);

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
    luna_GPU_CreateSampler(&sampler_info, &world->sampler);

    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &world->sampler.vksampler },
    };
    create_undescriptor_set(&world->set, bindings, array_len(bindings));

    VkDescriptorBufferInfo bufferinfo = {
        .buffer = world->vb,
        .offset = align_up(vertex_size, ub_alignment),
        .range = sizeof(ubdata)
    };

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = world->set.set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferinfo,
    };
    vkUpdateDescriptorSets(device, 1, &write, 0, NULL);

    const VkDescriptorImageInfo image_desc_info = {
        .sampler = world->sampler.vksampler,
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
    const VkDescriptorSetLayout layouts[] = { world->set.layout };

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
    luna_GPU_CreatePipelineLayout(&pc, &world->pipeline_layout);
    pc.pipeline_layout = world->pipeline_layout;
    luna_GPU_CreateGraphicsPipeline(&pc, &world->pipeline, 0);
}

void world_render(ccamera *camera, world_t *world) {
    ubdata ub = {};
    ub.model = m4init(1.0f);
    ub.view = cam_get_view(camera);
    ub.projection = camera->projection;
    *((ubdata *)world->ubmapped) = ub;

    VkDeviceSize offsets = 0;

    const VkCommandBuffer cmd = luna_Renderer_GetDrawBuffer(world->rd);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, world->pipeline_layout, 0, 1, &world->set.set, 0, NULL);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, world->pipeline);
    vkCmdBindVertexBuffers(cmd, 0, 1, &world->vb, &offsets);

    VkDeviceSize vertex_offset = 0;
    VkDeviceSize index_offset = 0;
    vkCmdBindIndexBuffer(cmd, world->ib, index_offset, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(cmd, WORLD_W * WORLD_H * 6, 1, 0, vertex_offset, 0);

    // for (int j = 0; j < WORLD_H; j++) {
    //     for (int i = 0; i < WORLD_W; i++) {
    //         if (!world->grid[j][i].to_render) {
    //             vertex_offset += sizeof(cvertex) * 4;
    //             index_offset += sizeof(uint32_t) * 6;
    //             continue;
    //         }
            

    //         vertex_offset += sizeof(cvertex) * 4;
    //         index_offset += sizeof(uint32_t) * 6;
    //     }
    // }
}

void world_update(ccamera *camera, world_t *world) {
    int mx, my;
    if (SDL_BUTTON(SDL_GetMouseState(&mx, &my)) == SDL_BUTTON_LEFT) {
        (void)camera;
        (void)world;
    }
}

#ifdef __cplusplus
    }
#endif

#endif//__WORLD_H