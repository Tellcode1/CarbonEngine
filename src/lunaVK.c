#include "../include/lunaVK.h"
#include "../include/lunaPipeline.h"
#include "../include/lunaGFXDef.h"
#include "../include/lunaInternalObjects.h"
#include "../include/lunaImage.h"

#include "../include/camera.h"
#include "../include/cinput.h"
#include "../include/ctext.h"
#include "../include/cengine.h"
#include "../include/lunaObject.h"

#include "../include/cshadermanager.h"
#include "../include/cshadermanagerdev.h"

#include "../include/lunaUI.h"

#include "../include/cgvector.h"

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include "../include/ctext_font_file.h"

#include <SDL2/SDL_vulkan.h>

#include <stdio.h>
#include <stdlib.h>

#define HAS_FLAG(flag) ((luna_GPU_vk_flag_register & flag) || (flags & flag))

// lunaVK.h
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer buffer = VK_NULL_HANDLE;

    luna_GPU_ResultCheckFn __cvk_resultFunc = __cvk_defaultResultCheckFunc; // To not cause nullptr dereference. SetResultCheckFunc also checks for nullptr and handles it.
u32 luna_GPU_vk_flag_register = 0;
// lunaVK.h

// lunaPipelines.h
    SystemPipelines g_Pipelines;

    VkPipeline base_pipeline = NULL;
    VkPipelineCache cache = NULL;

    cg_vector_t g_Samplers;
// lunaPipelines.h

// lunaGFX.h
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
// lunaGFX.h

// lunaGFX vv

lunaExtent2D luna_GetWindowSize()
{
    int ww, wh;
    SDL_GetWindowSize(window, &ww, &wh);
    return (lunaExtent2D){ ww,wh };
}

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

#define ABSF(x) ((x >= 0.0f) ? (x) : -(x))

bool luna_Quad_Visible(const vec3 *pos, const vec3 *siz) {
    const float half_width = siz->x * 0.5f;
    const float half_height = siz->y * 0.5f;
    const float dx = fabsf(pos->x - camera.position.x);
    const float dy = fabsf(pos->y - camera.position.y);

    return (
        dx <= (half_width + CAMERA_ORTHO_W) &&
        dy <= (half_height + CAMERA_ORTHO_H)
    );
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

            if (!luna_Quad_Visible(&drawcall->drawcall.quad.pos, &drawcall->drawcall.quad.siz)) {
                continue;
            }

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

            vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
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
// lunaGFX ^^

// ctext vv

#define MAX(a,b) ( (a > b) ? a : b )

typedef struct cglyph_vertex_t {
    vec3 pos;
    vec2 uv;
} cglyph_vertex_t;

typedef struct ctext_text_drawcall_t {
    u32 vertex_count;
    u32 index_count;
    u32 index_offset;
    cglyph_vertex_t *vertices;
    u32 *indices;
    vec4 color;
} ctext_text_drawcall_t;

struct push_constants {
    mat4 model;
    vec4 color;
    vec4 outline_color;
};

typedef enum ctext_err_t {
    CTEXT_ERR_SUCCESS,
    CTEXT_ERR_INVALID_GLYPH, // May also mean that there just isn't a glyph
    CTEXT_ERR_WRONG_CACHE, // This cache is not the one you're looking for
    CTEXT_ERR_FILE_ERROR,
} ctext_err_t;

void ctext_load_font_from_ft(const char *font_path, bool sdf, cfont_t *dstref) {
    FT_Library lib; FT_Face face;
    if (FT_Init_FreeType(&lib))
    {
        LOG_ERROR("failed to initialize ft\n");
    }

    if (FT_New_Face(lib, font_path, 0, &face))
    {
        LOG_ERROR("failed to load font file: %s\n", font_path);
        FT_Done_FreeType(lib);
    }
    FT_Set_Pixel_Sizes(face, 0, 256);

    strncpy(dstref->family_name, face->family_name, 127);
    strncpy(dstref->style_name, face->style_name, 127);

    if (face->size->metrics.height) {
        dstref->line_height = -face->size->metrics.height / (f32)face->units_per_EM;
    } else {
        dstref->line_height = -(f32)(face->ascender - face->descender) / face->size->metrics.y_scale;
    }
    dstref->atlas = catlas_init(4096, 4096);

    for (int i = 0; i < 256; ++i) {
        FT_UInt glyph_index = FT_Get_Char_Index(face, i);
        if (glyph_index == 0) {
            continue;
        }
        if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT)) {
            continue;
        }

        if (i == ' ') {
            dstref->space_width = (f32)face->glyph->metrics.horiAdvance / (f32)face->units_per_EM;
            continue;
        }

        enum FT_Render_Mode_ render_mode = sdf ? FT_RENDER_MODE_SDF : FT_RENDER_MODE_NORMAL;
        FT_Render_Glyph(face->glyph, render_mode);
        FT_GlyphSlot g = face->glyph;

        const int w = g->bitmap.width;
        const int h = g->bitmap.rows;
        const unsigned char *buffer = g->bitmap.buffer;

        int x,y;
        if (catlas_add_image(&dstref->atlas, w, h, buffer, &x, &y)) {
            printf("atlas error\n");
            continue;
        }

        FT_Glyph gl;
        FT_Get_Glyph(face->glyph, &gl);

        FT_BBox box;
        FT_Glyph_Get_CBox(gl, FT_GLYPH_BBOX_UNSCALED, &box);

        FT_Done_Glyph(gl);

        const ctext_glyph retglyph = {
            .x0 = box.xMin / (f32)face->units_per_EM,
            .x1 = box.xMax / (f32)face->units_per_EM,
            .y0 = box.yMin / (f32)face->units_per_EM,
            .y1 = box.yMax / (f32)face->units_per_EM,

            .l = (float)(x) / dstref->atlas.width,
            .r = (float)(x + w) / dstref->atlas.width,
            .b = (float)(y + h) / dstref->atlas.height,
            .t = (float)(y) / dstref->atlas.height,

            .advance = (f32)face->glyph->metrics.horiAdvance / (f32)face->units_per_EM
        };

        const char glyphi = i;
        cg_hashmap_insert(dstref->glyph_map, &glyphi, &retglyph);
    }

    FT_Done_Face(face);
    FT_Done_FreeType(lib);
}

/* I have no idea what any of this is */

void ctext_load_font(luna_Renderer_t *rd, const char *font_path, int scale, cfont_t **dst)
{
	if (!rd || !dst) {
		LOG_ERROR("pInfo or dst is NULL!");
	}

    if (scale <= 0.0f) {
        LOG_ERROR("attempting to load a font with 0 fontscale.");
    }

    *dst = calloc(1, sizeof(cfont_t));

    cfont_t *dstref = *dst;

    dstref->rd = rd;

    dstref->glyph_map = cg_hashmap_init(16, sizeof(char), sizeof(ctext_glyph), NULL, NULL);
    dstref->drawcalls = cg_vector_init(sizeof(ctext_text_drawcall_t), 4);

    // we somehow need multiple font cache support
    if (ctext_ff_read("atlas.ff", dstref) != CTEXT_ERR_SUCCESS) {
        ctext_load_font_from_ft(font_path, 0, dstref);
        ctext_ff_write("atlas.ff", dstref);
    }

    luna_GPU_TextureCreateInfo image_info = {
        .format = VK_FORMAT_R8_UNORM,
        .samples = CG_SAMPLE_COUNT_1_SAMPLES,
        .type = VK_IMAGE_TYPE_2D,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .extent = (VkExtent3D){ .width = dstref->atlas.width, .height = dstref->atlas.height, .depth = 1 },
        .arraylayers = 1,
        .miplevels = 1
    };
    luna_GPU_CreateTexture(&image_info, &dstref->texture);

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, luna_GPU_TextureGet(dstref->texture), &imageMemoryRequirements);

    luna_GPU_AllocateMemory(imageMemoryRequirements.size, LUNA_GPU_MEMORY_USAGE_GPU_LOCAL, &dstref->texture_mem);
    luna_GPU_BindTextureToMemory(&dstref->texture_mem, 0, dstref->texture);

    const int atlas_w = dstref->atlas.width, atlas_h = dstref->atlas.height;
    luna_VK_StageImageTransfer(
        luna_GPU_TextureGet(dstref->texture),
        dstref->atlas.data,
        atlas_w,
        atlas_h,
        atlas_w * atlas_h * vk_fmt_get_bpp(VK_FORMAT_R8_UNORM)
    );

    const luna_GPU_SamplerCreateInfo sampler_info = {
        .filter = VK_FILTER_LINEAR,
        .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT
    };
    luna_GPU_CreateSampler(&sampler_info, &dstref->sampler);

    luna_GPU_TextureViewCreateInfo view_info = {
        .format = VK_FORMAT_R8_UNORM,
        .view_type = VK_IMAGE_VIEW_TYPE_2D,
        .subresourceRange = (VkImageSubresourceRange){
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };
    luna_GPU_CreateTextureView(&view_info, &dstref->texture);

    const VkDescriptorImageInfo ctext_bitmap_image_info = {
        .sampler = luna_GPU_SamplerGet(dstref->sampler),
        .imageView = luna_GPU_TextureGetView(dstref->texture),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet writeSet = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = rd->ctext->desc_set->set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &ctext_bitmap_image_info
    };
    for (int i = 0; i < CTEXT_MAX_FONT_COUNT; i++)
    {
        writeSet.dstArrayElement = i;
        luna_DescriptorSetSubmitWrite(rd->ctext->desc_set, &writeSet);
    }
}

void ctext__font_resize_buffer(cfont_t *fnt, int new_buffer_size)
{
    int new_allocation_size;

    if (fnt->allocated_size < new_buffer_size) {
        new_allocation_size = MAX(fnt->allocated_size * 2, new_buffer_size);
    }
    else if (new_buffer_size < (fnt->allocated_size / 3)) {
        new_allocation_size = MAX(fnt->allocated_size / 3, new_buffer_size);
    }
    else
        return;

    vkDeviceWaitIdle(device);
    
    if (fnt->buffer.buffers[0]) {
        luna_GPU_DestroyBuffer(&fnt->buffer);
        luna_GPU_FreeMemory(&fnt->buffer_mem);
    }

    luna_GPU_AllocateMemory(new_allocation_size, LUNA_GPU_MEMORY_USAGE_GPU_LOCAL | LUNA_GPU_MEMORY_USAGE_CPU_WRITEABLE, &fnt->buffer_mem);

    luna_GPU_CreateBuffer(
        new_allocation_size,
        LUNA_GPU_ALIGNMENT_UNNECESSARY,
        1,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        &fnt->buffer
    );
    luna_GPU_BindBufferToMemory(&fnt->buffer_mem, 0, &fnt->buffer);

    fnt->allocated_size = new_allocation_size;
}

void ctext__render_drawcalls(luna_Renderer_t *rd, cfont_t *fnt, const mat4 *model) {
    const VkCommandBuffer cmd = luna_Renderer_GetDrawBuffer(rd);
    const VkDeviceSize offsets[] = { 0 };

    struct push_constants pc = {};

    pc.model = *model;

    const VkPipeline pipeline = g_Pipelines.Ctext.pipeline;
    const VkPipelineLayout pipeline_layout = g_Pipelines.Ctext.pipeline_layout;

    const VkDescriptorSet sets[] = { camera.sets->set, rd->ctext->desc_set->set };
    const uint32_t camera_ub_offset = luna_Renderer_GetFrame(rd) * sizeof(camera_uniform_buffer);

    // Viewport && scissor are set by renderer so no need to set them here
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 2, sets, 1, &camera_ub_offset);
    vkCmdBindVertexBuffers(cmd, 0, 1, &fnt->buffer.buffers[0], offsets);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindIndexBuffer(cmd, fnt->buffer.buffers[0], fnt->index_buffer_offset, VK_INDEX_TYPE_UINT32);

    int offset = 0;
    for (int i = 0; i < cg_vector_size(&fnt->drawcalls); i++) {
        ctext_text_drawcall_t *drawcall = (ctext_text_drawcall_t *)cg_vector_get(&fnt->drawcalls, i);

        pc.color = drawcall->color;
        vkCmdPushConstants(
            cmd,
            pipeline_layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(struct push_constants),
            &pc
        );

        vkCmdDrawIndexed(cmd, drawcall->index_count, 1, 0, offset, 0);
        offset += drawcall->vertex_count;
    }
}

void ctext_end_render(luna_Renderer_t *rd, cfont_t *fnt, mat4 model)
{
    if (fnt->to_render) {
        ctext__render_drawcalls(rd, fnt, &model);
    }

    u32 total_vertex_byte_size = 0;
    u32 total_index_count = 0;

    for (int i = 0; i < cg_vector_size(&fnt->drawcalls); i++) {
        const ctext_text_drawcall_t *drawcall = (ctext_text_drawcall_t *)cg_vector_get(&fnt->drawcalls, i);
        total_vertex_byte_size += drawcall->vertex_count * sizeof(cglyph_vertex_t);
        total_index_count += drawcall->index_count;
    }

    if (total_vertex_byte_size == 0 || total_index_count == 0) {
        return;
    }

    const u32 total_index_byte_size = total_index_count * sizeof(u32);
    const u32 total_buffer_size = total_index_byte_size + total_vertex_byte_size;

    ctext__font_resize_buffer(fnt, total_buffer_size);

    uint8_t *mapped;
    vkMapMemory(device, fnt->buffer_mem.memory, 0, fnt->allocated_size, 0, (void **)&mapped);

    // this may be dumb but I am too

    u32 vertex_copy_iterator = 0;
    u32 index_copy_iterator = 0;
    for (int i = 0; i < cg_vector_size(&fnt->drawcalls); i++) {
        const ctext_text_drawcall_t *drawcall = (ctext_text_drawcall_t *)cg_vector_get(&fnt->drawcalls, i);
        memcpy(
            mapped + vertex_copy_iterator,
            drawcall->vertices,
            drawcall->vertex_count * sizeof(cglyph_vertex_t)
        );
        memcpy(
            mapped + total_vertex_byte_size + index_copy_iterator,
            drawcall->indices,
            drawcall->index_count * sizeof(u32)
        );
        vertex_copy_iterator += drawcall->vertex_count * sizeof(cglyph_vertex_t);
        index_copy_iterator += drawcall->index_count * sizeof(u32);
    }
    vkUnmapMemory(device, fnt->buffer_mem.memory);

    fnt->index_buffer_offset = total_vertex_byte_size;
    fnt->index_count = total_index_count;
    fnt->to_render = true;

    fnt->chars_drawn = 0;
    
    for (int i = 0; i < cg_vector_size(&fnt->drawcalls); i++) {
        ctext_text_drawcall_t *drawcall = (ctext_text_drawcall_t *)cg_vector_get(&fnt->drawcalls, i);
        free(drawcall->vertices);
    }
    cg_vector_clear(&fnt->drawcalls);
}

static cg_vector_t split_string(const cg_string_t *str) {
    cg_vector_t result;
    cg_string_t *substr = NULL;
    const char *str_data = cg_string_data(str);
    const int str_len = cg_string_length(str);
    int i_start = 0;

    result = cg_vector_init(sizeof(cg_string_t *), 16);

    for (int i = 0; i < str_len; i++) {
        if (str_data[i] == '\n') {
            if (i > i_start) {
                substr = cg_string_substring(str, i_start, i - i_start);
                // substr should now handle errors internally
                cg_vector_push_back(&result, &substr);
            }
            i_start = i + 1;
        }
    }

    if (i_start < str_len) {
        substr = cg_string_substring(str, i_start, str_len - i_start);
        cg_vector_push_back(&result, &substr);
    }

    return result;
}

// Get the unscaled size of the string
// Warning: slow
static void ctext_get_text_size(const cfont_t *fnt, const cg_string_t *str, float *w, float *h) {
    const int len = cg_string_length(str);
    float width = 0.0f, height = fnt->line_height;

    for (int i = 0; i < len; i++) {
        const char c = str->data[i];
        switch (c) {
            case ' ':
                width += fnt->space_width;
                continue;
            case '\t':
                width += fnt->space_width * 4.0f;
                continue;
            case '\n':
                height += fnt->line_height;
                continue;
            default:
                const ctext_glyph *glyph = (ctext_glyph *)cg_hashmap_find(fnt->glyph_map, &c);
                if (!glyph) {
                    continue;
                }
                width += glyph->advance;
                continue;
        }
    }
    if (w) { *w = width;  }
    if (h) { *h = -height; }
}

void ctext_begin_render(cfont_t *fnt)
{
    (void)fnt;
}

static int render_line(
    const cfont_t *fnt,
    const cg_string_t *str,
    const ctext_text_drawcall_t* drawcall,
    const ctext_text_render_info *pInfo,
    const int glyph_iter,
    f32 x,
    const f32 y,
    float scale
) {
    const int len = cg_string_length(str);

    int iter = 0;
    for (int i = 0; i < len; i++) {
        const char c = str->data[i];
        switch (c) {
            case ' ':
                x += fnt->space_width;
                continue;
            case '\t':
                x += fnt->space_width * 4.0f;
                continue;
            default:
                const ctext_glyph *glyph = (const ctext_glyph *)cg_hashmap_find(fnt->glyph_map, &c);
                if (!glyph) {
                    LOG_INFO("no glyph when rendering char %c", c);
                    continue;
                }
                const f32 glyph_x0 = glyph->x0 + x;
                const f32 glyph_x1 = glyph->x1 + x;
                const f32 glyph_y0 = glyph->y0 + y;
                const f32 glyph_y1 = glyph->y1 + y;

                const int index_offset = (fnt->chars_drawn + iter) * 4;
                cglyph_vertex_t *v_out = drawcall->vertices + (glyph_iter + iter) * 4;
                v_out[0] = (cglyph_vertex_t){ v3muls((vec3){glyph_x0, glyph_y0, pInfo->position.z}, scale), (vec2){glyph->l, glyph->b}, };
                v_out[1] = (cglyph_vertex_t){ v3muls((vec3){glyph_x1, glyph_y0, pInfo->position.z}, scale), (vec2){glyph->r, glyph->b}, };
                v_out[2] = (cglyph_vertex_t){ v3muls((vec3){glyph_x1, glyph_y1, pInfo->position.z}, scale), (vec2){glyph->r, glyph->t}, };
                v_out[3] = (cglyph_vertex_t){ v3muls((vec3){glyph_x0, glyph_y1, pInfo->position.z}, scale), (vec2){glyph->l, glyph->t}  };

                u32 *i_out = drawcall->indices + (glyph_iter + iter) * 6;
                i_out[0] = index_offset;
                i_out[1] = index_offset + 1;
                i_out[2] = index_offset + 2;
                i_out[3] = index_offset + 2;
                i_out[4] = index_offset + 3;
                i_out[5] = index_offset;

                x += glyph->advance;
                iter++;
                continue;
        }
    }

    return iter;
}

static inline int get_effective_length(const char *buf, int buflen) {
    int len = 0;
    for (int i = 0; i < buflen; i++) {
        const char c = buf[i];
        if (c != ' ' && c != '\t' && c != '\n') {
            len++;
        }
    }
    return len;
}

int gen_vertices(
    cfont_t *fnt,
    ctext_text_drawcall_t* drawcall,
    const ctext_text_render_info *pInfo,
    const cg_string_t *str,
    const float scale
)
{
    if (cg_string_length(str) == 0) {
        return 1;
    }

    cg_vector_t splitted_strings = split_string(str);
    const int old_chars_drawn = fnt->chars_drawn, line_count = splitted_strings.m_size;

    // this is here because the functions above this are supposed to
    // make sure line count is not 0
    cassert(line_count > 0);

    float text_w, text_h;
    ctext_get_text_size(fnt, str, &text_w, &text_h);

    const vec2 size = (vec2){text_w * scale, text_h * scale};
    const cmrect2d rect = {
        .position = (vec2){pInfo->position.x, pInfo->position.y},
        .size = v2muls(size, 0.5f)
    };
    const cmrect2d cam = {
        .position = (vec2){ camera.position.x, camera.position.y },
        .size = (vec2){ CAMERA_ORTHO_W, CAMERA_ORTHO_H }
    };

    const bool object_is_in_view =
        fabsf(rect.position.x - cam.position.x) <= (rect.size.x + cam.size.x) &&
        fabsf(rect.position.y - cam.position.y) <= (rect.size.y + cam.size.y);

    if (!object_is_in_view) {
        for (int i = 0; i < line_count; i++) {
            cg_string_t *str = ((cg_string_t **)cg_vector_data(&splitted_strings))[i];
            cg_string_destroy(str);
        }
        cg_vector_destroy(&splitted_strings);
        return 1;
    }

    float ypos = pInfo->position.y;

    switch(pInfo->vertical)
    {
        case CTEXT_VERT_ALIGN_CENTER:
            ypos -= text_h / 2.0f;
            break;
        case CTEXT_VERT_ALIGN_BOTTOM:
            ypos -= text_h;
            break;
        case CTEXT_VERT_ALIGN_TOP:
            break;
        default:
            __builtin_unreachable();
            LOG_ERROR("Invalid vertical alignment. Specified (int)%u. (Implement?)", pInfo->vertical);
            break;
        break;
    }

    for (int i = 0; i < line_count; i++) {;
        // render_line returns the number of chars DRAWN. not the number of characters in the string.
        const cg_string_t *line = ((cg_string_t **)cg_vector_data(&splitted_strings))[i];

        ctext_get_text_size(fnt, line, &text_w, &text_h);

        float xpos = pInfo->position.x;
        switch(pInfo->horizontal) {
            case CTEXT_HORI_ALIGN_CENTER:
                xpos -= text_w / 2.0f;
                break;
            case CTEXT_HORI_ALIGN_RIGHT:
                xpos -= text_w;
                break;
            case CTEXT_HORI_ALIGN_LEFT:
                break;
            default:
                __builtin_unreachable();
                LOG_ERROR("Invalid horizontal alignment. Specified (int)%u. (Implement?)", pInfo->horizontal);
                break;
        }

        fnt->chars_drawn += render_line(
            fnt,
            line,
            drawcall,
            pInfo,
            // This gives us the number of characters this function call has drawn.
            // only this call.
            MAX(fnt->chars_drawn - old_chars_drawn, 0),
            xpos,
            ypos + (i * fnt->line_height),
            scale
        );
    }

    for (int i = 0; i < line_count; i++) {
        cg_string_t *str = ((cg_string_t **)cg_vector_data(&splitted_strings))[i];
        cg_string_destroy(str);
    }
    cg_vector_destroy(&splitted_strings);
    
    return 0;
}

void ctext_render(cfont_t *fnt, const ctext_text_render_info *pInfo, const char *fmt, ...)
{
    cg_string_t *str;
    int num, effective_length;
    {
        char buffer[ 100000 ];
        va_list arg;

        va_start(arg, fmt);
        num = vsnprintf(buffer, 100000, fmt, arg);
        va_end(arg);

        effective_length = get_effective_length(buffer, num);
        if (effective_length == 0) {
            return;
        }

        str = cg_string_init_str(buffer);
    }

    const u32 vertex_count = effective_length * 4;
    const u32 index_count = effective_length * 6;
    const int allocation_size = (vertex_count * sizeof(cglyph_vertex_t)) + (index_count * sizeof(u32));

    void *allocation = malloc(allocation_size);

    ctext_text_drawcall_t drawcall = {};
    drawcall.vertices = (cglyph_vertex_t *)allocation;
    drawcall.index_offset = (vertex_count * sizeof(cglyph_vertex_t));
    drawcall.indices = (u32 *)((uchar *)allocation + drawcall.index_offset);

    drawcall.color = pInfo->color;

    float scale = pInfo->scale ;
    if (pInfo->scale_for_fit) {
        scale *= ctext_get_scale_for_fit(fnt, str, pInfo->bbox);
    }

    // we divide the normal scale by 10 because it's too large.
    scale /= 10.0f;
    
    // can we not have a bounds check before making the vertices?
    // probably not..
    if (gen_vertices(fnt, &drawcall, pInfo, str, scale) != 0) {
        free(allocation);
        cg_string_destroy(str);
        return;
    }

    drawcall.vertex_count = effective_length * 4;
    drawcall.index_count = effective_length * 6;

    cg_vector_push_back(&fnt->drawcalls, &drawcall);

    cg_string_destroy(str);
}

void ctext_init(struct luna_Renderer_t *rd)
{
    rd->ctext = calloc(1, sizeof(cg_ctext_module));
    cg_ctext_module *ctext = rd->ctext;

    const VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, CTEXT_MAX_FONT_COUNT, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
    };

    luna_AllocateDescriptorSet(&g_pool, bindings, array_len(bindings), &ctext->desc_set);

    const VkDescriptorImageInfo empty_img_info = {
        .sampler = sprite_get_sampler(sprite_empty),
        .imageView = sprite_get_internal_view(sprite_empty),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet write_set = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = ctext->desc_set->set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &empty_img_info,
    };
    for (int i = 0; i < CTEXT_MAX_FONT_COUNT; i++) {
        write_set.dstArrayElement = i;
        luna_DescriptorSetSubmitWrite(ctext->desc_set, &write_set);
    }
}

void ctext_ff_write(const char *path, const cfont_t *fnt)
{
    ctext_ff_header header = {
        .line_height = fnt->line_height,
        .space_width = fnt->space_width,
        .bmpwidth = fnt->atlas.width,
        .bmpheight = fnt->atlas.height,
        .numglyphs = cg_hashmap_size(fnt->glyph_map)
    };

    strncpy(header.path, fnt->path, 128);
    strncpy(header.family_name, fnt->family_name, 128);
    strncpy(header.style_name, fnt->style_name, 128);

    FILE *f = fopen(path, "wb");
    cassert(f != NULL);

    fwrite(&header, sizeof(ctext_ff_header), 1, f);
    ch_node_t **glyph_map_root = cg_hashmap_root_node(fnt->glyph_map);
    for (int i = 0; i < cg_hashmap_capacity(fnt->glyph_map); i++) {
        const ch_node_t *node = glyph_map_root[i];
        if (node && node->is_occupied) {
            const int node_key = *(char *)node->key;
            const ctext_glyph *node_value = (ctext_glyph *)node->value;

            const ctext_glyph_entry entry = {
                .codepoint = node_key,
                .advance = node_value->advance,
                .x0 = node_value->x0,
                .x1 = node_value->x1,
                .y0 = node_value->y0,
                .y1 = node_value->y1,
                .l = node_value->l,
                .b = node_value->b,
                .r = node_value->r,
                .t = node_value->t,
            };
            fwrite(&entry, sizeof(ctext_glyph_entry), 1, f);
        }
    }
    fwrite(fnt->atlas.data, fnt->atlas.width * fnt->atlas.height, 1, f);

    const luna_Image img = (luna_Image){
        .w = fnt->atlas.width,
        .h = fnt->atlas.height,
        .data = fnt->atlas.data,
        .fmt = VK_FORMAT_R8_UNORM
    };
    luna_ImageWritePNG(&img, "PISS.png");
    fclose(f);
}

int ctext_ff_read(const char *path, cfont_t *fnt) {
    // warning, we somehow need to check if the cache is the exact font file the user is requesting for...
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return 1;
    }

    ctext_ff_header header;
    fread(&header, sizeof(ctext_ff_header), 1, f);

    bool correct_cache_file = (strncmp(header.path, path, 128) == 0);
    if (!correct_cache_file) {
        // the user is requesting a different font file
        fclose(f);
        return CTEXT_ERR_WRONG_CACHE;
    }

    strncpy(fnt->path, header.path, 128);
    strncpy(fnt->family_name, header.family_name, 128);
    strncpy(fnt->style_name, header.style_name, 128);

    fnt->line_height = header.line_height;
    fnt->space_width = header.space_width;
    fnt->atlas.width = header.bmpwidth;
    fnt->atlas.height = header.bmpheight;

    for (int i = 0; i < header.numglyphs; i++) {
        ctext_glyph_entry entry;
        fread(&entry, sizeof(ctext_glyph_entry), 1, f);

        char glyphi = entry.codepoint;
        ctext_glyph glyph;
        
        glyph.advance = entry.advance;
        glyph.x0 = entry.x0;
        glyph.x1 = entry.x1;
        glyph.y0 = entry.y0;
        glyph.y1 = entry.y1;
        glyph.l = entry.l;
        glyph.b = entry.b;
        glyph.r = entry.r;
        glyph.t = entry.t;
        cg_hashmap_insert(fnt->glyph_map, &glyphi, &glyph);
    }
    fnt->atlas.data = calloc(header.bmpwidth, header.bmpheight);
    fread(fnt->atlas.data, header.bmpwidth * header.bmpheight, 1, f);

    fclose(f);

    return CTEXT_ERR_SUCCESS; // return 0
}

float ctext_get_scale_for_fit(const cfont_t *fnt, const cg_string_t *str, vec2 bbox)
{
    float width, height;
    ctext_get_text_size(fnt, str, &width, &height);

    float scale_x = bbox.x / width;
    float scale_y = bbox.y / height;
    return fminf(scale_x, scale_y);
}

// ctext ^^

// lunaDescriptors vv
void luna_DescriptorSetSubmitWrite(luna_DescriptorSet *set, const VkWriteDescriptorSet *write) {
    set->writes = realloc(set->writes, (set->nwrites+1) * sizeof(VkWriteDescriptorSet));
    cassert(set->writes != NULL);
    memcpy(&set->writes[set->nwrites], write, sizeof(VkWriteDescriptorSet));
    set->nwrites++;
    vkUpdateDescriptorSets(device, 1, write, 0, 0);
}

void luna_DescriptorSetDestroy(luna_DescriptorSet *set) {
    vkDestroyDescriptorSetLayout(device, set->layout, NULL);
    free(set->writes);
    free(set);
}

void luna_DescriptorPoolDestroy(luna_DescriptorPool *pool) {
    for (int i = 0; i < pool->nsets; i++) {
        luna_DescriptorSetDestroy(pool->sets[i]);
    }
    free(pool->sets);
    vkDestroyDescriptorPool(device, pool->pool, NULL);
}

void __luna_DescriptorPool_Allocate(luna_DescriptorPool *pool) {
    VkDescriptorPoolSize allocations[11] = {};
    int descriptors_written = 0;

    for (int i = 0; i < 11; i++) {
        if (pool->descriptors[i].capacity == 0) {
            continue;
        }
        allocations[descriptors_written] = (VkDescriptorPoolSize){ pool->descriptors[i].type, pool->descriptors[i].capacity };
        descriptors_written++;
    }

    bool need_more_max_sets = (pool->nsets + 1) > pool->max_child_sets;
    if (need_more_max_sets) {
            pool->max_child_sets = cmmax(pool->max_child_sets * 2, 1);
    }

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = pool->max_child_sets,
        .poolSizeCount = descriptors_written,
        .pPoolSizes = allocations
    };

    VkDescriptorPool new_pool;
    cassert(vkCreateDescriptorPool(device, &poolInfo, NULL, &new_pool) == VK_SUCCESS);

    VkDescriptorSet *new_sets = malloc(sizeof(VkDescriptorSet) * pool->nsets);
    cassert(new_sets != NULL);

    if (pool->nsets > 0) {
        VkDescriptorSetLayout *layouts = malloc(sizeof(VkDescriptorSetLayout) * pool->nsets);
        for (int i = 0; i < pool->nsets; i++) {
            layouts[i] = pool->sets[i]->layout;
        }

        VkDescriptorSetAllocateInfo setAllocInfo = {};
        setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        setAllocInfo.descriptorPool = new_pool;
        setAllocInfo.descriptorSetCount = pool->nsets;
        setAllocInfo.pSetLayouts = layouts;
        cassert(vkAllocateDescriptorSets(device, &setAllocInfo, new_sets) == VK_SUCCESS);

        free(layouts);
    }

    int ncopies = 0;
    for (int i = 0; i < pool->nsets; i++) {
        ncopies += pool->sets[i]->nwrites;
    }
    VkCopyDescriptorSet *copies = malloc(sizeof(VkCopyDescriptorSet) * ncopies);

    ncopies = 0;
    for (int i = 0; i < pool->nsets; i++) {
        luna_DescriptorSet *old_set = pool->sets[i];

        cassert(old_set->canary == SET_CANARY);

        for (int writei = 0; writei < old_set->nwrites; writei++) {
            VkWriteDescriptorSet *write = &old_set->writes[writei];
             copies[ncopies] = (VkCopyDescriptorSet){
                .sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
                .srcSet = old_set->set,
                .srcBinding = write->dstBinding,
                .srcArrayElement = write->dstArrayElement,
                .dstSet = new_sets[i],
                .dstBinding = write->dstBinding,
                .dstArrayElement = write->dstArrayElement,
                .descriptorCount = write->descriptorCount,
            };
            ncopies++;
        }
        old_set->set = new_sets[i];
    }
    vkUpdateDescriptorSets(device, 0, NULL, ncopies, copies);

    if (pool->pool) {
        vkDestroyDescriptorPool(device, pool->pool, NULL);
    }
    pool->pool = new_pool;

    free(new_sets);
    free(copies);
}

void luna_DescriptorPool_Init(luna_DescriptorPool *dst) {
    *dst = (luna_DescriptorPool){};
    luna_DescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 0, 0 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, 0 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0, 0 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, 0 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 0, 0 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 0, 0 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 0 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, 0 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 0, 0 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 0, 0 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0, 0 },
    };
    memcpy(dst->descriptors, pool_sizes, sizeof(pool_sizes));
    dst->sets = malloc(sizeof(luna_DescriptorSet *));
    dst->max_child_sets = 1;
    dst->nsets = 0;
    dst->canary = POOL_CANARY;
    __luna_DescriptorPool_Allocate(dst);
    cassert(dst->canary == POOL_CANARY);
}

void luna_AllocateDescriptorSet(luna_DescriptorPool *pool, const VkDescriptorSetLayoutBinding *bindings, int nbindings, luna_DescriptorSet **dst) {
    cassert(pool->canary == POOL_CANARY);

    bool need_realloc = 0;
    for (int i = 0; i < 11; i++) {
        for (int j = 0; j < nbindings; j++) {
            luna_DescriptorPoolSize *descriptor = &pool->descriptors[i];
            if (descriptor->type == bindings[j].descriptorType) {
                descriptor->capacity = cmmax(descriptor->capacity * 2, (int)bindings[j].descriptorCount + descriptor->capacity);
                need_realloc = 1;
            }
        }
    }

    if (need_realloc || ((pool->nsets + 1) > pool->max_child_sets)) {
        if ((pool->nsets + 1) > pool->max_child_sets) {
            pool->max_child_sets = cmmax(pool->max_child_sets * 2, 1);
            pool->sets = realloc(pool->sets, pool->max_child_sets * sizeof(luna_DescriptorSet));
        }

        __luna_DescriptorPool_Allocate(pool);
    }

    luna_DescriptorSet *set = calloc(1, sizeof(luna_DescriptorSet));

    pool->sets[pool->nsets] = set;
    pool->nsets++;

    (*dst) = set;

    set->canary = SET_CANARY;
    set->pool = pool;
    set->writes = malloc(sizeof(VkWriteDescriptorSet));
    cassert(set->writes != NULL);

    VkDescriptorSetLayoutCreateInfo layoutinfo = {};
    layoutinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutinfo.pBindings = bindings;
    layoutinfo.bindingCount = nbindings;
    cassert(vkCreateDescriptorSetLayout(device, &layoutinfo, NULL, &set->layout) == VK_SUCCESS);

    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = pool->pool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &set->layout;
    cassert(vkAllocateDescriptorSets(device, &setAllocInfo, &set->set) == VK_SUCCESS);

    cassert(pool->canary == POOL_CANARY);
    cassert(set->canary == SET_CANARY);
}
// lunaDescriptors ^^

void __BakeUnlitPipeline( luna_Renderer_t *rd ) {
    VkDescriptorSetLayoutBinding bindings[] = {
        // binding; descriptorType; descriptorCount; stageFlags; pImmutableSamplers;
        { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
    };

    VkDescriptorSetLayoutCreateInfo layoutinfo = {};
    layoutinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutinfo.pBindings = bindings;
    layoutinfo.bindingCount = 1;
    cassert(vkCreateDescriptorSetLayout(device, &layoutinfo, NULL, &g_Pipelines.Unlit.descriptor_layout) == VK_SUCCESS);

	csm_shader_t *vertex, *fragment;
    csm_load_shader("Unlit/vert", &vertex);
    csm_load_shader("Unlit/frag", &fragment);

    cassert(vertex != NULL && fragment != NULL);

    const csm_shader_t *shaders[] = { vertex, fragment };
    const VkDescriptorSetLayout layouts[] = { camera.sets->layout, g_Pipelines.Unlit.descriptor_layout };

    const lunaExtent2D RenderExtent = luna_Renderer_GetRenderExtent(rd);

	const VkVertexInputAttributeDescription attributeDescriptions[] = {
        // location; binding; format; offset;
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // pos
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(vec3) }, // texcoord
    };

    const VkVertexInputBindingDescription bindingDescriptions[] = {
        // binding; stride; inputRate
        { 0, sizeof(vec3) + sizeof(vec2), VK_VERTEX_INPUT_RATE_VERTEX }
    };

    const VkPushConstantRange pushConstants[] = { 
        // stageFlags, offset, size
        {VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(mat4) + sizeof(vec4) + sizeof(vec2)}
    };

	luna_GPU_PipelineCreateInfo pc = luna_GPU_InitPipelineCreateInfo();
    pc.format = SwapChainImageFormat;
    pc.subpass = 0;
    pc.render_pass = luna_Renderer_GetRenderPass(rd);

    pc.nAttributeDescriptions = array_len(attributeDescriptions);
    pc.pAttributeDescriptions = attributeDescriptions;

    pc.nPushConstants = array_len(pushConstants);
    pc.pPushConstants = pushConstants;

    pc.nBindingDescriptions = array_len(bindingDescriptions);
    pc.pBindingDescriptions = bindingDescriptions;

    pc.nShaders = array_len(shaders);
    pc.pShaders = shaders;

    pc.nDescriptorLayouts = array_len(layouts);
    pc.pDescriptorLayouts = layouts;

    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.samples = Samples;
    luna_GPU_CreatePipelineLayout(&pc, &g_Pipelines.Unlit.pipeline_layout);
    pc.pipeline_layout = g_Pipelines.Unlit.pipeline_layout;
    luna_GPU_CreateGraphicsPipeline(&pc, &g_Pipelines.Unlit.pipeline, 0);
}

void __BakeCtextPipeline( luna_Renderer_t *rd ) {
	const VkVertexInputAttributeDescription attributeDescriptions[] = {
        // location; binding; format; offset;
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, // pos
        { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(vec3) }, // uv
    };

    const VkVertexInputBindingDescription bindingDescriptions[] = {
        // binding; stride; inputRate
        { 0, sizeof(vec3) + sizeof(vec2), VK_VERTEX_INPUT_RATE_VERTEX }
    };

	struct push_constants {
		mat4 model;
		vec4 color;
		vec4 outline_color;
	};

    const VkPushConstantRange pushConstants[] = {
        // stageFlags, offset, size
        { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(struct push_constants) },
    };

    csm_shader_t *vertex, *fragment;
    cassert(csm_load_shader("ctext/vert", &vertex) != -1);
    cassert(csm_load_shader("ctext/frag", &fragment) != -1);

    csm_shader_t * shaders[] = { vertex, fragment };
    VkDescriptorSetLayout layouts[] = { camera.sets->layout, rd->ctext->desc_set->layout };

    const luna_GPU_PipelineBlendState blend = luna_GPU_InitPipelineBlendState(CVK_BLEND_PRESET_ALPHA);

    luna_GPU_PipelineCreateInfo pc = luna_GPU_InitPipelineCreateInfo();
    pc.format = SwapChainImageFormat;
    pc.subpass = 0;
    pc.render_pass = luna_Renderer_GetRenderPass(rd);

    pc.nAttributeDescriptions = array_len(attributeDescriptions);
    pc.pAttributeDescriptions = attributeDescriptions;

    pc.nPushConstants = array_len(pushConstants);
    pc.pPushConstants = pushConstants;

    pc.nBindingDescriptions = array_len(bindingDescriptions);
    pc.pBindingDescriptions = bindingDescriptions;

    pc.nShaders = array_len(shaders);
    pc.pShaders = (const struct csm_shader_t *const *)shaders;

    pc.nDescriptorLayouts = array_len(layouts);
    pc.pDescriptorLayouts = layouts;

    const lunaExtent2D RenderExtent = luna_Renderer_GetRenderExtent(rd);
    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.blend_state = &blend;
    pc.samples = Samples;

    luna_GPU_CreatePipelineLayout(&pc, &g_Pipelines.Ctext.pipeline_layout);
    pc.pipeline_layout = g_Pipelines.Ctext.pipeline_layout;
    luna_GPU_CreateGraphicsPipeline(&pc, &g_Pipelines.Ctext.pipeline, CVK_PIPELINE_FLAGS_ENABLE_BLEND);
}

void __BakeDebugLinePipeline( luna_Renderer_t *rd ) {
	struct line_push_constants {
		mat4 model;
		vec4 color;
		vec2 line_begin;
        vec2 line_end;
	};

    const VkPushConstantRange pushConstants[] = {
        // stageFlags, offset, size
        { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(struct line_push_constants) },
    };

    csm_shader_t *vertex, *fragment;
    cassert(csm_load_shader("Debug/Line/vert", &vertex) != -1);
    cassert(csm_load_shader("Debug/Line/frag", &fragment) != -1);

    csm_shader_t * shaders[] = { vertex, fragment };
    VkDescriptorSetLayout layouts[] = { camera.sets->layout };

    const luna_GPU_PipelineBlendState blend = luna_GPU_InitPipelineBlendState(CVK_BLEND_PRESET_ALPHA);

    luna_GPU_PipelineCreateInfo pc = luna_GPU_InitPipelineCreateInfo();
    pc.format = SwapChainImageFormat;

    pc.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    pc.render_pass = luna_Renderer_GetRenderPass(rd);

    pc.nAttributeDescriptions = 0;
    pc.pAttributeDescriptions = NULL;

    pc.nPushConstants = array_len(pushConstants);
    pc.pPushConstants = pushConstants;

    pc.nBindingDescriptions = 0;
    pc.pBindingDescriptions = NULL;

    pc.nShaders = array_len(shaders);
    pc.pShaders = (const struct csm_shader_t *const *)shaders;

    pc.nDescriptorLayouts = array_len(layouts);
    pc.pDescriptorLayouts = layouts;

    const lunaExtent2D RenderExtent = luna_Renderer_GetRenderExtent(rd);
    pc.extent.width = RenderExtent.width;
    pc.extent.height = RenderExtent.height;
    pc.blend_state = &blend;
    pc.samples = Samples;

    luna_GPU_CreatePipelineLayout(&pc, &g_Pipelines.Line.pipeline_layout);
    pc.pipeline_layout = g_Pipelines.Line.pipeline_layout;
    luna_GPU_CreateGraphicsPipeline(&pc, &g_Pipelines.Line.pipeline, CVK_PIPELINE_FLAGS_ENABLE_BLEND);
}

void luna_VK_BakeGlobalPipelines( luna_Renderer_t *rd )
{
	__BakeUnlitPipeline(rd);
    __BakeDebugLinePipeline(rd);
	__BakeCtextPipeline(rd);
}

void luna_GPU_CreateGraphicsPipeline(const luna_GPU_PipelineCreateInfo *pCreateInfo, VkPipeline *dstPipeline, u32 flags)
{
	CVK_REQUIRED_PTR(device);
	CVK_REQUIRED_PTR(pCreateInfo);
	CVK_REQUIRED_PTR(dstPipeline);
	CVK_REQUIRED_PTR(pCreateInfo->render_pass);
	CVK_NOT_EQUAL_TO(pCreateInfo->nShaders, 0);
	CVK_NOT_EQUAL_TO(pCreateInfo->format, VK_FORMAT_UNDEFINED);
	CVK_NOT_EQUAL_TO(pCreateInfo->extent.width, 0);
	CVK_NOT_EQUAL_TO(pCreateInfo->extent.height, 0);

	if(HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING)) {
		// Vulkan requires samples to not be 1.
		CVK_NOT_EQUAL_TO(pCreateInfo->samples, VK_SAMPLE_COUNT_1_BIT);
	}

	VkPipelineVertexInputStateCreateInfo vertexInputState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,

		.vertexBindingDescriptionCount = pCreateInfo->nBindingDescriptions,
		.pVertexBindingDescriptions = pCreateInfo->pBindingDescriptions,
		.vertexAttributeDescriptionCount = pCreateInfo->nAttributeDescriptions,
		.pVertexAttributeDescriptions = pCreateInfo->pAttributeDescriptions,
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	inputAssemblyState.topology = pCreateInfo->topology;

	VkViewport viewportState = {};
	viewportState.x = 0;
	viewportState.y = 0;
	viewportState.width = (f32)(pCreateInfo->extent.width);
	viewportState.height = (f32)(pCreateInfo->extent.height);
	viewportState.minDepth = 0.0f;
	viewportState.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = (VkOffset2D){0, 0};
	scissor.extent = pCreateInfo->extent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.pViewports = &viewportState;
	viewportStateCreateInfo.pScissors = &scissor;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.viewportCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizerPipelineStateCreateInfo = {};
	rasterizerPipelineStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerPipelineStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizerPipelineStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerPipelineStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	
	if(HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_CULLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_CULLING))
		rasterizerPipelineStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; // No one uses other culling modes. If you do, I hate you.
	else
		rasterizerPipelineStateCreateInfo.cullMode = VK_CULL_MODE_NONE;

	rasterizerPipelineStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerPipelineStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizerPipelineStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizerPipelineStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizerPipelineStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizerPipelineStateCreateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisamplerPipelineStageCreateInfo = {};
	multisamplerPipelineStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplerPipelineStageCreateInfo.sampleShadingEnable = VK_FALSE;
	multisamplerPipelineStageCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisamplerPipelineStageCreateInfo.alphaToOneEnable = VK_FALSE;
	multisamplerPipelineStageCreateInfo.minSampleShading = 1.0f;
	multisamplerPipelineStageCreateInfo.pSampleMask = VK_NULL_HANDLE;
	
	if(HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING))
		multisamplerPipelineStageCreateInfo.rasterizationSamples = pCreateInfo->samples;
	else
		multisamplerPipelineStageCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorblendAttachmentState = {};

	if(HAS_FLAG(CVK_PIPELINE_FLAGS_ENABLE_BLEND))
	{
		const luna_GPU_PipelineBlendState  *blendState = pCreateInfo->blend_state;

		colorblendAttachmentState.blendEnable = VK_TRUE;
		colorblendAttachmentState.srcColorBlendFactor = blendState->srcColorBlendFactor;
		colorblendAttachmentState.dstColorBlendFactor = blendState->dstColorBlendFactor;
		colorblendAttachmentState.colorBlendOp = blendState->colorBlendOp;
		colorblendAttachmentState.srcAlphaBlendFactor = blendState->srcAlphaBlendFactor;
		colorblendAttachmentState.dstAlphaBlendFactor = blendState->dstAlphaBlendFactor;
		colorblendAttachmentState.alphaBlendOp = blendState->alphaBlendOp;
		colorblendAttachmentState.colorWriteMask = blendState->colorWriteMask;
	} else
	{
		colorblendAttachmentState.blendEnable = VK_FALSE;
		colorblendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo colorblendState = {};
	colorblendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorblendState.attachmentCount = 1;
	colorblendState.pAttachments = &colorblendAttachmentState;
	colorblendState.logicOp = VK_LOGIC_OP_COPY;
	colorblendState.logicOpEnable = VK_FALSE;
	colorblendState.blendConstants[0] = 0.0f;
	colorblendState.blendConstants[1] = 0.0f;
	colorblendState.blendConstants[2] = 0.0f;
	colorblendState.blendConstants[3] = 0.0f;

	VkPipelineShaderStageCreateInfo *shader_infos = (VkPipelineShaderStageCreateInfo *)calloc(pCreateInfo->nShaders, sizeof(VkPipelineShaderStageCreateInfo));
	for (int i = 0; i < pCreateInfo->nShaders; i++) {
		shader_infos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_infos[i].stage = (VkShaderStageFlagBits)pCreateInfo->pShaders[i]->stage;
		shader_infos[i].module = (VkShaderModule)pCreateInfo->pShaders[i]->shader_module;
		shader_infos[i].pName = "main";
	}

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = pCreateInfo->nShaders,
		.pStages = shader_infos,
		.pVertexInputState = &vertexInputState,
		.pInputAssemblyState = &inputAssemblyState,
		.pViewportState = &viewportStateCreateInfo,
		.pRasterizationState = &rasterizerPipelineStateCreateInfo,
		.pMultisampleState = &multisamplerPipelineStageCreateInfo,
		.pColorBlendState = &colorblendState,
		.layout = pCreateInfo->pipeline_layout,
		.renderPass = pCreateInfo->render_pass,
		.subpass = pCreateInfo->subpass,
		.basePipelineHandle = pCreateInfo->old_pipeline,
		.basePipelineIndex = 0, // ?
	};

	VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
	const VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_DYNAMIC_VIEWPORT) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_DYNAMIC_VIEWPORT)) {}
	else  {
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = 2;
		dynamicStateInfo.pDynamicStates = dynamicStates;
		graphicsPipelineCreateInfo.pDynamicState = &dynamicStateInfo;
	}
	
	VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
	if(HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_DEPTH_CHECK))
	{
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_TRUE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilState.depthBoundsTestEnable = VK_FALSE;
		depthStencilState.minDepthBounds = 0.0f;
		depthStencilState.maxDepthBounds = 1.0f;
		depthStencilState.stencilTestEnable = VK_FALSE;

		graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilState;
	}

	graphicsPipelineCreateInfo.basePipelineHandle = base_pipeline;

	// if(cacheIsNull) cacheCreator.join();
	CVK_ResultCheck(vkCreateGraphicsPipelines(device, pCreateInfo->cache, 1, &graphicsPipelineCreateInfo, VK_NULL_HANDLE, dstPipeline));

	base_pipeline = *dstPipeline;

	free(shader_infos);
}

void luna_GPU_CreateRenderPass(luna_GPU_RenderPassCreateInfo const *pCreateInfo, VkRenderPass *dstRenderPass, u32 flags)
{
	CVK_REQUIRED_PTR(device);
    CVK_REQUIRED_PTR(pCreateInfo);
    CVK_REQUIRED_PTR(dstRenderPass);
    CVK_NOT_EQUAL_TO(pCreateInfo->format, VK_FORMAT_UNDEFINED);

    if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_DEPTH_CHECK))
    {
        CVK_NOT_EQUAL_TO(pCreateInfo->depthBufferFormat, VK_FORMAT_UNDEFINED);
    }

    VkAttachmentDescription colorAttachmentDescription = {};
    colorAttachmentDescription.format = pCreateInfo->format;
    colorAttachmentDescription.samples = (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING)) ? pCreateInfo->samples : VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING)) ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // colorAttachmentDescription.finalLayout = (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING)) ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;

    cg_vector_t /* VkAttachmentDescription */ attachments = cg_vector_init(sizeof(VkAttachmentDescription), 5);
	cg_vector_push_back(&attachments, &colorAttachmentDescription);

	VkAttachmentDescription depthAttachment = {};
	VkAttachmentReference depthAttachmentRef = {};
    if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_DEPTH_CHECK))
    {
		depthAttachment.format = pCreateInfo->depthBufferFormat; // Why wasn't this being used?
    	depthAttachment.samples = pCreateInfo->samples;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		// if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING))
		// 	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// else
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		depthAttachmentRef.attachment = cg_vector_size(&attachments);
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        subpass.pDepthStencilAttachment = &depthAttachmentRef;

		cg_vector_push_back(&attachments, &depthAttachment);
    }

	VkAttachmentReference colorAttachmentResolveRef = {};
	VkAttachmentDescription colorAttachmentResolve = {};
    if ((flags & CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) && !(flags & CVK_PIPELINE_FLAGS_UNFORCE_MULTISAMPLING))
    {
        colorAttachmentResolve.format = pCreateInfo->format;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        colorAttachmentResolveRef.attachment = cg_vector_size(&attachments);
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		cg_vector_push_back(&attachments, &colorAttachmentResolve);

        subpass.pResolveAttachments = &colorAttachmentResolveRef;
    }

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = cg_vector_size(&attachments);
    renderPassInfo.pAttachments = (const VkAttachmentDescription *)cg_vector_data(&attachments);
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 0;
    renderPassInfo.pDependencies = NULL;

    CVK_ResultCheck(vkCreateRenderPass(device, &renderPassInfo, VK_NULL_HANDLE, dstRenderPass));
}

void luna_GPU_CreatePipelineLayout(luna_GPU_PipelineCreateInfo const *pCreateInfo, VkPipelineLayout *dstLayout)
{
	CVK_REQUIRED_PTR(device);
	CVK_REQUIRED_PTR(pCreateInfo);
	CVK_REQUIRED_PTR(dstLayout);

    // int totalLayouts = 0;
	// for (int i = 0; i < pCreateInfo->nShaders; i++) {
	// 	totalLayouts += pCreateInfo->pShaders[i]->nsetlayouts;
	// }

	// cg_vector_t *sets = cg_vector_init(sizeof(VkDescriptorSetLayout), totalLayouts);

	// for (int i = 0; i < pCreateInfo->nShaders; i++) {
	// 	const csm_shader_t *shader = pCreateInfo->pShaders[i];
	// 	for (int j = 0; j < shader->nsetlayouts; j++) {
	// 		cg_vector_push_back(sets, &shader->setlayouts[j]);
	// 	}
	// }

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = pCreateInfo->nDescriptorLayouts;
	pipelineLayoutCreateInfo.pSetLayouts = pCreateInfo->pDescriptorLayouts;
	pipelineLayoutCreateInfo.pushConstantRangeCount = pCreateInfo->nPushConstants;
	pipelineLayoutCreateInfo.pPushConstantRanges = pCreateInfo->pPushConstants;

	CVK_ResultCheck(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, VK_NULL_HANDLE, dstLayout));
}

void luna_GPU_CreateSwapchain(luna_GPU_SwapchainCreateInfo const* pCreateInfo, VkSwapchainKHR *dstSwapchain)
{
	CVK_REQUIRED_PTR(device);
	CVK_REQUIRED_PTR(pCreateInfo);
	CVK_NOT_EQUAL_TO(pCreateInfo->extent.width, 0);
	CVK_NOT_EQUAL_TO(pCreateInfo->extent.height, 0);
	CVK_NOT_EQUAL_TO(pCreateInfo->format, VK_FORMAT_UNDEFINED);
	CVK_NOT_EQUAL_TO(pCreateInfo->image_count, 0);

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;
	swapChainCreateInfo.imageExtent = pCreateInfo->extent;
	swapChainCreateInfo.minImageCount = pCreateInfo->image_count;
	swapChainCreateInfo.imageFormat = pCreateInfo->format;
	swapChainCreateInfo.imageColorSpace = pCreateInfo->color_space;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.clipped = VK_TRUE;
	swapChainCreateInfo.presentMode = pCreateInfo->present_mode;
	swapChainCreateInfo.oldSwapchain = pCreateInfo->old_swapchain;
	swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	CVK_ResultCheck(vkCreateSwapchainKHR(device, &swapChainCreateInfo, VK_NULL_HANDLE, dstSwapchain));
}

luna_GPU_PipelineBlendState luna_GPU_InitPipelineBlendState(luna_GPU_PipelineBlendPreset preset)
{
	luna_GPU_PipelineBlendState ret = {};
	ret.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	switch(preset)
	{
		case CVK_BLEND_PRESET_NONE:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.colorBlendOp = VK_BLEND_OP_ADD;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
		case CVK_BLEND_PRESET_ALPHA:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			ret.colorBlendOp = VK_BLEND_OP_ADD;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
		case CVK_BLEND_PRESET_ADDITIVE:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.colorBlendOp = VK_BLEND_OP_ADD;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
		case CVK_BLEND_PRESET_MULTIPLICATIVE:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.colorBlendOp = VK_BLEND_OP_ADD;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
		case CVK_BLEND_PRESET_PREMULTIPLIED_ALPHA:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			ret.colorBlendOp = VK_BLEND_OP_ADD;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			ret.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
		case CVK_BLEND_PRESET_SUBTRACTIVE:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.colorBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			ret.alphaBlendOp = VK_BLEND_OP_REVERSE_SUBTRACT;
			break;
		default:
			ret.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.colorBlendOp = VK_BLEND_OP_ADD;
			ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			ret.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
	}
	return ret;
}

void luna_VK_CreateBuffer( u64 size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer *dstBuffer, VkDeviceMemory *retMem, bool externallyAllocated)
{
    if(size == 0) {
        return;
    }

    VkBuffer newBuffer;
    VkDeviceMemory newMemory;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &bufferCreateInfo, NULL, &newBuffer) != VK_SUCCESS)
    {
        printf("Renderer::failed to create staging buffer!");
    }

    VkMemoryRequirements bufferMemoryRequirements;
    vkGetBufferMemoryRequirements(device, newBuffer, &bufferMemoryRequirements);
    
    if(!externallyAllocated)
    {
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = bufferMemoryRequirements.size;
        allocInfo.memoryTypeIndex = luna_VK_GetMemType(bufferMemoryRequirements.memoryTypeBits, propertyFlags);
        if (vkAllocateMemory(device, &allocInfo, NULL, &newMemory) != VK_SUCCESS)
        {
            LOG_ERROR("failed to alloc gpu memory for buffer");
        }
        
        vkBindBufferMemory(device, newBuffer, newMemory, 0);
        *retMem = newMemory;
    }

    *dstBuffer = newBuffer;
}

void luna_VK_StageBufferTransfer(VkBuffer dst, void *data, u64 size)
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkBufferCreateInfo stagingBufferInfo = {};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = size;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device, &stagingBufferInfo, NULL, &stagingBuffer) != VK_SUCCESS)
    {
        printf("Failed to create staging buffer!");
    }

    VkMemoryRequirements stagingBufferMemoryRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferMemoryRequirements);
    
    VkMemoryAllocateInfo stagingBufferAlloc = {};
    stagingBufferAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingBufferAlloc.allocationSize = stagingBufferMemoryRequirements.size;
    stagingBufferAlloc.memoryTypeIndex = luna_VK_GetMemType(stagingBufferMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (vkAllocateMemory(device, &stagingBufferAlloc, NULL, &stagingBufferMemory) != VK_SUCCESS)
    {
        printf("Failed to allocate staging buffer memory!");
    }
    
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void* mapped;
    vkMapMemory(device, stagingBufferMemory, 0, size, 0, &mapped);
    memcpy(mapped, data, size);
    vkUnmapMemory(device, stagingBufferMemory);

    const VkBufferCopy copy = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };
    
    VkCommandBuffer cmd = luna_VK_BeginCommandBuffer();
    vkCmdCopyBuffer(cmd, stagingBuffer, dst, 1, &copy);
    luna_VK_EndCommandBuffer(cmd, TransferQueue, 1);

    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);

}

u32 luna_VK_GetMemType( const u32 memoryTypeBits, const VkMemoryPropertyFlags memoryProperties)
{
    VkPhysicalDeviceMemoryProperties properties;
	vkGetPhysicalDeviceMemoryProperties(physDevice, &properties);

	for (u32 i = 0; i < properties.memoryTypeCount; i++) 
	{
    	if ((memoryTypeBits & (1 << i)) && (properties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties)
        	return i;
	}

    return UINT32_MAX;
}

VkCommandBuffer luna_VK_BeginCommandBufferFrom( VkCommandBuffer src)
{
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(src, &beginInfo);

    return src;
}

VkCommandBuffer luna_VK_BeginCommandBuffer()
{
    if (!pool) {
        VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
        cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolCreateInfo.queueFamilyIndex = GraphicsFamilyIndex;
        cmdPoolCreateInfo.flags = 0;
        if(vkCreateCommandPool(device, &cmdPoolCreateInfo, NULL, &pool) != VK_SUCCESS) {
            LOG_ERROR("Failed to create command pool");
        }
    }

    VkCommandBufferAllocateInfo cmdAllocInfo = {};
    cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    cmdAllocInfo.commandPool = pool;
    vkAllocateCommandBuffers(device, &cmdAllocInfo, &buffer);

    return luna_VK_BeginCommandBufferFrom(buffer);
}

VkResult luna_VK_EndCommandBuffer( VkCommandBuffer cmd, VkQueue queue, bool waitForExecution)
{
    VkResult res = vkEndCommandBuffer(cmd);
    if (res != VK_SUCCESS) return res;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0;

    if (waitForExecution) {
        res = vkCreateFence(device, &fenceInfo, NULL, &fence);
        if (res != VK_SUCCESS) return res;
    }

    res = vkQueueSubmit(queue, 1, &submitInfo, fence);
    if (res != VK_SUCCESS) return res;

    if (waitForExecution) {
        if (fence != VK_NULL_HANDLE) {
            res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
            if (res != VK_SUCCESS) return res;
            vkDestroyFence(device, fence, NULL);
        }
        vkDeviceWaitIdle(device);
        vkFreeCommandBuffers(device, pool, 1, &cmd);
        buffer = NULL;
    }

    return VK_SUCCESS;
}

// void help::Files::LoadBinary(const char * path, cg_vector<u8>* dst)
// {
//     FILE *f = fopen(path, "rb");
//     cassert(f != NULL);

//     fseek(f, 0, SEEK_END);
//     int file_size = ftell(f);
//     rewind(f);

//     dst->resize( file_size );
//     fread(dst->data(), 1, file_size, f);

//     fclose(f);
// }

void luna_VK_LoadBinaryFile( const char* path, u8* dst, u32* dstSize) {
    FILE *f = fopen(path, "rb");
    cassert(f != NULL);

    fseek(f, 0, SEEK_END);
    int file_size = ftell(f);

    if (dst == NULL) {
        *dstSize = file_size;
        fclose(f);
        return;
    }

    rewind(f);

    fread(dst, 1, file_size, f);

    fclose(f);
}

void luna_VK_StageImageTransfer(VkImage dst, const void *data, int width, int height, int image_size)
{
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(device, dst, &mem_req);

    const VkBufferCreateInfo stagingBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = mem_req.size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    vkCreateBuffer(device, &stagingBufferInfo, NULL, &stagingBuffer);

    VkMemoryRequirements stagingBufferRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferRequirements);

    const VkMemoryAllocateInfo stagingBufferAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = stagingBufferRequirements.size,
        .memoryTypeIndex = luna_VK_GetMemType(stagingBufferRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    vkAllocateMemory(device, &stagingBufferAllocInfo, NULL, &stagingBufferMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void *stagingBufferMapped;
    cassert(vkMapMemory(device, stagingBufferMemory, 0, stagingBufferRequirements.size, 0, &stagingBufferMapped) == VK_SUCCESS);
    memcpy(stagingBufferMapped, data, image_size);
    vkUnmapMemory(device, stagingBufferMemory);

    const VkCommandBuffer cmd = luna_VK_BeginCommandBuffer(device);

    luna_VK_TransitionTextureLayout(
        cmd, dst, 1, VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        0, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
    );

    VkBufferImageCopy region = {};
    region.imageExtent = (VkExtent3D){ width, height, 1 };
    region.imageOffset =(VkOffset3D){ 0, 0, 0 };
    region.bufferOffset = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.mipLevel = 0;
    vkCmdCopyBufferToImage(cmd, stagingBuffer, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    luna_VK_TransitionTextureLayout(
            cmd, dst, 1, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            0, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );

    luna_VK_EndCommandBuffer(cmd, TransferQueue, true);

    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);
}

void luna_VK_CreateTextureFromMemory(u8 *buffer, u32 width, u32 height, VkFormat format, VkImage *dst, VkDeviceMemory *dstMem)
{
    luna_VK_CreateTextureEmpty(width, height, format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, NULL, dst, dstMem);
    luna_VK_StageImageTransfer(*dst, buffer, width, height, width * height * vk_fmt_get_bpp(format));
}

void luna_VK_CreateTextureEmpty(u32 width, u32 height, VkFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage, int *image_size, VkImage *dst, VkDeviceMemory *dstMem)
{
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.format = format;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = usage;
    imageCreateInfo.samples = samples;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageCreateInfo, NULL, dst) != VK_SUCCESS)
        LOG_ERROR("Failed to create image");

    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(device, *dst, &imageMemoryRequirements);

    if (image_size) {
        *image_size = imageMemoryRequirements.size;
    }

    // allow for preallocated memory.
    if (dstMem != NULL) {

        const u32 localDeviceMemoryIndex = luna_VK_GetMemType(imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = imageMemoryRequirements.size;
        allocInfo.memoryTypeIndex = localDeviceMemoryIndex;

        vkAllocateMemory(device, &allocInfo, NULL, dstMem);
        vkBindImageMemory(device, *dst, *dstMem, 0);
    }
}

u8* luna_VK_CreateTextureFromDisk(const char *path, u32 *width, u32 *height, VkFormat *channels, VkImage *dst, VkDeviceMemory *dstMem)
{
    luna_Image tex = luna_ImageLoad(path);

    cassert(tex.data != NULL);

    *width = tex.w;
    *height = tex.h;
    *channels = tex.fmt;

    luna_VK_CreateTextureFromMemory(tex.data, tex.w, tex.h, *channels, dst, dstMem);
    return tex.data;
}

void luna_VK_TransitionTextureLayout( VkCommandBuffer cmd, VkImage image,
										 u32 mipLevels,
                                         VkImageAspectFlagBits aspect,
										 VkImageLayout oldLayout,
										 VkImageLayout newLayout,
										 VkAccessFlags srcAccessMask,
										 VkAccessFlags dstAccessMask,
										 VkPipelineStageFlags sourceStage,
										 VkPipelineStageFlags destinationStage)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspect;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

bool luna_VK_GetSupportedFormat( VkPhysicalDevice physDevice, VkSurfaceKHR surface, VkFormat *dstFormat, VkColorSpaceKHR *dstColorSpace)
{
	CVK_REQUIRED_PTR(device);
	CVK_REQUIRED_PTR(physDevice);
	CVK_REQUIRED_PTR(surface);
	CVK_REQUIRED_PTR(dstFormat);
	CVK_REQUIRED_PTR(dstColorSpace);

    u32 formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, VK_NULL_HANDLE);
	cg_vector_t /* VkSurfaceFormatKHR */ surfaceFormats = cg_vector_init(sizeof(VkSurfaceFormatKHR), formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, (VkSurfaceFormatKHR *)cg_vector_data(&surfaceFormats));

	VkSurfaceFormatKHR selectedFormat = { VK_FORMAT_MAX_ENUM, VK_COLOR_SPACE_MAX_ENUM_KHR };

	const VkSurfaceFormatKHR desired_formats[] = {
		{ VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR },
		{ VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }
	};

	for (u32 i = 0; i < formatCount; i++)
	{
		const VkSurfaceFormatKHR *surfaceFormat = (VkSurfaceFormatKHR *)cg_vector_get(&surfaceFormats, i);
		for (u32 j = 0; j < array_len(desired_formats); j++) {
			if (surfaceFormat->format == desired_formats[j].format 
				&&
				surfaceFormat->colorSpace == desired_formats[j].colorSpace
			   ) {
					selectedFormat.format = surfaceFormat->format;
					selectedFormat.colorSpace = surfaceFormat->colorSpace;
			   }
		}
	}

    cg_vector_destroy(&surfaceFormats);
	if (selectedFormat.format == VK_FORMAT_MAX_ENUM || selectedFormat.colorSpace == VK_COLOR_SPACE_MAX_ENUM_KHR) {
		return VK_FALSE;
	} else {
		*dstFormat = selectedFormat.format;
		*dstColorSpace = selectedFormat.colorSpace;
		
		return VK_TRUE;
	}

	return VK_FALSE;
}

u32 luna_VK_GetSurfaceImageCount(VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{
	CVK_REQUIRED_PTR(physDevice);
	CVK_REQUIRED_PTR(surface);

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &surfaceCapabilities);

	u32 requestedImageCount = surfaceCapabilities.minImageCount + 1;
	if(requestedImageCount < surfaceCapabilities.maxImageCount)
		requestedImageCount = surfaceCapabilities.maxImageCount;

	return requestedImageCount;
}

// LUNA_GPU_OBJECTS

// these parameters should be replaced
// properties should be replaced by usage. Like LUNA_GPU_MEMORY_USAGE_GPU, CPU_TO_GPU, GPU_TO_CPU, etc.
void luna_GPU_AllocateMemory(VkDeviceSize size, luna_GPU_MemoryUsage usage, luna_GPU_Memory *dst) {
    dst->size = size;
    dst->usage = usage;
    const VkMemoryPropertyFlags properties = (VkMemoryPropertyFlags)usage;

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = size;

    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &mem_properties);

    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((properties & mem_properties.memoryTypes[i].propertyFlags) == properties) {
            alloc_info.memoryTypeIndex = i;
            break;
        }
    }

    if (vkAllocateMemory(device, &alloc_info, NULL, &dst->memory) != VK_SUCCESS || !dst->memory) {
        printf("An allocation has failed! What are we to do???\n");
    }
}

void luna_GPU_CreateBuffer(int size, int alignment, int nchilds, VkBufferUsageFlags usage, luna_GPU_Buffer *dst) {
    dst->allocation.size = size;
    dst->size = size;
    dst->alignment = alignment;

    const int aligned_sz = align_up(size, alignment);

    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = aligned_sz;
    buffer_info.usage = usage;
    for (int i = 0; i < nchilds; i++) {
        cassert(vkCreateBuffer(device, &buffer_info, NULL, &dst->buffers[i]) == VK_SUCCESS);
    }
    dst->nchilds = nchilds;
}

void luna_GPU_FreeMemory(luna_GPU_Memory *mem) {
    vkFreeMemory(device, mem->memory, NULL);
}

// I think these given an error when, say offset is too big so maybe we can check their return values?
void luna_GPU_BindBufferToMemory(luna_GPU_Memory *mem, int offset, luna_GPU_Buffer *buffer) {
    buffer->allocation.memory = mem;
    buffer->allocation.memory_offset = offset;

    const int aligned_sz = align_up(buffer->size, buffer->alignment);
    for (int i = 0; i < buffer->nchilds; i++) {
        vkBindBufferMemory(device, buffer->buffers[i], mem->memory, offset + i * aligned_sz);
    }
}

void luna_GPU_TextureAttachView(luna_GPU_Texture *tex, VkImageView view)
{
    tex->view = view;
}

void luna_GPU_BindTextureToMemory(luna_GPU_Memory *mem, int offset, luna_GPU_Texture *tex) {

    tex->allocation.memory = mem;
    tex->allocation.memory_offset = offset;
    vkBindImageMemory(device, tex->image, mem->memory, offset);
}

void luna_GPU_DestroyBuffer(luna_GPU_Buffer *buffer) {
    for (int i = 0; i < buffer->nchilds; i++) {
        if (buffer->buffers[i]) {
            vkDestroyBuffer(device, buffer->buffers[i], NULL);
        } else {
            LOG_INFO("Attempt to destroy a buffer (child index %i) which is NULL", i);
        }
    }
}

void luna_GPU_DestroyTexture(luna_GPU_Texture *tex) {
    if (tex->image) {
        vkDestroyImage(device, tex->image, NULL);
        vkDestroyImageView(device, tex->view, NULL);
    } else {
        LOG_INFO("Attempt to destroy an image which is NULL");
    }
}

void luna_GPU_CreateSampler(const luna_GPU_SamplerCreateInfo *pInfo, luna_GPU_Sampler **sampler) {
    for (int i = 0; i < g_Samplers.m_size; i++) {
        luna_GPU_Sampler *cache = &((luna_GPU_Sampler *)cg_vector_data(&g_Samplers))[i];
        if (
                cache != NULL &&
                cache->filter == pInfo->filter &&
                cache->mipmap_mode == pInfo->mipmap_mode &&
                cache->address_mode == pInfo->address_mode &&
                cache->max_anisotropy == pInfo->max_anisotropy &&
                cache->mip_lod_bias == pInfo->mip_lod_bias &&
                cache->min_lod == pInfo->min_lod &&
                cache->max_lod == pInfo->max_lod &&
                cache->vksampler != NULL) {
            
            *sampler = cache;
            return;
        }
    }

    luna_GPU_Sampler smap = {
        .filter = pInfo->filter,
        .mipmap_mode = pInfo->mipmap_mode,
        .address_mode = pInfo->address_mode,
        .max_anisotropy = pInfo->max_anisotropy,
        .mip_lod_bias = pInfo->mip_lod_bias,
        .min_lod = pInfo->min_lod,
        .max_lod = pInfo->max_lod,
    };

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = pInfo->filter;
    samplerInfo.minFilter = pInfo->filter;
    samplerInfo.mipmapMode = pInfo->mipmap_mode;
    samplerInfo.addressModeU = pInfo->address_mode;
    samplerInfo.addressModeV = pInfo->address_mode;
    samplerInfo.addressModeW = pInfo->address_mode;
    samplerInfo.anisotropyEnable = pInfo->max_anisotropy > 1.0f;
    samplerInfo.maxLod = pInfo->max_lod;
    samplerInfo.minLod = pInfo->min_lod;
    cassert(vkCreateSampler(device, &samplerInfo, NULL, &smap.vksampler) == VK_SUCCESS);

    cg_vector_push_back(&g_Samplers, &smap);
    (*sampler) = &((luna_GPU_Sampler *)g_Samplers.m_data)[ g_Samplers.m_size - 1 ];
}

VkImage luna_GPU_TextureGet(const luna_GPU_Texture *tex)
{
    return tex->image;
}

VkImageView luna_GPU_TextureGetView(const luna_GPU_Texture *tex)
{
    return tex->view;
}

VkSampler luna_GPU_SamplerGet(const luna_GPU_Sampler *sampler)
{
    return sampler->vksampler;
}

void luna_GPU_CreateTexture(const luna_GPU_TextureCreateInfo *pInfo, luna_GPU_Texture **tex)
{
    (*tex) = calloc(1, sizeof(luna_GPU_Texture));
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = pInfo->type;
    imageCreateInfo.extent = pInfo->extent;
    imageCreateInfo.mipLevels = pInfo->miplevels;
    imageCreateInfo.arrayLayers = pInfo->arraylayers;
    imageCreateInfo.format = pInfo->format;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = pInfo->usage;
    imageCreateInfo.samples = (VkSampleCountFlagBits)pInfo->samples;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(device, &imageCreateInfo, NULL, &(*tex)->image) != VK_SUCCESS) {
        LOG_ERROR("Failed to create image");
    }
}

// vkimage is fetched from image!
void luna_GPU_CreateTextureView(const luna_GPU_TextureViewCreateInfo *pInfo, luna_GPU_Texture **tex) {
    const VkFormat dst_format = (pInfo->format != VK_FORMAT_UNDEFINED) ? pInfo->format : (*tex)->format;
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = (*tex)->image,
        .viewType = pInfo->view_type,
        .format = dst_format,
        .subresourceRange = pInfo->subresourceRange,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };
    cassert(vkCreateImageView(device, &view_info, NULL, &(*tex)->view) == VK_SUCCESS);
}