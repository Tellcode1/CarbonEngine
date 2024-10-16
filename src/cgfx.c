#include "../include/cgfx.h"
#include "../include/cgfxdef.h"

#include "../include/vkstdafx.h"
#include "../include/stdafx.h"
#include "../include/math/math.h"
#include "../include/cvk.h"
#include "../include/cengine.h"
#include "../include/vkhelper.h"

#include "../include/cgvector.h"

#include <SDL2/SDL_vulkan.h>

// TODO: move to internal cengine struct owned by renderer. user should query from renderer for these
VkFormat SwapChainImageFormat;
VkColorSpaceKHR SwapChainColorSpace;
u32 SwapChainImageCount = 0;
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
VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;

int crd_get_renderer_frame(const crenderer_t *rd)
{
    return rd->renderer_frame;
}

VkCommandBuffer crd_get_drawbuffer(const crenderer_t *rd)
{
    return *(VkCommandBuffer *)cg_vector_get(rd->drawBuffers, rd->renderer_frame);
}

VkRenderPass crd_get_render_pass(const crenderer_t *rd)
{
    return rd->render_pass;
}

cg_extent2d crd_get_render_extent(const crenderer_t *rd)
{
    return rd->render_extent;
}

void create_optional_images(crenderer_t *rd)
{
    vkGetSwapchainImagesKHR(rd->device->device, rd->swapchain, &SwapChainImageCount, NULL);
    VkImage *swapchainImages = (VkImage *)malloc(SwapChainImageCount * sizeof(VkImage));
	vkGetSwapchainImagesKHR(rd->device->device, rd->swapchain, &SwapChainImageCount, swapchainImages);

    cg_vector_resize(rd->renderData, SwapChainImageCount);

    if (rd->config.multisampling_enable) {
        vkh_image_create_empty(
            rd->device,
            rd->render_extent.width, rd->render_extent.height,
            SwapChainImageFormat,
            Samples,
            4,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            &rd->color_image, &rd->color_image_memory
        );

        VkImageViewCreateInfo resolve_view_create_info = {};
        resolve_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        resolve_view_create_info.image = rd->color_image;
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
        if (vkCreateImageView(rd->device->device, &resolve_view_create_info, NULL, &rd->color_image_view) != VK_SUCCESS) {
            LOG_ERROR("Failed to create view for resolve image");
        }
    }

    // attachment vector will be like <color resolve, depth attachment, swapchain image>
    for (int i = 0; i < (int)SwapChainImageCount; i++) {
        cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(rd->renderData, i);
        data->swapchainImage = swapchainImages[i];

        VkImageCreateInfo image = {};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.extent.width = rd->render_extent.width;
        image.extent.height = rd->render_extent.height;
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = 1;
        image.samples = Samples;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.format = VK_FORMAT_D16_UNORM;
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ;
        vkCreateImage(rd->device->device, &image, NULL, &data->depth_image);

        VkMemoryRequirements memReqs = {};
        vkGetImageMemoryRequirements(rd->device->device, data->depth_image, &memReqs);

        VkMemoryAllocateInfo memAlloc = {};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = vkh_get_mem_type(rd->device, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vkAllocateMemory(rd->device->device, &memAlloc, NULL, &data->shadow_image_memory);
        vkBindImageMemory(rd->device->device, data->depth_image, data->shadow_image_memory, 0);

        VkImageViewCreateInfo depthStencilView = {};
        depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthStencilView.format = VK_FORMAT_D16_UNORM;
        depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthStencilView.subresourceRange.baseMipLevel = 0;
        depthStencilView.subresourceRange.levelCount = 1;
        depthStencilView.subresourceRange.baseArrayLayer = 0;
        depthStencilView.subresourceRange.layerCount = 1;
        depthStencilView.image = data->depth_image;
        vkCreateImageView(rd->device->device, &depthStencilView, NULL, &data->depth_image_view);
    }

    free(swapchainImages);
}

void create_framebuffers_and_swapchain_image_views(crenderer_t *rd) {
    cg_vector_t * /* VkImageView */ attachments = cg_vector_init(sizeof(VkImageView), 3);

    for (int i = 0; i < (int)SwapChainImageCount; i++) {
        cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(rd->renderData, i);
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = data->swapchainImage;
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
        if (vkCreateImageView(rd->device->device, &imageViewCreateInfo, NULL, &data->swapchainImageView) != VK_SUCCESS) {
            LOG_ERROR("Failed to create view for swapchain image %d", i);
        }

        const VkImageView swapchain_image_view = data->swapchainImageView;
        const VkImageView depth_image_view = data->depth_image_view;

        cg_vector_clear(attachments);
        if (rd->config.multisampling_enable)
            cg_vector_push_set(attachments, (VkImageView[]){ rd->color_image_view, depth_image_view, swapchain_image_view }, 3);
        else
            cg_vector_push_set(attachments, (VkImageView[]){ swapchain_image_view, depth_image_view }, 2);

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = rd->render_pass;
        framebufferInfo.attachmentCount = cg_vector_size(attachments);
        framebufferInfo.pAttachments = (VkImageView *)cg_vector_data(attachments);
        framebufferInfo.width = rd->render_extent.width;
        framebufferInfo.height = rd->render_extent.height;
        framebufferInfo.layers = 1;
        if (vkCreateFramebuffer(rd->device->device, &framebufferInfo, NULL, &data->color_framebuffer) != VK_SUCCESS)
            LOG_ERROR("Failed to create framebuffer for swapchain image %d", i);
    }

    cg_vector_destroy(attachments);
}

crenderer_t *crenderer_init(cg_device_t *device, const crenderer_config *conf)
{
    struct crenderer_t *rd = (crenderer_t *)calloc(1, sizeof(struct crenderer_t));
    rd->device = device;

    memcpy(&rd->config, conf, sizeof(crenderer_config));

    /* Initialize graphics singleton */
    {
        if (!vkh_get_supported_fmt(rd->device, rd->device->physDevice, rd->device->surface, &SwapChainImageFormat, &SwapChainColorSpace)) {
            LOG_AND_ABORT("No supported format for display.");
        }
        SwapChainImageCount = vkh_get_image_count(rd->device->physDevice, rd->device->surface);

        u32 queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(rd->device->physDevice, &queueCount, NULL);
        cg_vector_t * /* VkQueueFamilyProperties */ queueFamilies = cg_vector_init(sizeof(VkQueueFamilyProperties), queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(rd->device->physDevice, &queueCount, (VkQueueFamilyProperties *)cg_vector_data(queueFamilies));

        u32 graphicsFamily = 0, graphicsAndComputeFamily = 0, presentFamily = 0, computeFamily = 0, transferFamily = 0;
        bool foundGraphicsFamily = false, foundGraphicsAndComputeFamily = false, foundPresentFamily = false, foundComputeFamily = false, foundTransferFamily = false;

        u32 i = 0;
        for (u32 j = 0; j < queueCount; j++) {
            const VkQueueFamilyProperties queueFamily = ((VkQueueFamilyProperties *)cg_vector_data(queueFamilies))[j];
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(rd->device->physDevice, i, rd->device->surface, &presentSupport);
            
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

        vkGetDeviceQueue(rd->device->device, GraphicsFamilyIndex, 0, &GraphicsQueue);
        vkGetDeviceQueue(rd->device->device, ComputeFamilyIndex, 0, &ComputeQueue);
        vkGetDeviceQueue(rd->device->device, TransferQueueIndex, 0, &TransferQueue);
        vkGetDeviceQueue(rd->device->device, PresentFamilyIndex, 0, &PresentQueue);
        vkGetDeviceQueue(rd->device->device, GraphicsAndComputeFamilyIndex, 0, &GraphicsAndComputeQueue);

        cg_vector_destroy(queueFamilies);
    }

    rd->render_extent.width = conf->initial_window_size.width;
    rd->render_extent.height = conf->initial_window_size.height;

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

    if (conf->vsync_enabled) {
        switch (conf->buffer_mode) {
            case CGFX_BUFFER_MODE_SINGLE_BUFFERED:
            case CGFX_BUFFER_MODE_DOUBLE_BUFFERED:
            default:
                present_mode = VK_PRESENT_MODE_FIFO_KHR;
                break;
            case CGFX_BUFFER_MODE_TRIPLE_BUFFERED:
                present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            break;
        };
    } else {
        present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    cvk_swapchain_create_info scio = {};
    scio.extent.width = rd->render_extent.width;
    scio.extent.height = rd->render_extent.height;
    scio.present_mode = present_mode;
    scio.format = SwapChainImageFormat;
    scio.color_space = SwapChainColorSpace;
    scio.image_count = SwapChainImageCount;
    cvk_create_swapchain(rd->device, &scio, &rd->swapchain);

    VkSampleCountFlagBits conf_samples;
    if (conf->samples == CGFX_SAMPLE_COUNT_MAX_SUPPORTED) {
        conf_samples = MAX_SAMPLES;
    } else {
        conf_samples = (VkSampleCountFlagBits)conf->samples;
    }
    const VkSampleCountFlagBits samples = conf->multisampling_enable ? conf_samples : VK_SAMPLE_COUNT_1_BIT;
    Samples = samples;

    if (conf->multisampling_enable) {
        cvk_flag_register |= CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING;
        rd->attachment_count++;
    }
    cvk_flag_register |= CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK;
    cvk_flag_register |= CVK_PIPELINE_FLAGS_FORCE_CULLING;
    rd->attachment_count++;

    VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCreateInfo.queueFamilyIndex = GraphicsFamilyIndex;
    cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;    
	if(vkCreateCommandPool(rd->device->device, &cmdPoolCreateInfo, NULL, &rd->commandPool) != VK_SUCCESS) {
		LOG_ERROR("Failed to create command pool");
	}

    // how many frames the renderer will render at once
    const int frames_in_flight = 1 + (int)conf->buffer_mode;

    rd->drawBuffers = cg_vector_init(sizeof(VkCommandBuffer), frames_in_flight);
    rd->renderData = cg_vector_init(sizeof(cg_framerender_data), frames_in_flight);

    cg_framerender_data data = {};
    for (int i = 0; i < frames_in_flight; i++) {
        cg_vector_push_back(rd->drawBuffers, &data);
        cg_vector_push_back(rd->renderData, &data);
    }

    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = frames_in_flight;
    cmdAllocInfo.commandPool = rd->commandPool;
    vkAllocateCommandBuffers(rd->device->device, &cmdAllocInfo, (VkCommandBuffer *)cg_vector_data(rd->drawBuffers));

    rd->depth_buffer_format = VK_FORMAT_D16_UNORM; // replace (probably)

    cvk_render_pass_create_info rpi = {};
    rpi.format = SwapChainImageFormat;
    rpi.depthBufferFormat = rd->depth_buffer_format;
    rpi.subpass = 0;
    rpi.samples = samples;
    cvk_create_render_pass(rd->device, &rpi, &rd->render_pass, cvk_flag_register);

    // cvk_render_pass_create_info depth_render_pass_info = {};
    // depth_render_pass_info.format = VK_FORMAT_D16_UNORM;
    // depth_render_pass_info.depthBufferFormat = depth_buffer_format;
    // depth_render_pass_info.subpass = 0;
    // depth_render_pass_info.samples = VK_SAMPLE_COUNT_1_BIT;
    // cvk_create_render_pass(device, &depth_render_pass_info, &ShadowPass, cvk_flag_register);
    // {
    //     VkAttachmentDescription depthAttachment = {};
    //     depthAttachment.format = VK_FORMAT_D16_UNORM;
    //     depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    //     depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //     depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //     depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //     depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //     depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //     depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //     VkAttachmentReference depthref = {};
    //     depthref.attachment = 0;
    //     depthref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    //     VkSubpassDescription subpass = {};
    //     subpass.colorAttachmentCount = 0;
    //     subpass.pDepthStencilAttachment = &depthref;

    //     VkRenderPassCreateInfo depth_pass = {};
    //     depth_pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    //     depth_pass.attachmentCount = 1;
    //     depth_pass.pAttachments = &depthAttachment;
    //     depth_pass.subpassCount = 1;
    //     depth_pass.pSubpasses = &subpass;
    //     vkCreateRenderPass(device, &depth_pass, NULL, &ShadowPass);
    // }

    create_optional_images(rd);
    create_framebuffers_and_swapchain_image_views(rd);

    for(int i = 0; i < frames_in_flight; i++)
	{
        const VkSemaphoreCreateInfo semaphoreCreateInfo = {
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0
        };

        const VkFenceCreateInfo fenceCreateInfo = {
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT
        };

        cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(rd->renderData, i);
        vkCreateSemaphore(rd->device->device, &semaphoreCreateInfo, NULL, &data->renderingFinishedSemaphore);
        vkCreateSemaphore(rd->device->device, &semaphoreCreateInfo, NULL, &data->imageAvailableSemaphore);
        vkCreateFence(rd->device->device, &fenceCreateInfo, NULL, &data->inFlightFence);
	}

    return rd;
}

void crenderer_resize(crenderer_t *rd) {
    vkDeviceWaitIdle(rd->device->device);

    const VkSemaphoreCreateInfo semaphoreCreateInfo = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0
    };

    const VkFenceCreateInfo fenceCreateInfo = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT
    };
    
    // adhoc method of resetting them
    for (int i = 0; i < (1 + (int)rd->config.buffer_mode); i++) {
        cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(rd->renderData, i);
        vkDestroySemaphore(rd->device->device, data->imageAvailableSemaphore, NULL);
        vkDestroySemaphore(rd->device->device, data->renderingFinishedSemaphore, NULL);
        vkDestroyFence(rd->device->device, data->inFlightFence, NULL);
    }
    for (u32 i = 0; i < SwapChainImageCount; i++)
    {
        cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(rd->renderData, i);
        vkDestroyImageView(rd->device->device, data->swapchainImageView, NULL);
        vkDestroyFramebuffer(rd->device->device, data->color_framebuffer, NULL);
    }
    cg_vector_clear(rd->renderData);

    i32 w, h;
    SDL_Vulkan_GetDrawableSize(rd->device->window, &w, &h);

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(rd->device->physDevice, rd->device->surface, &surface_capabilities);

    const u32 min_width = surface_capabilities.minImageExtent.width;
    const u32 min_height = surface_capabilities.minImageExtent.height;

    const u32 max_width = surface_capabilities.maxImageExtent.width;
    const u32 max_height = surface_capabilities.maxImageExtent.height;

    w = cmclamp((u32)w, min_width, max_width);
    h = cmclamp((u32)h, min_height, max_height);

    rd->render_extent = (cg_extent2d){ w, h };

    VkSwapchainKHR old_swapchain = rd->swapchain;

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

    if (rd->config.vsync_enabled) {
        switch (rd->config.buffer_mode) {
            case CGFX_BUFFER_MODE_SINGLE_BUFFERED:
            case CGFX_BUFFER_MODE_DOUBLE_BUFFERED:
            default:
                present_mode = VK_PRESENT_MODE_FIFO_KHR;
                break;
            case CGFX_BUFFER_MODE_TRIPLE_BUFFERED:
                present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            break;
        };
    } else {
        present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    cvk_swapchain_create_info scio = {};
    scio.extent.width = rd->render_extent.width;
    scio.extent.height = rd->render_extent.height;
    scio.present_mode = present_mode;
    scio.format = SwapChainImageFormat;
    scio.color_space = SwapChainColorSpace;
    scio.image_count = SwapChainImageCount;
    scio.old_swapchain = old_swapchain;
    cvk_create_swapchain(rd->device, &scio, &rd->swapchain);
    vkDestroySwapchainKHR(rd->device->device, old_swapchain, NULL);

    vkDestroyImage(rd->device->device, rd->color_image, NULL);
    vkDestroyImageView(rd->device->device, rd->color_image_view, NULL);

    create_optional_images(rd);
    create_framebuffers_and_swapchain_image_views(rd);

    for (u32 i = 0; i < SwapChainImageCount; i++)
    {
        cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(rd->renderData, i);
        vkCreateSemaphore(rd->device->device, &semaphoreCreateInfo, NULL, &data->renderingFinishedSemaphore);
        vkCreateSemaphore(rd->device->device, &semaphoreCreateInfo, NULL, &data->imageAvailableSemaphore);
        vkCreateFence(rd->device->device, &fenceCreateInfo, NULL, &data->inFlightFence);
    }

    cg__reset_frame_buffer_resized();
}

bool_t crd_begin_render(crenderer_t *rd)
{
    cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(rd->renderData, rd->renderer_frame);

    vkWaitForFences(rd->device->device, 1, &data->inFlightFence, VK_TRUE, UINT64_MAX);

    const VkResult imageAcquireResult = vkAcquireNextImageKHR(rd->device->device, rd->swapchain, UINT64_MAX, data->imageAvailableSemaphore, VK_NULL_HANDLE, &rd->imageIndex);

    const VkCommandBuffer drawBuffer = *(VkCommandBuffer *)cg_vector_get(rd->drawBuffers, rd->renderer_frame);

	if(imageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR || imageAcquireResult == VK_SUBOPTIMAL_KHR || cg_get_frame_buffer_resized())
	{
        crenderer_resize(rd);
		return false;
	}
	else if (imageAcquireResult != VK_SUCCESS && imageAcquireResult != VK_SUBOPTIMAL_KHR) 
	{
		LOG_ERROR("Failed to acquire image from swapchain");
		return false;
    }

    vkResetFences(rd->device->device, 1, &data->inFlightFence);

    VkClearValue clearValues[2];
    clearValues[0].color = (VkClearColorValue){ {0.0f, 0.0f, 0.0f, 1.0f} };
    clearValues[1].depthStencil = (VkClearDepthStencilValue){ 1.0f, 0 };

    VkFramebuffer fb = (*(cg_framerender_data *)(cg_vector_get(rd->renderData, rd->imageIndex))).color_framebuffer;
 
    static VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.framebuffer = fb;
    renderPassInfo.renderArea.extent.width = rd->render_extent.width;
    renderPassInfo.renderArea.extent.height = rd->render_extent.height;
    renderPassInfo.renderArea.offset = (VkOffset2D){0, 0};
    renderPassInfo.renderPass = rd->render_pass;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    const VkCommandBufferBeginInfo beginInfo = {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL
    };

    vkBeginCommandBuffer(drawBuffer, &beginInfo);
    vkCmdBeginRenderPass(drawBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = rd->render_extent.width;
    viewport.height = rd->render_extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(drawBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = (VkOffset2D){ 0, 0 };
    scissor.extent = (VkExtent2D){ (unsigned)rd->render_extent.width, (unsigned)rd->render_extent.height };
    vkCmdSetScissor(drawBuffer, 0, 1, &scissor);

    return true;
}

void crd_end_render(crenderer_t *rd)
{
    const VkCommandBuffer drawBuffer = *(VkCommandBuffer *)cg_vector_get(rd->drawBuffers, rd->renderer_frame);

    vkCmdEndRenderPass(drawBuffer);
    vkEndCommandBuffer(drawBuffer);

	VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(rd->renderData, rd->renderer_frame);
    const VkSemaphore waitSemaphores[] = {data->imageAvailableSemaphore};
    const VkSemaphore signalSemaphores[] = {data->renderingFinishedSemaphore};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;

    const VkCommandBuffer buffers[] = { drawBuffer };
    submitInfo.commandBufferCount = array_len(buffers);
    submitInfo.pCommandBuffers = buffers;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(PresentQueue, 1, &submitInfo, data->inFlightFence);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; // This is signalSemaphores so that this starts as soon as the signaled semaphores are signaled.
    presentInfo.pImageIndices = &rd->imageIndex;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &rd->swapchain;

    const VkResult result = vkQueuePresentKHR(PresentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || cg_get_frame_buffer_resized()) {
        crenderer_resize(rd);
    }

    rd->renderer_frame = (rd->renderer_frame + 1) % (1 + (int)rd->config.buffer_mode);
}

cg_device_t *crd_get_device(const crenderer_t *rd)
{
    return rd->device;
}