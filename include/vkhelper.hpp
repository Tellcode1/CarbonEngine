#ifndef __VKH_HPP
#define __VKH_HPP

#include "vkstdafx.h"

template <typename T>
struct cvector;

namespace help
{
    u32 GetMemoryType(const u32 memoryTypeBits, const VkMemoryPropertyFlags memoryProperties);

    namespace Memory
    {
        void MapMemory(VkDeviceMemory memory, u64 size, u64 offset, void** dstPtr);
        void UnmapMemory(VkDeviceMemory memory);
        void FlushMappedMemory(VkDeviceMemory memory, u64 size, u64 offset);
	    u32 GetMemoryType(VkPhysicalDevice physDevice, const u32 memoryTypeBits, const VkMemoryPropertyFlags properties);
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
        VkResult EndSingleTimeCommands(VkCommandBuffer cmd, VkQueue queue = GraphicsAndComputeQueue, bool waitForExecution = true);
    }
    namespace Images
    {
        // Making a library for loading vulkan images is so goddamn hard. I don't even want to anymore.

        /*
            No allocation needed
            dst may be nullptr
        */
        void LoadFromDisk(const char* path, u8 channels, u8** dst, u32* dstWidth, u32* dstHeight);

        // There were so many things wrong with this function. I don't want to look at this function again

        // void LoadVulkanImage(
        //     u8* buffer, u32 width, u32 height,
        //     VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
        //     VkMemoryPropertyFlags properties, VkSampleCountFlagBits samples, u32 mipMapLevels,
        //     VkImage* dstImage, VkDeviceMemory* dstMemory, bool externallyAllocated = false
        // );

        void StageImageTransfer(VkImage dst, void* data, u32 width, u32 height, u32 channels);
        
        // makes image for vulkan by loading it with stb
        // I'm the god of removing something, realising it was important and then adding it back again, just differently.
        void killme(u8 *buffer, u32 width, u32 height, VkFormat format, u32 channels, VkImage *dst, VkDeviceMemory *dstMem);
        void create_empty(u32 width, u32 height, VkFormat format, VkSampleCountFlagBits samples, u32 channels, VkImageUsageFlags usage, VkImage *dst, VkDeviceMemory *dstMem);
        u8* killme(const char *path, u32 *width, u32 *height, VkFormat *channels, VkImage *dst, VkDeviceMemory *dstMem);

        // incomplete = true = the command will be recorded to the active command buffer
        void TransitionImageLayout(
            VkCommandBuffer cmd, VkImage image, u32 mipLevels, VkImageAspectFlagBits aspect,
            VkImageLayout oldLayout, VkImageLayout newLayout,
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
            VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage
        );
        
        // I don't know where to keep this functions.
        bool GetSupportedFormat(VkDevice device, VkPhysicalDevice physDevice, VkSurfaceKHR surface, VkFormat* dstFormat, VkColorSpaceKHR* dstColorSpace);
        u32 GetImageCount(VkPhysicalDevice physDevice, VkSurfaceKHR surface);
    }
    namespace Files
    {
        void Load(const char* path, u8* dst, u32* dstSize);

        /*
        *   Calls resize on dst!
        */
        void LoadBinary(const char *path, cvector<u8> *dst);
        void LoadBinary(const char* path, u8* dst, u32* dstSize);
    }
}

#endif//__VKH_HPP