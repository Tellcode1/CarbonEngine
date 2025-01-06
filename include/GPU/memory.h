#ifndef __LUNA_MEMORY_H__
#define __LUNA_MEMORY_H__

// ! doesn't take into account alignment, etc.
// * This is so when we eventually have to switch to an allocator it's much easier

// This header should be fragmented into multiple, each for their own object.

#include "../lunaGFXstdafx.h"
#include "format.h"

#define LUNA_GPU_MAX_CHILDREN_PER_OBJECT 8
#define LUNA_GPU_ALIGNMENT_UNNECESSARY (1)
#define LUNA_GPU_ALL_CHILDREN (__INT32_MAX__)

typedef enum luna_GPU_MemoryUsageBits {
    LUNA_GPU_MEMORY_USAGE_GPU_LOCAL = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    LUNA_GPU_MEMORY_USAGE_CPU_VISIBLE = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    LUNA_GPU_MEMORY_USAGE_CPU_WRITEABLE = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    LUNA_GPU_MEMORY_USAGE_LAZILY_ALLOCATED = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
} luna_GPU_MemoryUsageBits;
typedef unsigned luna_GPU_MemoryUsage;

typedef struct luna_GPU_Memory {
    VkDeviceMemory memory;
    void *mapped;
    int map_size, map_offset;
    luna_GPU_MemoryUsage usage;
    int size;
} luna_GPU_Memory;

// I think we should make like a cgfx_err_t enum
extern void luna_GPU_AllocateMemory(VkDeviceSize size, luna_GPU_MemoryUsage usage, luna_GPU_Memory *dst);
extern void luna_GPU_FreeMemory(luna_GPU_Memory *mem);

extern void luna_GPU_MapMemory(luna_GPU_Memory *memory, int size, int offset, void **out);
extern void luna_GPU_UnmapMemory(luna_GPU_Memory *memory);

#endif//__LUNA_MEMORY_H__