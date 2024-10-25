#include "../include/cgfx.h"
#include "../include/cgfxdef.h"

#include "../include/vkstdafx.h"
#include "../include/stdafx.h"
#include "../include/math/math.h"
#include "../include/cvk.h"
#include "../include/cengine.h"
#include "../include/vkhelper.h"

#include "../include/cgvector.h"

#include "../include/cdevicememory.h"

#include <SDL2/SDL_vulkan.h>

VkInstance instance = NULL;
VkDevice device = NULL;
VkPhysicalDevice physDevice = NULL;
VkSurfaceKHR surface = NULL;
SDL_Window *window = NULL;
VkDebugUtilsMessengerEXT debugMessenger = NULL;

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
    return *(VkCommandBuffer *)cg_vector_get(&rd->drawBuffers, rd->renderer_frame);
}

VkRenderPass crd_get_render_pass(const crenderer_t *rd)
{
    return rd->render_pass;
}

cg_extent2d crd_get_render_extent(const crenderer_t *rd)
{
    return rd->render_extent;
}

void crenderer_destroy(crenderer_t *rd)
{
    vkDeviceWaitIdle(device);

    for (int i = 0; i < SwapChainImageCount; i++) {
        cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(&rd->renderData, i);
        // weeeee
        cgfx_gpu_image_free(&data->depth_image);
        vkDestroyFramebuffer(device, data->color_framebuffer, NULL);
        // CHECKMEOUTPARTNER: Uhh, these are causing vulkan validation layer errors...
        // vkDestroySemaphore(device, data->image_available_semaphore, NULL);
        // vkDestroySemaphore(device, data->render_finish_semaphore, NULL);
        // vkDestroyFence(device, data->in_flight_fence, NULL);
    }
    cg_vector_destroy(&rd->renderData);
    cgfx_gpu_memory_free(&rd->depth_image_memory);
    vkFreeCommandBuffers(device, rd->commandPool, cg_vector_size(&rd->drawBuffers), (VkCommandBuffer *)cg_vector_data(&rd->drawBuffers));
    vkDestroyCommandPool(device, rd->commandPool, NULL);

    vkDestroySwapchainKHR(device, rd->swapchain, NULL);

    if (rd->ctext) {
        // ! FIXME: Implement
    }
    if (rd->config.multisampling_enable) {
        cgfx_gpu_image_free(&rd->color_image);
        cgfx_gpu_memory_free(&rd->color_image_memory);
    }
    vkDestroyRenderPass(device, rd->render_pass, NULL);
    free(rd);
}

void create_optional_images(crenderer_t *rd)
{
    vkGetSwapchainImagesKHR(device, rd->swapchain, &SwapChainImageCount, NULL);
    VkImage *swapchainImages = (VkImage *)malloc(SwapChainImageCount * sizeof(VkImage));
	vkGetSwapchainImagesKHR(device, rd->swapchain, &SwapChainImageCount, swapchainImages);

    cg_vector_resize(&rd->renderData, SwapChainImageCount);

    if (rd->config.multisampling_enable) {
        int color_image_size = 0;
        vkh_image_create_empty(
            rd->render_extent.width, rd->render_extent.height,
            SwapChainImageFormat,
            Samples,
            4,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            &color_image_size,
            &rd->color_image.image, NULL
        );
        cgfx_gpu_memory_allocate(color_image_size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, &rd->color_image_memory);
        cgfx_gpu_memory_bind_image(&rd->color_image_memory, 0, &rd->color_image);

        cgfx_gpu_image_view_create_info resolve_view_info = {
            .format = SwapChainImageFormat,
            .view_type = VK_IMAGE_VIEW_TYPE_2D,
            .subresourceRange = (VkImageSubresourceRange) {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };
        cgfx_gpu_create_image_view(&resolve_view_info, &rd->color_image);
        printf("%i\n", SwapChainImageFormat);
        cassert(0 !=  SwapChainImageFormat);
    }

    // attachment vector will be like <color resolve, depth attachment, swapchain image>
    for (int i = 0; i < (int)SwapChainImageCount; i++) {
        cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(&rd->renderData, i);
        data->sc_image = (cgfx_gpu_image_t){};
        data->sc_image.image = swapchainImages[i];

        const cgfx_gpu_image_create_info image_info = {
            .format = VK_FORMAT_D32_SFLOAT,
            .samples = Samples,
            .type = VK_IMAGE_TYPE_2D,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .extent = (VkExtent3D){ .width = rd->render_extent.width, .height = rd->render_extent.height, .depth = 1 },
            .arraylayers = 1,
            .miplevels = 1,
        };
        cgfx_gpu_create_image(&image_info, &data->depth_image);

        if (i == 0) {
            VkMemoryRequirements memReqs = {};
            vkGetImageMemoryRequirements(device, data->depth_image.image, &memReqs);

            rd->shadow_image_size = memReqs.size;
            cgfx_gpu_memory_allocate(rd->shadow_image_size * SwapChainImageCount, CGFX_MEMORY_USAGE_GPU_LOCAL, &rd->depth_image_memory);
        }

        cgfx_gpu_memory_bind_image(&rd->depth_image_memory, i * rd->shadow_image_size, &data->depth_image);

        const cgfx_gpu_image_view_create_info view_info = {
            .format = VK_FORMAT_D32_SFLOAT,
            .view_type = VK_IMAGE_VIEW_TYPE_2D,
            .subresourceRange = (VkImageSubresourceRange){
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        cgfx_gpu_create_image_view(&view_info, &data->depth_image);
    }

    free(swapchainImages);
}

void create_framebuffers_and_swapchain_image_views(crenderer_t *rd) {
    cg_vector_t /* VkImageView */ attachments = cg_vector_init(sizeof(VkImageView), 3);

    for (int i = 0; i < (int)SwapChainImageCount; i++) {
        cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(&rd->renderData, i);

        // we don't know anything about the swapchain_image, as it's a swapchain image
        // so we have to manually create the image view;
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = data->sc_image.image;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = SwapChainImageFormat;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &imageViewCreateInfo, NULL, &data->sc_image.view) != VK_SUCCESS) {
            LOG_ERROR("Failed to create view for swapchain image %d", i);
        }

        const VkImageView swapchain_image_view = data->sc_image.view;
        const VkImageView depth_image_view = data->depth_image.view;

        cg_vector_clear(&attachments);
        if (rd->config.multisampling_enable) {
            cg_vector_push_set(&attachments, (VkImageView[]){ rd->color_image.view, depth_image_view, swapchain_image_view }, 3);
        }
        else {
            cg_vector_push_set(&attachments, (VkImageView[]){ swapchain_image_view, depth_image_view }, 2);
        }

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = rd->render_pass;
        framebufferInfo.attachmentCount = cg_vector_size(&attachments);
        framebufferInfo.pAttachments = (VkImageView *)cg_vector_data(&attachments);
        framebufferInfo.width = rd->render_extent.width;
        framebufferInfo.height = rd->render_extent.height;
        framebufferInfo.layers = 1;
        if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &data->color_framebuffer) != VK_SUCCESS) {
            LOG_ERROR("Failed to create framebuffer for swapchain image %d", i);
        }
    }

    cg_vector_destroy(&attachments);
}

crenderer_t *crenderer_init( const crenderer_config *conf)
{
    struct crenderer_t *rd = (crenderer_t *)calloc(1, sizeof(struct crenderer_t));
    device = device;

    memcpy(&rd->config, conf, sizeof(crenderer_config));

    /* Initialize graphics singleton */
    {
        if (!vkh_get_supported_fmt(physDevice, surface, &SwapChainImageFormat, &SwapChainColorSpace)) {
            LOG_AND_ABORT("No supported format for display.");
        }
        SwapChainImageCount = vkh_get_image_count(physDevice, surface);

        u32 queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, NULL);
        cg_vector_t /* VkQueueFamilyProperties */ queueFamilies = cg_vector_init(sizeof(VkQueueFamilyProperties), queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, (VkQueueFamilyProperties *)cg_vector_data(&queueFamilies));

        u32 graphicsFamily = 0, graphicsAndComputeFamily = 0, presentFamily = 0, computeFamily = 0, transferFamily = 0;
        bool foundGraphicsFamily = false, foundGraphicsAndComputeFamily = false, foundPresentFamily = false, foundComputeFamily = false, foundTransferFamily = false;

        u32 i = 0;
        for (u32 j = 0; j < queueCount; j++) {
            const VkQueueFamilyProperties queueFamily = ((VkQueueFamilyProperties *)cg_vector_data(&queueFamilies))[j];
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

        cg_vector_destroy(&queueFamilies);
    }

    rd->render_extent.width = conf->initial_window_size.width;
    rd->render_extent.height = conf->initial_window_size.height;

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

    if (conf->vsync_enabled) {
        switch (conf->buffer_mode) {
            case CG_BUFFER_MODE_SINGLE_BUFFERED:
            case CG_BUFFER_MODE_DOUBLE_BUFFERED:
            default:
                present_mode = VK_PRESENT_MODE_FIFO_KHR;
                break;
            case CG_BUFFER_MODE_TRIPLE_BUFFERED:
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
    cvk_create_swapchain(&scio, &rd->swapchain);

    VkSampleCountFlagBits conf_samples;
    if (conf->samples == CG_SAMPLE_COUNT_MAX_SUPPORTED) {
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
	if(vkCreateCommandPool(device, &cmdPoolCreateInfo, NULL, &rd->commandPool) != VK_SUCCESS) {
		LOG_ERROR("Failed to create command pool");
	}

    // how many frames the renderer will render at once
    const int frames_in_flight = 1 + (int)conf->buffer_mode;

    rd->drawBuffers = cg_vector_init(sizeof(VkCommandBuffer), frames_in_flight);
    rd->renderData = cg_vector_init(sizeof(cg_framerender_data), frames_in_flight);

    cg_framerender_data data = {};
    for (int i = 0; i < frames_in_flight; i++) {
        cg_vector_push_back(&rd->drawBuffers, &data);
        cg_vector_push_back(&rd->renderData, &data);
    }

    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = frames_in_flight;
    cmdAllocInfo.commandPool = rd->commandPool;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, (VkCommandBuffer *)cg_vector_data(&rd->drawBuffers));

    rd->depth_buffer_format = VK_FORMAT_D32_SFLOAT; // replace (probably)

    cvk_render_pass_create_info rpi = {};
    rpi.format = SwapChainImageFormat;
    rpi.depthBufferFormat = rd->depth_buffer_format;
    rpi.subpass = 0;
    rpi.samples = samples;
    cvk_create_render_pass(&rpi, &rd->render_pass, cvk_flag_register);

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

        cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(&rd->renderData, i);
        vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &data->render_finish_semaphore);
        vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &data->image_available_semaphore);
        vkCreateFence(device, &fenceCreateInfo, NULL, &data->in_flight_fence);
	}

    return rd;
}

void crenderer_resize(crenderer_t *rd) {
    vkDeviceWaitIdle(device);

    const VkSemaphoreCreateInfo semaphoreCreateInfo = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0
    };

    const VkFenceCreateInfo fenceCreateInfo = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT
    };
    
    // adhoc method of resetting them
    for (int i = 0; i < (1 + (int)rd->config.buffer_mode); i++) {
        cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(&rd->renderData, i);
        vkDestroySemaphore(device, data->image_available_semaphore, NULL);
        vkDestroySemaphore(device, data->render_finish_semaphore, NULL);
        vkDestroyFence(device, data->in_flight_fence, NULL);
    }
    for (u32 i = 0; i < SwapChainImageCount; i++)
    {
        cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(&rd->renderData, i);
        vkDestroyImageView(device, data->sc_image.view, NULL); // as the view was silently smushed into the structure, we just kinda smush it out as well.
        vkDestroyFramebuffer(device, data->color_framebuffer, NULL);
    }
    cg_vector_clear(&rd->renderData);

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

    rd->render_extent = (cg_extent2d){ w, h };

    VkSwapchainKHR old_swapchain = rd->swapchain;

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

    if (rd->config.vsync_enabled) {
        switch (rd->config.buffer_mode) {
            case CG_BUFFER_MODE_SINGLE_BUFFERED:
            case CG_BUFFER_MODE_DOUBLE_BUFFERED:
            default:
                present_mode = VK_PRESENT_MODE_FIFO_KHR;
                break;
            case CG_BUFFER_MODE_TRIPLE_BUFFERED:
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
    cvk_create_swapchain(&scio, &rd->swapchain);
    vkDestroySwapchainKHR(device, old_swapchain, NULL);

    cgfx_gpu_image_free(&rd->color_image);

    create_optional_images(rd);
    create_framebuffers_and_swapchain_image_views(rd);

    for (u32 i = 0; i < SwapChainImageCount; i++)
    {
        cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(&rd->renderData, i);
        vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &data->render_finish_semaphore);
        vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &data->image_available_semaphore);
        vkCreateFence(device, &fenceCreateInfo, NULL, &data->in_flight_fence);
    }

    cg__reset_frame_buffer_resized();
}

bool_t crd_begin_render(crenderer_t *rd)
{
    cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(&rd->renderData, rd->renderer_frame);

    vkWaitForFences(device, 1, &data->in_flight_fence, VK_TRUE, UINT64_MAX);

    const VkResult imageAcquireResult = vkAcquireNextImageKHR(device, rd->swapchain, UINT64_MAX, data->image_available_semaphore, VK_NULL_HANDLE, &rd->imageIndex);

    const VkCommandBuffer drawBuffer = *(VkCommandBuffer *)cg_vector_get(&rd->drawBuffers, rd->renderer_frame);

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

    vkResetFences(device, 1, &data->in_flight_fence);

    // * Maybe the user should have control of the clear color but I don't really care lmao

    VkFramebuffer fb = (*(cg_framerender_data *)(cg_vector_get(&rd->renderData, rd->imageIndex))).color_framebuffer;
    
    // Why was this static?
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = rd->render_pass,
        .framebuffer = fb,
        .renderArea = (VkRect2D) {
            .extent = (VkExtent2D) {
                rd->render_extent.width,
                rd->render_extent.height
            },
            .offset = {}
        },
        .clearValueCount = 2,
        .pClearValues = (VkClearValue[2]) {
            { .color = (VkClearColorValue){ {0.0f, 0.0f, 0.0f, 1.0f} } },
            { .depthStencil = (VkClearDepthStencilValue){ 1.0f, 0 } }
        },
    };

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
    const VkCommandBuffer drawBuffer = *(VkCommandBuffer *)cg_vector_get(&rd->drawBuffers, rd->renderer_frame);

    vkCmdEndRenderPass(drawBuffer);
    vkEndCommandBuffer(drawBuffer);

	VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    cg_framerender_data *data = (cg_framerender_data *)cg_vector_get(&rd->renderData, rd->renderer_frame);
    const VkSemaphore waitSemaphores[] = {data->image_available_semaphore};
    const VkSemaphore signalSemaphores[] = {data->render_finish_semaphore};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;

    const VkCommandBuffer buffers[] = { drawBuffer };
    submitInfo.commandBufferCount = array_len(buffers);
    submitInfo.pCommandBuffers = buffers;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(PresentQueue, 1, &submitInfo, data->in_flight_fence);

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