#include "Image.hpp"
#include "Renderer.hpp"
#include "Context.hpp"
#include "epichelperlib.hpp"

static void* tempAlloc(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) {
    return pUserData;
}

static void* tempRealloc(void *pUserData, void *pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) {
    return pUserData;
}

static void tempFree(void *pUserData, void *pMemory) {}

static u8* LoadDiskImage(const char* path, Image* dstImage)
{
    i32 width, height, channels;
    u8* data = stbi_load(path, &width, &height, &channels, 4);
    dstImage->width = static_cast<u32>(width);
    dstImage->height = static_cast<u32>(height);
    return data;
}

ImageLoadStatus ImagesSingleton::LoadImage(ImageLoadInfo *pInfo, Image *dstImage, ImageLoadType loadType)
{
    u8* imageData;
    
    if(loadType == IMAGE_LOAD_TYPE_FROM_DISK)
        imageData = LoadDiskImage(pInfo->pPath, dstImage);
    else
        imageData = pInfo->data;
    
    u32 width, height;
    if(loadType == IMAGE_LOAD_TYPE_FROM_DISK) {
        width = dstImage->width;
        height = dstImage->height;
    } else {
        width = pInfo->width;
        height = pInfo->height;
    }

    const VkDevice device = ctx->device;
    const VkPhysicalDevice physDevice = ctx->physDevice;

    if((pInfo->samples & (pInfo->samples - 1)) != 0)
    {
        printf("pInfo->samples must be a valid VkSampleCountFlagBits value\n");
        return IMAGE_LOAD_INVALID_VALUE;
    }

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = pInfo->format;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.samples = pInfo->samples;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageCreateInfo, nullptr, &dstImage->image) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image");
    }

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, dstImage->image, &imageMemoryRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = imageMemoryRequirements.size;
    allocInfo.memoryTypeIndex = pro::GetMemoryType(physDevice, imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if(vkAllocateMemory(device, &allocInfo, nullptr, &dstImage->memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate memory");
    }
    vkBindImageMemory(device, dstImage->image, dstImage->memory, 0);

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
        throw std::runtime_error("Failed to allocate memory");
    }

    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void *stagingBufferMapped;
    if (vkMapMemory(device, stagingBufferMemory, 0, width * height, 0, &stagingBufferMapped) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to map staging buffer memory!");
    }
    memcpy(stagingBufferMapped, imageData, width * height);
    vkUnmapMemory(device, stagingBufferMemory);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.format = pInfo->format;
    viewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.image = dstImage->image;
    if(vkCreateImageView(device, &viewInfo, nullptr, &dstImage->view) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view");
    }

    // Image layout transition : UNDEFINED -> TRANSFER_DST_OPTIMAL
    const VkCommandBuffer cmd = help::Commands::BeginSingleTimeCommands();

    VkImageMemoryBarrier preCopyBarrier{};
    preCopyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    preCopyBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    preCopyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    preCopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preCopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    preCopyBarrier.image = dstImage->image;
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
    vkCmdCopyBufferToImage(cmd, stagingBuffer, dstImage->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    // Image layout transition : TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    VkImageMemoryBarrier postCopyBarrier{};
    postCopyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    postCopyBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    postCopyBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    postCopyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postCopyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    postCopyBarrier.image = dstImage->image;
    postCopyBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    postCopyBarrier.subresourceRange.baseMipLevel = 0;
    postCopyBarrier.subresourceRange.levelCount = 1;
    postCopyBarrier.subresourceRange.baseArrayLayer = 0;
    postCopyBarrier.subresourceRange.layerCount = 1;
    postCopyBarrier.srcAccessMask = 0;
    postCopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &postCopyBarrier);

    help::Commands::EndSingleTimeCommands(cmd);

    vkDestroyBuffer(device, stagingBuffer, &temporaryAllocator);
    vkFreeMemory(device, stagingBufferMemory, &temporaryAllocator);

    return IMAGE_LOAD_SUCCESS;
}