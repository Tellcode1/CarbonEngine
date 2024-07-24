#include "Bootstrap.hpp"

// VkInstance bootstrap::CreateInstance(const char* windowTitle) {
//     VkApplicationInfo appInfo{};
// 	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
// 	appInfo.applicationVersion = VK_API_VERSION_1_0;
// 	appInfo.apiVersion = VK_API_VERSION_1_0;
// 	appInfo.engineVersion = 0;
// 	appInfo.pApplicationName = windowTitle;
// 	appInfo.pEngineName = "No Engine";

// 	uint32_t SDLExtensionCount = 0;
// 	const char* const* SDLExtensions = SDL_Vulkan_GetInstanceExtensions(&SDLExtensionCount);
// 	std::vector<const char*> Extensions;
	
// 	for (const auto& ext : InstanceExtensions)
// 		Extensions.push_back(ext);

// 	for (uint32_t i = 0; i < SDLExtensionCount; i++) 
// 		Extensions.push_back(SDLExtensions[i]);

// 	VkInstanceCreateInfo instanceCreateinfo{};
// 	instanceCreateinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
// 	instanceCreateinfo.pApplicationInfo = &appInfo;
// 	instanceCreateinfo.enabledExtensionCount = static_cast<uint32_t>(Extensions.size());
// 	instanceCreateinfo.ppEnabledExtensionNames = Extensions.data();

// 	#ifdef DEBUG

// 	bool validationLayersAvailable = false;
// 	uint32_t layerCount = 0;

// 	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
// 	VkLayerProperties layerProperties[layerCount];
// 	vkEnumerateInstanceLayerProperties(&layerCount, layerProperties);

// 	for(const auto& layer : ValidationLayers)
// 	{
// 		for (uint32_t i = 0; i < layerCount; i++)
// 		{
// 			if (strcmp(layer, layerProperties[i].layerName) == 0)
// 			{
// 				validationLayersAvailable = true;
// 			}
// 		}
// 	}

// 	if (!validationLayersAvailable)
// 	{
// 		printf("VALIDTATION LAYERS COULD NOT BE LOADED\nRequested layers:\n");
// 		for(const char* layer : ValidationLayers)
// 		{
// 			printf("\t%s\n", layer);
// 		}
// 		printf("Instance provided these layers (i.e. These layers are available):\n");
// 		for(uint32_t i = 0; i < layerCount; i++)
// 		{
// 			printf("\t%s\n", layerProperties[i].layerName);
// 		}
// 		printf("But instance asked for (i.e. are not available):\n");

// 		std::vector<const char*> missingLayers;
	
// 		for(const auto& layer : ValidationLayers)
// 		{
// 			bool layerAvailable = false;
// 			for (uint32_t i = 0; i < layerCount; i++)
// 			{
// 				if (strcmp(layer, layerProperties[i].layerName) == 0)
// 				{
// 					layerAvailable = true;
// 					break;
// 				}
// 			}
// 			if(!layerAvailable) missingLayers.push_back(layer);
// 		}
// 		for(const auto& layer : missingLayers)
// 		{
// 			printf("\t%s\n", layer);
// 		}
		
// 		exit(VK_ERROR_LAYER_NOT_PRESENT);
// 	}

// 	instanceCreateinfo.enabledLayerCount = ValidationLayers.size();
// 	instanceCreateinfo.ppEnabledLayerNames = ValidationLayers.data();

// 	#else

// 	instanceCreateinfo.enabledLayerCount = 0;
// 	instanceCreateinfo.ppEnabledLayerNames = nullptr;

// 	#endif

//     VkInstance instance;
// 	vkCreateInstance(&instanceCreateinfo, nullptr, &instance);

//     return instance;
// }

VkPhysicalDevice bootstrap::GetSelectedPhysicalDevice(const VkInstance& instance, const VkSurfaceKHR& surface) {

    VkPhysicalDevice chosenDevice = VK_NULL_HANDLE;
	uint32_t physDeviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, nullptr);
	
	if(physDeviceCount == 0) {
		printf("No physical devices found\n");
		abort();
	}

	VkPhysicalDevice physicalDevices[physDeviceCount];
	vkEnumeratePhysicalDevices(instance, &physDeviceCount, physicalDevices);

	for(const auto& device : physicalDevices) {
		// VkSurfaceCapabilitiesKHR capabilities{};

		// vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		VkSurfaceFormatKHR formats[formatCount];
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats);

		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		VkPresentModeKHR presentModes[presentModeCount];
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModes);

		uint32_t queueCount = 0;

		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);
		VkQueueFamilyProperties queueFamilies[queueCount];
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, queueFamilies);
		
		bool extensionsAvailable = true;

		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		VkExtensionProperties availableExtensions[extensionCount];
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions);

		for (const auto& extension : RequiredDeviceExtensions) {
			bool validated = false;
			for(const auto& availableExt : availableExtensions) {
				if (strcmp(extension, availableExt.extensionName) == 0) {
					validated = true;
				}
			}
			if (!validated) {
				printf("Failed to validate extension with name: %s\n", extension);
				extensionsAvailable = false;
			}
		}
		
		bool foundGraphicsFamily = false, foundPresentFamily = false, foundComputeFamily = false;
		
		uint32_t i = 0;
		for (const auto& queueFamily : queueFamilies) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				foundGraphicsFamily = true;
			}
			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
				foundComputeFamily = true;
			}
			if (presentSupport) {
				foundPresentFamily = true;
			}
			if (foundGraphicsFamily && foundComputeFamily && foundPresentFamily)
				break;

			i++;
		}

		if (((foundGraphicsFamily && foundComputeFamily && foundPresentFamily)) && extensionsAvailable && (formatCount > 0 && presentModeCount > 0)) { // Not sure how the last checks are supposed to be helpful.
			chosenDevice = device;
			break;
		}
	}

	if(chosenDevice == VK_NULL_HANDLE) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevices[0], &properties);

		printf("Could not find an appropriate device. Falling back to device 0: %s\n", properties.deviceName);
		chosenDevice = physicalDevices[0];
	}

	return chosenDevice;
}

u32 bootstrap::GetQueueIndex(VkPhysicalDevice physDevice, QueueType type)
{
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilies.data());

    u32 queueIndex = 0;
    for (const auto& family : queueFamilies) {
        if (family.queueFlags & type) {
            return queueIndex;
        }
        queueIndex++;
    }

	return queueIndex;
}

VkCommandPool bootstrap::CreateCommandPool(VkDevice device, VkPhysicalDevice physDevice, const QueueType queueType)
{
	VkCommandPool commandPool;

    VkCommandPoolCreateInfo cmdPoolCreateInfo{};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCreateInfo.queueFamilyIndex = bootstrap::GetQueueIndex(physDevice, bootstrap::QueueType::GRAPHICS);
    cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
	if(vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool");
	}

	return commandPool;
}

bootstrap::Image bootstrap::CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties)
{
	Image retval;

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
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

	vkCreateImage(device, &imageCreateInfo, nullptr, &retval.image);

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, retval.image, &memRequirements);

	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memRequirements.size;
	memAllocInfo.memoryTypeIndex = bootstrap::GetMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	vkAllocateMemory(device, &memAllocInfo, nullptr, &retval.memory);

	vkBindImageMemory(device, retval.image, retval.memory, 0);

	retval.view = bootstrap::CreateImageView(device, retval.image, format, VK_IMAGE_ASPECT_COLOR_BIT);

	return retval;
}

void bootstrap::TransitionImageLayout(VkDevice device, VkCommandPool pool, VkQueue queue, Image &img, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	const VkCommandBuffer cmd = BeginSingleTimeCommands(device, pool);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = img.image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier );

	EndSingleTimeCommands(device, cmd, queue, pool);
}

void bootstrap::TransitionImageLayout(VkDevice device, VkCommandPool pool, VkQueue queue, VkImage img, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	const VkCommandBuffer cmd = BeginSingleTimeCommands(device, pool);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = img;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	} else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier );

	EndSingleTimeCommands(device, cmd, queue, pool);
}

uint32_t bootstrap::GetMemoryType(const VkPhysicalDevice &physicalDevice, const uint32_t MemoryTypeBits, const VkMemoryPropertyFlags MemoryProperties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) 
	{
    	if ((MemoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & MemoryProperties) == MemoryProperties) {
        	return i;
    	}
	}

	return -1;
}

VkCommandBuffer bootstrap::BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	if(vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffer");
	}

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void bootstrap::EndSingleTimeCommands(VkDevice device, VkCommandBuffer buffer, VkQueue queue, VkCommandPool commandPool)
{
	vkEndCommandBuffer(buffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;
	VkFence fence;
	vkCreateFence(device, &fenceInfo, nullptr, &fence);

    if(vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
		std::cerr << "Single time command failed to submit. Program should not continue.\n";
	}
	vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);

	vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, commandPool, 1, &buffer);
}

void bootstrap::CopyBufferToImage(VkBuffer buffer, VkDevice device, VkCommandPool pool, VkQueue queue, VkImage image, uint32_t width, uint32_t height)
{
	const VkCommandBuffer cmd = BeginSingleTimeCommands(device, pool);
	
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = { width, height, 1};

	vkCmdCopyBufferToImage(cmd, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	EndSingleTimeCommands(device, cmd, queue, pool);
}

VkBuffer bootstrap::CreateBuffer(VkPhysicalDevice &physicalDevice, VkDevice &device, VkDeviceMemory &bufferMemory, VkDeviceSize size, const VkBufferUsageFlags usageFlags, const VkMemoryPropertyFlags propertyFlags)
{
	VkBuffer buffer;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = GetMemoryType(physicalDevice, memRequirements.memoryTypeBits, propertyFlags);

    vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
    
	return buffer;
}

void bootstrap::StageBufferTransfer(VkPhysicalDevice physicalDevice, VkDevice device, VkCommandPool commandPool, VkQueue queue, VkBuffer dstBuffer, void *data, size_t size)
{
	VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    stagingBuffer = bootstrap::CreateBuffer(
        ctx::physDevice, ctx::device, stagingBufferMemory, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

	void* mapped;
	vkMapMemory(ctx::device, stagingBufferMemory, 0, size, 0, &mapped);
	memcpy(mapped, data, size);
	vkUnmapMemory(ctx::device, stagingBufferMemory);

	VkCommandBuffer cmd = BeginSingleTimeCommands(device, commandPool);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(cmd, stagingBuffer, dstBuffer, 1, &copyRegion);
	
	EndSingleTimeCommands(device, cmd, queue, commandPool);
}

VkImageView bootstrap::CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = format;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	
	VkImageView view;
	vkCreateImageView(device, &imageViewCreateInfo, nullptr, &view);
	
	return view;
}
