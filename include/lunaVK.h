#ifndef __LUNA_VK_H
#define __LUNA_VK_H

#ifdef __cplusplus
    extern "C" {
#endif

#include "vkstdafx.h"
#include "lunaFormat.h"

extern u32 luna_VK_GetMemType(const u32 memoryTypeBits, const VkMemoryPropertyFlags memoryProperties);

/* externallyAllocated = true asserts *dstMemory will not be written to by this function */
extern void luna_VK_CreateBuffer(u64 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer* dstBuffer, VkDeviceMemory* dstMemory, bool externallyAllocated);

/*  */
extern void luna_VK_StageBufferTransfer(VkBuffer dst, void* data, u64 size);

/* src Must be a valid VkCommandBuffer */
extern VkCommandBuffer luna_VK_BeginCommandBufferFrom(VkCommandBuffer src);

/* BeginSingleTimeCommands(new CommandBuffer) */
extern VkCommandBuffer luna_VK_BeginCommandBuffer();

/* WARNING: waitForExecution = false implies you take responsibility of freeing the commandBuffer! */
extern VkResult luna_VK_EndCommandBuffer(VkCommandBuffer cmd, VkQueue queue, bool waitForExecution);

extern void luna_VK_StageImageTransfer(VkImage dst, const void* data, int width, int height, int image_size);

extern void luna_VK_CreateTextureFromMemory(u8 *buffer, u32 width, u32 height, lunaFormat format, VkImage *dst, VkDeviceMemory *dstMem);

extern u8* luna_VK_CreateTextureFromDisk(const char *path, u32 *width, u32 *height, lunaFormat *channels, VkImage *dst, VkDeviceMemory *dstMem);

extern void luna_VK_CreateTextureEmpty(u32 width, u32 height, lunaFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage, int *image_size, VkImage *dst, VkDeviceMemory *dstMem);

extern void luna_VK_TransitionTextureLayout(
    VkCommandBuffer cmd, VkImage image, u32 mipLevels, VkImageAspectFlagBits aspect,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
    VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage
);

extern lunaFormat luna_VK_GetSupportedFormatForDraw(lunaFormat fmt);

extern bool luna_VK_GetSupportedFormat(VkPhysicalDevice physDevice, VkSurfaceKHR surface, VkFormat* dstFormat, VkColorSpaceKHR* dstColorSpace);

extern u32 luna_VK_GetSurfaceImageCount(VkPhysicalDevice physDevice, VkSurfaceKHR surface);

extern void luna_VK_LoadBinaryFile(const char* path, u8* dst, u32* dstSize);

#ifdef __cplusplus
    }
#endif

#endif//__LUNA_VK_H