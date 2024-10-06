#include <stdlib.h>
#include "../include/vkstdafx.h"
#include "../include/cbuffer.h"

#include "../external/volk/volk.h"
#include "cbuffer.h"

typedef struct cbuffer_t {
    VkBuffer buf;
    VkDeviceMemory mem;
    cgfx_buffer_type type; cgfx_buffer_usage usage; // i think it can be stored as a mask to save memory
    int offset;
} cbuffer_t;

cbuffer_t *cbuffer_create(int datasize, cgfx_buffer_type type, cgfx_buffer_usage usage)
{
    VkBuffer newBuffer;
    VkDeviceMemory newMemory;

    VkBufferUsageFlags usage;
    switch(type) {
        case CGFX_BUFFER_TYPE_VERTEX_BUFFER:
            usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            break;
        case CGFX_BUFFER_TYPE_INDEX_BUFFER:
            usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            break;
        case CGFX_BUFFER_TYPE_SSBO:
            usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            break;
        default:
            usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            break;
        break;
    }

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = datasize;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bufferCreateInfo, NULL, &newBuffer) != VK_SUCCESS)
    {
        printf("failed to create staging buffer\n");
        return NULL;
    }

    VkMemoryRequirements bufferMemoryRequirements;
    vkGetBufferMemoryRequirements(device, newBuffer, &bufferMemoryRequirements);

    VkMemoryPropertyFlags propertyFlags;
    switch (usage) {

    }
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = bufferMemoryRequirements.size;
    allocInfo.memoryTypeIndex = GetMemoryType(bufferMemoryRequirements.memoryTypeBits, propertyFlags);
    if (vkAllocateMemory(device, &allocInfo, NULL, &newMemory) != VK_SUCCESS)
    {
        printf("failed to allocate staging buffer memory\n");
        return NULL;
    }
    
    vkBindBufferMemory(device, newBuffer, newMemory, 0);
    
    cbuffer_t *buf = malloc(sizeof(cbuffer_t));
    buf->buf = newBuffer;
    buf->mem = newMemory;
    return buf;
}
