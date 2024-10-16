#ifndef __VKH_HPP
#define __VKH_HPP

#ifdef __cplusplus
    extern "C" {
#endif

#include "vkstdafx.h"

extern u32 vkh_get_mem_type(cg_device_t *device, const u32 memoryTypeBits, const VkMemoryPropertyFlags memoryProperties);

/* externallyAllocated = true asserts *dstMemory will not be written to by this function */
extern void vkh_buffer_create(cg_device_t *device, u64 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer* dstBuffer, VkDeviceMemory* dstMemory, bool externallyAllocated);

/*  */
extern void vkh_buffer_stage_transfer(cg_device_t *device, VkBuffer dst, void* data, u64 size);

/* src Must be a valid VkCommandBuffer */
extern VkCommandBuffer vkh_cmd_begin_from(cg_device_t *device, VkCommandBuffer src);

/* BeginSingleTimeCommands(new CommandBuffer) */
extern VkCommandBuffer vkh_cmd_begin(cg_device_t *device);

/* WARNING: waitForExecution = false implies you take responsibility of freeing the commandBuffer! */
extern VkResult vkh_cmd_end(cg_device_t *device, VkCommandBuffer cmd, VkQueue queue, bool waitForExecution);

extern void vkh_stage_image_transfer(cg_device_t *device, VkImage dst, void* data, u32 width, u32 height, u32 channels);

extern void vkh_image_from_mem(cg_device_t *device, u8 *buffer, u32 width, u32 height, VkFormat format, u32 channels, VkImage *dst, VkDeviceMemory *dstMem);
extern u8* vkh_image_from_disk(cg_device_t *device, const char *path, u32 *width, u32 *height, VkFormat *channels, VkImage *dst, VkDeviceMemory *dstMem);

extern void vkh_image_create_empty(cg_device_t *device, u32 width, u32 height, VkFormat format, VkSampleCountFlagBits samples, u32 channels, VkImageUsageFlags usage, VkImage *dst, VkDeviceMemory *dstMem);

extern bool vkh_get_supported_fmt(cg_device_t *device, VkPhysicalDevice physDevice, VkSurfaceKHR surface, VkFormat* dstFormat, VkColorSpaceKHR* dstColorSpace);
extern u32 vkh_get_image_count(VkPhysicalDevice physDevice, VkSurfaceKHR surface);
extern void vkh_transition_image_layout(
    cg_device_t *device, VkCommandBuffer cmd, VkImage image, u32 mipLevels, VkImageAspectFlagBits aspect,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
    VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage
);
extern void vkh_files_load_binary(cg_device_t *device, const char* path, u8* dst, u32* dstSize);

#ifdef __cplusplus
    }
#endif

#endif//__VKH_HPP