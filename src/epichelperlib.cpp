#include "epichelperlib.hpp"
#include <boost/filesystem.hpp>

// stb loves to kick compilers in the liver so it shouldn't be put in the precompiled header
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

void help::Buffers::CreateBuffer(u64 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer *dstBuffer, VkDeviceMemory *retMem, bool externallyAllocated)
{
    if(size == 0) {
        #ifndef __EPIC_REMOVE_WARNINGS
        std::cout << "Warning: Size specified as 0. Invalid value\n";
        #endif
        
        return;
    }

    VkBuffer newBuffer;
    VkDeviceMemory newMemory;

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &newBuffer) != VK_SUCCESS)
    {
        printf("Renderer::failed to create staging buffer!");
    }

    VkMemoryRequirements bufferMemoryRequirements;
    vkGetBufferMemoryRequirements(device, newBuffer, &bufferMemoryRequirements);
    
    if(!externallyAllocated)
    {
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = bufferMemoryRequirements.size;
        allocInfo.memoryTypeIndex = GetMemoryType(bufferMemoryRequirements.memoryTypeBits, propertyFlags);
        if (vkAllocateMemory(device, &allocInfo, nullptr, &newMemory) != VK_SUCCESS)
        {
            printf("Renderer::failed to allocate staging buffer memory!");
        }
        
        vkBindBufferMemory(device, newBuffer, newMemory, 0);
        *retMem = newMemory;
    }

    *dstBuffer = newBuffer;
}

void help::Buffers::StageBufferTransfer(VkBuffer dst, void *data, u64 size)
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkBufferCreateInfo stagingBufferInfo = {};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = size;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &stagingBufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
    {
        printf("Failed to create staging buffer!");
    }

    VkMemoryRequirements stagingBufferMemoryRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferMemoryRequirements);
    
    VkMemoryAllocateInfo stagingBufferAlloc = {};
    stagingBufferAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingBufferAlloc.allocationSize = stagingBufferMemoryRequirements.size;
    stagingBufferAlloc.memoryTypeIndex = GetMemoryType(stagingBufferMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device, &stagingBufferAlloc, nullptr, &stagingBufferMemory) != VK_SUCCESS)
    {
        printf("Failed to allocate staging buffer memory!");
    }
    
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void* mapped;
    vkMapMemory(device, stagingBufferMemory, 0, size, 0, &mapped);
    memcpy(mapped, data, size);
    vkUnmapMemory(device, stagingBufferMemory);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void help::Memory::FlushMappedMemory(VkDeviceMemory memory, u64 size,u64 offset)
{
    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    vkFlushMappedMemoryRanges(device, 1, &mappedRange);
}

u32 help::GetMemoryType(const u32 memoryTypeBits, const VkMemoryPropertyFlags memoryProperties)
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

VkCommandBuffer help::Commands::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = Renderer::commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	if(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
		printf("Failed to allocate command buffer");
	}

    return BeginSingleTimeCommands(commandBuffer);
}

VkCommandBuffer help::Commands::BeginSingleTimeCommands(VkCommandBuffer src)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(src, &beginInfo);

    return src;
}

VkResult help::Commands::EndSingleTimeCommands(VkCommandBuffer cmd, VkQueue queue, bool waitForExecution)
{
    VkResult res = vkEndCommandBuffer(cmd);
    if(res != VK_SUCCESS) return res;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;
	VkFence fence = VK_NULL_HANDLE;
	
    if(waitForExecution) {
        res = vkCreateFence(device, &fenceInfo, nullptr, &fence);
        if(res != VK_SUCCESS) return res;
    }

    res = vkQueueSubmit(queue, 1, &submitInfo, fence);
    if(res != VK_SUCCESS) return res;

	if (waitForExecution) {
        vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	    vkDestroyFence(device, fence, nullptr);
        vkFreeCommandBuffers(device, Renderer::commandPool, 1, &cmd);
    }

    return VK_SUCCESS;
}

void help::Files::LoadBinary(const char* path, u8* dst, u32* dstSize) {
    if(!dst) {
        *dstSize = (u32)boost::filesystem::file_size(path);
        return;
    }
    std::ifstream instream(path, std::ios::ate | std::ios::binary);
    if (!instream.is_open()) {
        printf("Failed to open file at path %s\n", path);
        *dstSize = 0;
        instream.close(); // I think instream automatically closes when it goes out of scope but I don't want to find out
        return;
    }

    *dstSize = (u32)instream.tellg();
    instream.seekg(0);
    instream.read(reinterpret_cast<char*>(dst), *dstSize);

    instream.close();
}

// See epichelperlib.hpp
// void help::Images::LoadVulkanImage(u8 *buffer,
//                                   u32 width, u32 height,
//                                   VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
//                                   VkMemoryPropertyFlags properties, VkSampleCountFlagBits samples, u32 mipLevels,
//                                   VkImage *dstImage, VkDeviceMemory *dstMemory, bool externallyAllocated)
// {
//     VkImage retval;
//     VkDeviceMemory retMem = *dstMemory;

//     VkImageCreateInfo imageCreateInfo{};
//     imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//     imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
//     imageCreateInfo.extent = VkExtent3D{ width, height, 1 };
//     imageCreateInfo.mipLevels = mipLevels;
//     imageCreateInfo.arrayLayers = 1;
//     imageCreateInfo.format = format;
//     imageCreateInfo.tiling = tiling;
//     imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//     imageCreateInfo.usage = usage;
//     imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//     imageCreateInfo.samples = samples;
//     if (vkCreateImage(device, &imageCreateInfo, nullptr, &retval) != VK_SUCCESS)
//         printf("Failed to create image");

//     VkMemoryRequirements imageMemoryRequirements;
//     vkGetImageMemoryRequirements(device, retval, &imageMemoryRequirements);

//     if (!externallyAllocated)
//     {
//         VkMemoryAllocateInfo allocInfo{};
//         allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//         allocInfo.allocationSize = imageMemoryRequirements.size;
//         allocInfo.memoryTypeIndex = pro::GetMemoryType(physDevice, imageMemoryRequirements.memoryTypeBits, properties);
//         if(vkAllocateMemory(device, &allocInfo, nullptr, &retMem) != VK_SUCCESS) {
//             printf("Failed to allocate memory");
//         }
//         vkBindImageMemory(device, retval, retMem, 0);
//     }
    
//     VkBuffer stagingBuffer = VK_NULL_HANDLE;
//     VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

//     uchar stagingBufferBacking[512];

//     VkAllocationCallbacks temporaryAllocator{};
//     temporaryAllocator.pfnAllocation = tempAlloc;
//     temporaryAllocator.pfnReallocation = tempRealloc;
//     temporaryAllocator.pfnFree = tempFree;
//     temporaryAllocator.pUserData = reinterpret_cast<void*>(stagingBufferBacking);

//     VkBufferCreateInfo stagingBufferInfo{};
//     stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//     stagingBufferInfo.size = imageMemoryRequirements.size;
//     stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
//     stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//     vkCreateBuffer(device, &stagingBufferInfo, &temporaryAllocator, &stagingBuffer);

//     VkMemoryRequirements stagingBufferRequirements;
//     vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferRequirements);

//     VkMemoryAllocateInfo stagingBufferAllocInfo{};
//     stagingBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//     stagingBufferAllocInfo.allocationSize = stagingBufferRequirements.size;
//     stagingBufferAllocInfo.memoryTypeIndex = pro::GetMemoryType(physDevice, stagingBufferRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

//     if(vkAllocateMemory(device, &stagingBufferAllocInfo, &temporaryAllocator, &stagingBufferMemory) != VK_SUCCESS) {
//         printf("Failed to allocate memory");
//     }

//     vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

//     void *stagingBufferMapped;
//     if (vkMapMemory(device, stagingBufferMemory, 0, width * height, 0, &stagingBufferMapped) != VK_SUCCESS)
//     {
//         printf("failed to map staging buffer memory!");
//     }
//     memcpy(stagingBufferMapped, buffer, width * height);
//     vkUnmapMemory(device, stagingBufferMemory);                                                
    
//     // Image layout transition : UNDEFINED -> TRANSFER_DST_OPTIMAL
//     const VkCommandBuffer cmd = help::Commands::BeginSingleTimeCommands();

//     TransitionImageLayout(
//         cmd, retval, mipLevels,
//         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//         0, VK_ACCESS_TRANSFER_WRITE_BIT,
//         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
//     );

//     VkBufferImageCopy region{};
//     region.imageExtent = { width, height, 1 };
//     region.imageOffset = { 0, 0, 0 };
//     region.bufferOffset = 0;
//     region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//     region.imageSubresource.layerCount = 1;
//     region.imageSubresource.baseArrayLayer = 0;
//     region.imageSubresource.mipLevel = 0;
//     vkCmdCopyBufferToImage(cmd, stagingBuffer, retval, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

//     if(mipLevels == 1) {
//         TransitionImageLayout(
//             cmd, retval, mipLevels,
//             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//             0, VK_ACCESS_TRANSFER_WRITE_BIT,
//             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
//         );
//     }

//     help::Commands::EndSingleTimeCommands(cmd);

//     vkDestroyBuffer(device, stagingBuffer, &temporaryAllocator);
//     vkFreeMemory(device, stagingBufferMemory, &temporaryAllocator);

//     *dstImage = retval;
// }

void help::Images::StageImageTransfer(VkImage dst, void *data, u32 width, u32 height, u32 channels)
{
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = width * height * channels;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(device, &stagingBufferInfo, nullptr, &stagingBuffer);

    VkMemoryRequirements stagingBufferRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferRequirements);

    VkMemoryAllocateInfo stagingBufferAllocInfo{};
    stagingBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingBufferAllocInfo.allocationSize = stagingBufferRequirements.size;
    stagingBufferAllocInfo.memoryTypeIndex = pro::GetMemoryType(physDevice, stagingBufferRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(device, &stagingBufferAllocInfo, nullptr, &stagingBufferMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void *stagingBufferMapped;
    if (vkMapMemory(device, stagingBufferMemory, 0, width * height * channels, 0, &stagingBufferMapped) != VK_SUCCESS)
        LOG_ERROR("Failed to map staging buffer memory!");
    memcpy(stagingBufferMapped, data, width * height * channels);
    vkUnmapMemory(device, stagingBufferMemory);

    const VkCommandBuffer cmd = help::Commands::BeginSingleTimeCommands();

    help::Images::TransitionImageLayout(
        cmd, dst, 1,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        0, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );

    VkBufferImageCopy region{};
    region.imageExtent = { width, height, 1 };
    region.imageOffset = { 0, 0, 0 };
    region.bufferOffset = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.mipLevel = 0;
    vkCmdCopyBufferToImage(cmd, stagingBuffer, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    help::Commands::EndSingleTimeCommands(cmd, vctx::TransferQueue);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void help::Images::killme(u8 *buffer, u32 width, u32 height, VkFormat format, u32 channels, VkImage *dst, VkDeviceMemory *dstMem)
{
    VkImageCreateInfo imageCreateInfo{};
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
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageCreateInfo, nullptr, dst) != VK_SUCCESS)
        LOG_ERROR("Failed to create image");

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, *dst, &imageMemoryRequirements);

    const u32 localDeviceMemoryIndex = pro::GetMemoryType(physDevice, imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = imageMemoryRequirements.size;
    allocInfo.memoryTypeIndex = localDeviceMemoryIndex;

    vkAllocateMemory(device, &allocInfo, nullptr, dstMem);
    vkBindImageMemory(device, *dst, *dstMem, 0);

    StageImageTransfer(*dst, buffer, width, height, channels);
}

u8* help::Images::killme(const char *path, u32 *width, u32 *height, VkFormat *channels, VkImage *dst, VkDeviceMemory *dstMem)
{
    i32 w, h, c;
    u8 *buffer = stbi_load(path, &w, &h, &c, 3);

    *width = w;
    *height = h;

    VkFormat fmt = VK_FORMAT_UNDEFINED;
    if (c == 1)
        fmt = VK_FORMAT_R8_SRGB;
    else if (c == 2)
        fmt = VK_FORMAT_R8G8_SRGB;
    else if (c == 3)
        fmt = VK_FORMAT_R8G8B8_SRGB;
    else if (c == 4)
        fmt = VK_FORMAT_R8G8B8A8_SRGB;
    else
        LOG_ERROR("Channels requested %i (\"%s\")", c, path);

    *channels = fmt;

    killme(buffer, w, h, fmt, c, dst, dstMem);
    return buffer;
}

void help::Images::TransitionImageLayout(VkCommandBuffer cmd, VkImage image,
										 u32 mipLevels,
										 VkImageLayout oldLayout,
										 VkImageLayout newLayout,
										 VkAccessFlags srcAccessMask,
										 VkAccessFlags dstAccessMask,
										 VkPipelineStageFlags sourceStage,
										 VkPipelineStageFlags destinationStage)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    vkCmdPipelineBarrier(cmd, sourceStage, dstAccessMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}
// void help::Images::LoadFromDisk(const char *path, u32 channels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkSampleCountFlagBits samples, u32 mipMapLevels, VkImage *dstImage, VkDeviceMemory *dstMemory, bool externallyAllocated)
// {
//     stbi_set_flip_vertically_on_load(false);
//     i32 width, height, gotchannels;
//     u8* buffer = stbi_load(path, &width, &height, &gotchannels, channels);
//     if ((u32)gotchannels != channels)
//         LOG_WARNING("Requested %u channels but got %i for image on path %s", channels, gotchannels, path);
//     LoadVulkanImage(buffer, width, height, format, tiling, usage, properties, samples, mipMapLevels, dstImage, dstMemory, externallyAllocated);
// }

void help::Images::LoadFromDisk(const char *path, u8 channels, u8 **dst, u32 *dstWidth, u32 *dstHeight)
{
    i32 width, height;
    u8* buffer = stbi_load(path, &width, &height, nullptr, channels);
    *dst = buffer;
    *dstWidth = width;
    *dstHeight = height;
}