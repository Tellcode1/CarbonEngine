#ifndef __EPIC_HELPER_LIBRARY_HPP__
#define __EPIC_HELPER_LIBRARY_HPP__

#include "stdafx.hpp"
#include "Context.hpp"
#include "Renderer.hpp"

namespace help
{
    u32 GetMemoryType(const u32 memoryTypeBits, const VkMemoryPropertyFlags memoryProperties);

    namespace Memory
    {
        void MapMemory(VkDeviceMemory memory, u64 size, u64 offset, void** dstPtr);
        void UnmapMemory(VkDeviceMemory memory);
        void FlushMappedMemory(VkDeviceMemory memory, u64 size, u64 offset);
    }
    namespace Buffers
    {
        /* externallyAllocated = true asserts *dstMemory will not be written to by this function */
        void CreateBuffer(u64 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer* dstBuffer, VkDeviceMemory* dstMemory, bool externallyAllocated = false);

        /*  */
        void StageBufferTransfer(VkBuffer dst, void* data, u64 size);
    }
    namespace Commands
    {
        /* src Must be a valid VkCommandBuffer */
        VkCommandBuffer BeginSingleTimeCommands(VkCommandBuffer src);
        
        /* BeginSingleTimeCommands(new CommandBuffer) */
        VkCommandBuffer BeginSingleTimeCommands();
        
        /* WARNING: waitForExecution = false implies you take responsibility of freeing the commandBuffer! */
        VkResult EndSingleTimeCommands(VkCommandBuffer cmd, VkQueue queue = Graphics->GraphicsAndComputeQueue, bool waitForExecution = true);
    }
    namespace Images
    {
        void LoadFromDisk(
            const char* path, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties, VkImage* dstImage, VkDeviceMemory* dstMemory, bool externallyAllocated = false
        );
        /* No allocation needed */
        void LoadFromDisk(const char* path, u8** dst, u32* dstSize);

        void LoadFromMemory(
            u8* buffer, u32 width, u32 height,
            VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties, VkSampleCountFlagBits samples, u32 mipMapLevels,
            VkImage* dstImage, VkDeviceMemory* dstMemory, bool externallyAllocated = false
        );

        // incomplete = true = the command will be recorded to the active command buffer
        void TransitionImageLayout(
            VkCommandBuffer cmd, VkImage image, u32 mipLevels,
            VkImageLayout oldLayout, VkImageLayout newLayout,
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage
        );
    }
    namespace Files
    {
        void Load(const char* path, u8* dst, u32* dstSize);
        void LoadBinary(const char* path, u8* dst, u32* dstSize);
    }
}

#endif