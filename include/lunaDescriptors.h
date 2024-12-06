#ifndef __UNDESCRIPTOR_SET_H
#define __UNDESCRIPTOR_SET_H

#include "vkstdafx.h"
#include "math/math.h"

// WARNING: Currently only supports the first 11 descriptor types.

#define PISS_CANARY 293012

typedef struct luna_DescriptorPoolSize {
    VkDescriptorType type;
    int capacity;
    int using; // how many are being used
} luna_DescriptorPoolSize;

typedef struct luna_DescriptorPool {
    int canary;
    VkDescriptorPool pool;
    int child_sets, max_child_sets;
    luna_DescriptorPoolSize descriptors[ 11 ];
} luna_DescriptorPool;

typedef struct luna_DescriptorSet {
    VkDescriptorSetLayout layout;
    VkDescriptorSet set;
    const luna_DescriptorPool *pool;
} luna_DescriptorSet;

static inline void __luna_DescriptorPool_Allocate(luna_DescriptorPool *pool) {
    VkDescriptorPoolSize allocations[ 11 ] = {};
    int descriptors_written = 0;
    for (int i = 0; i < 11; i++) {
        if (pool->descriptors[i].capacity == 0) {
            continue;
        }
        allocations[descriptors_written] = (VkDescriptorPoolSize){ pool->descriptors[i].type, pool->descriptors[i].capacity };
        descriptors_written++;
    }
    bool need_more_max_sets = (pool->child_sets + 1) > pool->max_child_sets;
    // if (descriptors_written == 0 && !need_more_max_sets) {
    //     return;
    // }
    if (need_more_max_sets) {
        pool->max_child_sets = cmmax(pool->max_child_sets * 2, 1);
    }
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = pool->max_child_sets,
        .poolSizeCount = descriptors_written,
        .pPoolSizes = allocations
    };
    cassert(vkCreateDescriptorPool(device, &poolInfo, NULL, &pool->pool) == VK_SUCCESS);
}

static inline void luna_DescriptorPool_Init(luna_DescriptorPool *dst) {
    *dst = (luna_DescriptorPool){};
    luna_DescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 0, 0 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 0 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0, 0 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, 0 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 0, 0 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 0, 0 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 0 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, 0 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0, 0 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 0, 0 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0, 0 },
    };
    memcpy(dst->descriptors, pool_sizes, sizeof(pool_sizes));
    dst->max_child_sets = 1;
    dst->child_sets = 0;
    dst->canary = PISS_CANARY;
    __luna_DescriptorPool_Allocate(dst);
    cassert(dst->canary == PISS_CANARY);
}

static inline void luna_AllocateDescriptorSet(luna_DescriptorPool *pool, const VkDescriptorSetLayoutBinding *bindings, int nbindings, luna_DescriptorSet *dst) {
    cassert(pool->canary == PISS_CANARY);
    bool need_realloc = 0;
    for (int i = 0; i < 11; i++) {
        for (int j = 0; j < nbindings; j++) {
            luna_DescriptorPoolSize *descriptor = &pool->descriptors[i];
            if (descriptor->type == bindings[j].descriptorType) {
                descriptor->capacity = 100;
                need_realloc = 1;
            }
        }
    }

    if (need_realloc || ((pool->child_sets + 1) > pool->max_child_sets)) {
        if ((pool->child_sets + 1) > pool->max_child_sets) {
            pool->max_child_sets = cmmax(pool->max_child_sets * 2, 1);
        }

        __luna_DescriptorPool_Allocate(pool);
    }

    cassert(pool->canary == PISS_CANARY);

    VkDescriptorSetLayoutCreateInfo layoutinfo = {};
    layoutinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutinfo.pBindings = bindings;
    layoutinfo.bindingCount = nbindings;
    cassert(vkCreateDescriptorSetLayout(device, &layoutinfo, NULL, &dst->layout) == VK_SUCCESS);

    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = pool->pool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &dst->layout;
    cassert(vkAllocateDescriptorSets(device, &setAllocInfo, &dst->set) == VK_SUCCESS);

    pool->child_sets++;
    dst->pool = pool;

    cassert(pool->canary == PISS_CANARY);
}

#endif