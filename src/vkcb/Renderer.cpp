#include "../../include/vkcb/Renderer.hpp"
#include "../../include/engine/ctext/ctext.hpp"

#include "../../include/stdafx.hpp"
#include "../../include/vkcb/pro.hpp"
#include "../../include/vkcb/Bootstrap.hpp"
#include "../../include/vkcb/epichelperlib.hpp"

VkFormat VulkanContextSingleton::SwapChainImageFormat;
VkColorSpaceKHR VulkanContextSingleton::SwapChainColorSpace;
u32 VulkanContextSingleton::SwapChainImageCount = 0;
VkRenderPass VulkanContextSingleton::GlobalRenderPass = VK_NULL_HANDLE;
VkExtent2D VulkanContextSingleton::RenderExtent = { 0, 0 };
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
csignal VulkanContextSingleton::OnWindowResized;
VkSampleCountFlagBits VulkanContextSingleton::Samples = VK_SAMPLE_COUNT_1_BIT;

VkSwapchainKHR Renderer::swapchain = VK_NULL_HANDLE;
u32 Renderer::attachment_count = 1;
u32 Renderer::renderer_frame = 0;
u32 Renderer::imageIndex = 0;
VkCommandPool Renderer::commandPool = VK_NULL_HANDLE;
cvector<FrameRenderData> Renderer::renderData;
cvector<VkCommandBuffer> Renderer::drawBuffers;
renderer_config Renderer::config;

const void *Renderer::empty_array;

VkImage Renderer::color_image = VK_NULL_HANDLE;
VkDeviceMemory Renderer::color_image_memory = VK_NULL_HANDLE;
VkImageView Renderer::color_image_view = VK_NULL_HANDLE;

VkFormat Renderer::depth_buffer_format = VK_FORMAT_UNDEFINED;
VkImage Renderer::depth_image = VK_NULL_HANDLE;
VkImageView Renderer::depth_image_view = VK_NULL_HANDLE;
VkDeviceMemory Renderer::depth_image_memory = VK_NULL_HANDLE;

ccamera camera(glm::radians(45.0f));

void Renderer::initialize(const renderer_config *conf) {
    Renderer::empty_array = calloc(1, 256);
    memcpy(&config, conf, sizeof(renderer_config));

    SDL_PumpEvents();
    i32 w, h;
    SDL_Vulkan_GetDrawableSize(window, &w, &h);
    vctx::RenderExtent = { (u32)w, (u32)h };

    /* Initialize graphics singleton */
    {
        if (!help::Images::GetSupportedFormat(device, physDevice, surface, &vctx::SwapChainImageFormat, &vctx::SwapChainColorSpace)) {
            LOG_AND_ABORT("No supported format for display.");
        }
        vctx::SwapChainImageCount = help::Images::GetImageCount(physDevice, surface);

        u32 queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, nullptr);
        VkQueueFamilyProperties *queueFamilies = new VkQueueFamilyProperties[queueCount];
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, queueFamilies);

        u32 graphicsFamily = 0, graphicsAndComputeFamily = 0, presentFamily = 0, computeFamily = 0, transferFamily = 0;
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

        vkGetDeviceQueue(device, vctx::GraphicsFamilyIndex, 0, &vctx::GraphicsQueue);
        vkGetDeviceQueue(device, vctx::ComputeFamilyIndex, 0, &vctx::ComputeQueue);
        vkGetDeviceQueue(device, vctx::TransferQueueIndex, 0, &vctx::TransferQueue);
        vkGetDeviceQueue(device, vctx::PresentFamilyIndex, 0, &vctx::PresentQueue);
        vkGetDeviceQueue(device, vctx::GraphicsAndComputeFamilyIndex, 0, &vctx::GraphicsAndComputeQueue);
    }

    pro::SwapchainCreateInfo scio{};
    scio.extent = vctx::RenderExtent;
    scio.ePresentMode = config.present_mode;
    scio.physDevice = physDevice;
    scio.surface = surface;
    scio.format = vctx::SwapChainImageFormat;
    scio.eColorSpace = vctx::SwapChainColorSpace;
    scio.requestedImageCount = vctx::SwapChainImageCount;
    pro::CreateSwapChain(device, &scio, &swapchain);

    const VkSampleCountFlagBits samples = config.multisampling_enable ? config.samples : VK_SAMPLE_COUNT_1_BIT;
    vctx::Samples = config.samples;

    PipelineCreateFlags renderpass_create_flags = 0;

    if (config.multisampling_enable) {
        renderpass_create_flags |= PIPELINE_CREATE_FLAGS_ENABLE_MULTISAMPLING;
        attachment_count++;
    }
    if (config.depth_buffer_enable) {
        renderpass_create_flags |= PIPELINE_CREATE_FLAGS_ENABLE_DEPTH_CHECK;
        attachment_count++;
    }

    VkCommandPoolCreateInfo cmdPoolCreateInfo{};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCreateInfo.queueFamilyIndex = vctx::GraphicsFamilyIndex;
    cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;    
	if(vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
		LOG_ERROR("Failed to create command pool");
	}

    drawBuffers.resize(config.max_frames_in_flight);

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = config.max_frames_in_flight;
    cmdAllocInfo.commandPool = commandPool;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, drawBuffers.data());

    depth_buffer_format = VK_FORMAT_D32_SFLOAT; // replace (probably)
    vctx::RenderExtent.width = conf->window_size.width;
    vctx::RenderExtent.height = conf->window_size.height;
    _create_optional_images();

    pro::RenderPassCreateInfo rpi{};
    rpi.format = vctx::SwapChainImageFormat;
    rpi.depthBufferFormat = depth_buffer_format;
    rpi.subpass = 0;
    rpi.samples = samples;
    pro::CreateRenderPass(device, &rpi, &vctx::GlobalRenderPass, renderpass_create_flags);

    _create_framebuffers_and_swapchain_image_views();

    for(u32 i = 0; i < config.max_frames_in_flight; i++)
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
}

bool Renderer::BeginRender()
{
    vkWaitForFences(device, 1, &renderData.at(renderer_frame).inFlightFence, VK_TRUE, UINT64_MAX);

    const VkResult imageAcquireResult = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, renderData.at(renderer_frame).imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    const VkCommandBuffer& drawBuffer = drawBuffers.at(renderer_frame);

	if(imageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR || imageAcquireResult == VK_SUBOPTIMAL_KHR || cengine::get_frame_buffer_resized())
	{
        _SignalResize();
		return false;
	}
	else if (imageAcquireResult != VK_SUCCESS && imageAcquireResult != VK_SUBOPTIMAL_KHR) 
	{
		LOG_ERROR("Failed to acquire image from swapchain");
		return false;
    }

    vkResetFences(device, 1, &renderData.at(renderer_frame).inFlightFence);

    VkClearValue clearValues[2];
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = { 1.0, 0 };
 
    static VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.framebuffer = renderData.at(imageIndex).framebuffer;
    renderPassInfo.renderArea.extent = vctx::RenderExtent;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderPass = vctx::GlobalRenderPass;
    renderPassInfo.clearValueCount = config.depth_buffer_enable ? 2 : 1;
    renderPassInfo.pClearValues = clearValues;

    constexpr static VkCommandBufferBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr
    };

    vkBeginCommandBuffer(drawBuffer, &beginInfo);
    vkCmdBeginRenderPass(drawBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = vctx::RenderExtent.width;
    viewport.height = vctx::RenderExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(drawBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { vctx::RenderExtent.width, vctx::RenderExtent.height };
    vkCmdSetScissor(drawBuffer, 0, 1, &scissor);

    return true;
}

void Renderer::EndRender()
{
    const auto& drawBuffer = drawBuffers.at(renderer_frame);

    vkCmdEndRenderPass(drawBuffer);
    vkEndCommandBuffer(drawBuffer);

	VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    const VkSemaphore waitSemaphores[] = {renderData[renderer_frame].imageAvailableSemaphore};
    const VkSemaphore signalSemaphores[] = {renderData[renderer_frame].renderingFinishedSemaphore};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;

    const VkCommandBuffer buffers[] = { drawBuffer };
    submitInfo.commandBufferCount = array_len(buffers);
    submitInfo.pCommandBuffers = buffers;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(vctx::PresentQueue, 1, &submitInfo, renderData.at(renderer_frame).inFlightFence);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // This is signalSemaphores so that this starts as soon as the signaled semaphores are signaled.
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;

    const VkResult result = vkQueuePresentKHR(vctx::PresentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || cengine::get_frame_buffer_resized())
        _SignalResize();

    renderer_frame = (renderer_frame + 1) % config.max_frames_in_flight;
}

void Renderer::_SignalResize()
{
    vkDeviceWaitIdle(device);

    constexpr VkSemaphoreCreateInfo semaphoreCreateInfo {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0
    };

    constexpr VkFenceCreateInfo fenceCreateInfo {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT
    };
    
    // adhoc method of resetting them
    for (u32 i = 0; i < config.max_frames_in_flight; i++) {
        vkDestroySemaphore(device, renderData[i].imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device, renderData[i].renderingFinishedSemaphore, nullptr);
        vkDestroyFence(device, renderData[i].inFlightFence, nullptr);
    }
    for (u32 i = 0; i < vctx::SwapChainImageCount; i++)
    {
        vkDestroyImageView(device, renderData[i].swapchainImageView, nullptr);
        vkDestroyFramebuffer(device, renderData[i].framebuffer, nullptr);
    }
    renderData.clear();

    i32 w, h;
    SDL_Vulkan_GetDrawableSize(window, &w, &h);

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &surface_capabilities);

    const u32 min_width = surface_capabilities.minImageExtent.width;
    const u32 min_height = surface_capabilities.minImageExtent.height;

    const u32 max_width = surface_capabilities.maxImageExtent.width;
    const u32 max_height = surface_capabilities.maxImageExtent.height;

    w = std::clamp((u32)w, min_width, max_width);
    h = std::clamp((u32)h, min_height, max_height);

    vctx::RenderExtent = { (u32)w, (u32)h };

    VkSwapchainKHR old_swapchain = swapchain;

    pro::SwapchainCreateInfo scio{};
    scio.extent = vctx::RenderExtent;
    scio.ePresentMode = config.present_mode;
    scio.physDevice = physDevice;
    scio.surface = surface;
    scio.format = vctx::SwapChainImageFormat;
    scio.requestedImageCount = vctx::SwapChainImageCount;
    scio.eColorSpace = vctx::SwapChainColorSpace;
    scio.pOldSwapchain = swapchain;
    pro::CreateSwapChain(device, &scio, &swapchain);
    vkDestroySwapchainKHR(device, old_swapchain, nullptr);

    vkDestroyImage(device, color_image, nullptr);
    vkDestroyImage(device, depth_image, nullptr);
    vkDestroyImageView(device, color_image_view, nullptr);
    vkDestroyImageView(device, depth_image_view, nullptr);

    _create_optional_images();
    _create_framebuffers_and_swapchain_image_views();

    for (u32 i = 0; i < vctx::SwapChainImageCount; i++)
    {
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderData[i].renderingFinishedSemaphore);
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderData[i].imageAvailableSemaphore);
        vkCreateFence(device, &fenceCreateInfo, nullptr, &renderData[i].inFlightFence);
    }

    cengine::_reset_frame_buffer_resized();

    vctx::OnWindowResized.signal();
}

void Renderer::_create_optional_images()
{
    if (config.multisampling_enable) {
        help::Images::create_empty(
            vctx::RenderExtent.width, vctx::RenderExtent.height,
            vctx::SwapChainImageFormat,
            config.samples,
            4,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            &color_image, &color_image_memory
        );

        VkImageViewCreateInfo resolve_view_create_info{};
        resolve_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        resolve_view_create_info.image = color_image;
        resolve_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        resolve_view_create_info.format = vctx::SwapChainImageFormat;
        resolve_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        resolve_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        resolve_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        resolve_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        resolve_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        resolve_view_create_info.subresourceRange.baseMipLevel = 0;
        resolve_view_create_info.subresourceRange.levelCount = 1;
        resolve_view_create_info.subresourceRange.baseArrayLayer = 0;
        resolve_view_create_info.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &resolve_view_create_info, nullptr, &color_image_view) != VK_SUCCESS) {
            LOG_ERROR("Failed to create view for resolve image");
        }
    }

    // attachment vector will be like <color resolve, depth attachment, swapchain image>
    if (config.depth_buffer_enable) {
        help::Images::create_empty(
            vctx::RenderExtent.width, vctx::RenderExtent.height,
            depth_buffer_format,
            config.samples,
            1,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            &depth_image, &depth_image_memory
        );

        VkCommandBuffer cmd = help::Commands::BeginSingleTimeCommands();
        help::Images::TransitionImageLayout(
            cmd, depth_image, 1, VK_IMAGE_ASPECT_DEPTH_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
        );
        help::Commands::EndSingleTimeCommands(cmd);

        VkImageViewCreateInfo depth_view_create_info{};
        depth_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depth_view_create_info.image = depth_image;
        depth_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depth_view_create_info.format = depth_buffer_format;
        depth_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        depth_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        depth_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        depth_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        depth_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depth_view_create_info.subresourceRange.baseMipLevel = 0;
        depth_view_create_info.subresourceRange.levelCount = 1;
        depth_view_create_info.subresourceRange.baseArrayLayer = 0;
        depth_view_create_info.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &depth_view_create_info, nullptr, &depth_image_view) != VK_SUCCESS) {
            LOG_ERROR("Failed to create view for depth image");
        }
    }
}

void Renderer::_create_framebuffers_and_swapchain_image_views()
{
    u32 requestedImageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &requestedImageCount, nullptr);
    VkImage swapchainImages[requestedImageCount];
    renderData.resize(requestedImageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &requestedImageCount, swapchainImages);

    cvector<VkImageView> attachments(3);

    for (int i = 0; i < (int)requestedImageCount; i++) {
        renderData[i].swapchainImage = swapchainImages[i];

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

        const VkImageView &swapchain_image_view = renderData[i].swapchainImageView;

        if (config.depth_buffer_enable) {
            if (config.multisampling_enable)
                attachments = { color_image_view, depth_image_view, swapchain_image_view };
            else
                attachments = { swapchain_image_view, depth_image_view };
        }
        else if (config.multisampling_enable)
            attachments = { color_image_view, swapchain_image_view };
        else
            attachments = { swapchain_image_view };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vctx::GlobalRenderPass;
        framebufferInfo.attachmentCount = attachments.length();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = vctx::RenderExtent.width;
        framebufferInfo.height = vctx::RenderExtent.height;
        framebufferInfo.layers = 1;
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &renderData[i].framebuffer) != VK_SUCCESS)
            LOG_ERROR("Failed to create framebuffer for swapchain image %d", i);
    }
}
