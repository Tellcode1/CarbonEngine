#include "Renderer.hpp"
#include "Atlas.hpp"
#include "pro.hpp"

#define CACHE_VKFUNCTION(instance, func) static const PFN_##func func = reinterpret_cast<PFN_##func>(vkGetInstanceProcAddr(instance, #func))

constexpr u32 PipelineFlags = PIPELINE_CREATE_FLAGS_ENABLE_BLEND;

const SafeArray<VkVertexInputAttributeDescription> pAttributeDescriptions{ 
    // location, binding, format, offset,
    {  0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 },
};
    
const SafeArray<VkVertexInputBindingDescription> pBindingDescription{ 
    // binding, stride, inputRate,
    { 0, sizeof(vec2) * 2 /* 2 vec2's for position and 2 for texture positions */, VK_VERTEX_INPUT_RATE_VERTEX },
};
    
const SafeArray<VkPushConstantRange> pPushConstants{ 
    //stageFlags, offset, size,
    { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4) },
};

Renderer::Renderer(VulkanContext* ctx) {
    this->ctx = ctx;

    if (!pro::GetSupportedFormat(ctx->device, ctx->physDevice, ctx->surface, &format, &colorSpace)) {
        throw std::runtime_error("Failed to find a supported format!");
    }
    imageCount = pro::GetImageCount(ctx->physDevice, ctx->surface);

    pro::SwapchainCreateInfo scio{};
    scio.extent = renderArea;
    scio.presentMode = VSyncEnabled ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;
    scio.physDevice = ctx->physDevice;
    scio.surface = ctx->surface;
    scio.format = format;
    scio.colorSpace = colorSpace;
    scio.requestedImageCount = imageCount;
    pro::CreateSwapChain(ctx->device, &scio, &swapchain);
    
    vkGetDeviceQueue(ctx->device, bootstrap::GetQueueIndex(ctx->physDevice, bootstrap::QueueType::GRAPHICS), 0, &graphicsQueue );

    commandPool = bootstrap::CreateCommandPool(ctx->device, ctx->physDevice, bootstrap::QueueType::GRAPHICS);

    images.resize(1);
    // images[0] = LoadImage("/home/debian/Downloads/texture.jpg");
    TIME_FUNCTION(images[0] = LoadImage("/home/debian/dev/Pixelworlds3.0/Atlas/atlas.png"));

    // // Initialize FreeType library
    // FT_Library ft;
    // if (FT_Init_FreeType(&ft)) {
    //     std::cerr << "Failed to initialize FreeType library" << std::endl;
    // }
    // const std::string fontFile = "/usr/share/fonts/truetype/malayalam/Dyuthi-Regular.ttf";
    // constexpr u32 fontSize = 16;

    // // Load font face
    // FT_Face face;
    // if (FT_New_Face(ft, fontFile.c_str(), 0, &face)) {
    //     std::cerr << "Failed to load font file: " << fontFile << std::endl;
    //     FT_Done_FreeType(ft);
    // }

    // // Set font size
    // FT_Set_Pixel_Sizes(face, 0, fontSize);

    // std::vector<Image> imageDatas;

    // for(uchar c = 0; c < 128; c++)
    // {
    //     if(FT_Load_Char(face, c, FT_LOAD_RENDER))
    //     {
    //         std::cerr << "ERROR::FREETYTPE: Failed to load Glyph at index " << (uint)c << std::endl;
    //         continue;
    //     }
 
    //     u8* imageData = face->glyph->bitmap.buffer;
 
    //     if (imageData == nullptr) {
    //         std::cerr << "Glyph bitmap data is null for character: " << (uint)c << std::endl;
    //         continue;;
    //     }

    //     imageDatas.push_back(
    //         Image
    //         {
    //             std::to_string(c),
    //             static_cast<i32>(face->glyph->bitmap.width), 
    //             static_cast<i32>(face->glyph->bitmap.rows),
    //             imageData
    //         }
    //     );
    //     bootstrap::Image img = LoadImage(VK_FORMAT_R8_UNORM, face->glyph->bitmap.width, face->glyph->bitmap.rows, imageData);

    //     Character character = {
    //         .image = img, 
    //         .size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
    //         .bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
    //         .advance = static_cast<u32>(face->glyph->advance.x)
    //     };
    //     characters.insert(std::pair<char, Character>(c, character));
    // }

    // // Atlas::WriteImage(imageDatas);

    // FT_Done_Face(face);
    // FT_Done_FreeType(ft);

    TIME_FUNCTION(turd.Init(ctx->device, ctx->physDevice, graphicsQueue, commandPool, "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"));

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MaxFramesInFlight;
    allocInfo.commandPool = commandPool;
    vkAllocateCommandBuffers(ctx->device, &allocInfo, drawBuffers.data());

    // img = LoadImage("/home/debian/Downloads/texture.jpg");

    for(u32 i = 0; i < MaxFramesInFlight; i++) {
		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 0;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding fontTextureBinding{};
		fontTextureBinding.binding = 1;
		fontTextureBinding.descriptorCount = 1;
		fontTextureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		fontTextureBinding.pImmutableSamplers = nullptr;
		fontTextureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		SafeArray<VkDescriptorSetLayoutBinding> bindings = {samplerLayoutBinding,fontTextureBinding};

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<u32>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		vkCreateDescriptorSetLayout(ctx->device, &layoutInfo, nullptr, &descSetLayouts[i]);
    }
    
    VkDescriptorPoolSize poolSizes[2];
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = static_cast<u32>(MaxFramesInFlight);
    
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<u32>(MaxFramesInFlight);

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.maxSets = static_cast<u32>(MaxFramesInFlight);
	descriptorPoolCreateInfo.poolSizeCount = std::size(poolSizes);
	descriptorPoolCreateInfo.pPoolSizes = poolSizes;

	vkCreateDescriptorPool(ctx->device, &descriptorPoolCreateInfo, nullptr, &descPool);

    VkDescriptorSetAllocateInfo descriptorAllocateInfo{};
	descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorAllocateInfo.descriptorPool = descPool;
	descriptorAllocateInfo.pSetLayouts = descSetLayouts;
	descriptorAllocateInfo.descriptorSetCount = static_cast<u32>(MaxFramesInFlight);

	vkAllocateDescriptorSets(ctx->device, &descriptorAllocateInfo, descSet);

    for(u32 i = 0; i < MaxFramesInFlight; i++)
	{
        constexpr VkSemaphoreCreateInfo semaphoreCreateInfo{
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0
        };

        constexpr VkFenceCreateInfo fenceCreateInfo{
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT
        };

        vkCreateSemaphore(ctx->device, &semaphoreCreateInfo, nullptr, &renderingFinishedSemaphore[i]);
        vkCreateSemaphore(ctx->device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore[i]);
        vkCreateFence(ctx->device, &fenceCreateInfo, nullptr, &InFlightFence[i]);

		std::vector<VkWriteDescriptorSet> descWriteSets{};

		VkDescriptorImageInfo imageInfo{};
    	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    	imageInfo.imageView = images[0].view;
    	imageInfo.sampler = images[0].sampler;

        VkWriteDescriptorSet writeSet0{};
		writeSet0.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet0.dstSet = descSet[i];
		writeSet0.dstBinding = 0;
		writeSet0.dstArrayElement = 0;
		writeSet0.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeSet0.descriptorCount = 1;
		writeSet0.pImageInfo = &imageInfo;

        descWriteSets.push_back(writeSet0);

        // {
            VkDescriptorImageInfo fontImageInfo{};
            fontImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            fontImageInfo.imageView = turd.FontTextureView;
            fontImageInfo.sampler = turd.FontTextureSampler;

            VkWriteDescriptorSet writeSet{};
            writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeSet.dstSet = descSet[i];
            writeSet.dstBinding = 1;
            writeSet.dstArrayElement = 0;
            writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeSet.descriptorCount = 1;
            writeSet.pImageInfo = &fontImageInfo;
            
            descWriteSets.push_back(writeSet);
        // }


		vkUpdateDescriptorSets(ctx->device, descWriteSets.size(), descWriteSets.data(), 0, nullptr);
	}

    pipeline.vertexShader = Loader::LoadShaderModule("/home/debian/dev/Pixelworlds3.0/build/Shaders/vert.spv", ctx->device);
    pipeline.fragmentShader = Loader::LoadShaderModule("/home/debian/dev/Pixelworlds3.0/build/Shaders/frag.spv", ctx->device);

    VkPipelineShaderStageCreateInfo vshader{};
    vshader.module = pipeline.vertexShader;
    vshader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vshader.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vshader.pName = "main";

    VkPipelineShaderStageCreateInfo fshader{};
    fshader.module = pipeline.fragmentShader;
    fshader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fshader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fshader.pName = "main";

    const SafeArray<VkPipelineShaderStageCreateInfo> shaders = { vshader, fshader };
    
    const SafeArray<VkDescriptorSetLayout> pDescriptorLayouts{
        descSetLayouts
    };
    
    pro::PipelineCreateInfo pcio{};
    pcio.extent = renderArea;
    pcio.format = format;
    pcio.pShaderCreateInfos = &shaders;
    pcio.pAttributeDescriptions = &pAttributeDescriptions;
    pcio.pBindingDescription = &pBindingDescription;
    pcio.pDescriptorLayouts = &pDescriptorLayouts;
    pcio.pPushConstants = &pPushConstants;
    pcio.pShaderCreateInfos = &shaders;
    pcio.subpass = 0;
    TIME_FUNCTION(pro::CreateEntirePipeline(ctx->device, &pcio, &pipeline, PipelineFlags));
    
    u32 requestedImageCount = 0;
    vkGetSwapchainImagesKHR(ctx->device, swapchain, &requestedImageCount, nullptr);
	swapchainImages.resize(requestedImageCount);
	swapchainImageViews.resize(requestedImageCount);
    framebuffers.resize(requestedImageCount);
	vkGetSwapchainImagesKHR(ctx->device, swapchain, &requestedImageCount, swapchainImages.data());

    for (u32 i = 0; i < swapchainImageViews.size(); i++) {

        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(ctx->device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create view for swapchain image " << i << std::endl;
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pipeline.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &swapchainImageViews[i];
        framebufferInfo.width = renderArea.width;
        framebufferInfo.height = renderArea.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(ctx->device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create framebuffer for swapchain image " << i << std::endl;
        }
    }
}

VulkanContext CreateVulkanContext(const char *title, u32 windowWidth, u32 windowHeight)
{
    VulkanContext retval{};

    retval.window = SDL_CreateWindow(title, 800, 600, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    assert(retval.window != nullptr);

    retval.instance = bootstrap::CreateInstance(title);
    assert(retval.instance != nullptr);
    
    SDL_Vulkan_CreateSurface(retval.window, retval.instance, nullptr, &retval.surface);
    assert(retval.surface != nullptr && SDL_GetError());
    
    retval.physDevice = bootstrap::GetSelectedPhysicalDevice(retval.instance, retval.surface);

	u32 queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(retval.physDevice, &queueCount, nullptr);
	VkQueueFamilyProperties queueFamilies[queueCount];
	vkGetPhysicalDeviceQueueFamilyProperties(retval.physDevice, &queueCount, queueFamilies);

    // Clang loves complaining about these.
	__attribute__((unused)) u32 graphicsFamily, presentFamily, computeFamily;
	__attribute__((unused)) bool foundGraphicsFamily = false, foundPresentFamily = false, foundComputeFamily = false;
		
	u32 i = 0;
	for (const auto& queueFamily : queueFamilies) {
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(retval.physDevice, i, retval.surface, &presentSupport);
	
    	if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			graphicsFamily = i;
			foundGraphicsFamily = true;
		}
		if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
			computeFamily = i;
			foundComputeFamily = true;
		}
		if (presentSupport) {
			presentFamily = i;
			foundPresentFamily = true;
		}
		if (foundGraphicsFamily && foundComputeFamily && foundPresentFamily)
			break;
	
    	i++;
	}
    
    std::set<u32> uniqueQueueFamilies = {graphicsFamily, presentFamily, computeFamily};
	VkDeviceQueueCreateInfo queueCreateInfos[uniqueQueueFamilies.size()];

	constexpr float queuePriority = 1.0f;

	u32 j = 0;
	for (const auto& queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos[j++] = queueCreateInfo;
	}

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<u32>(uniqueQueueFamilies.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.enabledExtensionCount = static_cast<u32>(DeviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = DeviceExtensions.data();

	if (vkCreateDevice(retval.physDevice, &deviceCreateInfo, nullptr, &retval.device) != VK_SUCCESS) {
		printf("Failed to create device\n");
		abort();
	}

    return retval;
}

bool Renderer::BeginRender()
{
    const auto& drawBuffer = drawBuffers[currentFrame];

    vkWaitForFences(ctx->device, 1, &InFlightFence[currentFrame], VK_TRUE, UINT64_MAX);

    const VkResult imageAcquireResult = vkAcquireNextImageKHR(ctx->device, swapchain, UINT64_MAX, imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if(imageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR || imageAcquireResult == VK_SUBOPTIMAL_KHR || framebufferResized)
	{
		framebufferResized = false;
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physDevice, ctx->surface, &surfaceCapabilities);

        ResizeRenderWindow(surfaceCapabilities.currentExtent);
		return false;
	}
	else if (imageAcquireResult != VK_SUCCESS && imageAcquireResult != VK_SUBOPTIMAL_KHR) 
	{
		throw std::runtime_error("Failed to acquire image from swapchain!!! ");
		return false;
    }

    vkResetFences(ctx->device, 1, &InFlightFence[currentFrame]);

    constexpr VkClearValue clearValues = {{0.0f, 0.0f, 0.0f, 0.0f}};

    static VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea = {{}, renderArea};
    renderPassInfo.renderPass = pipeline.renderPass;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValues;

    constexpr static VkCommandBufferBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr
    };

    vkBeginCommandBuffer(drawBuffer, &beginInfo);
    vkCmdBeginRenderPass(drawBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);        

    return true;
}

void Renderer::EndRender()
{
    const auto& drawBuffer = drawBuffers[currentFrame];
	
    vkCmdEndRenderPass(drawBuffer);
    vkEndCommandBuffer(drawBuffer);

	VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    const VkSemaphore waitSemaphores[] = {imageAvailableSemaphore[currentFrame]};
    const VkSemaphore signalSemaphores[] = {renderingFinishedSemaphore[currentFrame]};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &drawBuffer;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;


    // Submit command buffer
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, InFlightFence[currentFrame]);
    
	// Present the image
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // This is signalSemaphores so that this starts as soon as the signaled semaphores are signaled.
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;

    vkQueuePresentKHR(graphicsQueue, &presentInfo);

    currentFrame = ((currentFrame + 1) % MaxFramesInFlight);
}

// void Renderer::AddTextureToQueue(Texture *tex)
// {
//     unloadedTextures.push_back(tex);
// }



// void Renderer::LoadAllTexturesInQueue()
// {
// }

bootstrap::Image Renderer::LoadImage(const char *path)
{
    i32 width = 0, height = 0, textureChannels = 0;
	u8* pixels = stbi_load(path, &width, &height, &textureChannels, STBI_rgb_alpha);
	assert(pixels != nullptr);
	
	VkDeviceSize textureSize = width * height * sizeof(float);

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
	stagingBuffer = bootstrap::CreateBuffer(ctx->physDevice, ctx->device, stagingBufferMemory, textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	void* data;
	vkMapMemory(ctx->device, stagingBufferMemory, 0, textureSize, 0, &data);
	memcpy(data, pixels, static_cast<u64>(textureSize));
	vkUnmapMemory(ctx->device, stagingBufferMemory);

	stbi_image_free(pixels);

	bootstrap::Image img = bootstrap::CreateImage(ctx->device, ctx->physDevice, width, height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	bootstrap::TransitionImageLayout(ctx->device, commandPool, graphicsQueue, img, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	bootstrap::CopyBufferToImage(stagingBuffer, ctx->device, commandPool, graphicsQueue, img.image, static_cast<u32>(width), static_cast<u32>(height));
	bootstrap::TransitionImageLayout(ctx->device, commandPool, graphicsQueue, img, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(ctx->device, stagingBuffer, nullptr);
    vkFreeMemory(ctx->device, stagingBufferMemory, nullptr);

    img.view = bootstrap::CreateImageView(ctx->device, img.image, format, VK_IMAGE_ASPECT_COLOR_BIT);

	img.sampler = CreateSampler(ctx->device);

    return img;
}

bootstrap::Image Renderer::LoadImage(VkFormat format, u64 width, u64 height, void *data)
{
    VkDeviceSize textureSize = width * height * sizeof(float);

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
	stagingBuffer = bootstrap::CreateBuffer(ctx->physDevice, ctx->device, stagingBufferMemory, textureSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

	void* ndata;
	vkMapMemory(ctx->device, stagingBufferMemory, 0, textureSize, 0, &ndata);
	memcpy(ndata, data, static_cast<u64>(textureSize));
	vkUnmapMemory(ctx->device, stagingBufferMemory);

	bootstrap::Image img = bootstrap::CreateImage(ctx->device, ctx->physDevice, width, height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	bootstrap::TransitionImageLayout(ctx->device, commandPool, graphicsQueue, img, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	bootstrap::CopyBufferToImage(stagingBuffer, ctx->device, commandPool, graphicsQueue, img.image, static_cast<u32>(width), static_cast<u32>(height));
	bootstrap::TransitionImageLayout(ctx->device, commandPool, graphicsQueue, img, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(ctx->device, stagingBuffer, nullptr);
    vkFreeMemory(ctx->device, stagingBufferMemory, nullptr);

    img.view = bootstrap::CreateImageView(ctx->device, img.image, format, VK_IMAGE_ASPECT_COLOR_BIT);

	img.sampler = CreateSampler(ctx->device);

    return img;
}

void Renderer::ResizeRenderWindow(const VkExtent2D newExtent, const bool forceWindowResize)
{
    renderArea = newExtent;
    if (forceWindowResize) SDL_SetWindowSize(ctx->window, newExtent.width, newExtent.height);

    vkDeviceWaitIdle(ctx->device);

    vkDestroySwapchainKHR(ctx->device, swapchain, nullptr);
    for(u32 i = 0; i < MaxFramesInFlight; i++) {
        vkDestroyFramebuffer(ctx->device, framebuffers[i], nullptr);
        vkDestroySemaphore(ctx->device, imageAvailableSemaphore[i], nullptr);
        vkDestroySemaphore(ctx->device, renderingFinishedSemaphore[i], nullptr);
        vkDestroyFence(ctx->device, InFlightFence[i], nullptr);
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physDevice, ctx->surface, &surfaceCapabilities);

    if(surfaceCapabilities.currentExtent.width == UINT32_MAX) {
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, nullptr);
        SDL_Vulkan_CreateSurface(ctx->window, ctx->instance, nullptr, &ctx->surface);
    }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physDevice, ctx->surface, &surfaceCapabilities);
    renderArea = surfaceCapabilities.currentExtent;

    pro::SwapchainCreateInfo scio{};
    scio.extent = renderArea;
    scio.presentMode = VSyncEnabled ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;
    scio.physDevice = ctx->physDevice;
    scio.surface = ctx->surface;
    scio.format = format;
    scio.requestedImageCount = imageCount;
    scio.colorSpace = colorSpace;
    pro::CreateSwapChain(ctx->device, &scio, &swapchain);

    VkPipelineShaderStageCreateInfo vshader{};
    vshader.module = pipeline.vertexShader;
    vshader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vshader.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vshader.pName = "main";

    VkPipelineShaderStageCreateInfo fshader{};
    fshader.module = pipeline.fragmentShader;
    fshader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fshader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fshader.pName = "main";

    const SafeArray<VkPipelineShaderStageCreateInfo> shaders = { vshader, fshader };

    const SafeArray<VkDescriptorSetLayout> pDescriptorLayouts{
        descSetLayouts
    };
        
    pro::PipelineCreateInfo pcio{};
    pcio.extent = renderArea;
    pcio.format = format;
    pcio.pShaderCreateInfos = &shaders;
    pcio.pAttributeDescriptions = &pAttributeDescriptions;
    pcio.pBindingDescription = &pBindingDescription;
    pcio.pDescriptorLayouts = &pDescriptorLayouts;
    pcio.pPushConstants = &pPushConstants;
    pcio.pShaderCreateInfos = &shaders;
    pcio.subpass = 0;
    pro::CreateEntirePipeline(ctx->device, &pcio, &pipeline, PipelineFlags);
   
    constexpr VkSemaphoreCreateInfo semaphoreCreateInfo{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0
    };

    constexpr VkFenceCreateInfo fenceCreateInfo{
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (u32 i = 0; i < MaxFramesInFlight; i++) {
        vkCreateSemaphore(ctx->device, &semaphoreCreateInfo, nullptr, &renderingFinishedSemaphore[i]);
        vkCreateSemaphore(ctx->device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore[i]);
        vkCreateFence(ctx->device, &fenceCreateInfo, nullptr, &InFlightFence[i]);
    }

    u32 requestedImageCount = 0;
    vkGetSwapchainImagesKHR(ctx->device, swapchain, &requestedImageCount, nullptr);
	swapchainImages.resize(requestedImageCount);
	swapchainImageViews.resize(requestedImageCount);
    framebuffers.resize(requestedImageCount);
	vkGetSwapchainImagesKHR(ctx->device, swapchain, &requestedImageCount, swapchainImages.data());

    for (u32 i = 0; i < swapchainImageViews.size(); i++) {

        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(ctx->device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create view for swapchain image " << i << std::endl;
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pipeline.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &swapchainImageViews[i];
        framebufferInfo.width = renderArea.width;
        framebufferInfo.height = renderArea.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(ctx->device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create framebuffer for swapchain image " << i << std::endl;
        }
    }
}

// std::map<char, GlyphData> Renderer::LoadFont(std::string& fontFile, i32 fontSize)
// {
//     std::map<char, GlyphData> glyphMap;

//     // Initialize FreeType library
//     FT_Library ft;
//     if (FT_Init_FreeType(&ft)) {
//         std::cerr << "Failed to initialize FreeType library" << std::endl;
//         return glyphMap;
//     }

//     // Load font face
//     FT_Face face;
//     if (FT_New_Face(ft, fontFile.c_str(), 0, &face)) {
//         std::cerr << "Failed to load font file: " << fontFile << std::endl;
//         FT_Done_FreeType(ft);
//         return glyphMap;
//     }

//     // Set font size
//     FT_Set_Pixel_Sizes(face, 0, fontSize);

//     // Load glyphs
//     for (uchar c = 0; c < 128; c++) {
//         // Load character glyph
//         if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
//             std::cerr << "Failed to load glyph for character: " << c << std::endl;
//             break;
//         }

//         // Generate glyph data
//         GlyphData glyphData;
//         glyphData.width = face->glyph->bitmap.width;
//         glyphData.height = face->glyph->bitmap.rows;
//         glyphData.bearingX = face->glyph->bitmap_left;
//         glyphData.bearingY = face->glyph->bitmap_top;
//         glyphData.advance = face->glyph->advance.x >> 6; // 1/64th pixel units

//         // Copy glyph bitmap data
//         glyphData.buffer.resize(glyphData.width * glyphData.height);
//         memcpy(glyphData.buffer.data(), face->glyph->bitmap.buffer, glyphData.width * glyphData.height);

//         // Add glyph data to map
//         glyphMap[c] = glyphData;
//     }

//     // Clean up FreeType
//     FT_Done_Face(face);
//     FT_Done_FreeType(ft);

//     return glyphMap;
// }

Texture::Texture(Renderer *rd)
{

}
