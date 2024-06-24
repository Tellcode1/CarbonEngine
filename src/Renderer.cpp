#include "Renderer.hpp"
#include "pro.hpp"
#include "Bootstrap.hpp"

constexpr VkPresentModeKHR PresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

void RendererSingleton::Initialize() {
    /* Initialize graphics singleton */
    {
        if (!pro::GetSupportedFormat(device, physDevice, surface, &Graphics->SwapChainImageFormat, &Graphics->SwapChainColorSpace)) {
            LOG_AND_ABORT("Failed to find a supported format!\n");
        }
        Graphics->SwapChainImageCount = pro::GetImageCount(physDevice, surface);

        u32 queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, nullptr);
        VkQueueFamilyProperties queueFamilies[queueCount];
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, queueFamilies);

        u32 graphicsFamily, graphicsAndComputeFamily, presentFamily, computeFamily, transferFamily;
        bool foundGraphicsFamily = false, foundGraphicsAndComputeFamily = false, foundPresentFamily = false, foundComputeFamily = false, foundTransferFamily = false;
            
        u32 i = 0;
        for (const auto& queueFamily : queueFamilies) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, surface, &presentSupport);
            
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                graphicsAndComputeFamily = i;
                foundGraphicsAndComputeFamily = true;
            }
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamily = i;
                foundGraphicsFamily = true;
            }
            if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                computeFamily = i;
                foundComputeFamily = true;
            }
            if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                transferFamily = i;
                foundTransferFamily = true;
            }
            if (presentSupport) {
                presentFamily = i;
                foundPresentFamily = true;
            }
            if (foundGraphicsFamily && foundGraphicsAndComputeFamily && foundPresentFamily && foundComputeFamily && foundTransferFamily)
                break;
        
            i++;
        }

        Graphics->GraphicsFamilyIndex = graphicsFamily;
        Graphics->ComputeFamilyIndex = computeFamily;
        Graphics->TransferQueueIndex = transferFamily;
        Graphics->PresentFamilyIndex = presentFamily;
        Graphics->GraphicsAndComputeFamilyIndex = graphicsAndComputeFamily;

        vkGetDeviceQueue(device, graphicsFamily, 0, &Graphics->GraphicsQueue);
        vkGetDeviceQueue(device, computeFamily, 0, &Graphics->ComputeQueue);
        vkGetDeviceQueue(device, transferFamily, 0, &Graphics->TransferQueue);
        vkGetDeviceQueue(device, presentFamily, 0, &Graphics->PresentQueue);
        vkGetDeviceQueue(device, graphicsAndComputeFamily, 0, &Graphics->GraphicsAndComputeQueue);
    }

    i32 w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);

    Graphics->RenderExtent.width = w;
    Graphics->RenderExtent.height = h;

    pro::SwapchainCreateInfo scio{};
    scio.extent = Graphics->RenderExtent;
    scio.ePresentMode = PresentMode;
    scio.physDevice = physDevice;
    scio.surface = surface;
    scio.format = Graphics->SwapChainImageFormat;
    scio.eColorSpace = Graphics->SwapChainColorSpace;
    scio.requestedImageCount = Graphics->SwapChainImageCount;
    pro::CreateSwapChain(device, &scio, &swapchain);

    pro::RenderPassCreateInfo rpi{};
    rpi.format = Graphics->SwapChainImageFormat;
    rpi.subpass = 0;
    pro::CreateRenderPass(device, &rpi, &Graphics->GlobalRenderPass, 0);

    commandPool = bootstrap::CreateCommandPool(device, physDevice, bootstrap::QueueType::GRAPHICS);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = MaxFramesInFlight;
    allocInfo.commandPool = commandPool;
    vkAllocateCommandBuffers(device, &allocInfo, drawBuffers);

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
            LOG_ERROR("Failed to create view for swapchain image %d", i);
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = Graphics->GlobalRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &swapchainImageViews[i];
        framebufferInfo.width = Graphics->RenderExtent.width;
        framebufferInfo.height = Graphics->RenderExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
            LOG_ERROR("Failed to create framebuffer for swapchain image %d", i);
    }
}

void RendererSingleton::Update()
{
}

void RendererSingleton::ProcessEvent(SDL_Event *event)
{
    if ((event->type == SDL_EVENT_QUIT) || (event->type == SDL_EVENT_KEY_DOWN && event->key.keysym.scancode == EXIT_KEY))
        running = false;

    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        resizeRequested = true;
    }

    if (resizeRequested) {
        i32 w, h;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        
        Graphics->RenderExtent.width = w;
        Graphics->RenderExtent.height = h;

        _SignalResize(Graphics->RenderExtent);

        resizeRequested = false;
    }
}

bool RendererSingleton::BeginRender()
{
    const VkCommandBuffer& drawBuffer = GetDrawBuffer();

    vkWaitForFences(device, 1, &InFlightFence[currentFrame], VK_TRUE, UINT64_MAX);

    const VkResult imageAcquireResult = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if(imageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR || imageAcquireResult == VK_SUBOPTIMAL_KHR)
	{
        resizeRequested = true;
		return false;
	}
	else if (imageAcquireResult != VK_SUCCESS && imageAcquireResult != VK_SUBOPTIMAL_KHR) 
	{
		throw std::runtime_error("Failed to acquire image from swapchain!!! ");
		return false;
    }

    vkResetFences(device, 1, &InFlightFence[currentFrame]);

    constexpr VkClearValue clearValues = { { 0.0f, 0.0f, 0.0f, 1.0f} };

    static VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.extent = Graphics->RenderExtent;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderPass = Graphics->GlobalRenderPass;
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
    submitInfo.commandBufferCount = array_len(buffers);
    submitInfo.pCommandBuffers = buffers;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    vkQueueSubmit(Graphics->GraphicsQueue, 1, &submitInfo, InFlightFence[currentFrame]);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // This is signalSemaphores so that this starts as soon as the signaled semaphores are signaled.
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;

    // if(TextRenderer->dispatchedCompute)
    // {
    //     vkWaitForFences(device, 1, &TextRenderer->fence, VK_TRUE, UINT64_MAX);
    //     vkResetFences(device, 1, &TextRenderer->fence);
    //     vkResetCommandBuffer(TextRenderer->renderCmd[currentFrame], 0);

    //     TextRenderer->dispatchedCompute = false;
    // }
    
    VkResult result = vkQueuePresentKHR(Graphics->PresentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        resizeRequested = true;
    }
    
    currentFrame = ((currentFrame + 1) % MaxFramesInFlight);
}

void RendererSingleton::_SignalResize(VkExtent2D newExtent)
{
    std::cout << "resize\n";
    vkDeviceWaitIdle(device);

    // adhoc method of resetting them
    for(u32 i = 0; i < MaxFramesInFlight; i++) {
        vkDestroySemaphore(device, imageAvailableSemaphore[i], nullptr);
        vkDestroySemaphore(device, renderingFinishedSemaphore[i], nullptr);
        vkDestroyFence(device, InFlightFence[i], nullptr);
    }

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

    vkDestroySwapchainKHR(device, swapchain, nullptr);

    pro::SwapchainCreateInfo scio{};
    scio.extent = newExtent;
    scio.ePresentMode = PresentMode;
    scio.physDevice = physDevice;
    scio.surface = surface;
    scio.format = Graphics->SwapChainImageFormat;
    scio.requestedImageCount = Graphics->SwapChainImageCount;
    scio.eColorSpace = Graphics->SwapChainColorSpace;
    pro::CreateSwapChain(device, &scio, &swapchain);

    for (auto view : swapchainImageViews)
        vkDestroyImageView(device, view, nullptr);
    for (auto fb : framebuffers)
        vkDestroyFramebuffer(device, fb, nullptr);

    u32 requestedImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &requestedImageCount, nullptr);
	swapchainImages.resize(requestedImageCount);
	swapchainImageViews.resize(requestedImageCount);
    framebuffers.resize(requestedImageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &requestedImageCount, swapchainImages.data());

    Graphics->SwapChainImageCount = requestedImageCount;

    std::cout << newExtent.width << 'x' << newExtent.height << '\n';

    for (u32 i = 0; i < requestedImageCount; i++)
    {
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
            LOG_ERROR("Failed to create view for swapchain image %u", i);
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = Graphics->GlobalRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &swapchainImageViews[i];
        framebufferInfo.width = newExtent.width;
        framebufferInfo.height = newExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
           LOG_ERROR("Failed to create framebuffer for swapchain image %u", i);
        }
    }
}
