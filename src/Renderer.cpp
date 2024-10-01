#include "../include/Renderer.hpp"
#include "../include/ctext.hpp"

#include "../include/stdafx.h"
#include "../include/cvk.hpp"
#include "../../include/vkhelper.hpp"

#include "../include/containers/csignal.hpp"

VkFormat SwapChainImageFormat;
VkColorSpaceKHR SwapChainColorSpace;
u32 SwapChainImageCount = 0;
VkRenderPass GlobalRenderPass = VK_NULL_HANDLE;
// VkRenderPass ShadowPass = VK_NULL_HANDLE;
VkExtent2D RenderExtent = { 0, 0 };
u32 GraphicsFamilyIndex = 0;
u32 PresentFamilyIndex = 0;
u32 ComputeFamilyIndex = 0;
u32 TransferQueueIndex = 0;
u32 GraphicsAndComputeFamilyIndex = 0;
VkQueue GraphicsQueue = VK_NULL_HANDLE;
VkQueue GraphicsAndComputeQueue = VK_NULL_HANDLE;
VkQueue PresentQueue = VK_NULL_HANDLE;
VkQueue ComputeQueue = VK_NULL_HANDLE;
VkQueue TransferQueue = VK_NULL_HANDLE;
csignal OnWindowResized;
VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;

VkSwapchainKHR Renderer::swapchain = VK_NULL_HANDLE;
u32 Renderer::attachment_count = 1;
u32 Renderer::renderer_frame = 0;
u32 Renderer::imageIndex = 0;
VkCommandPool Renderer::commandPool = VK_NULL_HANDLE;
cvector_t *Renderer::renderData;
cvector_t *Renderer::drawBuffers;
renderer_config Renderer::config;

const void *Renderer::empty_array;

VkImage Renderer::color_image = VK_NULL_HANDLE;
VkDeviceMemory Renderer::color_image_memory = VK_NULL_HANDLE;
VkImageView Renderer::color_image_view = VK_NULL_HANDLE;

VkFormat Renderer::depth_buffer_format = VK_FORMAT_UNDEFINED;

ccamera camera(cmdeg2rad(45.0f));

void Renderer::initialize(const renderer_config *conf) {
    Renderer::empty_array = calloc(1, 256);
    memcpy(&config, conf, sizeof(renderer_config));

    /* Initialize graphics singleton */
    {
        if (!help::Images::GetSupportedFormat(device, physDevice, surface, &SwapChainImageFormat, &SwapChainColorSpace)) {
            LOG_AND_ABORT("No supported format for display.");
        }
        SwapChainImageCount = help::Images::GetImageCount(physDevice, surface);

        u32 queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, nullptr);
        cvector<VkQueueFamilyProperties> queueFamilies(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, queueFamilies.data());

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

        GraphicsFamilyIndex = graphicsFamily;
        ComputeFamilyIndex = computeFamily;
        TransferQueueIndex = transferFamily;
        PresentFamilyIndex = presentFamily;
        GraphicsAndComputeFamilyIndex = graphicsAndComputeFamily;

        vkGetDeviceQueue(device, GraphicsFamilyIndex, 0, &GraphicsQueue);
        vkGetDeviceQueue(device, ComputeFamilyIndex, 0, &ComputeQueue);
        vkGetDeviceQueue(device, TransferQueueIndex, 0, &TransferQueue);
        vkGetDeviceQueue(device, PresentFamilyIndex, 0, &PresentQueue);
        vkGetDeviceQueue(device, GraphicsAndComputeFamilyIndex, 0, &GraphicsAndComputeQueue);
    }

    RenderExtent.width = conf->window_size.width;
    RenderExtent.height = conf->window_size.height;

    cvk_swapchain_create_info scio{};
    scio.extent = RenderExtent;
    scio.present_mode = config.present_mode;
    scio.format = SwapChainImageFormat;
    scio.color_space = SwapChainColorSpace;
    scio.image_count = SwapChainImageCount;
    cvk_create_swapchain(device, &scio, &swapchain);

    const VkSampleCountFlagBits samples = config.multisampling_enable ? config.samples : VK_SAMPLE_COUNT_1_BIT;
    Samples = samples;

    if (config.multisampling_enable) {
        cvk_flag_register |= CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING;
        attachment_count++;
    }
    cvk_flag_register |= CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK;
    cvk_flag_register |= CVK_PIPELINE_FLAGS_FORCE_CULLING;
    attachment_count++;

    VkCommandPoolCreateInfo cmdPoolCreateInfo{};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCreateInfo.queueFamilyIndex = GraphicsFamilyIndex;
    cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;    
	if(vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
		LOG_ERROR("Failed to create command pool");
	}

    // how many frames the renderer will render at once
    const int frames_in_flight = 1 + (int)config.buffer_mode;

    drawBuffers = cvector_init(sizeof(VkCommandBuffer), frames_in_flight);
    renderData = cvector_init(sizeof(FrameRenderData), frames_in_flight);

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = frames_in_flight;
    cmdAllocInfo.commandPool = commandPool;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, (VkCommandBuffer *)cvector_data(drawBuffers));

    depth_buffer_format = VK_FORMAT_D16_UNORM; // replace (probably)

    cvk_render_pass_create_info rpi{};
    rpi.format = SwapChainImageFormat;
    rpi.depthBufferFormat = depth_buffer_format;
    rpi.subpass = 0;
    rpi.samples = samples;
    cvk_create_render_pass(device, &rpi, &GlobalRenderPass, cvk_flag_register);

    // cvk_render_pass_create_info depth_render_pass_info{};
    // depth_render_pass_info.format = VK_FORMAT_D16_UNORM;
    // depth_render_pass_info.depthBufferFormat = depth_buffer_format;
    // depth_render_pass_info.subpass = 0;
    // depth_render_pass_info.samples = VK_SAMPLE_COUNT_1_BIT;
    // cvk_create_render_pass(device, &depth_render_pass_info, &ShadowPass, cvk_flag_register);
    // {
    //     VkAttachmentDescription depthAttachment{};
    //     depthAttachment.format = VK_FORMAT_D16_UNORM;
    //     depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    //     depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //     depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //     depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //     depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //     depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //     depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //     VkAttachmentReference depthref{};
    //     depthref.attachment = 0;
    //     depthref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //     VkSubpassDescription subpass{};
    //     subpass.colorAttachmentCount = 0;
    //     subpass.pDepthStencilAttachment = &depthref;

    //     VkRenderPassCreateInfo depth_pass{};
    //     depth_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    //     depth_pass.attachmentCount = 1;
    //     depth_pass.pAttachments = &depthAttachment;
    //     depth_pass.subpassCount = 1;
    //     depth_pass.pSubpasses = &subpass;
    //     vkCreateRenderPass(device, &depth_pass, nullptr, &ShadowPass);
    // }

    _create_optional_images();
    _create_framebuffers_and_swapchain_image_views();

    for(int i = 0; i < frames_in_flight; i++)
	{
        constexpr VkSemaphoreCreateInfo semaphoreCreateInfo{
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0
        };

        constexpr VkFenceCreateInfo fenceCreateInfo{
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, VK_FENCE_CREATE_SIGNALED_BIT
        };

        FrameRenderData &data = *(FrameRenderData *)cvector_get(renderData, i);
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &data.renderingFinishedSemaphore);
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &data.imageAvailableSemaphore);
        vkCreateFence(device, &fenceCreateInfo, nullptr, &data.inFlightFence);
	}
}

bool Renderer::BeginRender()
{
    FrameRenderData &data = *(FrameRenderData *)cvector_get(renderData, renderer_frame);

    vkWaitForFences(device, 1, &data.inFlightFence, VK_TRUE, UINT64_MAX);

    const VkResult imageAcquireResult = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, data.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    const VkCommandBuffer& drawBuffer = *(VkCommandBuffer *)cvector_get(drawBuffers, renderer_frame);

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

    vkResetFences(device, 1, &data.inFlightFence);

    VkClearValue clearValues[2];
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkFramebuffer &fb = (*(FrameRenderData *)(cvector_get(renderData, imageIndex))).color_framebuffer;
 
    static VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.framebuffer = fb;
    renderPassInfo.renderArea.extent = RenderExtent;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderPass = GlobalRenderPass;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    constexpr VkCommandBufferBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, 0, nullptr
    };

    vkBeginCommandBuffer(drawBuffer, &beginInfo);
    vkCmdBeginRenderPass(drawBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = RenderExtent.width;
    viewport.height = RenderExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(drawBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = { RenderExtent.width, RenderExtent.height };
    vkCmdSetScissor(drawBuffer, 0, 1, &scissor);

    return true;
}

void Renderer::EndRender()
{
    const VkCommandBuffer& drawBuffer = *(VkCommandBuffer *)cvector_get(drawBuffers, renderer_frame);

    vkCmdEndRenderPass(drawBuffer);
    vkEndCommandBuffer(drawBuffer);

	VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    FrameRenderData &data = *(FrameRenderData *)cvector_get(renderData, renderer_frame);
    const VkSemaphore waitSemaphores[] = {data.imageAvailableSemaphore};
    const VkSemaphore signalSemaphores[] = {data.renderingFinishedSemaphore};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;

    const VkCommandBuffer buffers[] = { drawBuffer };
    submitInfo.commandBufferCount = array_len(buffers);
    submitInfo.pCommandBuffers = buffers;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(PresentQueue, 1, &submitInfo, data.inFlightFence);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // This is signalSemaphores so that this starts as soon as the signaled semaphores are signaled.
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;

    const VkResult result = vkQueuePresentKHR(PresentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || cengine::get_frame_buffer_resized())
        _SignalResize();

    renderer_frame = (renderer_frame + 1) % (1 + (int)config.buffer_mode);
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
    for (int i = 0; i < (1 + (int)config.buffer_mode); i++) {
        FrameRenderData &data = *(FrameRenderData *)cvector_get(renderData, i);
        vkDestroySemaphore(device, data.imageAvailableSemaphore, nullptr);
        vkDestroySemaphore(device, data.renderingFinishedSemaphore, nullptr);
        vkDestroyFence(device, data.inFlightFence, nullptr);
    }
    for (u32 i = 0; i < SwapChainImageCount; i++)
    {
        FrameRenderData &data = *(FrameRenderData *)cvector_get(renderData, i);
        vkDestroyImageView(device, data.swapchainImageView, nullptr);
        vkDestroyFramebuffer(device, data.color_framebuffer, nullptr);
    }
    cvector_clear(renderData);

    i32 w, h;
    SDL_Vulkan_GetDrawableSize(window, &w, &h);

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &surface_capabilities);

    const u32 min_width = surface_capabilities.minImageExtent.width;
    const u32 min_height = surface_capabilities.minImageExtent.height;

    const u32 max_width = surface_capabilities.maxImageExtent.width;
    const u32 max_height = surface_capabilities.maxImageExtent.height;

    w = cmclamp((u32)w, min_width, max_width);
    h = cmclamp((u32)h, min_height, max_height);

    RenderExtent = { (u32)w, (u32)h };

    VkSwapchainKHR old_swapchain = swapchain;

    cvk_swapchain_create_info scio{};
    scio.extent = RenderExtent;
    scio.present_mode = config.present_mode;
    scio.format = SwapChainImageFormat;
    scio.color_space = SwapChainColorSpace;
    scio.image_count = SwapChainImageCount;
    scio.old_swapchain = old_swapchain;
    cvk_create_swapchain(device, &scio, &swapchain);
    vkDestroySwapchainKHR(device, old_swapchain, nullptr);

    vkDestroyImage(device, color_image, nullptr);
    vkDestroyImageView(device, color_image_view, nullptr);

    _create_optional_images();
    _create_framebuffers_and_swapchain_image_views();

    for (u32 i = 0; i < SwapChainImageCount; i++)
    {
        FrameRenderData &data = *(FrameRenderData *)cvector_get(renderData, i);
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &data.renderingFinishedSemaphore);
        vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &data.imageAvailableSemaphore);
        vkCreateFence(device, &fenceCreateInfo, nullptr, &data.inFlightFence);
    }

    cengine::_reset_frame_buffer_resized();

    OnWindowResized.signal();
}

void Renderer::_create_optional_images()
{
    vkGetSwapchainImagesKHR(device, swapchain, &SwapChainImageCount, nullptr);
    VkImage swapchainImages[SwapChainImageCount];
    cvector_resize(renderData, SwapChainImageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &SwapChainImageCount, swapchainImages);

    if (config.multisampling_enable) {
        help::Images::create_empty(
            RenderExtent.width, RenderExtent.height,
            SwapChainImageFormat,
            Samples,
            4,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            &color_image, &color_image_memory
        );

        VkImageViewCreateInfo resolve_view_create_info{};
        resolve_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        resolve_view_create_info.image = color_image;
        resolve_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        resolve_view_create_info.format = SwapChainImageFormat;
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
    for (int i = 0; i < SwapChainImageCount; i++) {
        FrameRenderData &data = *(FrameRenderData *)cvector_get(renderData, i);
        data.swapchainImage = swapchainImages[i];

        VkImageCreateInfo image{};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.extent.width = RenderExtent.width;
        image.extent.height = RenderExtent.height;
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = Samples;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.format = VK_FORMAT_D16_UNORM;
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ;
        vkCreateImage(device, &image, nullptr, &data.depth_image);

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, data.depth_image, &memReqs);

        VkMemoryAllocateInfo memAlloc{};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = help::GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vkAllocateMemory(device, &memAlloc, nullptr, &data.shadow_image_memory);
        vkBindImageMemory(device, data.depth_image, data.shadow_image_memory, 0);

        VkImageViewCreateInfo depthStencilView{};
        depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthStencilView.format = VK_FORMAT_D16_UNORM;
        depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthStencilView.subresourceRange.baseMipLevel = 0;
        depthStencilView.subresourceRange.levelCount = 1;
        depthStencilView.subresourceRange.baseArrayLayer = 0;
        depthStencilView.subresourceRange.layerCount = 1;
        depthStencilView.image = data.depth_image;
        vkCreateImageView(device, &depthStencilView, nullptr, &data.depth_image_view);
    }
}

void Renderer::_create_framebuffers_and_swapchain_image_views()
{
    cvector<VkImageView> attachments(3);

    for (int i = 0; i < (int)SwapChainImageCount; i++) {
        FrameRenderData &data = *(FrameRenderData *)cvector_get(renderData, i);
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = data.swapchainImage;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = SwapChainImageFormat;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &data.swapchainImageView) != VK_SUCCESS) {
            LOG_ERROR("Failed to create view for swapchain image %d", i);
        }

        const VkImageView &swapchain_image_view = data.swapchainImageView;
        const VkImageView &depth_image_view = data.depth_image_view;

        if (config.multisampling_enable)
            attachments = { color_image_view, depth_image_view, swapchain_image_view };
        else
            attachments = { swapchain_image_view, depth_image_view };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = GlobalRenderPass;
        framebufferInfo.attachmentCount = attachments.length();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = RenderExtent.width;
        framebufferInfo.height = RenderExtent.height;
        framebufferInfo.layers = 1;
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &data.color_framebuffer) != VK_SUCCESS)
            LOG_ERROR("Failed to create framebuffer for swapchain image %d", i);
    }
}
