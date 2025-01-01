#ifndef __LUNA_GPU_MEMORY_H
#define __LUNA_GPU_MEMORY_H

// ! doesn't take into account alignment, etc.
// * This is so when we eventually have to switch to an allocator it's much easier

// This header should be fragmented into multiple, each for their own object.

#include "lunaGFXstdafx.h"
#include "lunaFormat.h"

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

typedef struct luna_GPU_Memory {
    VkDeviceMemory memory;
    void *mapped;
    int map_size, map_offset;
    luna_GPU_MemoryUsage usage;
    int size;
} luna_GPU_Memory;

typedef struct luna_GPU_Buffer {
    VkBuffer buffers[ LUNA_GPU_MAX_CHILDREN_PER_OBJECT ];
    int nchilds;
    // The size of the buffer
    // Even if there are multiple children, this gives only the size of ONE buffer
    int size, alignment, offset;
    luna_GPU_Memory *memory;
    luna_GPU_BufferType type;
} luna_GPU_Buffer;

typedef struct luna_GPU_TextureCreateInfo {
    lunaFormat format;
    lunaSampleCount samples;
    VkImageType type;
    VkImageUsageFlags usage;
    VkExtent3D extent;
    int arraylayers;
    int miplevels;
} luna_GPU_TextureCreateInfo;

typedef struct luna_GPU_TextureViewCreateInfo {
    lunaFormat format; // May be VK_FORMAT_UNDEFINED for automatic fetching of format from image
    VkImageViewType view_type;
    VkImageSubresourceRange subresourceRange;
} luna_GPU_TextureViewCreateInfo;

typedef struct luna_GPU_SamplerCreateInfo {
    VkFilter filter;
    VkSamplerMipmapMode mipmap_mode;
    VkSamplerAddressMode address_mode;
    float max_anisotropy;
    float mip_lod_bias, min_lod, max_lod;
} luna_GPU_SamplerCreateInfo;

typedef struct luna_GPU_Texture luna_GPU_Texture;
typedef struct luna_GPU_Sampler luna_GPU_Sampler;

// I think we should make like a cgfx_err_t enum
extern void luna_GPU_AllocateMemory(VkDeviceSize size, luna_GPU_MemoryUsage usage, luna_GPU_Memory *dst);
extern void luna_GPU_FreeMemory(luna_GPU_Memory *mem);

// Note: 'nchilds' count of buffers of size 'size' will be created.
// The size will NOT be divided among the children.
extern void luna_GPU_CreateBuffer(int size, int alignment, int nchilds, VkBufferUsageFlags usage, luna_GPU_Buffer *dst);

extern void luna_GPU_MapMemory(luna_GPU_Memory *memory, int size, int offset, void **out);
extern void luna_GPU_UnmapMemory(luna_GPU_Memory *memory);

extern void luna_GPU_WriteToBuffer(luna_GPU_Buffer *buffer, int size, void *data, int offset);

// Note: Memory must be able to hold all the buffers!
// You can get the size of the memory by just looking up the size of one buffer
// and then multiplying it with the count.
extern void luna_GPU_BindBufferToMemory(luna_GPU_Memory *mem, int offset, luna_GPU_Buffer *buffer);
extern void luna_GPU_DestroyBuffer(luna_GPU_Buffer *buffer);

extern int luna_GPU_GetBufferSize(const luna_GPU_Buffer *buffer);
extern void luna_GPU_GetTextureSize(const luna_GPU_Texture *tex, int *w, int *h);

extern void luna_GPU_CreateTexture(const luna_GPU_TextureCreateInfo *pInfo, luna_GPU_Texture **tex);
extern void luna_GPU_CreateTextureView(const luna_GPU_TextureViewCreateInfo *pInfo, luna_GPU_Texture **tex);

extern void luna_GPU_TextureAttachView(luna_GPU_Texture *tex, VkImageView view);

extern void luna_GPU_BindTextureToMemory(luna_GPU_Memory *mem, int offset, luna_GPU_Texture *tex);
extern void luna_GPU_DestroyTexture(luna_GPU_Texture *tex);

extern void luna_GPU_CreateSampler(const luna_GPU_SamplerCreateInfo *pInfo, luna_GPU_Sampler **sampler);

extern VkImage luna_GPU_TextureGet(const luna_GPU_Texture *tex);
extern VkImageView luna_GPU_TextureGetView(const luna_GPU_Texture *tex);
extern VkSampler luna_GPU_SamplerGet(const luna_GPU_Sampler *sampler);

#endif//__LUNA_GPU_MEMORY_H