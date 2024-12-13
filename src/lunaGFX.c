#include "../include/lunaGFX.h"
#include "../include/lunaGFXDef.h"

#include "../include/vkstdafx.h"
#include "../include/stdafx.h"
#include "../include/math/math.h"
#include "../include/math/vec2.h"
#include "../include/math/vec3.h"
#include "../include/lunaPipeline.h"
#include "../include/cengine.h"
#include "../include/lunaVK.h"
#include "../include/lunaUI.h"
#include "../include/lunaObject.h"
#include "../include/lunaInternalObjects.h"
#include "../include/cshadermanager.h"
#include "../include/ctext.h"

#include "../include/cgvector.h"

#include "../include/lunaGPUObjects.h"

#include "../include/sprite.h"

#include "../include/camera.h"
#include "../include/cinput.h"

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

luna_DescriptorPool g_pool;
ccamera camera;

typedef struct quad_vertex {
    vec3 position;
    vec2 tex_coords;
} quad_vertex;

typedef struct line_vertex {
    vec2 position;
} line_vertex;

static const quad_vertex quad_vertices_unflipped[4] = {
    (quad_vertex){ (vec3){+0.5f, +0.5f, 0.0f}, { 0.0f, 0.0f } },
    (quad_vertex){ (vec3){-0.5f, +0.5f, 0.0f}, { 1.0f, 0.0f } },
    (quad_vertex){ (vec3){-0.5f, -0.5f, 0.0f}, { 1.0f, 1.0f } },
    (quad_vertex){ (vec3){+0.5f, -0.5f, 0.0f}, { 0.0f, 1.0f } }
};

static const uint32_t quad_indices[] = {
    0,1,2,
    0,2,3
};

int luna_Renderer_GetFrame(const luna_Renderer_t *rd)
{
    return rd->renderer_frame;
}


VkCommandBuffer luna_Renderer_GetDrawBuffer(const luna_Renderer_t *rd)
{
    return *(VkCommandBuffer *)cg_vector_get(&rd->drawBuffers, rd->renderer_frame);
}

VkRenderPass luna_Renderer_GetRenderPass(const luna_Renderer_t *rd)
{
    return rd->render_pass;
}

lunaExtent2D luna_Renderer_GetRenderExtent(const luna_Renderer_t *rd)
{
    return rd->render_extent;
}

int luna_Renderer_GetMaxFramesInFlight(const luna_Renderer_t *rd)
{
    return 1 + (int)rd->buffer_mode;
}

void luna_Renderer_DrawQuad(luna_Renderer_t *rd, sprite_t *spr, vec2 tex_coord_multiplier, vec3 position, vec3 size, vec4 color, int layer)
{
    luna_DrawCall_t drawcall = {
        .type = LUNA_DRAWCALL_QUAD,
        .layer = layer,
        .drawcall = {
            .quad = {
                .spr = spr,
                .tex_multiplier = tex_coord_multiplier,
                .pos = position,
                .siz = size,
                .col = color
            }
        }
    };
    cg_vector_push_back(&rd->drawcalls, &drawcall);
}

void luna_Renderer_DrawLine(luna_Renderer_t *rd, vec2 start, vec2 end, vec4 color, int layer)
{
    luna_DrawCall_t drawcall = {
        .type = LUNA_DRAWCALL_LINE,
        .layer = layer,
        .drawcall = {
            .line = {
                .begin = start,
                .end = end,
                .col = color
            }
        }
    };
    cg_vector_push_back(&rd->drawcalls, &drawcall);
}

// thjs will just sort the array from small layer to big layer :>
int __drawcall_compar(const void *obj1, const void *obj2) {
    return (((luna_DrawCall_t *)obj1)->layer - ((luna_DrawCall_t *)obj2)->layer);
}

void luna_Renderer_RenderAll(luna_Renderer_t *rd) {
    const uint32_t camera_ub_offset = luna_Renderer_GetFrame(rd) * sizeof(camera_uniform_buffer);
    const VkCommandBuffer cmd = luna_Renderer_GetDrawBuffer(rd);
    const VkDescriptorSet camera_set = camera.sets->set;

    cg_vector_sort(&rd->drawcalls, __drawcall_compar);

    for (int i = 0; i < cg_vector_size(&rd->drawcalls); i++) {
        const luna_DrawCall_t *drawcall = &((luna_DrawCall_t *)cg_vector_data(&rd->drawcalls))[i];

        if (drawcall->type == LUNA_DRAWCALL_QUAD) {

            struct push_constants {
                mat4 model;
                vec4 color;
                vec2 tex_multiplier; // Multiplied with the tex coords
            } pc;

            VkDeviceSize offsets = 0;

            VkDescriptorSet sprite_set = sprite_get_descriptor(drawcall->drawcall.quad.spr);
            const VkDescriptorSet sets[] = { camera_set, sprite_set };

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g_Pipelines.Unlit.pipeline_layout, 0, 2, sets, 1, &camera_ub_offset);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g_Pipelines.Unlit.pipeline);
            vkCmdBindVertexBuffers(cmd, 0, 1, &rd->quad_vb.buffers[0], &offsets);
            vkCmdBindIndexBuffer(cmd, rd->quad_vb.buffers[0], sizeof(quad_vertices_unflipped), VK_INDEX_TYPE_UINT32);

            mat4 scale = m4scale(m4init(1.0f), drawcall->drawcall.quad.siz);
            mat4 rotate = m4init(1.0f);
            mat4 translate = m4translate(m4init(1.0f), drawcall->drawcall.quad.pos);
            pc.model = m4mul(translate, m4mul(rotate, scale));
            pc.color = drawcall->drawcall.quad.col;
            pc.tex_multiplier = drawcall->drawcall.quad.tex_multiplier;
            vkCmdPushConstants(
                cmd,
                g_Pipelines.Unlit.pipeline_layout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(struct push_constants),
                &pc
            );

            VkDeviceSize vertex_offset = 0;
            vkCmdDrawIndexed(cmd, 6, 1, 0, vertex_offset, 0);
        } else if (drawcall->type == LUNA_DRAWCALL_LINE) {
            struct line_push_constants {
                mat4 model;
                vec4 color;
                vec2 line_begin;
                vec2 line_end;
            } pc;

            const VkDescriptorSet sets[] = { camera_set };

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g_Pipelines.Line.pipeline_layout, 0, 1, sets, 1, &camera_ub_offset);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g_Pipelines.Line.pipeline);

            pc.model = m4init(1.0f);
            pc.color = drawcall->drawcall.line.col;
            pc.line_begin = (vec2){drawcall->drawcall.line.begin.x, drawcall->drawcall.line.begin.y};
            pc.line_end = (vec2){drawcall->drawcall.line.end.x, drawcall->drawcall.line.end.y};
            vkCmdPushConstants(
                cmd,
                g_Pipelines.Line.pipeline_layout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(struct line_push_constants),
                &pc
            );

            vkCmdDraw(cmd, 2, 1, 0, 0);
        }
    }

    cg_vector_clear(&rd->drawcalls);
}

void luna_Renderer_PrepareQuadRenderer(luna_Renderer_t *rd)
{
    luna_GPU_CreateBuffer(
        sizeof(quad_vertices_unflipped) + sizeof(quad_indices),
        LUNA_GPU_ALIGNMENT_UNNECESSARY,
        1,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        &rd->quad_vb
    );
    luna_GPU_AllocateMemory(
        sizeof(quad_vertices_unflipped) + sizeof(quad_indices),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        &rd->quad_memory
    );
    luna_GPU_BindBufferToMemory(&rd->quad_memory, 0, &rd->quad_vb);
    vkMapMemory(device, rd->quad_memory.memory, 0, VK_WHOLE_SIZE, 0, &rd->mapped);

    memcpy(rd->mapped, quad_vertices_unflipped, sizeof(quad_vertices_unflipped));
    memcpy((char *)rd->mapped + sizeof(quad_vertices_unflipped), quad_indices, sizeof(quad_indices));
}

void luna_Renderer_Destroy(luna_Renderer_t *rd)
{
    vkDeviceWaitIdle(device);

    // we were only making frames_in_flight fences and it was working for some reason!!
    // that was the reason we were getting errors!
    for (int i = 0; i < cg_vector_size(&rd->renderData); i++) {
        lunaFrameRenderData *data = (lunaFrameRenderData *)cg_vector_get(&rd->renderData, i);
        vkDestroyImageView(device, luna_GPU_TextureGetView(data->sc_image), NULL); // as the view was silently smushed into the structure, we just kinda smush it out as well.
        vkDestroyFramebuffer(device, data->color_framebuffer, NULL);

        vkDestroySemaphore(device, data->image_available_semaphore, NULL);
        vkDestroySemaphore(device, data->render_finish_semaphore, NULL);
        vkDestroyFence(device, data->in_flight_fence, NULL);
    }

    cg_vector_destroy(&rd->renderData);
    luna_GPU_FreeMemory(&rd->depth_image_memory);
    vkFreeCommandBuffers(device, rd->commandPool, cg_vector_size(&rd->drawBuffers), (VkCommandBuffer *)cg_vector_data(&rd->drawBuffers));
    vkDestroyCommandPool(device, rd->commandPool, NULL);

    vkDestroySwapchainKHR(device, rd->swapchain, NULL);

    if (rd->ctext) {
        vkDestroyDescriptorSetLayout(device, rd->ctext->desc_set->layout, NULL);
    }
    if (rd->flags & CG_RENDERER_MULTISAMPLING_ENABLE) {
        luna_GPU_DestroyTexture(rd->color_image);
        luna_GPU_FreeMemory(&rd->color_image_memory);
    }
    vkDestroyRenderPass(device, rd->render_pass, NULL);
    free(rd);
}

void create_optional_images(luna_Renderer_t *rd)
{
    vkGetSwapchainImagesKHR(device, rd->swapchain, &SwapChainImageCount, NULL);
    VkImage *swapchainImages = (VkImage *)malloc(SwapChainImageCount * sizeof(VkImage));
	vkGetSwapchainImagesKHR(device, rd->swapchain, &SwapChainImageCount, swapchainImages);

    cg_vector_resize(&rd->renderData, SwapChainImageCount);

    if (rd->flags & CG_RENDERER_MULTISAMPLING_ENABLE) {
        int color_image_size = 0;
        luna_VK_CreateTextureEmpty(
            rd->render_extent.width, rd->render_extent.height,
            SwapChainImageFormat,
            Samples,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            &color_image_size,
            &rd->color_image->image, NULL
        );
        luna_GPU_AllocateMemory(color_image_size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, &rd->color_image_memory);
        luna_GPU_BindTextureToMemory(&rd->color_image_memory, 0, rd->color_image);

        luna_GPU_TextureViewCreateInfo resolve_view_info = {
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
        luna_GPU_CreateTextureView(&resolve_view_info, &rd->color_image);
    }

    // attachment vector will be like <color resolve, depth attachment, swapchain image>
    for (int i = 0; i < (int)SwapChainImageCount; i++) {
        lunaFrameRenderData *data = (lunaFrameRenderData *)cg_vector_get(&rd->renderData, i);
        (data->sc_image) = calloc(1, sizeof(luna_GPU_Texture));
        data->sc_image->image = swapchainImages[i];

        const luna_GPU_TextureCreateInfo image_info = {
            .format = VK_FORMAT_D32_SFLOAT,
            .samples = Samples,
            .type = VK_IMAGE_TYPE_2D,
            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .extent = (VkExtent3D){ .width = rd->render_extent.width, .height = rd->render_extent.height, .depth = 1 },
            .arraylayers = 1,
            .miplevels = 1,
        };
        luna_GPU_CreateTexture(&image_info, &data->depth_image);

        if (i == 0) {
            VkMemoryRequirements memReqs = {};
            vkGetImageMemoryRequirements(device, luna_GPU_TextureGet(data->depth_image), &memReqs);

            rd->shadow_image_size = memReqs.size;
            luna_GPU_AllocateMemory(rd->shadow_image_size * SwapChainImageCount, LUNA_GPU_MEMORY_USAGE_GPU_LOCAL, &rd->depth_image_memory);
        }

        luna_GPU_BindTextureToMemory(&rd->depth_image_memory, i * rd->shadow_image_size, data->depth_image);

        const luna_GPU_TextureViewCreateInfo view_info = {
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
        luna_GPU_CreateTextureView(&view_info, &data->depth_image);
    }

    free(swapchainImages);
}

void create_framebuffers_and_swapchain_image_views(luna_Renderer_t *rd) {
    cg_vector_t /* VkImageView */ attachments = cg_vector_init(sizeof(VkImageView), 3);

    for (int i = 0; i < (int)SwapChainImageCount; i++) {
        lunaFrameRenderData *data = (lunaFrameRenderData *)cg_vector_get(&rd->renderData, i);

        // we don't know anything about the swapchain_image, as it's a swapchain image
        // so we have to manually create the image view;
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = luna_GPU_TextureGet(data->sc_image);
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = SwapChainImageFormat;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        VkImageView vioew;
        if (vkCreateImageView(device, &imageViewCreateInfo, NULL, &vioew) != VK_SUCCESS) {
            LOG_ERROR("Failed to create view for swapchain image %d", i);
        }

        luna_GPU_TextureAttachView(data->sc_image, vioew);

        const VkImageView swapchain_image_view = luna_GPU_TextureGetView(data->sc_image);
        const VkImageView depth_image_view = luna_GPU_TextureGetView(data->depth_image);

        cg_vector_clear(&attachments);
        if (rd->flags & CG_RENDERER_MULTISAMPLING_ENABLE) {
            cg_vector_push_set(&attachments, (VkImageView[]){ luna_GPU_TextureGetView(rd->color_image), depth_image_view, swapchain_image_view }, 3);
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

void luna_Renderer_InitializeGraphicsSingleton() {
    if (!luna_VK_GetSupportedFormat(physDevice, surface, &SwapChainImageFormat, &SwapChainColorSpace)) {
            LOG_AND_ABORT("No supported format for display.");
    }
    SwapChainImageCount = luna_VK_GetSurfaceImageCount(physDevice, surface);

    u32 queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, NULL);
    cg_vector_t /* VkQueueFamilyProperties */ queueFamilies = cg_vector_init(sizeof(VkQueueFamilyProperties), queueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, (VkQueueFamilyProperties *)cg_vector_data(&queueFamilies));

    u32 graphicsFamily = 0, graphicsAndComputeFamily = 0, presentFamily = 0, computeFamily = 0, transferFamily = 0;
    bool foundGraphicsFamily = false, foundGraphicsAndComputeFamily = false, foundPresentFamily = false, foundComputeFamily = false, foundTransferFamily = false;

    u32 i = 0;
    for (u32 j = 0; j < queueCount; j++) {
        const VkQueueFamilyProperties queueFamily = ((VkQueueFamilyProperties *)cg_vector_data(&queueFamilies))[j];
        VkBool32 presentSupport;
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

void luna_Renderer_InitializeRenderingComponents(luna_Renderer_t *rd, const luna_Renderer_Config *conf) {
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
        present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    }

    luna_GPU_SwapchainCreateInfo scio = {};
    scio.extent.width = rd->render_extent.width;
    scio.extent.height = rd->render_extent.height;
    scio.present_mode = present_mode;
    scio.format = SwapChainImageFormat;
    scio.color_space = SwapChainColorSpace;
    scio.image_count = SwapChainImageCount;
    luna_GPU_CreateSwapchain(&scio, &rd->swapchain);

    VkSampleCountFlagBits conf_samples;
    if (conf->samples == CG_SAMPLE_COUNT_MAX_SUPPORTED) {
        conf_samples = MAX_SAMPLES;
    } else {
        conf_samples = (VkSampleCountFlagBits)conf->samples;
    }
    const VkSampleCountFlagBits samples = conf->multisampling_enable ? conf_samples : VK_SAMPLE_COUNT_1_BIT;
    Samples = samples;

    if (conf->multisampling_enable) {
        luna_GPU_vk_flag_register |= CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING;
        rd->attachment_count++;
    }
    luna_GPU_vk_flag_register |= CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK;
    luna_GPU_vk_flag_register |= CVK_PIPELINE_FLAGS_FORCE_CULLING;
    rd->attachment_count++;

    VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
	cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCreateInfo.queueFamilyIndex = GraphicsFamilyIndex;
    cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;    
	if(vkCreateCommandPool(device, &cmdPoolCreateInfo, NULL, &rd->commandPool) != VK_SUCCESS) {
		LOG_ERROR("Failed to create command pool");
	}

    const int frames_in_flight = 1 + (int)conf->buffer_mode;

    lunaFrameRenderData data = {};
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

    luna_GPU_RenderPassCreateInfo rpi = {};
    rpi.format = SwapChainImageFormat;
    rpi.depthBufferFormat = rd->depth_buffer_format;
    rpi.subpass = 0;
    rpi.samples = samples;
    luna_GPU_CreateRenderPass(&rpi, &rd->render_pass, luna_GPU_vk_flag_register);

    create_optional_images(rd);
    create_framebuffers_and_swapchain_image_views(rd);
}

luna_Renderer_t *luna_Renderer_Init(const luna_Renderer_Config *conf)
{
    if (conf->multisampling_enable == 1) {
        LOG_ERROR("config samples must not be 1 if multisampling is enabled.");
        cassert(conf->samples != CG_SAMPLE_COUNT_1_SAMPLES);
    }
    struct luna_Renderer_t *rd = (luna_Renderer_t *)calloc(1, sizeof(struct luna_Renderer_t));

    // how many frames the renderer will render at once
    const int frames_in_flight = 1 + (int)conf->buffer_mode;

    g_Samplers = cg_vector_init(sizeof(luna_GPU_Sampler), 4);
    rd->drawcalls = cg_vector_init(sizeof(luna_DrawCall_t), 4);
    rd->drawBuffers = cg_vector_init(sizeof(VkCommandBuffer), frames_in_flight);
    rd->renderData = cg_vector_init(sizeof(lunaFrameRenderData), frames_in_flight);

    if (conf->multisampling_enable) {
        rd->flags |= CG_RENDERER_MULTISAMPLING_ENABLE;
    }
    if (conf->window_resizable) {
        rd->flags |= CG_RENDERER_WINDOW_RESIZABLE;
    }
    if (conf->vsync_enabled) {
        rd->flags |= CG_RENDERER_VSYNC_ENABLE;
    }
    rd->buffer_mode = conf->buffer_mode;


    rd->render_extent.width = conf->initial_window_size.width;
    rd->render_extent.height = conf->initial_window_size.height;

    luna_Renderer_InitializeGraphicsSingleton();
    luna_Renderer_InitializeRenderingComponents(rd, conf);

    for(int i = 0; i < (int)SwapChainImageCount; i++)
	{
        const VkSemaphoreCreateInfo semaphoreCreateInfo = {
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0
        };

        const VkFenceCreateInfo fenceCreateInfo = {
            VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT
        };

        lunaFrameRenderData *data = (lunaFrameRenderData *)cg_vector_get(&rd->renderData, i);
        cassert(vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &data->render_finish_semaphore) == VK_SUCCESS);
        cassert(vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &data->image_available_semaphore) == VK_SUCCESS);
        cassert(vkCreateFence(device, &fenceCreateInfo, NULL, &data->in_flight_fence) == VK_SUCCESS);
	}


    luna_DescriptorPool_Init(&g_pool);

    sprite_empty = sprite_load_disk("../Assets/empty.png");

    camera = ccamera_init();

    // Initialize subsystems.
    csm_compile_updated();
    cinput_init();
    ctext_init(rd);

    luna_VK_BakeGlobalPipelines(rd);

    luna_Renderer_PrepareQuadRenderer(rd);

    return rd;
}

void crenderer_resize(luna_Renderer_t *rd) {
    vkDeviceWaitIdle(device);

    const VkSemaphoreCreateInfo semaphoreCreateInfo = {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0
    };

    const VkFenceCreateInfo fenceCreateInfo = {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT
    };
    
    // adhoc method of resetting them
    for (int i = 0; i < (int)SwapChainImageCount; i++) {
        lunaFrameRenderData *data = (lunaFrameRenderData *)cg_vector_get(&rd->renderData, i);
        vkDestroySemaphore(device, data->image_available_semaphore, NULL);
        vkDestroySemaphore(device, data->render_finish_semaphore, NULL);
        vkDestroyFence(device, data->in_flight_fence, NULL);

        data->image_available_semaphore = NULL;
        data->render_finish_semaphore = NULL;
        data->in_flight_fence = NULL;
    }
    for (u32 i = 0; i < SwapChainImageCount; i++)
    {
        lunaFrameRenderData *data = (lunaFrameRenderData *)cg_vector_get(&rd->renderData, i);
        vkDestroyImageView(device, luna_GPU_TextureGetView(data->sc_image), NULL); // as the view was silently smushed into the structure, we just kinda smush it out as well.
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

    rd->render_extent = (lunaExtent2D){ w, h };

    VkSwapchainKHR old_swapchain = rd->swapchain;

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

    if (rd->flags & CG_RENDERER_VSYNC_ENABLE) {
        switch (rd->buffer_mode) {
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

    luna_GPU_SwapchainCreateInfo scio = {};
    scio.extent.width = rd->render_extent.width;
    scio.extent.height = rd->render_extent.height;
    scio.present_mode = present_mode;
    scio.format = SwapChainImageFormat;
    scio.color_space = SwapChainColorSpace;
    scio.image_count = SwapChainImageCount;
    scio.old_swapchain = old_swapchain;
    luna_GPU_CreateSwapchain(&scio, &rd->swapchain);
    vkDestroySwapchainKHR(device, old_swapchain, NULL);

    // luna_GPU_DestroyImage(&rd->color_image);

    create_optional_images(rd);
    create_framebuffers_and_swapchain_image_views(rd);

    for (int i = 0; i < (int)SwapChainImageCount; i++)
    {
        lunaFrameRenderData *data = (lunaFrameRenderData *)cg_vector_get(&rd->renderData, i);
        cassert(vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &data->render_finish_semaphore) == VK_SUCCESS);
        cassert(vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &data->image_available_semaphore) == VK_SUCCESS);
        cassert(vkCreateFence(device, &fenceCreateInfo, NULL, &data->in_flight_fence) == VK_SUCCESS);
    }

    cg__reset_frame_buffer_resized();
}

bool luna_Renderer_BeginRender(luna_Renderer_t *rd)
{
    lunaFrameRenderData *data = (lunaFrameRenderData *)cg_vector_get(&rd->renderData, rd->renderer_frame);

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

    VkFramebuffer fb = (*(lunaFrameRenderData *)(cg_vector_get(&rd->renderData, rd->imageIndex))).color_framebuffer;
    
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

void luna_Renderer_EndRender(luna_Renderer_t *rd)
{
    const VkCommandBuffer drawBuffer = *(VkCommandBuffer *)cg_vector_get(&rd->drawBuffers, rd->renderer_frame);

    luna_UI_Render(rd);
    luna_ObjectsRender(rd);
    luna_Renderer_RenderAll(rd);

    vkCmdEndRenderPass(drawBuffer);
    vkEndCommandBuffer(drawBuffer);

	VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    const lunaFrameRenderData *data = (lunaFrameRenderData *)cg_vector_get(&rd->renderData, rd->renderer_frame);
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

    rd->renderer_frame = (rd->renderer_frame + 1) % (1 + (int)rd->buffer_mode);
}