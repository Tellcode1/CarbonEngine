#ifndef __LUNA_BUFFER_H__
#define __LUNA_BUFFER_H__

#include "vkstdafx.h"

typedef struct luna_GPU_Memory luna_GPU_Memory;

// This is mainly here to bar the user from using unsupported buffer types
typedef enum luna_GPU_BufferType {
    LUNA_GPU_BUFFER_TYPE_VERTEX_BUFFER = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    LUNA_GPU_BUFFER_TYPE_INDEX_BUFFER = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    LUNA_GPU_BUFFER_TYPE_UNIFORM_BUFFER = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    LUNA_GPU_BUFFER_TYPE_STORAGE_BUFFER = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    LUNA_GPU_BUFFER_TYPE_TRANSFER_SOURCE = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    LUNA_GPU_BUFFER_TYPE_TRANSFER_DESTINATION = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    LUNA_GPU_BUFFER_TYPE_INDIRECT_BUFFER = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
} luna_GPU_BufferType;

typedef struct luna_GPU_Buffer {
    VkBuffer buffer;
    // The size of the buffer
    // Even if there are multiple children, this gives only the size of ONE buffer
    int size, alignment, offset;
    luna_GPU_Memory *memory;
    luna_GPU_BufferType type;
} luna_GPU_Buffer;

// Note: 'nchilds' count of buffers of size 'size' will be created.
// The size will NOT be divided among the children.
extern void luna_GPU_CreateBuffer(int size, int alignment, VkBufferUsageFlags usage, luna_GPU_Buffer *dst);
extern void luna_GPU_DestroyBuffer(luna_GPU_Buffer *buffer);

extern void luna_GPU_WriteToBuffer(luna_GPU_Buffer *buffer, int size, void *data, int offset);

// Note: Memory must be able to hold all the buffers!
// You can get the size of the memory by just looking up the size of one buffer
// and then multiplying it with the count.
extern void luna_GPU_BindBufferToMemory(luna_GPU_Memory *mem, int offset, luna_GPU_Buffer *buffer);

extern int luna_GPU_GetBufferSize(const luna_GPU_Buffer *buffer);

#endif//__LUNA_BUFFER_H__