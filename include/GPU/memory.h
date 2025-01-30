#ifndef __NOVA_GPU_MEMORY_H__
#define __NOVA_GPU_MEMORY_H__

// ! doesn't take into account alignment, etc.
// * This is so when we eventually have to switch to an allocator it's much easier

// This header should be fragmented into multiple, each for their own object.

#include "../../common/stdafx.h"
#include "format.h"
#include "vkstdafx.h"

NOVA_HEADER_START;

#define NOVA_GPU_MAX_CHILDREN_PER_OBJECT 8
#define NOVA_GPU_ALIGNMENT_UNNECESSARY   (1)

typedef struct NV_GPU_Memory NV_GPU_Memory;

typedef enum NV_GPU_MemoryUsageBits {
  NOVA_GPU_MEMORY_USAGE_GPU_LOCAL        = 1,
  NOVA_GPU_MEMORY_USAGE_CPU_VISIBLE      = 2, // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
  NOVA_GPU_MEMORY_USAGE_CPU_WRITEABLE    = 4, // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
  NOVA_GPU_MEMORY_USAGE_LAZILY_ALLOCATED = 16, // VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT
} NV_GPU_MemoryUsageBits;
typedef unsigned NV_GPU_MemoryUsage;

// I think we should make like a cgfx_err_t enum
extern void NV_GPU_AllocateMemory(size_t size, NV_GPU_MemoryUsage usage, NV_GPU_Memory **dst);
extern void NV_GPU_FreeMemory(NV_GPU_Memory *mem);

extern void NV_GPU_MapMemory(NV_GPU_Memory *memory, size_t size, size_t offset, void **out);
extern void NV_GPU_UnmapMemory(NV_GPU_Memory *memory);

NOVA_HEADER_END;

#endif //__NOVA_MEMORY_H__