#ifndef __UNDESCRIPTOR_SET_H
#define __UNDESCRIPTOR_SET_H

#include "vkstdafx.h"

#define MAX_DESCRIPTOR_SETS 4

typedef struct undescriptorset {
    VkDescriptorPool pool;
    VkDescriptorSetLayout layout;
    VkDescriptorSet set;
} undescriptorset;

static void create_undescriptor_set(undescriptorset *dst, const VkDescriptorSetLayoutBinding *bindings, int nbindings) {
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

    cassert(vkCreateDescriptorPool(device, &poolInfo, NULL, &dst->pool) == VK_SUCCESS);

    VkDescriptorSetLayoutCreateInfo layoutinfo = {};
    layoutinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutinfo.pBindings = bindings;
    layoutinfo.bindingCount = nbindings;
    vkCreateDescriptorSetLayout(device, &layoutinfo, NULL, &dst->layout);

    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = dst->pool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &dst->layout;
    cassert(vkAllocateDescriptorSets(device, &setAllocInfo, &dst->set) == VK_SUCCESS);
}

#endif