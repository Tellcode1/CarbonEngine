#ifndef __C_GAMEOBJECT_HPP__
#define __C_GAMEOBJECT_HPP__

#include "../../stdafx.hpp"

struct cgameobject_t;
struct csprite_t;

struct VkBuffer_T;
struct VkDeviceMemory_T;

typedef VkBuffer_T *VkBuffer;
typedef VkDeviceMemory_T *VkDeviceMemory;

struct ctransform {
    vec2 position;
    float rotation;
};

struct cpipeline_t {
    VkPipeline pipeline;
};

struct cgameobject_t {
    const char *name;
    ctransform transform;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;

    cpipeline_t *pipeline;

    vec2 *vertex_data;
    int vertex_count;
};

struct csquare_t : cgameobject_t{
    constexpr static float vertices[] = {
        -0.5f,  0.5f, // 0
        -0.5f, -0.5f, // 1
         0.5f, -0.5f, // 2
         0.5f, -0.5f, // 2
         0.5f,  0.5f  // 3
        -0.5f,  0.5f, // 0
    };
};

extern csquare_t *ccreate_square();
extern cpipeline_t *ccreate_pipeline();
extern void crender_square(csquare_t *shape, VkCommandBuffer buf);

#endif//__C_GAMEOBJECT_HPP__