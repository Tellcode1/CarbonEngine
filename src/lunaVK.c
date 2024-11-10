#include "../include/lunaVK.h"
#include "../include/lunaPipeline.h"
#include "../include/lunaGFX.h"
#include "../include/lunaImage.h"

#include "../include/cgvector.h"

#include <stdio.h>
#include <stdlib.h>

VkCommandPool pool = VK_NULL_HANDLE;
VkCommandBuffer buffer = VK_NULL_HANDLE;

void luna_VK_CreateBuffer( u64 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer *dstBuffer, VkDeviceMemory *retMem, bool externallyAllocated)
{
    if(size == 0) {
        return;
    }

    VkBuffer newBuffer;
    VkDeviceMemory newMemory;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bufferCreateInfo, NULL, &newBuffer) != VK_SUCCESS)
    {
        printf("Renderer::failed to create staging buffer!");
    }

    VkMemoryRequirements bufferMemoryRequirements;
    vkGetBufferMemoryRequirements(device, newBuffer, &bufferMemoryRequirements);
    
    if(!externallyAllocated)
    {
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = bufferMemoryRequirements.size;
        allocInfo.memoryTypeIndex = luna_VK_GetMemType(bufferMemoryRequirements.memoryTypeBits, propertyFlags);
        if (vkAllocateMemory(device, &allocInfo, NULL, &newMemory) != VK_SUCCESS)
        {
            LOG_ERROR("failed to alloc gpu memory for buffer");
        }
        
        vkBindBufferMemory(device, newBuffer, newMemory, 0);
        *retMem = newMemory;
    }

    *dstBuffer = newBuffer;
}

void luna_VK_StageBufferTransfer( VkBuffer dst, void *data, u64 size)
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkBufferCreateInfo stagingBufferInfo = {};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = size;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &stagingBufferInfo, NULL, &stagingBuffer) != VK_SUCCESS)
    {
        printf("Failed to create staging buffer!");
    }

    VkMemoryRequirements stagingBufferMemoryRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferMemoryRequirements);
    
    VkMemoryAllocateInfo stagingBufferAlloc = {};
    stagingBufferAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingBufferAlloc.allocationSize = stagingBufferMemoryRequirements.size;
    stagingBufferAlloc.memoryTypeIndex = luna_VK_GetMemType(stagingBufferMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device, &stagingBufferAlloc, NULL, &stagingBufferMemory) != VK_SUCCESS)
    {
        printf("Failed to allocate staging buffer memory!");
    }
    
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void* mapped;
    vkMapMemory(device, stagingBufferMemory, 0, size, 0, &mapped);
    memcpy(mapped, data, size);
    vkUnmapMemory(device, stagingBufferMemory);

    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);
}

u32 luna_VK_GetMemType( const u32 memoryTypeBits, const VkMemoryPropertyFlags memoryProperties)
{
    VkPhysicalDeviceMemoryProperties properties;
	vkGetPhysicalDeviceMemoryProperties(physDevice, &properties);

	for (u32 i = 0; i < properties.memoryTypeCount; i++) 
	{
    	if ((memoryTypeBits & (1 << i)) && (properties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties)
        	return i;
	}

    return UINT32_MAX;
}

VkCommandBuffer luna_VK_BeginCommandBufferFrom( VkCommandBuffer src)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(src, &beginInfo);

    return src;
}

VkCommandBuffer luna_VK_BeginCommandBuffer()
{
    if (!pool) {
        VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
        cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolCreateInfo.queueFamilyIndex = GraphicsFamilyIndex;
        cmdPoolCreateInfo.flags = 0;
        if(vkCreateCommandPool(device, &cmdPoolCreateInfo, NULL, &pool) != VK_SUCCESS) {
            LOG_ERROR("Failed to create command pool");
        }
    }

    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.commandPool = pool;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, &buffer);

    return luna_VK_BeginCommandBufferFrom(buffer);
}

VkResult luna_VK_EndCommandBuffer( VkCommandBuffer cmd, VkQueue queue, bool waitForExecution)
{
    VkResult res = vkEndCommandBuffer(cmd);
    if (res != VK_SUCCESS) return res;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0;

    if (waitForExecution) {
        res = vkCreateFence(device, &fenceInfo, NULL, &fence);
        if (res != VK_SUCCESS) return res;
    }

    res = vkQueueSubmit(queue, 1, &submitInfo, fence);
    if (res != VK_SUCCESS) return res;

    if (waitForExecution) {
        if (fence != VK_NULL_HANDLE) {
            res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
            if (res != VK_SUCCESS) return res;
            vkDestroyFence(device, fence, NULL);
        }
        vkDeviceWaitIdle(device);
        vkFreeCommandBuffers(device, pool, 1, &cmd);
        buffer = NULL;
    }

    return VK_SUCCESS;
}

// void help::Files::LoadBinary(const char * path, cg_vector<u8>* dst)
// {
//     FILE *f = fopen(path, "rb");
//     cassert(f != NULL);

//     fseek(f, 0, SEEK_END);
//     int file_size = ftell(f);
//     rewind(f);

//     dst->resize( file_size );
//     fread(dst->data(), 1, file_size, f);

//     fclose(f);
// }

void luna_VK_LoadBinaryFile( const char* path, u8* dst, u32* dstSize) {
    FILE *f = fopen(path, "rb");
    cassert(f != NULL);

    fseek(f, 0, SEEK_END);
    int file_size = ftell(f);

    if (dst == NULL) {
        *dstSize = file_size;
        fclose(f);
        return;
    }

    rewind(f);

    fread(dst, 1, file_size, f);

    fclose(f);
}

void luna_VK_StageImageTransfer( VkImage dst, const void *data, int width, int height)
{
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(device, dst, &mem_req);

    const VkBufferCreateInfo stagingBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = mem_req.size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    vkCreateBuffer(device, &stagingBufferInfo, NULL, &stagingBuffer);

    VkMemoryRequirements stagingBufferRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferRequirements);

    const VkMemoryAllocateInfo stagingBufferAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = stagingBufferRequirements.size,
        .memoryTypeIndex = luna_VK_GetMemType(stagingBufferRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    vkAllocateMemory(device, &stagingBufferAllocInfo, NULL, &stagingBufferMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void *stagingBufferMapped;
    cassert(vkMapMemory(device, stagingBufferMemory, 0, mem_req.size, 0, &stagingBufferMapped) == VK_SUCCESS);
    memcpy(stagingBufferMapped, data, mem_req.size);
    vkUnmapMemory(device, stagingBufferMemory);

    const VkCommandBuffer cmd = luna_VK_BeginCommandBuffer(device);

    luna_VK_TransitionTextureLayout(
        cmd, dst, 1, VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        0, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );

    VkBufferImageCopy region = {};
    region.imageExtent = (VkExtent3D){ width, height, 1 };
    region.imageOffset =(VkOffset3D){ 0, 0, 0 };
    region.bufferOffset = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.mipLevel = 0;
    vkCmdCopyBufferToImage(cmd, stagingBuffer, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    luna_VK_TransitionTextureLayout(
            cmd, dst, 1, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            0, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );

    luna_VK_EndCommandBuffer(cmd, TransferQueue, true);

    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);
}

void luna_VK_CreateTextureFromMemory( u8 *buffer, u32 width, u32 height, VkFormat format, u32 channels, VkImage *dst, VkDeviceMemory *dstMem)
{
    luna_VK_CreateTextureEmpty(width, height, format, VK_SAMPLE_COUNT_1_BIT, channels, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, NULL, dst, dstMem);
    luna_VK_StageImageTransfer(*dst, buffer, width, height);
}

void luna_VK_CreateTextureEmpty( u32 width, u32 height, VkFormat format, VkSampleCountFlagBits samples, u32 channels, VkImageUsageFlags usage, int *image_size, VkImage *dst, VkDeviceMemory *dstMem)
{
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usage;
    imageCreateInfo.samples = samples;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageCreateInfo, NULL, dst) != VK_SUCCESS)
        LOG_ERROR("Failed to create image");

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, *dst, &imageMemoryRequirements);

    if (image_size) {
        *image_size = imageMemoryRequirements.size;
    }

    // allow for preallocated memory.
    if (dstMem != NULL) {

        const u32 localDeviceMemoryIndex = luna_VK_GetMemType(imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = imageMemoryRequirements.size;
        allocInfo.memoryTypeIndex = localDeviceMemoryIndex;

        vkAllocateMemory(device, &allocInfo, NULL, dstMem);
        vkBindImageMemory(device, *dst, *dstMem, 0);
    }
}

u8* luna_VK_CreateTextureFromDisk( const char *path, u32 *width, u32 *height, VkFormat *channels, VkImage *dst, VkDeviceMemory *dstMem)
{
    luna_Image tex = luna_ImageLoad(path);

    cassert(tex.data != NULL);

    *width = tex.w;
    *height = tex.h;
    *channels = tex.fmt;

    luna_VK_CreateTextureFromMemory(tex.data, tex.w, tex.h, *channels, 4, dst, dstMem);
    return tex.data;
}

void luna_VK_TransitionTextureLayout( VkCommandBuffer cmd, VkImage image,
										 u32 mipLevels,
                                         VkImageAspectFlagBits aspect,
										 VkImageLayout oldLayout,
										 VkImageLayout newLayout,
										 VkAccessFlags srcAccessMask,
										 VkAccessFlags dstAccessMask,
										 VkPipelineStageFlags sourceStage,
										 VkPipelineStageFlags destinationStage)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

// void help::Images::LoadFromDisk(const char *path, u8 channels, u8 **dst, u32 *dstWidth, u32 *dstHeight)
// {
//     // {Chef's kiss}
//     ctex2D tex = luna_ImageLoad(path);
//     *dst = tex.data;
//     *dstWidth = tex.w;
//     *dstHeight = tex.h;
// }

bool luna_VK_GetSupportedFormat( VkPhysicalDevice physDevice, VkSurfaceKHR surface, VkFormat *dstFormat, VkColorSpaceKHR *dstColorSpace)
{
	CVK_REQUIRED_PTR(device);
	CVK_REQUIRED_PTR(physDevice);
	CVK_REQUIRED_PTR(surface);
	CVK_REQUIRED_PTR(dstFormat);
	CVK_REQUIRED_PTR(dstColorSpace);

    u32 formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, VK_NULL_HANDLE);
	cg_vector_t /* VkSurfaceFormatKHR */ surfaceFormats = cg_vector_init(sizeof(VkSurfaceFormatKHR), formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, (VkSurfaceFormatKHR *)cg_vector_data(&surfaceFormats));

	VkSurfaceFormatKHR selectedFormat = { VK_FORMAT_MAX_ENUM, VK_COLOR_SPACE_MAX_ENUM_KHR };

	const VkSurfaceFormatKHR desired_formats[] = {
		{ VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
		{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
	};

	for (u32 i = 0; i < formatCount; i++)
	{
		const VkSurfaceFormatKHR *surfaceFormat = (VkSurfaceFormatKHR *)cg_vector_get(&surfaceFormats, i);
		for (u32 j = 0; j < array_len(desired_formats); j++) {
			if (surfaceFormat->format == desired_formats[j].format 
				&&
				surfaceFormat->colorSpace == desired_formats[j].colorSpace
			   ) {
					selectedFormat.format = surfaceFormat->format;
					selectedFormat.colorSpace = surfaceFormat->colorSpace;
			   }
		}
	}

    cg_vector_destroy(&surfaceFormats);
	if (selectedFormat.format == VK_FORMAT_MAX_ENUM || selectedFormat.colorSpace == VK_COLOR_SPACE_MAX_ENUM_KHR) {
		return VK_FALSE;
	} else {
		*dstFormat = selectedFormat.format;
		*dstColorSpace = selectedFormat.colorSpace;
		
		return VK_TRUE;
	}

	return VK_FALSE;
}

u32 luna_VK_GetSurfaceImageCount(VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{
	CVK_REQUIRED_PTR(physDevice);
	CVK_REQUIRED_PTR(surface);

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &surfaceCapabilities);

	u32 requestedImageCount = surfaceCapabilities.minImageCount + 1;
	if(requestedImageCount < surfaceCapabilities.maxImageCount)
		requestedImageCount = surfaceCapabilities.maxImageCount;

	return requestedImageCount;
}
