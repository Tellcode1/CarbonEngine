#include "../include/GPU/vk.h"

#include "../common/containers/dynarray.h"
#include "../common/printf.h"
#include "../common/string.h"
#include "../include/GPU/buffer.h"
#include "../include/GPU/memory.h"
#include "../include/GPU/pipeline.h"
#include "../include/engine/UI.h"
#include "../include/engine/camera.h"
#include "../include/engine/ctext.h"
#include "../include/engine/engine.h"
#include "../include/engine/fontc.h"
#include "../include/engine/image.h"
#include "../include/engine/shadermanager.h"
#include "../include/engine/shadermanagerdev.h"
#include "../include/engine/sprite_renderer.h"

#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>

#define HAS_FLAG(flag) ((NV_GPU_vk_flag_register & flag) || (flags & flag))
#define STR(s) #s

// vk.h
VkCommandPool        cmd_pool        = VK_NULL_HANDLE;
VkCommandBuffer      buffer          = VK_NULL_HANDLE;

NV_GPU_ResultCheckFn _NVVK_result_fn = _NVVK_default_result_check_fn; // To not cause nullptr dereference.
                                                                      // SetResultCheckFunc also checks for nullptr
                                                                      // and handles it.
u32 NV_GPU_vk_flag_register = 0;
// vk.h

// NVPipelines.h
NV_BakedPipelines g_Pipelines;

VkPipeline        base_pipeline = NULL;
VkPipelineCache   cache         = NULL;

NV_dynarray_t     g_Samplers;
// NVPipelines.h

// renderer.h
VkInstance               instance       = NULL;
VkDevice                 device         = NULL;
VkPhysicalDevice         physDevice     = NULL;
VkSurfaceKHR             surface        = NULL;
SDL_Window*              window         = NULL;
VkDebugUtilsMessengerEXT debugMessenger = NULL;

NVFormat                 SwapChainImageFormat;
u32                      SwapChainColorSpace;
u32                      SwapChainImageCount           = 0;
u32                      GraphicsFamilyIndex           = 0;
u32                      PresentFamilyIndex            = 0;
u32                      ComputeFamilyIndex            = 0;
u32                      TransferQueueIndex            = 0;
u32                      GraphicsAndComputeFamilyIndex = 0;
VkQueue                  GraphicsQueue                 = VK_NULL_HANDLE;
VkQueue                  GraphicsAndComputeQueue       = VK_NULL_HANDLE;
VkQueue                  PresentQueue                  = VK_NULL_HANDLE;
VkQueue                  ComputeQueue                  = VK_NULL_HANDLE;
VkQueue                  TransferQueue                 = VK_NULL_HANDLE;
u32                      Samples                       = VK_SAMPLE_COUNT_1_BIT;

NV_descriptor_pool       g_pool;
NV_camera_t              camera;

typedef struct NV_ctext_module
{
  NV_dynarray_t      fonts;
  NV_dynarray_t      labels;
  NV_descriptor_set* desc_set;
  unsigned           flags;
} NV_ctext_module;

typedef struct NV_QuadDrawCall_t
{
  NV_sprite* spr;
  vec3       siz, pos;
  vec2       tex_multiplier;
  vec4       col;
} NV_QuadDrawCall_t;

typedef struct NV_LineDrawCall_t
{
  vec2 begin, end;
  vec4 col;
} NV_LineDrawCall_t;

typedef enum NV_DrawCallType
{
  NOVA_DRAWCALL_QUAD = 0,
  NOVA_DRAWCALL_LINE = 1,
} NV_DrawCallType;

typedef struct NV_DrawCall_t
{
  NV_DrawCallType type;
  int             layer;
  union NV_DrawCallData
  {
    NV_LineDrawCall_t line;
    NV_QuadDrawCall_t quad;
  } drawcall;
} NV_DrawCall_t;

struct NV_renderer_t
{
  unsigned       flags;
  NV_buffer_mode buffer_mode;

  VkRenderPass   render_pass;
  NV_extent2d    render_extent;

  VkSwapchainKHR swapchain;
  VkCommandPool  commandPool;

  u32            attachment_count;
  u32            renderer_frame;
  u32            imageIndex;

  int            shadow_image_size; // the size of ONE depth texture. Multiply by
                                    // SwapchainImageCount to get total size
  NV_GPU_Memory*   depth_image_memory;

  NV_GPU_Texture*  color_image;
  NV_GPU_Memory*   color_image_memory;

  VkFormat         depth_buffer_format;

  NV_dynarray_t    renderData;
  NV_dynarray_t    drawBuffers;

  NV_dynarray_t    drawcalls;

  NV_ctext_module* ctext;

  // These are used to render all the sprites in the game (quad based sprites
  // that is)
  NV_GPU_Buffer  quad_vb;
  NV_GPU_Memory* quad_memory;

  vec4           clear_color;

  void*          mapped;
};

extern NV_dynarray_t g_Samplers;

struct NV_GPU_Sampler
{
  VkFilter             filter;
  VkSamplerMipmapMode  mipmap_mode;
  VkSamplerAddressMode address_mode;
  float                max_anisotropy;
  float                mip_lod_bias, min_lod, max_lod;
  VkSampler            vksampler;
};

struct NV_GPU_Texture
{
  NV_GPU_Memory*     memory;
  size_t             size, offset;

  VkImageLayout      layout;
  VkImageAspectFlags aspect;
  VkImageType        type;
  VkImageUsageFlags  usage;

  VkImage            image;
  VkImageView        view;
  VkExtent3D         extent;
  int                miplevels, arraylayers;
  NVFormat           format;
  NV_sample_count    samples;
};

// renderer.h

// NVGFX vv

NV_extent2d
NV_get_window_size()
{
  int ww, wh;
  SDL_GetWindowSize(window, &ww, &wh);
  return (NV_extent2d){ ww, wh };
}

typedef struct quad_vertex
{
  vec3 position;
  vec2 tex_coords;
} quad_vertex;

typedef struct line_vertex
{
  vec2 position;
} line_vertex;

static quad_vertex    quad_vertices[4];

static const uint32_t quad_indices[] = { 0, 1, 2, 0, 2, 3 };

// ctext
typedef struct cglyph_vertex_t
{
  vec3 pos;
  vec2 uv;
} cglyph_vertex_t;

struct ctext_drawcall_t
{
  mat4             model;
  size_t           vertex_count;
  size_t           index_count;
  size_t           index_offset;
  cglyph_vertex_t* vertices;
  u32*             indices;
  vec4             color;
  float            scale;
};

struct ctext_label
{
  ctext_hori_align h_align;
  ctext_vert_align v_align;
  float            scale;
  int              index;
  NV_string_t      text;
  cfont_t*         fnt;
  NVObject*        obj;
};

struct push_constants
{
  mat4  model;
  vec4  color;
  vec4  outline_color;
  float scale;
};

typedef enum ctext_err_t
{
  CTEXT_ERR_SUCCESS,
  CTEXT_ERR_INVALID_GLYPH, // May also mean that there just isn't a glyph
  CTEXT_ERR_WRONG_CACHE,   // This cache is not the one you're looking for
  CTEXT_ERR_FILE_ERROR,
} ctext_err_t;
// ctext

int
NV_renderer_get_frame(const NV_renderer_t* rd)
{
  return rd->renderer_frame;
}

VkCommandBuffer
NV_renderer_get_draw_buffer(const NV_renderer_t* rd)
{
  return *(VkCommandBuffer*)NV_dynarray_get(&rd->drawBuffers, rd->renderer_frame);
}

VkRenderPass
NV_renderer_get_render_pass(const NV_renderer_t* rd)
{
  return rd->render_pass;
}

NV_extent2d
NV_renderer_get_render_extent(const NV_renderer_t* rd)
{
  return rd->render_extent;
}

int
NV_renderer_get_max_frames_in_flight(const NV_renderer_t* rd)
{
  return 1 + (int)rd->buffer_mode;
}

#define ABSF(x) ((x >= 0.0f) ? (x) : -(x))

bool
NV_Quad_Visible(const vec3* pos, const vec3* siz)
{
  const float half_width  = siz->x * 0.5f;
  const float half_height = siz->y * 0.5f;
  const float deltax      = pos->x - camera.position.x;
  const float deltay      = pos->y - camera.position.y;
  const float dx          = ABSF(deltax); // delta x & y
  const float dy          = ABSF(deltay); //

  return (dx <= (half_width + camera.ortho_size.x) && dy <= (half_height + camera.ortho_size.y));
}

void
NV_renderer_render_textured_quad(NV_renderer_t* rd, const NV_SpriteRenderer* sprite_renderer, vec3 position, vec3 size, int layer)
{
  NV_DrawCall_t drawcall = { .type = NOVA_DRAWCALL_QUAD,
    .layer                         = layer,
    .drawcall
    = { .quad = { .spr = sprite_renderer->spr, .tex_multiplier = sprite_renderer->tex_coord_multiplier, .pos = position, .siz = size, .col = sprite_renderer->color } } };
  NV_dynarray_push_back(&rd->drawcalls, &drawcall);
}

void
NV_renderer_render_quad(NV_renderer_t* rd, NV_sprite* spr, vec2 tex_coord_multiplier, vec3 position, vec3 size, vec4 color, int layer)
{
  // create a temporary sprite renderer and render an untextured quad with it
  NV_SpriteRenderer sprite_renderer    = NV_SpriteRendererInit();
  sprite_renderer.spr                  = spr;
  sprite_renderer.tex_coord_multiplier = tex_coord_multiplier;
  sprite_renderer.color                = color;
  NV_renderer_render_textured_quad(rd, &sprite_renderer, position, size, layer);
}

void
NV_renderer_render_line(NV_renderer_t* rd, vec2 start, vec2 end, vec4 color, int layer)
{
  NV_DrawCall_t drawcall = { .type = NOVA_DRAWCALL_LINE, .layer = layer, .drawcall = { .line = { .begin = start, .end = end, .col = color } } };
  NV_dynarray_push_back(&rd->drawcalls, &drawcall);
}

// thjs will just sort the array from small layer to big layer :>
int
__drawcall_compar(const void* obj1, const void* obj2)
{
  const NV_DrawCall_t* call1 = obj1;
  const NV_DrawCall_t* call2 = obj2;
  return (call1->layer - call2->layer) + (call1->type - call2->type);
}

void
__NVRenderer_FlushRenders(NV_renderer_t* rd)
{
  const uint32_t        camera_ub_offset = NV_renderer_get_frame(rd) * sizeof(camera_uniform_buffer);
  const VkCommandBuffer cmd              = NV_renderer_get_draw_buffer(rd);
  const VkDescriptorSet camera_set       = camera.sets->set;

  bool                  bound_quad_state = 0;

  NV_dynarray_sort(&rd->drawcalls, __drawcall_compar);

  NV_DrawCallType state = (unsigned)-1;

  for (int i = 0; i < (int)NV_dynarray_size(&rd->drawcalls); i++)
  {
    const NV_DrawCall_t* drawcall = &((NV_DrawCall_t*)NV_dynarray_data(&rd->drawcalls))[i];

    if (drawcall->type == NOVA_DRAWCALL_QUAD)
    {
      // if (!NV_Quad_Visible(&drawcall->drawcall.quad.pos,
      // &drawcall->drawcall.quad.siz)) {
      //     continue;
      // }

      if (state != NOVA_DRAWCALL_QUAD)
      {
        VkDeviceSize offsets = 0;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g_Pipelines.Unlit.pipeline);

        if (!bound_quad_state)
        {
          vkCmdBindVertexBuffers(cmd, 0, 1, &rd->quad_vb.buffer, &offsets);
          vkCmdBindIndexBuffer(cmd, rd->quad_vb.buffer, sizeof(quad_vertices), VK_INDEX_TYPE_UINT32);
        }

        state            = NOVA_DRAWCALL_QUAD;
        bound_quad_state = 1;
      }

      struct push_constants
      {
        mat4 model;
        vec4 color;
        vec2 tex_multiplier; // Multiplied with the tex coords
      } pc;

      mat4 scale        = m4scale(m4init(1.0f), v3muls(drawcall->drawcall.quad.siz, 2.0f));
      mat4 rotate       = m4init(1.0f);
      mat4 translate    = m4translate(m4init(1.0f), drawcall->drawcall.quad.pos);
      pc.model          = m4mul(translate, m4mul(rotate, scale));
      pc.color          = drawcall->drawcall.quad.col;
      pc.tex_multiplier = drawcall->drawcall.quad.tex_multiplier;
      vkCmdPushConstants(cmd, g_Pipelines.Unlit.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(struct push_constants), &pc);

      VkDescriptorSet       sprite_set = NV_sprite_get_descriptor_set(drawcall->drawcall.quad.spr);
      const VkDescriptorSet sets[]     = { camera_set, sprite_set };

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g_Pipelines.Unlit.pipeline_layout, 0, 2, sets, 1, &camera_ub_offset);
      vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
    }
    else if (drawcall->type == NOVA_DRAWCALL_LINE)
    {
      if (state != NOVA_DRAWCALL_LINE)
      {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g_Pipelines.Line.pipeline);

        state = NOVA_DRAWCALL_LINE;
      }

      const VkDescriptorSet sets[] = { camera_set };
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, g_Pipelines.Line.pipeline_layout, 0, 1, sets, 1, &camera_ub_offset);

      struct line_push_constants
      {
        mat4 model;
        vec4 color;
        vec2 line_begin;
        vec2 line_end;
      } pc;
      pc.model      = m4init(1.0f);
      pc.color      = drawcall->drawcall.line.col;
      pc.line_begin = (vec2){ drawcall->drawcall.line.begin.x, drawcall->drawcall.line.begin.y };
      pc.line_end   = (vec2){ drawcall->drawcall.line.end.x, drawcall->drawcall.line.end.y };
      vkCmdPushConstants(cmd, g_Pipelines.Line.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(struct line_push_constants), &pc);

      vkCmdDraw(cmd, 2, 1, 0, 0);
    }
  }

  NV_dynarray_clear(&rd->drawcalls);
}

void
NVRenderer_PrepareQuadRenderer(NV_renderer_t* rd)
{
  NV_memcpy(quad_vertices,
      (const quad_vertex[4]){ (const quad_vertex){ .position = (vec3){ +0.5f, +0.5f, 0.0f }, .tex_coords = (vec2){ 1.0f, 0.0f } },
          (const quad_vertex){ .position = (vec3){ -0.5f, +0.5f, 0.0f }, .tex_coords = (vec2){ 0.0f, 0.0f } },
          (const quad_vertex){ .position = (vec3){ -0.5f, -0.5f, 0.0f }, .tex_coords = (vec2){ 0.0f, 1.0f } },
          (const quad_vertex){ .position = (vec3){ +0.5f, -0.5f, 0.0f }, .tex_coords = (vec2){ 1.0f, 1.0f } } },
      sizeof(quad_vertices));

  NV_GPU_CreateBuffer(
      sizeof(quad_vertices) + sizeof(quad_indices), NOVA_GPU_ALIGNMENT_UNNECESSARY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &rd->quad_vb);
  NV_GPU_AllocateMemory(sizeof(quad_vertices) + sizeof(quad_indices), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &rd->quad_memory);
  NV_GPU_BindBufferToMemory(rd->quad_memory, 0, &rd->quad_vb);

  void* data = NV_malloc(sizeof(quad_vertices) + sizeof(quad_indices));
  NV_memcpy(data, quad_vertices, sizeof(quad_vertices));
  NV_memcpy((char*)data + sizeof(quad_vertices), quad_indices, sizeof(quad_indices));
  NV_GPU_WriteToBuffer(&rd->quad_vb, sizeof(quad_vertices) + sizeof(quad_indices), data, 0);

  NV_free(data);
}

void
NVRenderer_Destroy(NV_renderer_t* rd)
{
  vkDeviceWaitIdle(device);

  // we were only making frames_in_flight fences and it was working for some
  // reason!! that was the reason we were getting errors!
  for (int i = 0; i < (int)SwapChainImageCount; i++)
  {
    NV_renderer_frame_render_info* data = (NV_renderer_frame_render_info*)NV_dynarray_get(&rd->renderData, i);
    NV_GPU_DestroyTexture(data->depth_image);
    vkDestroyImageView(device, NV_GPU_TextureGetView(data->sc_image),
        NOVA_VK_ALLOCATOR); // as the view was silently smushed into the
                            // structure, we just kinda smush it out as well.
    vkDestroyFramebuffer(device, data->color_framebuffer, NOVA_VK_ALLOCATOR);

    vkDestroySemaphore(device, data->image_available_semaphore, NOVA_VK_ALLOCATOR);
    vkDestroySemaphore(device, data->render_finish_semaphore, NOVA_VK_ALLOCATOR);
    vkDestroyFence(device, data->in_flight_fence, NOVA_VK_ALLOCATOR);
  }
  for (int i = 0; i < (int)NV_dynarray_size(&g_Samplers); i++)
  {
    NV_GPU_Sampler* samp = NV_dynarray_get(&g_Samplers, i);
    vkDestroySampler(device, samp->vksampler, NOVA_VK_ALLOCATOR);
  }

  NV_sprite_destroy(NV_sprite_empty);

  NV_camera_destroy(&camera);

  NV_VK_DestroyGlobalPipelines();
  NV_descriptor_poolDestroy(&g_pool);

  NV_dynarray_destroy(&g_Samplers);
  NV_dynarray_destroy(&rd->drawcalls);
  NV_dynarray_destroy(&rd->renderData);

  NV_GPU_FreeMemory(rd->depth_image_memory);

  NV_GPU_DestroyBuffer(&rd->quad_vb);
  NV_GPU_FreeMemory(rd->quad_memory);

  vkFreeCommandBuffers(device, cmd_pool, 1, &buffer);
  vkDestroyCommandPool(device, cmd_pool, NOVA_VK_ALLOCATOR);

  vkFreeCommandBuffers(device, rd->commandPool, NV_dynarray_size(&rd->drawBuffers), (VkCommandBuffer*)NV_dynarray_data(&rd->drawBuffers));
  vkDestroyCommandPool(device, rd->commandPool, NOVA_VK_ALLOCATOR);
  NV_dynarray_destroy(&rd->drawBuffers);

  NVSM_shutdown();

  vkDestroySwapchainKHR(device, rd->swapchain, NOVA_VK_ALLOCATOR);

  if (rd->flags & NOVA_RENDERER_MULTISAMPLING_ENABLE)
  {
    NV_GPU_DestroyTexture(rd->color_image);
    NV_GPU_FreeMemory(rd->color_image_memory);
  }
  vkDestroyRenderPass(device, rd->render_pass, NOVA_VK_ALLOCATOR);
  NV_free(rd);

  vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, NOVA_VK_ALLOCATOR);
  vkDestroySurfaceKHR(instance, surface, NULL);
  SDL_DestroyWindow(window);
  vkDestroyDevice(device, NOVA_VK_ALLOCATOR);
  vkDestroyInstance(instance, NOVA_VK_ALLOCATOR);
}

void
create_optional_images(NV_renderer_t* rd)
{
  NVVK_ResultCheck(vkGetSwapchainImagesKHR(device, rd->swapchain, &SwapChainImageCount, NULL));
  VkImage* swapchainImages = (VkImage*)NV_malloc(SwapChainImageCount * sizeof(VkImage));
  NVVK_ResultCheck(vkGetSwapchainImagesKHR(device, rd->swapchain, &SwapChainImageCount, swapchainImages));

  NV_dynarray_resize(&rd->renderData, SwapChainImageCount);

  if (rd->flags & NOVA_RENDERER_MULTISAMPLING_ENABLE)
  {
    int color_image_size = 0;
    NV_VK_CreateTextureEmpty(rd->render_extent.width, rd->render_extent.height, SwapChainImageFormat, Samples,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &color_image_size, &rd->color_image->image, NULL);
    NV_GPU_AllocateMemory(color_image_size, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, &rd->color_image_memory);
    NV_GPU_BindTextureToMemory(rd->color_image_memory, 0, rd->color_image);
  }

  // attachment vector will be like <color resolve, depth attachment, swapchain
  // image>
  for (int i = 0; i < (int)SwapChainImageCount; i++)
  {
    NV_renderer_frame_render_info* data       = (NV_renderer_frame_render_info*)NV_dynarray_get(&rd->renderData, i);
    (data->sc_image)                          = calloc(1, sizeof(NV_GPU_Texture));
    data->sc_image->image                     = swapchainImages[i];

    const NV_GPU_TextureCreateInfo image_info = {
      .format      = NOVA_FORMAT_D32,
      .samples     = Samples,
      .type        = VK_IMAGE_TYPE_2D,
      .usage       = NOVA_GPU_TEXTURE_USAGE_DEPTH_TEXTURE,
      .extent      = (NV_extent3D){ .width = rd->render_extent.width, .height = rd->render_extent.height, .depth = 1 },
      .arraylayers = 1,
      .miplevels   = 1,
    };
    NV_GPU_CreateTexture(&image_info, &data->depth_image);

    if (i == 0)
    {
      VkMemoryRequirements memReqs = {};
      vkGetImageMemoryRequirements(device, NV_GPU_TextureGet(data->depth_image), &memReqs);

      rd->shadow_image_size = memReqs.size;
      NV_GPU_AllocateMemory(rd->shadow_image_size * SwapChainImageCount, NOVA_GPU_MEMORY_USAGE_GPU_LOCAL, &rd->depth_image_memory);
    }

    NV_GPU_BindTextureToMemory(rd->depth_image_memory, i * rd->shadow_image_size, data->depth_image);
  }

  NV_free(swapchainImages);
}

void
create_framebuffers_and_swapchain_image_views(NV_renderer_t* rd)
{
  NV_dynarray_t /* VkImageView */ attachments = NV_dynarray_init(sizeof(VkImageView), 3);

  for (int i = 0; i < (int)SwapChainImageCount; i++)
  {
    NV_renderer_frame_render_info* data = (NV_renderer_frame_render_info*)NV_dynarray_get(&rd->renderData, i);

    // we don't know anything about the swapchain_image, as it's a swapchain
    // image so we have to manually create the image view;
    VkImageViewCreateInfo imageViewCreateInfo           = {};
    imageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image                           = NV_GPU_TextureGet(data->sc_image);
    imageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format                          = NV_NVFormatToVKFormat(SwapChainImageFormat);
    imageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    imageViewCreateInfo.subresourceRange.levelCount     = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount     = 1;
    VkImageView vioew;
    NVVK_ResultCheck(vkCreateImageView(device, &imageViewCreateInfo, NOVA_VK_ALLOCATOR, &vioew));

    NV_GPU_TextureAttachView(data->sc_image, vioew);

    const VkImageView swapchain_image_view = NV_GPU_TextureGetView(data->sc_image);
    const VkImageView depth_image_view     = NV_GPU_TextureGetView(data->depth_image);

    NV_dynarray_clear(&attachments);
    if (rd->flags & NOVA_RENDERER_MULTISAMPLING_ENABLE)
    {
      NV_dynarray_push_set(&attachments, (VkImageView[]){ NV_GPU_TextureGetView(rd->color_image), depth_image_view, swapchain_image_view }, 3);
    }
    else
    {
      NV_dynarray_push_set(&attachments, (VkImageView[]){ swapchain_image_view, depth_image_view }, 2);
    }

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass              = rd->render_pass;
    framebufferInfo.attachmentCount         = NV_dynarray_size(&attachments);
    framebufferInfo.pAttachments            = (VkImageView*)NV_dynarray_data(&attachments);
    framebufferInfo.width                   = rd->render_extent.width;
    framebufferInfo.height                  = rd->render_extent.height;
    framebufferInfo.layers                  = 1;
    NVVK_ResultCheck(vkCreateFramebuffer(device, &framebufferInfo, NOVA_VK_ALLOCATOR, &data->color_framebuffer));
  }

  NV_dynarray_destroy(&attachments);
}

void
NVRenderer_InitializeGraphicsSingleton()
{
  if (!NV_VK_GetSupportedFormat(physDevice, surface, &SwapChainImageFormat, &SwapChainColorSpace))
  {
    NV_LOG_AND_ABORT("No supported format for display.");
  }
  SwapChainImageCount = NV_VK_GetSurfaceImageCount(physDevice, surface);

  u32 queueCount      = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, NULL);
  NV_dynarray_t /* VkQueueFamilyProperties */ queueFamilies = NV_dynarray_init(sizeof(VkQueueFamilyProperties), queueCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, (VkQueueFamilyProperties*)NV_dynarray_data(&queueFamilies));

  u32  graphicsFamily = 0, graphicsAndComputeFamily = 0, presentFamily = 0, computeFamily = 0, transferFamily = 0;
  bool foundGraphicsFamily = false, foundGraphicsAndComputeFamily = false, foundPresentFamily = false, foundComputeFamily = false, foundTransferFamily = false;

  u32  i = 0;
  for (u32 j = 0; j < queueCount; j++)
  {
    const VkQueueFamilyProperties queueFamily = ((VkQueueFamilyProperties*)NV_dynarray_data(&queueFamilies))[j];
    VkBool32                      presentSupport;
    NVVK_ResultCheck(vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, surface, &presentSupport));

    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
      graphicsAndComputeFamily      = i;
      foundGraphicsAndComputeFamily = true;
    }
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      graphicsFamily      = i;
      foundGraphicsFamily = true;
    }
    if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
      computeFamily      = i;
      foundComputeFamily = true;
    }
    if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
    {
      transferFamily      = i;
      foundTransferFamily = true;
    }
    if (presentSupport)
    {
      presentFamily      = i;
      foundPresentFamily = true;
    }
    if (foundGraphicsFamily && foundGraphicsAndComputeFamily && foundPresentFamily && foundComputeFamily && foundTransferFamily)
      break;

    i++;
  }

  GraphicsFamilyIndex           = graphicsFamily;
  ComputeFamilyIndex            = computeFamily;
  TransferQueueIndex            = transferFamily;
  PresentFamilyIndex            = presentFamily;
  GraphicsAndComputeFamilyIndex = graphicsAndComputeFamily;

  vkGetDeviceQueue(device, GraphicsFamilyIndex, 0, &GraphicsQueue);
  vkGetDeviceQueue(device, ComputeFamilyIndex, 0, &ComputeQueue);
  vkGetDeviceQueue(device, TransferQueueIndex, 0, &TransferQueue);
  vkGetDeviceQueue(device, PresentFamilyIndex, 0, &PresentQueue);
  vkGetDeviceQueue(device, GraphicsAndComputeFamilyIndex, 0, &GraphicsAndComputeQueue);

  NV_dynarray_destroy(&queueFamilies);
}

void
NVRenderer_InitializeRenderingComponents(NV_renderer_t* rd, const NV_renderer_config* conf)
{
  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

  if (conf->vsync_enabled)
  {
    switch (conf->buffer_mode)
    {
      case NOVA_BUFFER_MODE_TRIPLE_BUFFERED:
        present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        break;
      case NOVA_BUFFER_MODE_SINGLE_BUFFERED:
      case NOVA_BUFFER_MODE_DOUBLE_BUFFERED:
      default:
        present_mode = VK_PRESENT_MODE_FIFO_KHR;
        break;
    };
  }
  else
  {
    present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
  }

  NV_GPU_SwapchainCreateInfo scio = {};
  scio.extent.width               = rd->render_extent.width;
  scio.extent.height              = rd->render_extent.height;
  scio.present_mode               = present_mode;
  scio.format                     = SwapChainImageFormat;
  scio.color_space                = SwapChainColorSpace;
  scio.image_count                = SwapChainImageCount;
  NV_GPU_CreateSwapchain(&scio, &rd->swapchain);

  VkSampleCountFlagBits conf_samples;
  if (conf->samples == NOVA_SAMPLE_COUNT_MAX_SUPPORTED)
  {
    conf_samples = MAX_SAMPLES;
  }
  else
  {
    conf_samples = (VkSampleCountFlagBits)conf->samples;
  }
  const VkSampleCountFlagBits samples = conf->multisampling_enable ? conf_samples : VK_SAMPLE_COUNT_1_BIT;
  Samples                             = samples;

  if (conf->multisampling_enable)
  {
    NV_GPU_vk_flag_register |= CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING;
    rd->attachment_count++;
  }
  NV_GPU_vk_flag_register |= CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK;
  NV_GPU_vk_flag_register |= CVK_PIPELINE_FLAGS_FORCE_CULLING;
  rd->attachment_count++;

  VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
  cmdPoolCreateInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolCreateInfo.queueFamilyIndex        = GraphicsFamilyIndex;
  cmdPoolCreateInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  NVVK_ResultCheck(vkCreateCommandPool(device, &cmdPoolCreateInfo, NOVA_VK_ALLOCATOR, &rd->commandPool));

  const int                     frames_in_flight = 1 + (int)conf->buffer_mode;

  NV_renderer_frame_render_info data             = {};
  for (int i = 0; i < frames_in_flight; i++)
  {
    NV_dynarray_push_back(&rd->drawBuffers, &data);
    NV_dynarray_push_back(&rd->renderData, &data);
  }

  VkCommandBufferAllocateInfo cmdAllocInfo = {};
  cmdAllocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdAllocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdAllocInfo.commandBufferCount          = frames_in_flight;
  cmdAllocInfo.commandPool                 = rd->commandPool;
  NVVK_ResultCheck(vkAllocateCommandBuffers(device, &cmdAllocInfo, (VkCommandBuffer*)NV_dynarray_data(&rd->drawBuffers)));

  rd->depth_buffer_format         = NV_NVFormatToVKFormat(NOVA_FORMAT_D32); // replace (probably)

  NV_GPU_RenderPassCreateInfo rpi = {};
  rpi.format                      = SwapChainImageFormat;
  rpi.depthBufferFormat           = NV_VKFormatToNVFormat(rd->depth_buffer_format);
  rpi.subpass                     = 0;
  rpi.samples                     = samples;
  NV_GPU_CreateRenderPass(&rpi, &rd->render_pass, NV_GPU_vk_flag_register);

  create_optional_images(rd);
  create_framebuffers_and_swapchain_image_views(rd);
}

NV_renderer_t*
NVRenderer_Init(const NV_renderer_config* conf)
{
  if (conf->multisampling_enable == 1)
  {
    NV_LOG_ERROR("config samples must not be 1 if multisampling is enabled.");
    NV_assert(conf->samples != NOVA_SAMPLE_COUNT_1_SAMPLES);
  }
  struct NV_renderer_t* rd = (NV_renderer_t*)calloc(1, sizeof(struct NV_renderer_t));

  // how many frames the renderer will render at once
  int frames_in_flight = (1 + (int)conf->buffer_mode);
  if (frames_in_flight <= 0)
  {
    frames_in_flight = 1;
  }

  g_Samplers      = NV_dynarray_init(sizeof(NV_GPU_Sampler), 4);
  rd->drawcalls   = NV_dynarray_init(sizeof(NV_DrawCall_t), 4);
  rd->drawBuffers = NV_dynarray_init(sizeof(VkCommandBuffer), frames_in_flight);
  rd->renderData  = NV_dynarray_init(sizeof(NV_renderer_frame_render_info), frames_in_flight);

  if (conf->multisampling_enable)
  {
    rd->flags |= NOVA_RENDERER_MULTISAMPLING_ENABLE;
  }
  if (conf->window_resizable)
  {
    rd->flags |= NOVA_RENDERER_WINDOW_RESIZABLE;
  }
  if (conf->vsync_enabled)
  {
    rd->flags |= NOVA_RENDERER_VSYNC_ENABLE;
  }
  rd->buffer_mode          = conf->buffer_mode;

  rd->render_extent.width  = conf->initial_window_size.width;
  rd->render_extent.height = conf->initial_window_size.height;

  NVRenderer_InitializeGraphicsSingleton();
  NVRenderer_InitializeRenderingComponents(rd, conf);

  for (int i = 0; i < (int)SwapChainImageCount; i++)
  {
    const VkSemaphoreCreateInfo    semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0 };

    const VkFenceCreateInfo        fenceCreateInfo     = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT };

    NV_renderer_frame_render_info* data                = (NV_renderer_frame_render_info*)NV_dynarray_get(&rd->renderData, i);
    NVVK_ResultCheck(vkCreateSemaphore(device, &semaphoreCreateInfo, NOVA_VK_ALLOCATOR, &data->render_finish_semaphore));
    NVVK_ResultCheck(vkCreateSemaphore(device, &semaphoreCreateInfo, NOVA_VK_ALLOCATOR, &data->image_available_semaphore));
    NVVK_ResultCheck(vkCreateFence(device, &fenceCreateInfo, NOVA_VK_ALLOCATOR, &data->in_flight_fence));
  }

  NV_descriptor_pool_Init(&g_pool);

  const unsigned char empty_data[3] = { 255, 255, 255 }; // fill rgb with 255 so it's white
  NV_sprite_empty                   = NV_sprite_load_from_memory(empty_data, 1, 1, NOVA_FORMAT_RGB8);

  camera                            = NV_camera_init();

  // Initialize subsystems.
  NVSM_compile_updated();
  ctext_init(rd);

  NV_VK_BakeGlobalPipelines(rd);

  NVRenderer_PrepareQuadRenderer(rd);

  return rd;
}

void
_NVVK_renderer_resize(NV_renderer_t* rd)
{
  vkDeviceWaitIdle(device);

  const VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, NULL, 0 };

  const VkFenceCreateInfo     fenceCreateInfo     = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, NULL, VK_FENCE_CREATE_SIGNALED_BIT };

  // adhoc method of resetting them
  for (int i = 0; i < (int)SwapChainImageCount; i++)
  {
    NV_renderer_frame_render_info* data = (NV_renderer_frame_render_info*)NV_dynarray_get(&rd->renderData, i);
    vkDestroySemaphore(device, data->image_available_semaphore, NOVA_VK_ALLOCATOR);
    vkDestroySemaphore(device, data->render_finish_semaphore, NOVA_VK_ALLOCATOR);
    vkDestroyFence(device, data->in_flight_fence, NOVA_VK_ALLOCATOR);

    NV_GPU_DestroyTexture(data->depth_image);

    data->image_available_semaphore = NULL;
    data->render_finish_semaphore   = NULL;
    data->in_flight_fence           = NULL;
  }

  if (rd->color_image)
  {
    NV_GPU_DestroyTexture(rd->color_image);
    NV_GPU_FreeMemory(rd->color_image_memory);
  }
  NV_GPU_FreeMemory(rd->depth_image_memory);

  for (u32 i = 0; i < SwapChainImageCount; i++)
  {
    NV_renderer_frame_render_info* data = (NV_renderer_frame_render_info*)NV_dynarray_get(&rd->renderData, i);
    vkDestroyImageView(device, NV_GPU_TextureGetView(data->sc_image),
        NOVA_VK_ALLOCATOR); // as the view was silently smushed into the
                            // structure, we just kinda smush it out as well.
    vkDestroyFramebuffer(device, data->color_framebuffer, NOVA_VK_ALLOCATOR);
    NV_free(data->sc_image);
  }
  NV_dynarray_clear(&rd->renderData);

  i32 w, h;
  SDL_Vulkan_GetDrawableSize(window, &w, &h);

  VkSurfaceCapabilitiesKHR surface_capabilities;
  NVVK_ResultCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &surface_capabilities));

  const u32 min_width            = surface_capabilities.minImageExtent.width;
  const u32 min_height           = surface_capabilities.minImageExtent.height;

  const u32 max_width            = surface_capabilities.maxImageExtent.width;
  const u32 max_height           = surface_capabilities.maxImageExtent.height;

  w                              = NVM_CLAMP((u32)w, min_width, max_width);
  h                              = NVM_CLAMP((u32)h, min_height, max_height);

  rd->render_extent              = (NV_extent2d){ w, h };

  VkSwapchainKHR   old_swapchain = rd->swapchain;

  VkPresentModeKHR present_mode  = VK_PRESENT_MODE_FIFO_KHR;

  if (rd->flags & NOVA_RENDERER_VSYNC_ENABLE)
  {
    switch (rd->buffer_mode)
    {
      case NOVA_BUFFER_MODE_TRIPLE_BUFFERED:
        present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        break;
      case NOVA_BUFFER_MODE_SINGLE_BUFFERED:
      case NOVA_BUFFER_MODE_DOUBLE_BUFFERED:
      default:
        present_mode = VK_PRESENT_MODE_FIFO_KHR;
        break;
    };
  }
  else
  {
    present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
  }

  NV_GPU_SwapchainCreateInfo scio = {};
  scio.extent.width               = rd->render_extent.width;
  scio.extent.height              = rd->render_extent.height;
  scio.present_mode               = present_mode;
  scio.format                     = SwapChainImageFormat;
  scio.color_space                = SwapChainColorSpace;
  scio.image_count                = SwapChainImageCount;
  scio.old_swapchain              = old_swapchain;
  NV_GPU_CreateSwapchain(&scio, &rd->swapchain);
  vkDestroySwapchainKHR(device, old_swapchain, NOVA_VK_ALLOCATOR);

  NV_dynarray_resize(&rd->renderData, SwapChainImageCount);
  create_optional_images(rd);
  create_framebuffers_and_swapchain_image_views(rd);

  for (int i = 0; i < (int)SwapChainImageCount; i++)
  {
    NV_renderer_frame_render_info* data = (NV_renderer_frame_render_info*)NV_dynarray_get(&rd->renderData, i);
    NVVK_ResultCheck(vkCreateSemaphore(device, &semaphoreCreateInfo, NOVA_VK_ALLOCATOR, &data->render_finish_semaphore));
    NVVK_ResultCheck(vkCreateSemaphore(device, &semaphoreCreateInfo, NOVA_VK_ALLOCATOR, &data->image_available_semaphore));
    NVVK_ResultCheck(vkCreateFence(device, &fenceCreateInfo, NOVA_VK_ALLOCATOR, &data->in_flight_fence));
  }

  _NV_reset_frame_buffer_resized();
}

bool
NV_renderer_begin(NV_renderer_t* rd)
{
  NV_renderer_frame_render_info* data = (NV_renderer_frame_render_info*)NV_dynarray_get(&rd->renderData, rd->renderer_frame);

  vkWaitForFences(device, 1, &data->in_flight_fence, VK_TRUE, UINT64_MAX);

  const VkResult        imageAcquireResult = vkAcquireNextImageKHR(device, rd->swapchain, UINT64_MAX, data->image_available_semaphore, VK_NULL_HANDLE, &rd->imageIndex);

  const VkCommandBuffer drawBuffer         = *(VkCommandBuffer*)NV_dynarray_get(&rd->drawBuffers, rd->renderer_frame);

  if (imageAcquireResult == VK_ERROR_OUT_OF_DATE_KHR || imageAcquireResult == VK_SUBOPTIMAL_KHR || NV_get_frame_buffer_resized())
  {
    _NVVK_renderer_resize(rd);
    return false;
  }
  else if (imageAcquireResult != VK_SUCCESS && imageAcquireResult != VK_SUBOPTIMAL_KHR)
  {
    NV_LOG_ERROR("Failed to acquire image from swapchain");
    return false;
  }

  vkResetFences(device, 1, &data->in_flight_fence);

  // * Maybe the user should have control of the clear color but I don't really
  // care lmao

  VkFramebuffer fb = (*(NV_renderer_frame_render_info*)(NV_dynarray_get(&rd->renderData, rd->imageIndex))).color_framebuffer;

  // Why was this static?
  VkRenderPassBeginInfo renderPassInfo = {
    .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass      = rd->render_pass,
    .framebuffer     = fb,
    .renderArea      = (VkRect2D){ .extent = (VkExtent2D){ rd->render_extent.width, rd->render_extent.height }, .offset = {} },
    .clearValueCount = 2,
    .pClearValues    = (VkClearValue[2]){ { .color = (VkClearColorValue){ { rd->clear_color.x, rd->clear_color.y, rd->clear_color.z, rd->clear_color.w } } },
           { .depthStencil = (VkClearDepthStencilValue){ 1.0f, 0 } } },
  };

  const VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, NULL, 0, NULL };

  vkBeginCommandBuffer(drawBuffer, &beginInfo);
  vkCmdBeginRenderPass(drawBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {};
  viewport.x          = 0.0f;
  viewport.y          = 0.0f;
  viewport.width      = rd->render_extent.width;
  viewport.height     = rd->render_extent.height;
  viewport.minDepth   = 0.0f;
  viewport.maxDepth   = 1.0f;
  vkCmdSetViewport(drawBuffer, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset   = (VkOffset2D){ 0, 0 };
  scissor.extent   = (VkExtent2D){ (unsigned)rd->render_extent.width, (unsigned)rd->render_extent.height };
  vkCmdSetScissor(drawBuffer, 0, 1, &scissor);

  return true;
}

#define V2_TO_V3(v) ((vec3){ (v).x, (v).y, 0.0f })

void
NV_renderer_end(NV_renderer_t* rd)
{
  const VkCommandBuffer drawBuffer = *(VkCommandBuffer*)NV_dynarray_get(&rd->drawBuffers, rd->renderer_frame);

  for (int i = 0; i < (int)rd->ctext->labels.m_size; i++)
  {
    ctext_label*           label  = NV_dynarray_get(&rd->ctext->labels, i);

    NV_SpriteRenderer*     spr_rd = NVObject_GetSpriteRenderer(label->obj);

    ctext_text_render_info r_info = ctext_init_text_render_info();
    r_info.position               = V2_TO_V3(NVObject_GetPosition(label->obj));
    r_info.color                  = spr_rd->color;
    r_info.horizontal             = label->h_align;
    r_info.vertical               = label->v_align;
    ctext_render(label->fnt, &r_info, "%s", NV_string_data(&label->text));
  }
  ctext_flush_renders(rd);

  // NVUI internally checks whether it has been initialized or not
  NVUI_Render(rd);
  __NVRenderer_FlushRenders(rd);

  vkCmdEndRenderPass(drawBuffer);
  vkEndCommandBuffer(drawBuffer);

  VkSubmitInfo submitInfo                                 = {};
  submitInfo.sType                                        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  const NV_renderer_frame_render_info* data               = (NV_renderer_frame_render_info*)NV_dynarray_get(&rd->renderData, rd->renderer_frame);
  const VkSemaphore                    waitSemaphores[]   = { data->image_available_semaphore };
  const VkSemaphore                    signalSemaphores[] = { data->render_finish_semaphore };

  VkPipelineStageFlags                 waitStages[]       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  submitInfo.pWaitDstStageMask                            = waitStages;

  submitInfo.waitSemaphoreCount                           = 1;
  submitInfo.pWaitSemaphores                              = waitSemaphores;

  const VkCommandBuffer buffers[]                         = { drawBuffer };
  submitInfo.commandBufferCount                           = NV_arrlen(buffers);
  submitInfo.pCommandBuffers                              = buffers;

  submitInfo.signalSemaphoreCount                         = 1;
  submitInfo.pSignalSemaphores                            = signalSemaphores;

  vkQueueSubmit(PresentQueue, 1, &submitInfo, data->in_flight_fence);

  VkPresentInfoKHR presentInfo   = {};
  presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores; // This is signalSemaphores so that this starts as
                                                     // soon as the signaled semaphores are signaled.
  presentInfo.pImageIndices  = &rd->imageIndex;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains    = &rd->swapchain;

  VkResult result            = VK_SUCCESS;
  result                     = vkQueuePresentKHR(PresentQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || NV_get_frame_buffer_resized())
  {
    _NVVK_renderer_resize(rd);
  }

  rd->renderer_frame = (rd->renderer_frame + 1) % (1 + (int)rd->buffer_mode);
}

void
NV_renderer_set_clear_color(struct NV_renderer_t* rd, vec4 col)
{
  rd->clear_color = col;
}
// NVGFX ^^

// engine
u32                           MAX_SAMPLES;
unsigned char                 SUPPORTS_MULTISAMPLING;
float                         MAX_ANISOTROPY;

static SDL_UNUSED const char* ValidationLayers[] = {
  "VK_LAYER_KHRONOS_validation",
};

static SDL_UNUSED const char* RequiredInstanceExtensions[] = {
  VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
  VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
};

static SDL_UNUSED const char* WantedInstanceExtensions[] = {
  // VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
};

static SDL_UNUSED const char* WantedDeviceExtensions[] = {
  // VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
};

static SDL_UNUSED const char* RequiredDeviceExtensions[] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

// we'll just request them as needed

static const VkPhysicalDeviceFeatures WantedFeatures = {
  .samplerAnisotropy = VK_TRUE,
};

void
__VK_DEBUG_LOG(const char* fmt, ...)
{
  const char* preceder  = "vkdebug: ";
  const char* succeeder = "\n";
  va_list     args;
  va_start(args, fmt);
  _NV_LOG(args, STR(NVVKDebugMessenger), succeeder, preceder, fmt, 1);
  va_end(args);
}

static SDL_UNUSED VKAPI_ATTR VkBool32 VKAPI_CALL
NVVKDebugMessenger(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
  (void)messageSeverity;
  (void)messageType;
  (void)pUserData;

  __VK_DEBUG_LOG("%s", pCallbackData->pMessage);

  return VK_FALSE;
}

NV_dynarray_t
setify(u32 i1, u32 i2, u32 i3, u32 i4)
{
  NV_dynarray_t ret     = NV_dynarray_init(sizeof(u32), 4);
  u32           nums[4] = { i1, i2, i3, i4 };
  for (int j = 0; j < NV_arrlen(nums); j++)
  {
    const u32 e          = nums[j];
    bool      already_in = false;
    for (int i = 0; i < (int)NV_dynarray_size(&ret); i++)
    {
      if (e == *(u32*)NV_dynarray_get(&ret, i))
        already_in = true;
    }
    if (!already_in)
    {
      NV_dynarray_push_back(&ret, &e);
    }
  }
  return ret;
}

VkInstance
CreateInstance(const char* title)
{
  if (volkInitialize() != VK_SUCCESS)
  {
    NV_LOG_AND_ABORT("Volk could not initialize. You probably don't have the "
                     "vulkan loader installed. "
                     "I can't do anything about that.");
  }

  VkApplicationInfo appInfo  = {};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.applicationVersion = VK_API_VERSION_1_0;
  appInfo.apiVersion         = VK_API_VERSION_1_0;
  appInfo.pApplicationName   = title;
  appInfo.pEngineName        = "Carbon";
  appInfo.engineVersion      = 0;

  uint32_t SDLExtensionCount = 0;
  NV_assert(SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, NULL) == SDL_TRUE);
  const char** SDLExtensions = NV_malloc(sizeof(const char*) * SDLExtensionCount);
  NV_assert(SDL_Vulkan_GetInstanceExtensions(window, &SDLExtensionCount, SDLExtensions) == SDL_TRUE);

  u32 extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
  NV_dynarray_t /* VkExtensionProperties */ extensions = NV_dynarray_init(sizeof(VkExtensionProperties), extensionCount);
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, (VkExtensionProperties*)NV_dynarray_data(&extensions));

  int          enabled_exts_count = 0;
  const char** enabled_exts
      = NV_malloc(sizeof(const char*) * (NV_arrlen(RequiredInstanceExtensions) + SDLExtensionCount + extensionCount + NV_arrlen(WantedInstanceExtensions)));

  for (int i = 0; i < NV_arrlen(RequiredInstanceExtensions); i++)
  {
    const char* ext                  = RequiredInstanceExtensions[i];
    enabled_exts[enabled_exts_count] = ext;
    enabled_exts_count++;
  }

  for (int i = 0; i < (int)SDLExtensionCount; i++)
  {
    const char* ext                  = SDLExtensions[i];
    enabled_exts[enabled_exts_count] = ext;
    enabled_exts_count++;
  }

  for (u32 i = 0; i < extensionCount; i++)
  {
    const char* name = ((VkExtensionProperties*)NV_dynarray_data(&extensions))[i].extensionName;
    for (int j = 0; j < NV_arrlen(WantedInstanceExtensions); j++)
    {
      const char* want = WantedInstanceExtensions[j];
      if (NV_strcmp(name, want) == 0)
      {
        enabled_exts[enabled_exts_count] = name;
        enabled_exts_count++;
        break;
      }
    }
  }

  VkInstanceCreateInfo instanceCreateinfo    = {};
  instanceCreateinfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateinfo.pApplicationInfo        = &appInfo;
  instanceCreateinfo.enabledExtensionCount   = enabled_exts_count;
  instanceCreateinfo.ppEnabledExtensionNames = enabled_exts;

#ifdef DEBUG

  bool     validationLayersAvailable = false;
  uint32_t layerCount                = 0;

  vkEnumerateInstanceLayerProperties(&layerCount, NULL);
  NV_dynarray_t /* VkLayerProperties */ layerProperties = NV_dynarray_init(sizeof(VkLayerProperties), layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, (VkLayerProperties*)NV_dynarray_data(&layerProperties));

  if (NV_arrlen(ValidationLayers) != 0)
  {
    for (int j = 0; j < NV_arrlen(ValidationLayers); j++)
    {
      for (uint32_t i = 0; i < layerCount; i++)
      {
        if (NV_strcmp(ValidationLayers[j], ((VkLayerProperties*)NV_dynarray_data(&layerProperties))[i].layerName) == 0)
        {
          validationLayersAvailable = true;
        }
      }
    }

    if (!validationLayersAvailable)
    {
      NV_LOG_ERROR("Failed to initialize validation layers\nRequested layers:");
      for (int i = 0; i < NV_arrlen(ValidationLayers); i++)
      {
        NV_LOG_ERROR("\t%s", ValidationLayers[i]);
      }

      NV_LOG_ERROR("Available Layers::");
      for (uint32_t i = 0; i < layerCount; i++)
        NV_LOG_ERROR("\t%s", ((VkLayerProperties*)NV_dynarray_data(&layerProperties))[i].layerName);

      NV_LOG_ERROR("But instance asked for (i.e. are not available):");

      NV_dynarray_t /* const char* */ missingLayers = NV_dynarray_init(sizeof(const char*), 16);

      for (int i = 0; i < NV_arrlen(ValidationLayers); i++)
      {
        const char* layer          = ValidationLayers[i];
        bool        layerAvailable = false;
        for (uint32_t i = 0; i < layerCount; i++)
        {
          if (NV_strcmp(layer, ((VkLayerProperties*)NV_dynarray_data(&layerProperties))[i].layerName) == 0)
          {
            layerAvailable = true;
            break;
          }
        }
        if (!layerAvailable)
          NV_dynarray_push_back(&missingLayers, &layer);
      }
      for (int i = 0; i < (int)NV_dynarray_size(&missingLayers); i++)
        NV_LOG_ERROR("\t%s", (const char*)NV_dynarray_get(&missingLayers, i));

      NV_dynarray_destroy(&missingLayers);

      abort();
    }

    instanceCreateinfo.enabledLayerCount   = NV_arrlen(ValidationLayers);
    instanceCreateinfo.ppEnabledLayerNames = ValidationLayers;
  }

#else

  instanceCreateinfo.enabledLayerCount   = 0;
  instanceCreateinfo.ppEnabledLayerNames = NULL;

#endif

  VkInstance instance;
  NVVK_ResultCheck(vkCreateInstance(&instanceCreateinfo, NOVA_VK_ALLOCATOR, &instance));

#ifdef DEBUG
  if (validationLayersAvailable)
  {
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity
        = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback                                    = NVVKDebugMessenger;

    PFN_vkCreateDebugUtilsMessengerEXT _CreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (_CreateDebugUtilsMessenger)
    {
      VkResult r;
      if ((r = _CreateDebugUtilsMessenger(instance, &createInfo, NOVA_VK_ALLOCATOR, &debugMessenger)) != VK_SUCCESS)
        NV_LOG_ERROR("Vulkan debug messenger could not start. err %i", r);
      else
        NV_LOG_DEBUG("Vulkan debug messenger has been set to %s()", STR(NVVKDebugMessenger));
    }
    else
      NV_LOG_ERROR("vkCreateDebugUtilsMessengerEXT proc address not found");
  }

  NV_dynarray_destroy(&layerProperties);
#endif

  NV_free(SDLExtensions);
  NV_dynarray_destroy(&extensions);
  NV_free(enabled_exts);

  volkLoadInstance(instance);
  return instance;
}

void
_NVVK_PrintDeviceInfo(VkPhysicalDevice device)
{
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(device, &properties);

  const char* deviceTypeStr;
  switch (properties.deviceType)
  {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      deviceTypeStr = "Discrete";
      break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      deviceTypeStr = "Integrated";
      break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
      deviceTypeStr = "Software/CPU";
      break;
    default:
      deviceTypeStr = "Unknown";
      break;
  }

  // I think it looks cleaner this way
  NV_LOG_INFO("(%s) %s VK API Version %d.%d", deviceTypeStr, properties.deviceName, properties.apiVersion >> 22, (properties.apiVersion >> 12) & 0x3FF);
}

VkPhysicalDevice
_NVVK_ChoosePhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
{
  uint32_t physDeviceCount = 0;

  VkResult r               = VK_SUCCESS;

  if ((r = vkEnumeratePhysicalDevices(instance, &physDeviceCount, NULL)) != VK_SUCCESS)
  {
    NV_LOG_AND_ABORT("Error fetching physical devices. VkResult=%i", r);
  }

  if (physDeviceCount == 0)
  {
    NV_LOG_AND_ABORT("Huuuhhh??? No physical devices found? Are you running this "
                     "on a banana???");
  }

  NV_dynarray_t /* VkPhysicalDevice */ physicalDevices = NV_dynarray_init(sizeof(VkPhysicalDevice), physDeviceCount);
  vkEnumeratePhysicalDevices(instance, &physDeviceCount, (VkPhysicalDevice*)NV_dynarray_data(&physicalDevices));

  for (u32 i = 0; i < physDeviceCount; i++)
  {
    const VkPhysicalDevice device      = ((VkPhysicalDevice*)NV_dynarray_data(&physicalDevices))[i];

    uint32_t               formatCount = 0;
    NVVK_ResultCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL));

    uint32_t presentModeCount = 0;
    NVVK_ResultCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL));

    bool     extensionsAvailable = true;

    uint32_t extensionCount      = 0;
    NVVK_ResultCheck(vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL));
    NV_dynarray_t /* VkExtensionProperties */ availableExtensions = NV_dynarray_init(sizeof(VkExtensionProperties), extensionCount);
    NVVK_ResultCheck(vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, (VkExtensionProperties*)NV_dynarray_data(&availableExtensions)));

    for (int i = 0; i < NV_arrlen(RequiredDeviceExtensions); i++)
    {
      const char* extension = RequiredDeviceExtensions[i];
      bool        validated = false;
      for (u32 j = 0; j < extensionCount; j++)
      {
        if (NV_strcmp(extension, ((VkExtensionProperties*)NV_dynarray_data(&availableExtensions))[j].extensionName) == 0)
          validated = true;
      }
      if (!validated)
      {
        NV_LOG_ERROR("Failed to validate extension with name: %s", extension);
        extensionsAvailable = false;
      }
    }

    NV_dynarray_destroy(&availableExtensions);

    if (extensionsAvailable && formatCount > 0 && presentModeCount > 0)
    {
      _NVVK_PrintDeviceInfo(device);
      return device;
    }
  }

  VkPhysicalDevice           fallback = ((VkPhysicalDevice*)NV_dynarray_data(&physicalDevices))[0];

  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(fallback, &properties);

  NV_LOG_ERROR("No device found. Falling back to device \"%s\".", properties.deviceName);

  _NVVK_PrintDeviceInfo(fallback);

  NV_dynarray_destroy(&physicalDevices);

  return fallback;
}

VkDevice
CreateDevice()
{
  u32 queueCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, NULL);
  NV_dynarray_t /* VkQueueFamilyProperties */ queueFamilies = NV_dynarray_init(sizeof(VkQueueFamilyProperties), queueCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physDevice, &queueCount, (VkQueueFamilyProperties*)NV_dynarray_data(&queueFamilies));

  // Clang loves complaining about these.
  u32 graphicsFamily = 0, presentFamily = 0, computeFamily = 0, transferFamily = 0;
  (void)graphicsFamily, (void)presentFamily, (void)computeFamily, (void)transferFamily;

  bool foundGraphicsFamily = false, foundPresentFamily = false, foundComputeFamily = false, foundTransferFamily = false;

  u32  i = 0;
  for (int j = 0; j < (int)NV_dynarray_size(&queueFamilies); j++)
  {
    const VkQueueFamilyProperties queueFamily    = ((VkQueueFamilyProperties*)NV_dynarray_data(&queueFamilies))[j];
    VkBool32                      presentSupport = false;
    NVVK_ResultCheck(vkGetPhysicalDeviceSurfaceSupportKHR(physDevice, i, surface, &presentSupport));

    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      graphicsFamily      = i;
      foundGraphicsFamily = true;
    }
    if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
      computeFamily      = i;
      foundComputeFamily = true;
    }
    if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
    {
      transferFamily      = i;
      foundTransferFamily = true;
    }
    if (presentSupport)
    {
      presentFamily      = i;
      foundPresentFamily = true;
    }
    if (foundGraphicsFamily && foundComputeFamily && foundPresentFamily && foundTransferFamily)
      break;

    i++;
  }

  NV_dynarray_t /* u32 */                     uniqueQueueFamilies = setify(graphicsFamily, presentFamily, computeFamily, transferFamily);
  NV_dynarray_t /* VkDeviceQueueCreateInfo */ queueCreateInfos    = NV_dynarray_init(sizeof(VkDeviceQueueCreateInfo), NV_dynarray_size(&uniqueQueueFamilies));

  const float                                 queuePriority       = 1.0f;

  for (int i = 0; i < (int)NV_dynarray_size(&uniqueQueueFamilies); i++)
  {
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex        = ((u32*)NV_dynarray_data(&uniqueQueueFamilies))[i];
    queueInfo.queueCount              = 1;
    queueInfo.pQueuePriorities        = &queuePriority;
    NV_dynarray_push_back(&queueCreateInfos, &queueInfo);
  }

  NV_dynarray_t /* const char* */ enabledExtensions = NV_dynarray_init(sizeof(const char*), NV_arrlen(WantedDeviceExtensions) + NV_arrlen(RequiredDeviceExtensions));

  u32                             extensionCount    = 0;
  vkEnumerateDeviceExtensionProperties(physDevice, NULL, &extensionCount, NULL);
  NV_dynarray_t /* VkExtensionProperties */ extensions = NV_dynarray_init(sizeof(VkExtensionProperties), extensionCount);
  vkEnumerateDeviceExtensionProperties(physDevice, NULL, &extensionCount, (VkExtensionProperties*)NV_dynarray_data(&extensions));

  for (int i = 0; i < NV_arrlen(WantedDeviceExtensions); i++)
  {
    const char* wanted = WantedDeviceExtensions[i];
    for (u32 i = 0; i < extensionCount; i++)
    {
      VkExtensionProperties ext = ((VkExtensionProperties*)NV_dynarray_data(&extensions))[i];
      if (NV_strcmp(wanted, ext.extensionName) == 0)
      {
        NV_dynarray_push_back(&enabledExtensions, &ext.extensionName);
      }
    }
  }

  for (int i = 0; i < NV_arrlen(RequiredDeviceExtensions); i++)
  {
    const char* required  = RequiredDeviceExtensions[i];
    bool        validated = false;
    for (u32 i = 0; i < extensionCount; i++)
    {
      const char* extName = ((VkExtensionProperties*)NV_dynarray_data(&extensions))[i].extensionName;
      if (NV_strcmp(required, extName) == 0)
      {
        NV_dynarray_push_back(&enabledExtensions, &extName);
        validated = true;
      }
    }

    if (!validated)
      NV_LOG_ERROR("Failed to validate required extension with name %s", required);
  }

  VkDeviceCreateInfo deviceCreateInfo      = {};
  deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount    = NV_dynarray_size(&queueCreateInfos);
  deviceCreateInfo.pQueueCreateInfos       = (const VkDeviceQueueCreateInfo*)NV_dynarray_data(&queueCreateInfos);
  deviceCreateInfo.enabledExtensionCount   = NV_dynarray_size(&enabledExtensions);
  deviceCreateInfo.ppEnabledExtensionNames = (const char* const*)NV_dynarray_data(&enabledExtensions);
  deviceCreateInfo.pEnabledFeatures        = &WantedFeatures;

  NVVK_ResultCheck(vkCreateDevice(physDevice, &deviceCreateInfo, NOVA_VK_ALLOCATOR, &device));

  NV_dynarray_destroy(&extensions);
  NV_dynarray_destroy(&enabledExtensions);
  NV_dynarray_destroy(&queueFamilies);
  NV_dynarray_destroy(&uniqueQueueFamilies);
  NV_dynarray_destroy(&queueCreateInfos);

  return device;
}

void
_NVVK_initialize_context(const char* title, u32 windowWidth, u32 windowHeight)
{
  instance = CreateInstance(title);
  NV_assert(instance != NULL);

  if (SDL_Vulkan_CreateSurface(window, instance, &surface) != SDL_TRUE)
  {
    NV_LOG_AND_ABORT("Surface creation failed.\nSDL reports: %s", SDL_GetError());
  }

  physDevice = _NVVK_ChoosePhysicalDevice(instance, surface);

  device     = CreateDevice();
  NV_assert(device != NULL);

  volkLoadDevice(device);

  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(physDevice, &props);

  MAX_ANISOTROPY                   = props.limits.maxSamplerAnisotropy;
  SUPPORTS_MULTISAMPLING           = true;

  const VkSampleCountFlags samples = props.limits.framebufferColorSampleCounts;
  if (samples & VK_SAMPLE_COUNT_64_BIT)
  {
    MAX_SAMPLES = VK_SAMPLE_COUNT_64_BIT;
  }
  else if (samples & VK_SAMPLE_COUNT_32_BIT)
  {
    MAX_SAMPLES = VK_SAMPLE_COUNT_32_BIT;
  }
  else if (samples & VK_SAMPLE_COUNT_16_BIT)
  {
    MAX_SAMPLES = VK_SAMPLE_COUNT_16_BIT;
  }
  else if (samples & VK_SAMPLE_COUNT_8_BIT)
  {
    MAX_SAMPLES = VK_SAMPLE_COUNT_8_BIT;
  }
  else if (samples & VK_SAMPLE_COUNT_4_BIT)
  {
    MAX_SAMPLES = VK_SAMPLE_COUNT_4_BIT;
  }
  else if (samples & VK_SAMPLE_COUNT_2_BIT)
  {
    MAX_SAMPLES = VK_SAMPLE_COUNT_2_BIT;
  }
  else
  {
    MAX_SAMPLES            = VK_SAMPLE_COUNT_1_BIT;
    SUPPORTS_MULTISAMPLING = false;
  }
}
// engine

// ctext vv

/* I have no idea what any of this is */

void
ctext_load_font(NV_renderer_t* rd, const char* font_path, int scale, cfont_t** dst)
{
  if (!rd || !dst)
  {
    NV_LOG_ERROR("pInfo or dst is NULL!");
  }

  if (scale <= 0.0f)
  {
    NV_LOG_ERROR("attempting to load a font with 0 fontscale.");
  }

  {
    cfont_t stack;
    NV_dynarray_push_back(&rd->ctext->fonts, &stack);
  }
  *dst = &((cfont_t*)(rd->ctext->fonts.m_data))[rd->ctext->fonts.m_size - 1];
  NV_memset(*dst, 0, sizeof(cfont_t));

  cfont_t* dstref   = *dst;

  dstref->rd        = rd;

  dstref->glyph_map = NV_hashmap_init(16, sizeof(char), sizeof(ctext_glyph), NULL, NULL);
  dstref->drawcalls = NV_dynarray_init(sizeof(ctext_drawcall_t), 4);

  fontc_file f_file;
  fontc_read_font(font_path, &f_file);

  NV_atlas_t atlas;

  dstref->line_height = f_file.header.line_height;
  dstref->space_width = f_file.header.space_width;
  atlas.width         = f_file.header.bmpwidth;
  atlas.height        = f_file.header.bmpheight;
  atlas.data          = f_file.bitmap;

  for (int i = 0; i < f_file.header.numglyphs; i++)
  {
    ctext_glyph glyph  = { .x0 = f_file.glyphs[i].x0,
       .x1                     = f_file.glyphs[i].x1,
       .y0                     = f_file.glyphs[i].y0,
       .y1                     = f_file.glyphs[i].y1,
       .l                      = f_file.glyphs[i].l,
       .r                      = f_file.glyphs[i].r,
       .b                      = f_file.glyphs[i].b,
       .t                      = f_file.glyphs[i].t,
       .advance                = f_file.glyphs[i].advance };
    char        glyphi = f_file.glyphs[i].codepoint;
    NV_hashmap_insert(dstref->glyph_map, &glyphi, &glyph);
  }

  NV_GPU_TextureCreateInfo image_info = { .format = NOVA_FORMAT_R8,
    .samples                                      = NOVA_SAMPLE_COUNT_1_SAMPLES,
    .type                                         = VK_IMAGE_TYPE_2D,
    .usage                                        = NOVA_GPU_TEXTURE_USAGE_SAMPLED_TEXTURE,
    .extent                                       = (NV_extent3D){ .width = atlas.width, .height = atlas.height, .depth = 1 },
    .arraylayers                                  = 1,
    .miplevels                                    = 1 };
  NV_GPU_CreateTexture(&image_info, &dstref->texture);

  VkMemoryRequirements imageMemoryRequirements;
  vkGetImageMemoryRequirements(device, NV_GPU_TextureGet(dstref->texture), &imageMemoryRequirements);

  NV_GPU_AllocateMemory(imageMemoryRequirements.size, NOVA_GPU_MEMORY_USAGE_GPU_LOCAL, &dstref->texture_mem);
  NV_GPU_BindTextureToMemory(dstref->texture_mem, 0, dstref->texture);

  const int atlas_w = atlas.width, atlas_h = atlas.height;

  NVImage   atlas_img = (NVImage){ .w = atlas_w, .h = atlas_h, .fmt = NOVA_FORMAT_R8, .data = atlas.data };
  NV_GPU_WriteToTexture(dstref->texture, &atlas_img);

  const NV_GPU_SamplerCreateInfo sampler_info = { .filter = VK_FILTER_LINEAR, .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR, .address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT };
  NV_GPU_CreateSampler(&sampler_info, &dstref->sampler);

  const VkDescriptorImageInfo ctext_bitmap_image_info = {
    .sampler     = NV_GPU_SamplerGet(dstref->sampler),
    .imageView   = NV_GPU_TextureGetView(dstref->texture),
    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };

  VkWriteDescriptorSet writeSet = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet                                = rd->ctext->desc_set->set,
    .dstBinding                            = 0,
    .descriptorCount                       = 1,
    .descriptorType                        = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo                            = &ctext_bitmap_image_info };
  for (int i = 0; i < CTEXT_MAX_FONT_COUNT; i++)
  {
    writeSet.dstArrayElement = i;
    NV_descriptor_setSubmitWrite(rd->ctext->desc_set, &writeSet);
  }
}

void
ctext_destroy_font(cfont_t* fnt)
{
  NV_GPU_DestroyTexture(fnt->texture);
  NV_GPU_FreeMemory(fnt->texture_mem);

  if (fnt->buffer.buffer != NULL)
  {
    NV_GPU_DestroyBuffer(&fnt->buffer);
    NV_GPU_FreeMemory(fnt->buffer_mem);
  }
  NV_dynarray_destroy(&fnt->drawcalls);
  NV_hashmap_destroy(fnt->glyph_map);
}

bool
ctext__font_resize_buffer(cfont_t* fnt, int new_buffer_size)
{
  int new_allocation_size;

  if (fnt->allocated_size < new_buffer_size)
  {
    new_allocation_size = NVM_MAX(fnt->allocated_size * 2, new_buffer_size);
  }
  else if (new_buffer_size < (fnt->allocated_size / 3))
  {
    new_allocation_size = NVM_MAX(fnt->allocated_size / 3, new_buffer_size);
  }
  else
    return false;

  vkDeviceWaitIdle(device);

  if (fnt->buffer.buffer)
  {
    NV_GPU_DestroyBuffer(&fnt->buffer);
    NV_GPU_FreeMemory(fnt->buffer_mem);
  }

  NV_GPU_AllocateMemory(new_allocation_size, NOVA_GPU_MEMORY_USAGE_GPU_LOCAL | NOVA_GPU_MEMORY_USAGE_CPU_WRITEABLE | NOVA_GPU_MEMORY_USAGE_CPU_VISIBLE, &fnt->buffer_mem);

  NV_GPU_CreateBuffer(new_allocation_size, NOVA_GPU_ALIGNMENT_UNNECESSARY, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &fnt->buffer);
  NV_GPU_BindBufferToMemory(fnt->buffer_mem, 0, &fnt->buffer);

  fnt->allocated_size = new_allocation_size;
  fnt->to_render      = 0;

  return true;
}

void
ctext__render_drawcalls(NV_renderer_t* rd, cfont_t* fnt)
{
  const VkCommandBuffer  cmd              = NV_renderer_get_draw_buffer(rd);
  const VkDeviceSize     offsets[]        = { 0 };

  struct push_constants  pc               = {};

  const VkPipeline       pipeline         = g_Pipelines.Ctext.pipeline;
  const VkPipelineLayout pipeline_layout  = g_Pipelines.Ctext.pipeline_layout;

  const VkDescriptorSet  sets[]           = { camera.sets->set, rd->ctext->desc_set->set };
  const uint32_t         camera_ub_offset = NV_renderer_get_frame(rd) * sizeof(camera_uniform_buffer);

  // Viewport && scissor are set by renderer so no need to set them here
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 2, sets, 1, &camera_ub_offset);
  vkCmdBindVertexBuffers(cmd, 0, 1, &fnt->buffer.buffer, offsets);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  vkCmdBindIndexBuffer(cmd, fnt->buffer.buffer, fnt->index_buffer_offset, VK_INDEX_TYPE_UINT32);

  int offset = 0;
  for (int i = 0; i < (int)NV_dynarray_size(&fnt->drawcalls); i++)
  {
    ctext_drawcall_t* drawcall = (ctext_drawcall_t*)NV_dynarray_get(&fnt->drawcalls, i);

    pc.model                   = drawcall->model;
    pc.scale                   = drawcall->scale;
    pc.color                   = drawcall->color;
    vkCmdPushConstants(cmd, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(struct push_constants), &pc);

    vkCmdDrawIndexed(cmd, drawcall->index_count, 1, 0, offset, 0);
    offset += drawcall->vertex_count;
  }
}

static NV_dynarray_t
split_string_by_lines(const NV_string_t* str)
{
  NV_dynarray_t result;
  NV_string_t   substr;
  const char*   str_data = NV_string_data(str);
  const int     str_len  = NV_string_length(str);
  int           i_start  = 0;

  result                 = NV_dynarray_init(sizeof(NV_string_t), 16);

  for (int i = 0; i < str_len; i++)
  {
    if (str_data[i] == '\n')
    {
      if (i > i_start)
      {
        substr = NV_string_substring(str, i_start, i - i_start);
        // substr should now handle errors internally
        NV_dynarray_push_back(&result, &substr);
      }
      i_start = i + 1;
    }
  }

  if (i_start < str_len)
  {
    substr = NV_string_substring(str, i_start, str_len - i_start);
    NV_dynarray_push_back(&result, &substr);
  }

  return result;
}

// Get the unscaled size of the string
// Warning: slow
static void
ctext_get_text_size(const cfont_t* fnt, const NV_string_t* str, float* w, float* h)
{
  const int len   = NV_string_length(str);
  float     width = 0.0f, height = fnt->line_height, prev_width = 0.0f;

  bool      is_new_line = true;

  for (int i = 0; i < len; i++)
  {
    const char c = str->m_data[i];

    switch (c)
    {
      case ' ':
        width += fnt->space_width;
        is_new_line = 0;
        continue;
      case '\t':
        width += fnt->space_width * 4.0f;
        is_new_line = 0;
        continue;
      case '\n':
      case '\r':
        if (!is_new_line)
        {
          height += fnt->line_height;
          prev_width = NVM_MAX(width, prev_width);
        }
        width       = 0.0f;
        is_new_line = 1;
        continue;
      default:
      {
        const ctext_glyph* glyph = (ctext_glyph*)NV_hashmap_find(fnt->glyph_map, &c);
        if (!glyph)
        {
          continue;
        }
        width += glyph->advance;
        is_new_line = 0;
        continue;
      }
    }
  }

  if (!is_new_line)
  {
    prev_width = NVM_MAX(width, prev_width);
  }

  if (w)
  {
    *w = prev_width;
  }

  if (h)
  {
    *h = -height;
  }
}

static int
render_line(const cfont_t* fnt, const NV_string_t* str, const ctext_drawcall_t* drawcall, float scale, float zpos, const int glyph_iter, float x, const float y)
{
  const int len  = NV_string_length(str);

  int       iter = 0;
  for (int i = 0; i < len; i++)
  {
    const char c = str->m_data[i];
    switch (c)
    {
      case ' ':
        x += fnt->space_width * scale;
        continue;
      case '\t':
        x += fnt->space_width * 4.0f * scale;
        continue;
      default:
      {
        const ctext_glyph* glyph = (const ctext_glyph*)NV_hashmap_find(fnt->glyph_map, &c);
        if (!glyph)
        {
          NV_LOG_INFO("no glyph when rendering char %i", c);
          continue;
        }
        const float      glyph_x0     = (glyph->x0 * scale) + x;
        const float      glyph_x1     = (glyph->x1 * scale) + x;
        const float      glyph_y0     = (glyph->y0 * scale) + y;
        const float      glyph_y1     = (glyph->y1 * scale) + y;

        const int        index_offset = (fnt->chars_drawn + iter) * 4;
        cglyph_vertex_t* v_out        = drawcall->vertices + (glyph_iter + iter) * 4;
        // clang-format off
        v_out[0] = (cglyph_vertex_t){ (vec3){glyph_x0, glyph_y0, zpos}, (vec2){glyph->l, glyph->b} };
        v_out[1] = (cglyph_vertex_t){ (vec3){glyph_x1, glyph_y0, zpos}, (vec2){glyph->r, glyph->b} };
        v_out[2] = (cglyph_vertex_t){ (vec3){glyph_x1, glyph_y1, zpos}, (vec2){glyph->r, glyph->t} };
        v_out[3] = (cglyph_vertex_t){ (vec3){glyph_x0, glyph_y1, zpos}, (vec2){glyph->l, glyph->t} };
        // clang-format on

        u32* i_out = drawcall->indices + (glyph_iter + iter) * 6;
        i_out[0]   = index_offset;
        i_out[1]   = index_offset + 1;
        i_out[2]   = index_offset + 2;
        i_out[3]   = index_offset + 2;
        i_out[4]   = index_offset + 3;
        i_out[5]   = index_offset;

        x += glyph->advance * scale;
        iter++;
        continue;
      }
    }
  }

  return iter;
}

static inline int
get_effective_length(const char* buf, int buflen)
{
  int len = 0;
  for (int i = 0; i < buflen; i++)
  {
    const char c = buf[i];
    if (c != ' ' && c != '\t' && c != '\n')
    {
      len++;
    }
  }
  return len;
}

int
gen_vertices(cfont_t* fnt, ctext_drawcall_t* drawcall, const ctext_text_render_info* pInfo, const NV_string_t* str)
{
  if (NV_string_length(str) == 0)
  {
    return 1;
  }

  NV_dynarray_t lines;
  float         text_w, text_h;
  float         scale;
  float         ypos, xpos;
  int           actual_chars_drawn;
  const int     old_chars_drawn = fnt->chars_drawn;

  lines                         = split_string_by_lines(str);

  scale                         = pInfo->scale;
  ctext_get_text_size(fnt, str, &text_w, NULL);
  text_h = -fnt->line_height * ((int)lines.m_size - 1);

  if (pInfo->scale_for_fit)
  {
    float scale_x = (pInfo->bbox.x) / text_w;
    float scale_y = (pInfo->bbox.y) / text_h;
    // multiply with normal scale to get new scale
    scale *= fminf(scale_x, scale_y);
  }
  drawcall->scale = scale;

  text_w *= scale;
  text_h *= scale;

  ypos = pInfo->position.y;
  switch (pInfo->vertical)
  {
    case CTEXT_VERT_ALIGN_CENTER:
      ypos += (text_h + fnt->line_height * scale) / 2.0f;
      break;
    case CTEXT_VERT_ALIGN_BOTTOM:
      ypos += text_h;
      break;
    case CTEXT_VERT_ALIGN_TOP:
      ypos += fnt->line_height * scale;
      break;
    default:
      __builtin_unreachable();
      NV_LOG_ERROR("Invalid vertical alignment. Specified (int)%u. (Implement?)", pInfo->vertical);
      break;
  }
  for (int i = 0; i < lines.m_size; i++)
  {
    // render_line returns the number of chars DRAWN. not the number of
    // characters in the string.
    const NV_string_t* line = &((NV_string_t*)NV_dynarray_data(&lines))[i];

    ctext_get_text_size(fnt, line, &text_w, &text_h);
    text_w *= scale;

    xpos = pInfo->position.x;
    switch (pInfo->horizontal)
    {
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
        NV_LOG_ERROR("Invalid horizontal alignment. Specified (int)%u. (Implement?)", pInfo->horizontal);
        break;
    }
    actual_chars_drawn = fnt->chars_drawn - old_chars_drawn;
    fnt->chars_drawn += render_line(fnt, line, drawcall, scale, pInfo->position.z,
        // This gives us the number of characters this function call
        // has drawn. only this call.
        NVM_MAX(actual_chars_drawn, 0), xpos, (ypos + ((float)i * fnt->line_height * scale)));
  }

  for (int i = 0; i < lines.m_size; i++)
  {
    NV_string_t* str = &((NV_string_t*)NV_dynarray_data(&lines))[i];
    NV_string_destroy(str);
  }
  NV_dynarray_destroy(&lines);

  return 0;
}

void
ctext_render(cfont_t* fnt, const ctext_text_render_info* pInfo, const char* fmt, ...)
{
  NV_string_t str;
  size_t      num, effective_length;
  char*       buffer;

  va_list     arg;
  va_start(arg, fmt);

  num    = NV_vsnprintf(NULL, NOVA_PRINTF_BUFSIZ, fmt, arg);
  buffer = NV_malloc(num + 1);
  va_start(arg, fmt);

  NV_vsnprintf(buffer, num + 1, fmt, arg);

  va_end(arg);

  effective_length = get_effective_length(buffer, num);
  if (effective_length == 0)
  {
    return;
  }

  str = NV_string_init_str(buffer);
  NV_free(buffer);

  const size_t     vertex_count    = effective_length * 4;
  const size_t     index_count     = effective_length * 6;
  const size_t     allocation_size = (vertex_count * sizeof(cglyph_vertex_t)) + (index_count * sizeof(u32));

  void*            allocation      = NV_malloc(allocation_size);

  ctext_drawcall_t drawcall        = {};
  drawcall.vertices                = (cglyph_vertex_t*)allocation;
  drawcall.index_offset            = (vertex_count * sizeof(cglyph_vertex_t));
  drawcall.indices                 = (u32*)((uchar*)allocation + drawcall.index_offset);

  drawcall.color                   = pInfo->color;
  drawcall.scale                   = pInfo->scale;
  drawcall.model                   = pInfo->model;

  drawcall.vertex_count            = effective_length * 4;
  drawcall.index_count             = effective_length * 6;

  // can we not have a bounds check before making the vertices?
  // probably not..
  if (gen_vertices(fnt, &drawcall, pInfo, &str) != 0)
  {
    NV_free(allocation);
    NV_string_destroy(&str);
    return;
  }

  NV_dynarray_push_back(&fnt->drawcalls, &drawcall);

  NV_string_destroy(&str);

  fnt->rendered_this_frame = 1;
}

void
__ctext_flush_font(NV_renderer_t* rd, cfont_t* fnt)
{
  if (!fnt->rendered_this_frame)
  {
    return;
  }
  else
  {
    fnt->rendered_this_frame = 0;
  }
  u32 total_vertex_byte_size = 0;
  u32 total_index_count      = 0;

  for (int i = 0; i < (int)NV_dynarray_size(&fnt->drawcalls); i++)
  {
    const ctext_drawcall_t* drawcall = (ctext_drawcall_t*)NV_dynarray_get(&fnt->drawcalls, i);
    total_vertex_byte_size += drawcall->vertex_count * sizeof(cglyph_vertex_t);
    total_index_count += drawcall->index_count;
  }

  if (total_vertex_byte_size == 0 || total_index_count == 0)
  {
    return;
  }

  const u32  total_index_byte_size = total_index_count * sizeof(u32);
  const u32  total_buffer_size     = total_index_byte_size + total_vertex_byte_size;

  const bool fnt_buffer_resized    = ctext__font_resize_buffer(fnt, total_buffer_size);

  if (fnt->to_render && !fnt_buffer_resized)
  {
    ctext__render_drawcalls(rd, fnt);
  }

  uint8_t* mapped;
  NV_GPU_MapMemory(fnt->buffer_mem, total_buffer_size, 0, (void**)&mapped);

  // this may be dumb but I am too

  u32 vertex_copy_iterator = 0;
  u32 index_copy_iterator  = 0;
  for (int i = 0; i < (int)NV_dynarray_size(&fnt->drawcalls); i++)
  {
    const ctext_drawcall_t* drawcall = (ctext_drawcall_t*)NV_dynarray_get(&fnt->drawcalls, i);
    NV_memcpy(mapped + vertex_copy_iterator, drawcall->vertices, drawcall->vertex_count * sizeof(cglyph_vertex_t));
    NV_memcpy(mapped + total_vertex_byte_size + index_copy_iterator, drawcall->indices, drawcall->index_count * sizeof(u32));
    vertex_copy_iterator += drawcall->vertex_count * sizeof(cglyph_vertex_t);
    index_copy_iterator += drawcall->index_count * sizeof(u32);
  }
  NV_GPU_UnmapMemory(fnt->buffer_mem);

  fnt->index_buffer_offset = total_vertex_byte_size;
  fnt->index_count         = total_index_count;
  fnt->to_render           = true;

  fnt->chars_drawn         = 0;

  for (int i = 0; i < (int)NV_dynarray_size(&fnt->drawcalls); i++)
  {
    ctext_drawcall_t* drawcall = (ctext_drawcall_t*)NV_dynarray_get(&fnt->drawcalls, i);
    NV_free(drawcall->vertices);
  }
  NV_dynarray_clear(&fnt->drawcalls);
}

void
ctext_flush_renders(NV_renderer_t* rd)
{
  for (int i = 0; i < (int)rd->ctext->fonts.m_size; i++)
  {
    cfont_t* fnt = NV_dynarray_get(&rd->ctext->fonts, i);
    __ctext_flush_font(rd, fnt);
  }
}

ctext_label*
ctext_create_label(NV_scene_t* scene, cfont_t* fnt)
{
  ctext_label label = {
    .h_align = CTEXT_HORI_ALIGN_LEFT,
    .v_align = CTEXT_VERT_ALIGN_TOP,
    .index   = fnt->rd->ctext->labels.m_size,
    .text    = NV_string_init(0),
    .fnt     = fnt,
    .obj     = NVObject_Create(scene, "Text Label", 0, 0, 0, (vec2){}, (vec2){ 1.0f, 1.0f }, NOVA_OBJECT_NO_COLLISION),
  };
  NV_dynarray_push_back(&fnt->rd->ctext->labels, &label);
  return &(((ctext_label*)fnt->rd->ctext->labels.m_data)[fnt->rd->ctext->labels.m_size - 1]);
}

void
ctext_destroy_label(ctext_label* label)
{
  NV_string_destroy(&label->text);
  NV_dynarray_remove(&label->fnt->rd->ctext->labels, label->index);
}

NVObject*
ctext_label_get_object(const ctext_label* label)
{
  return label->obj;
}
void
ctext_label_set_text(ctext_label* label, const char* text)
{
  NV_string_set(&label->text, text);
}
void
ctext_label_set_horizontal_align(ctext_label* label, ctext_hori_align h_align)
{
  label->h_align = h_align;
}
void
ctext_label_set_vertical_align(ctext_label* label, ctext_vert_align v_align)
{
  label->v_align = v_align;
}

void
ctext_label_set_text_scale(ctext_label* label, float scale)
{
  label->scale = scale;
}

void
ctext_init(struct NV_renderer_t* rd)
{
  rd->ctext                                     = calloc(1, sizeof(NV_ctext_module));
  NV_ctext_module* ctext                        = rd->ctext;

  ctext->fonts                                  = NV_dynarray_init(sizeof(cfont_t), 4);
  ctext->labels                                 = NV_dynarray_init(sizeof(ctext_label), 4);

  const VkDescriptorSetLayoutBinding bindings[] = {
    // binding; descriptorType; descriptorCount; stageFlags;
    // pImmutableSamplers;
    { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, CTEXT_MAX_FONT_COUNT, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
  };

  NV_AllocateDescriptorSet(&g_pool, bindings, NV_arrlen(bindings), &ctext->desc_set);

  const VkDescriptorImageInfo empty_img_info
      = { .sampler = NV_sprite_get_sampler(NV_sprite_empty), .imageView = NV_sprite_get_vk_image_view(NV_sprite_empty), .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

  VkWriteDescriptorSet write_set = {
    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet          = ctext->desc_set->set,
    .dstBinding      = 0,
    .descriptorCount = 1,
    .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo      = &empty_img_info,
  };
  for (int i = 0; i < CTEXT_MAX_FONT_COUNT; i++)
  {
    write_set.dstArrayElement = i;
    NV_descriptor_setSubmitWrite(ctext->desc_set, &write_set);
  }
}

void
ctext_shutdown(struct NV_renderer_t* rd)
{
  NV_dynarray_destroy(&rd->ctext->fonts);
  NV_dynarray_destroy(&rd->ctext->labels);
}

float
ctext_get_scale_for_fit(const cfont_t* fnt, const NV_string_t* str, vec2 bbox)
{
  float width, height;
  ctext_get_text_size(fnt, str, &width, &height);

  float scale_x = bbox.x / width;
  float scale_y = bbox.y / height;
  return fminf(scale_x, scale_y);
}

// ctext ^^

// NVDescriptors vv
void
NV_descriptor_setSubmitWrite(NV_descriptor_set* set, const VkWriteDescriptorSet* write)
{
  set->writes = realloc(set->writes, (set->nwrites + 1) * sizeof(VkWriteDescriptorSet));
  NV_assert(set->writes != NULL);
  NV_memcpy(&set->writes[set->nwrites], write, sizeof(VkWriteDescriptorSet));
  set->nwrites++;
  vkUpdateDescriptorSets(device, 1, write, 0, 0);
}

void
NV_descriptor_setDestroy(NV_descriptor_set* set)
{
  vkDestroyDescriptorSetLayout(device, set->layout, NOVA_VK_ALLOCATOR);
  NV_free(set->writes);
  NV_free(set);
}

void
NV_descriptor_poolDestroy(NV_descriptor_pool* pool)
{
  for (int i = 0; i < pool->nsets; i++)
  {
    NV_descriptor_setDestroy(pool->sets[i]);
  }
  NV_free(pool->sets);
  vkDestroyDescriptorPool(device, pool->pool, NOVA_VK_ALLOCATOR);
}

void
_NV_descriptor_pool_Allocate(NV_descriptor_pool* pool)
{
  VkDescriptorPoolSize allocations[11]     = {};
  int                  descriptors_written = 0;

  for (int i = 0; i < 11; i++)
  {
    if (pool->descriptors[i].capacity == 0)
    {
      continue;
    }
    allocations[descriptors_written] = (VkDescriptorPoolSize){ pool->descriptors[i].type, pool->descriptors[i].capacity };
    descriptors_written++;
  }

  if (descriptors_written == 0)
  {
    return;
  }

  bool need_more_max_sets = (pool->nsets + 1) > pool->max_child_sets;
  if (need_more_max_sets)
  {
    pool->max_child_sets = NVM_MAX(pool->max_child_sets * 2, 1);
  }

  VkDescriptorPoolCreateInfo poolInfo
      = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = pool->max_child_sets, .poolSizeCount = descriptors_written, .pPoolSizes = allocations };

  VkDescriptorPool new_pool;
  NVVK_ResultCheck(vkCreateDescriptorPool(device, &poolInfo, NOVA_VK_ALLOCATOR, &new_pool));

  VkDescriptorSet* new_sets = NV_malloc(sizeof(VkDescriptorSet) * pool->nsets);
  NV_assert(new_sets != NULL);

  if (pool->nsets > 0)
  {
    VkDescriptorSetLayout* layouts = NV_malloc(sizeof(VkDescriptorSetLayout) * pool->nsets);
    for (int i = 0; i < pool->nsets; i++)
    {
      layouts[i] = pool->sets[i]->layout;
    }

    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool              = new_pool;
    setAllocInfo.descriptorSetCount          = pool->nsets;
    setAllocInfo.pSetLayouts                 = layouts;
    NVVK_ResultCheck(vkAllocateDescriptorSets(device, &setAllocInfo, new_sets));

    NV_free(layouts);
  }

  int ncopies = 0;
  for (int i = 0; i < pool->nsets; i++)
  {
    ncopies += pool->sets[i]->nwrites;
  }
  VkCopyDescriptorSet* copies = NV_malloc(sizeof(VkCopyDescriptorSet) * ncopies);

  ncopies                     = 0;
  for (int i = 0; i < pool->nsets; i++)
  {
    NV_descriptor_set* old_set = pool->sets[i];

    for (int writei = 0; writei < old_set->nwrites; writei++)
    {
      VkWriteDescriptorSet* write = &old_set->writes[writei];
      copies[ncopies]             = (VkCopyDescriptorSet){
                    .sType           = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
                    .srcSet          = old_set->set,
                    .srcBinding      = write->dstBinding,
                    .srcArrayElement = write->dstArrayElement,
                    .dstSet          = new_sets[i],
                    .dstBinding      = write->dstBinding,
                    .dstArrayElement = write->dstArrayElement,
                    .descriptorCount = write->descriptorCount,
      };
      ncopies++;
    }
    old_set->set = new_sets[i];
  }
  vkUpdateDescriptorSets(device, 0, NULL, ncopies, copies);

  if (pool->pool)
  {
    vkDestroyDescriptorPool(device, pool->pool, NOVA_VK_ALLOCATOR);
  }
  pool->pool = new_pool;

  NV_free(new_sets);
  NV_free(copies);
}

void
NV_descriptor_pool_Init(NV_descriptor_pool* dst)
{
  *dst                                = (NV_descriptor_pool){};
  NV_descriptor_poolSize pool_sizes[] = {
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
  NV_memcpy(dst->descriptors, pool_sizes, sizeof(pool_sizes));
  dst->sets           = NV_malloc(sizeof(NV_descriptor_set*));
  dst->max_child_sets = 1;
  dst->nsets          = 0;
  _NV_descriptor_pool_Allocate(dst);
}

void
NV_AllocateDescriptorSet(NV_descriptor_pool* pool, const VkDescriptorSetLayoutBinding* bindings, int nbindings, NV_descriptor_set** dst)
{
  bool need_realloc = 0;
  for (int i = 0; i < 11; i++)
  {
    for (int j = 0; j < nbindings; j++)
    {
      NV_descriptor_poolSize* descriptor = &pool->descriptors[i];
      if (descriptor->type == bindings[j].descriptorType)
      {
        descriptor->capacity = NVM_MAX(descriptor->capacity * 2, (int)bindings[j].descriptorCount + descriptor->capacity);
        need_realloc         = 1;
      }
    }
  }

  if (need_realloc || ((pool->nsets + 1) > pool->max_child_sets))
  {
    if ((pool->nsets + 1) > pool->max_child_sets)
    {
      pool->max_child_sets = NVM_MAX(pool->max_child_sets * 2, 1);
      pool->sets           = realloc(pool->sets, pool->max_child_sets * sizeof(NV_descriptor_set));
    }

    _NV_descriptor_pool_Allocate(pool);
  }

  NV_descriptor_set* set  = calloc(1, sizeof(NV_descriptor_set));

  pool->sets[pool->nsets] = set;
  pool->nsets++;

  (*dst)      = set;

  set->pool   = pool;
  set->writes = NV_malloc(sizeof(VkWriteDescriptorSet));
  NV_assert(set->writes != NULL);

  VkDescriptorSetLayoutCreateInfo layoutinfo = {};
  layoutinfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutinfo.pBindings                       = bindings;
  layoutinfo.bindingCount                    = nbindings;
  NVVK_ResultCheck(vkCreateDescriptorSetLayout(device, &layoutinfo, NOVA_VK_ALLOCATOR, &set->layout));

  VkDescriptorSetAllocateInfo setAllocInfo = {};
  setAllocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  setAllocInfo.descriptorPool              = pool->pool;
  setAllocInfo.descriptorSetCount          = 1;
  setAllocInfo.pSetLayouts                 = &set->layout;
  NVVK_ResultCheck(vkAllocateDescriptorSets(device, &setAllocInfo, &set->set));
}
// NVDescriptors ^^

void
__BakeUnlitPipeline(NV_renderer_t* rd)
{
  VkDescriptorSetLayoutBinding bindings[] = {
    // binding; descriptorType; descriptorCount; stageFlags;
    // pImmutableSamplers;
    { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL },
  };

  VkDescriptorSetLayoutCreateInfo layoutinfo = {};
  layoutinfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutinfo.pBindings                       = bindings;
  layoutinfo.bindingCount                    = 1;
  NVVK_ResultCheck(vkCreateDescriptorSetLayout(device, &layoutinfo, NOVA_VK_ALLOCATOR, &g_Pipelines.Unlit.descriptor_layout));

  NVSM_shader_t *vertex, *fragment;
  NVSM_load_shader("Unlit/vert", &vertex);
  NVSM_load_shader("Unlit/frag", &fragment);

  NV_assert(vertex != NULL && fragment != NULL);

  const NVSM_shader_t*                    shaders[]               = { vertex, fragment };
  const VkDescriptorSetLayout             layouts[]               = { camera.sets->layout, g_Pipelines.Unlit.descriptor_layout };

  const NV_extent2d                       RenderExtent            = NV_renderer_get_render_extent(rd);

  const VkVertexInputAttributeDescription attributeDescriptions[] = {
    // location; binding; format; offset;
    { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },         // pos
    { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(vec3) }, // texcoord
  };

  const VkVertexInputBindingDescription bindingDescriptions[] = { // binding; stride; inputRate
    { 0, sizeof(vec3) + sizeof(vec2), VK_VERTEX_INPUT_RATE_VERTEX }
  };

  const VkPushConstantRange pushConstants[] = { // stageFlags, offset, size
    { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(mat4) + sizeof(vec4) + sizeof(vec2) }
  };

  NV_GPU_PipelineCreateInfo pc = NV_GPU_InitPipelineCreateInfo();
  pc.format                    = SwapChainImageFormat;
  pc.subpass                   = 0;
  pc.render_pass               = NV_renderer_get_render_pass(rd);

  pc.nAttributeDescriptions    = NV_arrlen(attributeDescriptions);
  pc.pAttributeDescriptions    = attributeDescriptions;

  pc.nPushConstants            = NV_arrlen(pushConstants);
  pc.pPushConstants            = pushConstants;

  pc.nBindingDescriptions      = NV_arrlen(bindingDescriptions);
  pc.pBindingDescriptions      = bindingDescriptions;

  pc.nShaders                  = NV_arrlen(shaders);
  pc.pShaders                  = shaders;

  pc.nDescriptorLayouts        = NV_arrlen(layouts);
  pc.pDescriptorLayouts        = layouts;

  pc.extent.width              = RenderExtent.width;
  pc.extent.height             = RenderExtent.height;
  pc.samples                   = Samples;
  NV_GPU_CreatePipelineLayout(&pc, &g_Pipelines.Unlit.pipeline_layout);
  pc.pipeline_layout = g_Pipelines.Unlit.pipeline_layout;
  NV_GPU_CreateGraphicsPipeline(&pc, &g_Pipelines.Unlit.pipeline, 0);
}

void
__BakeCtextPipeline(NV_renderer_t* rd)
{
  const VkVertexInputAttributeDescription attributeDescriptions[] = {
    // location; binding; format; offset;
    { 0, 0, NV_NVFormatToVKFormat(NOVA_FORMAT_RGB32), 0 },           // pos
    { 1, 0, NV_NVFormatToVKFormat(NOVA_FORMAT_RG32), sizeof(vec3) }, // uv
  };

  const VkVertexInputBindingDescription bindingDescriptions[] = { // binding; stride; inputRate
    { 0, sizeof(vec3) + sizeof(vec2), VK_VERTEX_INPUT_RATE_VERTEX }
  };

  const VkPushConstantRange pushConstants[] = {
    // stageFlags, offset, size
    { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(struct push_constants) },
  };

  NVSM_shader_t *vertex, *fragment;
  NV_assert(NVSM_load_shader("ctext/vert", &vertex) != -1);
  NV_assert(NVSM_load_shader("ctext/frag", &fragment) != -1);

  NVSM_shader_t*                  shaders[] = { vertex, fragment };
  VkDescriptorSetLayout           layouts[] = { camera.sets->layout, rd->ctext->desc_set->layout };

  const NV_GPU_PipelineBlendState blend     = NV_GPU_InitPipelineBlendState(CVK_BLEND_PRESET_ALPHA);

  NV_GPU_PipelineCreateInfo       pc        = NV_GPU_InitPipelineCreateInfo();
  pc.format                                 = SwapChainImageFormat;
  pc.subpass                                = 0;
  pc.render_pass                            = NV_renderer_get_render_pass(rd);

  pc.nAttributeDescriptions                 = NV_arrlen(attributeDescriptions);
  pc.pAttributeDescriptions                 = attributeDescriptions;

  pc.nPushConstants                         = NV_arrlen(pushConstants);
  pc.pPushConstants                         = pushConstants;

  pc.nBindingDescriptions                   = NV_arrlen(bindingDescriptions);
  pc.pBindingDescriptions                   = bindingDescriptions;

  pc.nShaders                               = NV_arrlen(shaders);
  pc.pShaders                               = (const struct NVSM_shader_t* const*)shaders;

  pc.nDescriptorLayouts                     = NV_arrlen(layouts);
  pc.pDescriptorLayouts                     = layouts;

  const NV_extent2d RenderExtent            = NV_renderer_get_render_extent(rd);
  pc.extent.width                           = RenderExtent.width;
  pc.extent.height                          = RenderExtent.height;
  pc.blend_state                            = &blend;
  pc.samples                                = Samples;

  NV_GPU_CreatePipelineLayout(&pc, &g_Pipelines.Ctext.pipeline_layout);
  pc.pipeline_layout = g_Pipelines.Ctext.pipeline_layout;
  NV_GPU_CreateGraphicsPipeline(&pc, &g_Pipelines.Ctext.pipeline, 0);
}

void
__BakeDebugLinePipeline(NV_renderer_t* rd)
{
  struct line_push_constants
  {
    mat4 model;
    vec4 color;
    vec2 line_begin;
    vec2 line_end;
  };

  const VkPushConstantRange pushConstants[] = {
    // stageFlags, offset, size
    { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(struct line_push_constants) },
  };

  NVSM_shader_t *vertex, *fragment;
  NV_assert(NVSM_load_shader("Debug/Line/vert", &vertex) != -1);
  NV_assert(NVSM_load_shader("Debug/Line/frag", &fragment) != -1);

  NVSM_shader_t*                  shaders[] = { vertex, fragment };
  VkDescriptorSetLayout           layouts[] = { camera.sets->layout };

  const NV_GPU_PipelineBlendState blend     = NV_GPU_InitPipelineBlendState(CVK_BLEND_PRESET_ALPHA);

  NV_GPU_PipelineCreateInfo       pc        = NV_GPU_InitPipelineCreateInfo();
  pc.format                                 = SwapChainImageFormat;

  pc.topology                               = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  pc.render_pass                            = NV_renderer_get_render_pass(rd);

  pc.nAttributeDescriptions                 = 0;
  pc.pAttributeDescriptions                 = NULL;

  pc.nPushConstants                         = NV_arrlen(pushConstants);
  pc.pPushConstants                         = pushConstants;

  pc.nBindingDescriptions                   = 0;
  pc.pBindingDescriptions                   = NULL;

  pc.nShaders                               = NV_arrlen(shaders);
  pc.pShaders                               = (const struct NVSM_shader_t* const*)shaders;

  pc.nDescriptorLayouts                     = NV_arrlen(layouts);
  pc.pDescriptorLayouts                     = layouts;

  const NV_extent2d RenderExtent            = NV_renderer_get_render_extent(rd);
  pc.extent.width                           = RenderExtent.width;
  pc.extent.height                          = RenderExtent.height;
  pc.blend_state                            = &blend;
  pc.samples                                = Samples;

  NV_GPU_CreatePipelineLayout(&pc, &g_Pipelines.Line.pipeline_layout);
  pc.pipeline_layout = g_Pipelines.Line.pipeline_layout;
  NV_GPU_CreateGraphicsPipeline(&pc, &g_Pipelines.Line.pipeline, 0);
}

void
NV_VK_BakeGlobalPipelines(NV_renderer_t* rd)
{
  __BakeUnlitPipeline(rd);
  __BakeDebugLinePipeline(rd);
  __BakeCtextPipeline(rd);
}

void
NV_VK_DestroyPipeline(NV_VK_Pipeline* pipeline)
{
  vkDestroyPipeline(device, pipeline->pipeline, NOVA_VK_ALLOCATOR);
  vkDestroyPipelineLayout(device, pipeline->pipeline_layout, NOVA_VK_ALLOCATOR);
  vkDestroyDescriptorSetLayout(device, pipeline->descriptor_layout, NOVA_VK_ALLOCATOR);
}

void
NV_VK_DestroyGlobalPipelines()
{
  NV_VK_Pipeline pipelines[] = {
    g_Pipelines.Unlit,
    g_Pipelines.Ctext,
    g_Pipelines.Line,
  };
  for (int i = 0; i < NV_arrlen(pipelines); i++)
  {
    NV_VK_DestroyPipeline(&pipelines[i]);
  }
}

void
NV_GPU_CreateGraphicsPipeline(const NV_GPU_PipelineCreateInfo* pCreateInfo, VkPipeline* dstPipeline, u32 flags)
{
  CVK_REQUIRED_PTR(device);
  CVK_REQUIRED_PTR(pCreateInfo);
  CVK_REQUIRED_PTR(dstPipeline);
  CVK_REQUIRED_PTR(pCreateInfo->render_pass);
  CVK_NOT_EQUAL_TO(pCreateInfo->nShaders, 0);
  CVK_NOT_EQUAL_TO(pCreateInfo->format, NOVA_FORMAT_UNDEFINED);
  CVK_NOT_EQUAL_TO(pCreateInfo->extent.width, 0);
  CVK_NOT_EQUAL_TO(pCreateInfo->extent.height, 0);

  if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING))
  {
    // Vulkan requires samples to not be 1.
    CVK_NOT_EQUAL_TO(pCreateInfo->samples, VK_SAMPLE_COUNT_1_BIT);
  }

  VkPipelineVertexInputStateCreateInfo vertexInputState = {
    .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,

    .vertexBindingDescriptionCount   = pCreateInfo->nBindingDescriptions,
    .pVertexBindingDescriptions      = pCreateInfo->pBindingDescriptions,
    .vertexAttributeDescriptionCount = pCreateInfo->nAttributeDescriptions,
    .pVertexAttributeDescriptions    = pCreateInfo->pAttributeDescriptions,
  };

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState                = {};
  inputAssemblyState.sType                                                 = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyState.primitiveRestartEnable                                = VK_FALSE;
  inputAssemblyState.topology                                              = pCreateInfo->topology;

  VkViewport viewportState                                                 = {};
  viewportState.x                                                          = 0;
  viewportState.y                                                          = 0;
  viewportState.width                                                      = (float)(pCreateInfo->extent.width);
  viewportState.height                                                     = (float)(pCreateInfo->extent.height);
  viewportState.minDepth                                                   = 0.0f;
  viewportState.maxDepth                                                   = 1.0f;

  VkRect2D scissor                                                         = {};
  scissor.offset                                                           = (VkOffset2D){ 0, 0 };
  scissor.extent                                                           = pCreateInfo->extent;

  VkPipelineViewportStateCreateInfo viewportStateCreateInfo                = {};
  viewportStateCreateInfo.sType                                            = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportStateCreateInfo.pViewports                                       = &viewportState;
  viewportStateCreateInfo.pScissors                                        = &scissor;
  viewportStateCreateInfo.scissorCount                                     = 1;
  viewportStateCreateInfo.viewportCount                                    = 1;

  VkPipelineRasterizationStateCreateInfo rasterizerPipelineStateCreateInfo = {};
  rasterizerPipelineStateCreateInfo.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizerPipelineStateCreateInfo.depthClampEnable                       = VK_FALSE;
  rasterizerPipelineStateCreateInfo.rasterizerDiscardEnable                = VK_FALSE;
  rasterizerPipelineStateCreateInfo.polygonMode                            = VK_POLYGON_MODE_FILL;

  if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_CULLING))
    rasterizerPipelineStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; // No one uses other culling modes. If you do, I
                                                                        // hate you.
  else
    rasterizerPipelineStateCreateInfo.cullMode = VK_CULL_MODE_NONE;

  rasterizerPipelineStateCreateInfo.frontFace                              = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizerPipelineStateCreateInfo.depthBiasEnable                        = VK_FALSE;
  rasterizerPipelineStateCreateInfo.depthBiasConstantFactor                = 0.0f;
  rasterizerPipelineStateCreateInfo.depthBiasClamp                         = 0.0f;
  rasterizerPipelineStateCreateInfo.depthBiasSlopeFactor                   = 0.0f;
  rasterizerPipelineStateCreateInfo.lineWidth                              = 1.0f;

  VkPipelineMultisampleStateCreateInfo multisamplerPipelineStageCreateInfo = {};
  multisamplerPipelineStageCreateInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisamplerPipelineStageCreateInfo.sampleShadingEnable                  = VK_FALSE;
  multisamplerPipelineStageCreateInfo.alphaToCoverageEnable                = VK_FALSE;
  multisamplerPipelineStageCreateInfo.alphaToOneEnable                     = VK_FALSE;
  multisamplerPipelineStageCreateInfo.minSampleShading                     = 1.0f;
  multisamplerPipelineStageCreateInfo.pSampleMask                          = VK_NULL_HANDLE;

  if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING))
    multisamplerPipelineStageCreateInfo.rasterizationSamples = pCreateInfo->samples;
  else
    multisamplerPipelineStageCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorblendAttachmentState = {};

  if (pCreateInfo->blend_state != NULL)
  {
    const NV_GPU_PipelineBlendState* blendState   = pCreateInfo->blend_state;

    colorblendAttachmentState.blendEnable         = VK_TRUE;
    colorblendAttachmentState.srcColorBlendFactor = blendState->srcColorBlendFactor;
    colorblendAttachmentState.dstColorBlendFactor = blendState->dstColorBlendFactor;
    colorblendAttachmentState.colorBlendOp        = blendState->colorBlendOp;
    colorblendAttachmentState.srcAlphaBlendFactor = blendState->srcAlphaBlendFactor;
    colorblendAttachmentState.dstAlphaBlendFactor = blendState->dstAlphaBlendFactor;
    colorblendAttachmentState.alphaBlendOp        = blendState->alphaBlendOp;
    colorblendAttachmentState.colorWriteMask      = blendState->colorWriteMask;
  }
  else
  {
    colorblendAttachmentState.blendEnable    = VK_FALSE;
    colorblendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  }

  VkPipelineColorBlendStateCreateInfo colorblendState = {};
  colorblendState.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorblendState.attachmentCount                     = 1;
  colorblendState.pAttachments                        = &colorblendAttachmentState;
  colorblendState.logicOp                             = VK_LOGIC_OP_COPY;
  colorblendState.logicOpEnable                       = VK_FALSE;
  colorblendState.blendConstants[0]                   = 0.0f;
  colorblendState.blendConstants[1]                   = 0.0f;
  colorblendState.blendConstants[2]                   = 0.0f;
  colorblendState.blendConstants[3]                   = 0.0f;

  VkPipelineShaderStageCreateInfo* shader_infos       = (VkPipelineShaderStageCreateInfo*)calloc(pCreateInfo->nShaders, sizeof(VkPipelineShaderStageCreateInfo));
  for (int i = 0; i < pCreateInfo->nShaders; i++)
  {
    shader_infos[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_infos[i].stage  = (VkShaderStageFlagBits)pCreateInfo->pShaders[i]->stage;
    shader_infos[i].module = (VkShaderModule)pCreateInfo->pShaders[i]->shader_module;
    shader_infos[i].pName  = "main";
  }

  VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount          = pCreateInfo->nShaders,
    .pStages             = shader_infos,
    .pVertexInputState   = &vertexInputState,
    .pInputAssemblyState = &inputAssemblyState,
    .pViewportState      = &viewportStateCreateInfo,
    .pRasterizationState = &rasterizerPipelineStateCreateInfo,
    .pMultisampleState   = &multisamplerPipelineStageCreateInfo,
    .pColorBlendState    = &colorblendState,
    .layout              = pCreateInfo->pipeline_layout,
    .renderPass          = pCreateInfo->render_pass,
    .subpass             = pCreateInfo->subpass,
    .basePipelineHandle  = pCreateInfo->old_pipeline,
    .basePipelineIndex   = 0, // ?
  };

  VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
  const VkDynamicState             dynamicStates[]  = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_DYNAMIC_VIEWPORT))
  {
  }
  else
  {
    dynamicStateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount       = 2;
    dynamicStateInfo.pDynamicStates          = dynamicStates;
    graphicsPipelineCreateInfo.pDynamicState = &dynamicStateInfo;
  }

  VkPipelineDepthStencilStateCreateInfo depthStencilState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
  if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK))
  {
    depthStencilState.sType                       = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable             = VK_TRUE;
    depthStencilState.depthWriteEnable            = VK_TRUE;
    depthStencilState.depthCompareOp              = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthBoundsTestEnable       = VK_FALSE;
    depthStencilState.minDepthBounds              = 0.0f;
    depthStencilState.maxDepthBounds              = 1.0f;
    depthStencilState.stencilTestEnable           = VK_FALSE;

    graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilState;
  }

  graphicsPipelineCreateInfo.basePipelineHandle = base_pipeline;

  // if(cacheIsNull) cacheCreator.join();
  NVVK_ResultCheck(vkCreateGraphicsPipelines(device, pCreateInfo->cache, 1, &graphicsPipelineCreateInfo, NOVA_VK_ALLOCATOR, dstPipeline));

  base_pipeline = *dstPipeline;

  NV_free(shader_infos);
}

void
NV_GPU_CreateRenderPass(NV_GPU_RenderPassCreateInfo const* pCreateInfo, VkRenderPass* dstRenderPass, u32 flags)
{
  CVK_REQUIRED_PTR(device);
  CVK_REQUIRED_PTR(pCreateInfo);
  CVK_REQUIRED_PTR(dstRenderPass);
  CVK_NOT_EQUAL_TO(pCreateInfo->format, NOVA_FORMAT_UNDEFINED);

  if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK))
  {
    CVK_NOT_EQUAL_TO(pCreateInfo->depthBufferFormat, NOVA_FORMAT_UNDEFINED);
  }

  VkAttachmentDescription colorAttachmentDescription = {};
  colorAttachmentDescription.format                  = NV_NVFormatToVKFormat(pCreateInfo->format);
  colorAttachmentDescription.samples                 = HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) ? pCreateInfo->samples : VK_SAMPLE_COUNT_1_BIT;
  colorAttachmentDescription.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachmentDescription.storeOp                 = HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING) ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachmentDescription.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachmentDescription.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachmentDescription.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachmentDescription.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  // colorAttachmentDescription.finalLayout =
  // (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING)) ?
  // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentReference          = {};
  colorAttachmentReference.attachment                     = 0;
  colorAttachmentReference.layout                         = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass                            = {};
  subpass.pipelineBindPoint                               = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount                            = 1;
  subpass.pColorAttachments                               = &colorAttachmentReference;

  NV_dynarray_t /* VkAttachmentDescription */ attachments = NV_dynarray_init(sizeof(VkAttachmentDescription), 5);
  NV_dynarray_push_back(&attachments, &colorAttachmentDescription);

  VkAttachmentDescription depthAttachment    = {};
  VkAttachmentReference   depthAttachmentRef = {};
  if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_DEPTH_CHECK))
  {
    depthAttachment.format         = NV_NVFormatToVKFormat(pCreateInfo->depthBufferFormat); // Why wasn't this being used?
    depthAttachment.samples        = pCreateInfo->samples;
    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;

    // if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING))
    // 	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // else
    depthAttachment.finalLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    depthAttachmentRef.attachment   = NV_dynarray_size(&attachments);
    depthAttachmentRef.layout       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    NV_dynarray_push_back(&attachments, &depthAttachment);
  }

  VkAttachmentReference   colorAttachmentResolveRef = {};
  VkAttachmentDescription colorAttachmentResolve    = {};
  if (HAS_FLAG(CVK_PIPELINE_FLAGS_FORCE_MULTISAMPLING))
  {
    colorAttachmentResolve.format         = NV_NVFormatToVKFormat(pCreateInfo->format);
    colorAttachmentResolve.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    colorAttachmentResolveRef.attachment  = NV_dynarray_size(&attachments);
    colorAttachmentResolveRef.layout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    NV_dynarray_push_back(&attachments, &colorAttachmentResolve);

    subpass.pResolveAttachments = &colorAttachmentResolveRef;
  }

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount        = NV_dynarray_size(&attachments);
  renderPassInfo.pAttachments           = (const VkAttachmentDescription*)NV_dynarray_data(&attachments);
  renderPassInfo.subpassCount           = 1;
  renderPassInfo.pSubpasses             = &subpass;
  renderPassInfo.dependencyCount        = 0;
  renderPassInfo.pDependencies          = NULL;

  NVVK_ResultCheck(vkCreateRenderPass(device, &renderPassInfo, NOVA_VK_ALLOCATOR, dstRenderPass));

  NV_dynarray_destroy(&attachments);
}

void
NV_GPU_CreatePipelineLayout(NV_GPU_PipelineCreateInfo const* pCreateInfo, VkPipelineLayout* dstLayout)
{
  CVK_REQUIRED_PTR(device);
  CVK_REQUIRED_PTR(pCreateInfo);
  CVK_REQUIRED_PTR(dstLayout);

  // int totalLayouts = 0;
  // for (int i = 0; i < pCreateInfo->nShaders; i++) {
  // 	totalLayouts += pCreateInfo->pShaders[i]->nsetlayouts;
  // }

  // NV_dynarray_t *sets = NV_dynarray_init(sizeof(VkDescriptorSetLayout),
  // totalLayouts);

  // for (int i = 0; i < pCreateInfo->nShaders; i++) {
  // 	const NVSM_shader_t *shader = pCreateInfo->pShaders[i];
  // 	for (int j = 0; j < shader->nsetlayouts; j++) {
  // 		NV_dynarray_push_back(sets, &shader->setlayouts[j]);
  // 	}
  // }

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
  pipelineLayoutCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount             = pCreateInfo->nDescriptorLayouts;
  pipelineLayoutCreateInfo.pSetLayouts                = pCreateInfo->pDescriptorLayouts;
  pipelineLayoutCreateInfo.pushConstantRangeCount     = pCreateInfo->nPushConstants;
  pipelineLayoutCreateInfo.pPushConstantRanges        = pCreateInfo->pPushConstants;

  NVVK_ResultCheck(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, NOVA_VK_ALLOCATOR, dstLayout));
}

void
NV_GPU_CreateSwapchain(NV_GPU_SwapchainCreateInfo const* pCreateInfo, VkSwapchainKHR* dstSwapchain)
{
  CVK_REQUIRED_PTR(device);
  CVK_REQUIRED_PTR(pCreateInfo);
  CVK_NOT_EQUAL_TO(pCreateInfo->extent.width, 0);
  CVK_NOT_EQUAL_TO(pCreateInfo->extent.height, 0);
  CVK_NOT_EQUAL_TO(pCreateInfo->format, NOVA_FORMAT_UNDEFINED);
  CVK_NOT_EQUAL_TO(pCreateInfo->image_count, 0);

  VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
  swapChainCreateInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapChainCreateInfo.surface                  = surface;
  swapChainCreateInfo.imageExtent              = pCreateInfo->extent;
  swapChainCreateInfo.minImageCount            = pCreateInfo->image_count;
  swapChainCreateInfo.imageFormat              = NV_NVFormatToVKFormat(pCreateInfo->format);
  swapChainCreateInfo.imageColorSpace          = pCreateInfo->color_space;
  swapChainCreateInfo.imageArrayLayers         = 1;
  swapChainCreateInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapChainCreateInfo.preTransform             = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  swapChainCreateInfo.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapChainCreateInfo.clipped                  = VK_TRUE;
  swapChainCreateInfo.presentMode              = pCreateInfo->present_mode;
  swapChainCreateInfo.oldSwapchain             = pCreateInfo->old_swapchain;
  swapChainCreateInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
  NVVK_ResultCheck(vkCreateSwapchainKHR(device, &swapChainCreateInfo, NOVA_VK_ALLOCATOR, dstSwapchain));
}

NV_GPU_PipelineBlendState
NV_GPU_InitPipelineBlendState(NV_GPU_PipelineBlendPreset preset)
{
  NV_GPU_PipelineBlendState ret = {};
  ret.colorWriteMask            = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  switch (preset)
  {
    case CVK_BLEND_PRESET_NONE:
      ret.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      ret.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      ret.colorBlendOp        = VK_BLEND_OP_ADD;
      ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      ret.alphaBlendOp        = VK_BLEND_OP_ADD;
      break;
    case CVK_BLEND_PRESET_ALPHA:
      ret.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
      ret.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      ret.colorBlendOp        = VK_BLEND_OP_ADD;
      ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      ret.alphaBlendOp        = VK_BLEND_OP_ADD;
      break;
    case CVK_BLEND_PRESET_ADDITIVE:
      ret.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
      ret.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
      ret.colorBlendOp        = VK_BLEND_OP_ADD;
      ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      ret.alphaBlendOp        = VK_BLEND_OP_ADD;
      break;
    case CVK_BLEND_PRESET_MULTIPLICATIVE:
      ret.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
      ret.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      ret.colorBlendOp        = VK_BLEND_OP_ADD;
      ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
      ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      ret.alphaBlendOp        = VK_BLEND_OP_ADD;
      break;
    case CVK_BLEND_PRESET_PREMULTIPLIED_ALPHA:
      ret.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
      ret.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      ret.colorBlendOp        = VK_BLEND_OP_ADD;
      ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      ret.alphaBlendOp        = VK_BLEND_OP_ADD;
      break;
    case CVK_BLEND_PRESET_SUBTRACTIVE:
      ret.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      ret.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
      ret.colorBlendOp        = VK_BLEND_OP_REVERSE_SUBTRACT;
      ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
      ret.alphaBlendOp        = VK_BLEND_OP_REVERSE_SUBTRACT;
      break;
    default:
      ret.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      ret.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
      ret.colorBlendOp        = VK_BLEND_OP_ADD;
      ret.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      ret.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
      ret.alphaBlendOp        = VK_BLEND_OP_ADD;
      break;
  }
  return ret;
}

void
NV_VK_CreateBuffer(size_t size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, VkBuffer* dstBuffer, VkDeviceMemory* retMem, bool externallyAllocated)
{
  if (size == 0)
  {
    return;
  }

  VkBuffer           newBuffer;
  VkDeviceMemory     newMemory;

  VkBufferCreateInfo bufferCreateInfo = {};
  bufferCreateInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.size               = size;
  bufferCreateInfo.usage              = usageFlags;
  bufferCreateInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;
  NVVK_ResultCheck(vkCreateBuffer(device, &bufferCreateInfo, NOVA_VK_ALLOCATOR, &newBuffer));

  VkMemoryRequirements bufferMemoryRequirements;
  vkGetBufferMemoryRequirements(device, newBuffer, &bufferMemoryRequirements);

  if (!externallyAllocated)
  {
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize       = bufferMemoryRequirements.size;
    allocInfo.memoryTypeIndex      = NV_VK_GetMemType(bufferMemoryRequirements.memoryTypeBits, propertyFlags);
    NVVK_ResultCheck(vkAllocateMemory(device, &allocInfo, NOVA_VK_ALLOCATOR, &newMemory));

    NVVK_ResultCheck(vkBindBufferMemory(device, newBuffer, newMemory, 0));
    *retMem = newMemory;
  }

  *dstBuffer = newBuffer;
}

void
NV_VK_StageBufferTransfer(VkBuffer dst, void* data, size_t size)
{
  VkBuffer           stagingBuffer;
  VkDeviceMemory     stagingBufferMemory;

  VkBufferCreateInfo stagingBufferInfo = {};
  stagingBufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferInfo.size               = size;
  stagingBufferInfo.usage              = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  stagingBufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;
  NVVK_ResultCheck(vkCreateBuffer(device, &stagingBufferInfo, NOVA_VK_ALLOCATOR, &stagingBuffer));

  VkMemoryRequirements stagingBufferMemoryRequirements;
  vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferMemoryRequirements);

  VkMemoryAllocateInfo stagingBufferAlloc = {};
  stagingBufferAlloc.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  stagingBufferAlloc.allocationSize       = stagingBufferMemoryRequirements.size;
  stagingBufferAlloc.memoryTypeIndex
      = NV_VK_GetMemType(stagingBufferMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  NVVK_ResultCheck(vkAllocateMemory(device, &stagingBufferAlloc, NULL, &stagingBufferMemory));

  NVVK_ResultCheck(vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0));

  void* mapped;
  NVVK_ResultCheck(vkMapMemory(device, stagingBufferMemory, 0, size, 0, &mapped));
  NV_memcpy(mapped, data, size);
  vkUnmapMemory(device, stagingBufferMemory);

  const VkBufferCopy copy = { .srcOffset = 0, .dstOffset = 0, .size = size };

  VkCommandBuffer    cmd  = NV_VK_BeginCommandBuffer();
  vkCmdCopyBuffer(cmd, stagingBuffer, dst, 1, &copy);
  NVVK_ResultCheck(NV_VK_EndCommandBuffer(cmd, TransferQueue, 1));

  vkDestroyBuffer(device, stagingBuffer, NOVA_VK_ALLOCATOR);
  vkFreeMemory(device, stagingBufferMemory, NOVA_VK_ALLOCATOR);
}

u32
NV_VK_GetMemType(const u32 memoryTypeBits, const VkMemoryPropertyFlags memoryProperties)
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

VkCommandBuffer
NV_VK_BeginCommandBufferFrom(VkCommandBuffer src)
{
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(src, &beginInfo);

  return src;
}

VkCommandBuffer
NV_VK_BeginCommandBuffer()
{
  if (!cmd_pool)
  {
    VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
    cmdPoolCreateInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCreateInfo.queueFamilyIndex        = GraphicsFamilyIndex;
    cmdPoolCreateInfo.flags                   = 0;
    NVVK_ResultCheck(vkCreateCommandPool(device, &cmdPoolCreateInfo, NOVA_VK_ALLOCATOR, &cmd_pool));
  }

  VkCommandBufferAllocateInfo cmdAllocInfo = {};
  cmdAllocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdAllocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdAllocInfo.commandBufferCount          = 1;
  cmdAllocInfo.commandPool                 = cmd_pool;
  NVVK_ResultCheck(vkAllocateCommandBuffers(device, &cmdAllocInfo, &buffer));

  return NV_VK_BeginCommandBufferFrom(buffer);
}

VkResult
NV_VK_EndCommandBuffer(VkCommandBuffer cmd, VkQueue queue, bool waitForExecution)
{
  VkResult res = vkEndCommandBuffer(cmd);
  if (res != VK_SUCCESS)
    return res;

  VkSubmitInfo submitInfo       = {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &cmd;

  VkFence           fence       = VK_NULL_HANDLE;
  VkFenceCreateInfo fenceInfo   = {};
  fenceInfo.sType               = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags               = 0;

  if (waitForExecution)
  {
    res = vkCreateFence(device, &fenceInfo, NOVA_VK_ALLOCATOR, &fence);
    if (res != VK_SUCCESS)
      return res;
  }

  res = vkQueueSubmit(queue, 1, &submitInfo, fence);
  if (res != VK_SUCCESS)
    return res;

  if (waitForExecution)
  {
    if (fence != VK_NULL_HANDLE)
    {
      res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
      if (res != VK_SUCCESS)
        return res;
      vkDestroyFence(device, fence, NOVA_VK_ALLOCATOR);
    }
    vkDeviceWaitIdle(device);
    vkFreeCommandBuffers(device, cmd_pool, 1, &cmd);
    buffer = NULL;
  }

  return VK_SUCCESS;
}

void
NV_VK_LoadBinaryFile(const char* path, u8* dst, u32* dstSize)
{
  FILE* f = fopen(path, "rb");
  NV_assert(f != NULL);

  fseek(f, 0, SEEK_END);
  int file_size = ftell(f);

  if (dst == NULL)
  {
    *dstSize = file_size;
    fclose(f);
    return;
  }

  rewind(f);

  fread(dst, 1, file_size, f);

  fclose(f);
}

void
NV_VK_StageImageTransfer(VkImage dst, const void* data, int width, int height, int image_size)
{
  VkBuffer             stagingBuffer       = VK_NULL_HANDLE;
  VkDeviceMemory       stagingBufferMemory = VK_NULL_HANDLE;

  VkMemoryRequirements mem_req;
  vkGetImageMemoryRequirements(device, dst, &mem_req);

  const VkBufferCreateInfo stagingBufferInfo = {
    .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size        = mem_req.size,
    .usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  NVVK_ResultCheck(vkCreateBuffer(device, &stagingBufferInfo, NOVA_VK_ALLOCATOR, &stagingBuffer));

  VkMemoryRequirements stagingBufferRequirements;
  vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingBufferRequirements);

  const VkMemoryAllocateInfo stagingBufferAllocInfo = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize                                            = stagingBufferRequirements.size,
    .memoryTypeIndex = NV_VK_GetMemType(stagingBufferRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) };

  NVVK_ResultCheck(vkAllocateMemory(device, &stagingBufferAllocInfo, NULL, &stagingBufferMemory));
  NVVK_ResultCheck(vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0));

  void* stagingBufferMapped;
  NVVK_ResultCheck(vkMapMemory(device, stagingBufferMemory, 0, stagingBufferRequirements.size, 0, &stagingBufferMapped));
  NV_memcpy(stagingBufferMapped, data, image_size);
  vkUnmapMemory(device, stagingBufferMemory);

  const VkCommandBuffer cmd = NV_VK_BeginCommandBuffer();

  NV_VK_TransitionTextureLayout(cmd, dst, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

  VkBufferImageCopy region               = {};
  region.imageExtent                     = (VkExtent3D){ width, height, 1 };
  region.imageOffset                     = (VkOffset3D){ 0, 0, 0 };
  region.bufferOffset                    = 0;
  region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.layerCount     = 1;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.mipLevel       = 0;
  vkCmdCopyBufferToImage(cmd, stagingBuffer, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  NV_VK_TransitionTextureLayout(cmd, dst, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0,
      VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

  NV_VK_EndCommandBuffer(cmd, TransferQueue, true);

  vkDestroyBuffer(device, stagingBuffer, NOVA_VK_ALLOCATOR);
  vkFreeMemory(device, stagingBufferMemory, NOVA_VK_ALLOCATOR);
}

void
NV_VK_CreateTextureFromMemory(u8* buffer, u32 width, u32 height, NVFormat format, VkImage* dst, VkDeviceMemory* dstMem)
{
  NV_VK_CreateTextureEmpty(width, height, format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, NULL, dst, dstMem);
  NV_VK_StageImageTransfer(*dst, buffer, width, height, width * height * NV_FormatGetBytesPerPixel(format));
}

// If the format given is oki then it'll just return it
// otherwise it gives you the next best optoin
NVFormat
NV_VK_GetSupportedFormatForDraw(NVFormat fmt)
{
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(physDevice, NV_NVFormatToVKFormat(fmt), &formatProperties);

  if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) == 0)
  {
    // format not supported
    const char* fmt_str;
    NV_FormatToString(fmt, &fmt_str);
    NV_LOG_WARNING("Format %i (%s) unsupported. Using NOVA_FORMAT_RGBA8", fmt, fmt_str);
    return NOVA_FORMAT_RGBA8;
  }

  return fmt;
}

void
NV_VK_CreateTextureEmpty(u32 width, u32 height, NVFormat format, VkSampleCountFlagBits samples, VkImageUsageFlags usage, int* image_size, VkImage* dst, VkDeviceMemory* dstMem)
{
  if (usage & VK_IMAGE_USAGE_SAMPLED_BIT)
  {
    format = NV_VK_GetSupportedFormatForDraw(format);
  }

  VkImageCreateInfo imageCreateInfo = {};
  imageCreateInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType         = VK_IMAGE_TYPE_2D;
  imageCreateInfo.extent.width      = width;
  imageCreateInfo.extent.height     = height;
  imageCreateInfo.extent.depth      = 1;
  imageCreateInfo.mipLevels         = 1;
  imageCreateInfo.arrayLayers       = 1;
  imageCreateInfo.format            = NV_NVFormatToVKFormat(format);
  imageCreateInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.usage             = usage;
  imageCreateInfo.samples           = samples;
  imageCreateInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
  NVVK_ResultCheck(vkCreateImage(device, &imageCreateInfo, NOVA_VK_ALLOCATOR, dst));

  VkMemoryRequirements imageMemoryRequirements;
  vkGetImageMemoryRequirements(device, *dst, &imageMemoryRequirements);

  if (image_size)
  {
    *image_size = imageMemoryRequirements.size;
  }

  // allow for preallocated memory.
  if (dstMem != NULL)
  {
    const u32            localDeviceMemoryIndex = NV_VK_GetMemType(imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocInfo              = {};
    allocInfo.sType                             = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize                    = imageMemoryRequirements.size;
    allocInfo.memoryTypeIndex                   = localDeviceMemoryIndex;

    NVVK_ResultCheck(vkAllocateMemory(device, &allocInfo, NULL, dstMem));
    NVVK_ResultCheck(vkBindImageMemory(device, *dst, *dstMem, 0));
  }
}

u8*
NV_VK_CreateTextureFromDisk(const char* path, u32* width, u32* height, NVFormat* channels, VkImage* dst, VkDeviceMemory* dstMem)
{
  NVImage tex = NV_img_load(path);

  NV_assert(tex.data != NULL);

  *width    = tex.w;
  *height   = tex.h;
  *channels = tex.fmt;

  NV_VK_CreateTextureFromMemory(tex.data, tex.w, tex.h, *channels, dst, dstMem);
  return tex.data;
}

void
NV_VK_TransitionTextureLayout(VkCommandBuffer cmd, VkImage image, u32 mipLevels, VkImageAspectFlagBits aspect, VkImageLayout oldLayout, VkImageLayout newLayout,
    VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage)
{
  VkImageMemoryBarrier barrier            = {};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = oldLayout;
  barrier.newLayout                       = newLayout;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = image;
  barrier.subresourceRange.aspectMask     = aspect;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.srcAccessMask                   = srcAccessMask;
  barrier.dstAccessMask                   = dstAccessMask;
  vkCmdPipelineBarrier(cmd, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

bool
NV_VK_GetSupportedFormat(VkPhysicalDevice physDevice, VkSurfaceKHR surface, NVFormat* dstFormat, VkColorSpaceKHR* dstColorSpace)
{
  CVK_REQUIRED_PTR(device);
  CVK_REQUIRED_PTR(physDevice);
  CVK_REQUIRED_PTR(surface);
  CVK_REQUIRED_PTR(dstFormat);
  CVK_REQUIRED_PTR(dstColorSpace);

  u32 formatCount = 0;
  NVVK_ResultCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, VK_NULL_HANDLE));
  NV_dynarray_t /* VkSurfaceFormatKHR */ surfaceFormats = NV_dynarray_init(sizeof(VkSurfaceFormatKHR), formatCount);
  NVVK_ResultCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &formatCount, (VkSurfaceFormatKHR*)NV_dynarray_data(&surfaceFormats)));

  VkSurfaceFormatKHR       selectedFormat = { VK_FORMAT_MAX_ENUM, VK_COLOR_SPACE_MAX_ENUM_KHR };

  const VkSurfaceFormatKHR desired_formats[]
      = { { VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }, { VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR } };

  for (u32 i = 0; i < formatCount; i++)
  {
    const VkSurfaceFormatKHR* surfaceFormat = (VkSurfaceFormatKHR*)NV_dynarray_get(&surfaceFormats, i);
    for (u32 j = 0; j < NV_arrlen(desired_formats); j++)
    {
      if (surfaceFormat->format == desired_formats[j].format && surfaceFormat->colorSpace == desired_formats[j].colorSpace)
      {
        selectedFormat.format     = surfaceFormat->format;
        selectedFormat.colorSpace = surfaceFormat->colorSpace;
      }
    }
  }

  NV_dynarray_destroy(&surfaceFormats);
  if (selectedFormat.format == VK_FORMAT_MAX_ENUM || selectedFormat.colorSpace == VK_COLOR_SPACE_MAX_ENUM_KHR)
  {
    return VK_FALSE;
  }
  else
  {
    *dstFormat     = NV_VKFormatToNVFormat(selectedFormat.format);
    *dstColorSpace = selectedFormat.colorSpace;

    return VK_TRUE;
  }

  return VK_FALSE;
}

u32
NV_VK_GetSurfaceImageCount(VkPhysicalDevice physDevice, VkSurfaceKHR surface)
{
  CVK_REQUIRED_PTR(physDevice);
  CVK_REQUIRED_PTR(surface);

  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  NVVK_ResultCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &surfaceCapabilities));

  u32 requestedImageCount = surfaceCapabilities.minImageCount + 1;
  if (requestedImageCount < surfaceCapabilities.maxImageCount)
    requestedImageCount = surfaceCapabilities.maxImageCount;

  return requestedImageCount;
}

// NOVA_GPU_OBJECTS

typedef struct NV_GPU_Memory
{
  VkDeviceMemory     memory;
  void*              mapped;
  size_t             map_size, map_offset; // if mapped, the mapping size and offset
  NV_GPU_MemoryUsage usage;
  size_t             size;
} NV_GPU_Memory;

// these parameters should be replaced
// properties should be replaced by usage. Like NOVA_GPU_MEMORY_USAGE_GPU,
// CPU_TO_GPU, GPU_TO_CPU, etc.
void
NV_GPU_AllocateMemory(size_t size, NV_GPU_MemoryUsage usage, NV_GPU_Memory** dst)
{
  (*dst)                                 = calloc(1, sizeof(NV_GPU_Memory));

  (*dst)->size                           = size;
  (*dst)->usage                          = usage;
  const VkMemoryPropertyFlags properties = (VkMemoryPropertyFlags)usage;

  VkMemoryAllocateInfo        alloc_info = {};
  alloc_info.sType                       = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize              = size;

  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physDevice, &mem_properties);

  for (u32 i = 0; i < mem_properties.memoryTypeCount; i++)
  {
    if ((properties & mem_properties.memoryTypes[i].propertyFlags) == properties)
    {
      alloc_info.memoryTypeIndex = i;
      break;
    }
  }

  NVVK_ResultCheck(vkAllocateMemory(device, &alloc_info, NULL, &(*dst)->memory));
}

void
NV_GPU_CreateBuffer(size_t size, int alignment, VkBufferUsageFlags usage, NV_GPU_Buffer* dst)
{
  dst->size                      = size;
  dst->alignment                 = alignment;
  dst->type                      = usage;

  const int          aligned_sz  = align_up(size, alignment);

  VkBufferCreateInfo buffer_info = {};
  buffer_info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size               = aligned_sz;
  buffer_info.usage              = usage;
  NVVK_ResultCheck(vkCreateBuffer(device, &buffer_info, NOVA_VK_ALLOCATOR, &dst->buffer));
}

void
NV_GPU_WriteToLocalBuffer(NV_GPU_Buffer* buffer, size_t size, void* data, size_t offset)
{
  NV_GPU_Buffer staging_buffer;
  NV_GPU_CreateBuffer(size, NOVA_GPU_ALIGNMENT_UNNECESSARY, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, &staging_buffer);

  NV_GPU_Memory* staging_memory;
  NV_GPU_AllocateMemory(size, NOVA_GPU_MEMORY_USAGE_CPU_VISIBLE | NOVA_GPU_MEMORY_USAGE_CPU_WRITEABLE, &staging_memory);

  NV_GPU_BindBufferToMemory(staging_memory, 0, &staging_buffer);

  void* mapped = NULL;
  NV_GPU_MapMemory(staging_memory, size, 0, &mapped);
  NV_memcpy(mapped, data, size);
  NV_GPU_UnmapMemory(staging_memory);

  VkCommandBuffer cmd  = NV_VK_BeginCommandBuffer();

  VkBufferCopy    copy = {
       .srcOffset = 0,
       .dstOffset = offset,
       .size      = size,
  };
  vkCmdCopyBuffer(cmd, staging_buffer.buffer, buffer->buffer, 1, &copy);

  if (NV_VK_EndCommandBuffer(cmd, GraphicsQueue, 1) != VK_SUCCESS)
  {
    NV_LOG_ERROR("Failed to write data to GPU buffer");
  }
}

void
NV_GPU_WriteToUniformBuffer(NV_GPU_Buffer* buffer, size_t size, void* data, size_t offset)
{
  void* mapped;
  NV_GPU_MapMemory(buffer->memory, size, offset, &mapped);
  NV_memcpy(mapped, data, size);
  NV_GPU_UnmapMemory(buffer->memory);
}

void
NV_GPU_WriteToBuffer(NV_GPU_Buffer* buffer, size_t size, void* data, size_t offset)
{
  if (!(buffer->memory->usage & NOVA_GPU_MEMORY_USAGE_CPU_VISIBLE))
  {
    NV_GPU_WriteToLocalBuffer(buffer, size, data, offset);
    return;
  }
  void* mapped = NULL;
  NV_GPU_MapMemory(buffer->memory, size, offset, &mapped);
  NV_memcpy(mapped, data, size);
  NV_GPU_UnmapMemory(buffer->memory);
}

void
NV_GPU_MapMemory(NV_GPU_Memory* memory, size_t size, size_t offset, void** out)
{
  if (!(memory->usage & NOVA_GPU_MEMORY_USAGE_CPU_VISIBLE))
  {
    NV_LOG_ERROR("Memory usage does not have NOVA_GPU_MEMORY_USAGE_CPU_VISIBLE "
                 "Use NV_GPU_WriteToBuffer() instead.");
    return;
  }
  if (memory->map_size != 0)
  {
    *out = memory->mapped;
    return;
  }
  if (vkMapMemory(device, memory->memory, offset, size, 0, &memory->mapped) != VK_SUCCESS)
  {
    NV_LOG_ERROR("Memory could not be mapped for write");
    return;
  }
  memory->map_size   = size;
  memory->map_offset = offset;
  *out               = memory->mapped;
}

void
NV_GPU_UnmapMemory(NV_GPU_Memory* memory)
{
  // if (!(memory->usage & NOVA_GPU_MEMORY_USAGE_CPU_WRITEABLE)) {
  //     VkMappedMemoryRange range = {
  //         .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
  //         .memory = memory->memory,
  //         .offset = memory->map_offset,
  //         .size = memory->map_size,
  //     };
  //     vkFlushMappedMemoryRanges(device, 1, &range);
  // }
  memory->map_offset = 0;
  memory->map_size   = 0;
  vkUnmapMemory(device, memory->memory);
}

void
NV_GPU_FreeMemory(NV_GPU_Memory* mem)
{
  if (mem->memory)
  {
    vkFreeMemory(device, mem->memory, NULL);
    free(mem);
  }
}

// I think these given an error when, say offset is too big so maybe we can
// check their return values?
void
NV_GPU_BindBufferToMemory(NV_GPU_Memory* mem, size_t offset, NV_GPU_Buffer* buffer)
{
  buffer->memory = mem;
  buffer->offset = offset;

  NVVK_ResultCheck(vkBindBufferMemory(device, buffer->buffer, mem->memory, offset));
}

void
NV_GPU_TextureAttachView(NV_GPU_Texture* tex, VkImageView view)
{
  tex->view = view;
}

void
NV_GPU_BindTextureToMemory(NV_GPU_Memory* mem, size_t offset, NV_GPU_Texture* tex)
{
  tex->memory = mem;
  tex->offset = offset;
  NVVK_ResultCheck(vkBindImageMemory(device, tex->image, mem->memory, offset));

  VkImageAspectFlags aspect = 0;
  if (NV_FormatHasDepthChannel(tex->format))
  {
    aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
  }
  else if (NV_FormatHasColorChannel(tex->format))
  {
    aspect = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  if (NV_FormatHasStencilChannel(tex->format))
  {
    aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }

  VkImageSubresourceRange subresourceRange
      = { .aspectMask = aspect, .baseMipLevel = 0, .levelCount = VK_REMAINING_MIP_LEVELS, .baseArrayLayer = 0, .layerCount = VK_REMAINING_ARRAY_LAYERS };

  NVFormat dst_format             = tex->format;
  dst_format                      = NV_VK_GetSupportedFormatForDraw(dst_format);

  VkImageViewCreateInfo view_info = {
    .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image            = tex->image,
    .viewType         = (VkImageViewType)tex->type,
    .format           = NV_NVFormatToVKFormat(dst_format),
    .subresourceRange = subresourceRange,
    .components.r     = VK_COMPONENT_SWIZZLE_IDENTITY,
    .components.g     = VK_COMPONENT_SWIZZLE_IDENTITY,
    .components.b     = VK_COMPONENT_SWIZZLE_IDENTITY,
    .components.a     = VK_COMPONENT_SWIZZLE_IDENTITY,
  };
  NVVK_ResultCheck(vkCreateImageView(device, &view_info, NOVA_VK_ALLOCATOR, &tex->view));
}

void
NV_GPU_DestroyBuffer(NV_GPU_Buffer* buffer)
{
  if (!buffer->buffer)
  {
    NV_LOG_INFO("Attempt to destroy a buffer %u which has a NULL VkBuffer", buffer);
    return;
  }
  vkDeviceWaitIdle(device);
  vkDestroyBuffer(device, buffer->buffer, NOVA_VK_ALLOCATOR);
  NV_memset(buffer, 0, sizeof(NV_GPU_Buffer));
}

void
NV_GPU_DestroyTexture(NV_GPU_Texture* tex)
{
  vkDeviceWaitIdle(device);
  if (!tex->view)
  {
    NV_LOG_INFO("Attempt to destroy an image view which is NULL");
    return;
  }
  if (!tex->image)
  {
    NV_LOG_INFO("Attempt to destroy an image which is NULL");
    return;
  }
  vkDestroyImage(device, tex->image, NOVA_VK_ALLOCATOR);
  vkDestroyImageView(device, tex->view, NOVA_VK_ALLOCATOR);
  NV_memset(tex, 0, sizeof(NV_GPU_Texture));
}

int
NV_GPU_GetBufferSize(const NV_GPU_Buffer* buffer)
{
  return buffer->size;
}

void
NV_GPU_BufferReadback(const NV_GPU_Buffer* buffer, void* dest)
{
  if (!(buffer->memory->usage & NOVA_GPU_BUFFER_TYPE_TRANSFER_SOURCE))
  {
    NV_LOG_ERROR("Cannot readback from buffer that is not transfer source");
    return;
  }

  NV_GPU_Buffer  staging;
  NV_GPU_Memory* staging_mem;
  NV_GPU_CreateBuffer(buffer->size, NOVA_GPU_ALIGNMENT_UNNECESSARY, NOVA_GPU_BUFFER_TYPE_TRANSFER_DESTINATION, &staging);
  NV_GPU_AllocateMemory(buffer->size, NOVA_GPU_MEMORY_USAGE_CPU_VISIBLE, &staging_mem);
  NV_GPU_BindBufferToMemory(staging_mem, 0, &staging);

  VkCommandBuffer cmd  = NV_VK_BeginCommandBuffer();

  VkBufferCopy    copy = { .srcOffset = 0, .dstOffset = 0, .size = buffer->size };
  vkCmdCopyBuffer(cmd, buffer->buffer, staging.buffer, 1, &copy);

  NV_VK_EndCommandBuffer(cmd, TransferQueue, 1);

  void* mapped;
  NV_GPU_MapMemory(staging_mem, buffer->size, 0, &mapped);
  NV_memcpy(dest, mapped, buffer->size);
  NV_GPU_UnmapMemory(staging_mem);

  NV_GPU_DestroyBuffer(&staging);
  NV_GPU_FreeMemory(staging_mem);
}

void
NV_GPU_GetTextureSize(const NV_GPU_Texture* tex, int* w, int* h)
{
  if (w)
  {
    *w = tex->extent.width;
  }
  if (h)
  {
    *h = tex->extent.height;
  }
}

void
NV_GPU_CreateSampler(const NV_GPU_SamplerCreateInfo* pInfo, NV_GPU_Sampler** sampler)
{
  for (int i = 0; i < (int)g_Samplers.m_size; i++)
  {
    NV_GPU_Sampler* cache = &((NV_GPU_Sampler*)NV_dynarray_data(&g_Samplers))[i];
    if (cache != NULL && cache->filter == pInfo->filter && cache->mipmap_mode == pInfo->mipmap_mode && cache->address_mode == pInfo->address_mode
        && cache->max_anisotropy == pInfo->max_anisotropy && cache->mip_lod_bias == pInfo->mip_lod_bias && cache->min_lod == pInfo->min_lod && cache->max_lod == pInfo->max_lod
        && cache->vksampler != NULL)
    {
      *sampler = cache;
      return;
    }
  }

  NV_GPU_Sampler smap = {
    .filter         = pInfo->filter,
    .mipmap_mode    = pInfo->mipmap_mode,
    .address_mode   = pInfo->address_mode,
    .max_anisotropy = pInfo->max_anisotropy,
    .mip_lod_bias   = pInfo->mip_lod_bias,
    .min_lod        = pInfo->min_lod,
    .max_lod        = pInfo->max_lod,
  };

  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter           = pInfo->filter;
  samplerInfo.minFilter           = pInfo->filter;
  samplerInfo.mipmapMode          = pInfo->mipmap_mode;
  samplerInfo.addressModeU        = pInfo->address_mode;
  samplerInfo.addressModeV        = pInfo->address_mode;
  samplerInfo.addressModeW        = pInfo->address_mode;
  samplerInfo.anisotropyEnable    = pInfo->max_anisotropy > 1.0f;
  samplerInfo.maxLod              = pInfo->max_lod;
  samplerInfo.minLod              = pInfo->min_lod;
  NVVK_ResultCheck(vkCreateSampler(device, &samplerInfo, NOVA_VK_ALLOCATOR, &smap.vksampler));

  NV_dynarray_push_back(&g_Samplers, &smap);
  (*sampler) = &((NV_GPU_Sampler*)g_Samplers.m_data)[g_Samplers.m_size - 1];
}

void
NV_GPU_WriteToTexture(NV_GPU_Texture* tex, const NVImage* src)
{
  if (!(tex->usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
  {
    NV_LOG_ERROR("Cannot write to an image which does not have usage "
                 "VK_IMAGE_USAGE_TRANSFER_DST_BIT");
  }

  const int tex_size = tex->extent.width * tex->extent.height * NV_FormatGetBytesPerPixel(tex->format);
  // if (tex->memory->usage & NOVA_GPU_MEMORY_USAGE_CPU_VISIBLE) {
  //     void *mapped;
  //     NV_GPU_MapMemory(tex->memory, tex_size, tex->offset, &mapped);
  //     memcpy(mapped, src->data, tex_size);
  //     NV_GPU_UnmapMemory(tex->memory);
  //     return;
  // }

  NV_VK_StageImageTransfer(tex->image, src->data, src->w, src->h, tex_size);
}

VkImage
NV_GPU_TextureGet(const NV_GPU_Texture* tex)
{
  return tex ? tex->image : NULL;
}

VkImageView
NV_GPU_TextureGetView(const NV_GPU_Texture* tex)
{
  return tex ? tex->view : NULL;
}

VkSampler
NV_GPU_SamplerGet(const NV_GPU_Sampler* sampler)
{
  return sampler ? sampler->vksampler : NULL;
}

void
NV_GPU_CreateTexture(const NV_GPU_TextureCreateInfo* pInfo, NV_GPU_Texture** tex)
{
  (*tex)       = calloc(1, sizeof(NV_GPU_Texture));

  NVFormat fmt = pInfo->format;
  if (pInfo->usage & VK_IMAGE_USAGE_SAMPLED_BIT)
  {
    fmt = NV_VK_GetSupportedFormatForDraw(fmt);
  }

  (*tex)->type            = pInfo->type;
  (*tex)->format          = pInfo->format;
  (*tex)->extent          = (VkExtent3D){ pInfo->extent.width, pInfo->extent.height, pInfo->extent.depth };

  VkImageUsageFlags usage = 0;

  switch (pInfo->usage)
  {
    case NOVA_GPU_TEXTURE_USAGE_SAMPLED_TEXTURE:
      usage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      break;
    case NOVA_GPU_TEXTURE_USAGE_COLOR_TEXTURE:
      usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
      break;
    case NOVA_GPU_TEXTURE_USAGE_DEPTH_TEXTURE:
      usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      break;
    case NOVA_GPU_TEXTURE_USAGE_STENCIL_TEXTURE:
      usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      break;
    case NOVA_GPU_TEXTURE_USAGE_STORAGE_TEXTURE:
      usage |= VK_IMAGE_USAGE_STORAGE_BIT;
      break;
    case NOVA_GPU_TEXTURE_USAGE_INPUT_ATTACHMENT:
      usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
      break;
    case NOVA_GPU_TEXTURE_USAGE_RESOLVE_TEXTURE:
      usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      break;
    case NOVA_GPU_TEXTURE_USAGE_TRANSIENT_ATTACHMENT:
      usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      break;
    case NOVA_GPU_TEXTURE_USAGE_PRESENTATION:
      usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
      break;
    default:
      NV_LOG_ERROR("Unknown texture usage: %u", pInfo->usage);
      break;
  }
  (*tex)->usage = usage;

  if (pInfo->usage & NOVA_GPU_TEXTURE_USAGE_PRESENTATION)
  {
    return;
  }

  VkImageCreateInfo imageCreateInfo = {};
  imageCreateInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType         = pInfo->type;
  imageCreateInfo.extent            = (VkExtent3D){ pInfo->extent.width, pInfo->extent.height, pInfo->extent.depth };
  imageCreateInfo.mipLevels         = pInfo->miplevels;
  imageCreateInfo.arrayLayers       = pInfo->arraylayers;
  imageCreateInfo.format            = NV_NVFormatToVKFormat(fmt);
  imageCreateInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.usage             = usage;
  imageCreateInfo.samples           = (VkSampleCountFlagBits)pInfo->samples;
  imageCreateInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
  NVVK_ResultCheck(vkCreateImage(device, &imageCreateInfo, NOVA_VK_ALLOCATOR, &(*tex)->image));
}

uint32_t
NV_NVFormatToVKFormat(NVFormat format)
{
  switch (format)
  {
    case NOVA_FORMAT_R8:
      return VK_FORMAT_R8_UNORM;
    case NOVA_FORMAT_RG8:
      return VK_FORMAT_R8G8_UNORM;
    case NOVA_FORMAT_RGB8:
      return VK_FORMAT_R8G8B8_UNORM;
    case NOVA_FORMAT_RGBA8:
      return VK_FORMAT_R8G8B8A8_UNORM;

    case NOVA_FORMAT_BGR8:
      return VK_FORMAT_B8G8R8_UNORM;
    case NOVA_FORMAT_BGRA8:
      return VK_FORMAT_B8G8R8A8_UNORM;

    case NOVA_FORMAT_RGB16:
      return VK_FORMAT_R16G16B16_UNORM;
    case NOVA_FORMAT_RGBA16:
      return VK_FORMAT_R16G16B16A16_UNORM;
    case NOVA_FORMAT_RG32:
      return VK_FORMAT_R32G32_SFLOAT;
    case NOVA_FORMAT_RGB32:
      return VK_FORMAT_R32G32B32_SFLOAT;
    case NOVA_FORMAT_RGBA32:
      return VK_FORMAT_R32G32B32A32_SFLOAT;

    case NOVA_FORMAT_R8_SINT:
      return VK_FORMAT_R8_SINT;
    case NOVA_FORMAT_RG8_SINT:
      return VK_FORMAT_R8G8_SINT;
    case NOVA_FORMAT_RGB8_SINT:
      return VK_FORMAT_R8G8B8_SINT;
    case NOVA_FORMAT_RGBA8_SINT:
      return VK_FORMAT_R8G8B8A8_SINT;

    case NOVA_FORMAT_R8_UINT:
      return VK_FORMAT_R8_UINT;
    case NOVA_FORMAT_RG8_UINT:
      return VK_FORMAT_R8G8_UINT;
    case NOVA_FORMAT_RGB8_UINT:
      return VK_FORMAT_R8G8B8_UINT;
    case NOVA_FORMAT_RGBA8_UINT:
      return VK_FORMAT_R8G8B8A8_UINT;

    case NOVA_FORMAT_R8_SRGB:
      return VK_FORMAT_R8_SRGB;
    case NOVA_FORMAT_RG8_SRGB:
      return VK_FORMAT_R8G8_SRGB;
    case NOVA_FORMAT_RGB8_SRGB:
      return VK_FORMAT_R8G8B8_SRGB;
    case NOVA_FORMAT_RGBA8_SRGB:
      return VK_FORMAT_R8G8B8A8_SRGB;

    case NOVA_FORMAT_BGR8_SRGB:
      return VK_FORMAT_B8G8R8_SRGB;
    case NOVA_FORMAT_BGRA8_SRGB:
      return VK_FORMAT_B8G8R8A8_SRGB;

    case NOVA_FORMAT_D16:
      return VK_FORMAT_D16_UNORM;
    case NOVA_FORMAT_D24:
      return VK_FORMAT_D24_UNORM_S8_UINT;
    case NOVA_FORMAT_D32:
      return VK_FORMAT_D32_SFLOAT;
    case NOVA_FORMAT_D24_S8:
      return VK_FORMAT_D24_UNORM_S8_UINT;
    case NOVA_FORMAT_D32_S8:
      return VK_FORMAT_D32_SFLOAT_S8_UINT;

    case NOVA_FORMAT_BC1:
      return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    case NOVA_FORMAT_BC3:
      return VK_FORMAT_BC3_UNORM_BLOCK;
    case NOVA_FORMAT_BC7:
      return VK_FORMAT_BC7_UNORM_BLOCK;

    default:
      return VK_FORMAT_UNDEFINED;
  }
}

NVFormat
NV_VKFormatToNVFormat(uint32_t format)
{
  switch (format)
  {
    case VK_FORMAT_R8_UNORM:
      return NOVA_FORMAT_R8;
    case VK_FORMAT_R8G8_UNORM:
      return NOVA_FORMAT_RG8;
    case VK_FORMAT_R8G8B8_UNORM:
      return NOVA_FORMAT_RGB8;
    case VK_FORMAT_R8G8B8A8_UNORM:
      return NOVA_FORMAT_RGBA8;

    case VK_FORMAT_B8G8R8_UNORM:
      return NOVA_FORMAT_BGR8;
    case VK_FORMAT_B8G8R8A8_UNORM:
      return NOVA_FORMAT_BGRA8;

    case VK_FORMAT_R16G16B16_UNORM:
      return NOVA_FORMAT_RGB16;
    case VK_FORMAT_R16G16B16A16_UNORM:
      return NOVA_FORMAT_RGBA16;
    case VK_FORMAT_R32G32_SFLOAT:
      return NOVA_FORMAT_RG32;
    case VK_FORMAT_R32G32B32_SFLOAT:
      return NOVA_FORMAT_RGB32;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
      return NOVA_FORMAT_RGBA32;

    case VK_FORMAT_R8_SINT:
      return NOVA_FORMAT_R8_SINT;
    case VK_FORMAT_R8G8_SINT:
      return NOVA_FORMAT_RG8_SINT;
    case VK_FORMAT_R8G8B8_SINT:
      return NOVA_FORMAT_RGB8_SINT;
    case VK_FORMAT_R8G8B8A8_SINT:
      return NOVA_FORMAT_RGBA8_SINT;

    case VK_FORMAT_R8_SRGB:
      return NOVA_FORMAT_R8_SRGB;
    case VK_FORMAT_R8G8_SRGB:
      return NOVA_FORMAT_RG8_SRGB;
    case VK_FORMAT_R8G8B8_SRGB:
      return NOVA_FORMAT_RGB8_SRGB;
    case VK_FORMAT_R8G8B8A8_SRGB:
      return NOVA_FORMAT_RGBA8_SRGB;

    case VK_FORMAT_B8G8R8_SRGB:
      return NOVA_FORMAT_BGR8_SRGB;
    case VK_FORMAT_B8G8R8A8_SRGB:
      return NOVA_FORMAT_BGRA8_SRGB;

    case VK_FORMAT_R8_UINT:
      return NOVA_FORMAT_R8_UINT;
    case VK_FORMAT_R8G8_UINT:
      return NOVA_FORMAT_RG8_UINT;
    case VK_FORMAT_R8G8B8_UINT:
      return NOVA_FORMAT_RGB8_UINT;
    case VK_FORMAT_R8G8B8A8_UINT:
      return NOVA_FORMAT_RGBA8_UINT;

    case VK_FORMAT_D16_UNORM:
      return NOVA_FORMAT_D16;
    case VK_FORMAT_D32_SFLOAT:
      return NOVA_FORMAT_D32;
    case VK_FORMAT_D24_UNORM_S8_UINT:
      return NOVA_FORMAT_D24_S8;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
      return NOVA_FORMAT_D32_S8;

    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
      return NOVA_FORMAT_BC1;
    case VK_FORMAT_BC3_UNORM_BLOCK:
      return NOVA_FORMAT_BC3;
    case VK_FORMAT_BC7_UNORM_BLOCK:
      return NOVA_FORMAT_BC7;

    default:
      return NOVA_FORMAT_UNDEFINED;
  }
}

// SPRITE
struct NV_sprite
{
  int                w, h;
  int                rcount;
  NVFormat           fmt;
  NV_GPU_Texture*    tex;
  NV_GPU_Memory*     mem;
  NV_GPU_Sampler*    sampler;
  NV_descriptor_set* set;
};

NV_sprite* NV_sprite_empty = NULL;

NV_sprite*
NV_sprite_load_from_memory(const unsigned char* data, int w, int h, NVFormat fmt)
{
  NV_sprite* spr                    = calloc(1, sizeof(struct NV_sprite));

  spr->rcount                       = 1;

  fmt                               = NV_VK_GetSupportedFormatForDraw(fmt);
  spr->fmt                          = fmt;

  NV_GPU_TextureCreateInfo tex_info = {
    .format      = fmt,
    .samples     = NOVA_SAMPLE_COUNT_1_SAMPLES,
    .type        = VK_IMAGE_TYPE_2D,
    .usage       = NOVA_GPU_TEXTURE_USAGE_SAMPLED_TEXTURE,
    .extent      = (NV_extent3D){ .width = w, .height = h, .depth = 1 },
    .arraylayers = 1,
    .miplevels   = 1,
  };
  NV_GPU_CreateTexture(&tex_info, &spr->tex);

  VkMemoryRequirements mem_req;
  vkGetImageMemoryRequirements(device, NV_GPU_TextureGet(spr->tex), &mem_req);
  NV_GPU_AllocateMemory(mem_req.size, NOVA_GPU_MEMORY_USAGE_CPU_VISIBLE, &spr->mem);
  NV_GPU_BindTextureToMemory(spr->mem, 0, spr->tex);

  const NVImage img = (const NVImage){ .w = w, .h = h, .fmt = fmt, .data = (unsigned char*)data };
  NV_GPU_WriteToTexture(spr->tex, &img);

  NV_GPU_SamplerCreateInfo sampler_info = {
    .filter         = VK_FILTER_NEAREST,
    .mipmap_mode    = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    .address_mode   = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .max_anisotropy = 1.0f,
    .mip_lod_bias   = 0.0f,
    .min_lod        = 0.0f,
    .max_lod        = VK_LOD_CLAMP_NONE,
  };
  NV_GPU_CreateSampler(&sampler_info, &spr->sampler);

  VkDescriptorSetLayoutBinding binding = (VkDescriptorSetLayoutBinding){
    .binding         = 0,
    .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
  };
  NV_AllocateDescriptorSet(&g_pool, &binding, 1, &spr->set);

  VkDescriptorImageInfo desc_img = {
    .sampler     = NV_GPU_SamplerGet(spr->sampler),
    .imageView   = NV_sprite_get_vk_image_view(spr),
    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };

  VkWriteDescriptorSet write = {
    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstSet          = spr->set->set,
    .dstBinding      = 0,
    .dstArrayElement = 0,
    .descriptorCount = 1,
    .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .pImageInfo      = &desc_img,
  };
  NV_descriptor_setSubmitWrite(spr->set, &write);

  return spr;
}

NV_sprite*
NV_sprite_load_from_disk(const char* path)
{
  NVImage    tex = NV_img_load(path);
  NV_sprite* spr = NV_sprite_load_from_memory(tex.data, tex.w, tex.h, tex.fmt);
  spr->rcount    = 1;
  NV_free(tex.data);
  return spr;
}

void
NV_sprite_destroy(NV_sprite* spr)
{
  NV_GPU_DestroyTexture(spr->tex);
  NV_GPU_FreeMemory(spr->mem);
}

void
NV_sprite_lock(NV_sprite* spr)
{
  spr->rcount++;
}

void
NV_sprite_release(NV_sprite* spr)
{
  spr->rcount--;
  if (spr->rcount <= 0)
  {
    NV_sprite_destroy(spr);
  }
}

void
NV_sprite_get_dimensions(const NV_sprite* spr, int* w, int* h)
{
  if (w)
  {
    *w = spr->w;
  }
  if (h)
  {
    *h = spr->h;
  }
}

VkImage
NV_sprite_get_vk_image(const NV_sprite* spr)
{
  return NV_GPU_TextureGet(spr->tex);
}

VkImageView
NV_sprite_get_vk_image_view(const NV_sprite* spr)
{
  return NV_GPU_TextureGetView(spr->tex);
}

VkDescriptorSet
NV_sprite_get_descriptor_set(const NV_sprite* spr)
{
  return spr->set->set;
}

VkSampler
NV_sprite_get_sampler(const NV_sprite* spr)
{
  return NV_GPU_SamplerGet(spr->sampler);
}

NVFormat
NV_sprite_get_format(const NV_sprite* spr)
{
  return spr->fmt;
}
// SPRITE

void
NV_camera_destroy(NV_camera_t* cam)
{
  // NV_descriptor_setDestroy(cam->sets);
  NV_GPU_DestroyBuffer(&cam->ub);
  NV_GPU_FreeMemory(cam->mem);
}

NV_camera_t
NV_camera_init()
{
  NV_camera_t camera = {
    .perspective = {},
    .ortho_size  = (vec2){ 10.0, 10.0 },
    .ortho       = m4ortho(-camera.ortho_size.x, camera.ortho_size.x, -camera.ortho_size.y, camera.ortho_size.y, 0.1f, 100.0f),
    .position    = (vec3){ 0.0f, 0.0f, 10.0f },
    .actual_pos  = (vec3){ 0.0f, 0.0f, 10.0f },
    .front       = (vec3){ 0.0f, 0.0f, 1.0f },
    .up          = (vec3){ 0.0f, 1.0f, 0.0f },
    .right       = (vec3){ 1.0f, 0.0f, 0.0f },

    // These angles should not be in radians because they're converted at update() time.
    .yaw       = -90.0,
    .pitch     = 0.0,
    .fov       = 90.0f,

    .near_clip = 0.1f,
    .far_clip  = 1000.0f,
  };

  VkPhysicalDeviceProperties phys_device_properties;
  vkGetPhysicalDeviceProperties(physDevice, &phys_device_properties);

  int ub_align = phys_device_properties.limits.minUniformBufferOffsetAlignment;

  // the size of a single uniform buffer.
  int ub_size = align_up(sizeof(camera_uniform_buffer), ub_align);

  NV_GPU_CreateBuffer(ub_size * CAMERA_FAKE_BUFFER_COUNT, ub_align, NOVA_GPU_BUFFER_TYPE_UNIFORM_BUFFER, &camera.ub);
  NV_GPU_AllocateMemory(ub_size * CAMERA_FAKE_BUFFER_COUNT, NOVA_GPU_MEMORY_USAGE_CPU_VISIBLE | NOVA_GPU_MEMORY_USAGE_CPU_WRITEABLE, &camera.mem);
  NV_GPU_BindBufferToMemory(camera.mem, 0, &camera.ub);

  VkDescriptorSetLayoutBinding bindings[] = { { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL } };
  NV_AllocateDescriptorSet(&g_pool, bindings, 1, &camera.sets);

  VkDescriptorBufferInfo bufferinfo = { .buffer = camera.ub.buffer, .offset = 0, .range = (VkDeviceSize)ub_size };
  VkWriteDescriptorSet   write      = {
           .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
           .dstSet          = camera.sets->set,
           .dstBinding      = 0,
           .descriptorCount = 1,
           .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
           .pBufferInfo     = &bufferinfo,
  };
  NV_descriptor_setSubmitWrite(camera.sets, &write);
  NV_GPU_MapMemory(camera.mem, VK_WHOLE_SIZE, 0, (void**)&camera.mem_mapped);
  return camera;
}

mat4
NV_camera_get_projection(NV_camera_t* cam)
{
  return cam->perspective;
}

mat4
NV_camera_get_view(NV_camera_t* cam)
{
  return cam->view;
}

vec3
NV_camera_get_up_vector(NV_camera_t* cam)
{
  return cam->up;
}

vec3
NV_camera_get_front_vector(NV_camera_t* cam)
{
  return cam->front;
}

void
NV_camera_rotate(NV_camera_t* cam, float yaw_, float pitch_)
{
  cam->yaw += yaw_;
  cam->pitch -= pitch_;

  cam->yaw          = fmodf(cam->yaw, 360.0f);

  const float bound = 89.9f;
  cam->pitch        = NVM_CLAMP(cam->pitch, -bound, bound);
}

void
NV_camera_move(NV_camera_t* cam, const vec3 amt)
{
  cam->actual_pos = v3add(cam->actual_pos, v3muls(cam->right, amt.x));
  cam->actual_pos = v3add(cam->actual_pos, v3muls(cam->up, amt.y));
  cam->actual_pos = v3add(cam->actual_pos, v3muls(cam->front, amt.z));
}

void
NV_camera_set_position(NV_camera_t* cam, const vec3 pos)
{
  cam->actual_pos = pos;
}

void
NV_camera_update(NV_camera_t* cam, struct NV_renderer_t* rd)
{
  const float yaw_rads = NVM_DEG2RAD(cam->yaw), pitch_rads = NVM_DEG2RAD(cam->pitch);
  const float cospitch = cosf(pitch_rads);
  vec3        new_front;
  new_front.x                                = cosf(yaw_rads) * cospitch;
  new_front.y                                = sinf(pitch_rads);
  new_front.z                                = sinf(yaw_rads) * cospitch;

  const vec3 world_up                        = (vec3){ 0.0f, 1.0f, 0.0f };

  cam->front                                 = v3normalize(new_front);
  cam->right                                 = v3normalize(v3cross(cam->front, world_up));
  cam->up                                    = v3normalize(v3cross(cam->right, cam->front));

  cam->view                                  = m4lookat(cam->actual_pos, v3add(cam->actual_pos, cam->front), cam->up);

  cam->position                              = cam->actual_pos;

  const NV_extent2d RenderExtent             = NV_renderer_get_render_extent(rd);
  const float       aspect                   = (float)RenderExtent.width / (float)RenderExtent.height;
  cam->perspective                           = m4perspective(cam->fov, aspect, cam->near_clip, cam->far_clip);

  camera_uniform_buffer ub                   = {};
  ub.perspective                             = cam->perspective;
  ub.ortho                                   = cam->ortho;
  ub.view                                    = cam->view;
  cam->mem_mapped[NV_renderer_get_frame(rd)] = ub;
}

vec2
NV_camera_get_global_mouse_position(const NV_camera_t* cam)
{
  vec2 ortho_pos = v2mulv(NV_input_get_mouse_position(), camera.ortho_size);
  return v2add(ortho_pos, (vec2){ cam->position.x, cam->position.y });
}

const char*
NVVK_vk_result_to_string(VkResult r)
{
  switch (r)
  {
    case VK_SUCCESS:
      return "VK_SUCCESS";
    case VK_NOT_READY:
      return "VK_NOT_READY";
    case VK_TIMEOUT:
      return "VK_TIMEOUT";
    case VK_EVENT_SET:
      return "VK_EVENT_SET";
    case VK_EVENT_RESET:
      return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
      return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
      return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
      return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
      return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
      return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
      return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
      return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
      return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN:
      return "VK_ERROR_UNKNOWN";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
      return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
      return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FRAGMENTATION:
      return "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
      return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_PIPELINE_COMPILE_REQUIRED:
      return "VK_PIPELINE_COMPILE_REQUIRED";
    case VK_ERROR_NOT_PERMITTED:
      return "VK_ERROR_NOT_PERMITTED";
    case VK_ERROR_SURFACE_LOST_KHR:
      return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
      return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:
      return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
      return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
      return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
      return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:
      return "VK_ERROR_INVALID_SHADER_NV";
    case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
      return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
      return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
      return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
      return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
      return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
      return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
      return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
      return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
    case VK_THREAD_IDLE_KHR:
      return "VK_THREAD_IDLE_KHR";
    case VK_THREAD_DONE_KHR:
      return "VK_THREAD_DONE_KHR";
    case VK_OPERATION_DEFERRED_KHR:
      return "VK_OPERATION_DEFERRED_KHR";
    case VK_OPERATION_NOT_DEFERRED_KHR:
      return "VK_OPERATION_NOT_DEFERRED_KHR";
    case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
      return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
      return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
    case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
      return "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
    case VK_PIPELINE_BINARY_MISSING_KHR:
      return "VK_PIPELINE_BINARY_MISSING_KHR";
    case VK_ERROR_NOT_ENOUGH_SPACE_KHR:
      return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
    case VK_RESULT_MAX_ENUM:
      return "VK_RESULT_MAX_ENUM";
    default:
      return "(Not A VkResult)";
  }
}