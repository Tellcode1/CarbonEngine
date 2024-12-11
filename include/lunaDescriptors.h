#ifndef __UNDESCRIPTOR_SET_H
#define __UNDESCRIPTOR_SET_H

#include "vkstdafx.h"
#include "math/math.h"
#include "cgvector.h"

// WARNING: Currently only supports the first 11 descriptor types.

#define POOL_CANARY 293012
#define SET_CANARY 120983

typedef struct luna_DescriptorPoolSize {
    VkDescriptorType type;
    int capacity;
    int using; // how many are being used
} luna_DescriptorPoolSize;

typedef struct luna_DescriptorSet luna_DescriptorSet;

typedef struct luna_DescriptorPool {
    int canary;
    VkDescriptorPool pool;
    int max_child_sets;
    luna_DescriptorPoolSize descriptors[ 11 ];
    luna_DescriptorSet **sets;
    int nsets;
} luna_DescriptorPool;

typedef struct luna_DescriptorSet {
    int canary;
    VkDescriptorSetLayout layout;
    VkDescriptorSet set;
    luna_DescriptorPool *pool;
    VkWriteDescriptorSet *writes;
    int nwrites;
} luna_DescriptorSet;

static inline void luna_DescriptorSetSubmitWrite(luna_DescriptorSet *set, const VkWriteDescriptorSet *write) {
    set->writes = realloc(set->writes, (set->nwrites+1) * sizeof(VkWriteDescriptorSet));
    cassert(set->writes != NULL);
    memcpy(&set->writes[set->nwrites], write, sizeof(VkWriteDescriptorSet));
    set->nwrites++;
    vkUpdateDescriptorSets(device, 1, write, 0, 0);
}

static inline void luna_DescriptorSetDestroy(luna_DescriptorSet *set) {
    vkDestroyDescriptorSetLayout(device, set->layout, NULL);
    free(set->writes);
    free(set);
}

static inline void luna_DescriptorPoolDestroy(luna_DescriptorPool *pool) {
    for (int i = 0; i < pool->nsets; i++) {
        luna_DescriptorSetDestroy(pool->sets[i]);
    }
    free(pool->sets);
    vkDestroyDescriptorPool(device, pool->pool, NULL);
}

static inline void __luna_DescriptorPool_Allocate(luna_DescriptorPool *pool) {
    VkDescriptorPoolSize allocations[11] = {};
    int descriptors_written = 0;

    for (int i = 0; i < 11; i++) {
        if (pool->descriptors[i].capacity == 0) {
            continue;
        }
        allocations[descriptors_written] = (VkDescriptorPoolSize){ pool->descriptors[i].type, pool->descriptors[i].capacity };
        descriptors_written++;
    }

    bool need_more_max_sets = (pool->nsets + 1) > pool->max_child_sets;
    if (need_more_max_sets) {
            pool->max_child_sets = cmmax(pool->max_child_sets * 2, 1);
    }

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = pool->max_child_sets,
        .poolSizeCount = descriptors_written,
        .pPoolSizes = allocations
    };

    VkDescriptorPool new_pool;
    cassert(vkCreateDescriptorPool(device, &poolInfo, NULL, &new_pool) == VK_SUCCESS);

    VkDescriptorSet *new_sets = malloc(sizeof(VkDescriptorSet) * pool->nsets);
    cassert(new_sets != NULL);

    if (pool->nsets > 0) {
        VkDescriptorSetLayout *layouts = malloc(sizeof(VkDescriptorSetLayout) * pool->nsets);
        for (int i = 0; i < pool->nsets; i++) {
            layouts[i] = pool->sets[i]->layout;
        }

        VkDescriptorSetAllocateInfo setAllocInfo = {};
        setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        setAllocInfo.descriptorPool = new_pool;
        setAllocInfo.descriptorSetCount = pool->nsets;
        setAllocInfo.pSetLayouts = layouts;
        cassert(vkAllocateDescriptorSets(device, &setAllocInfo, new_sets) == VK_SUCCESS);

        free(layouts);
    }

    int ncopies = 0;
    for (int i = 0; i < pool->nsets; i++) {
        ncopies += pool->sets[i]->nwrites;
    }
    VkCopyDescriptorSet *copies = malloc(sizeof(VkCopyDescriptorSet) * ncopies);

    ncopies = 0;
    for (int i = 0; i < pool->nsets; i++) {
        luna_DescriptorSet *old_set = pool->sets[i];

        cassert(old_set->canary == SET_CANARY);

        for (int writei = 0; writei < old_set->nwrites; writei++) {
            VkWriteDescriptorSet *write = &old_set->writes[writei];
             copies[ncopies] = (VkCopyDescriptorSet){
                .sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
                .srcSet = old_set->set,
                .srcBinding = write->dstBinding,
                .srcArrayElement = write->dstArrayElement,
                .dstSet = new_sets[i],
                .dstBinding = write->dstBinding,
                .dstArrayElement = write->dstArrayElement,
                .descriptorCount = write->descriptorCount,
            };
            ncopies++;
        }
        old_set->set = new_sets[i];
    }
    vkUpdateDescriptorSets(device, 0, NULL, ncopies, copies);

    if (pool->pool) {
        vkDestroyDescriptorPool(device, pool->pool, NULL);
    }
    pool->pool = new_pool;

    free(new_sets);
    free(copies);
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
    dst->sets = malloc(sizeof(luna_DescriptorSet *));
    dst->max_child_sets = 1;
    dst->nsets = 0;
    dst->canary = POOL_CANARY;
    __luna_DescriptorPool_Allocate(dst);
    cassert(dst->canary == POOL_CANARY);
}

static inline void luna_AllocateDescriptorSet(luna_DescriptorPool *pool, const VkDescriptorSetLayoutBinding *bindings, int nbindings, luna_DescriptorSet **dst) {
    cassert(pool->canary == POOL_CANARY);

    bool need_realloc = 0;
    for (int i = 0; i < 11; i++) {
        for (int j = 0; j < nbindings; j++) {
            luna_DescriptorPoolSize *descriptor = &pool->descriptors[i];
            if (descriptor->type == bindings[j].descriptorType) {
                descriptor->capacity = cmmax(descriptor->capacity * 2, (int)bindings[j].descriptorCount + descriptor->capacity);
                need_realloc = 1;
            }
        }
    }

    if (need_realloc || ((pool->nsets + 1) > pool->max_child_sets)) {
        if ((pool->nsets + 1) > pool->max_child_sets) {
            pool->max_child_sets = cmmax(pool->max_child_sets * 2, 1);
            pool->sets = realloc(pool->sets, pool->max_child_sets * sizeof(luna_DescriptorSet));
        }

        __luna_DescriptorPool_Allocate(pool);
    }

    luna_DescriptorSet *set = calloc(1, sizeof(luna_DescriptorSet));

    pool->sets[pool->nsets] = set;
    pool->nsets++;

    (*dst) = set;

    set->canary = SET_CANARY;
    set->pool = pool;
    set->writes = malloc(sizeof(VkWriteDescriptorSet));
    cassert(set->writes != NULL);

    VkDescriptorSetLayoutCreateInfo layoutinfo = {};
    layoutinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutinfo.pBindings = bindings;
    layoutinfo.bindingCount = nbindings;
    cassert(vkCreateDescriptorSetLayout(device, &layoutinfo, NULL, &set->layout) == VK_SUCCESS);

    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = pool->pool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &set->layout;
    cassert(vkAllocateDescriptorSets(device, &setAllocInfo, &set->set) == VK_SUCCESS);

    cassert(pool->canary == POOL_CANARY);
    cassert(set->canary == SET_CANARY);
}


#endif