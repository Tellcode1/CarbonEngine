#ifndef __RENDERER__
#define __RENDERER__

#include "defines.h"
#include "stdafx.hpp"
#include "cengine.hpp"
#include "containers/cvector.hpp"
#include "containers/carray.hpp"
#include "containers/chashmap.hpp"
#include "cobject.hpp"

struct FrameRenderData
{
    VkImage         swapchainImage;
    VkImageView     swapchainImageView;
    VkFramebuffer   framebuffer;
    VkSemaphore     imageAvailableSemaphore;
    VkSemaphore     renderingFinishedSemaphore;
    VkFence         inFlightFence;
};

struct renderer_config
{
    VkSampleCountFlagBits  samples = VK_SAMPLE_COUNT_1_BIT;
    bool                   multisampling_enable = false;
    VkFormat               depth_buffer_format = VK_FORMAT_D32_SFLOAT;
    bool                   depth_buffer_enable = false;
    VkExtent2D             window_size = { 800, 600 };
    bool                   window_resizable = false;
    u8                     max_frames_in_flight = 1;
    SDL_Scancode           exit_key = SDL_SCANCODE_ESCAPE;
    VkPresentModeKHR       present_mode = VK_PRESENT_MODE_FIFO_KHR;
};

struct Renderer
{
    Renderer() = default;
    ~Renderer() = default;

    static const void *empty_array; // you can use this for empty offset buffers, etc. it has 256 bytes of memory
    static renderer_config config;

    static VkSwapchainKHR swapchain;
    static VkCommandPool commandPool;

    static u32 attachment_count;
    static u32 renderer_frame;
    static u32 imageIndex;

    static VkImage color_image;
    static VkDeviceMemory color_image_memory;
    static VkImageView color_image_view;

    static cvector<FrameRenderData> renderData;
    static cvector<VkCommandBuffer> drawBuffers;
    static chashmap<cobject_base *, u32> obj_offsets;

    static void initialize(const renderer_config *conf);

    static VkCommandBuffer CARBON_FORCE_INLINE GetDrawBuffer() { return drawBuffers.at(renderer_frame); }
    static bool BeginRender();
    static void EndRender();
    // static void render(cobject_sprite_renderer *render);

    private:
    static void _SignalResize();
};

#endif