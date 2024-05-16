#include "epichelperlib.hpp"

static void* tempAlloc(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) {
    return pUserData;
}

static void* tempRealloc(void *pUserData, void *pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) {
    return pUserData;
}

static void tempFree(void *pUserData, void *pMemory) {}

void help::Buffers::CreateBuffer(u64 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer *dstBuffer, VkDeviceMemory *dstMemory, bool externallyAllocated)
{
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
        *dstMemory = newMemory;
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

void help::Memory::MapMemory(VkDeviceMemory bufferMemory, u64 size, u64 offset, void **dstPtr)
{
    vkMapMemory(device, bufferMemory, offset, size, 0, dstPtr);
}

void help::Memory::UnmapMemory(VkDeviceMemory bufferMemory)
{
    vkUnmapMemory(device, bufferMemory);
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
    allocInfo.commandPool = Renderer->commandPool;
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

VkResult help::Commands::EndSingleTimeCommands(VkCommandBuffer cmd, bool waitForExecution)
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

    res = vkQueueSubmit(Graphics->GraphicsQueue, 1, &submitInfo, fence);
    if(res != VK_SUCCESS) return res;

	if (waitForExecution) {
        vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	    vkDestroyFence(device, fence, nullptr);
        vkFreeCommandBuffers(device, Renderer->commandPool, 1, &cmd);
    }

    return VK_SUCCESS;
}

void help::Files::LoadBinary(const char* path, u8* dst, u32* dstSize) {
    std::ifstream instream(path, std::ios::ate | std::ios::binary);
    if (!instream.is_open()) {
        printf("Failed to open file at path %s\n", path);
        *dstSize = 0; // Signal error by setting size to 0
        return;
    }

    *dstSize = (u32)instream.tellg();
    if (dst) {
        instream.seekg(0);
        instream.read(reinterpret_cast<char*>(dst), *dstSize);
    }
}

void help::Images::LoadFromDisk(const char *path, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *dstImage, VkDeviceMemory *dstMemory, bool externallyAllocated)
{
    i32 _width, _height, channels;
    const u8* data = stbi_load(path, &_width, &_height, &channels, 4);
    const u32 width = static_cast<u32>(_width);
    const u32 height = static_cast<u32>(_height);
    const u32 imageSize = width * height * channels;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = tiling;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usage;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageCreateInfo, nullptr, dstImage) != VK_SUCCESS)
    {
        printf("Failed to create image");
    }

    if (!externallyAllocated)
    {
        VkMemoryRequirements imageMemoryRequirements;
        vkGetImageMemoryRequirements(device, *dstImage, &imageMemoryRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = imageMemoryRequirements.size;
        allocInfo.memoryTypeIndex = pro::GetMemoryType(physDevice, imageMemoryRequirements.memoryTypeBits, properties);
        if(vkAllocateMemory(device, &allocInfo, nullptr, dstMemory) != VK_SUCCESS) {
            printf("Failed to allocate memory");
        }
        vkBindImageMemory(device, *dstImage, *dstMemory, 0);

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

        uchar stagingBufferBacking[512];

        VkAllocationCallbacks temporaryAllocator{};
        temporaryAllocator.pfnAllocation = tempAlloc;
        temporaryAllocator.pfnReallocation = tempRealloc;
        temporaryAllocator.pfnFree = tempFree;
        temporaryAllocator.pUserData = reinterpret_cast<void*>(stagingBufferBacking);

        VkBufferCreateInfo stagingBufferInfo{};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = imageMemoryRequirements.size;
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(device, &stagingBufferInfo, &temporaryAllocator, &stagingBuffer);

        VkMemoryRequirements stagingBufferRequirements;
        vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferRequirements);

        VkMemoryAllocateInfo stagingBufferAllocInfo{};
        stagingBufferAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        stagingBufferAllocInfo.allocationSize = stagingBufferRequirements.size;
        stagingBufferAllocInfo.memoryTypeIndex = pro::GetMemoryType(physDevice, stagingBufferRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if(vkAllocateMemory(device, &stagingBufferAllocInfo, &temporaryAllocator, &stagingBufferMemory) != VK_SUCCESS) {
            printf("Failed to allocate memory");
        }

        vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

        void *stagingBufferMapped;
        if (vkMapMemory(device, stagingBufferMemory, 0, width * height, 0, &stagingBufferMapped) != VK_SUCCESS)
        {
            printf("failed to map staging buffer memory!");
        }
        memcpy(stagingBufferMapped, data, width * height);
        vkUnmapMemory(device, stagingBufferMemory);                                                
    }
    
    // Image layout transition : UNDEFINED -> TRANSFER_DST_OPTIMAL
    const VkCommandBuffer cmd = help::Commands::BeginSingleTimeCommands();

    VkImageMemoryBarrier preCopyBarrier{};
    preCopyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    preCopyBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    preCopyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    preCopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preCopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preCopyBarrier.image = *dstImage;
    preCopyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    preCopyBarrier.subresourceRange.baseMipLevel = 0;
    preCopyBarrier.subresourceRange.levelCount = 1;
    preCopyBarrier.subresourceRange.baseArrayLayer = 0;
    preCopyBarrier.subresourceRange.layerCount = 1;
    preCopyBarrier.srcAccessMask = 0;
    preCopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &preCopyBarrier);

    VkBufferImageCopy region{};
    region.imageExtent = { width, height, 1 };
    region.imageOffset = { 0, 0, 0 };
    region.bufferOffset = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.mipLevel = 0;
    // vkCmdCopyBufferToImage(cmd, stagingBuffer, *dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Image layout transition : TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    VkImageMemoryBarrier postCopyBarrier{};
    postCopyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    postCopyBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    postCopyBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    postCopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postCopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postCopyBarrier.image = *dstImage;
    postCopyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    postCopyBarrier.subresourceRange.baseMipLevel = 0;
    postCopyBarrier.subresourceRange.levelCount = 1;
    postCopyBarrier.subresourceRange.baseArrayLayer = 0;
    postCopyBarrier.subresourceRange.layerCount = 1;
    postCopyBarrier.srcAccessMask = 0;
    postCopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &postCopyBarrier);

    help::Commands::EndSingleTimeCommands(cmd);

    // vkDestroyBuffer(device, stagingBuffer, &temporaryAllocator);
    // vkFreeMemory(device, stagingBufferMemory, &temporaryAllocator);
}
