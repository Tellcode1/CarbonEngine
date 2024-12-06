#ifndef __UNDESCRIPTOR_SET_H
#define __UNDESCRIPTOR_SET_H

#include "vkstdafx.h"
#include "math/math.h"

#define MAX_DESCRIPTOR_SETS 4

typedef struct piss {
    VkDescriptorType type;
    int capacity;
    int using; // how many are being used
} piss;

typedef struct luna_DescriptorPool {
    VkDescriptorPool pool;
    piss descriptors[ 11 ];
} luna_DescriptorPool;

typedef struct luna_DescriptorSet {
    VkDescriptorSetLayout layout;
    VkDescriptorSet set;
} luna_DescriptorSet;

static inline void luna_DescriptorPool_Init(luna_DescriptorPool *dst) {
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 2 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 2 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 2 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 2 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 2 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2 },
    };
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = MAX_DESCRIPTOR_SETS,
        .poolSizeCount = 11,
        .pPoolSizes = pool_sizes
    };
    for (int i = 0; i < 11; i++) {
        dst->descriptors[i] = (piss){ (VkDescriptorType)i, 2, 0 };
    }
    cassert(vkCreateDescriptorPool(device, &poolInfo, NULL, &dst->pool) == VK_SUCCESS);
}

static inline void luna_AllocateDescriptorSet(luna_DescriptorPool *pool, const VkDescriptorSetLayoutBinding *bindings, int nbindings, luna_DescriptorSet *dst) {
    bool need_realloc = 0;
    for (int i = 0; i < 11; i++) {
        for (int j = 0; j < nbindings; j++) {
            if (pool->descriptors[i].type == bindings[j].descriptorType) {
                if ((u32)pool->descriptors[i].capacity < (bindings[j].descriptorCount + pool->descriptors[i].using)) {
                    need_realloc = 1;
                }
            }
        }
    }

    if (need_realloc) {
        VkDescriptorPoolSize pool_sizes[11];
        for (int i = 0; i < 11; i++) {
            for (int j = 0; j < nbindings; j++) {
                if (pool->descriptors[i].type == bindings[j].descriptorType) {
                    pool_sizes[i] = (VkDescriptorPoolSize){ pool->descriptors[i].type, cmmax( pool->descriptors[i].capacity * 2U, bindings[j].descriptorCount ) };
                }
            }
        }

        VkDescriptorPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = MAX_DESCRIPTOR_SETS,
            .poolSizeCount = 11,
            .pPoolSizes = pool_sizes
        };
        vkDestroyDescriptorPool(device, pool->pool, NULL);
        cassert(vkCreateDescriptorPool(device, &poolInfo, NULL, &pool->pool) == VK_SUCCESS);
    }

    VkDescriptorSetLayoutCreateInfo layoutinfo = {};
    layoutinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutinfo.pBindings = bindings;
    layoutinfo.bindingCount = nbindings;
    vkCreateDescriptorSetLayout(device, &layoutinfo, NULL, &dst->layout);

    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = pool->pool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &dst->layout;
    cassert(vkAllocateDescriptorSets(device, &setAllocInfo, &dst->set) == VK_SUCCESS);
}

#endif