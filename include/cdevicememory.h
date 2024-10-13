#ifndef __CGFX_MEMORY_H
#define __CGFX_MEMORY_H

// ! doesn't take into account alignment, etc.
// * This is so when we eventually have to switch to an allocator it's much easier

#include "vkstdafx.h"

typedef enum cgfx_memory_usage_bits {
    CGFX_MEMORY_USAGE_GPU_LOCAL = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    CGFX_MEMORY_USAGE_CPU_VISIBLE = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    CGFX_MEMORY_USAGE_CPU_READABLE = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // ? what are these two supposed to actually be??
    CGFX_MEMORY_USAGE_CPU_WRITEABLE = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    CGFX_MEMORY_USAGE_LAZILY_ALLOCATED = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
    // ext i think
    CGFX_MEMORY_USAGE_PROTECTED = VK_MEMORY_PROPERTY_PROTECTED_BIT,
    CGFX_MEMORY_USAGE_DEVICE_COHERENT = VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD,
    CGFX_MEMORY_USAGE_DEVICE_UNCACHED = VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD
} cgfx_memory_usage_bits;
typedef unsigned cgfx_memory_usage_t;

typedef struct cgfx_gpu_memory {
    VkDeviceMemory memory;
    cgfx_memory_usage_t usage;
    int size;
} cgfx_gpu_memory;

// I think we should make like a cgfx_err_t enum

// these parameters should be replaced
// properties should be replaced by usage. Like CGFX_MEMORY_USAGE_GPU, CPU_TO_GPU, GPU_TO_CPU, etc.
static void inline cgfx_gpu_memory_allocate(VkDeviceSize size, cgfx_memory_usage_t usage, cgfx_gpu_memory *dst) {
    dst->size = size;
    dst->usage = usage;
    const VkMemoryPropertyFlags properties = (VkMemoryPropertyFlags)usage;

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = size;

    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &mem_properties);

    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((properties & mem_properties.memoryTypes[i].propertyFlags) == properties) {
            alloc_info.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(device, &alloc_info, NULL, &dst->memory) != VK_SUCCESS || !dst->memory) {
        printf("An allocation has failed! What are we to do???\n");
    }
}

static inline void cgfx_gpu_memory_free(cgfx_gpu_memory *mem) {
    vkFreeMemory(device, mem->memory, NULL);
}

// I think these given an error when, say offset is too big so maybe we can check their return values?
static inline void cgfx_gpu_memory_bind_buffer(cgfx_gpu_memory *mem, VkBuffer buffer, VkDeviceSize offset) {
    vkBindBufferMemory(device, buffer, mem->memory, offset);
}
static inline void cgfx_gpu_memory_bind_image(cgfx_gpu_memory *mem, VkImage image, VkDeviceSize offset) {
    vkBindImageMemory(device, image, mem->memory, offset);
}

#endif//__CGFX_MEMORY_H