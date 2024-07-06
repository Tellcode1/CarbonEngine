#include "Renderer.hpp"
#include "pro.hpp"
#include "Bootstrap.hpp"

VkFormat VulkanContextSingleton::SwapChainImageFormat;
VkColorSpaceKHR VulkanContextSingleton::SwapChainColorSpace;
u32 VulkanContextSingleton::SwapChainImageCount = 0;
VkRenderPass VulkanContextSingleton::GlobalRenderPass = VK_NULL_HANDLE;
VkExtent2D VulkanContextSingleton::RenderExtent = DefaultExtent;
u32 VulkanContextSingleton::GraphicsFamilyIndex = 0;
u32 VulkanContextSingleton::PresentFamilyIndex = 0;
u32 VulkanContextSingleton::ComputeFamilyIndex = 0;
u32 VulkanContextSingleton::TransferQueueIndex = 0;
u32 VulkanContextSingleton::GraphicsAndComputeFamilyIndex = 0;
VkQueue VulkanContextSingleton::GraphicsQueue = VK_NULL_HANDLE;
VkQueue VulkanContextSingleton::GraphicsAndComputeQueue = VK_NULL_HANDLE;
VkQueue VulkanContextSingleton::PresentQueue = VK_NULL_HANDLE;
VkQueue VulkanContextSingleton::ComputeQueue = VK_NULL_HANDLE;
VkQueue VulkanContextSingleton::TransferQueue = VK_NULL_HANDLE;
boost::signals2::signal<void(void)> VulkanContextSingleton::OnWindowResized;

VkSwapchainKHR Renderer::swapchain = VK_NULL_HANDLE;
u8 Renderer::currentFrame = 0;
u32 Renderer::imageIndex = 0;
bool Renderer::resizeRequested = false;
bool Renderer::running = true;
VkCommandPool Renderer::commandPool = VK_NULL_HANDLE;
std::vector<FrameRenderData> Renderer::renderData;
VkCommandBuffer Renderer::drawBuffers[MaxFramesInFlight];

constexpr VkPresentModeKHR PresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

void Renderer::Initialize() {
    /* Initialize graphics singleton */
    {
        if (!pro::GetSupportedFormat(device, physDevice, surface, &vctx::SwapChainImageFormat, &vctx::SwapChainColorSpace)) {
            LOG_AND_ABORT("No supported format for display.");
        }
        vctx::SwapChainImageCount = pro::GetImageCount(physDevice, surface);

        u32 queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, nullptr);
        VkQueueFamilyProperties *queueFamilies = new VkQueueFamilyProperties[queueCount];
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, queueFamilies);

        u32 graphicsFamily, graphicsAndComputeFamily, presentFamily, computeFamily, transferFamily;
        bool foundGraphicsFamily = false, foundGraphicsAndComputeFamily = false, foundPresentFamily = false, foundComputeFamily = false, foundTransferFamily = false;
            
        u32 i = 0;
        for (u32 j = 0; j < queueCount; j++) {
            const VkQueueFamilyProperties& queueFamily = queueFamilies[j];
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

        delete[] queueFamilies;

        vctx::GraphicsFamilyIndex = graphicsFamily;
        vctx::ComputeFamilyIndex = computeFamily;
        vctx::TransferQueueIndex = transferFamily;
        vctx::PresentFamilyIndex = presentFamily;
        vctx::GraphicsAndComputeFamilyIndex = graphicsAndComputeFamily;

        vkGetDeviceQueue(device, graphicsFamily, 0, &vctx::GraphicsQueue);
        vkGetDeviceQueue(device, computeFamily, 0, &vctx::ComputeQueue);
        vkGetDeviceQueue(device, transferFamily, 0, &vctx::TransferQueue);
        vkGetDeviceQueue(device, presentFamily, 0, &vctx::PresentQueue);
        vkGetDeviceQueue(device, graphicsAndComputeFamily, 0, &vctx::GraphicsAndComputeQueue);
    }

    i32 w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);

    vctx::RenderExtent.width = w;
    vctx::RenderExtent.height = h;

    pro::SwapchainCreateInfo scio{};
    scio.extent = vctx::RenderExtent;
    scio.ePresentMode = PresentMode;
    scio.physDevice = physDevice;
    scio.surface = surface;
    scio.format = vctx::SwapChainImageFormat;
    scio.eColorSpace = vctx::SwapChainColorSpace;
    scio.requestedImageCount = vctx::SwapChainImageCount;
    pro::CreateSwapChain(device, &scio, &swapchain);

    pro::RenderPassCreateInfo rpi{};
    rpi.format = vctx::SwapChainImageFormat;
    rpi.subpass = 0;
    pro::CreateRenderPass(device, &rpi, &vctx::GlobalRenderPass, 0);

    VkCommandPoolCreateInfo cmdPoolCreateInfo{};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCreateInfo.queueFamilyIndex = vctx::GraphicsFamilyIndex;
    cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;    
	if(vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
		LOG_ERROR("Failed to create command pool");
	}

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = MaxFramesInFlight;
    cmdAllocInfo.commandPool = commandPool;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, drawBuffers);

    u32 requestedImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &requestedImageCount, nullptr);
    VkImage swapchainImages[requestedImageCount];
    renderData.resize(requestedImageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &requestedImageCount, swapchainImages);

    for(u32 i = 0; i < MaxFramesInFlight; i++)
	{
        constexpr VkSemaphoreCreateInfo semaphoreCreateInfo{
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0
        };

        constexpr VkFenceCreateInfo fenceCreateInfo{
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT
        };

        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderData[i].renderingFinishedSemaphore);
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderData[i].imageAvailableSemaphore);
        vkCreateFence(device, &fenceCreateInfo, nullptr, &renderData[i].inFlightFence);
	}

    for (u32 i = 0; i < requestedImageCount; i++) {

        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = vctx::SwapChainImageFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &renderData[i].swapchainImageView) != VK_SUCCESS) {
            LOG_ERROR("Failed to create view for swapchain image %d", i);
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vctx::GlobalRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &renderData[i].swapchainImageView;
        framebufferInfo.width = vctx::RenderExtent.width;
        framebufferInfo.height = vctx::RenderExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &renderData[i].framebuffer) != VK_SUCCESS)
            LOG_ERROR("Failed to create framebuffer for swapchain image %d", i);
    }
}

void Renderer::ProcessEvent(SDL_Event *event)
{
    if ((event->type == SDL_EVENT_QUIT) || (event->type == SDL_EVENT_KEY_DOWN && event->key.scancode == EXIT_KEY))
        running = false;

    if (event->type == SDL_EVENT_WINDOW_RESIZED) {
        resizeRequested = true;
    }

    if (resizeRequested) {
        _SignalResize();
        resizeRequested = false;
    }
}

bool Renderer::BeginRender()
{
    const VkCommandBuffer& drawBuffer = GetDrawBuffer();

    vkWaitForFences(device, 1, &renderData[currentFrame].inFlightFence, VK_TRUE, UINT64_MAX);

    const VkResult imageAcquireResult = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, renderData[currentFrame].imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if(imageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR || imageAcquireResult == VK_SUBOPTIMAL_KHR)
	{
        resizeRequested = true;
		return false;
	}
	else if (imageAcquireResult != VK_SUCCESS && imageAcquireResult != VK_SUBOPTIMAL_KHR) 
	{
		LOG_ERROR("Failed to acquire image from swapchain");
		return false;
    }

    vkResetFences(device, 1, &renderData[currentFrame].inFlightFence);

    constexpr VkClearValue clearValues = { { 0.0f, 0.0f, 0.0f, 1.0f} };

    static VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.framebuffer = renderData[imageIndex].framebuffer;
    renderPassInfo.renderArea.extent = vctx::RenderExtent;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderPass = vctx::GlobalRenderPass;
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

    const VkSemaphore waitSemaphores[] = {renderData[currentFrame].imageAvailableSemaphore};
    const VkSemaphore signalSemaphores[] = {renderData[currentFrame].renderingFinishedSemaphore};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;

    const VkCommandBuffer buffers[] = { drawBuffer };
    submitInfo.commandBufferCount = array_len(buffers);
    submitInfo.pCommandBuffers = buffers;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    vkQueueSubmit(vctx::PresentQueue, 1, &submitInfo, renderData[currentFrame].inFlightFence);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // This is signalSemaphores so that this starts as soon as the signaled semaphores are signaled.
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    
    VkResult result = vkQueuePresentKHR(vctx::PresentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        resizeRequested = true;
    }
    
    currentFrame = ((currentFrame + 1) % MaxFramesInFlight);
}

void Renderer::_SignalResize()
{
    vkDeviceWaitIdle(device);

    constexpr VkSemaphoreCreateInfo semaphoreCreateInfo{
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0
    };

    constexpr VkFenceCreateInfo fenceCreateInfo{
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT
    };

    // adhoc method of resetting them
    for (u32 i = 0; i < MaxFramesInFlight; i++) {
        vkDestroySemaphore(device, renderData[i].imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device, renderData[i].renderingFinishedSemaphore, nullptr);
        vkDestroyFence(device, renderData[i].inFlightFence, nullptr);

        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderData[i].renderingFinishedSemaphore);
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderData[i].imageAvailableSemaphore);
        vkCreateFence(device, &fenceCreateInfo, nullptr, &renderData[i].inFlightFence);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);

    i32 w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    
    vctx::RenderExtent.width = w;
    vctx::RenderExtent.height = h;

    pro::SwapchainCreateInfo scio{};
    scio.extent = vctx::RenderExtent;
    scio.ePresentMode = PresentMode;
    scio.physDevice = physDevice;
    scio.surface = surface;
    scio.format = vctx::SwapChainImageFormat;
    scio.requestedImageCount = vctx::SwapChainImageCount;
    scio.eColorSpace = vctx::SwapChainColorSpace;
    pro::CreateSwapChain(device, &scio, &swapchain);

    for (u32 i = 0; i < vctx::SwapChainImageCount; i++)
    {
        vkDestroyImageView(device, renderData[i].swapchainImageView, nullptr);
        vkDestroyFramebuffer(device, renderData[i].framebuffer, nullptr);
    }

    u32 requestedImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &requestedImageCount, nullptr);
	renderData.resize(requestedImageCount);
    VkImage swapchainImages[requestedImageCount];
	vkGetSwapchainImagesKHR(device, swapchain, &requestedImageCount, swapchainImages);

    vctx::SwapChainImageCount = requestedImageCount;

    for (u32 i = 0; i < requestedImageCount; i++)
    {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchainImages[i];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = vctx::SwapChainImageFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &renderData[i].swapchainImageView) != VK_SUCCESS) {
            LOG_ERROR("Failed to create view for swapchain image %u", i);
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vctx::GlobalRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &renderData[i].swapchainImageView;
        framebufferInfo.width = vctx::RenderExtent.width;
        framebufferInfo.height = vctx::RenderExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &renderData[i].framebuffer) != VK_SUCCESS) {
           LOG_ERROR("Failed to create framebuffer for swapchain image %u", i);
        }
    }

    vctx::OnWindowResized();
}
