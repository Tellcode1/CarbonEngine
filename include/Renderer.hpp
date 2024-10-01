#ifndef __RENDERER__
#define __RENDERER__

#include "defines.h"
#include "stdafx.h"
#include "vkstdafx.h"

#include "../external/volk/volk.h"

#include "cengine.hpp"
#include "containers/cvector.h"

#include "engine/camera.hpp"

#include <SDL2/SDL.h>

// The camera should also own the output texture, etc.
// Basically, the camera should be the first parameter that the renderer needs.
// Something like render_begin( renderer, camera )
// Btw this struct is for testing, this isn't an actual camera.
// I'll get to that when I switch to making the game object, transforms, etc.
extern ccamera camera;

// Move ownership to camera VV
struct FrameRenderData
{
    VkImage         swapchainImage;
    VkImageView     swapchainImageView;
    VkFramebuffer   color_framebuffer;
    VkImage         depth_image;
    VkImageView     depth_image_view;
    VkDeviceMemory  shadow_image_memory; // FIXME: move to Renderer struct to allocate all at once
    VkSemaphore     imageAvailableSemaphore;
    VkSemaphore     renderingFinishedSemaphore;
    VkFence         inFlightFence;
};

enum buffering_mode {
    BUFFER_MODE_NONE = 0,
    BUFFER_MODE_DOUBLE_BUFFERED = 1,
    BUFFER_MODE_TRIPLE_BUFFERED = 2,
};

struct renderer_config
{
    VkSampleCountFlagBits  samples = VK_SAMPLE_COUNT_1_BIT;
    bool                   multisampling_enable = false;
    VkExtent2D             window_size = { 800, 600 };
    bool                   window_resizable = false;
    // u8                     max_frames_in_flight = 1;
    enum buffering_mode    buffer_mode;
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

    static VkFormat depth_buffer_format;

    static cvector_t *renderData;
    static cvector_t *drawBuffers;

    static void initialize(const renderer_config *conf);

    static VkCommandBuffer CARBON_FORCE_INLINE GetDrawBuffer() { return *(VkCommandBuffer *)cvector_get(drawBuffers, renderer_frame); }
    static bool BeginRender();
    static void EndRender();

    static void _create_optional_images();
    static void _create_framebuffers_and_swapchain_image_views();
    static void _SignalResize();
};

#endif