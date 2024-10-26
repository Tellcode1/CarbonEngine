#ifndef __WORLD_H
#define __WORLD_H

#include "cquad.h"

#define WORLD_W 40
#define WORLD_H 30

typedef struct block_t {
    int x,y; // The position of the block in the grid
    cmesh_t *mesh;
} block_t;

typedef struct world_t {
    VkBuffer vb;
    VkDeviceMemory vbm;
    void *ubmapped;

    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    VkSampler sampler;
    undescriptorset set;
    block_t grid[ WORLD_W * WORLD_H ];
} world_t;

#endif//__WORLD_H