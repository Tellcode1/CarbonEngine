#ifndef __NV_VK_H
#define __NV_VK_H

#include "../../common/containers/vector.h"
#include "../../common/mem.h"
#include "../../common/stdafx.h"
#include "../../external/volk/volk.h"
#include "format.h"

NOVA_HEADER_START;

// YOU SAW NOTHING

static inline void *vkalloc(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
  NVAllocator alloc;
  NVAllocatorBindHeapAllocator(&alloc, &pool);
  return poolmalloc(&alloc, alignment, size);
}

static inline void *vkrealloc(void *pUserData, void *pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
  NVAllocator alloc;
  NVAllocatorBindHeapAllocator(&alloc, &pool);
  return poolrealloc(&alloc, pOriginal, alignment, size);
}

static inline void vkfree(void *user_data, void *p)
{
  NVAllocator alloc;
  NVAllocatorBindHeapAllocator(&alloc, &pool);
  poolfree(&alloc, p);
}

static inline void vkInternalAllocationNotification(void *pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) {}
static inline void vkInternalFreeNotification(void *pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) {}

static VkAllocationCallbacks vkallocator = {.pUserData             = NULL,
                                            .pfnAllocation         = vkalloc,
                                            .pfnReallocation       = vkrealloc,
                                            .pfnFree               = vkfree,
                                            .pfnInternalAllocation = vkInternalAllocationNotification,
                                            .pfnInternalFree       = vkInternalFreeNotification};

extern u32                   NV_VK_GetMemType(const u32 memoryTypeBits, const VkMemoryPropertyFlags memoryProperties);

/* externallyAllocated = true asserts *dstMemory will not be written to by this function */
extern void NV_VK_CreateBuffer(size_t size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer *dstBuffer, VkDeviceMemory *dstMemory,
                                 bool externallyAllocated);

/*  */
extern void NV_VK_StageBufferTransfer(VkBuffer dst, void *data, size_t size);

/* src Must be a valid VkCommandBuffer */
extern VkCommandBuffer NV_VK_BeginCommandBufferFrom(VkCommandBuffer src);

/* BeginSingleTimeCommands(new CommandBuffer) */
extern VkCommandBuffer NV_VK_BeginCommandBuffer();

/* WARNING: waitForExecution = false implies you take responsibility of freeing the commandBuffer! */
extern VkResult NV_VK_EndCommandBuffer(VkCommandBuffer cmd, VkQueue queue, bool waitForExecution);

extern void     NV_VK_StageImageTransfer(VkImage dst, const void *data, int width, int height, int image_size);

extern void     NV_VK_CreateTextureFromMemory(u8 *buffer, u32 width, u32 height, NVFormat format, VkImage *dst, VkDeviceMemory *dstMem);

extern u8      *NV_VK_CreateTextureFromDisk(const char *path, u32 *width, u32 *height, NVFormat *channels, VkImage *dst, VkDeviceMemory *dstMem);

extern void     NV_VK_CreateTextureEmpty(u32 width, u32 height, NVFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage, int *image_size, VkImage *dst,
                                           VkDeviceMemory *dstMem);

extern void NV_VK_TransitionTextureLayout(VkCommandBuffer cmd, VkImage image, u32 mipLevels, VkImageAspectFlagBits aspect, VkImageLayout oldLayout, VkImageLayout newLayout,
                                            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);

extern NVFormat NV_VK_GetSupportedFormatForDraw(NVFormat fmt);

extern bool       NV_VK_GetSupportedFormat(VkPhysicalDevice physDevice, VkSurfaceKHR surface, NVFormat *dstFormat, VkColorSpaceKHR *dstColorSpace);

extern u32        NV_VK_GetSurfaceImageCount(VkPhysicalDevice physDevice, VkSurfaceKHR surface);

extern void       NV_VK_LoadBinaryFile(const char *path, u8 *dst, u32 *dstSize);

NOVA_HEADER_END;

#endif //__NV_VK_H