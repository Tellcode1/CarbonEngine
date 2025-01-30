#ifndef __NOVA_BUFFER_H__
#define __NOVA_BUFFER_H__

#include "../../common/stdafx.h"

NOVA_HEADER_START;

typedef struct NV_GPU_Memory NV_GPU_Memory;

// This is mainly here to bar the user from using unsupported buffer types
typedef enum NV_GPU_BufferType {
  NOVA_GPU_BUFFER_TYPE_VERTEX_BUFFER        = 128,
  NOVA_GPU_BUFFER_TYPE_INDEX_BUFFER         = 64,
  NOVA_GPU_BUFFER_TYPE_UNIFORM_BUFFER       = 16,
  NOVA_GPU_BUFFER_TYPE_STORAGE_BUFFER       = 32,
  NOVA_GPU_BUFFER_TYPE_TRANSFER_SOURCE      = 1,
  NOVA_GPU_BUFFER_TYPE_TRANSFER_DESTINATION = 2,
  NOVA_GPU_BUFFER_TYPE_INDIRECT_BUFFER      = 256,
} NV_GPU_BufferType;

typedef struct NV_GPU_Buffer {
  struct VkBuffer_T *buffer;
  // The size of the buffer
  // Even if there are multiple children, this gives only the size of ONE buffer
  size_t size, offset;
  int alignment;
  NV_GPU_Memory *memory;
  NV_GPU_BufferType type;
} NV_GPU_Buffer;

// Note: 'nchilds' count of buffers of size 'size' will be created.
// The size will NOT be divided among the children.
extern void NV_GPU_CreateBuffer(size_t size, int alignment, uint32_t usage, NV_GPU_Buffer *dst);
extern void NV_GPU_DestroyBuffer(NV_GPU_Buffer *buffer);

extern void NV_GPU_WriteToBuffer(NV_GPU_Buffer *buffer, size_t size, void *data, size_t offset);

// Note: Memory must be able to hold all the buffers!
// You can get the size of the memory by just looking up the size of one buffer
// and then multiplying it with the count.
extern void NV_GPU_BindBufferToMemory(NV_GPU_Memory *mem, size_t offset, NV_GPU_Buffer *buffer);

extern int NV_GPU_GetBufferSize(const NV_GPU_Buffer *buffer);

// dest must be atleast the size of the buffer
extern void NV_GPU_BufferReadback(const NV_GPU_Buffer *buffer, void *dest);

// extern NVAsync_Context NV_GPU_BufferReadbackAsync(const NV_GPU_Buffer *buffer, void *dest);

NOVA_HEADER_END;

#endif //__NOVA_BUFFER_H__