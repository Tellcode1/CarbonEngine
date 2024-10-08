#ifndef __C_MESH_HPP__
#define __C_MESH_HPP__

// TODO: move this + stb implementation to some stdafx.cpp or something it looks ugly man
#define TINYOBJLOADER_IMPLEMENTATION

#include <iostream>
#include "defines.h"

#include "cvk.h"
#include "vkhelper.h"
#include "cshadermanager.h"

#include "math/vec3.hpp"
#include "math/vec2.hpp"

#include "../external/tinyobjloader/tiny_obj_loader.h"

#include "cimage.h"

struct Transform {
    cm::vec3 position = cm::vec3(0.0f);
    cm::vec3 scale = cm::vec3(20.0f);
    cm::vec3 rotation = cm::vec3(0.0f);
};

struct Material {
    alignas(sizeof(cm::vec4)) cm::vec3 ambient;
    alignas(sizeof(cm::vec4)) cm::vec3 diffuse;
    alignas(sizeof(cm::vec4)) cm::vec3 specular;
    alignas(sizeof(float)) float shininess;
};

struct ubdata {
    alignas(sizeof(cm::vec4)) cm::mat4 model;
    alignas(sizeof(cm::vec4)) cm::mat4 view;
    alignas(sizeof(cm::vec4)) cm::mat4 projection;
    alignas(sizeof(cm::vec4)) cm::vec3 cameraposition;
    alignas(sizeof(cm::vec4)) cm::vec3 lightposition;
    alignas(sizeof(cm::vec4)) cm::vec3 lightcolor;
    alignas(sizeof(cm::vec4)) cm::vec3 color;
    struct Material mat;
};

struct cmesh_t {
    Transform transform = {};
    cmesh_t() : transform(Transform()) {}

    VkBuffer vb;
    VkDeviceMemory vbm;

    VkBuffer ib;
    VkDeviceMemory ibm;

    VkBuffer ub;
    VkDeviceMemory ubm;
    void *ubmapped;

    VkSampler sampler;
    VkImage texture;
    VkImage normalmap;
    VkImageView texture_view;
    VkImageView normalmapview;
    VkDeviceMemory texture_memory;
    VkDeviceMemory normalmap_memory;

    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    VkDescriptorPool descpool;
    VkDescriptorSet set;
    VkDescriptorSetLayout setlayout;

    int index_count;
};

struct vertex {
    cm::vec3 position;
    cm::vec3 normal;
    cm::vec3 tangent;
    cm::vec3 bitangent;
    cm::vec2 texcoord;

    bool operator==(const vertex& other) const {
        return position == other.position && texcoord == other.texcoord && normal == other.normal;
    }
};

struct soosydata {
    cvector_t * /* vertex */ vertices;
    cvector_t * /* u32 */    indices;
};

static bool_t eq_vertex(const void *key1, const void *key2, unsigned long) {
    const vertex *v1 = (vertex *)key1;
    const vertex *v2 = (vertex *)key2;
    return v1->position == v2->position && v1->texcoord == v2->texcoord && v1->normal == v2->normal;
};

static u32 hash_vertex(const void *in, int nbytes) {
    const vertex *v = (vertex *)in;
    u32 h = chashmap_std_hash(&v->position, sizeof(cm::vec3));
    h ^= (chashmap_std_hash(&v->normal, sizeof(cm::vec3)) + 0x9e3779b9 + (h << 6) + (h >> 2));
    h ^= (chashmap_std_hash(&v->tangent, sizeof(cm::vec3)) + 0x9e3779b9 + (h << 6) + (h >> 2));
    h ^= (chashmap_std_hash(&v->texcoord, sizeof(cm::vec2)) + 0x9e3779b9 + (h << 6) + (h >> 2));
    return h;
}

static soosydata loadmdl(const char *mdlpath, bool to_retrieve_normals) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    soosydata ret{};
    ret.vertices = cvector_init(sizeof(vertex), 0);
    ret.indices = cvector_init(sizeof(u32), 0);

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, mdlpath))
        LOG_ERROR((warn + err).c_str());

    int v_count = 0;
    for (const auto &shape : shapes) {
        v_count += shape.mesh.indices.size();
    }
    chashmap_t *unique_vertices = chashmap_init(v_count, sizeof(vertex), sizeof(unsigned), hash_vertex, eq_vertex);

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            vertex vertex;

            vertex.position = (cm::vec3){
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (to_retrieve_normals) {
                vertex.normal = (cm::vec3){
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }

            vertex.texcoord = (cm::vec2){
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            u32 *exists = (u32 *)chashmap_find(unique_vertices, &vertex);

            if (!exists) {
                unsigned idx = cvector_size(ret.vertices);
                chashmap_insert(unique_vertices, &vertex, &idx);
                cvector_push_back(ret.indices, &idx);
                cvector_push_back(ret.vertices, &vertex);
            } else {
                cvector_push_back(ret.indices, exists);
            }
        }
    }

    // calcula tangents
    // you can look at how it's claculated at https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    for (int i = 0; i < cvector_size(ret.indices); i += 3) {
        const u32 *indices = (u32 *)cvector_data(ret.indices);
        vertex *vertices = (vertex *)cvector_data(ret.vertices);

        u32 i0 = indices[i];
        u32 i1 = indices[i + 1];
        u32 i2 = indices[i + 2];

        const cm::vec3 &pos1 = vertices[i0].position;
        const cm::vec3 &pos2 = vertices[i1].position;
        const cm::vec3 &pos3 = vertices[i2].position;

        const cm::vec2 &uv1 = vertices[i0].texcoord;
        const cm::vec2 &uv2 = vertices[i1].texcoord;
        const cm::vec2 &uv3 = vertices[i2].texcoord;

        cm::vec3 edge1 = pos2 - pos1;
        cm::vec3 edge2 = pos3 - pos1;
        cm::vec2 deltaUV1 = uv2 - uv1;
        cm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        cm::vec3 tangent = (cm::vec3){
            f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
            f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
            f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z)
        };
        vertices[i0].tangent += tangent;
        vertices[i1].tangent += tangent;
        vertices[i2].tangent += tangent;

        cm::vec3 bitangent = (cm::vec3){
            f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x),
            f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y),
            f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z)
        };
        vertices[i0].bitangent = bitangent;
        vertices[i1].bitangent = bitangent;
        vertices[i2].bitangent = bitangent;
    }

    chashmap_destroy(unique_vertices);

    return ret;
}

struct cmesh_t *load_mesh(crenderer_t *rd, const char *mdlpath, const char *texpath, const char *normalmappath) {
    soosydata data = loadmdl(mdlpath, (true));

    cmesh_t *mesh = (cmesh_t *)malloc(sizeof(cmesh_t));
    mesh->index_count = cvector_size(data.indices);

    mesh->transform = Transform();

    const int vertexsize = sizeof(vertex) * cvector_size(data.vertices);
    const int indexsize = sizeof(u32) * cvector_size(data.indices);

    vkh_buffer_create(
        vertexsize,
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
    vkMapMemory(device, mesh->vbm, 0, vertexsize, 0, &mapped);
    memcpy(mapped, cvector_data(data.vertices), vertexsize);
    vkUnmapMemory(device, mesh->vbm);

    vkMapMemory(device, mesh->ibm, 0, indexsize, 0, &mapped);
    memcpy(mapped, cvector_data(data.indices), indexsize);
    vkUnmapMemory(device, mesh->ibm);

    const VkVertexInputAttributeDescription attributeDescriptions[] = {
        // location; binding; format; offset;
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // pos
        { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(cm::vec3) }, // normal
        { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, 2 * sizeof(cm::vec3) }, // tangent
        { 3, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(cm::vec3) }, // bitangent
        { 4, 0, VK_FORMAT_R32G32_SFLOAT,    4 * sizeof(cm::vec3) }, // uv
    };

    const VkVertexInputBindingDescription bindingDescriptions[] = {
        // binding; stride; inputRate
        { 0, sizeof(vertex), VK_VERTEX_INPUT_RATE_VERTEX }
    };

    const VkPushConstantRange pushConstants[] = { 
        // stageFlags, offset, size
        {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(cm::vec3)}
    };

    ctex2D texture = cimg_load( texpath );
    vkh_image_from_mem(texture.data, texture.w, texture.h, cfmt_conv_cfmt_to_vkfmt(texture.fmt), cfmt_get_bytesperpixel(texture.fmt), &mesh->texture, &mesh->texture_memory);

    ctex2D normal = cimg_load( normalmappath );
    vkh_image_from_mem(normal.data, normal.w, normal.h, cfmt_conv_cfmt_to_vkfmt(normal.fmt), cfmt_get_bytesperpixel(normal.fmt), &mesh->normalmap, &mesh->normalmap_memory);

    free(texture.data);
    free(normal.data);

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    if (vkCreateSampler(device, &sampler_info, nullptr, &mesh->sampler) != VK_SUCCESS)
        LOG_ERROR("Failed to create sampler");

    VkImageViewCreateInfo view_info{};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = cfmt_conv_cfmt_to_vkfmt(texture.fmt);
    view_info.image = mesh->texture;
    view_info.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &view_info, nullptr, &mesh->texture_view) != VK_SUCCESS)
        LOG_ERROR("Failed to create image view");

    view_info.image = mesh->normalmap;
    view_info.format = cfmt_conv_cfmt_to_vkfmt(normal.fmt);
    if (vkCreateImageView(device, &view_info, nullptr, &mesh->normalmapview) != VK_SUCCESS)
        LOG_ERROR("Failed to create image view");

    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
        { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &mesh->sampler },
        { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, &mesh->sampler },
    };

    const VkDescriptorPoolSize poolSizes[] = {
        // type; descriptorCount;
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 },
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

    VkDescriptorImageInfo imageinfo{};
    imageinfo.sampler = mesh->sampler;
    imageinfo.imageView = mesh->texture_view;
    imageinfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageinfo;
    write.dstSet = mesh->set;
    write.descriptorCount = 1;
    write.dstBinding = 1;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    imageinfo.sampler = mesh->sampler;
    imageinfo.imageView = mesh->normalmapview;
    imageinfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    write.pImageInfo = &imageinfo;
    write.dstBinding = 2;
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

    csm_shader_t *vertex, *fragment;
    csm_load_shader("test/vert", &vertex);
    csm_load_shader("test/frag", &fragment);

    cassert(vertex != NULL && fragment != NULL);

    const csm_shader_t *shaders[] = { vertex, fragment };
    const VkDescriptorSetLayout layouts[] = { mesh->setlayout };

    const cengine_extent2d RenderExtent = crenderer_get_render_extent(rd);
    cvk_pipeline_create_info pc = cvk_init_pipeline_create_info();
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

    pc.nDescriptorLayouts = array_len(layouts);
    pc.pDescriptorLayouts = layouts;

    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.samples = Samples;
    cvk_create_pipeline_layout(device, &pc, &mesh->pipeline_layout);
    pc.pipeline_layout = mesh->pipeline_layout;
    cvk_create_graphics_pipeline(device, &pc, &mesh->pipeline, 0);

    return mesh;
}

static void render(crenderer_t *rd, ccamera camera, cmesh_t *mesh, cm::vec3 lightposition) {
    const cm::vec3 rotationradians = mesh->transform.rotation * (float)cmDEG2RAD_CONSTANT;
    cm::mat4 scaling = scale(cm::mat4(1.0f), mesh->transform.scale);
    cm::mat4 rotation = rotate(cm::mat4(1.0f), rotationradians);
    cm::mat4 translation = translate(cm::mat4(1.0f), mesh->transform.position);
    ubdata ub{};
    ub.model = scaling * rotation  * translation;
    ub.projection = camera.get_projection();
    ub.view = camera.get_view();
    ub.cameraposition = camera.position;
    ub.lightposition = lightposition;
    ub.color = cm::vec3(1.0f, 1.0f, 1.0f);
    ub.lightcolor = cm::vec3(1.0f, 1.0f, 1.0f);
    memcpy(mesh->ubmapped, &ub, sizeof(ubdata));

    const VkDeviceSize offsets[1] = {};

    const VkCommandBuffer cmd = crenderer_get_drawbuffer(rd);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh->pipeline_layout, 0, 1, &mesh->set, 0, nullptr);
    vkCmdPushConstants(cmd, mesh->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(cm::vec3), &lightposition);
    vkCmdBindVertexBuffers(cmd, 0, 1, &mesh->vb, offsets);
    vkCmdBindIndexBuffer(cmd, mesh->ib, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mesh->pipeline);
    vkCmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, 0);
    // vkCmdDraw(cmd, 36, 1, 0, 0);
}

#endif//__C_MESH_HPP__