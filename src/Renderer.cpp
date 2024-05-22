#include "Renderer.hpp"
#include "pro.hpp"
// #include "Bootstrap.hpp"
#include "TextRenderer.hpp"
#include "Image.hpp"

constexpr u32 PipelineFlags = 0;

const std::vector<VkVertexInputAttributeDescription> pAttributeDescriptions{ 
    // location, binding, format, offset,
    {  0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 },
};
    
const std::vector<VkVertexInputBindingDescription> pBindingDescription{ 
    // binding, stride, inputRate,
    { 0, sizeof(vec2) * 2 /* 2 vec2's for position and 2 for texture positions */, VK_VERTEX_INPUT_RATE_VERTEX },
};
    
const std::vector<VkPushConstantRange> pPushConstants{ 
    //stageFlags, offset, size,
    { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4) },
};

constexpr VkPresentModeKHR PresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

void RendererSingleton::Initialize() {
    /* Initialize graphics singleton */
    {
        if (!pro::GetSupportedFormat(device, physDevice, surface, &Graphics->SwapChainImageFormat, &Graphics->SwapChainColorSpace)) {
            throw std::runtime_error("Failed to find a supported format!");
        }
        Graphics->SwapChainImageCount = pro::GetImageCount(physDevice, surface);

        const u32 graphicsQueueIndex = bootstrap::GetQueueIndex(physDevice, bootstrap::QueueType::GRAPHICS);
        const u32 computeQueueIndex = bootstrap::GetQueueIndex(physDevice, bootstrap::QueueType::COMPUTE);
        const u32 transferQueueIndex = bootstrap::GetQueueIndex(physDevice, bootstrap::QueueType::TRANSFER);
        u32 graphicsAndComputeQueueIndex = 0;

        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, nullptr);
        VkQueueFamilyProperties queueFamilies[queueFamilyCount];
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueFamilyCount, queueFamilies);

        u32 queueIndex = 0;
        for (const auto& family : queueFamilies) {
            if (family.queueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT))
                graphicsAndComputeQueueIndex = queueIndex;
            queueIndex++;
        }

        Graphics->GraphicsFamilyIndex = graphicsQueueIndex;
        Graphics->ComputeFamilyIndex = computeQueueIndex;
        Graphics->PresentFamilyIndex = transferQueueIndex;
        Graphics->GraphicsAndComputeFamilyIndex = graphicsAndComputeQueueIndex;

        vkGetDeviceQueue(device, graphicsQueueIndex, 0, &Graphics->GraphicsQueue);
        vkGetDeviceQueue(device, computeQueueIndex, 0, &Graphics->ComputeQueue);
        vkGetDeviceQueue(device, transferQueueIndex, 0, &Graphics->TransferQueue);
        vkGetDeviceQueue(device, graphicsAndComputeQueueIndex, 0, &Graphics->GraphicsAndComputeQueue);
    }

    pro::SwapchainCreateInfo scio{};
    scio.extent = Graphics->RenderArea;
    scio.ePresentMode = PresentMode;
    scio.physDevice = physDevice;
    scio.surface = surface;
    scio.format = Graphics->SwapChainImageFormat;
    scio.eColorSpace = Graphics->SwapChainColorSpace;
    scio.requestedImageCount = Graphics->SwapChainImageCount;
    pro::CreateSwapChain(device, &scio, &swapchain);

    commandPool = bootstrap::CreateCommandPool(device, physDevice, bootstrap::QueueType::GRAPHICS);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MaxFramesInFlight;
    allocInfo.commandPool = commandPool;
    vkAllocateCommandBuffers(device, &allocInfo, drawBuffers);

    ImageLoadInfo imginfo{};
    imginfo.pPath = "/home/debian/Downloads/texture.jpg";
    imginfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imginfo.samples = VK_SAMPLE_COUNT_1_BIT;

    Image img;
    assert(Images->LoadImage(&imginfo, &img, IMAGE_LOAD_TYPE_FROM_DISK) == IMAGE_LOAD_SUCCESS);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.maxAnisotropy = 1.0f;

    VkSampler sampler;
    if(vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create font texture sampler!");

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

		std::vector<VkDescriptorSetLayoutBinding> bindings = {samplerLayoutBinding,fontTextureBinding};

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<u32>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descSetLayouts[i]);
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

	vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descPool);

    VkDescriptorSetAllocateInfo descriptorAllocateInfo{};
	descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorAllocateInfo.descriptorPool = descPool;
	descriptorAllocateInfo.pSetLayouts = descSetLayouts;
	descriptorAllocateInfo.descriptorSetCount = static_cast<u32>(MaxFramesInFlight);

	vkAllocateDescriptorSets(device, &descriptorAllocateInfo, descSet);

    for(u32 i = 0; i < MaxFramesInFlight; i++)
	{
        constexpr VkSemaphoreCreateInfo semaphoreCreateInfo{
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0
        };

        constexpr VkFenceCreateInfo fenceCreateInfo{
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT
        };

        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderingFinishedSemaphore[i]);
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore[i]);
        vkCreateFence(device, &fenceCreateInfo, nullptr, &InFlightFence[i]);

		VkDescriptorImageInfo imageInfo{};
    	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    	imageInfo.imageView = img.view;
    	imageInfo.sampler = sampler;

        VkWriteDescriptorSet writeSet{};
		writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeSet.dstSet = descSet[i];
		writeSet.dstBinding = 0;
		writeSet.dstArrayElement = 0;
		writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeSet.descriptorCount = 1;
		writeSet.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
	}

    pipeline.vertexShader = LoadShaderModule("./Shaders/vert.spv");
    pipeline.fragmentShader = LoadShaderModule("./Shaders/frag.spv");

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

    const std::vector<VkPipelineShaderStageCreateInfo> shaders = { vshader, fshader };

    std::vector<VkDescriptorSetLayout> pDescriptorLayouts;
    for(const auto& layout : descSetLayouts)
        pDescriptorLayouts.push_back(layout);
    
    pro::PipelineCreateInfo pcio{};
    pcio.extent = Graphics->RenderArea;
    pcio.format = Graphics->SwapChainImageFormat;
    pcio.pShaderCreateInfos = &shaders;
    pcio.pAttributeDescriptions = &pAttributeDescriptions;
    pcio.pBindingDescriptions = &pBindingDescription;
    pcio.pDescriptorLayouts = &pDescriptorLayouts;
    pcio.pPushConstants = &pPushConstants;
    pcio.pShaderCreateInfos = &shaders;
    pcio.subpass = 0;
    TIME_FUNCTION(pro::CreateEntirePipeline(device, &pcio, &pipeline, PipelineFlags));

    Graphics->GlobalRenderPass = pipeline.renderPass;

    u32 requestedImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &requestedImageCount, nullptr);
	swapchainImages.resize(requestedImageCount);
	swapchainImageViews.resize(requestedImageCount);
    framebuffers.resize(requestedImageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &requestedImageCount, swapchainImages.data());

    for (u32 i = 0; i < swapchainImageViews.size(); i++) {

        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = Graphics->SwapChainImageFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create view for swapchain image " << i << std::endl;
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pipeline.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &swapchainImageViews[i];
        framebufferInfo.width = Graphics->RenderArea.x;
        framebufferInfo.height = Graphics->RenderArea.y;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
            std::cerr << "Failed to create framebuffer for swapchain image " << i << std::endl;
    }
}

void RendererSingleton::ProcessEvent(SDL_Event *event)
{
    if ((event->type == SDL_EVENT_QUIT) || (event->type == SDL_EVENT_KEY_DOWN && event->key.keysym.sym == EXIT_KEY))
        exit(0);

    
}

bool RendererSingleton::BeginRender()
{
    const auto& drawBuffer = GetDrawBuffer();

    vkWaitForFences(device, 1, &InFlightFence[currentFrame], VK_TRUE, UINT64_MAX);

    const VkResult imageAcquireResult = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if(imageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR || imageAcquireResult == VK_SUBOPTIMAL_KHR || framebufferResized)
	{
		framebufferResized = false;
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &surfaceCapabilities);

        ResizeRenderWindow(surfaceCapabilities.currentExtent);
        Graphics->OnWindowResized();
		return false;
	}
	else if (imageAcquireResult != VK_SUCCESS && imageAcquireResult != VK_SUBOPTIMAL_KHR) 
	{
		throw std::runtime_error("Failed to acquire image from swapchain!!! ");
		return false;
    }

    vkResetFences(device, 1, &InFlightFence[currentFrame]);

    constexpr VkClearValue clearValues = {{ 0.0f, 0.0f, 0.0f, 0.0f} };

    static VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea = {{}, VkExtent2D({Graphics->RenderArea.x, Graphics->RenderArea.y})};
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

void RendererSingleton::EndRender()
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

    const VkCommandBuffer buffers[] = { drawBuffer };
    submitInfo.commandBufferCount = std::size(buffers);
    submitInfo.pCommandBuffers = buffers;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    // Submit command buffer
    vkQueueSubmit(Graphics->GraphicsQueue, 1, &submitInfo, InFlightFence[currentFrame]);

	// Present the image
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // This is signalSemaphores so that this starts as soon as the signaled semaphores are signaled.
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;

    if(TextRenderer->dispatchedCompute)
    {
        vkWaitForFences(device, 1, &TextRenderer->fence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &TextRenderer->fence);
        vkResetCommandBuffer(TextRenderer->computeCmdBuffers[currentFrame], 0);

        TextRenderer->dispatchedCompute = false;
    }
    
    vkQueuePresentKHR(Graphics->GraphicsQueue, &presentInfo);

    currentFrame = ((currentFrame + 1) % MaxFramesInFlight);
}

VkShaderModule RendererSingleton::LoadShaderModule(const char *path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if(!file.is_open()) {
        throw std::runtime_error("Failed to open shader file at path " + std::string(path));
    }

    size_t fileSize = (size_t)file.tellg();
    char buffer[fileSize];
    file.seekg(0);
    file.read(buffer, fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = fileSize;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer);
    VkShaderModule shaderModule;
    if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }
    return shaderModule;
}

void RendererSingleton::ResizeRenderWindow(const VkExtent2D newExtent, const bool forceWindowResize)
{
    Graphics->RenderArea = WindowSizeType{newExtent.width, newExtent.height};
    if (forceWindowResize) SDL_SetWindowSize(window, newExtent.width, newExtent.height);

    vkDeviceWaitIdle(device);

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    for(u32 i = 0; i < MaxFramesInFlight; i++) {
        vkDestroyFramebuffer(device, framebuffers[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphore[i], nullptr);
        vkDestroySemaphore(device, renderingFinishedSemaphore[i], nullptr);
        vkDestroyFence(device, InFlightFence[i], nullptr);
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &surfaceCapabilities);

    if(surfaceCapabilities.currentExtent.width == UINT32_MAX) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface);
    }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &surfaceCapabilities);
    Graphics->RenderArea.x = surfaceCapabilities.currentExtent.width;
    Graphics->RenderArea.y = surfaceCapabilities.currentExtent.height;

    pro::SwapchainCreateInfo scio{};
    scio.extent = Graphics->RenderArea;
    scio.ePresentMode = PresentMode;
    scio.physDevice = physDevice;
    scio.surface = surface;
    scio.format = Graphics->SwapChainImageFormat;
    scio.requestedImageCount = Graphics->SwapChainImageCount;
    scio.eColorSpace = Graphics->SwapChainColorSpace;
    pro::CreateSwapChain(device, &scio, &swapchain);

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

    const std::vector<VkPipelineShaderStageCreateInfo> shaders = { vshader, fshader };

    std::vector<VkDescriptorSetLayout> pDescriptorLayouts;
    for(const auto& layout : descSetLayouts)
        pDescriptorLayouts.push_back(layout);
    
        
    pro::PipelineCreateInfo pcio{};
    pcio.extent = VkExtent2D({Graphics->RenderArea.x, Graphics->RenderArea.y});
    pcio.format = Graphics->SwapChainImageFormat;
    pcio.pShaderCreateInfos = &shaders;
    pcio.pAttributeDescriptions = &pAttributeDescriptions;
    pcio.pBindingDescriptions = &pBindingDescription;
    pcio.pDescriptorLayouts = &pDescriptorLayouts;
    pcio.pPushConstants = &pPushConstants;
    pcio.pShaderCreateInfos = &shaders;
    pcio.subpass = 0;
    pro::CreateEntirePipeline(device, &pcio, &pipeline, PipelineFlags);
   
    constexpr VkSemaphoreCreateInfo semaphoreCreateInfo{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0
    };

    constexpr VkFenceCreateInfo fenceCreateInfo{
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (u32 i = 0; i < MaxFramesInFlight; i++) {
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderingFinishedSemaphore[i]);
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphore[i]);
        vkCreateFence(device, &fenceCreateInfo, nullptr, &InFlightFence[i]);
    }

    u32 requestedImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &requestedImageCount, nullptr);
	swapchainImages.resize(requestedImageCount);
	swapchainImageViews.resize(requestedImageCount);
    framebuffers.resize(requestedImageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &requestedImageCount, swapchainImages.data());

    for (u32 i = 0; i < swapchainImageViews.size(); i++) {

        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = Graphics->SwapChainImageFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create view for swapchain image " << i << std::endl;
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = pipeline.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &swapchainImageViews[i];
        framebufferInfo.width = Graphics->RenderArea.x;
        framebufferInfo.height = Graphics->RenderArea.y;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            std::cerr << "Failed to create framebuffer for swapchain image " << i << std::endl;
        }
    }
}
